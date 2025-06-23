#include "Server.hpp"
#include "Client.hpp"
#include "manage_request.hpp"

const Server* findMatchedServer(const std::vector<Server>& servers, int fd) 
{
    for (size_t i = 0; i < servers.size(); ++i) 
    {
        if (servers[i].isServerFd(fd)) 
        {
            return &servers[i];
        }
    }
    return NULL; 
}

std::string loadHtmlFile(const std::string &filePath)
{
    std::ifstream file(filePath.c_str()); // Open the file
    if (!file.is_open())
    {
        std::cerr << "Error: Unable to open file " << filePath << std::endl;
        return "<html><body><h1>404 - File Not Found</h1></body></html>"; // Return a default 404 page
    }

    std::stringstream buffer;
    buffer << file.rdbuf(); // Read the file contents into a stringstream
    return buffer.str(); // Return the file contents as a string
}

std::string handle_request(std::string uri, const route *matched_route, std::string requested_method)
{
    const std::vector<std::string> &allowed_methods = matched_route->allowed_methods;
    if (std::find(allowed_methods.begin(), allowed_methods.end(), requested_method) == allowed_methods.end())
    {
        return "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
    }

    if (requested_method == "GET")
    {
        std::string filePath = "./www" + uri; // Map URI to file path
        std::string body = loadHtmlFile(filePath);

        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Connection: close\r\n"
               "\r\n" +
               body;
    }

    if (requested_method == "POST")
    {
        // Process POST data (e.g., save uploaded files)
        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Connection: close\r\n"
               "\r\n"
               "<html><body><h1>POST request processed successfully!</h1></body></html>";
    }

    if (requested_method == "DELETE")
    {
        // Remove the requested resource
        std::string filePath = "./www" + uri; // Map URI to file path
        if (remove(filePath.c_str()) == 0)
        {
            return "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Connection: close\r\n"
                   "\r\n"
                   "<html><body><h1>Resource deleted successfully!</h1></body></html>";
        }
        else
        {
            return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        }
    }
    return "HTTP/1.1 404 Not Found\r\n\r\n";
}

std::string the_response(const Request &request, const Server &server)
{
    const route *matched_route = NULL;

    // Extract URI and method from the request
    std::string uri_requested = request.getUri();       // e.g., "/upload"
    std::string requested_method = request.getMethod(); // e.g., "POST"

    // Match URI to a route in the server
    for (size_t i = 0; i < server.getRoutesSize(); ++i)
    {
        const route &current_route = server.getRoute(i); // Access the route
        if (uri_requested == current_route.uri) // Match the URI
        {
            matched_route = &current_route;
            break;
        }
    }
    if (!matched_route)
    {
        return "HTTP/1.1 404 Not Found\r\n\r\n";
    }
    else
    {
        return (handle_request(uri_requested, matched_route, requested_method));
    }
}

void set_response_for_client(Client &client, const Server &server)
{
    const Request &request = client.getRequest();
    std::string response = the_response(request, server);
    client.set_response(response);
}