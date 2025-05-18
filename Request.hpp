#pragma once

#include <iostream>
#include <unistd.h>
#include <string.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/select.h>

class Request
{
    private:
        std::string buffer;
    public:
        Request();
        Request(Request const &other);
		Request &operator=(Request const &other);
        ~Request();
        const std::string &getBuffer(void) const;
        int receiveData(int fd);
};
