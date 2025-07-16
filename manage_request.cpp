#include "Server.hpp"
#include "Client.hpp"
#include "manage_request.hpp"
#include "utils.hpp"
#include "cgi_utils.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include <fstream>
#include <sstream>
#include <algorithm>

void debug_message(const std::string &uri, const route &matched_route,
                   const std::string &relativePath, const std::string &filePath,
                   const std::string &requested_method, bool methodAllowed,
                   const std::string &upload_path)
{
    std::cout << "\n\033[38;5;154m=========== MANAGE REQUEST ===========\n";
    std::cout << "Requested method: " << requested_method << "\n";
    std::cout << "URI received: "    << uri               << "\n";
    std::cout << "Route URI: "       << matched_route.uri << "\n";
    std::cout << "Root directory: "  << matched_route.root_directory << "\n";
    std::cout << "Relative path: "   << relativePath      << "\n";
    std::cout << "Final path: "      << filePath          << "\n";
    std::cout << "Upload path: "     << upload_path       << "\n";
    std::cout << "Method allowed: "  << (methodAllowed ? "yes" : "no") << "\n";
    std::cout << "===============================================\033[0m\n\n";
}

std::string generate_error_response(int code, const Server &serv)
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

static std::string upload_response(bool ok, const std::string &msg)
{
    std::string body = "<html><body><h1>";
    body += (ok ? "Upload Success" : "Upload Failed");
    body += "</h1><p>" + msg + "</p></body></html>";
    std::stringstream res;
    res << (ok ? "HTTP/1.1 201 Created" : "HTTP/1.1 500 Internal Server Error")
        << "\r\nContent-Type: text/html\r\nContent-Length: " << body.size()
        << "\r\nConnection: close\r\n\r\n" << body;
    return res.str();
}

static std::string unique_filename(const std::string &dir)
{
    char buf[20];
    std::time_t t = std::time(NULL);
    std::tm *tm_ptr = std::localtime(&t);
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", tm_ptr);
    std::stringstream ss; ss << dir << "/upload_" << buf << ".dat";
    return ss.str();
}

static std::string extract_boundary(const std::string &ctype)
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

static std::string handle_post_upload_multipart(const std::string &dir,
                                                const std::string &body,
                                                const std::string &ctype)
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

std::string handle_request(std::string uri, const route &rt,
                           std::string method,
                           const Client &cli, const Server &srv)
{
    bool allowed = std::find(rt.allowed_methods.begin(), rt.allowed_methods.end(), method) != rt.allowed_methods.end();
    if (!allowed) 
        return generate_error_response(405, srv);

    if (!rt.redirect.empty())
        return "HTTP/1.1 301 Moved Permanently\r\nLocation: " + rt.redirect + "\r\n\r\n";

    debug_message(uri, rt, "", "", method, allowed, rt.upload_path);

    std::string pathOnly = uri.substr(0, uri.find('?'));
    if (isCgiRequest(pathOnly, rt))
    {
        std::string bodyIn = (method == "POST") ? cli.getRequest().getBody() : "";
        std::string rel = pathOnly.substr(rt.uri.length());
        if (rel.empty() || rel == "/") rel = "/" + rt.default_file;
        std::string script = rt.root_directory + rel;

        struct stat sb;
        if (stat(script.c_str(), &sb) == -1 || !S_ISREG(sb.st_mode))
            return generate_error_response(404, srv);

        std::string raw = runCgi(cli, rt, script, bodyIn);

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
    if (method == "POST") 
    {
        if (rt.upload_path.empty())
            return generate_error_response(500, srv);
        return handle_post_upload_multipart(rt.upload_path,
                                            cli.getRequest().getBody(),
                                            cli.getRequest().getHeader("Content-Type"));
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


    if (method == "GET") 
    {
        std::ifstream file(filePath.c_str(), std::ios::binary);

        if (!file.is_open()) 
            return generate_error_response(404, srv);
        std::stringstream buf; buf << file.rdbuf();
        std::string body = buf.str();
        std::stringstream resp;
        resp << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << body.length() << "\r\nConnection: keep-alive\r\n\r\n" << body;
        return resp.str();
    }

    if (method == "DELETE") 
    {
        if (remove(filePath.c_str()) == 0)
            return "HTTP/1.1 200 OK\r\n\r\nFile deleted successfully.";
        return generate_error_response(500, srv);
    }

    return generate_error_response(400, srv);
}  

void set_response_for_client(Client &c)
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

