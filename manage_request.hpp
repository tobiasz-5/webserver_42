#pragma once
#include "Server.hpp"
#include "Client.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctime>
#include <iomanip>

void set_response_for_client(Client &client);