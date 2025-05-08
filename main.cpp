
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#define PORT 8080

int main()
{
    //create server socket
    int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_fd < 0)
    {
        std::cout << "error in creation socket" << std::endl;
        return(1);
    }

    //creo struttura per dati dedl server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //lega il socket creato all'indirizzo IP e alla porta specificati
    if (bind(serv_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cout << "bind error" << std::endl;
        close(serv_fd);
        return(1);
    }

    //ascolto connessioni
    if (listen(serv_fd, 10) < 0)
    {
        std::cout << "listen error" << std::endl;
        close(serv_fd);
        return(1);
    }

    //loop che attende connessione del client
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        std::cout << "Waiting for connection:\n" << "\n" << std::endl;

        int client_fd = accept(serv_fd, (sockaddr *)&client_addr, &client_len);
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
            "<html><body><h1>Hello, World!,Dio cat</h1></body></html>\n";  // Corpo HTML della risposta
    
            send(client_fd, response, strlen(response), 0);
            //close(client_fd);
        }
    }
    close(serv_fd);
    return(0);
}
