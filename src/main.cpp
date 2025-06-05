#include "server.h"
#include "state.h"
#include <csignal>

// Signal handler for SIGINT
void signal_handler(int signum)
{
    if (signum == SIGINT)
	{
        std::cout << "\nReceived SIGINT, shutting down server...\n";
        g_server_shutdown = true;
    }
}

int main(int argc, char** argv)
{
    if (argc != 3)
	{
        std::cerr<<"Usage: "<<argv[0]<<" <port> <password>\n";
        return 1;
    }

    int port = std::atoi(argv[1]);
    // ignore Ctrl+\, Ctrl+Z, SIGPIPE
    std::signal(SIGQUIT, SIG_IGN);
    std::signal(SIGTSTP, SIG_IGN);
    std::signal(SIGPIPE, SIG_IGN);
    // Handle Ctrl+C
    std::signal(SIGINT, signal_handler);

    init_server(port, argv[2]);
    server_run();
    cleanup_server();
    return 0;
}
