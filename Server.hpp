#pragma once

#define PORT 8080

#include <iostream>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

class Server
{
    private:
        int serv_fd;
        struct sockaddr_in server_addr;
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
        const int &getServfd(void) const;
        const struct sockaddr_in &getStructaddr(void) const;
        void bind_listen(void);
};
