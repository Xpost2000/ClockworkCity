#define RENDERER_MAX_TEXTURES (2048)
#define RENDERER_MAX_FONTS    (256)

local uint16_t font_count                   = 0;
local struct font fonts[RENDERER_MAX_FONTS+1] = {};
local uint16_t texture_count                           = 0;
local struct texture textures[RENDERER_MAX_TEXTURES+1] = {};
local SDL_Window*   global_window;
local struct camera* _active_camera = NULL;
local struct camera  camera_sentinel = {};
local int screen_dimensions[2] = {};

#include "camera.c"

void report_screen_dimensions(int* dimensions) {
    screen_dimensions[0] = dimensions[0];
    screen_dimensions[1] = dimensions[1];
}

void get_screen_dimensions(int* width, int* height) {
    safe_assignment(width)  = screen_dimensions[0];
    safe_assignment(height) = screen_dimensions[1];
}

void get_texture_dimensions(texture_id texture, int* width, int* height) {
    struct texture* texture_target = &textures[texture.id];
    safe_assignment(width)  = texture_target->width;
    safe_assignment(height) = texture_target->height;
}

void get_text_dimensions(font_id font, const char* cstr, int* width, int* height) {
    assert((font.id > 0 && font.id < RENDERER_MAX_FONTS+1) && "wtf? bad font id??");
    TTF_Font* font_object = fonts[font.id].font_object;

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

union color4f color4f(float r, float g, float b, float a) {
    return (union color4f) {
        .r = clampf(r, 0, 1.0),
        .g = clampf(g, 0, 1.0),
        .b = clampf(b, 0, 1.0),
        .a = clampf(a, 0, 1.0),
    };
}

union color4f color4f_normalize(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return color4f(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

union color4u8 color4u8(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (union color4u8) {
        .r = r,
        .g = g,
        .b = b,
        .a = a,
    };
}

union color4u8 color4u8_from_color4f(union color4f color) {
    return color4u8(clampf(color.r, 0, 1.0) * 255,
                    clampf(color.g, 0, 1.0) * 255,
                    clampf(color.b, 0, 1.0) * 255,
                    clampf(color.a, 0, 1.0) * 255);
}

float ratio_with_screen_width(float dividend) {
    return (float)screen_dimensions[0] / dividend;
}

float ratio_with_screen_height(float dividend) {
    return (float)screen_dimensions[1] / dividend;
}

bool within_screen_bounds(int x, int y, int w, int h) {
    return rectangle_overlapping_v(x, y, w, h, 0, 0, screen_dimensions[0], screen_dimensions[1]);
}

