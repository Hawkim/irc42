#include "commands.h"
#include "util.h"
#include "state.h"

// NAMES [ <chan>{,<chan>} ]
void cmd_NAMES(Client* c, const std::vector<std::string>& p)
{
    std::vector<std::string> chans;

    if (p.size() < 2)
	{
        for (std::map<std::string,Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
            chans.push_back(it->first);
    }

	else
        chans = split(p[1], ',');

    for (size_t i = 0; i < chans.size(); ++i)
	{
        const std::string& cn = chans[i];
        Channel* ch = channels[cn];

        std::ostringstream message;

        if (ch)
		{
            for (size_t j = 0; j < ch->members.size(); ++j)
			{
                if (j)
					message << " ";

                if (ch->operators.count(ch->members[j]->nick))
                    message << "@" << ch->members[j]->nick;

                else
                    message << ch->members[j]->nick;
            }
        }

        queue_raw(c, ":" + server_name + " 353 " + c->nick + " = " + cn + " :" + message.str());
        queue_raw(c, ":" + server_name + " 366 " + c->nick + " " + cn + " :End of /NAMES list");

        std::cout << "Client " << c->fd << " requested NAMES for " << cn << "\n";
    }
}
