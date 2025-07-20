#pragma once

#include "Server.hpp"
#include "Client.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctime>
#include <iomanip>
#include "utils.hpp"

std::string handle_post_upload_multipart(const std::string& uploadDir, const std::string& body, const std::string& content_type);
std::string handle_post_urlencoded(const std::string &body);
