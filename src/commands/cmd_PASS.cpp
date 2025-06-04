#include "commands.h"
#include "util.h"
#include "state.h"

// PASS <password>
void cmd_PASS(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 2)
	{
        send_err(c,"461","PASS","Not enough parameters");
        std::cout << "PASS: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    if (c->got_pass)
	{
        send_err(c,"462","PASS","You may not re-register");
        std::cout << "PASS: Already registered (client " << c->fd << ")\n";
        return;
    }
    if (p[1] != server_pass)
	{
        send_err(c,"464","PASS","Password incorrect");
        std::cout << "PASS rejected: wrong password (client " << c->fd << ")\n";
        return;
    }
    c->got_pass = true;
    send_rpl(c,"381","*","Password accepted");
    std::cout << "PASS accepted from client " << c->fd << "\n";
}
