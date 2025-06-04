#include "commands.h"
#include "util.h"
#include "state.h"

// PART <#channel>
void cmd_PART(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 2)
	{
        send_err(c, "461", "PART", "Not enough parameters");
        return;
    }
    const std::string& chanName = p[1];
    Channel* ch = get_chan(chanName);

    // were they an operator?
    bool wasOp = ch->operators.count(c->nick) > 0;

    // remove from members
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
        send_err(c, "442", chanName, "You're not on that channel");
        return;
    }
    ch->operators.erase(c->nick);
    ch->invited.erase(c->nick);

    // broadcast PART
    std::string partMsg = ":" + c->nick + "!" + c->user + "@" + server_name + " PART " + chanName;
    queue_raw(c, partMsg);
    for (size_t j = 0; j < ch->members.size(); ++j)
        queue_raw(ch->members[j], partMsg);

    // ——— operator hand‐off ———
    if (wasOp && !ch->members.empty())
	{
        // pick the next member
        Client* newOp = ch->members.front();
        ch->operators.insert(newOp->nick);

        // **broadcast** the MODE change
        std::string modeMsg = ":" + server_name + " MODE " + chanName + " +o " + newOp->nick;
        for (size_t j = 0; j < ch->members.size(); ++j)
            queue_raw(ch->members[j], modeMsg);
    }
}
