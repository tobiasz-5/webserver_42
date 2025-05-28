#pragma once

#define PORT 8080
#define PORT2 8001

#include <iostream>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <vector>
#include <cstring>
#include <algorithm>

class Server
{
    private:
        int num_port;  //dati presi da config file
        std::vector<int> serv_fds;
        std::vector<sockaddr_in> server_addr;
        std::vector<int> ports;  //dati presi da config file
    public:
        Server();
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
};
