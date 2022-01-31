/*
  TODO(jerry):
  all of this is pretty slow right now but it'll get my job done for now.
  Rewrite this to be more efficient in the coming days once shit actually works.
  
  TODO(jerry):
  fix the weird id issues and assertions
  
  TODO(jerry):
  Blend modes
  
  TODO(jerry):
  batching (I mean I'm going to turn this into an opengl renderer, but we do need render commands!)
*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "common.h"
#include "camera.h"
#include "graphics.h"

#define RENDERER_MAX_TEXTURES (2048)
#define RENDERER_MAX_FONTS    (256)

local uint16_t texture_count                           = 0;
local SDL_Texture*   textures[RENDERER_MAX_TEXTURES+1] = {};

local uint16_t font_count                   = 0;
local TTF_Font* fonts[RENDERER_MAX_FONTS+1] = {};

local SDL_Renderer* global_renderer;
local SDL_Window*   global_window;

local int screen_dimensions[2] = {};

/* this shouldn't be called debug anymore, since this is legitmately used lol */
local float DEBUG_scale = 1.0;
#include "camera.c"

inline union color4f color4f(float r, float g, float b, float a) {
    return (union color4f) {
        .r = r, .g = g, .b = b, .a = a,
    };
}

float ratio_with_screen_width(float dividend) {
    return (float)screen_dimensions[0] / dividend;
}

float ratio_with_screen_height(float dividend) {
    return (float)screen_dimensions[1] / dividend;
}

/* separate from camera scaling mind you! */
void set_render_scale(float scale_factor) {
    DEBUG_scale = scale_factor;
}

float get_render_scale(void) {
    return DEBUG_scale;
}

bool within_screen_bounds(int x, int y, int w, int h) {
    return rectangle_overlapping_v(x, y, w, h, 0, 0, screen_dimensions[0], screen_dimensions[1]);
}

void graphics_initialize(void* window_handle) {
    global_window = window_handle;
    /*hardware*/
    global_renderer = SDL_CreateRenderer((SDL_Window*) window_handle, -1,
                                         SDL_RENDERER_ACCELERATED /* | SDL_RENDERER_PRESENTVSYNC */
                                         /* SDL_RENDERER_SOFTWARE */
    );
    SDL_SetRenderDrawBlendMode(global_renderer,  SDL_BLENDMODE_BLEND);
    active_camera = &global_camera;
}

void graphics_deinitialize(void) {
    SDL_DestroyRenderer(global_renderer);
}

void report_screen_dimensions(int* dimensions) {
    screen_dimensions[0] = dimensions[0];
    screen_dimensions[1] = dimensions[1];
}

void get_screen_dimensions(int* width, int* height) {
    safe_assignment(width)  = screen_dimensions[0];
    safe_assignment(height) = screen_dimensions[1];
}

local void _set_draw_color(union color4f color) {
    SDL_SetRenderDrawColor(global_renderer, color.r * 255, color.g * 255,
                           color.b * 255, color.a * 255);
}

/*rename?*/
void begin_graphics_frame(void) {
    /* TBD or semantic */
    set_active_camera(NULL);
    set_render_scale(1.0);
}

void end_graphics_frame(void) {
    /* TBD */
}

void present_graphics_frame(void) {
    SDL_RenderPresent(global_renderer);
}

void clear_color(union color4f color) {
    _set_draw_color(color);
    SDL_RenderClear(global_renderer);
}

void draw_filled_rectangle(float x, float y, float w, float h, union color4f color) {
    _set_draw_color(color);
    x *= DEBUG_scale;
    y *= DEBUG_scale;
    w *= DEBUG_scale;
    h *= DEBUG_scale;
    _camera_transform_v2(&x, &y);
    SDL_RenderFillRect(global_renderer, &(SDL_Rect){x, y, w, h});
}

void draw_rectangle(float x, float y, float w, float h, union color4f color) {
    _set_draw_color(color);
    x *= DEBUG_scale;
    y *= DEBUG_scale;
    w *= DEBUG_scale;
    h *= DEBUG_scale;
    _camera_transform_v2(&x, &y);

    if (within_screen_bounds(x, y, w, h)) {
        SDL_RenderDrawRect(global_renderer, &(SDL_Rect){x, y, w, h});
    }
}

void draw_texture(texture_id texture, float x, float y, float w, float h, union color4f color) {
    int dimens[2];
    get_texture_dimensions(texture, dimens, dimens+1);
    draw_texture_subregion(texture, x, y, w, h, 0, 0, dimens[0], dimens[1], color);
}

void draw_texture_subregion(texture_id texture, float x, float y, float w, float h,
                            int srx, int sry, int srw, int srh, union color4f color) {
    SDL_Texture* texture_object = textures[texture.id];

    x *= DEBUG_scale;
    y *= DEBUG_scale;
    /* weird sdl rendering shenanigans. */
    if (roundf(DEBUG_scale) != 1.0f) {
        w *= DEBUG_scale+1;
        h *= DEBUG_scale+1;
    }

    if (texture.id != 0)
        assert(texture_object && "weird... bad texture?");

    SDL_SetTextureColorMod(texture_object, color.r * 255, color.g * 255, color.b * 255);
    SDL_SetTextureAlphaMod(texture_object, color.a * 255);

    _camera_transform_v2(&x, &y);

    if (within_screen_bounds(x, y, w, h)) {
        SDL_RenderCopy(global_renderer, texture_object,
                       &(SDL_Rect){srx, sry, srw, srh},
                       &(SDL_Rect){x, y, w, h});
    }
}

