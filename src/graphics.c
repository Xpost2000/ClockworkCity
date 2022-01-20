/*
  TODO(jerry):
  all of this is pretty slow right now but it'll get my job done for now.
  Rewrite this to be more efficient in the coming days once shit actually works.
*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "common.h"
#include "camera.h"
#include "graphics.h"

#define RENDERER_MAX_TEXTURES (2048)
#define RENDERER_MAX_FONTS    (256)

local uint16_t texture_count                         = 0;
local SDL_Texture*   textures[RENDERER_MAX_TEXTURES] = {};

local uint16_t font_count                 = 0;
local TTF_Font* fonts[RENDERER_MAX_FONTS] = {};

local SDL_Renderer* global_renderer;
local SDL_Window*   global_window;

local int screen_dimensions[2] = {};

/*camera code*/
local struct camera global_camera = {};
local struct camera* active_camera = NULL;
local struct camera dummy_camera = {};

struct camera* get_global_camera(void) {
    return &global_camera;
}

void set_active_camera(struct camera* camera) {
    if (camera == NULL) {
        active_camera = &dummy_camera;
    } else {
        active_camera = camera;
    }
}

local float _camera_lerp(float a, float b, float t) {
    return (1.0 - t) * a + (b * t);
}

local void _camera_transform_v2(float* x, float* y) {
    if (active_camera == &dummy_camera)
        return;

    assert(x && y);
    assert(screen_dimensions[0] > 0 && screen_dimensions[1] > 0);

    *x -= global_camera.visual_position_x;
    *x += screen_dimensions[0] / 2.0f;
    *y -= global_camera.visual_position_y;
    *y += screen_dimensions[1] / 2.0f;

    *x = floorf(*x);
    *y = floorf(*y);
}

void camera_set_position(float x, float y) {
    active_camera->visual_position_x = active_camera->target_position_x = active_camera->last_position_x = x;
    active_camera->visual_position_y = active_camera->target_position_y = active_camera->last_position_y = y;
}

void camera_set_focus_position(float x, float y) {
    active_camera->last_position_x = active_camera->visual_position_x;
    active_camera->last_position_y = active_camera->visual_position_y;

    active_camera->target_position_x = x;
    active_camera->target_position_y = y;

    active_camera->interpolation_time[0] = active_camera->interpolation_time[1] = 0.0;
}

void camera_set_focus_speed_x(float speed) {
    active_camera->interpolation_speed[0] = speed;
}

void camera_set_focus_speed_y(float speed) {
    active_camera->interpolation_speed[1] = speed;
}

void camera_set_focus_speed(float speed) {
    active_camera->interpolation_speed[0] = active_camera->interpolation_speed[1] = speed;
}

void camera_update(float dt) {
    /*
      this doesn't touch active camera. All cameras should be free to update
      on their own. It really shouldn't matter, and it probably looks more consistent
      this way.
     */
    for (int index = 0; index < 2; ++index) {
        if (global_camera.interpolation_time[index] < 1.0) {
            global_camera.interpolation_time[index] += dt * global_camera.interpolation_speed[index];
        }
    }

    /*
      Because of the frequency of set_focus_position... This is actually not linear but
      quadratic. So this is technically the *wrong* way to use lerp.

      But whatever.
     */
    global_camera.visual_position_x = _camera_lerp(global_camera.last_position_x, global_camera.target_position_x, global_camera.interpolation_time[0]);
    global_camera.visual_position_y = _camera_lerp(global_camera.last_position_y, global_camera.target_position_y, global_camera.interpolation_time[1]);
}

void camera_reset_transform(void) {
    global_camera = (struct camera) {
        .interpolation_speed[0] = 1.0, .interpolation_speed[1] = 1.0
    };
}

inline union color4f color4f(float r, float g, float b, float a) {
    return (union color4f) {
        .r = r, .g = g, .b = b, .a = a,
    };
}

void graphics_initialize(void* window_handle) {
    global_window = window_handle;
    global_renderer = SDL_CreateRenderer((SDL_Window*) window_handle, -1, SDL_RENDERER_ACCELERATED);
    active_camera = &global_camera;
}

void graphics_deinitialize(void) {
    SDL_DestroyRenderer(global_renderer);
}

void report_screen_dimensions(int* dimensions) {
    screen_dimensions[0] = dimensions[0];
    screen_dimensions[1] = dimensions[1];
}

local void _set_draw_color(union color4f color) {
    SDL_SetRenderDrawColor(global_renderer, color.r * 255, color.g * 255,
                           color.b * 255, color.a * 255);
}

/*rename?*/
void begin_graphics_frame(void) {
    /* TBD or semantic */
    set_active_camera(NULL);
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
    _camera_transform_v2(&x, &y);
    SDL_RenderFillRect(global_renderer, &(SDL_Rect){x, y, w, h});
}

void draw_rectangle(float x, float y, float w, float h, union color4f color) {
    _set_draw_color(color);
    _camera_transform_v2(&x, &y);
    SDL_RenderDrawRect(global_renderer, &(SDL_Rect){x, y, w, h});
}

void draw_texture(texture_id texture, float x, float y, float w, float h, union color4f color) {
    SDL_Texture* texture_object = textures[texture.id];
    assert(texture_object && "weird... bad texture?");

    SDL_SetTextureColorMod(texture_object, color.r * 255, color.g * 255, color.b * 255);
    SDL_SetTextureAlphaMod(texture_object, color.a * 255);

    _camera_transform_v2(&x, &y);
    SDL_RenderCopy(global_renderer, texture_object,
                   NULL, &(SDL_Rect){x, y, w, h});
}

/* 
   TODO(jerry): allow scaled fonts later 
   not really a big deal though.
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

        SDL_RenderCopy(global_renderer, rendered_text,
                       NULL, &(SDL_Rect){x, y, dimensions[0], dimensions[1]});
    }

    SDL_DestroyTexture(rendered_text);
    return dimensions[1];
}

void draw_text(font_id font, float x, float y, const char* cstr, union color4f color) {
    TTF_Font* font_object = fonts[font.id];
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

font_id load_font(const char* font_path, int size) {
    uint16_t free_id = font_count++;
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
    unimplemented();
}

void unload_texture(texture_id texture) {
    unimplemented();
}

void get_texture_dimensions(texture_id texture, int* width, int* height) {
    assert((texture.id >= 0 && texture.id < RENDERER_MAX_TEXTURES) && "wtf? bad texture id??");
    SDL_Texture* texture_object = textures[texture.id];
    SDL_QueryTexture(texture_object, 0, 0, width, height);
}

void get_text_dimensions(font_id font, const char* cstr, int* width, int* height) {
    assert((font.id >= 0 && font.id < RENDERER_MAX_FONTS) && "wtf? bad font id??");
    TTF_Font* font_object = fonts[font.id];

    int line_starting_index = 0;
    char* current_line;

    int max_width = 0;
    int max_height = 0;

    while (current_line = get_line_starting_from(cstr, &line_starting_index)) {
        int current_width = TTF_SizeUTF8(font_object, current_line, &max_width, NULL);
        if (current_width > max_width) {
            max_width = current_width;
        }

        max_height += TTF_FontHeight(font_object);
    }

    safe_assignment(width)  = max_width;
    safe_assignment(height) = max_height;
}
