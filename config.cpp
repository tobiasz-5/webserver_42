
#include "config.hpp"
#include "Server.hpp"
#include "utils.hpp"

void fill_route(route &current_route, const std::vector<std::string> &tokens)
{
    if (tokens[0] == "allowed_methods")
    {
        current_route.allowed_methods.assign(tokens.begin() + 1, tokens.end());
    }
    else if (tokens[0] == "directory_listing")
        current_route.directory_listing = (tokens[1] == "on");
    else if (tokens[0] == "default_file")
        current_route.default_file = tokens[1];
    else if (tokens[0] == "cgi_extension")
    {
        current_route.cgi_extensions.assign(tokens.begin() + 1, tokens.end());
    }
    else if (tokens[0] == "cgi_path")
        current_route.cgi_path = tokens[1];
    else if (tokens[0] == "upload_path")
        current_route.upload_path = tokens[1];
	else if (tokens[0] == "root_directory")
        current_route.root_directory = tokens[1];
	else if (tokens[0] == "redirect")
        current_route.redirect = tokens[1];
}

void fill_config(config &current_config, const std::vector<std::string> &tokens)
{
    if (tokens[0] == "listen")
    {
        std::string addr_port = tokens[1];
        if (!addr_port.empty() && addr_port[addr_port.size() - 1] == ';')
            addr_port.erase(addr_port.size() - 1);
        size_t colon_pos = addr_port.find(':');
        if (colon_pos == std::string::npos)
        {
            current_config.listen_address = "0.0.0.0";
            int port = to_int(addr_port);
            current_config.ports.push_back(port);
        }
        else
        {
            current_config.listen_address = addr_port.substr(0, colon_pos);
            int port = to_int(addr_port.substr(colon_pos + 1));
            current_config.ports.push_back(port);
        }
    }
    else if (tokens[0] == "server_name")
        current_config.server_name = tokens[1];
    else if (tokens[0] == "host")
        current_config.host = tokens[1];
    else if (tokens[0] == "max_body_size")
        current_config.max_body_size = to_long(tokens[1]);
    else if (tokens[0] == "error_page")
    {
        int err = to_int(tokens[1]);
        current_config.error_pages[err] = tokens[2]; //inserisce nella mappa (se gia presente sovrascrive)
    }
}

std::vector<std::string> divide_location_line(const std::string &line)
{
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string word;

    while (iss >> word)
    {
        if (!word.empty() && word[word.size() - 1] == '{') // Remove trailing '{' if present
		{
            word.erase(word.size() - 1);
        }
        tokens.push_back(word);
    }
    return tokens;
}

route parse_location_block(std::istream &in, const std::string &uri)
{
    route current_route;
    current_route.uri = uri;
    std::string line;
    while (std::getline(in, line))
    {
        line = trim_space(line);
        if (line.empty() || line[0] == '#')
			continue;
        if (line == "}")
			return current_route;
        std::vector<std::string> tokens = divide_words(line);
        if (!tokens.empty())
            fill_route(current_route, tokens);
    }
    throw std::runtime_error("Unclosed location block");
}

config parse_server_block(std::istream &in)
{
    config current_config;
    std::string line;
    while (std::getline(in, line))
    {
        line = trim_space(line);
        if (line.empty() || line[0] == '#')
			continue;
        if (line == "}")
			return current_config;
        if (line.rfind("location ", 0) == 0 && line[line.size() - 1] == '{')
		{
            std::vector<std::string> tokens = divide_location_line(line);
            if (tokens.size() < 2)
                throw std::runtime_error("Invalid location block");
            route r = parse_location_block(in, tokens[1]);
            current_config.routes.push_back(r);
        }
		else
		{
            std::vector<std::string> tokens = divide_words(line);
            if (!tokens.empty())
                fill_config(current_config, tokens);
        }
    }
    throw std::runtime_error("Unclosed server block");
}

void fill_configstruct(std::vector<config> &conf, const std::string &filename)
{
    std::ifstream infile(filename.c_str());
    if (!infile.is_open())
        throw std::runtime_error("Can not open config file: " + filename);
    std::string line;
    while (std::getline(infile, line))
    {
        line = trim_space(line);
        if (line.empty() || line[0] == '#') continue;
        if (line == "server {")
        {
            config server_conf = parse_server_block(infile);
            conf.push_back(server_conf);
        }
        else
        {
            throw std::runtime_error("Unexpected content outside of server block: " + line);
        }
    }
    infile.close();
    if (conf.empty())
        throw std::runtime_error("No server block in config file");
    print_config(conf);
}

