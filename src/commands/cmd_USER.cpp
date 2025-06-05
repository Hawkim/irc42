#include "commands.h"
#include "util.h"
#include "state.h"

// USER <username> 0 * :<realname>
void cmd_USER(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 5)
	{
        send_err(c, "461", "USER", "Not enough parameters");
        std::cout << "USER: Not enough parameters for client " << c->fd << "\n";
        return;
    }

    if (c->registered)
	{
        send_err(c, "462", "USER", "You may not re-register");
        std::cout << "USER: Already registered (client " << c->fd << ")\n";
        return;
    }

    c->user = p[1];
    c->realname = p[4];

    if (!c->got_pass || c->nick.empty())
	{
        send_err(c, "451", "*", "You have not registered");
        std::cout << "USER: client " << c->fd << " missing PASS/NICK\n";
        return;
    }

    c->registered = true;

    queue_raw(c, ":" + server_name + " 001 " + c->nick + " :Welcome to the IRC server: " + server_name);

    std::cout << "Client " << c->fd << " registered with username: " << c->user << "\n";
    std::cout << "Welcome sent to " << c->nick << "\n";
}
