#include "state.h"

int    listen_fd;
std::string server_pass;
std::string server_name = "ircserv";

std::vector<struct pollfd> fds;
std::vector<Client*>        clients;
std::map<std::string,Client*>  nick_map;
std::map<std::string,Channel*> channels;
