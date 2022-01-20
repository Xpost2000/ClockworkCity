#ifndef GRAPHICS_H
#define GRAPHICS_H

typedef struct {
    uint16_t id;
} texture_id;

typedef struct {
    uint16_t id;
} font_id;

union color4f {
    struct {
        float r;
        float g;
        float b;
        float a;
    };
    float rgba[4];
};

shared_storage union color4f COLOR4F_BLACK = {{0, 0, 0, 1}};
shared_storage union color4f COLOR4F_RED   = {{1, 0, 0, 1}};
shared_storage union color4f COLOR4F_GREEN = {{0, 1, 0, 1}};
shared_storage union color4f COLOR4F_BLUE  = {{0, 0, 1, 1}};
shared_storage union color4f COLOR4F_WHITE = {{1, 1, 1, 1}};

union color4f color4f(float r, float g, float b, float a);

void get_texture_dimensions(texture_id texture, int* width, int* height);
void get_text_dimensions(font_id font, const char* cstr, int* width, int* height);

font_id    load_font(const char* font_path, int size);
texture_id load_texture(const char* texture_path);
void       unload_font(font_id font);
void       unload_texture(texture_id texture);

void graphics_initialize(void* window_handle);
void graphics_deinitialize(void);

void begin_graphics_frame(void);
void end_graphics_frame(void);
void report_screen_dimensions(int* dimensions);

void clear_color(union color4f color);

void draw_filled_rectangle(float x, float y, float w, float h, union color4f color);
void draw_rectangle(float x, float y, float w, float h, union color4f color);
void draw_texture(texture_id texture, float x, float y, float w, float h, union color4f color);
void draw_text(font_id font, float x, float y, const char* cstr, union color4f color);

#endif
