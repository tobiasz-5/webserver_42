#pragma once
#include <string>

struct route;
class Client;

bool isCgiRequest(const std::string& uri, const route& r);

std::string runCgi(const Client& cli,
                   const route& r,
                   const std::string& scriptPath,
                   const std::string& body);
