#include "server.h"
#include "state.h"
#include "util.h"
#include "commands.h"

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstdio>

static bool client_receive(Client* c) {
    char buf[512];
    while (true) {
        ssize_t n = recv(c->fd, buf, sizeof(buf), 0);
        if (n > 0) {
            c->recv_buf.append(buf, n);
            continue;
        }
        if (n == 0) return false; // clean EOF
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;
        return false; // error
    }
    return true;
}

static bool client_has_msg(Client* c) {
    return c->recv_buf.find('\n') != std::string::npos;
}

static std::string client_next_msg(Client* c) {
    size_t pos = c->recv_buf.find('\n');
    std::string line = c->recv_buf.substr(0, pos);
    if (!line.empty() && line[line.size()-1] == '\r')
        line.resize(line.size()-1);
    c->recv_buf.erase(0, pos + 1);
    return line;
}

static void cleanup_client(Client* c) {
    // Remove from all channels and notify members
    for (std::map<std::string,Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        Channel* ch = it->second;
        bool was_member = false;
        bool was_op = false;
        for (size_t i = 0; i < ch->members.size(); ++i) {
            if (ch->members[i] == c) {
                was_member = true;
                was_op = ch->operators.count(c->nick) > 0;
                ch->members.erase(ch->members.begin() + i);
                break;
            }
        }
        if (was_member) {
            // Notify channel members about the quit
            std::string quitMsg = ":" + c->nick + "!" + c->user + "@" + server_name + " QUIT :Client disconnected";
            for (size_t j = 0; j < ch->members.size(); ++j) {
                queue_raw(ch->members[j], quitMsg);
            }
            // Remove from operators and invited lists
            ch->operators.erase(c->nick);
            ch->invited.erase(c->nick);

            // Handle operator handoff if needed
            if (was_op && !ch->members.empty()) {
                // Pick the next member as operator
                Client* newOp = ch->members.front();
                ch->operators.insert(newOp->nick);

                // Broadcast the MODE change
                std::string modeMsg = ":" + server_name + " MODE " + ch->name + " +o " + newOp->nick;
                for (size_t j = 0; j < ch->members.size(); ++j) {
                    queue_raw(ch->members[j], modeMsg);
                }
            }
        }
    }

    // Remove from nick map
    if (!c->nick.empty()) {
        nick_map.erase(c->nick);
    }

    // Remove from clients list
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i] == c) {
            clients.erase(clients.begin() + i);
            break;
        }
    }

    // Remove from fds list
    for (size_t i = 0; i < fds.size(); ++i) {
        if (fds[i].fd == c->fd) {
            fds.erase(fds.begin() + i);
            break;
        }
    }

    // Close the socket
    close(c->fd);
    delete c;
}

void init_server(int port, const std::string& pass) {
    server_pass = pass;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); exit(1); }
    set_nb(listen_fd);

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(listen_fd,(sockaddr*)&addr,sizeof(addr))<0) {
        if (errno==EADDRINUSE)
            std::cerr<<"Port "<<port<<" in use\n";
        else
            perror("bind");
        exit(1);
    }
    if (listen(listen_fd, SOMAXCONN)<0) { perror("listen"); exit(1); }

    fds.push_back((struct pollfd){listen_fd, POLLIN, 0});
    std::cout<<"Server listening on port "<<port<<"\n";
}

void server_run() {
    while (true) {
        if (poll(&fds[0], fds.size(), -1) < 0) break;
        for (size_t i = 0; i < fds.size(); ++i) {
            if (!fds[i].revents) continue;
            int fd = fds[i].fd;

            // new connection
            if (fd == listen_fd && (fds[i].revents & POLLIN)) {
                struct sockaddr_in cli; socklen_t len = sizeof(cli);
                int cfd = accept(listen_fd,(sockaddr*)&cli,&len);
                if (cfd < 0) continue;
                set_nb(cfd);
                Client* c = new Client();
                c->fd = cfd;
                c->got_pass = c->registered = false;
                clients.push_back(c);
                fds.push_back((struct pollfd){cfd, POLLIN, 0});
                std::cout<<"New connection: fd="<<cfd<<"\n";
                continue;
            }

            // find client
            Client* c = find_client(fd);
            if (!c) continue;

            // readable
            if (fds[i].revents & POLLIN) {
                if (!client_receive(c)) {
                    std::cout<<"Client disconnected: fd="<<fd
                             <<(c->nick.empty()?"":" nick="+c->nick)<<"\n";
                    cleanup_client(c);
                    continue;
                }
                while (client_has_msg(c))
				{
                    std::string line = client_next_msg(c);
				    std::cout << "Parsing command from client " << fd << " (" << (c->nick.empty() ? "unknown" : c->nick) << "): " << line << "\n";
                    std::vector<std::string> tok = split(line, ' ');
                    std::string cmd = tok[0];
                    if (!c->registered)
					{
        				if (cmd == "PASS")
							cmd_PASS(c, tok);
        				else if (cmd == "NICK")
							cmd_NICK(c, tok);
        				else if (cmd == "USER")
							cmd_USER(c, tok);
        				else
						{
        				    send_err(c,"451","*","You have not registered");
        				    std::cout << "Unknown or disallowed pre-register command: "
        				              << cmd << "\n";
        				}
					}
					else
					{
        				if (cmd == "JOIN")
							cmd_JOIN(c, tok);
        				else if (cmd == "PART")
							cmd_PART(c, tok);
        				else if (cmd == "PRIVMSG")
							cmd_PRIVMSG(c, tok);
        				else if (cmd == "INVITE")
							cmd_INVITE(c, tok);
        				else if (cmd == "KICK")
							cmd_KICK(c, tok);
        				else if (cmd == "MODE")
							cmd_MODE(c, tok);
        				else if (cmd == "TOPIC")
							cmd_TOPIC(c, tok);
        				else if (cmd == "NAMES")
							cmd_NAMES(c, tok);
        				else
						{
        				    send_err(c,"421",cmd,"Unknown command");
        				    std::cout << "Unknown command: " << cmd << "\n";
        				}
    				}
                }
            }

            // writable
            if (fds[i].revents & POLLOUT) {
                while (!c->send_q.empty()) {
                    const std::string& m = c->send_q.front();
                    ssize_t s = send(fd, m.c_str(), m.size(), 0);
                    if (s <= 0) break;
                    if ((size_t)s < m.size())
                        c->send_q.front().erase(0, s);
                    else
                        c->send_q.pop_front();
                }
                if (c->send_q.empty())
                    fds[i].events &= ~POLLOUT;
            }

            // hangup / error
            if (fds[i].revents & (POLLHUP|POLLERR)) {
                std::cout<<"Client disconnected(err): fd="<<fd<<"\n";
                cleanup_client(c);
            }
        }
    }
}
