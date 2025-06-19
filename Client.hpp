#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <map>
#include "Request.hpp"

class Client
{
    private:
        int fd;
        struct sockaddr_in addr;
        Request request;
        std::string response;
    public:
        Client();
        Client(int fd, struct sockaddr_in addr);
        Client(Client const &other);
		Client &operator=(Client const &other);
        ~Client();
        const int &getClientfd(void) const;
        const struct sockaddr_in &getStructaddr(void) const;
        const Request &getRequest(void) const;
        void set_response(std::string s);
        const std::string &getresponse(void) const;
        int receiveRequest();
};
