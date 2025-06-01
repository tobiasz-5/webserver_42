SRCS = main.cpp Server.cpp Client.cpp Request.cpp config.cpp

OBJS = $(SRCS:.cpp=.o)

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

NAME = webserv

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

.PHONY: all clean fclean re