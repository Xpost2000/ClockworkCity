# TODO(jerry): Consider replacing these libraries with stb equivalents later since
# it makes distribution easier so I don't have to worry about lots of dlls.
CC=gcc
SRCFILES=glad/glad.c $(wildcard ./src/*.c) $(wildcard ./src/*.h)

CFLAGS=-std=c11
CLIBS=`pkg-config --libs --cflags sdl2 sdl2_ttf sdl2_mixer sdl2_image`

.phony: all clean build run

all: clean build

build: dgame.exe game.exe
src/generated_initialize_entity_assets.c: src/entity_asset_list.txt src/manifest_meta.lisp
	sbcl --script src/manifest_meta.lisp
dgame.exe: src/generated_initialize_entity_assets.c $(SRCFILES)
	$(CC) src/main.c src/common.c src/graphics.c src/input.c src/audio.c src/memory_arena.c $(CFLAGS) $(CLIBS) -ggdb3 -o $@
game.exe: src/generated_initialize_entity_assets.c $(SRCFILES)
	$(CC) src/main.c src/common.c src/graphics.c src/input.c src/audio.c src/memory_arena.c $(CFLAGS) $(CLIBS) -O2 -o $@
clean: 
	-rm game.exe
	-rm dgame.exe
drun: dgame.exe
	./dgame.exe
run: game.exe
	./game.exe
