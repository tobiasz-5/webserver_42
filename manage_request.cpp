#include "Server.hpp"
#include "Client.hpp"
#include "manage_request.hpp"
#include "utils.hpp"

void debug_message(const std::string &uri, const route &matched_route, const std::string &relativePath, const std::string &filePath, const std::string &requested_method, bool methodAllowed, const std::string &upload_path)
{
    std::cout << "\n\033[38;5;154m=========== MANAGE REQUEST ===========\n";
    std::cout << "Requested method: " << requested_method << "\n";
    std::cout << "URI received: " << uri << "\n";
    std::cout << "Route URI: " << matched_route.uri << "\n";
    std::cout << "Root directory: " << matched_route.root_directory << "\n";
    std::cout << "Relative path: " << relativePath << "\n";
    std::cout << "Final path: " << filePath << "\n";
	std::cout << "Upload path: " << upload_path << "\n";
    std::cout << "Method allowed: " << (methodAllowed ? "yes" : "no") << "\n";
    std::cout << "===============================================\033[0m\n\n";
}

std::string generate_error_response(int code, const Server &serv)
{
    std::map<int, std::string>::const_iterator it = serv.getError_pages().find(code);
    if (it != serv.getError_pages().end()) // page found
    {
        std::cout << it->second.c_str() << std::endl;
		std::ifstream file(it->second.c_str());
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string body = buffer.str();
            std::stringstream response;
            response << "HTTP/1.1 " << code << " \r\n"
                     << "Content-Type: text/html\r\n"
                     << "Content-Length: " << body.length() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << body;
            return response.str();
        }
		else
		{
			std::cerr << "❌ Failed to open error file: " << strerror(errno) << std::endl;
		}
    }
	std::string error_message;
	switch (code)
	{
		case 404: error_message = "404 Not Found"; break;
		case 403: error_message = "403 Forbidden"; break;
		case 500: error_message = "500 Internal Server Error"; break;
		case 405: error_message = "405 Method Not Allowed"; break;
		case 400: error_message = "400 Bad Request"; break;
		case 301: error_message = "301 Moved Permanently"; break;
		case 201: error_message = "201 Created"; break;
		default:  error_message = to_stringgg(code) + " Error"; break;
	}
	std::string body = "<html><head><title>" + error_message + "</title></head>"
                   "<body><h1>" + error_message + "</h1></body></html>";
	std::stringstream fallback;
	fallback << "HTTP/1.1 " << code << " " << error_message << "\r\n"
			<< "Content-Type: text/html\r\n"
			<< "Content-Length: " << body.length() << "\r\n"
			<< "Connection: close\r\n\r\n"
			<< body;
    return fallback.str();
}

std::string generate_upload_response(bool success, const std::string& message)
{
    std::stringstream response;
    if (success)
    {
        std::string body = "<html><body><h1>Upload Success</h1><p>" + message + "</p></body></html>";
        response << "HTTP/1.1 201 Created\r\n"
                 << "Content-Type: text/html\r\n"
                 << "Content-Length: " << body.size() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << body;
    }
    else
    {
        std::string body = "<html><body><h1>Upload Failed</h1><p>" + message + "</p></body></html>";
        response << "HTTP/1.1 500 Internal Server Error\r\n"
                 << "Content-Type: text/html\r\n"
                 << "Content-Length: " << body.size() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << body;
    }
    return response.str();
}

std::string generate_unique_filename(const std::string& uploadDir)
{
    char buffer[20];
    std::time_t t = std::time(NULL);
    std::tm* tm_ptr = std::localtime(&t);
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", tm_ptr);
    std::stringstream filename;
    filename << uploadDir << "/upload_" << buffer << ".dat";
    return filename.str();
}

std::string extract_boundary(const std::string& content_type)
{
    std::string boundary_prefix = "boundary=";
    std::size_t pos = content_type.find(boundary_prefix);
    if (pos == std::string::npos)
        return "";

    std::string boundary = content_type.substr(pos + boundary_prefix.length());

    // Rimuove virgolette se presenti
    if (!boundary.empty() && boundary[0] == '"')
        boundary = boundary.substr(1, boundary.length());

    // Rimuove eventuali spazi (trim leading/trailing)
    boundary.erase(0, boundary.find_first_not_of(" \t\r\n"));
    boundary.erase(boundary.find_last_not_of(" \t\r\n") + 1);

    return boundary;
}

std::string handle_post_upload_multipart(const std::string& uploadDir, const std::string& body, const std::string& content_type)
{
    std::string boundary = extract_boundary(content_type);

	std::cout << "bounday trovato:" << boundary << std::endl;

    if (boundary.empty())
        return generate_upload_response(false, "Missing boundary in Content-Type");
    size_t file_start = body.find("filename=");
    if (file_start == std::string::npos)
        return generate_upload_response(false, "No file found in multipart body");
    size_t data_start = body.find("\r\n\r\n", file_start); // Trova inizio vero dei dati (dopo 2 CRLF)
    if (data_start == std::string::npos)
        return generate_upload_response(false, "Malformed multipart body");
    data_start += 4; // skip \r\n\r\n

	std::cout << "data start:" << data_start << std::endl;
	std::cout << "bounday trovato:" << boundary << "FINE" << std::endl;
	size_t data_end = body.find(boundary, data_start);

	std::cout << "data end:" << data_end << std::endl;

    if (data_end == std::string::npos)
        return generate_upload_response(false, "No closing boundary");
    std::string file_data = body.substr(data_start, data_end - data_start - 2); // remove final \r\n
    struct stat st; // Salva il file
	memset(&st, 0, sizeof(st));
    if (stat(uploadDir.c_str(), &st) == -1 && mkdir(uploadDir.c_str(), 0755) == -1)
        return generate_upload_response(false, "Cannot create upload directory.");
    std::string filename = generate_unique_filename(uploadDir);
    std::ofstream outfile(filename.c_str(), std::ios::binary);
    if (!outfile.is_open())
        return generate_upload_response(false, "Cannot open file for writing.");
    outfile.write(file_data.c_str(), file_data.size());
    outfile.close();
    return generate_upload_response(true, "File saved as " + filename);
}

