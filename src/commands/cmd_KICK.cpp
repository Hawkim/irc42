#include "commands.h"
#include "util.h"
#include "state.h"

// KICK <#channel> <nick> [ :reason ]
void cmd_KICK(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 3)
	{
        send_err(c, "461", "KICK", "Not enough parameters");
        std::cout << "KICK: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    const std::string& chanName   = p[1];
    const std::string& targetNick = p[2];
    Channel* ch = get_chan(chanName);

    // Check if client is in the channel
    bool on_chan = false;
    for (size_t i = 0; i < ch->members.size(); ++i)
	{
        if (ch->members[i] == c)
		{
			on_chan = true;
			break;
		}
    }
    if (!on_chan)
	{
        send_err(c, "442", chanName, "You're not on that channel");
        std::cout << "KICK: Client " << c->fd << " not on channel " << chanName << "\n";
        return;
    }

    // Check if client is operator
    if (!ch->operators.count(c->nick))
	{
        send_err(c, "482", chanName, "You're not channel operator");
        std::cout << "KICK: Client " << c->fd << " not operator on " << chanName << "\n";
        return;
    }

    // Check if target exists
    Client* target = find_nick(targetNick);
    if (!target)
	{
        send_err(c, "401", targetNick, "No such nick");
        std::cout << "KICK: No such nick " << targetNick << "\n";
        return;
    }

    // Check if target is in the channel
    bool target_on_chan = false;
    for (size_t i = 0; i < ch->members.size(); ++i)
	{
        if (ch->members[i] == target)
		{
			target_on_chan = true;
			break;
		}
    }
    if (!target_on_chan)
	{
        send_err(c, "441", targetNick + " " + chanName, "They aren't on that channel");
        std::cout << "KICK: Target " << targetNick << " not on channel " << chanName << "\n";
        return;
    }

    // remember if they were an op
    bool tgtWasOp = ch->operators.count(targetNick) > 0;

    // build + broadcast the KICK line
    std::string reason;
    if (p.size() > 3)
	{
		if (p[3][0] == ':')
			reason = " :" + p[3].substr(1);
		else
			reason = " :" + p[3];
	}
    std::string kickMsg = ":" + c->nick + "!" + c->user + "@" + server_name + " KICK " + chanName + " " + targetNick + reason;

    // remove them from channel
    ch->members.erase(std::remove(ch->members.begin(), ch->members.end(), target), ch->members.end());
    ch->operators.erase(targetNick);
    ch->invited.erase(targetNick);

    // notify everyone
    queue_raw(target, kickMsg);
    for (size_t j = 0; j < ch->members.size(); ++j)
        queue_raw(ch->members[j], kickMsg);

    // handle operator handoff if needed
    if (tgtWasOp && !ch->members.empty())
	{
        Client* newOp = ch->members.front();
        ch->operators.insert(newOp->nick);

        std::string modeMsg = ":" + server_name + " MODE " + chanName + " +o " + newOp->nick;
        for (size_t j = 0; j < ch->members.size(); ++j)
            queue_raw(ch->members[j], modeMsg);
    }

    std::cout << "Client " << c->fd << " kicked " << targetNick << " from " << chanName << "\n";
}