/* 
   TODO(jerry):
   We need a form of "dynamic" font, that can be used for ingame renderings.
   IE: a scaled font.
*/
float _draw_text_line(TTF_Font* font_object, float x, float y, const char* cstr, union color4f color) {
    SDL_Texture* rendered_text;
    SDL_Surface* text_surface = TTF_RenderUTF8_Blended(font_object, cstr,
                                                       (SDL_Color) {color.r * 255, color.g * 255,
                                                           color.b * 255, color.a * 255});
    _camera_transform_v2(&x, &y);
    rendered_text = SDL_CreateTextureFromSurface(global_renderer, text_surface);
    SDL_FreeSurface(text_surface);

    int dimensions[2] = {};
    {
        SDL_QueryTexture(rendered_text, 0, 0, dimensions, dimensions+1);

        if (within_screen_bounds(x, y, dimensions[0], dimensions[1])) {
            SDL_RenderCopy(global_renderer, rendered_text,
                           NULL, &(SDL_Rect){x, y, dimensions[0], dimensions[1]});
        }
    }

    SDL_DestroyTexture(rendered_text);
    return dimensions[1];
}

void draw_codepoint(font_id font, float x, float y, uint32_t codepoint, union color4f color) {
    TTF_Font* font_object = fonts[font.id];
    if (font.id != 0)
        assert(font_object && "weird... bad font?");

    SDL_Texture* rendered_text;
    SDL_Surface* text_surface = TTF_RenderGlyph_Blended(font_object, codepoint,
                                                        (SDL_Color) {color.r * 255, color.g * 255,
                                                           color.b * 255, color.a * 255});

    _camera_transform_v2(&x, &y);
    rendered_text = SDL_CreateTextureFromSurface(global_renderer, text_surface);
    SDL_FreeSurface(text_surface);

    int dimensions[2] = {};
    {
        SDL_QueryTexture(rendered_text, 0, 0, dimensions, dimensions+1);
        SDL_RenderCopy(global_renderer, rendered_text,
                       NULL, &(SDL_Rect){x, y, dimensions[0], dimensions[1]});
    }

    SDL_DestroyTexture(rendered_text);
}

void draw_text(font_id font, float x, float y, const char* cstr, union color4f color) {
    TTF_Font* font_object = fonts[font.id];

    if (font.id != 0)
        assert(font_object && "weird... bad font?");

    /*
      sorry performance gods.
      this is the easiest thing right now
     */
    int line_starting_index = 0;
    char* current_line;

    float y_cursor = 0;
    while (current_line = get_line_starting_from(cstr, &line_starting_index)) {
        float line_offset = _draw_text_line(font_object, x, y + y_cursor, current_line, color);
        y_cursor += line_offset;
    }
}

texture_id load_texture(const char* texture_path) {
    uint16_t free_id = 1 + texture_count++;
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

font_id load_font(const char* font_path, int size) {
    uint16_t free_id = 1 + font_count++;
    font_id result = {
        .id = free_id
    };

    assert(font_count < RENDERER_MAX_FONTS && "too many fonts");

    fonts[free_id] =
        TTF_OpenFont(font_path, size);

    assert(fonts[free_id] && "bad font");

    return result;
}

void unload_font(font_id font) {
    fonts[font.id] = fonts[--font_count];
}

void unload_texture(texture_id texture) {
    textures[texture.id] = textures[--texture_count];
}

void get_texture_dimensions(texture_id texture, int* width, int* height) {
    /* assert((texture.id > 0 && texture.id < RENDERER_MAX_TEXTURES+1) && "wtf? bad texture id??"); */
    SDL_Texture* texture_object = textures[texture.id];
    SDL_QueryTexture(texture_object, 0, 0, width, height);
}

void get_text_dimensions(font_id font, const char* cstr, int* width, int* height) {
    assert((font.id > 0 && font.id < RENDERER_MAX_FONTS+1) && "wtf? bad font id??");
    TTF_Font* font_object = fonts[font.id];

    int line_starting_index = 0;
    char* current_line;

    int max_width = 0;
    int max_height = 0;

    while (current_line = get_line_starting_from(cstr, &line_starting_index)) {
        int current_width;
        TTF_SizeUTF8(font_object, current_line, &current_width, NULL);
        if (current_width > max_width) {
            max_width = current_width;
        }

        max_height += TTF_FontHeight(font_object);
    }

    safe_assignment(width)  = max_width;
    safe_assignment(height) = max_height;
}

void unload_all_textures(void) {
    for (unsigned index = texture_count; index != 0; --index) {
        unload_texture((texture_id){ .id = index });
    }

    assert(texture_count==0);
}

void unload_all_fonts(void) {
    for (unsigned index = font_count; index != 0; --index) {
        unload_font((font_id){ .id = index });
    }

    assert(font_count==0);
}
