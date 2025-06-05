#include "commands.h"
#include "util.h"
#include "state.h"

// NICK <nickname>
void cmd_NICK(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 2 || p[1].empty())
	{
        send_err(c,"431","*","No nickname given");
        std::cout << "NICK: No nickname given for client " << c->fd << "\n";
        return;
    }

    const std::string& newnick = p[1];

    if (find_nick(newnick))
	{
        send_err(c, "433", newnick, "Nickname is already in use");
        std::cout << "NICK: " << newnick << " already in use\n";
        return;
    }

    if (!c->nick.empty())
		nick_map.erase(c->nick);

    c->nick = newnick;
    nick_map[newnick] = c;
    std::cout << "Client " << c->fd << " set nickname to " << newnick << "\n";
}
