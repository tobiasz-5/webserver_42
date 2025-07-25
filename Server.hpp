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
#include "config.hpp"

class Server
{
    private:
		std::vector<std::pair<std::string, int> > listen_por;
        std::string host;
        std::vector<std::string> server_name;
        std::map<int, std::string> error_pages;
        size_t max_body_size;
        std::vector<route> routes; // BLOCK / or /upload
        std::vector<int> serv_fds;
        std::vector<sockaddr_in> server_addr;
    public:
        Server(const struct config &config, std::vector<SocketBinding> &bindings);
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
        const route &getRoute(size_t i) const;
        size_t getnumport(void) const;
        size_t getRoutesSize() const;
        size_t getMaxBodySize(void) const;
		const std::string &getIP(int i) const;
		int getPort(int i) const;
		const std::vector<std::string> &getServerNames() const;
		const std::vector<int> &getServerFDS() const;
		const std::map<int, std::string> &getError_pages(void) const;
        bool isServerFd(int fd) const;
        void bind_listen(int fd, const sockaddr_in &addr);
        void closing_fd(void);
        void print_server_var(void) const;
};
