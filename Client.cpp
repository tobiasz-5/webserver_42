
#include "Client.hpp"

Client::Client()
{
}

Client::Client(int fd, struct sockaddr_in addr) : fd(fd), addr(addr)
{
}

Client::Client(Client const &other)
{
	*this = other;
}

Client &Client::operator=(Client const &other)
{
	this->fd = other.fd;
    this->addr = other.addr;
	return (*this);
}

Client::~Client()
{
}

const int &Client::getClientfd() const
{
    return(fd);
}

const struct sockaddr_in &Client::getStructaddr() const
{
    return(addr);
}
