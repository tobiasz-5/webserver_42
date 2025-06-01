
#include "Server.hpp"
#include "config.hpp"

Server::Server(const struct config &config)
{
    ports = config.ports;
    serv_fds.reserve(config.ports.size());
    server_addr.reserve(config.ports.size());

    for (size_t i = 0; i < ports.size(); ++i)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            throw ServerCreationException();
        fcntl(fd, F_SETFL, O_NONBLOCK);  // rendi il socket non-bloccante
        serv_fds.push_back(fd);  // aggiungi il fd al vector

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ports.at(i));
        addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.push_back(addr);  // aggiungi struct addr
    }
    std::cout << "Server sockets created for ports:";
    for (size_t i = 0; i < ports.size(); ++i)
        std::cout << " " << ports.at(i) << std::endl;
}

Server::Server(Server const &other)
{
	*this = other;
}

Server &Server::operator=(Server const &other)
{
	this->serv_fds = other.serv_fds;
    this->server_addr = other.server_addr;
    this->ports = other.ports;
	return (*this);
}

Server::~Server()
{
    for (size_t i = 0; i < serv_fds.size(); ++i)
	    close(serv_fds.at(i));
    std::cout << "Closing all server fd" << std::endl;
}

size_t Server::getnumport(void) const
{
    return(static_cast<size_t>(ports.size()));
}

const int &Server::getServfd(int i) const
{
    return(serv_fds.at(i));
}

const struct sockaddr_in &Server::getStructaddr(int i) const
{
    return(server_addr.at(i));
}

bool Server::isServerFd(int fd) const
{
    return std::find(serv_fds.begin(), serv_fds.end(), fd) != serv_fds.end();
}

void Server::bind_listen(void)
{
    for (size_t i = 0; i < ports.size(); ++i)
    {
        if (bind(serv_fds.at(i), (struct sockaddr *)&server_addr.at(i), sizeof(server_addr.at(i))) < 0)     //lega il socket creato all'indirizzo IP e alla porta specificati
        {
            std::cout << "bind error" << std::endl;
            close(serv_fds.at(i));
            throw ServerCreationException();
        }
        fcntl(serv_fds.at(i), F_SETFL, O_NONBLOCK); //server fd non blocking
        if (listen(serv_fds.at(i), 10) < 0)    //ascolto connessioni
        {
            std::cout << "listen error" << std::endl;
            close(serv_fds.at(i));
            throw ServerCreationException();
        }
    }
    return;
}
