#include "common.h"
#include "graphics.h"

#include <SDL2/SDL.h>

static SDL_Renderer* global_renderer;
static SDL_Window*   global_window;

inline union color4f color4f(float r, float g, float b, float a) {
    return (union color4f) {
        .r = r, .g = g, .b = b, .a = a,
    };
}

void graphics_initialize(void* window_handle) {
    global_window = window_handle;
    global_renderer = SDL_CreateRenderer((SDL_Window*) window_handle, -1, SDL_RENDERER_ACCELERATED);
}

void graphics_deinitialize(void) {
    SDL_DestroyRenderer(global_renderer);
}

void begin_graphics_frame(void) {
    
}

void end_graphics_frame(void) {
    SDL_RenderPresent(global_renderer); 
}

static void _set_draw_color(union color4f color) {
    SDL_SetRenderDrawColor(global_renderer, color.r * 255, color.g * 255,
                           color.b * 255, color.a * 255);
}

void clear_color(union color4f color) {
    _set_draw_color(color);
    SDL_RenderClear(global_renderer);
}

void draw_filled_rectangle(float x, float y, float w, float h, union color4f color) {
    _set_draw_color(color);
    SDL_RenderFillRect(global_renderer, &(SDL_Rect){x, y, w, h});
}

void draw_rectangle(float x, float y, float w, float h, union color4f color) {
    _set_draw_color(color);
    SDL_RenderDrawRect(global_renderer, &(SDL_Rect){x, y, w, h});
}
