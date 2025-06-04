#include "commands.h"
#include "util.h"
#include "state.h"

// MODE <#channel> <modes> [ params... ]
void cmd_MODE(Client* c, const std::vector<std::string>& p)
{
    // If the client sent exactly “MODE #channel” (no mode chars), reply with current modes
    if (p.size() == 2)
	{
        const std::string& chanName = p[1];
        Channel* ch = get_chan(chanName);

        // Verify the client is in the channel
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
            std::cout << "MODE query: client " << c->fd << " not on " << chanName << "\n";
            return;
        }

        // Build a string of all set modes
        std::string modeFlags = "+";
        std::ostringstream params;

        if (ch->invite_only)
            modeFlags += "i";
        if (ch->topic_locked)
            modeFlags += "t";
        if (!ch->key.empty())
		{
            modeFlags += "k";
            params << " " << ch->key;
        }
        if (ch->limit > 0)
		{
            modeFlags += "l";
            params << " " << ch->limit;
        }

        // If only "+" remains, no modes are set
        if (modeFlags.size() == 1)
            // RPL_CHANNELMODEIS is numeric 324
            queue_raw(c, ":" + server_name + " 324 " + c->nick + " " + chanName + " +");
		else
            queue_raw(c, ":" + server_name + " 324 " + c->nick + " " + chanName + " " + modeFlags + params.str());
        std::cout << "MODE query: sent current modes " << modeFlags << " for channel " << chanName << " to client " << c->fd << "\n";
        return;
    }

    // Otherwise, must be “MODE #channel +/-modes [params]”
    if (p.size() < 3)
	{
        send_err(c, "461", "MODE", "Not enough parameters");
        std::cout << "MODE: Not enough parameters for client " << c->fd << "\n";
        return;
    }

    const std::string& chanName = p[1];
    const std::string& modes    = p[2];
    Channel* ch = get_chan(chanName);

    // Verify the client is in the channel
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
        std::cout << "MODE: client " << c->fd << " not on " << chanName << "\n";
        return;
    }
    // Verify the client is an operator
    if (!ch->operators.count(c->nick))
	{
        send_err(c, "482", chanName, "You're not channel operator");
        std::cout << "MODE: client " << c->fd << " not operator on " << chanName << "\n";
        return;
    }

    bool add = true;
    size_t argi = 3;
    std::vector<Client*>& mems = ch->members;

    for (size_t i = 0; i < modes.size(); ++i)
	{
        char m = modes[i];

        if (m == '+')
		{
            add = true;
            continue;
        }
        if (m == '-')
		{
            add = false;
            continue;
        }

        std::string modeStr;
        std::string param;

        if (m == 'i')
		{
            ch->invite_only = add;
            if (add)
                modeStr = "+i";
            else
                modeStr = "-i";
        }
        else if (m == 't')
		{
            ch->topic_locked = add;
            if (add)
                modeStr = "+t";
            else
                modeStr = "-t";
        }
        else if (m == 'k')
		{
            if (argi < p.size())
			{
                param = p[argi++];
                if (!param.empty() && param[0] == ':')
                    param = param.substr(1);
                if (add)
                    ch->key = param;
                else
                    ch->key.clear();
                if (add)
                    modeStr = "+k";
                else
                    modeStr = "-k";
            }
			else
			{
                send_err(c, "461", "MODE", "Not enough parameters");
                std::cout << "MODE: missing key parameter for client " << c->fd << "\n";
                return;
            }
        }
        else if (m == 'l')
		{
            if (argi < p.size())
			{
                param = p[argi++];
                int lim = std::atoi(param.c_str());
                if (add)
                    ch->limit = lim;
                else
                    ch->limit = 0;
                if (add)
                    modeStr = "+l";
                else
                    modeStr = "-l";
            }
			else
			{
                send_err(c, "461", "MODE", "Not enough parameters");
                std::cout << "MODE: missing limit parameter for client " << c->fd << "\n";
                return;
            }
        }
        else if (m == 'o')
		{
            if (argi < p.size())
			{
                param = p[argi++];
                if (!param.empty() && param[0] == ':')
                    param = param.substr(1);
                if (add)
                    ch->operators.insert(param);
                else
                    ch->operators.erase(param);
                if (add)
                    modeStr = "+o";
                else
                    modeStr = "-o";
            }
			else
			{
                send_err(c, "461", "MODE", "Not enough parameters");
                std::cout << "MODE: missing user parameter for client " << c->fd << "\n";
                return;
            }
        }
        else
            // unknown mode letter—skip it
            continue;

        // Broadcast a MODE line for this single change
        std::string msg = ":" + c->nick + "!" + c->user + "@" + server_name + " MODE " + chanName + " " + modeStr;
        if (!param.empty())
            msg += " " + param;
        for (size_t j = 0; j < mems.size(); ++j)
		{
            queue_raw(mems[j], msg);
        }

        std::cout << "Client " << c->fd << " set mode " << modeStr << " on channel " << chanName;
        if (!param.empty())
            std::cout << " param=" << param;
        std::cout << "\n";
    }
}
