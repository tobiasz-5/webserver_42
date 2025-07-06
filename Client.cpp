
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

Request& Client::getRequest() { return request; }

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
    int bytesRead = request.receiveData(fd);
    if (bytesRead <= 0)
        return bytesRead;
    const std::string &buffer = request.getBuffer();
    std::size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd != std::string::npos)
    {
        headerEnd += 4; // Fine header
        std::string headerPart = buffer.substr(0, headerEnd);
        std::istringstream headerStream(headerPart);
        std::string line;
        int contentLength = 0;
        while (std::getline(headerStream, line))
        {
            if (line.find("Content-Length:") != std::string::npos)
            {
                size_t pos = line.find(":");
                if (pos != std::string::npos)
                {
                    std::string value = line.substr(pos + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    contentLength = std::stoi(value);
                }
            }
        }
        if (buffer.size() >= headerEnd + contentLength) // Controllo presenza body completo
        {
            request.setComplete(true);
            request.parseRequest(); 
        }
    }
    return bytesRead;
}