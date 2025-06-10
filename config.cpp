
#include "config.hpp"
#include "Server.hpp"

// Funzione trim: rimuove spazi iniziali e finali
static std::string trim(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace(s[start])) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(s[end-1])) --end;
    return s.substr(start, end - start);
}

// Funzione split: divide una stringa in parole separate da spazi
static std::vector<std::string> split(const std::string &str)
{
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string word;
    while (iss >> word)
    {
        if (!word.empty() && word[word.size() - 1] == ';')
            word.erase(word.size() - 1); // rimuove ';' finale
        tokens.push_back(word);
    }
    return tokens;
}

static int toInt(const std::string &s)
{
    int val = 0;
    std::istringstream iss(s);
    iss >> val;
    if (iss.fail())
        throw std::runtime_error("Conversione a int fallita: " + s);
    return val;
}

static unsigned long toULong(const std::string &s)
{
    unsigned long val = 0;
    std::istringstream iss(s);
    iss >> val;
    if (iss.fail())
        throw std::runtime_error("Conversione a unsigned long fallita: " + s);
    return val;
}

void fill_configstruct(std::vector<config> &conf, const std::string &filename)
{
    std::ifstream infile(filename.c_str());
    if (!infile.is_open())
        throw std::runtime_error("Impossibile aprire file di configurazione.");
    std::string line;
    config current_config;
    Route current_route;
    bool in_server = false;
    bool in_location = false;
    while (std::getline(infile, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line.find("server {") != std::string::npos) //for now
        {
            in_server = true;
            current_config = config();
            continue;
        }
        if (line.find("location /") != std::string::npos) //for now
        {
            in_location = true;
            current_route = Route();
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
        std::vector<std::string> tokens = split(line);
        if (tokens.empty())
            continue;
        if (in_location)
        {
            if (tokens[0] == "allowed_methods")
                current_route.allowed_methods.assign(tokens.begin() + 1, tokens.end());
            else if (tokens[0] == "directory_listing")
                current_route.directory_listing = (tokens[1] == "on");
            else if (tokens[0] == "default_file")
                current_route.default_file = tokens[1];
            else if (tokens[0] == "cgi_extension")
                current_route.cgi_extension = tokens[1];
            else if (tokens[0] == "cgi_path")
                current_route.cgi_path = tokens[1];
            else if (tokens[0] == "upload_path")
                current_route.upload_path = tokens[1];
        }
        else if (in_server)
        {
            if (tokens[0] == "listen")
            {
                std::string addr_port = tokens[1];

                // In C++98, niente .back(), si usa .size() e []
                if (!addr_port.empty() && addr_port[addr_port.size() - 1] == ';')
                    addr_port.erase(addr_port.size() - 1);

                size_t colon_pos = addr_port.find(':');
                if (colon_pos == std::string::npos)
                {
                    // Solo porta, senza indirizzo
                    current_config.listen_address = "0.0.0.0";
                    int port = toInt(addr_port);
                    current_config.ports.push_back(port);
                }
                else
                {
                    current_config.listen_address = addr_port.substr(0, colon_pos);
                    int port = toInt(addr_port.substr(colon_pos + 1));
                    current_config.ports.push_back(port);
                }
            }
            else if (tokens[0] == "server_name")
                current_config.server_name = tokens[1];
            else if (tokens[0] == "host")
                current_config.host = tokens[1];
            else if (tokens[0] == "max_body_size")
                current_config.max_body_size = toULong(tokens[1]);
            else if (tokens[0] == "error_page")
            {
                int err = toInt(tokens[1]);
                current_config.error_pages[err] = tokens[2];
            }
        }
    }
    infile.close();
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

void print_config(const std::vector<config> &conf_list)
{
    std::cout << "inizio" << std::endl;
    for (size_t i = 0; i < conf_list.size(); ++i)
    {
        const config &c = conf_list[i];
        std::cout << "=== Server " << i + 1 << " ===\n";
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
            const Route &route = c.routes[r];
            std::cout << "  --- Route " << r + 1 << " ---\n";
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
            std::cout << "  CGI Extension: " << route.cgi_extension << "\n";
            std::cout << "  CGI Path: " << route.cgi_path << "\n";
            std::cout << "  Upload Path: " << route.upload_path << "\n";
        }
        std::cout << "\n";
    }
}

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