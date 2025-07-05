#pragma once
#include "Server.hpp"
#include "Client.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

void set_response_for_client(Client &client);