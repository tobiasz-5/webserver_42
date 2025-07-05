#include "Client.hpp"

Client::Client()
{
}

Client::Client(int fd, struct sockaddr_in addr, const Server *accepted_server) : fd(fd), addr(addr), accepted_server(accepted_server)
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
    this->accepted_server = other.accepted_server;
	return (*this);
}

Client::~Client()
{
}

const int &Client::getClientfd() const
{
    return(fd);
}

const Server *Client::getServer() const
{
    return(accepted_server); // Return the server associated with this client
}

const struct sockaddr_in &Client::getStructaddr() const
{
    return(addr);
}

const Request  &Client::getRequest(void) const
{
    return request;
}

const std::string &Client::getresponse(void) const
{
    return(response);
}

void Client::set_response(std::string s)
{
    this->response = s;
}

void Client::clearRequest()
{
    request.clearData();
}

int Client::receiveRequest()
{
    int bytesRead = request.receiveData(fd); // Read data from the client socket
    if (bytesRead > 0)
    {
        request.parseRequest(); // Parse the HTTP request after receiving data
    }
    return bytesRead;
}
