
#include "Server.hpp"

Server::Server()
{
    //create server socket
    serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_fd < 0)
        throw ServerCreationException();
    //dati del server messi dentro struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
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
    //lega il socket creato all'indirizzo IP e alla porta specificati
    if (bind(serv_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cout << "bind error" << std::endl;
        close(serv_fd);
        throw ServerCreationException();
    }
    //ascolto connessioni
    if (listen(serv_fd, 10) < 0)
    {
        std::cout << "listen error" << std::endl;
        close(serv_fd);
        throw ServerCreationException();
    }
    return;
}
