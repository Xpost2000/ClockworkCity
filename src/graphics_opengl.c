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


/* these would go in a renderer, but these are actually meant to be global. So that's okay. */

struct texture {
    uint32_t width;
    uint32_t height;
    /*pitch optional*/
    uint8_t* pixels;
    /* for now I'll assume the format is rgba32, expensive but okay. Easiest to work with */
    uint32_t opengl_texture_object;
};

uint32_t opengl_global_text_texture_object  = 0;
struct font {
    TTF_Font* font_object;
};

#include "graphics_common.c"

local SDL_Renderer* global_renderer;


void graphics_initialize(void* window_handle) {
    global_window = window_handle;
}

void graphics_deinitialize(void) {
    unload_all_textures();
    unload_all_fonts();
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
    unimplemented();
}

void clear_color(union color4f color) {
    unimplemented();
}

/* these crappy shapes are going to just be one per draw call. Not efficient. Don't need these to be */
void draw_filled_rectangle(float x, float y, float w, float h, union color4f color) {
    unimplemented();
}

void draw_rectangle(float x, float y, float w, float h, union color4f color) {
    unimplemented();
}

void draw_texture(texture_id texture, float x, float y, float w, float h, union color4f color) {
    int dimens[2];
    get_texture_dimensions(texture, dimens, dimens+1);
    draw_texture_subregion(texture, x, y, w, h, 0, 0, dimens[0], dimens[1], color);
}

void draw_texture_subregion(texture_id texture, float x, float y, float w, float h,
                            int srx, int sry, int srw, int srh, union color4f color) {
    unimplemented();
}

float _draw_text_line(TTF_Font* font_object, float x, float y, const char* cstr, float scale, union color4f color) {
    unimplemented();
}

void draw_codepoint(font_id font, float x, float y, uint32_t codepoint, union color4f color) {
    unimplemented();
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
    while (current_line = get_line_starting_from(cstr, &line_starting_index)) {
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
        local SDL_PixelFormat* rgba32_format = NULL;

        if (!(rgba32_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32)));
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
        /* new_texture->texture_object = SDL_CreateTextureFromSurface(global_renderer, image_surface); */
        {
            
        }

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
        system_deallocate_memory(deleted->pixels);

        zero_buffer_memory(deleted, sizeof(*deleted));
    }
    textures[texture.id] = textures[--texture_count];
}
