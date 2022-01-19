#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "common.h"
#include "graphics.h"

#define RENDERER_MAX_TEXTURES (2048)
local uint16_t texture_count                         = 0;
local SDL_Texture*   textures[RENDERER_MAX_TEXTURES] = {};

local SDL_Renderer* global_renderer;
local SDL_Window*   global_window;

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

local void _set_draw_color(union color4f color) {
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

void draw_texture(texture_id texture, float x, float y, float w, float h, union color4f color) {
    SDL_Texture* texture_object = textures[texture.id];
    assert(texture_object && "weird... bad texture?");

    SDL_SetTextureColorMod(texture_object, color.r * 255, color.g * 255, color.b * 255);
    SDL_SetTextureAlphaMod(texture_object, color.a * 255);

    SDL_RenderCopy(global_renderer, texture_object,
                   NULL, &(SDL_Rect){x, y, w, h});
}

texture_id load_texture(const char* texture_path) {
    uint16_t free_id = texture_count++;
    texture_id result = {
        .id = free_id
    };

    SDL_Surface* image_surface = IMG_Load(texture_path);

    assert(texture_count < RENDERER_MAX_TEXTURES && "too many textures");
    assert(image_surface && "bad image");

    textures[free_id] =
        SDL_CreateTextureFromSurface(global_renderer, image_surface);

    SDL_FreeSurface(image_surface);
    return result;
}

void unload_texture(texture_id texture) {
    // TODO(jerry);
}
