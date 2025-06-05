// src/commands.cpp

#include "commands.h"
#include "util.h"
#include "state.h"

// PASS <password>
void cmd_PASS(Client* c, const std::vector<std::string>& p) {
    // Require exactly two tokens: "PASS" and a single‐word password
    if (p.size() != 2) {
        send_err(c, "461", "PASS", "Password must not have whitespaces");
        std::cout << "PASS: invalid format (too many tokens) for client " << c->fd << "\n";
        return;
    }
    if (c->got_pass) {
        send_err(c, "462", "PASS", "You may not re-register");
        std::cout << "PASS: Already registered (client " << c->fd << ")\n";
        return;
    }

    const std::string& supplied = p[1];
    // Though split() already broke on spaces, we still check for hidden whitespace:
    for (size_t i = 0; i < supplied.size(); ++i) {
        if (std::isspace(static_cast<unsigned char>(supplied[i]))) {
            send_err(c, "461", "PASS", "Password must not contain whitespace");
            std::cout << "PASS: invalid format (contains whitespace) for client " << c->fd << "\n";
            return;
        }
    }

    if (supplied != server_pass) {
        send_err(c, "464", "PASS", "Password incorrect");
        std::cout << "PASS rejected: wrong password (client " << c->fd << ")\n";
        return;
    }

    c->got_pass = true;
    send_rpl(c, "381", "*", "Password accepted");
    std::cout << "PASS accepted from client " << c->fd << "\n";
}
