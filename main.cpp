#include "config.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "manage_request.hpp"

#include <sys/epoll.h>
#include <vector>
#include <fcntl.h>
#include <signal.h>

#define MAX_EVENTS 1024

volatile sig_atomic_t stop = 0;

void create_server_from_config(std::vector<Server> &serv, const std::vector<config> &conf)
{
    size_t i=0;
    while(i < conf.size())
    {
        serv.push_back(Server(conf.at(i)));
        std::cout << "Creating server for: " << conf.at(i).server_name << std::endl;
        serv.at(i).bind_listen();
        i++;
    }
	for (size_t i = 0; i < serv.size(); ++i) //stampa i dati di ogni server
	{
		std::cout << "--- Server " << i + 1 << " --- created " << std::endl;
		serv[i].print_var();
		std::cout << "---------------- " << std::endl;
	}
}

int add_server_fd(const std::vector<Server> &serv, int epoll_fd)
{
    for (size_t s = 0; s < serv.size(); ++s)
    {
        for (size_t i = 0; i < serv[s].getnumport(); ++i)
        {
            int server_fd = serv[s].getServfd(i);
            fcntl(server_fd, F_SETFL, O_NONBLOCK);

            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = server_fd;

            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
            {
                perror("epoll_ctl: server_fd");
                return(-1);
            }
        }
    }
	return 0;
}

const Server* findServerByFd(const std::vector<Server> &servers, int fd)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
        if (servers[i].isServerFd(fd))
            return &servers[i];
    }
    return NULL;
}

int addClient(int server_fd, std::map<int, Client> &client, int epoll_fd, const Server *server)
{
    sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr *)&client_addr, &len);
	if (client_fd == -1)
	{
		std::cerr << "Error in accept: " << std::endl;
		return(-1);
	}
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
    {
        perror("epoll_ctl: client_fd");
        close(client_fd);
        return -1;
    }
    client.insert(std::make_pair(client_fd, Client(client_fd, client_addr, server)));
    std::cout << "New client: fd = " << client_fd << " PORT " << ntohs(client_addr.sin_port) << std::endl;
    return 0;
}

void disconnectClient(int fd, std::map<int, Client> &client, int epoll_fd)
{
    std::cout << "Client disconnected: fd = " << fd << std::endl;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    client.erase(fd);
}

bool isAnyServerFd(const std::vector<Server> &servers, int fd)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
        if (servers[i].isServerFd(fd))
            return true;
    }
    return false;
}

void close_all_fd(std::vector<Server> &servers, std::map<int, Client> &client)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
		servers[i].closing_fd();
    }
	std::map<int, Client>::iterator it = client.begin();
	for (; it != client.end(); it++)  //close all client file descriptor
		close(it->first);
}

void handle_signal(int signum)
{
	std::cout << " ---Stopping signal received--- "<< std::endl;
	(void)signum;
	stop = 1;
}

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2)
            throw config::ConfigException();

        std::vector<config> conf;
        std::vector<Server> serv;
        std::map<int, Client> client;
        std::string s(argv[1]);

        signal(SIGINT, handle_signal);
        signal(SIGTERM, handle_signal);

        fill_configstruct(conf, s);
        create_server_from_config(serv, conf);

        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1)
        {
            perror("epoll_create1");
            return(-1);
        }

        if (add_server_fd(serv, epoll_fd) < 0)
		{
            close(epoll_fd);
            return(-1);
        }

        struct epoll_event events[MAX_EVENTS];

        while (stop == 0)
        {
            int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
            if (n == -1)
            {
                if (errno == EINTR)
                    continue;
                perror("epoll_wait");
                break;
            }

            for (int i = 0; i < n; ++i)
            {
                int fd = events[i].data.fd;

                if (events[i].events & (EPOLLHUP | EPOLLERR))
                {
                    if (!isAnyServerFd(serv, fd))
                        disconnectClient(fd, client, epoll_fd);
                    continue;
                }

                if (isAnyServerFd(serv, fd))
                {
                    addClient(fd, client, epoll_fd, findServerByFd(serv, fd));
                }
                else if (events[i].events & EPOLLIN)
                {
                    if (client.at(fd).receiveRequest() <= 0)
                    {
                        disconnectClient(fd, client, epoll_fd);
                        continue;
                    }
                    else
                    {
                        set_response_for_client(client.at(fd));
                        //std::cout << client.at(fd).getresponse() << std::endl; //print repsonse for debug
                        struct epoll_event ev;
                        ev.events = EPOLLOUT;
                        ev.data.fd = fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
                    }
                }
                else if (events[i].events & EPOLLOUT)
                {
                    //std::cout << client.at(fd).getresponse().c_str() << std::endl;
                    send(fd, client.at(fd).getresponse().c_str(), client.at(fd).getresponse().size(), 0);
                    //std::cout << "sended response" << std::endl; //print for debug
                    client.at(fd).set_response("");
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
                }
            }
        }
        close_all_fd(serv, client);
        close(epoll_fd);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception called: " << e.what() << std::endl;
    }
    return 0;
}
