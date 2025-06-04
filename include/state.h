#ifndef STATE_H
#define STATE_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <sys/poll.h>

// A "plain C" client record:
struct Client
{
    int						fd;
    std::string 			nick;
	std::string 			user;
	std::string 			realname;
    bool					got_pass;
	bool					registered;
    std::string 			recv_buf;
    std::deque<std::string> send_q;
};

// A "plain C" channel record:
struct Channel
{
    std::string 			name;
	std::string 			topic;
	std::string 			key;
    bool					invite_only;
	bool					topic_locked;
    int						limit;
    std::vector<Client*>	members;
    std::set<std::string>	operators;
	std::set<std::string>	invited;
};

// Global server state (all C-style globals):
extern int listen_fd;
extern std::string server_pass;
extern std::string server_name;
extern std::vector<struct pollfd> fds;
extern std::vector<Client*> clients;
extern std::map<std::string,Client*>  nick_map;
extern std::map<std::string,Channel*> channels;

// Global flag for server shutdown
extern bool g_server_shutdown;

// Cleanup function
void cleanup_server();

#endif
