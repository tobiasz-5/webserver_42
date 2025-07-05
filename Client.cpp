
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

int Client::receiveRequest()
{
    int bytesRead = request.receiveData(fd);
    if (bytesRead <= 0)
        return bytesRead;
    if (request.getBuffer().find("\r\n\r\n") != std::string::npos) // Controlla se gli header sono completi
    {
        std::size_t headerEnd = request.getBuffer().find("\r\n\r\n") + 4; // \r\n\r\n significa fine dell'header
        std::string headerPart = request.getBuffer().substr(0, headerEnd); //estrae la parte dell'header
        std::istringstream headerStream(headerPart);
        std::string line;
        int contentLength = 0;
        while (std::getline(headerStream, line) && line != "\r")
        {
            if (line.find("Content-Length:") != std::string::npos) // Estrai Content-Length se presente per vedere se ho ricevuto tutto il body
            {
                contentLength = std::stoi(line.substr(15));
            }
        }
        if (request.getBuffer().size() >= headerEnd + contentLength)  // Controlla se ho tutto il body
        {
            request.setComplete(true);
            request.parseRequest(); // Solo ora effettuo il parsing completo
        }
    }
    return bytesRead;
}
