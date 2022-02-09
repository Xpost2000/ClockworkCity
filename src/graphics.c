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
  
  TODO(jerry):
  Deprecating this, and favoring opengl only.
*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "common.h"
#include "camera.h"
#include "graphics.h"

#define IS_OPENGL 0
struct texture {
    uint32_t width;
    uint32_t height;
    /*pitch optional*/
    uint8_t* pixels;
    /* for now I'll assume the format is rgba32, expensive but okay. Easiest to work with */
    SDL_Texture* texture_object;
};

struct font {
    TTF_Font* font_object;
};

#include "graphics_common.c"
local SDL_Renderer* global_renderer;

void graphics_initialize(void* window_handle) {
    global_window = window_handle;
    /*hardware*/
    global_renderer = SDL_CreateRenderer((SDL_Window*) window_handle, -1,
                                         SDL_RENDERER_ACCELERATED /* | SDL_RENDERER_PRESENTVSYNC */
                                         /* SDL_RENDERER_SOFTWARE */
    );
    SDL_SetRenderDrawBlendMode(global_renderer,  SDL_BLENDMODE_BLEND);
}

void graphics_deinitialize(void) {
    SDL_DestroyRenderer(global_renderer);
}

local void _set_draw_color(union color4f color) {
    SDL_SetRenderDrawColor(global_renderer, color.r * 255, color.g * 255,
                           color.b * 255, color.a * 255);
}

void begin_graphics_frame(struct camera* camera) {
    camera_set_position(&camera_sentinel, screen_dimensions[0]/2, screen_dimensions[1]/2);
    _active_camera = camera;

    if (!_active_camera) {
        _active_camera = &camera_sentinel;
    }
}

void end_graphics_frame(void) {
    _active_camera = NULL;
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
    x *= _camera_render_scale(_active_camera);
    y *= _camera_render_scale(_active_camera);
    w *= _camera_render_scale(_active_camera);
    h *= _camera_render_scale(_active_camera);

    if (w < 1) w = 1;
    if (h < 1) h = 1;
    _camera_transform_v2(_active_camera, &x, &y);
    if (within_screen_bounds(x, y, w, h)) {
        SDL_RenderFillRect(global_renderer, &(SDL_Rect){x, y, w, h});
    }
}

