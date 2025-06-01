
#pragma once

#include <string>
#include <vector>

class Server;

struct config  //rimepire le struct con parsing del config file
{
    std::string listen_address;
    std::vector<int> ports;
    std::string server_name;
    std::string host;
};

void fill_configstruct(std::vector<config> &conf);
void create_server_from_config(std::vector<Server> &serv, const std::vector<config> &conf);