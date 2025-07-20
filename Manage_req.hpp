#pragma once

#include "Server.hpp"
#include "Client.hpp"
#include "utils.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctime>
#include <iomanip>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <limits.h> 
#include <stdlib.h> 

class Manage_req
{
    public:
		static void set_response_for_client(Client &client);
		static std::string handle_request(std::string uri, const route &rt, std::string method, const Client &cli, const Server &srv);

		static bool isCgiRequest(const std::string& uri, const route& r);
		static std::string handle_cgi(const std::string& pathOnly, const route& rt, const std::string& method, const Client& cli, const Server& srv);
		static std::string runCgi(const Client& cli, const route&  r, const std::string& scriptPathRel, const std::string& body);
		static std::vector<char*> buildEnv(const Client& cli, const std::string& scriptPath, const std::string& body);

		static std::string handle_directory(const std::string& uri, const std::string& filePath, const route& rt, const Server& srv);

		static std::string generate_error_response(int code, const Server &serv);
		static void debug_message(const std::string &uri, const route &matched_route, const std::string &relativePath, const std::string &filePath, const std::string &requested_method, bool methodAllowed, const std::string &upload_path);
};
