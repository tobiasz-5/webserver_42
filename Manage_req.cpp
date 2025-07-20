
#include "Manage_req.hpp"

void Manage_req::set_response_for_client(Client &c)
{
    std::string uri = c.getRequest().getUri();
    std::string m   = c.getRequest().getMethod();
    const route *match = NULL;
    size_t best = 0;

    for (size_t i = 0; i < c.getServer()->getRoutesSize(); ++i) 
    {
        const route &r = c.getServer()->getRoute(i);
        bool ok = false;
        if (uri == r.uri) 
            ok = true;
        else if (r.uri != "/" && uri.find(r.uri + "/") == 0) 
            ok = true;
        else if (r.uri == "/" && best == 0) 
            ok = true;
        if (ok && r.uri.length() > best) 
        {
            match = &r;
            best = r.uri.length();
        }
    }
    if (match)
        c.set_response(handle_request(uri, *match, m, c, *c.getServer()));
    else
        c.set_response(generate_error_response(404, *c.getServer()));
}

std::string Manage_req::handle_request(std::string uri, const route &rt, std::string method, const Client &cli, const Server &srv)
{
    bool allowed = std::find(rt.allowed_methods.begin(), rt.allowed_methods.end(), method) != rt.allowed_methods.end();
    if (!allowed)
        return generate_error_response(405, srv);
    if (!rt.redirect.empty())
        return "HTTP/1.1 301 Moved Permanently\r\nLocation: " + rt.redirect + "\r\n\r\n";
    debug_message(uri, rt, "", "", method, allowed, rt.upload_path);
    std::string pathOnly = uri.substr(0, uri.find('?'));//estrae il path dalla uri escludendo eventuali query string ?x=42
	if (isCgiRequest(pathOnly, rt))
    {
		return handle_cgi(pathOnly, rt, method, cli, srv);
    }
    if (method == "POST")
    {
		std::string body = cli.getRequest().getBody();
		std::stringstream response;
		response << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "<< body.size() << "\r\n\r\n" << body;
		return response.str();
    }
    std::string rel = uri.substr(rt.uri.length());
    if (rel.empty() || rel == "/") 
        rel = "/" + rt.default_file;
    std::string filePath = rt.root_directory + rel;
    debug_message(uri, rt, rel, filePath, method, allowed, rt.upload_path);
    struct stat fs;
    if (stat(filePath.c_str(), &fs) == -1)
        return generate_error_response(404, srv);
    if (S_ISDIR(fs.st_mode))
    {
		return handle_directory(uri, filePath, rt, srv);
    }
    if (method == "GET") 
    {
        std::ifstream file(filePath.c_str(), std::ios::binary);

        if (!file.is_open()) 
            return generate_error_response(404, srv);
        std::stringstream buf; buf << file.rdbuf();
        std::string body = buf.str();
        std::stringstream resp;

        std::string content_type = get_type(filePath);
        resp << "HTTP/1.1 200 OK\r\nContent-Type: " << content_type << "\r\nContent-Length: " << body.length() << "\r\nConnection: keep-alive\r\n\r\n" << body;
        return resp.str();
    }
    if (method == "DELETE") 
    {
        struct stat fs;
        if (stat(filePath.c_str(), &fs) == -1)
            return generate_error_response(404, srv); // File non trovato
        if (S_ISDIR(fs.st_mode))
            return generate_error_response(403, srv); // È una directory: opzionalmente potresti rifiutare o supportare la cancellazione ricorsiva
        if (remove(filePath.c_str()) == 0)
            return "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n"; // Risposta standard 204 No Content
		else
            return generate_error_response(403, srv); // Errore cancellazione: probabilmente permessi
    }
    return generate_error_response(400, srv);
}

// ---------CGI handling------------------------

bool Manage_req::isCgiRequest(const std::string& uri, const route& r)
{
    size_t dot = uri.rfind('.'); //cerca il . partendo da destra
    if (dot == std::string::npos) 
        return false;
    std::string ext = uri.substr(dot); //estrae l estensione
	bool ret = std::find(r.cgi_extensions.begin(), r.cgi_extensions.end(), ext) != r.cgi_extensions.end();
    return ret; //ritorna true se trova ext nelle cgi_extensions , altrimenti find trova end e ritorna false
}

