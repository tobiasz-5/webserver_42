
#include "Server.hpp"
#include "config.hpp"

Server::Server(const struct config &config)
{
    ports = config.ports;
    host = config.host;
    listen_address = config.listen_address;
    server_name = config.server_name;
    error_pages = config.error_pages;
    max_body_size = config.max_body_size;
    routes = config.routes;
    std::cout << "listen: " << listen_address;

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
    //debug message
    std::cout << "Server sockets created for ports:" << std::endl;
    for (size_t i = 0; i < ports.size(); ++i)
        std::cout << " " << ports.at(i);
    std::cout << std::endl;
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
    this->host = other.host;
    this->listen_address = other.listen_address;
    this->server_name = other.server_name;
    this->error_pages = other.error_pages;
    this->max_body_size = other.max_body_size;
    this->routes = other.routes;
	return (*this);
}

Server::~Server()
{
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

void Server::closing_fd(void)
{
    for (size_t i = 0; i < serv_fds.size(); ++i)
	    close(serv_fds.at(i));
    std::cout << "Closing all server fd" << std::endl;
}

void Server::print_var(void) const
{
    std::cout << "inizio" << std::endl;
    std::cout << "Listen Address: " << listen_address << "\n";
        std::cout << "Ports: ";
        for (size_t j = 0; j < ports.size(); ++j)
        {
            std::cout << ports[j];
            if (j != ports.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        std::cout << "Server Name: " << server_name << "\n";
        std::cout << "Host: " << host << "\n";
        std::cout << "Max Body Size: " << max_body_size << "\n";
        std::cout << "Error Pages:\n";
        for (std::map<int, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end(); ++it)
        {
            std::cout << "  " << it->first << " => " << it->second << "\n";
        }
        std::cout << "Routes:\n";
        for (size_t r = 0; r < routes.size(); ++r)
        {
            const Route &route = routes[r];
            std::cout << "  --- Route " << r + 1 << " ---\n";
            std::cout << "  Allowed Methods: ";
            for (size_t m = 0; m < route.allowed_methods.size(); ++m)
            {
                std::cout << route.allowed_methods[m];
                if (m != route.allowed_methods.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
            std::cout << "  Redirect: " << route.redirect << "\n";
            std::cout << "  Directory Listing: " << (route.directory_listing ? "on" : "off") << "\n";
            std::cout << "  Default File: " << route.default_file << "\n";
            std::cout << "  CGI Extension: " << route.cgi_extension << "\n";
            std::cout << "  CGI Path: " << route.cgi_path << "\n";
            std::cout << "  Upload Path: " << route.upload_path << "\n";
        }
        std::cout << "\n";
}