void print_config(const std::vector<config> &conf_list)
{
    std::cout << "\033[36m READ FORM CONFIG FILE" << std::endl;
    for (size_t i = 0; i < conf_list.size(); ++i)
    {
        const config &c = conf_list[i];
        std::cout << "=== Config for Server " << i + 1 << " ===\n";
        std::cout << "Listen Address: " << c.listen_address << "\n";
        std::cout << "Ports: ";
        for (size_t j = 0; j < c.ports.size(); ++j)
        {
            std::cout << c.ports[j];
            if (j != c.ports.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        std::cout << "Server Name: " << c.server_name << "\n";
        std::cout << "Host: " << c.host << "\n";
        std::cout << "Max Body Size: " << c.max_body_size << "\n";
        std::cout << "Error Pages:\n";
        for (std::map<int, std::string>::const_iterator it = c.error_pages.begin(); it != c.error_pages.end(); ++it)
        {
            std::cout << "  " << it->first << " => " << it->second << "\n";
        }
        std::cout << "Routes:\n";
        for (size_t r = 0; r < c.routes.size(); ++r)
        {
            const route &route = c.routes[r];
            std::cout << "  --- Route " << r + 1 << " ---\n";
			std::cout << "  URI: " << route.uri << "\n";
            std::cout << "  Allowed Methods: ";
            for (size_t m = 0; m < route.allowed_methods.size(); ++m)
            {
                std::cout << route.allowed_methods[m];
                if (m != route.allowed_methods.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
            std::cout << "  Redirect: " << route.redirect << "\n";
            std::cout << "  Directory Listing: " << (route.directory_listing ? "on" : "off") << "\n";
            std::cout << "  Default File: " << route.default_file << "\n";
            std::cout << "  CGI Extension: " ;
            for (size_t m = 0; m < route.cgi_extensions.size(); ++m)
            {
                std::cout << route.cgi_extensions[m];
                if (m != route.cgi_extensions.size() - 1) std::cout << ", ";
            }
            std::cout << "  CGI Path: " << route.cgi_path << "\n";
            std::cout << "  Upload Path: " << route.upload_path << "\n";
			std::cout << "  Root directory: " << route.root_directory << "\n";
        }
        std::cout << "\n";
    }
    std::cout << "END FROM CONFIG FILE" << std::endl;
    std::cout << "------------------------------- \033[0m" << std::endl;
}

/*
void fill_configstruct(std::vector<config> &conf, const std::string &filename)
{
    std::ifstream infile(filename.c_str());
    if (!infile.is_open())
        throw std::runtime_error("Can not open config file: " + filename);
    std::string line;
    config current_config;
    route current_route;
    bool in_server = false;
    bool in_location = false;
    while (std::getline(infile, line))
    {
        line = trim_space(line); //SOLO space?
        if (line.empty() || line[0] == '#')
            continue;
        if (line.find("server {") != std::string::npos) //for now, controlla se trova blocco server su linea
        {
            in_server = true;
            current_config = config(); //struct
            continue;
        }
        if (line.find("location ") != std::string::npos) //for now
        {
            in_location = true;
            current_route = route();
            std::vector<std::string> location_line = divide_location_line(line);
            current_route.uri = location_line[1]; //primo token dopo location
            continue;
        }
        if (line == "}" && in_location)
        {
            in_location = false;
            current_config.routes.push_back(current_route);
            continue;
        }
        if (line == "}" && in_server)
        {
            in_server = false;
            conf.push_back(current_config);
            continue;
        }
        std::vector<std::string> tokens = divide_words(line);
        if (tokens.empty())
            continue;
        if (in_location)
            fill_route(current_route, tokens);
        else if (in_server)
            fill_config(current_config, tokens);
    }
    infile.close();
    if (conf.size() < 1)
        throw std::runtime_error("No server block in config file");
    print_config(conf); //stampa le struct config, for debug
}*/

//old one test
/*
void fill_configstruct(std::vector<config> &conf, std::string s)
{
    struct config s1;
    s1.listen_address = "0.0.0.0";
    s1.ports.push_back(8080);
    s1.ports.push_back(8001);
    s1.server_name = "sito_1.com";
    s1.host = "127.0.0.1";
    s1.error_pages.insert(std::make_pair(404, "/errors/404.html"));
    s1.error_pages.insert(std::make_pair(405, "/errors/405.html"));
    s1.max_body_size = 1024;
    conf.push_back(s1);

    struct config s2;
    s2.listen_address = "0.0.0.0";
    s2.ports.push_back(8002);
    s2.ports.push_back(8003);
    s2.server_name = "sito_2.com";
    s2.host = "0.0.0.0";
    s2.error_pages.insert(std::make_pair(404, "/errors/404.html"));
    s2.error_pages.insert(std::make_pair(405, "/errors/405.html"));
    s2.max_body_size = 1024;
    conf.push_back(s2);
}*/