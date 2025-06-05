#include "commands.h"
#include "util.h"
#include "state.h"

// MODE <#channel> [ +/-modes [ params... ] ]
void cmd_MODE(Client* c, const std::vector<std::string>& p)
{
    // If the client sent exactly "MODE #channel" (no mode chars), reply with current modes
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
        std::ostringstream paramsOut;

        if (ch->invite_only)
		{
            modeFlags += "i";
        }

        if (ch->topic_locked)
		{
            modeFlags += "t";
        }

        if (!ch->key.empty())
		{
            modeFlags += "k";
            paramsOut << " " << ch->key;
        }

        if (ch->limit > 0)
		{
            modeFlags += "l";
            paramsOut << " " << ch->limit;
        }

        // If only "+" remains, no modes are set
        if (modeFlags.size() == 1)
            queue_raw(c, ":" + server_name + " 324 " + c->nick + " " + chanName + " +");

		else
            queue_raw(c, ":" + server_name + " 324 " + c->nick + " " + chanName + " " + modeFlags + paramsOut.str());

        std::cout << "MODE query: sent current modes " << modeFlags << " for channel " << chanName << " to client " << c->fd << "\n";
        return;
    }

    // Otherwise, must be "MODE #channel +/-modes [params]"
    if (p.size() < 3)
	{
        send_err(c, "461", "MODE", "Not enough parameters");
        std::cout << "MODE: Not enough parameters for client " << c->fd << "\n";
        return;
    }

    const std::string& chanName = p[1];
    const std::string& modes = p[2];
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
            if (add)
            {	
                // +k requires exactly one parameter and that parameter must be one single word
                if (argi < p.size())
                {
                    param = p[argi++];

                    if (!param.empty() && param[0] == ':')
                        param = param.substr(1);

                    // If there are any extra tokens beyond this one, reject
                    if (argi < p.size())
                    {
                        send_err(c, "467", chanName, "Invalid channel key");
                        std::cout << "MODE: Channel key invalid" << c->fd << "\n";
                        return;
                    }

                    ch->key = param;
                    modeStr = "+k";
                }
                else
                {
                    send_err(c, "461", "MODE", "Not enough parameters");
                    std::cout << "MODE: missing key parameter for client " << c->fd << "\n";
                    return;
                }
            }
            else
            {
                // -k now also requires the parameter to match exactly
                if (argi < p.size())
                {
                    param = p[argi++];
                    if (!param.empty() && param[0] == ':')
                        param = param.substr(1);

                    // Again, no extra tokens allowed
                    if (argi < p.size())
                    {
                        send_err(c, "467", chanName, "Invalid channel key");
                        std::cout << "MODE: Extra tokens after key for client " << c->fd << "\n";
                        return;
                    }

                    if (param == ch->key)
                    {
                        ch->key.clear();
                        modeStr = "-k";
                    }
                    else
                    {
                        send_err(c, "467", chanName, "Key is incorrect");
                        std::cout << "MODE: incorrect key for client " << c->fd << "\n";
                        return;
                    }
                }
                else
                {
                    send_err(c, "461", "MODE", "Not enough parameters");
                    std::cout << "MODE: missing key parameter for client " << c->fd << "\n";
                    return;
                }
            }
        }

        else if (m == 'l')
		{
            if (add)
			{
                if (argi < p.size())
				{
                    param = p[argi++];

                    // Validate that param is a positive integer
                    bool validNumber = true;
					
                    for (size_t k = 0; k < param.size(); ++k)
					{
                        if (!std::isdigit(param[k]))
						{
                            validNumber = false;
                            break;
                        }
                    }

                    if (validNumber)
					{
                        int lim = std::atoi(param.c_str());

                        if (lim > 0)
						{
                            ch->limit = lim;
                            modeStr = "+l";
                        }
						else
                            validNumber = false;
                    }

                    if (!validNumber)
					{
                        send_err(c, "461", "MODE", "Invalid limit parameter");
                        std::cout << "MODE: invalid limit parameter for client " << c->fd << "\n";
                        return;
                    }
                }
				else
				{
                    send_err(c, "461", "MODE", "Not enough parameters");
                    std::cout << "MODE: missing limit parameter for client " << c->fd << "\n";
                    return;
                }
            }
			else
			{
                ch->limit = 0;
                modeStr = "-l";
            }
        }

        else if (m == 'o')
		{
            // +o or -o for a given user
            if (argi < p.size())
			{
                param = p[argi++];

                if (!param.empty() && param[0] == ':')
				{
                    param = param.substr(1);
                }

                // Check that the target user exists
                Client* tgt = find_nick(param);

                if (!tgt)
				{
                    send_err(c, "401", param, "No such nick");
                    std::cout << "MODE: no such nick " << param << " for client " << c->fd << "\n";
                    return;
                }

                // Check that target is on the channel
                bool tgt_on_chan = false;

                for (size_t k = 0; k < ch->members.size(); ++k)
				{
                    if (ch->members[k] == tgt)
					{
                        tgt_on_chan = true;
                        break;
                    }
                }

                if (!tgt_on_chan)
				{
                    send_err(c, "442", chanName, param);
                    std::cout << "MODE: target " << param << " not on channel " << chanName << "\n";
                    return;
                }

                if (add)
				{
                    ch->operators.insert(param);
                    modeStr = "+o";
                }
				else
				{
                    // If removing operator, ensure at least one op remains
                    if (ch->operators.count(param) > 0)
					{
                        ch->operators.erase(param);
                        modeStr = "-o";
                    }
					else
					{
                        // Trying to remove op status from someone who isn't an operator
                        send_err(c, "482", chanName, "User is not an operator");
                        std::cout << "MODE: " << param << " is not op in " << chanName << "\n";
                        return;
                    }
                }
            }
			else
			{
                send_err(c, "461", "MODE", "Not enough parameters");
                std::cout << "MODE: missing user parameter for client " << c->fd << "\n";
                return;
            }
        }

        else
		{
            // unknown mode letter—skip
            continue;
        }

        // Broadcast a MODE line for just this single change
        std::string msg = ":" + c->nick + "!" + c->user + "@" + server_name + " MODE " + chanName + " " + modeStr;

        if (!param.empty())
            msg += " " + param;

		for (size_t j = 0; j < mems.size(); ++j)
		{
            queue_raw(mems[j], msg);
        }

        std::cout << "Client " << c->fd << " (" << c->nick << ") set mode " << modeStr << " on channel " << chanName;

        if (!param.empty())
            std::cout << " param=" << param;

        std::cout << "\n";
    }
}
