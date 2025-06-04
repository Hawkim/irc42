#include "commands.h"
#include "util.h"
#include "state.h"

// INVITE <nick> <#channel>
void cmd_INVITE(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 3)
	{
        send_err(c, "461", "INVITE", "Not enough parameters");
        std::cout << "INVITE: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    const std::string& nick    = p[1];
    const std::string& chanName= p[2];
    Channel* ch = get_chan(chanName);

    bool on_chan = false;
    for (size_t i = 0; i < ch->members.size(); ++i)
        if (ch->members[i] == c)
		{
			on_chan = true;
			break;
		}
    if (!on_chan)
	{
        send_err(c, "442", chanName, "You're not on that channel");
        std::cout << "INVITE: Client " << c->fd << " not on channel " << chanName << "\n";
        return;
    }
    if (!ch->operators.count(c->nick))
	{
        send_err(c, "482", chanName, "You're not channel operator");
        std::cout << "INVITE: Client " << c->fd << " not operator on " << chanName << "\n";
        return;
    }
    Client* tgt = find_nick(nick);
    if (!tgt)
	{
        send_err(c, "401", nick, "No such nick");
        std::cout << "INVITE: No such nick " << nick << "\n";
        return;
    }

    ch->invited.insert(nick);
    std::string msg = ":" + c->nick + "!" + c->user + "@" + server_name + " INVITE " + nick + " :" + chanName;
    queue_raw(c, msg);
    queue_raw(tgt, msg);

    std::cout << "Client " << c->fd << " invited " << nick << " to channel " << chanName << "\n";
}
