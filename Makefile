CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=

SRC = \
    main.c \
    game.c \
    board.c \
    io.c

OBJ = $(SRC:.c=.o)

all: connect4

connect4: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) connect4

.PHONY: all clean
