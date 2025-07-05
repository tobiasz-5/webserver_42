#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <map>
#include "Request.hpp"
#include "Server.hpp"

class Client
{
    private:
        int fd;
        struct sockaddr_in addr;
        std::string response;
        const Server *accepted_server; // server a cui è connesso il client
        Request request;
    public:
        Client();
        Client(int fd, struct sockaddr_in addr, const Server *accepted_server);
        Client(Client const &other);
		Client &operator=(Client const &other);
        ~Client();

        const int &getClientfd(void) const;
        const Server *getServer(void) const;
        const struct sockaddr_in &getStructaddr(void) const;
        const Request &getRequest(void) const;
        Request& getRequest(void);
        const std::string &getresponse(void) const;

        void set_response(std::string s);

        int receiveRequest();
};
