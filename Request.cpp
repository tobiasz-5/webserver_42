
#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <stdio.h>

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
    {
        this->buffer = other.buffer;
        this->method = other.method;
        this->uri = other.uri;
        this->http_version = other.http_version;
        this->headers = other.headers;
        this->body = other.body;
        this->complete = other.complete;
    }
    return (*this);
}

Request::~Request()
{
}

const std::string &Request::getMethod() const { return method; }
const std::string &Request::getUri() const { return uri; }
const std::string &Request::getBody() const { return body; }
const std::string &Request::gethttp_version() const { return http_version; }
const std::map<std::string, std::string> &Request::getHeaders() const { return headers; }
const std::string &Request::getBuffer(void) const{ return(buffer); }
const bool &Request::getComplete(void) const{ return(complete); }

std::string Request::getHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end())
        return it->second;
    return ""; // o throw, o valore di default
}

void Request::setComplete(bool b)
{
    complete = b;
    return;
}

void Request::clearData()
{
    buffer.clear();
    method.clear();
    uri.clear();
    http_version.clear();
    headers.clear();
    body.clear();
    complete = false;
}

int Request::receiveData(int fd)
{
    char temp_buffer[2048];
    int bytes_read = recv(fd, temp_buffer, sizeof(temp_buffer) - 1, 0);
    if (bytes_read <= 0)
        perror("recv failed");
    if (bytes_read > 0)
    {
        temp_buffer[bytes_read] = '\0'; // terminatore stringa
        buffer.append(temp_buffer, bytes_read);
        std::cout << "\033[32m---REQUEST RECEIVED---\n" << buffer << std::endl;
        std::cout << "---END REQUEST----- (byte read:" << bytes_read  << ")\033[0m" << std::endl;
    }
    else if (bytes_read == 0)
    {
        std::cout << "Client closed connection." << std::endl;
        return 0;
    }
    else
        buffer.clear(); // se errore o chiusura connessione, svuoto il buffer
    return (bytes_read);
}

void Request::parseRequest()
{
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return; // Header incompleto
    std::string headerPart = buffer.substr(0, headerEnd);
    std::istringstream stream(headerPart);
    std::string line;
    if (std::getline(stream, line))
    {
        std::istringstream lineStream(line);
        lineStream >> method >> uri >> http_version;
    }
    while (std::getline(stream, line)) // Headers
    {
        if (line == "\r" || line.empty())
            break;
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            headers[key] = value;
        }
    }
	body = buffer.substr(headerEnd + 4); // Body (se presente)
	std::cout << "\n\033[38;5;22m--- Parsed Request ---" << std::endl;
    std::cout << "Method       : " << method << std::endl;
    std::cout << "URI          : " << uri << std::endl;
    std::cout << "HTTP Version : " << http_version << std::endl;
    std::cout << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    std::cout << "Body Length  : " << body.length() << " bytes" << std::endl;
    std::cout << "Body Content : " << body << std::endl;
    std::cout << "-----------------------\033[0m" << std::endl;
}
