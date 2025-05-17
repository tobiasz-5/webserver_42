
#include "Server.hpp"
#include "Client.hpp"

int main()
{
    try
    {
        Server server;
        server.bind_listen();

        std::map<int, Client> client;
        int fdmax = server.getServfd();
        fd_set fds;
        fd_set temp_fds;
        FD_ZERO(&fds);                 // svuota il set
        FD_SET(server.getServfd(), &fds);      // aggiunge il socket del server
        //loop che attende connessione del client
        while (1)
        {
            temp_fds = fds;           // copia il set perchè select lo modifica
            if (select(fdmax + 1, &temp_fds, nullptr, nullptr, nullptr) == -1) //controlla fino a fdmax se c'è fd pronti
            {
                std::cerr << "Error in select()" << std::endl;
                break;
            }
            for (int i = 0; i <= fdmax; ++i)
            {
                if (i == server.getServfd())
                {
                    // Nuova connessione in arrivo
                    sockaddr_in client_addr;
                    socklen_t len = sizeof(client_addr);
                    int client_fd = accept(server.getServfd(), (sockaddr*)&client_addr, &len);
                    if (client_fd != -1)
                    {
                        client.emplace(client_fd, Client(client_fd, client_addr));
                        FD_SET(client_fd, &fds); // Aggiunge il nuovo client al set master
                        if (client_fd > fdmax)
                            fdmax = client_fd; // Aggiorna il valore massimo di file descriptor
                        std::cout << "New client: fd = " << client_fd << std::endl;
                    }
                }
                else
                {
                    // Dati ricevuti da un client esistente
                    char buffer[2048];
                    int bytes_read = recv(i, buffer, sizeof(buffer) - 1, 0); // Riceve i dati dal client
                    if (bytes_read <= 0)
                    {
                        // Il client ha chiuso la connessione o si è verificato un errore
                        std::cout << "Client disconnesso: fd = " << i << std::endl;
                        close(i); // Chiude il socket del client
                        FD_CLR(i, &fds); // Rimuove il client dal set master
                        client.erase(i); // Rimuove il client dal vettore
                    }
                    else
                    {
                        buffer[bytes_read] = '\0'; // Aggiunge il terminatore di stringa
                        std::cout << "Richiesta ricevuta da fd = " << i << ":\n" << buffer << std::endl;
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
        close(server.getServfd());
    }
    catch (const std::exception &e)
	{
		std::cerr << "Exception called: " << e.what() << std::endl;
	}
    return(0);
}
