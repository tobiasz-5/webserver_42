
#include "Server.hpp"
#include "Client.hpp"

#include <poll.h>
#include <vector>
#include <fcntl.h>

int addClient(const Server &server, std::map<int, Client> &client, std::vector<pollfd> &fds)  // New connection for client
{
	sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	int client_fd = accept(server.getServfd(), (sockaddr*)&client_addr, &len);
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
	//it = fds.erase(it); // Rimuove fd del client dal vector di poll
	return(fds.erase(it));
}

const char *Response(int fd)  //temporary function
{
	std::cout << "request received from fd = " << fd << std::endl;
	// Prepara la risposta HTTP
	const char *response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"<html><body><h1> I am server </h1></body></html>\n";
	return(response);
}

int main()
{
	try
	{
		Server server;
		server.bind_listen();

		std::map<int, Client> client;

		std::vector<pollfd> fds;
		// Aggiunta del server socket
		pollfd server_pollfd;
		server_pollfd.fd = server.getServfd();
		server_pollfd.events = POLLIN;
		fds.push_back(server_pollfd);

		while (1)  //loop che attende connessioni e richieste client
		{
			int ret = poll(fds.data(), fds.size(), -1);
			if (ret < 0)
			{
				std::cerr << " poll failed " << std::endl;
				break;
   			}
			std::vector<pollfd>::iterator it = fds.begin();
			for (; it != fds.end();)
			{
				if (it->revents & POLLIN)
				{
					if (it->fd == server.getServfd())
					{
						addClient(server, client, fds);
						it++;
					}
					else
					{
						if (client.at(it->fd).receiveRequest() <= 0)
							it = disconnectClient(it, fds, client);
						else
						{
							const char *response = Response(it->fd);
							send(it->fd, response, strlen(response), 0); // Invia la risposta al client
							it++;
						}
					}
				}
				else
					it++;
			}
		}
		std::map<int, Client>::iterator it = client.begin();
		for (; it != client.end(); it++)  //close all client file descriptor
			close(it->first);
		close(server.getServfd());    // close server file descriptor
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception called: " << e.what() << std::endl;
	}
	return(0);
}

/* old one with select

int addClient(const Server &server, std::map<int, Client> &client, int &fdmax, fd_set &fds)  // New connection for client
{
	sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	int client_fd = accept(server.getServfd(), (sockaddr*)&client_addr, &len);
	if (client_fd == -1)
	{
		std::cerr << "Errore in accept(): " << std::endl;
		return(-1);
	}
	else
	{
		client.insert(std::make_pair(client_fd, Client(client_fd, client_addr)));
		FD_SET(client_fd, &fds); // Aggiunge il nuovo client al set master
		if (client_fd > fdmax)
			fdmax = client_fd; // Aggiorna il valore massimo di file descriptor
		std::cout << "New client: fd = " << client_fd << " PORT " << ntohs(client_addr.sin_port) << std::endl;
	}
	return(0);
}

void disconnectClient(int i, fd_set &fds, std::map<int, Client> &client) // Il client ha chiuso la connessione o si è verificato un errore
{
	std::cout << "Client disconnected: fd = " << i << std::endl;
	close(i); // Chiude il socket del client
	FD_CLR(i, &fds); // Rimuove il client dal set master
	client.erase(i); // Rimuove il client dal vettore
}

const char *Response(int i)  //temporary function
{
	std::cout << "request received from fd = " << i << std::endl;
	// Prepara la risposta HTTP
	const char *response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"<html><body><h1>I am server</h1></body></html>\n";
	return(response);
}

int main()
{
	try
	{
		Server server;
		server.bind_listen();

		std::map<int, Client> client;
		int fdmax = server.getServfd();
		fd_set fds, temp_fds;
		FD_ZERO(&fds);                           // svuota il set
		FD_SET(server.getServfd(), &fds);       // aggiunge il socket del server
		while (1)                              //loop che attende connessioni e richieste client
		{
			temp_fds = fds;           // copia il set perchè select lo modifica
			if (select(fdmax + 1, &temp_fds, NULL, NULL, NULL) == -1) //controlla fino a fdmax se c'è fd pronti
			{
				std::cerr << "Error in select()" << std::endl;
				break;
			}
			for (int i = 0; i <= fdmax; ++i)
			{
				if (!FD_ISSET(i, &temp_fds))    // verify if fd is ready
					continue; 					// salta i fd non pronti
				if (i == server.getServfd())
				{
					if (addClient(server, client, fdmax, fds) < 0)
						continue;
				}
				else
				{
					if (client.at(i).receiveRequest() <= 0)
						disconnectClient(i, fds, client);
					else
					{
						const char *response = Response(i);
						send(i, response, strlen(response), 0); // Invia la risposta al client
					}
				}
			}
		}
		std::map<int, Client>::iterator it = client.begin();
		for (; it != client.end(); it++)  //close all client file descriptor
			close(it->first);
		close(server.getServfd());    // close server file descriptor
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception called: " << e.what() << std::endl;
	}
	return(0);
}
*/