std::string Manage_req::handle_cgi(const std::string& pathOnly, const route& rt, const std::string& method, const Client& cli, const Server& srv)
{
    std::string bodyIn;
	if (method == "POST")
		bodyIn = cli.getRequest().getBody();
	else
		bodyIn = "";
    std::string rel = pathOnly.substr(rt.uri.length()); 
    if (rel.empty() || rel == "/") 
        rel = "/" + rt.default_file; 
    std::string script = rt.root_directory + rel;
    struct stat sb; 
    if (stat(script.c_str(), &sb) == -1 || !S_ISREG(sb.st_mode)) 
        return generate_error_response(404, srv);
    std::string raw = runCgi(cli, rt, script, bodyIn);  //output from cgi in raw
    size_t hdrEnd = raw.find("\r\n\r\n");
    if (hdrEnd == std::string::npos)
    {
        std::stringstream res; 
        res << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " << raw.size() << "\r\n\r\n" << raw;
        return res.str();
    }
    std::string hdr = raw.substr(0, hdrEnd); 
    std::string bodyOut = raw.substr(hdrEnd + 4); 
    std::string statusLine = "HTTP/1.1 200 OK";
    if (hdr.compare(0, 7, "Status:") == 0)
    {
        size_t eol = hdr.find("\r\n");
        std::string sVal = hdr.substr(7, eol - 7); 
        hdr.erase(0, eol + 2);               
        statusLine = "HTTP/1.1 " + sVal;      
    }
    if (hdr.find("Content-Length:") == std::string::npos)
    {
        std::ostringstream extra;
        extra << "\r\nContent-Length: " << bodyOut.size(); 
        hdr += extra.str();
    }
    std::ostringstream response;
    response << statusLine << "\r\n" << hdr << "\r\n\r\n" << bodyOut;
    return response.str();
}

std::string Manage_req::runCgi(const Client& cli, const route&  r, const std::string& scriptPathRel, const std::string& body)
{
    char absBuf[PATH_MAX];
    if (!realpath(scriptPathRel.c_str(), absBuf))
        return "Status: 404 Not Found\r\n"
               "Content-Type: text/plain\r\n\r\n"
               "CGI script not found";

    std::string scriptAbs(absBuf);
    int inPipe[2], outPipe[2];
    if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
        throw std::runtime_error("pipe failed");
    pid_t pid = fork();
    if (pid == -1)
        throw std::runtime_error("fork failed");
    if (pid == 0) // figlio
    {
        if (chdir(r.root_directory.c_str()) == -1)
            _exit(1);

        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        close(inPipe[1]);
        close(outPipe[0]);

        const char* dot = strrchr(scriptAbs.c_str(), '.');
        bool isPhp = (dot && std::strcmp(dot, ".php") == 0);

        char* argvPhp[] = {
            const_cast<char*>(r.cgi_path.c_str()),
            const_cast<char*>(scriptAbs.c_str()),
            NULL
        };
        char* argvOther[] = {
            const_cast<char*>(r.cgi_path.c_str()),
            const_cast<char*>(scriptAbs.c_str()),
            NULL
        };
        char** argv = isPhp ? argvPhp : argvOther;

        std::vector<char*> env = buildEnv(cli, scriptAbs, body);

        execve(r.cgi_path.c_str(), argv, &env[0]);
        _exit(1);
    }

    // padre
    close(inPipe[0]);
    close(outPipe[1]);
    if (!body.empty())
        write(inPipe[1], body.data(), body.size());
    close(inPipe[1]);
    const int timeout_ms = 5000;
    int waited_ms = 0;
    const int sleep_interval_ms = 100;

    int status;
    bool timed_out = false;
    while (true)
	{
        pid_t ret = waitpid(pid, &status, WNOHANG);
        if (ret == -1)
            break; // errore waitpid
		else if (ret > 0)
            break; // figlio terminato
        if (waited_ms >= timeout_ms)
		{
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            timed_out = true;
            break;
        }
        usleep(sleep_interval_ms * 1000);
        waited_ms += sleep_interval_ms;
    }
    std::string output;
    if (!timed_out)
	{
        char buf[4096];
        ssize_t n;
        while ((n = read(outPipe[0], buf, sizeof(buf))) > 0)
            output.append(buf, n);
    }
	else
        output = "Status: 504 Gateway Timeout\r\nContent-Type: text/plain\r\n\r\nCGI script timed out";
    close(outPipe[0]);
    return output;
}

