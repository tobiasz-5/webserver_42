#pragma once

#include <iostream>
#include <unistd.h>
#include <string.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/select.h>
#include <map>
#include <map>

class Request
{
    private:
        std::string buffer; // Stores the raw HTTP request
        std::string method; // GET, POST, etc.
        std::string uri;  // /picture/profile1.html /home, etc.
        std::string http_version;  // HTTP/1.1
        std::map<std::string, std::string> headers; // header: valore
        std::string body;     // solo se presente
        std::string resource; // Stores the requested resource (e.g., /index.html)
        //bool complete;                     // true se la richiesta è completa
    public:
        Request();
        Request(Request const &other);
		Request &operator=(Request const &other);
        ~Request();

        const std::string &getBuffer() const;
        const std::string &getMethod() const;
        const std::string &getUri() const;
        const std::string &getBody() const;
        const std::map<std::string, std::string> &getHeaders() const;

        int receiveData(int fd); // Reads data from the client socket
        void parseRequest(); // Parses the HTTP request
};