void draw_rectangle(float x, float y, float w, float h, union color4f color) {
    _set_draw_color(color);
    x *= _camera_render_scale(_active_camera);
    y *= _camera_render_scale(_active_camera);
    w *= _camera_render_scale(_active_camera);
    h *= _camera_render_scale(_active_camera);
    _camera_transform_v2(_active_camera, &x, &y);

    if (w < 1) w = 1;
    if (h < 1) h = 1;
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
    SDL_Texture* texture_object = textures[texture.id].texture_object;

    x *= _camera_render_scale(_active_camera);
    y *= _camera_render_scale(_active_camera);
    /* weird sdl rendering shenanigans. */
    if (roundf(_camera_render_scale(_active_camera)) != 1.0f) {
        w *= _camera_render_scale(_active_camera)+1;
        h *= _camera_render_scale(_active_camera)+1;
    }

    if (texture.id != 0)
        assert(texture_object && "weird... bad texture?");

    SDL_SetTextureColorMod(texture_object, color.r * 255, color.g * 255, color.b * 255);
    SDL_SetTextureAlphaMod(texture_object, color.a * 255);

    _camera_transform_v2(_active_camera, &x, &y);

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
float _draw_text_line(TTF_Font* font_object, float x, float y, const char* cstr, float scale, union color4f color) {
    SDL_Texture* rendered_text;
    SDL_Surface* text_surface = TTF_RenderUTF8_Blended(font_object, cstr,
                                                       (SDL_Color) {color.r * 255, color.g * 255,
                                                           color.b * 255, color.a * 255});
    _camera_transform_v2(_active_camera, &x, &y);
    rendered_text = SDL_CreateTextureFromSurface(global_renderer, text_surface);
    SDL_FreeSurface(text_surface);

    int dimensions[2] = {};
    {
        SDL_QueryTexture(rendered_text, 0, 0, dimensions, dimensions+1);

        if (within_screen_bounds(x, y, dimensions[0], dimensions[1])) {
            SDL_RenderCopy(global_renderer, rendered_text,
                           NULL, &(SDL_Rect){x, y, dimensions[0] * scale, dimensions[1] * scale});
        }
    }

    SDL_DestroyTexture(rendered_text);
    return dimensions[1];
}

void draw_codepoint(font_id font, float x, float y, uint32_t codepoint, union color4f color) {
    TTF_Font* font_object = fonts[font.id].font_object;
    if (font.id != 0)
        assert(font_object && "weird... bad font?");

    SDL_Texture* rendered_text;
    SDL_Surface* text_surface = TTF_RenderGlyph_Blended(font_object, codepoint,
                                                        (SDL_Color) {color.r * 255, color.g * 255,
                                                           color.b * 255, color.a * 255});

    _camera_transform_v2(_active_camera, &x, &y);
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
}

void draw_text(font_id font, float x, float y, const char* cstr, union color4f color) {
    draw_text_scaled(font, x, y, cstr, 1, color);
}

void draw_text_scaled(font_id font, float x, float y, const char* cstr, float scale, union color4f color) {
    TTF_Font* font_object = fonts[font.id].font_object;

    if (font.id != 0)
        assert(font_object && "weird... bad font?");

    int line_starting_index = 0;
    char* current_line;

    float y_cursor = 0;
    while ((current_line = get_line_starting_from(cstr, &line_starting_index))) {
        float line_offset = _draw_text_line(font_object, x, y + y_cursor, current_line, scale, color);
        y_cursor += line_offset;
    }
}

void draw_text_right_justified(font_id font, float x, float y, float w, const char* cstr, union color4f color) {
    int width;
    get_text_dimensions(font, cstr, &width, NULL);
    draw_text(font, x + (w-width), y, cstr, color);
}

struct image_buffer get_texture_buffer(texture_id id) {
    struct texture* texture_object = &textures[id.id];

    return (struct image_buffer) {
        .width  = texture_object->width,
        .height = texture_object->height,
        .pixels = texture_object->pixels 
    };
};

texture_id load_texture(const char* texture_path) {
    uint16_t free_id = 1 + texture_count++;
    texture_id result = {
        .id = free_id
    };

    SDL_Surface* image_surface = IMG_Load(texture_path);
    {
        /*stupid but I don't want to have lots of special case code as I want to read the pixels for reasons and it's easier
          to just have one format IE: prefer RGBA exclusive. Makes loading a bit slower... 
         Probably unnoticable though.*/

        /* lazy init implicit global object. Never free */
        local SDL_PixelFormat* rgba32_format = NULL;

        if (!(rgba32_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32)))
            ;
        assert(rgba32_format && "uh... wtf? SDL failed to make pixel format structure?");

        SDL_Surface* reformatted_as_rgba32 = SDL_ConvertSurface(image_surface, rgba32_format, 0);
        assert(reformatted_as_rgba32 && "conversion failed? wtf?");
        SDL_FreeSurface(image_surface);
        image_surface = reformatted_as_rgba32;
    }

    assert(texture_count < RENDERER_MAX_TEXTURES && "too many textures");
    assert(image_surface && "bad image");

    {
        struct texture* new_texture = &textures[free_id];
        new_texture->texture_object = SDL_CreateTextureFromSurface(global_renderer, image_surface);

        uint32_t image_width  = image_surface->w;
        uint32_t image_height = image_surface->h;

        new_texture->width  = image_width;
        new_texture->height = image_height;
        assert(image_surface->format->BytesPerPixel == 4 && "Only RGBA images for simplicity!");
        new_texture->pixels = system_clone_buffer(image_surface->pixels,
                                                  image_surface->format->BytesPerPixel * image_width * image_height);
    }

    SDL_FreeSurface(image_surface);
    return result;
}

font_id load_font(const char* font_path, int size) {
    uint16_t free_id = 1 + font_count++;
    font_id result = {
        .id = free_id
    };

    assert(font_count < RENDERER_MAX_FONTS && "too many fonts");

    struct font* new_font = fonts + free_id;
    new_font->font_object = TTF_OpenFont(font_path, size);
    assert(new_font->font_object && "bad font");

    return result;
}

void unload_font(font_id font) {
    struct font* deleted = fonts + font.id;
    {
        TTF_CloseFont(deleted->font_object);
        deleted->font_object = NULL;
    }
    fonts[font.id] = fonts[--font_count];
}

void unload_texture(texture_id texture) {
    struct texture* deleted = &textures[texture.id];
    {
        SDL_DestroyTexture(deleted->texture_object);
        system_deallocate_memory(deleted->pixels);

        zero_buffer_memory(deleted, sizeof(*deleted));
    }
    textures[texture.id] = textures[--texture_count];
}
