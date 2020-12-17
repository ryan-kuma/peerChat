.PHONY:clean all
CC=gcc
CFLAGS=-Wall -g -lcurses
BIN=srvPeer cliPeer
all:$(BIN)
%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -rf *.o $(BIN)
