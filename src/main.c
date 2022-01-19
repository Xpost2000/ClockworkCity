#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "common.h"
#include "graphics.h"

#define WINDOW_NAME "Metroidvania Jam 15"

SDL_Window* global_window;
bool running = true;

static void initialize(void) {
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);

    global_window = SDL_CreateWindow(
        WINDOW_NAME,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
    );

    graphics_initialize(global_window);
}

static void deinitialize(void) {
    graphics_deinitialize();
    SDL_DestroyWindow(global_window);
    IMG_Quit();
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
    unused_expression(argc);
    unused_expression(argv);

    initialize();

    texture_id knight_twoview = load_texture("assets/knight_twoview.png");

    while (running) {
        {
            SDL_Event event;

            while (SDL_PollEvent(&event)) {
                handle_sdl_event(event);
            }
        }

        begin_graphics_frame(); {
            clear_color(COLOR4F_BLACK);
            draw_filled_rectangle(100, 100, 250, 300, color4f(1.0, 0.0, 1.0, 1.0));
            draw_texture(knight_twoview, 150, 100, 200, 200, COLOR4F_WHITE);
        } end_graphics_frame();
    }

    deinitialize();
    return 0;
}
