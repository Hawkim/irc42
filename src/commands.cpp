// src/commands.cpp

#include "commands.h"
#include "util.h"
#include "state.h"

#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <iostream>

// PASS <password>
void cmd_PASS(Client* c, const std::vector<std::string>& p) {
    if (p.size() < 2) {
        send_err(c,"461","PASS","Not enough parameters");
        std::cout << "PASS: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    if (c->got_pass) {
        send_err(c,"462","PASS","You may not re-register");
        std::cout << "PASS: Already registered (client " << c->fd << ")\n";
        return;
    }
    if (p[1] != server_pass) {
        send_err(c,"464","PASS","Password incorrect");
        std::cout << "PASS rejected: wrong password (client " << c->fd << ")\n";
        return;
    }
    c->got_pass = true;
    send_rpl(c,"381","*","Password accepted");
    std::cout << "PASS accepted from client " << c->fd << "\n";
}

// NICK <nickname>
void cmd_NICK(Client* c, const std::vector<std::string>& p) {
    if (p.size() < 2 || p[1].empty()) {
        send_err(c,"431","*","No nickname given");
        std::cout << "NICK: No nickname given for client " << c->fd << "\n";
        return;
    }
    const std::string& newnick = p[1];
    if (find_nick(newnick)) {
        send_err(c,"433",newnick,"Nickname is already in use");
        std::cout << "NICK: " << newnick << " already in use\n";
        return;
    }
    if (!c->nick.empty()) nick_map.erase(c->nick);
    c->nick = newnick;
    nick_map[newnick] = c;
    std::cout << "Client " << c->fd << " set nickname to " << newnick << "\n";
}

// USER <username> 0 * :<realname>
void cmd_USER(Client* c, const std::vector<std::string>& p) {
    if (p.size() < 5) {
        send_err(c,"461","USER","Not enough parameters");
        std::cout << "USER: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    if (c->registered) {
        send_err(c,"462","USER","You may not re-register");
        std::cout << "USER: Already registered (client " << c->fd << ")\n";
        return;
    }
    c->user     = p[1];
    c->realname = p[4];
    if (!c->got_pass || c->nick.empty()) {
        send_err(c,"451","*","You have not registered");
        std::cout << "USER: client " << c->fd << " missing PASS/NICK\n";
        return;
    }
    c->registered = true;

    queue_raw(c, ":" + server_name + " 001 " + c->nick + " :Welcome to the IRC server");
    queue_raw(c, ":" + server_name + " 002 " + c->nick + " :Your host is " + server_name);
    queue_raw(c, ":" + server_name + " 003 " + c->nick + " :This server was created today");
    queue_raw(c, ":" + server_name + " 004 " + c->nick + " " + server_name + " 1.0 oiwsz");

    std::cout << "Client " << c->fd << " registered with username: " << c->user << "\n";
    std::cout << "Welcome sent to " << c->nick << "\n";
}

