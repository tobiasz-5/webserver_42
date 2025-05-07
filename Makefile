SRCS = main.cpp

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

NAME = server

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(NAME)

clean:

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re