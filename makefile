SRCFILES=src/main.c
CFLAGS=-Wall -Wextra -std=c11
CLIBS=`pkg-config --libs --cflags sdl2`

.phony: all clean build run

all: clean build

build: game.exe
run: game.exe
	./game.exe

game.exe: $(SRCFILES)
	gcc src/main.c $(CFLAGS) $(CLIBS) -o $@


clean: 
	-rm game.exe

