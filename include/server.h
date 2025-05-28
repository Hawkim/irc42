#ifndef SERVER_H
#define SERVER_H

#include <string>

// initialize and run the server
void init_server(int port, const std::string& pass);
void server_run();

#endif
