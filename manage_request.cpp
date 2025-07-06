#include "Server.hpp"
#include "Client.hpp"
#include "manage_request.hpp"

std::string handle_request(std::string uri, const route &matched_route, std::string requested_method, const Client &client)
{
    bool methodAllowed = std::find(matched_route.allowed_methods.begin(), matched_route.allowed_methods.end(), requested_method) != matched_route.allowed_methods.end();
    if (!methodAllowed)
        return "HTTP/1.1 405 Method Not Allowed\r\nAllow: GET, POST, DELETE\r\n\r\n";
    if (!matched_route.redirect.empty()) // Redirect
    {
        return "HTTP/1.1 301 Moved Permanently\r\n"
               "Location: " + matched_route.redirect + "\r\n\r\n";
    }
    std::string relativePath = uri.substr(matched_route.uri.length()); // Mapping URI -> File system
    if (relativePath.empty() || relativePath == "/")
        relativePath = "/" + matched_route.default_file;
    std::string filePath = matched_route.root_directory + relativePath;
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) == -1) // controlla se file esiste
    {
        return "HTTP/1.1 404 Not Found\r\n\r\n";
    }
    if (S_ISDIR(fileStat.st_mode)) // se directory
    {
        if (!matched_route.directory_listing)          // se non ha permessi, errore
            return "HTTP/1.1 403 Forbidden\r\n\r\n";
        DIR *dir = opendir(filePath.c_str());
        if (!dir)
            return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
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
    /*
    // Esecuzione CGI, da gestire ancora
    for (size_t i = 0; i < matched_route.cgi_extensions.size(); ++i)
    {
        if (filePath.size() >= matched_route.cgi_extensions[i].size() &&
            filePath.compare(filePath.size() - matched_route.cgi_extensions[i].size(),
                             matched_route.cgi_extensions[i].size(),
                             matched_route.cgi_extensions[i]) == 0)
        {
            return execute_cgi(filePath); // Ti preparo un esempio dopo se vuoi
        }
    }*/
    if (requested_method == "GET")
    {
        std::ifstream file(filePath.c_str());
        if (!file.is_open())
            return "HTTP/1.1 404 Not Found\r\n\r\n";
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string body = buffer.str();
        std::stringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: text/html\r\n"
                 << "Content-Length: " << body.length() << "\r\n"
                 << "Connection: keep-alive\r\n\r\n"
                 << body;
        return (response.str());
    }
    if (requested_method == "POST")
    {
        std::string uploadDir = matched_route.upload_path;
        std::string filename = "uploaded_file";
        std::string fullPath = uploadDir + "/" + filename;
        std::ofstream outfile(fullPath.c_str(), std::ios::binary); //apre il file e se il file non esiste lo crea automaticamente
        if (!outfile.is_open())
            return "HTTP/1.1 500 Internal Server Error\r\n\r\nFailed to save file.";
        outfile << client.getRequest().getBody(); // Scrivi il body della richiesta nel file
        outfile.close();
        return "HTTP/1.1 201 Created\r\n\r\nFile uploaded successfully.";
    }
    if (requested_method == "DELETE")
    {
        if (remove(filePath.c_str()) == 0)
            return "HTTP/1.1 200 OK\r\n\r\nFile deleted successfully.";
        else
            return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
    }
    return "HTTP/1.1 400 Bad Request\r\n\r\n";
}

void set_response_for_client(Client &client)
{
    std::string response;
    std::string uri_requested = client.getRequest().getUri();       // Extract URI and method from the request // e.g., "/upload"
    std::cout << "URI requested: =" << uri_requested << "=" << std::endl;
    std::string requested_method = client.getRequest().getMethod(); // e.g., "POST"
    std::cout << "Method requested: =" << requested_method << "=" << std::endl;
    for (size_t i = 0; i < client.getServer()->getRoutesSize(); ++i) // Match URI to a route in the server
    {
        std::string route_uri = client.getServer()->getRoute(i).uri;
        if (uri_requested.compare(0, route_uri.size(), route_uri) == 0 && (uri_requested.size() == route_uri.size() || uri_requested[route_uri.size()] == '/' || route_uri[route_uri.size() - 1] == '/'))
        {
            const route matched_route = client.getServer()->getRoute(i);
            response = (handle_request(uri_requested, matched_route, requested_method, client));
            client.set_response(response);
            return;
        }
    }
    client.set_response("HTTP/1.1 404 Not Found\r\n\r\n");
    return;
}
