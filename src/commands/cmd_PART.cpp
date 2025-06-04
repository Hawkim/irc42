#include "commands.h"
#include "util.h"
#include "state.h"

// PART <#channel>
void cmd_PART(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 2)
	{
        send_err(c,"461","PART","Not enough parameters");
        std::cout << "PART: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    const std::string& chanName = p[1];
    Channel* ch = get_chan(chanName);

    bool wasOp = false;
    // remove client from channel members
    bool found = false;
    for (size_t i = 0; i < ch->members.size(); ++i)
	{
        if (ch->members[i] == c)
		{
            ch->members.erase(ch->members.begin() + i);
            found = true;
            break;
        }
    }
    if (!found)
	{
        send_err(c,"442",chanName,"You're not on that channel");
        std::cout << "PART: Client " << c->fd << " not on channel " << chanName << "\n";
        return;
    }

    // check if client was operator
    if (ch->operators.count(c->nick) > 0)
	{
        wasOp = true;
        ch->operators.erase(c->nick);
    }
    // remove any outstanding invitation
    ch->invited.erase(c->nick);

    // broadcast PART
    std::string partMsg = ":" + c->nick + "!" + c->user + "@" + server_name + " PART " + chanName;
    queue_raw(c, partMsg);
    for (size_t j = 0; j < ch->members.size(); ++j)
	{
        queue_raw(ch->members[j], partMsg);
    }
    std::cout << "Client " << c->fd << " parted channel " << chanName << "\n";

    // if channel now empty, delete it
    if (ch->members.empty())
	{
        // remove from global map and free memory
        channels.erase(chanName);
        delete ch;
        std::cout << "All users have left channel " << chanName << "; channel will be deleted\n";
        return;
    }

    // operator hand-off if needed
    if (wasOp && !ch->members.empty())
	{
        Client* newOp = ch->members.front();
        ch->operators.insert(newOp->nick);

        std::string modeMsg = ":" + server_name + " MODE " + chanName + " +o " + newOp->nick;
        for (size_t j = 0; j < ch->members.size(); ++j)
		{
            queue_raw(ch->members[j], modeMsg);
        }
        std::cout << "Client " << c->fd << " handed operator to " << newOp->nick << " on channel " << chanName << "\n";
    }
}
