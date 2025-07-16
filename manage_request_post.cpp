#include "manage_request_post.hpp"

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

std::string handle_post_upload_multipart(const std::string &dir,
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

//-------------manage post_urlencoded

std::string url_decode(const std::string &str)
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

std::map<std::string, std::string> parse_urlencoded_body(const std::string &body)
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

std::string handle_post_urlencoded(const std::string &body)
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
}
