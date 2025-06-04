#ifndef COMMANDS_H
#define COMMANDS_H

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include "state.h"

// prototypes for each IRC command handler:
void cmd_PASS   (Client* c, const std::vector<std::string>& p);
void cmd_NICK   (Client* c, const std::vector<std::string>& p);
void cmd_USER   (Client* c, const std::vector<std::string>& p);
void cmd_JOIN   (Client* c, const std::vector<std::string>& p);
void cmd_PART   (Client* c, const std::vector<std::string>& p);
void cmd_PRIVMSG(Client* c, const std::vector<std::string>& p);
void cmd_INVITE (Client* c, const std::vector<std::string>& p);
void cmd_KICK   (Client* c, const std::vector<std::string>& p);
void cmd_MODE   (Client* c, const std::vector<std::string>& p);
void cmd_TOPIC  (Client* c, const std::vector<std::string>& p);
void cmd_NAMES  (Client* c, const std::vector<std::string>& p);

#endif
