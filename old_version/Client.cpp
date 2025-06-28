
#include "Client.hpp"

Client::Client()
{
}

Client::Client(int fd, struct sockaddr_in addr, int server_fd) : fd(fd), addr(addr), server_fd(server_fd)
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
    this->response = other.response;
    this->server_fd = other.server_fd;
	return (*this);
}

Client::~Client()
{
}

const int &Client::getClientfd() const
{
    return(fd);
}

const int &Client::getServerfd() const
{
    return(server_fd); // Return the server file descriptor associated with this client
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

void Client::set_response(std::string s)
{
    this->response = s;
}

const std::string &Client::getresponse(void) const
{
    return(response);
}