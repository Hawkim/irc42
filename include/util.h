#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include "state.h"

// Basic helpers:
std::vector<std::string> split(const std::string& s, char delim);
void set_nb(int fd);

// state lookup
Client*  find_client(int fd);
Client*  find_nick(const std::string& n);
Channel* get_chan(const std::string& name);

// sending & queuing
void queue_raw(Client* c, const std::string& line);
void send_err(Client* c,
              const std::string& code,
              const std::string& tgt,
              const std::string& txt);
void send_rpl(Client* c,
              const std::string& code,
              const std::string& tgt,
              const std::string& txt);

#endif // UTIL_H
