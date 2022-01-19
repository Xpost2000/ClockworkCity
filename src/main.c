#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define WINDOW_NAME "Metroidvania Jam 15"

SDL_Window* global_window;
bool running = true;

static void initialize(void) {
    SDL_Init(SDL_INIT_EVERYTHING);
}

static void deinitialize(void) {
    SDL_DestroyWindow(global_window);
    SDL_Quit();
}

static void handle_sdl_event(SDL_Event event) {
    switch (event.type) {
        case SDL_QUIT: {
            running = false;
        } break;
    }
}

int main(int argc, char** argv) {
    initialize();

    global_window = SDL_CreateWindow(
        WINDOW_NAME,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
    );

    while (running) {
        {
            SDL_Event event;

            while (SDL_PollEvent(&event)) {
                handle_sdl_event(event);
            }
        }
    }

    deinitialize();
    return 0;
}
