
#include "Client.hpp"

Client::Client()
{
}

Client::Client(int fd, struct sockaddr_in addr, const int accepted_listen_fd) : fd(fd), addr(addr), accepted_listen_fd(accepted_listen_fd)
{
	accepted_server = NULL;
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
	this->accepted_listen_fd = other.accepted_listen_fd;
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

const int &Client::getServerfd() const
{
    return(accepted_listen_fd);
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

std::string Client::getHostWithoutPort(const std::string& host)
{
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos)
	{
        return host.substr(0, colonPos);
    }
    return host;
}

Server* Client::findRightServer(const std::vector<SocketBinding> &bindings, const std::vector<Server> &servers, const std::string &host)
{
    (void)bindings;
	std::vector<Server*> candidateServers;

    for (size_t i = 0; i < servers.size(); ++i) // 1. Trova tutti i Server che ascoltano su accepted_listen_fd
    {
        const std::vector<int>& fds = servers[i].getServerFDS();
        for (size_t j = 0; j < fds.size(); ++j)
        {
            if (fds[j] == accepted_listen_fd)
            {
                candidateServers.push_back(const_cast<Server*>(&servers[i]));
                break;
            }
        }
    }
    if (candidateServers.empty())
        return NULL;
    for (size_t i = 0; i < candidateServers.size(); ++i) // 2. Cerca tra i candidati quello con un server_name che combacia
    {
        const std::vector<std::string>& names = candidateServers[i]->getServerNames();
        for (size_t j = 0; j < names.size(); ++j)
        {
			if (names[j] == host)
                return candidateServers[i];
        }
    }
    return candidateServers[0];
}

int Client::receiveRequest(const std::vector<SocketBinding> &bindings, const std::vector<Server> &servers)
{
	int bytesRead = request.receiveData(fd);
    if (bytesRead <= 0)
        return bytesRead;
    const std::string &buffer = request.getBuffer();
    std::size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd != std::string::npos)
    {
        headerEnd += 4;

		if (accepted_server == NULL)  // se non è ancora stato associato un server
        {
            std::string host_port = request.getHostHeader();
			std::string host = getHostWithoutPort(host_port);  // Estrai l'header Host

			accepted_server = findRightServer(bindings, servers, host);
            if (accepted_server == NULL)
            {
                std::cerr << "Server not found for host: " << host << std::endl;
                return -1;
            }
        }
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
                    contentLength = to_int(value);
                }
            }

        }
		//std::cout << contentLength << "---" << accepted_server->getMaxBodySize() << std::endl;
		if ((size_t)contentLength > accepted_server->getMaxBodySize())
		{
			std::cerr << "❌ Body size (" << contentLength << " bytes) exceeds maximum allowed (" << accepted_server->getMaxBodySize() << " bytes)\n";
			//request.setComplete(true); // forza chiusura della richiesta
			return -1; // oppure segna per generare risposta 413
		}
        if (buffer.size() >= headerEnd + contentLength) // Controllo presenza body completo
        {
            request.setComplete(true);
            request.parseRequest(); 
        }
    }
    return bytesRead;
}