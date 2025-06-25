#pragma once
#include "Server.hpp"
#include "Client.hpp"

const Server* findMatchedServer(const std::vector<Server>& servers, int fd);
void set_response_for_client(Client &client, std::vector<Server> &server);