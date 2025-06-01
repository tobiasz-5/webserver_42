
#include "config.hpp"
#include "Server.hpp"

void fill_configstruct(std::vector<config> &conf)
{
    struct config s1;
    s1.listen_address = "0.0.0.0";
    s1.ports.push_back(8080);
    s1.ports.push_back(8001);
    s1.server_name = "sito_1.com";
    s1.host = "127.0.0.1";
    conf.push_back(s1);

    struct config s2;
    s2.listen_address = "0.0.0.0";
    s2.ports.push_back(8002);
    s2.ports.push_back(8003);
    s2.server_name = "sito_2.com";
    s2.host = "0.0.0.0";
    conf.push_back(s2);
}

void create_server_from_config(std::vector<Server> &serv, const std::vector<config> &conf)
{
    size_t i=0;
    while(i < conf.size())
    {
        //Server s(conf.at(i));
        serv.push_back(Server(conf.at(i)));
        serv.at(i).bind_listen();
        i++;
    }
}
