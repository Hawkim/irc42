NAME = ircserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude

SRC =	src/state.cpp \
		src/util.cpp \
		src/server.cpp \
		src/main.cpp \
		src/commands/cmd_INVITE.cpp \
		src/commands/cmd_JOIN.cpp \
		src/commands/cmd_KICK.cpp \
		src/commands/cmd_MODE.cpp \
		src/commands/cmd_NAMES.cpp \
		src/commands/cmd_NICK.cpp \
		src/commands/cmd_PART.cpp \
		src/commands/cmd_PASS.cpp \
		src/commands/cmd_PRIVMSG.cpp \
		src/commands/cmd_TOPIC.cpp \
		src/commands/cmd_USER.cpp \

OBJS = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean re fclean
