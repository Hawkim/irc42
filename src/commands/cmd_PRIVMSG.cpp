#include "commands.h"
#include "util.h"
#include "state.h"

// PRIVMSG <target> :<message>
void cmd_PRIVMSG(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 3)
	{
        send_err(c, "461", "PRIVMSG", "Not enough parameters");
        std::cout << "PRIVMSG: Not enough parameters for client " << c->fd << "\n";
        return;
    }

    const std::string& tgt = p[1];

    // Reconstruct the full message text from p[2] onward
    std::string txt;

    for (size_t i = 2; i < p.size(); ++i)
	{
        if (i > 2)
			txt += " ";
        if (i == 2 && !p[i].empty() && p[i][0] == ':')
            txt += p[i].substr(1);  // strip leading ':'
        else
            txt += p[i];
    }

    if (tgt.size() > 0 && tgt[0] == '#')
	{
        Channel* ch = get_chan(tgt);
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
            send_err(c, "442", tgt, "You're not on that channel");
            std::cout << "PRIVMSG: Client " << c->fd << " not on channel " << tgt << "\n";
            return;
        }

        std::string m = ":" + c->nick + "!" + c->user + "@" + server_name + " PRIVMSG " + tgt + " :" + txt;

        for (size_t i = 0; i < ch->members.size(); ++i)
		{
            if (ch->members[i] != c)
                queue_raw(ch->members[i], m);
        }

        std::cout << "Client " << c->fd << " sent PRIVMSG to " << tgt << ": " << txt << "\n";
    }

    else
	{
        Client* d = find_nick(tgt);

        if (!d)
		{
            send_err(c, "401", tgt, "No such nick");
            std::cout << "PRIVMSG: No such nick " << tgt << "\n";
            return;
        }

        std::string m = ":" + c->nick + "!" + c->user + "@" + server_name + " PRIVMSG " + tgt + " :" + txt;
        queue_raw(d, m);

        std::cout << "Client " << c->fd << " sent PRIVMSG to user " << tgt << ": " << txt << "\n";
    }
}
