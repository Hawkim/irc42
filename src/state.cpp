#include "state.h"
#include "server.h"

int    listen_fd;
std::string server_pass;
std::string server_name = "ircserv";

std::vector<struct pollfd> fds;
std::vector<Client*>        clients;
std::map<std::string,Client*>  nick_map;
std::map<std::string,Channel*> channels;

// Global shutdown flag
bool g_server_shutdown = false;

void cleanup_server()
{
    // Close all client connections
    for (size_t i = 0; i < clients.size(); ++i)
	{
        close(clients[i]->fd);
        delete clients[i];
    }
    clients.clear();
    nick_map.clear();

    // Clean up all channels
    for (std::map<std::string,Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
        delete it->second;
    }
    channels.clear();

    // Close server socket
    if (listen_fd != -1)
	{
        close(listen_fd);
        listen_fd = -1;
    }
}
