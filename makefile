SRCFILES=src/main.c \
	 src/graphics.c \
	 src/graphics.h \
	 src/audio.c \
	 src/audio.h \
	 src/camera.h \
	 src/input.c \
	 src/input.h \
	 src/game.c \
	 src/common.h\
	 src/common.c

CFLAGS=-Wall -Wextra -std=c11 -ggdb3
CLIBS=`pkg-config --libs --cflags sdl2 sdl2_ttf sdl2_mixer sdl2_image`

.phony: all clean build run

all: clean build

build: game.exe
run: game.exe
	./game.exe

game.exe: $(SRCFILES)
	gcc src/main.c src/common.c src/graphics.c src/input.c src/audio.c $(CFLAGS) $(CLIBS) -o $@


clean: 
	-rm game.exe

