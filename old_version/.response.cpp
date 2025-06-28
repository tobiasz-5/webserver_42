#include "response.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

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

std::string the_response(const Request &request)
{
    if (request.getMethod() == "GET")
    {
        std::string resourcePath = request.getUri();
        std::cout << "======Resource requested: " << resourcePath << std::endl;
        if (resourcePath == "/")
            resourcePath = "/index.html"; // Default to index.html for root requests

        std::string filePath = "./www" + resourcePath; // Map URI to file path
        std::string body = loadHtmlFile(filePath);

        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Connection: close\r\n"
               "\r\n" +
               body;
    }
    else
    {
        // Serve index.html for non-GET methods
        std::string filePath = "./www/index.html"; // Always serve index.html
        std::string body = loadHtmlFile(filePath);

        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Connection: close\r\n"
               "\r\n" +
               body;
    }
}