# TODO(jerry): Consider replacing these libraries with stb equivalents later since
# it makes distribution easier so I don't have to worry about lots of dlls.
SRCFILES=glad/glad.c $(wildcard ./src/*.c) $(wildcard ./src/*.h)

CFLAGS=-std=c11
CLIBS=`pkg-config --libs --cflags sdl2 sdl2_ttf sdl2_mixer sdl2_image`

.phony: all clean build run

all: clean build

build: dgame.exe game.exe
dgame.exe: $(SRCFILES)
	gcc glad/glad.c src/main.c src/common.c src/graphics.c src/input.c src/audio.c src/memory_arena.c $(CFLAGS) $(CLIBS) -ggdb3 -o $@
game.exe: $(SRCFILES)
	gcc glad/glad.c src/main.c src/common.c src/graphics.c src/input.c src/audio.c src/memory_arena.c $(CFLAGS) $(CLIBS) -O2 -o $@
clean: 
	-rm game.exe
	-rm dgame.exe
drun: dgame.exe
	./dgame.exe
run: game.exe
	./game.exe