std::string handle_request(std::string uri, const route &matched_route, std::string requested_method, const Client &client, const Server &server)
{
    // Controlla se il metodo è permesso
    bool methodAllowed = std::find(matched_route.allowed_methods.begin(), matched_route.allowed_methods.end(), requested_method) != matched_route.allowed_methods.end();
    if (!methodAllowed)
        return generate_error_response(405, server);

    // Redirect, se presente
    if (!matched_route.redirect.empty())
    {
        return "HTTP/1.1 301 Moved Permanently\r\n"
               "Location: " + matched_route.redirect + "\r\n\r\n";
    }

    debug_message(uri, matched_route, "", "", requested_method, methodAllowed, matched_route.upload_path);

    // Gestione POST - Upload file
    if (requested_method == "POST")
    {
        if (matched_route.upload_path.empty())
            return generate_error_response(500, server);

        std::string body = client.getRequest().getBody();
        std::string content_type = client.getRequest().getHeader("Content-Type");
        return handle_post_upload_multipart(matched_route.upload_path, body, content_type);
    }

    // Mapping URI su filesystem
    std::string relativePath = uri.substr(matched_route.uri.length());
    if (relativePath.empty() || relativePath == "/")
        relativePath = "/" + matched_route.default_file;
    std::string filePath = matched_route.root_directory + relativePath;

    debug_message(uri, matched_route, relativePath, filePath, requested_method, methodAllowed, matched_route.upload_path);

    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) == -1)
        return generate_error_response(404, server);

    // Se è una directory
    if (S_ISDIR(fileStat.st_mode))
    {
        if (!matched_route.directory_listing)
            return generate_error_response(403, server);

        DIR *dir = opendir(filePath.c_str());
        if (!dir)
            return generate_error_response(500, server);

        std::stringstream body;
        body << "<html><body><h1>Index of " << uri << "</h1><ul>";
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            body << "<li><a href=\"" << uri << "/" << entry->d_name << "\">" << entry->d_name << "</a></li>";
        }
        body << "</ul></body></html>";
        closedir(dir);

        std::stringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: text/html\r\n"
                 << "Content-Length: " << body.str().length() << "\r\n"
                 << "Connection: keep-alive\r\n\r\n"
                 << body.str();
        return response.str();
    }

    // GET - Serve file
    if (requested_method == "GET")
    {
        std::ifstream file(filePath.c_str());
        if (!file.is_open())
            return generate_error_response(404, server);

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string body = buffer.str();

        std::stringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: text/html\r\n"
                 << "Content-Length: " << body.length() << "\r\n"
                 << "Connection: keep-alive\r\n\r\n"
                 << body;
        return response.str();
    }

    // DELETE
    if (requested_method == "DELETE")
    {
        if (remove(filePath.c_str()) == 0)
            return "HTTP/1.1 200 OK\r\n\r\nFile deleted successfully.";
        else
            return generate_error_response(500, server);
    }

    return generate_error_response(400, server);
}

void set_response_for_client(Client &client)
{
    std::string response;
    std::string uri_requested = client.getRequest().getUri();       // es. "/upload/file.txt"
    std::string requested_method = client.getRequest().getMethod(); // es. "POST"
    const route* matched_route = NULL;
    size_t longest_match_length = 0;
    for (size_t i = 0; i < client.getServer()->getRoutesSize(); ++i)
    {
        const route& current_route = client.getServer()->getRoute(i);
        const std::string& route_uri = current_route.uri;

        bool match = false;

        if (uri_requested == route_uri)
            match = true;
        else if (route_uri != "/" && uri_requested.find(route_uri + "/") == 0)
            match = true;
        else if (route_uri == "/" && longest_match_length == 0)
            match = true;  // fallback solo se nessuna route ha matchato prima

        if (match && route_uri.length() > longest_match_length)
        {
            matched_route = &current_route;
            longest_match_length = route_uri.length();
        }
    }
    if (matched_route)
    {
        response = handle_request(uri_requested, *matched_route, requested_method, client, *client.getServer());
        client.set_response(response);
    }
    else
    {
        client.set_response(generate_error_response(404, *client.getServer())); // URI non trovato
    }
}


/*
void set_response_for_client(Client &client)
{
    std::string response;
    std::string uri_requested = client.getRequest().getUri();       // Extract URI and method from the request // e.g., "/upload"
    std::string requested_method = client.getRequest().getMethod(); // e.g., "POST"
    for (size_t i = 0; i < client.getServer()->getRoutesSize(); ++i) // Match URI to a route in the server
    {
        std::string route_uri = client.getServer()->getRoute(i).uri;
        if (uri_requested == route_uri || (uri_requested.find(route_uri + "/") == 0 && route_uri != "/"))
        {
            const route matched_route = client.getServer()->getRoute(i);
            response = (handle_request(uri_requested, matched_route, requested_method, client, *client.getServer()));
            client.set_response(response);
            return;
        }
    }
    client.set_response(generate_error_response(0, *client.getServer())); //URI not found
    return;
}*/
