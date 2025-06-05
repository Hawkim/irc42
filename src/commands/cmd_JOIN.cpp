#include "commands.h"
#include "util.h"
#include "state.h"

// JOIN <#channel>[,#other] [key[,key2]]
void cmd_JOIN(Client* c, const std::vector<std::string>& p)
{
    if (p.size() < 2)
	{
        send_err(c,"461","JOIN","Not enough parameters");
        std::cout << "JOIN: Not enough parameters for client " << c->fd << "\n";
        return;
    }

    std::vector<std::string> chans = split(p[1], ',');
    std::vector<std::string> keys;

    if (p.size() > 2)
		keys = split(p[2], ',');

    for (size_t i = 0; i < chans.size(); ++i)
	{
        const std::string& chanName = chans[i];
        std::string  key;

		if (i < keys.size())
			key = keys[i];

		else
			key = "";

        Channel* ch = get_chan(chanName);

        // check +i, +k, +l as before
        if (ch->invite_only && !ch->invited.count(c->nick) && !ch->operators.count(c->nick))
        {
            send_err(c,"473",chanName,"Cannot join channel (+i)");
            std::cout << "JOIN: Client " << c->fd << " cannot join +i channel " << chanName << "\n";
            continue;
        }

        if (!ch->key.empty() && ch->key != key)
		{
            send_err(c,"475",chanName,"Cannot join channel (+k)");
            std::cout << "JOIN: Client " << c->fd << " provided wrong key for " << chanName << "\n";
            continue;
        }

        if (ch->limit > 0 && (int)ch->members.size() >= ch->limit)
        {
            send_err(c,"471",chanName,"Cannot join channel (+l)");
            std::cout << "JOIN: Client " << c->fd << " channel " << chanName << " is full\n";
            continue;
        }

        // actually add the client
        ch->members.push_back(c);
        if (ch->members.size() == 1)
		{
            ch->operators.insert(c->nick);
        }

        // broadcast JOIN line
        std::string joinMsg = ":" + c->nick + "!" + c->user + "@" + server_name + " JOIN " + chanName;
        for (size_t j = 0; j < ch->members.size(); ++j)
		{
            queue_raw(ch->members[j], joinMsg);
        }

        // send topic (331/332)
        if (ch->topic.empty())
            queue_raw(c, ":" + server_name + " 331 " + c->nick + " " + chanName + " :No topic is set");
		else
            queue_raw(c, ":" + server_name + " 332 " + c->nick + " " + chanName + " :" + ch->topic);

		// send NAMES (353) + 366
		std::ostringstream ns;
		for (size_t j = 0; j < ch->members.size(); ++j)
		{
			if (j)
				ns << " ";
			if (ch->operators.count(ch->members[j]->nick))
            	ns << "@" << ch->members[j]->nick;
			else
        		ns << ch->members[j]->nick;
		}
		queue_raw(c, ":" + server_name + " 353 " + c->nick + " = " + chanName + " :" + ns.str());
		queue_raw(c, ":" + server_name + " 366 " + c->nick + " " + chanName + " :End of /NAMES list");
	
		// "* Invite mode: +i” or “* Invite mode: -i"
		std::string inviteMode;

		if (ch->invite_only)
			inviteMode = "+i";
		else
			inviteMode = "-i";

		std::string inviteNotice = ":" + server_name + " NOTICE " + c->nick + " :* Invite mode: " + inviteMode;
		queue_raw(c, inviteNotice);

		//“* Key mode: +k <key>” or “* Key mode: -k”
		std::string keyMode;

		if (!ch->key.empty())
		{
			keyMode = "+k ";
			keyMode += ch->key;
		}

		else
			keyMode = "-k";

		std::string keyNotice = ":" + server_name + " NOTICE " + c->nick + " :* Key mode: " + keyMode;
		queue_raw(c, keyNotice);

		//“* limit is : <limit>” or “* limit is : no limit”
		std::string limitStr;

		if (ch->limit > 0)
		{
			// convert integer to string
			std::ostringstream os;
			os << ch->limit;
			limitStr = os.str();
		}

		else
			limitStr = "no limit";

		std::string limitNotice = ":" + server_name + " NOTICE " + c->nick + " :* limit is : " + limitStr;
		queue_raw(c, limitNotice);

		//“* TOPIC LOCKED: YES” or “* TOPIC LOCKED: NO”
		std::string topicLocked;

		if (ch->topic_locked)
			topicLocked = "YES";

		else
			topicLocked = "NO";

		std::string topicNotice = ":" + server_name + " NOTICE " + c->nick + " :* TOPIC LOCKED: " + topicLocked;
		queue_raw(c, topicNotice);

		// send current channel‐mode (+i/+t/+k/+l) to the joining user
		{
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

            // Only send if at least one flag is set
            if (modeFlags.size() > 1)
			{
                std::string modeMsg = ":" + server_name + " MODE " + chanName + " " + modeFlags + params.str();
                queue_raw(c, modeMsg);
                std::cout << "JOIN: sent modes " << modeFlags << " to client " << c->fd << " on channel " << chanName << "\n";
            }
        }

        std::cout << "Client " << c->fd << " joined channel " << chanName << "\n";
    }
}