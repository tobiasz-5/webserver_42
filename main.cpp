#include "config.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "response.hpp"

#include <poll.h>
#include <vector>
#include <fcntl.h>
#include <signal.h>

#define MAX_CLIENT 1024

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

void add_server_fd(const std::vector<Server> &serv, std::vector<pollfd> &fds)
{
	for (size_t s = 0; s < serv.size(); ++s) // Aggiunta di tutti gli fd di tutti i server alla struttura pollfd
	{
		for (size_t i = 0; i < serv[s].getnumport(); ++i)
		{
			pollfd server_pollfd;
			server_pollfd.fd = serv[s].getServfd(i);
			server_pollfd.events = POLLIN; //aggiungo evento da monitorare
			fds.push_back(server_pollfd);
		}
	}
	return;
}

int addClient(int server_fd, std::map<int, Client> &client, std::vector<pollfd> &fds)  // New connection for client
{
	sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	int client_fd = accept(server_fd, (sockaddr*)&client_addr, &len);
	if (client_fd == -1)
	{
		std::cerr << "Error in accept: " << std::endl;
		return(-1);
	}
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    fds.push_back(client_pollfd);
	client.insert(std::make_pair(client_fd, Client(client_fd, client_addr)));

	std::cout << "New client: fd = " << client_fd << " PORT " << ntohs(client_addr.sin_port) << std::endl;
	return(0);
}

std::vector<pollfd>::iterator disconnectClient(std::vector<pollfd>::iterator it, std::vector<pollfd> &fds, std::map<int, Client> &client) // Il client ha chiuso la connessione o si è verificato un errore
{
	std::cout << "Client disconnected: fd = " << it->fd << std::endl;
	close(it->fd);
	client.erase(it->fd); // Rimuove il client dal vettore
	it = fds.erase(it); // Rimuove fd del client dal vector di poll
	return(it);
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
		std::vector<config> conf;  //vettore con dati di ogni server, preso da file di config
		std::vector<Server> serv; //crea vettore di server con dati di config
		std::map<int, Client> client; //map per salvare i client
		std::vector<pollfd> fds; //vettore dei file descriptor da monitorare
		std::string s(argv[1]); //passa nome del file
		signal(SIGINT, handle_signal);
    	signal(SIGTERM, handle_signal);

		fill_configstruct(conf, s); //riempie struct dal file
		create_server_from_config(serv, conf);
		fds.reserve(MAX_CLIENT);
		add_server_fd(serv, fds); // Aggiunta di tutti gli fd di tutti i server alla struttura pollfd
		while (stop == 0)  //loop principale che attende connessioni e richieste client
		{
			int ret = poll(fds.data(), fds.size(), -1);
			if (ret < 0)
			{
				if (errno == EINTR)
					continue;
				else
				{
					std::cerr << "poll failed: " << strerror(errno) << std::endl;
					break;
				}
   			}
			for (std::vector<pollfd>::iterator it = fds.begin(); it != fds.end();)
			{
				if (it->revents & (POLLHUP | POLLERR | POLLNVAL))
				{
					std::cout << "Client disconnected or error: fd = " << it->fd << std::endl;
					it = disconnectClient(it, fds, client);
					continue;
				}
				if (it->revents & POLLIN)
				{
					if (isAnyServerFd(serv, it->fd))
					{
						addClient(it->fd, client, fds);
						++it;
					}
					else
					{
						if (client.at(it->fd).receiveRequest() <= 0)
						{
							it = disconnectClient(it, fds, client);
							continue;
						}
						else
						{
							const Request &request = client.at(it->fd).getRequest();

							std::string resourcePath = request.getUri();
							std::cout << "=====URI = " << resourcePath << std::endl;
							//std::cout << "======METHOD: " << request.getMethod() << std::endl;
							client.at(it->fd).set_response(the_response(request));
							//std::cout << "[DEBUG] RISPOSTA DA INVIARE:\n" << client.at(it->fd).getresponse() << std::endl;
							it->events |= POLLOUT;
							++it;
						}
					}
				}
				else if (it->revents & POLLOUT)
				{
					//std::cout << "RISPOSTA" << client.at(it->fd).getresponse().c_str() << std::endl;
					send(it->fd, client.at(it->fd).getresponse().c_str(), client.at(it->fd).getresponse().size(), 0); // Invia la risposta al client
					client.at(it->fd).set_response(""); //svuota risposta dopo averla mandata
					it->events &= ~POLLOUT;
				}
				else
					++it;
			}
		}
		close_all_fd(serv, client);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception called: " << e.what() << std::endl;
	}
	return(0);
}
