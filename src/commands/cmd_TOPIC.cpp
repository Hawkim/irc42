#include "commands.h"
#include "util.h"
#include "state.h"

// TOPIC <#channel> [ :<topic> ]
void cmd_TOPIC(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 2)
    {
        send_err(c, "461", "TOPIC", "Not enough parameters");
        std::cout << "TOPIC: Not enough parameters for client " << c->fd << "\n";
        return;
    }

    const std::string& cn = p[1];
    Channel* ch = get_chan(cn);

    // Verify that client is in the channel
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
        send_err(c, "442", cn, "You're not on that channel");
        std::cout << "TOPIC: Client " << c->fd << " not on " << cn << "\n";
        return;
    }

    // If only "TOPIC #channel" was sent, return current topic (or 331/332)
    if (p.size() == 2)
    {
        if (ch->topic.empty())
            send_rpl(c, "331", cn, "No topic is set");

        else
            send_rpl(c, "332", cn, ch->topic);

        std::cout << "Client " << c->fd << " requested topic for " << cn << "\n";
        return;
    }

    // If topic is locked and client not operator, deny
    if (ch->topic_locked && !ch->operators.count(c->nick))
    {
        send_err(c, "482", cn, "You're not channel operator");
        std::cout << "TOPIC: Client " << c->fd << " not operator for " << cn << "\n";
        return;
    }

    // Reconstruct the full topic from p[2] onward
    std::string newt;
    for (size_t i = 2; i < p.size(); ++i)
    {
        if (i == 2)
        {
            // Strip leading ':' from the first token, if present
            if (!p[i].empty() && p[i][0] == ':')
                newt = p[i].substr(1);

            else
                newt = p[i];
        }
        else
        {
            // Append a space and the next token
            newt += " ";
            newt += p[i];
        }
    }

    // Set and broadcast the new topic
    ch->topic = newt;
    std::string msg = ":" + c->nick + "!" + c->user + "@" + server_name + " TOPIC " + cn + " :" + newt;

    for (size_t i = 0; i < ch->members.size(); ++i)
        queue_raw(ch->members[i], msg);

    std::cout << "Client " << c->fd << " set topic for " << cn << " to \"" << newt << "\"\n";
}
