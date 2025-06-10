
#include "Request.hpp"

Request::Request() : buffer("")
{
}

Request::Request(Request const &other)
{
	*this = other;
}

Request &Request::operator=(Request const &other)
{
    if (this != &other)
        this->buffer = other.buffer;
    return (*this);
}

Request::~Request()
{
}

const std::string &Request::getBuffer(void) const
{
    return(buffer);
}

int Request::receiveData(int fd)
{
    char temp_buffer[2048];
    int bytes_read = recv(fd, temp_buffer, sizeof(temp_buffer) - 1, 0);
    if (bytes_read > 0)
    {
        temp_buffer[bytes_read] = '\0'; // terminatore stringa
        buffer.append(temp_buffer, bytes_read);

        std::cout << "request :\n" << buffer << std::endl; //debug
    }
    else if (bytes_read == 0)
    {
        std::cout << "Client closed connection." << std::endl;
        return 0;
    }
    else
        buffer.clear(); // se errore o chiusura connessione, svuoto il buffer
    std::cout << "byte read :" << bytes_read << std::endl; //debug
    return (bytes_read);
}
