
#pragma once

#include <string>
#include <vector>
#include <map>

class Server;

struct Route
{
    std::vector<std::string> allowed_methods;  // GET, POST, DELETE
    std::string redirect;                      // Es: "301 http://..."
    bool directory_listing;                    // Directory listing
    std::string default_file;
    std::string cgi_extension;                 // ".py"
    std::string cgi_path;                      // "/usr/bin/python3"
    std::string upload_path;
};

struct config  //rimepire le struct con parsing del config file
{
    std::string listen_address;
    std::vector<int> ports;
    std::string server_name;
    std::string host;
    std::map<int, std::string> error_pages;
    size_t max_body_size;
    std::vector<Route> routes;  //vettore delle routes, ossia informazioni per ogni blocco location del file .config
};

void fill_configstruct(std::vector<config> &conf);
void create_server_from_config(std::vector<Server> &serv, const std::vector<config> &conf);