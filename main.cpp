
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main
{
    int serv_fd;
    if (serv_fd = socket(AF_INET, SOCK_STREAM, 0) < 0)
    {
        std::cout << "error in creation socket" << std::endl;
        return(1);
    }
    struct sockaddr_in s;
    s.sin_family = AF_INET;
    s.sin_port = htons(PORT);
    s.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serv_fd, (struct sockaddr_in *)&s, sizeof(s)) < 0)
    {
        std::cout << "bind error" << std::endl;
        return(1);
    }
    return(0);
}