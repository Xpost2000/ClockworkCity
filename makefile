# TODO(jerry): Consider replacing these libraries with stb equivalents later since
# it makes distribution easier so I don't have to worry about lots of dlls.
SRCFILES=$(wildcard ./src/*.c) $(wildcard ./src/*.h)

CFLAGS=-std=c11 -ggdb3
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

