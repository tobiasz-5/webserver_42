
#include "Server.hpp"

int main()
{
    try
    {
        Server server;
        server.bind_listen();
        //loop che attende connessione del client
        while (1)
        {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            std::cout << "Waiting for connection:\n" << "\n" << std::endl;

            int client_fd = accept(server.getServfd(), (sockaddr *)&client_addr, &client_len);
            if (client_fd < 0)
            {
                std::cerr << "accept error\n";
                continue;
            }
            
            char buffer[2048];
            int bytes_read = recv(client_fd, buffer, 1024, 0); //legge 1024 byte dal socket e li mette in buffer
            if (bytes_read > 0)
            {
                buffer[bytes_read] = '\0'; //mette terminatore nullo dopo ultimo byte
                std::cout << "Richiesta del client:\n" << buffer << "\n" << std::endl; //stampa su terminale la richiesta del browser

                const char *response = 
                "HTTP/1.1 200 OK\r\n"  // Codice di stato HTTP
                "Content-Type: text/html\r\n"  // Tipo di contenuto
                "Connection: keep-alive\r\n"  // Connessione chiusa dopo la risposta
                "\r\n"  // Linea vuota tra gli header e il corpo
                "<html><body><h1>I am server</h1></body></html>\n";  // Corpo HTML della risposta
        
                send(client_fd, response, strlen(response), 0);
                //close(client_fd);
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
