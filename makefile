SRCFILES=src/main.c
CFLAGS=-Wall -Wextra -std=c11
CLIBS=

all: clean build

build: game.exe
game.exe: $(SRCFILES)
	gcc src/main.c $(CFLAGS) $(CLIBS) -o $@


clean: 
	rm game.exe


.phony: all clean build