std::vector<char*> Manage_req::buildEnv(const Client& cli, const std::string& scriptPath, const std::string& body)
{
    const Request& req = cli.getRequest(); 
    std::vector<std::string> temp;

    //costruire l ambiente (temporaneo) per ogni processo/esecuzione-di-cgi
    //ovvero alcune variabili cgi standard da passare all'interprete
    temp.push_back("REQUEST_METHOD="  + req.getMethod());
    temp.push_back("SCRIPT_FILENAME=" + scriptPath);
    temp.push_back("PATH_INFO="       + scriptPath);

    std::string uri = req.getUri();
    size_t q = uri.find('?');
    temp.push_back("QUERY_STRING=" + (q == std::string::npos ? "" : uri.substr(q + 1))); //se c'e estrae la query string altrimenti ""

    if (!body.empty())
        temp.push_back("CONTENT_LENGTH=" + to_stringgg(body.size())); //se c'e un body lo trasforma in numero e lo aggiunge a content legnth
    std::string ctype = req.getHeader("Content-Type");
    if (!ctype.empty())
        temp.push_back("CONTENT_TYPE=" + ctype);//aggiunge il content type

    temp.push_back("SERVER_PROTOCOL=HTTP/1.1");
    temp.push_back("SERVER_NAME=" + req.getHeader("Host"));

    std::vector<char*> env;
    for (size_t i = 0; i < temp.size(); ++i)
        env.push_back(strdup(temp[i].c_str())); //riempi il vettore di strnighe
    env.push_back(strdup("REDIRECT_STATUS=200")); //aggiunge in fondo quest ultiam variabile d ambienete per il cgi
    env.push_back(NULL);//null termina
    return env; //ritorna il vettore di stringhe da passare ad execvue
}
//---handle if directory---------------

std::string Manage_req::handle_directory(const std::string& uri, const std::string& filePath, const route& rt, const Server& srv)
{
	std::string indexPath = filePath + "/" + rt.default_file;
    struct stat st2;
    if (stat(indexPath.c_str(), &st2) == 0 && !S_ISDIR(st2.st_mode))
    {
        std::ifstream f(indexPath.c_str(), std::ios::binary);
        if (!f.is_open()) 
			return generate_error_response(404, srv);
        std::stringstream buf; buf << f.rdbuf();
        std::string body = buf.str();
        std::stringstream res;
        res << "HTTP/1.1 200 OK\r\nContent-Length: " << body.size()
			<< "\r\nContent-Type: text/plain\r\n\r\n" << body;
		return res.str();
    }
    if (rt.directory_listing)
    {
        DIR *d = opendir(filePath.c_str());
        if (!d) 
            return generate_error_response(500, srv);
        std::stringstream body;
        body << "<html><body><h1>Index of " << uri << "</h1><ul>";
        struct dirent *e;
        while ((e = readdir(d)))
            body << "<li><a href=\"" << uri << "/" << e->d_name << "\">" << e->d_name << "</a></li>";
        body << "</ul></body></html>";
        closedir(d);

        std::stringstream res;
        res << "HTTP/1.1 200 OK\r\nContent-Length: " << body.str().size() << "\r\nContent-Type: text/html\r\n\r\n" << body.str();
        return res.str();
    }
    return generate_error_response(404, srv);
}

//----------error message and debug message-------------------------------

std::string Manage_req::generate_error_response(int code, const Server &serv)
{
    std::map<int, std::string>::const_iterator it = serv.getError_pages().find(code);
    if (it != serv.getError_pages().end())
    {
        std::ifstream file(it->second.c_str());
        if (file.is_open())
        {
            std::stringstream buf; buf << file.rdbuf();
            std::string body = buf.str();
            std::stringstream res;
            res << "HTTP/1.1 " << code << " \r\n"
                << "Content-Type: text/html\r\n"
                << "Content-Length: " << body.length() << "\r\n"
                << "Connection: close\r\n\r\n" << body;
            return res.str();
        }
    }
    std::string msg;
    switch (code) {
        case 404: msg = "404 Not Found"; break;
        case 403: msg = "403 Forbidden"; break;
        case 500: msg = "500 Internal Server Error"; break;
        case 405: msg = "405 Method Not Allowed"; break;
        case 400: msg = "400 Bad Request"; break;
        case 301: msg = "301 Moved Permanently"; break;
        case 201: msg = "201 Created"; break;
        default : msg = to_stringgg(code) + " Error"; break;
    }
    std::string body = "<html><head><title>"+msg+"</title></head><body><h1>"+msg+"</h1></body></html>";
    std::stringstream res;
    res << "HTTP/1.1 " << code << ' ' << msg << "\r\n"
        << "Content-Type: text/html\r\n"
        << "Content-Length: " << body.length() << "\r\n"
        << "Connection: close\r\n\r\n" << body;
    return res.str();
}

void Manage_req::debug_message(const std::string &uri, const route &matched_route, const std::string &relativePath, const std::string &filePath, const std::string &requested_method, bool methodAllowed, const std::string &upload_path)
{
    std::cout << "\n\033[38;5;154m=========== MANAGE REQUEST ===========\n";
    std::cout << "Requested method: " << requested_method << "\n";
    std::cout << "URI received: "    << uri               << "\n";
    std::cout << "Route URI: "       << matched_route.uri << "\n";
    std::cout << "Root directory: "  << matched_route.root_directory << "\n";
    std::cout << "Relative path: "   << relativePath      << "\n";
    std::cout << "Final path: "      << filePath          << "\n";
    std::cout << "Upload path: "     << upload_path       << "\n";
	if (methodAllowed)
		std::cout << "Method allowed: yes\n";
	else
		std::cout << "Method allowed: no\n";
    std::cout << "===============================================\033[0m\n\n";
}

