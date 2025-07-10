
#include "Server.hpp"
#include "config.hpp"

Server::Server(const struct config &config)
{
    std::cout << "\033[33m--Server costructor called--" << std::endl;
	ports = config.ports;
    host = config.host;
    listen_address = config.listen_address;
    server_name = config.server_name;
    error_pages = config.error_pages;
    max_body_size = config.max_body_size;
    routes = config.routes;

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
        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
            throw ServerCreationException();
        server_addr.push_back(addr);  // aggiungi struct addr
    }
    print_server_var();
    std::cout << "\033[0m" << std::endl;
}

Server::Server(Server const &other)
{
	*this = other;
}

Server &Server::operator=(Server const &other)
{
	if (this != &other)
    {
        this->ports = other.ports;
        this->host = other.host;
        this->listen_address = other.listen_address;
        this->server_name = other.server_name;
        this->error_pages = other.error_pages;
        this->max_body_size = other.max_body_size;
        this->routes = other.routes;
        this->serv_fds = other.serv_fds;
        this->server_addr = other.server_addr;
    }
	return (*this);
}

Server::~Server()
{
}

size_t Server::getnumport(void) const {return (static_cast<size_t>(ports.size())); }
const int &Server::getServfd(int i) const {return(serv_fds.at(i));}
const struct sockaddr_in &Server::getStructaddr(int i) const {return(server_addr.at(i));}
size_t Server::getRoutesSize() const {return routes.size();}
const std::map<int, std::string> &Server::getError_pages(void) const {return(error_pages);}

const route &Server::getRoute(size_t i) const
{
    if (i >= routes.size())
        throw std::out_of_range("Index out of range in getRoute");
    return routes.at(i);
}

bool Server::isServerFd(int fd) const
{
    return std::find(serv_fds.begin(), serv_fds.end(), fd) != serv_fds.end();
}

void Server::bind_listen(void)
{
    for (size_t i = 0; i < ports.size(); ++i)
    {
        int opt = 1;
        if (setsockopt(serv_fds.at(i), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            std::cerr << "setsockopt failed: " << strerror(errno) << std::endl;
            close(serv_fds.at(i));
            throw ServerCreationException();
        }
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

void Server::closing_fd(void)
{
    for (size_t i = 0; i < serv_fds.size(); ++i)
	    close(serv_fds.at(i));
    std::cout << "Closing all server fd" << std::endl;
}

void Server::print_server_var(void) const
{
    std::cout << "listen from : " << listen_address << std::endl;
    std::cout << "server_name: " << server_name << std::endl;
    std::cout << "network interface: " << host << std::endl;
    std::cout << "max_body_size: " << max_body_size << std::endl;

	std::cout << "Server sockets created for ports:" << std::endl;
    for (size_t i = 0; i < ports.size(); ++i)
		std::cout << " " << ports.at(i);
	std::cout << std::endl;
	std::cout << "Error Pages    : " << std::endl;
    for (std::map<int, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end(); ++it)
		std::cout << "  [" << it->first << "] -> " << it->second << std::endl;
    std::cout << "Routes         : " << std::endl;
    for (size_t i = 0; i < routes.size(); ++i)
    {
        const route &r = routes.at(i);
        std::cout << "  Route " << i + 1 << ":" << std::endl;
        std::cout << "    URI               : " << r.uri << std::endl;
        std::cout << "    Root Directory    : " << r.root_directory << std::endl;
        std::cout << "    Default File      : " << r.default_file << std::endl;
        std::cout << "    Redirect          : " << r.redirect << std::endl;
        std::cout << "    Directory Listing : " << (r.directory_listing ? "on" : "off") << std::endl;
        std::cout << "    Allowed Methods   : ";
        for (size_t j = 0; j < r.allowed_methods.size(); ++j)
            std::cout << r.allowed_methods.at(j) << " ";
        std::cout << std::endl;
        std::cout << "    CGI Extensions    : ";
        for (size_t j = 0; j < r.cgi_extensions.size(); ++j)
            std::cout << r.cgi_extensions.at(j) << " ";
        std::cout << std::endl;
        std::cout << "    CGI Path          : " << r.cgi_path << std::endl;
        std::cout << "    Upload Path       : " << r.upload_path << std::endl;
    }

    std::cout << "Server FDs     : ";
    for (size_t i = 0; i < serv_fds.size(); ++i)
        std::cout << serv_fds.at(i) << " ";
    std::cout << std::endl;

    std::cout << "------------------------------" << std::endl;
}
