SRCFILES=src/main.c \
	 src/graphics.c \
	 src/graphics.h

CFLAGS=-Wall -Wextra -std=c11
CLIBS=`pkg-config --libs --cflags sdl2 sdl2_ttf sdl2_mixer sdl2_image`

.phony: all clean build run

all: clean build

build: game.exe
run: game.exe
	./game.exe

game.exe: $(SRCFILES)
	gcc src/main.c src/graphics.c $(CFLAGS) $(CLIBS) -o $@


clean: 
	-rm game.exe

