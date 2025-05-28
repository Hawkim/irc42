#include "server.h"
#include <csignal>
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr<<"Usage: "<<argv[0]<<" <port> <password>\n";
        return 1;
    }
    int port = std::atoi(argv[1]);
    // ignore Ctrl+\, Ctrl+Z, SIGPIPE
    std::signal(SIGQUIT, SIG_IGN);
    std::signal(SIGTSTP, SIG_IGN);
    std::signal(SIGPIPE, SIG_IGN);

    init_server(port, argv[2]);
    server_run();
    return 0;
}
