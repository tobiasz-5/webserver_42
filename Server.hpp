#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cerrno>  // Per errno
#include "config.hpp"

class Server
{
    private:
        std::vector<int> ports;
        std::string host;
        std::string listen_address;
        std::string server_name;
        std::map<int, std::string> error_pages;
        size_t max_body_size;
        std::vector<Route> routes;

        std::vector<int> serv_fds;
        std::vector<sockaddr_in> server_addr;
    public:
        //Server();
        Server(const struct config &config);
        Server(Server const &other);
		Server &operator=(Server const &other);
        ~Server();
    	class ServerCreationException : public std::exception
		{
			public:
				virtual const char* what() const throw()
				{
					return ("Server init error");
				}
		};
        const int &getServfd(int i) const;
        const struct sockaddr_in &getStructaddr(int i) const;
        size_t getnumport(void) const;
        bool isServerFd(int fd) const;
        void bind_listen(void);
        void closing_fd(void);
        void print_var(void) const;
};
