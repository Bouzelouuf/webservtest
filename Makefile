NAME = webserv

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)
FLAGS = -Wall -Wextra -Werror -std=c++98
HPP = $(wildcard *.hpp)

.PHONY: all clean fclean re

$(NAME): $(OBJ) $(HPP)
	g++ $(FLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	g++ -c $(FLAGS) $< -o $@

all: $(NAME)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all