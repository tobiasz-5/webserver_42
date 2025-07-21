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
#include "utils.hpp"

class Client
{
    private:
        int fd;
        struct sockaddr_in addr;
        std::string response;

		int accepted_listen_fd; // fd della socket di ascolto da cui è stato accettato
        Server *accepted_server; // server a cui è connesso il client

        Request request;
    public:
        Client();
        Client(int fd, struct sockaddr_in addr, const int accepted_listen_fd);
        Client(Client const &other);
		Client &operator=(Client const &other);
        ~Client();

        const int &getClientfd(void) const;
		const int &getServerfd(void) const;
        const Server *getServer(void) const;
        const struct sockaddr_in &getStructaddr(void) const;
        const Request &getRequest(void) const;
        Request& getRequest(void);
        const std::string &getresponse(void) const;

        void set_response(std::string s);
        void clearRequest(); // Clears the request data for the next request

        int receiveRequest(const std::vector<SocketBinding> &bindings, const std::vector<Server> &servers);
		Server* findRightServer(const std::vector<SocketBinding> &bindings, const std::vector<Server> &servers, const std::string &host);
		std::string getHostWithoutPort(const std::string& host);
};
