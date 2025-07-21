#include "config.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Manage_req.hpp"

#include <sys/epoll.h>
#include <vector>
#include <fcntl.h>
#include <signal.h>

#define MAX_EVENTS 1024

volatile sig_atomic_t stop = 0;

void create_server_from_config(std::vector<Server> &serv, const std::vector<config> &conf, std::vector<SocketBinding> &bindings)
{
    size_t i=0;
    while(i < conf.size())
    {
        serv.push_back(Server(conf.at(i), bindings));
        i++;
    }
	for (size_t i = 0; i < bindings.size(); ++i)
	{
        const SocketBinding& b = bindings[i];
        std::cout << "\033[31m"
                  << "Binding " << i << ":\n"
                  << "  IP: " << b.ip << "\n"
                  << "  Port: " << b.port << "\n"
                  << "  FD: " << b.fd << "\n"
                  //<< "  Server ptr: " << b.server
                  << "\033[0m" << "\n";
    }
	return;
}

void collect_unique_server_fds(const std::vector<Server> &servers, std::vector<int> &unique_server_fd)
{
    for (size_t s = 0; s < servers.size(); ++s)
	{
        for (size_t i = 0; i < servers[s].getnumport(); ++i)
		{
            int fd = servers[s].getServfd(i);
            if (std::find(unique_server_fd.begin(), unique_server_fd.end(), fd) == unique_server_fd.end())
			{
                unique_server_fd.push_back(fd);  // aggiungi solo se non presente
            }
        }
    }
	std::cout << "\033[35m"; // Codice ANSI per magenta/rosa
    std::cout << "unique FDs (size = " << unique_server_fd.size() << "): ";
    for (size_t i = 0; i < unique_server_fd.size(); ++i)
    {
        std::cout << unique_server_fd[i];
        if (i != unique_server_fd.size() - 1)
            std::cout << ", ";
    }
    std::cout << "\033[0m" << std::endl; // Reset colore
    return;
}

int add_server_fd(const std::vector<int> &unique_server_fd, int epoll_fd)
{
    for (size_t i = 0; i < unique_server_fd.size(); ++i)
    {
        int server_fd = unique_server_fd[i];
        fcntl(server_fd, F_SETFL, O_NONBLOCK);

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = server_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
        {
            perror("epoll_ctl: server_fd");
            return -1;
        }
    }
    return 0;
}

int addClient(int server_fd, std::map<int, Client> &client, int epoll_fd)
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
    client.insert(std::make_pair(client_fd, Client(client_fd, client_addr, server_fd)));
    std::cout << "\033[35mNew client connected: fd = " << client_fd << " PORT " << ntohs(client_addr.sin_port) << "\033[0m" << std::endl;
    return 0;
}

void disconnectClient(int fd, std::map<int, Client> &client, int epoll_fd)
{
    std::cout << "\033[31mClient disconnection: fd = " << fd << "\033[0m" <<std::endl;
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

void close_all_fd(std::vector<int> &unique_server_fd, std::map<int, Client> &client)
{
	std::cout << " ---Closing all fd--- "<< std::endl;
	for (size_t i = 0; i < unique_server_fd.size(); ++i)
    {
		close(unique_server_fd[i]);
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
		
		std::vector<SocketBinding> bindings; //vector of binding server
        std::vector<config> conf;
        std::vector<Server> serv;
        std::map<int, Client> client;
        std::string s(argv[1]);

		std::vector<int> unique_server_fd;

        signal(SIGINT, handle_signal);
        signal(SIGTERM, handle_signal);

        fill_configstruct(conf, s);
        create_server_from_config(serv, conf, bindings);

        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1)
        {
            perror("epoll_create1");
            return(-1);
        }

		collect_unique_server_fds(serv, unique_server_fd);

        if (add_server_fd(unique_server_fd, epoll_fd) < 0)
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
                    addClient(fd, client, epoll_fd);
                }
                else if (events[i].events & EPOLLIN)
                {
                    if (client.at(fd).receiveRequest(bindings, serv) <= 0)
                    {
                        disconnectClient(fd, client, epoll_fd);
                        continue;
                    }
                    else if (client.at(fd).getRequest().getComplete() == true)
                    {
                        //client.at(fd).getRequest().setComplete(false);
                        Manage_req::set_response_for_client(client.at(fd));
                        //std::cout << client.at(fd).getresponse() << std::endl; //print repsonse for debug
                        struct epoll_event ev;
                        ev.events = EPOLLOUT;
                        ev.data.fd = fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
                    }
                }
                else if (events[i].events & EPOLLOUT)
                {
                    std::cout << "\033[38;5;208m---Response---" << std::endl << client.at(fd).getresponse().c_str() << std::endl;
                    send(fd, client.at(fd).getresponse().c_str(), client.at(fd).getresponse().size(), 0);
                    std::cout << "---End Sended response---\033[0m" << std::endl;
                    client.at(fd).set_response("");
                    client.at(fd).clearRequest(); // Clear request data for next request
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
                }
            }
        }
        close_all_fd(unique_server_fd, client);
        close(epoll_fd);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception called: " << e.what() << std::endl;
    }
    return 0;
}

/*
const Server* findServerByFd(const std::vector<Server> &servers, int fd)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
        if (servers[i].isServerFd(fd))
            return &servers[i];
    }
    return NULL;
}*/