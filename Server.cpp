
#include "Server.hpp"

Server::Server()
{
    serv_fd = socket(AF_INET, SOCK_STREAM, 0);   //create server socket
    if (serv_fd < 0)
        throw ServerCreationException();
    server_addr.sin_family = AF_INET;           //dati del server messi dentro struct
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //std::cout << server_addr.sin_family << " - " << server_addr.sin_port << " - " << server_addr.sin_addr.s_addr << std::endl;
}

Server::Server(Server const &other)
{
	*this = other;
}

Server &Server::operator=(Server const &other)
{
	this->serv_fd = other.serv_fd;
    this->server_addr = other.server_addr;
	return (*this);
}

Server::~Server()
{
}

const int &Server::getServfd() const
{
    return(serv_fd);
}

const struct sockaddr_in &Server::getStructaddr() const
{
    return(server_addr);
}

void Server::bind_listen(void)
{
    if (bind(serv_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)     //lega il socket creato all'indirizzo IP e alla porta specificati
    {
        std::cout << "bind error" << std::endl;
        close(serv_fd);
        throw ServerCreationException();
    }
    if (listen(serv_fd, 10) < 0)    //ascolto connessioni
    {
        std::cout << "listen error" << std::endl;
        close(serv_fd);
        throw ServerCreationException();
    }
    return;
}
