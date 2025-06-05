#include "util.h"

// split on a single‐char delimiter
std::vector<std::string> split(const std::string& s, char delim)
{
    std::vector<std::string>	out;
    std::string					cur;

    for (size_t i = 0; i < s.size(); ++i)
	{
        if (s[i] == delim)
		{
            out.push_back(cur);
            cur.clear();
        }

		else
            cur.push_back(s[i]);
    }

    out.push_back(cur);
    return out;
}

// set non-blocking
void set_nb(int fd)
{
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

Client* find_client(int fd)
{
    for (size_t i = 0; i < clients.size(); ++i)
	{
        if (clients[i]->fd == fd)
			return clients[i];
	}

    return NULL;
}

Client* find_nick(const std::string& n)
{
    std::map<std::string,Client*>::iterator it = nick_map.find(n);

    if (it == nick_map.end())
		return NULL;

	else
		return it->second;
}

Channel* get_chan(const std::string& name)
{
    Channel*& ch = channels[name];

    if (!ch)
	{
        ch = new Channel();
        ch->name          = name;
        ch->topic         = "";
        ch->invite_only   = false;
        ch->topic_locked  = false;
        ch->key           = "";
        ch->limit         = 0;
    }
    return ch;
}

void queue_raw(Client* c, const std::string& line)
{
    c->send_q.push_back(line + "\r\n");
    // arm POLLOUT
    for (size_t i = 0; i < fds.size(); ++i)
	{
        if (fds[i].fd == c->fd)
		{
            fds[i].events |= POLLOUT;
            return;
        }
    }
}

void send_err(Client* c, const std::string& code, const std::string& tgt, const std::string& txt)
{
    queue_raw(c, ":" + server_name + " " + code + " " + c->nick + " " + tgt + " :" + txt);
}

void send_rpl(Client* c, const std::string& code, const std::string& tgt, const std::string& txt)
{
    queue_raw(c, ":" + server_name + " " + code + " " + c->nick + " " + tgt + " :" + txt);
}


bool is_number(const std::string& s) {
    if (s.empty()) return false;
    size_t start = 0;
    if (s[0] == '+' || s[0] == '-')
        start = 1;
    for (size_t i = start; i < s.size(); ++i) {
        if (!std::isdigit(s[i]))
            return false;
    }
    return true;
}
