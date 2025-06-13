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
        std::string buffer;

        std::string method;                    // GET, POST, etc.
        std::string uri;                       // /login, /home, etc.
        std::string http_version;             // HTTP/1.1
        std::map<std::string, std::string> headers; // header: valore
        std::string body;                      // solo se presente
        //bool complete;                     // true se la richiesta è completa
    public:
        Request();
        Request(Request const &other);
		Request &operator=(Request const &other);
        ~Request();
        const std::string &getBuffer(void) const;
        int receiveData(int fd);
};
