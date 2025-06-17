
#include "Request.hpp"
#include <iostream>
#include <sstream> 

Request::Request() : buffer("")
{
}

Request::Request(Request const &other)
{
	*this = other;
}

Request &Request::operator=(Request const &other)
{
    if (this != &other)
        this->buffer = other.buffer;
    return (*this);
}

const std::string &Request::getMethod() const { return method; }
const std::string &Request::getUri() const { return uri; }
const std::string &Request::getBody() const { return body; }
const std::map<std::string, std::string> &Request::getHeaders() const { return headers; }
const std::string &Request::getBuffer(void) const{ return(buffer);}

Request::~Request(){}

int Request::receiveData(int fd)
{
    char temp_buffer[2048];
    int bytes_read = recv(fd, temp_buffer, sizeof(temp_buffer) - 1, 0);
    if (bytes_read > 0)
    {
        temp_buffer[bytes_read] = '\0'; // terminatore stringa
        buffer.append(temp_buffer, bytes_read);

        std::cout << "--->START BUFFER---<\n" << buffer << std::endl; //debug
        std::cout << "--->END   BUFFER---<\n" << std::endl; //debug
    }
    else if (bytes_read == 0)
    {
        std::cout << "Client closed connection." << std::endl;
        return 0;
    }
    else
        buffer.clear(); // se errore o chiusura connessione, svuoto il buffer
    std::cout << "byte read :" << bytes_read << std::endl; //debug
    return (bytes_read);
}

//extracts the URI from the request line
void Request::parseRequest()
{
    std::istringstream stream(buffer);
    std::string line;

    std::cout << "0000000000" <<std::endl;
    // Parse the request line (e.g., "GET /index.html HTTP/1.1")
    if (std::getline(stream, line))
    {
        std::cout << "111111111" <<std::endl;
        std::istringstream lineStream(line);
        lineStream >> method >> uri >> http_version;
        std::cout << "Parsed Method: " << method << ", URI: " << uri << ", HTTP Version: " << http_version << std::endl;
    }

    // Parse headers
    while (std::getline(stream, line) && line != "\r")
    {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            headers[key] = value;
        }
    }

    // Parse body (if present)
    std::string bodyContent;
    while (std::getline(stream, line))
    {
        bodyContent += line + "\n";
    }
    body = bodyContent;
}
