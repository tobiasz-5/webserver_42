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
        bool        complete;                     // true se la richiesta è completa
        bool        chunked;
    public:
        Request();
        Request(Request const &other);
		Request &operator=(Request const &other);
        ~Request();

        const std::string &getBuffer() const;
        const std::string &getMethod() const;
        const std::string &getUri() const;
        const std::string &gethttp_version() const;
        const std::map<std::string, std::string> &getHeaders() const;
        const std::string &getBody() const;
		std::string getHeader(const std::string& key) const;
        const bool &getComplete() const;
		std::string getHostHeader() const;

        void clearData(void);
        void setComplete(bool b);
        int receiveData(int fd); // Reads data from the client socket
        void parseRequest(); // Parses the HTTP request
        bool isChunked() const { return chunked; }
        std::string unchunkBody(const std::string& raw);
};