// JOIN <#channel>[,#other] [key[,key2]]
void cmd_JOIN(Client* c, const std::vector<std::string>& p) {
    if (p.size() < 2) {
        send_err(c,"461","JOIN","Not enough parameters");
        std::cout << "JOIN: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    std::vector<std::string> chans = split(p[1], ',');
    std::vector<std::string> keys;
    if (p.size() > 2) keys = split(p[2], ',');

    for (size_t i = 0; i < chans.size(); ++i) {
        const std::string& cn = chans[i];
        const std::string  key = (i < keys.size() ? keys[i] : "");
        Channel* ch = get_chan(cn);

        if (ch->invite_only && !ch->invited.count(c->nick) && !ch->operators.count(c->nick)) {
            send_err(c,"473",cn,"Cannot join channel (+i)");
            std::cout << "JOIN: Client " << c->fd << " cannot join +i channel " << cn << "\n";
            continue;
        }
        if (!ch->key.empty() && ch->key != key) {
            send_err(c,"475",cn,"Cannot join channel (+k)");
            std::cout << "JOIN: Client " << c->fd << " provided wrong key for " << cn << "\n";
            continue;
        }
        if (ch->limit > 0 && (int)ch->members.size() >= ch->limit) {
            send_err(c,"471",cn,"Cannot join channel (+l)");
            std::cout << "JOIN: Client " << c->fd << " channel " << cn << " is full\n";
            continue;
        }

        ch->members.push_back(c);
        if (ch->members.size() == 1)
            ch->operators.insert(c->nick);

        std::string joinMsg = ":" + c->nick + "!" + c->user + "@" + server_name + " JOIN " + cn;
        for (size_t j = 0; j < ch->members.size(); ++j)
            queue_raw(ch->members[j], joinMsg);

        // topic
        if (ch->topic.empty())
            queue_raw(c, ":" + server_name + " 331 " + c->nick + " " + cn + " :No topic is set");
        else
            queue_raw(c, ":" + server_name + " 332 " + c->nick + " " + cn + " :" + ch->topic);

        // names
        std::ostringstream ns;
        for (size_t j = 0; j < ch->members.size(); ++j) {
            if (j) ns << " ";
            if (ch->operators.count(ch->members[j]->nick))
                ns << "@" << ch->members[j]->nick;
            else
                ns << ch->members[j]->nick;
        }
        queue_raw(c, ":" + server_name + " 353 " + c->nick + " = " + cn + " :" + ns.str());
        queue_raw(c, ":" + server_name + " 366 " + c->nick + " " + cn + " :End of /NAMES list");

        std::cout << "Client " << c->fd << " joined channel " << cn << "\n";
    }
}

// PART <#channel>
void cmd_PART(Client* c, const std::vector<std::string>& p) {
    if (p.size() < 2) {
        send_err(c,"461","PART","Not enough parameters");
        return;
    }
    const std::string& chanName = p[1];
    Channel* ch = get_chan(chanName);

    // were they an operator?
    bool wasOp = ch->operators.count(c->nick) > 0;

    // remove from members
    bool found = false;
    for (size_t i = 0; i < ch->members.size(); ++i) {
        if (ch->members[i] == c) {
            ch->members.erase(ch->members.begin() + i);
            found = true;
            break;
        }
    }
    if (!found) {
        send_err(c,"442",chanName,"You're not on that channel");
        return;
    }
    ch->operators.erase(c->nick);
    ch->invited.erase(c->nick);

    // broadcast PART
    std::string partMsg = ":" + c->nick + "!" + c->user
                        + "@" + server_name
                        + " PART " + chanName;
    queue_raw(c, partMsg);
    for (size_t j = 0; j < ch->members.size(); ++j)
        queue_raw(ch->members[j], partMsg);

    // ——— operator hand‐off ———
    if (wasOp && !ch->members.empty()) {
        // pick the next member
        Client* newOp = ch->members.front();
        ch->operators.insert(newOp->nick);

        // **broadcast** the MODE change
        std::string modeMsg = ":" + server_name
                            + " MODE " + chanName
                            + " +o " + newOp->nick;
        for (size_t j = 0; j < ch->members.size(); ++j)
            queue_raw(ch->members[j], modeMsg);
    }
}

// PRIVMSG <target> :<message>
void cmd_PRIVMSG(Client* c, const std::vector<std::string>& p) {
    if (p.size() < 3) {
        send_err(c,"461","PRIVMSG","Not enough parameters");
        std::cout << "PRIVMSG: Not enough parameters for client " << c->fd << "\n";
        return;
    }

    const std::string& tgt = p[1];

    // Reconstruct the full message text from p[2] onward
    std::string txt;
    for (size_t i = 2; i < p.size(); ++i) {
        if (i > 2) txt += " ";
        if (i == 2 && !p[i].empty() && p[i][0] == ':')
            txt += p[i].substr(1);  // strip leading ':'
        else
            txt += p[i];
    }

    if (tgt.size() > 0 && tgt[0] == '#') {
        Channel* ch = get_chan(tgt);
        bool on_chan = false;
        for (size_t i = 0; i < ch->members.size(); ++i) {
            if (ch->members[i] == c) { on_chan = true; break; }
        }
        if (!on_chan) {
            send_err(c,"442",tgt,"You're not on that channel");
            std::cout << "PRIVMSG: Client " << c->fd << " not on channel " << tgt << "\n";
            return;
        }

        std::string m = ":" + c->nick + "!" + c->user + "@"
                      + server_name + " PRIVMSG " + tgt
                      + " :" + txt;
        for (size_t i = 0; i < ch->members.size(); ++i) {
            if (ch->members[i] != c)
                queue_raw(ch->members[i], m);
        }

        std::cout << "Client " << c->fd
                  << " sent PRIVMSG to " << tgt
                  << ": " << txt << "\n";
    }
    else {
        Client* d = find_nick(tgt);
        if (!d) {
            send_err(c,"401",tgt,"No such nick");
            std::cout << "PRIVMSG: No such nick " << tgt << "\n";
            return;
        }

        std::string m = ":" + c->nick + "!" + c->user + "@"
                      + server_name + " PRIVMSG " + tgt
                      + " :" + txt;
        queue_raw(d, m);

        std::cout << "Client " << c->fd
                  << " sent PRIVMSG to user " << tgt
                  << ": " << txt << "\n";
    }
}

// INVITE <nick> <#channel>
void cmd_INVITE(Client* c, const std::vector<std::string>& p) {
    if (p.size() < 3) {
        send_err(c,"461","INVITE","Not enough parameters");
        std::cout << "INVITE: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    const std::string& nick    = p[1];
    const std::string& chanName= p[2];
    Channel* ch = get_chan(chanName);

    bool on_chan = false;
    for (size_t i = 0; i < ch->members.size(); ++i)
        if (ch->members[i] == c) { on_chan = true; break; }
    if (!on_chan) {
        send_err(c,"442",chanName,"You're not on that channel");
        std::cout << "INVITE: Client " << c->fd << " not on channel " << chanName << "\n";
        return;
    }
    if (!ch->operators.count(c->nick)) {
        send_err(c,"482",chanName,"You're not channel operator");
        std::cout << "INVITE: Client " << c->fd << " not operator on " << chanName << "\n";
        return;
    }
    Client* tgt = find_nick(nick);
    if (!tgt) {
        send_err(c,"401",nick,"No such nick");
        std::cout << "INVITE: No such nick " << nick << "\n";
        return;
    }

    ch->invited.insert(nick);
    std::string msg = ":" + c->nick + "!" + c->user + "@" + server_name
                    + " INVITE " + nick + " :" + chanName;
    queue_raw(c,   msg);
    queue_raw(tgt, msg);

    std::cout << "Client " << c->fd << " invited " << nick
              << " to channel " << chanName << "\n";
}

// KICK <#channel> <nick> [ :reason ]
void cmd_KICK(Client* c, const std::vector<std::string>& p) {
    if (p.size() < 3) {
        send_err(c,"461","KICK","Not enough parameters");
        std::cout << "KICK: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    const std::string& chanName   = p[1];
    const std::string& targetNick = p[2];
    Channel* ch = get_chan(chanName);

    // Check if client is in the channel
    bool on_chan = false;
    for (size_t i = 0; i < ch->members.size(); ++i) {
        if (ch->members[i] == c) { on_chan = true; break; }
    }
    if (!on_chan) {
        send_err(c,"442",chanName,"You're not on that channel");
        std::cout << "KICK: Client " << c->fd << " not on channel " << chanName << "\n";
        return;
    }

    // Check if client is operator
    if (!ch->operators.count(c->nick)) {
        send_err(c,"482",chanName,"You're not channel operator");
        std::cout << "KICK: Client " << c->fd << " not operator on " << chanName << "\n";
        return;
    }

    // Check if target exists
    Client* target = find_nick(targetNick);
    if (!target) {
        send_err(c,"401",targetNick,"No such nick");
        std::cout << "KICK: No such nick " << targetNick << "\n";
        return;
    }

    // Check if target is in the channel
    bool target_on_chan = false;
    for (size_t i = 0; i < ch->members.size(); ++i) {
        if (ch->members[i] == target) { target_on_chan = true; break; }
    }
    if (!target_on_chan) {
        send_err(c,"441",targetNick + " " + chanName,"They aren't on that channel");
        std::cout << "KICK: Target " << targetNick << " not on channel " << chanName << "\n";
        return;
    }

    // remember if they were an op
    bool tgtWasOp = ch->operators.count(targetNick) > 0;

    // build + broadcast the KICK line
    std::string reason;
    if (p.size() > 3)
        reason = " :" + (p[3][0]==':'? p[3].substr(1) : p[3]);
    std::string kickMsg = ":" + c->nick + "!" + c->user
                        + "@" + server_name
                        + " KICK " + chanName
                        + " " + targetNick
                        + reason;

    // remove them from channel
    ch->members.erase(
        std::remove(ch->members.begin(), ch->members.end(), target),
        ch->members.end()
    );
    ch->operators.erase(targetNick);
    ch->invited.erase(targetNick);

    // notify everyone
    queue_raw(target, kickMsg);
    for (size_t j = 0; j < ch->members.size(); ++j)
        queue_raw(ch->members[j], kickMsg);

    // handle operator handoff if needed
    if (tgtWasOp && !ch->members.empty()) {
        Client* newOp = ch->members.front();
        ch->operators.insert(newOp->nick);

        std::string modeMsg = ":" + server_name
                            + " MODE " + chanName
                            + " +o " + newOp->nick;
        for (size_t j = 0; j < ch->members.size(); ++j)
            queue_raw(ch->members[j], modeMsg);
    }

    std::cout << "Client " << c->fd << " kicked " << targetNick << " from " << chanName << "\n";
}

// MODE <#channel> <modes> [ params... ]
void cmd_MODE(Client* c, const std::vector<std::string>& p) {
    if (p.size() < 3) {
        send_err(c, "461", "MODE", "Not enough parameters");
        std::cout << "MODE: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    const std::string& chanName = p[1];
    const std::string& modes    = p[2];
    Channel* ch = get_chan(chanName);

    // ensure client is in channel
    bool on_chan = false;
    for (size_t i = 0; i < ch->members.size(); ++i) {
        if (ch->members[i] == c) { on_chan = true; break; }
    }
    if (!on_chan) {
        send_err(c, "442", chanName, "You're not on that channel");
        std::cout << "MODE: client " << c->fd << " not on " << chanName << "\n";
        return;
    }
    // ensure client is operator
    if (!ch->operators.count(c->nick)) {
        send_err(c, "482", chanName, "You're not channel operator");
        std::cout << "MODE: client " << c->fd << " not operator on " << chanName << "\n";
        return;
    }

    bool add = true;
    size_t argi = 3;
    // process each mode char
    for (size_t i = 0; i < modes.size(); ++i) {
        char m = modes[i];
        if (m == '+') {
            add = true;
            continue;
        }
        if (m == '-') {
            add = false;
            continue;
        }

        // construct the mode string for this single flag (and its parameter, if any)
        std::string modeStr;
        std::string param;

        switch (m) {
            case 'i':
                ch->invite_only = add;
                modeStr = std::string(add ? "+i" : "-i");
                break;

            case 't':
                ch->topic_locked = add;
                modeStr = std::string(add ? "+t" : "-t");
                break;

            case 'k':
                if (argi < p.size()) {
                    param = p[argi++];
                    if (!param.empty() && param[0] == ':')
                        param = param.substr(1);
                    if (add) ch->key = param;
                    else      ch->key.clear();
                    modeStr = std::string(add ? "+k" : "-k");
                } else {
                    // missing key parameter
                    send_err(c, "461", "MODE", "Not enough parameters");
                    std::cout << "MODE: missing key parameter for client " << c->fd << "\n";
                    return;
                }
                break;

            case 'l':
                if (argi < p.size()) {
                    param = p[argi++];
                    int lim = std::atoi(param.c_str());
                    if (add) ch->limit = lim;
                    else     ch->limit = 0;
                    modeStr = std::string(add ? "+l" : "-l");
                } else {
                    send_err(c, "461", "MODE", "Not enough parameters");
                    std::cout << "MODE: missing limit parameter for client " << c->fd << "\n";
                    return;
                }
                break;

            case 'o':
                if (argi < p.size()) {
                    param = p[argi++];
                    if (param[0] == ':') param = param.substr(1);
                    if (add)   ch->operators.insert(param);
                    else       ch->operators.erase(param);
                    modeStr = std::string(add ? "+o" : "-o");
                } else {
                    send_err(c, "461", "MODE", "Not enough parameters");
                    std::cout << "MODE: missing user parameter for client " << c->fd << "\n";
                    return;
                }
                break;

            default:
                // unknown mode letter, ignore
                continue;
        }

        // build and broadcast a MODE line for this single change
        std::string msg = ":" + c->nick + "!" + c->user + "@" + server_name
                        + " MODE " + chanName + " " + modeStr;
        if (!param.empty()) {
            msg += " " + param;
        }

        for (size_t j = 0; j < ch->members.size(); ++j) {
            queue_raw(ch->members[j], msg);
        }

        std::cout << "Client " << c->fd
                  << " set mode " << modeStr
                  << " on channel " << chanName;
        if (!param.empty()) std::cout << " param=" << param;
        std::cout << "\n";
    }
}

// TOPIC <#channel> [ :<topic> ]
void cmd_TOPIC(Client* c, const std::vector<std::string>& p) {
    if (p.size() < 2) {
        send_err(c,"461","TOPIC","Not enough parameters");
        std::cout << "TOPIC: Not enough parameters for client " << c->fd << "\n";
        return;
    }
    const std::string& cn = p[1];
    Channel* ch = get_chan(cn);

    bool on_chan = false;
    for (size_t i = 0; i < ch->members.size(); ++i)
        if (ch->members[i] == c) { on_chan = true; break; }
    if (!on_chan) {
        send_err(c,"442",cn,"You're not on that channel");
        std::cout << "TOPIC: Client " << c->fd << " not on " << cn << "\n";
        return;
    }

    if (p.size() == 2) {
        if (ch->topic.empty())
            send_rpl(c,"331",cn,"No topic is set");
        else
            send_rpl(c,"332",cn,ch->topic);
        std::cout << "Client " << c->fd << " requested topic for " << cn << "\n";
        return;
    }

    if (ch->topic_locked && !ch->operators.count(c->nick)) {
        send_err(c,"482",cn,"You're not channel operator");
        std::cout << "TOPIC: Client " << c->fd << " not operator for " << cn << "\n";
        return;
    }

    std::string newt = (p[2][0]==':'? p[2].substr(1) : p[2]);
    ch->topic = newt;
    std::string msg = ":" + c->nick + "!" + c->user + "@"
                    + server_name + " TOPIC " + cn + " :" + newt;
    for (size_t i = 0; i < ch->members.size(); ++i)
        queue_raw(ch->members[i], msg);

    std::cout << "Client " << c->fd << " set topic for " << cn
              << " to \"" << newt << "\"\n";
}

// NAMES [ <chan>{,<chan>} ]
void cmd_NAMES(Client* c, const std::vector<std::string>& p) {
    std::vector<std::string> chans;
    if (p.size() < 2) {
        for (std::map<std::string,Channel*>::iterator it = channels.begin();
             it != channels.end(); ++it)
            chans.push_back(it->first);
    } else {
        chans = split(p[1], ',');
    }

    for (size_t i = 0; i < chans.size(); ++i) {
        const std::string& cn = chans[i];
        Channel* ch = channels[cn];

        std::ostringstream ns;
        if (ch) {
            for (size_t j = 0; j < ch->members.size(); ++j) {
                if (j) ns << " ";
                if (ch->operators.count(ch->members[j]->nick))
                    ns << "@" << ch->members[j]->nick;
                else
                    ns << ch->members[j]->nick;
            }
        }

        queue_raw(c,
            ":" + server_name
          + " 353 " + c->nick + " = " + cn
          + " :" + ns.str()
        );
        queue_raw(c,
            ":" + server_name
          + " 366 " + c->nick + " " + cn
          + " :End of /NAMES list"
        );

        std::cout << "Client " << c->fd << " requested NAMES for " << cn << "\n";
    }
}
