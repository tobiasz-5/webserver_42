
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
    this->request = other.request;
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

const Request  &Client::getRequest(void) const
{ return request; }

int Client::receiveRequest()
{
    int bytesRead = request.receiveData(fd); // Read data from the client socket
    if (bytesRead > 0)
    {
        request.parseRequest(); // Parse the HTTP request after receiving data
    }
    return bytesRead;
}
