
#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

class Server;

struct route
{
    std::string uri;  // URI for the location (e.g., "/", "/upload")
    std::vector<std::string> allowed_methods;  // GET, POST, DELETE
    std::string redirect;                      // Es: "301 http://..."
    std::string root_directory;
    bool directory_listing;                    // Directory listing
    std::string default_file;
    std::vector<std::string> cgi_extensions;   // ".py" many extension possible
    std::string cgi_path;                      // "/usr/bin/python3"
    std::string upload_path;
};

struct config  //rimepire le struct con parsing del config file
{
    std::vector<std::pair<std::string, int> > listen_por; // IP:PORT coppie
    std::vector<std::string> server_name;
    std::string host;
    std::map<int, std::string> error_pages;
    size_t max_body_size;
    std::vector<route> routes;  //vettore delle routes, ossia informazioni per ogni blocco location del file .config
    class ConfigException : public std::exception
	{
		public:
			virtual const char* what() const throw()
			{
				return ("Wrong number of arguments exception");
			}
	};
};

struct SocketBinding //struct for save each socket 
{
    std::string ip;
    int port;
    int fd;
    //Server* server;
};

void fill_configstruct(std::vector<config> &conf, const std::string &filename);

