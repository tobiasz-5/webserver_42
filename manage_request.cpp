#include "Server.hpp"
#include "Client.hpp"
#include "manage_request.hpp"

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

std::string handle_request(std::string uri, const route &matched_route, std::string requested_method)
{
    const std::vector<std::string> allowed_methods = matched_route.allowed_methods;
    std::cout << "-->Requested method: =" << requested_method << "=" << std::endl;
    std::cout << allowed_methods.size() << " allowed methodS for this route." << std::endl;
    if (requested_method == "GET")
    {
        std::string filePath = "./www" + uri; // Map URI to file path
        std::string body = loadHtmlFile(filePath);

        return "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 13\r\n"
                    "Connection: keep-alive\r\n"
                    "\r\n"
                    "Hello, world!"; //test
        
        /*return "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Connection: keep-alive\r\n"
               "\r\n" +
               body;*/
    }
    if (requested_method == "POST")
    {
        // Process POST data (e.g., save uploaded files)
        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Connection: keep-alive\r\n"
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

void set_response_for_client(Client &client)
{
    std::string response;
    // Extract URI and method from the request
    std::string uri_requested = client.getRequest().getUri();       // e.g., "/upload"
    std::cout << "URI requested: =" << uri_requested << "=" << std::endl;
    std::string requested_method = client.getRequest().getMethod(); // e.g., "POST"
    std::cout << "Method requested: =" << requested_method << "=" << std::endl;
    // Match URI to a route in the server
    for (size_t i = 0; i < client.getServer()->getRoutesSize(); ++i)
    {
        if (uri_requested == client.getServer()->getRoute(i).uri) // Match the URI
        {
            const route matched_route = client.getServer()->getRoute(i);
            response = (handle_request(uri_requested, matched_route, requested_method));
            client.set_response(response);
            return;
        }
    }
    client.set_response("HTTP/1.1 404 Not Found\r\n\r\n");
    return;
}