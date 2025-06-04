#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <csignal>
#include <iostream>
#include <cstdlib>

// initialize and run the server
void init_server(int port, const std::string& pass);
void server_run();

#endif
