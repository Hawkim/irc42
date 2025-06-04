#include "commands.h"
#include "util.h"
#include "state.h"

// KICK <#channel> <nick> [ :reason ]
void cmd_KICK(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 3)
	{
        send_err(c,"461","KICK","Not enough parameters");
        std::cout << "KICK: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    const std::string& chanName   = p[1];
    const std::string& targetNick = p[2];
    Channel* ch = get_chan(chanName);

    // verify kicker is on channel
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
        send_err(c,"442",chanName,"You're not on that channel");
        std::cout << "KICK: Client " << c->fd << " not on channel " << chanName << "\n";
        return;
    }
    // verify kicker is operator
    if (!ch->operators.count(c->nick))
	{
        send_err(c,"482",chanName,"You're not channel operator");
        std::cout << "KICK: Client " << c->fd << " not operator on " << chanName << "\n";
        return;
    }
    // find target client
    Client* tgt = find_nick(targetNick);
    bool tgt_on_chan = false;
    if (tgt)
	{
        for (size_t i = 0; i < ch->members.size(); ++i)
		{
            if (ch->members[i] == tgt)
			{
                tgt_on_chan = true;
                break;
            }
        }
    }
    if (!tgt || !tgt_on_chan)
	{
        send_err(c,"441",targetNick,"They aren't on that channel");
        std::cout << "KICK: Target " << targetNick << " not on channel " << chanName << "\n";
        return;
    }

    // remember if target was operator
    bool tgtWasOp = (ch->operators.count(targetNick) > 0);

    // build KICK message
    std::string reason;
    if (p.size() > 3)
	{
		if (p[3].size() && p[3][0] == ':')
			reason = " :" + p[3].substr(1);
		else
			reason = " :" + p[3];
    }
    std::string kickMsg = ":" + c->nick + "!" + c->user + "@" + server_name + " KICK " + chanName + " " + targetNick + reason;

    // notify kicked client
    queue_raw(tgt, kickMsg);

    // remove target from channel
    ch->members.erase(std::remove(ch->members.begin(), ch->members.end(), tgt), ch->members.end());
    ch->operators.erase(targetNick);
    ch->invited.erase(targetNick);

    // notify remaining members
    for (size_t i = 0; i < ch->members.size(); ++i)
	{
        queue_raw(ch->members[i], kickMsg);
    }
    std::cout << "Client " << c->fd << " kicked " << targetNick << " from channel " << chanName << "\n";

    // if channel now empty, delete it
    if (ch->members.empty())
	{
        channels.erase(chanName);
        delete ch;
        std::cout << "All users have left channel " << chanName << "; channel will be deleted\n";
        return;
    }

    // operator hand-off if needed
    if (tgtWasOp && !ch->members.empty())
	{
        Client* newOp = ch->members.front();
        ch->operators.insert(newOp->nick);

        std::string modeMsg = ":" + server_name + " MODE " + chanName + " +o " + newOp->nick;
        for (size_t j = 0; j < ch->members.size(); ++j)
		{
            queue_raw(ch->members[j], modeMsg);
        }
        std::cout << "KICK: handed operator to " << newOp->nick << " on channel " << chanName << "\n";
    }
}