//----------------------------------------------------------------------

//old POST handling
/*

std::string Manage_req::upload_response(bool ok, const std::string& msg)
{
    std::string body = "<html><body><h1>";
    if (ok)
        body += "Upload Success";
    else
        body += "Upload Failed";
    body += "</h1><p>" + msg + "</p></body></html>";

    std::stringstream res;
    if (ok)
        res << "HTTP/1.1 201 Created\r\n";
    else
        res << "HTTP/1.1 500 Internal Server Error\r\n";

    res << "Content-Type: text/html\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << body;

    return res.str();
}

std::string Manage_req::unique_filename(const std::string &dir)
{
    char buf[20];
    std::time_t t = std::time(NULL);
    std::tm *tm_ptr = std::localtime(&t);
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", tm_ptr);
    std::stringstream ss; ss << dir << "/upload_" << buf << ".dat";
    return ss.str();
}

std::string Manage_req::extract_boundary(const std::string &ctype)
{
    const std::string key = "boundary=";
    size_t pos = ctype.find(key);
    if (pos == std::string::npos) 
        return "";
    std::string b = ctype.substr(pos + key.size());
    if (!b.empty() && b[0] == '"') 
        b = b.substr(1);
    b.erase(0, b.find_first_not_of(" \t\r\n"));
    b.erase(b.find_last_not_of(" \t\r\n") + 1);
    return b;
}

std::string Manage_req::handle_post_upload_multipart(const std::string &dir, const std::string &body, const std::string &ctype)
{
    std::string boundary = extract_boundary(ctype);
    if (boundary.empty())
        return upload_response(false, "Missing boundary");

    size_t file_start = body.find("filename=");
    if (file_start == std::string::npos)
        return upload_response(false, "No file in body");
    size_t data_start = body.find("\r\n\r\n", file_start);
    if (data_start == std::string::npos) 
        return upload_response(false, "Malformed body");
    data_start += 4;
    size_t data_end = body.find(boundary, data_start);
    if (data_end == std::string::npos)
        return upload_response(false, "No closing boundary");

    std::string file_data = body.substr(data_start, data_end - data_start - 2);

    struct stat st; 
    if (stat(dir.c_str(), &st) == -1)
    {
        if (mkdir(dir.c_str(), 0755) == -1)
            return upload_response(false, "Cannot create dir");
    }
    std::string fname = unique_filename(dir);
    std::ofstream out(fname.c_str(), std::ios::binary);
    if (!out.is_open())
        return upload_response(false, "Cannot write file");
    out.write(file_data.c_str(), file_data.size()); out.close();
    return upload_response(true, "Saved as " + fname);
}

//-------------manage post_urlencoded

std::string Manage_req::url_decode(const std::string &str)
{
    std::string decoded;
    char ch;
    size_t i, ii;
    for (i=0; i<str.length(); i++)
	{
        if (str[i] == '+')
		{
            decoded += ' ';
        }
        else if (str[i] == '%' && i + 2 < str.length())
		{
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> ii)
			{
                ch = static_cast<char>(ii);
                decoded += ch;
                i += 2;
            }
            else
			{
                decoded += '%'; // malformed, lascia così
            }
        }
        else
		{
            decoded += str[i];
        }
    }
    return decoded;
}

std::map<std::string, std::string> Manage_req::parse_urlencoded_body(const std::string &body)
{
    std::map<std::string, std::string> params;
    std::string key, value;
    std::istringstream ss(body);
    std::string pair;

    while (std::getline(ss, pair, '&'))
	{
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos)
		{
            key = url_decode(pair.substr(0, eq_pos));
            value = url_decode(pair.substr(eq_pos + 1));
            params[key] = value;
        }
    }
    return params;
}

std::string Manage_req::handle_post_urlencoded(const std::string &body)
{
    std::map<std::string, std::string> params = parse_urlencoded_body(body);
    // Ad esempio fai una risposta semplice:
    std::stringstream response_body;
    response_body << "<html><body><h1>Received form data</h1><ul>";
    for (std::map<std::string, std::string>::iterator it = params.begin(); it != params.end(); ++it)
	{
        response_body << "<li>" << it->first << ": " << it->second << "</li>";
    }
    response_body << "</ul></body></html>";

    std::stringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << response_body.str().length() << "\r\n"
             << "Connection: close\r\n\r\n"
             << response_body.str();
    return response.str();
}*/