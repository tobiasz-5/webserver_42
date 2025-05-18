
#include "Server.hpp"
#include "Client.hpp"

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
						std::cout << "Richiesta ricevuta da fd = " << i << std::endl;
						// Prepara la risposta HTTP
						const char *response =
							  "HTTP/1.1 200 OK\r\n"
								"Content-Type: text/html\r\n"
								"Connection: keep-alive\r\n"
								"\r\n"
								"<html><body><h1>I am server</h1></body></html>\n";
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
