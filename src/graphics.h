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

shared_storage union color4f COLOR4F_BLACK    = {{0, 0, 0, 1}};
shared_storage union color4f COLOR4F_RED      = {{1, 0, 0, 1}};
shared_storage union color4f COLOR4F_GREEN    = {{0, 1, 0, 1}};
shared_storage union color4f COLOR4F_BLUE     = {{0, 0, 1, 1}};
shared_storage union color4f COLOR4F_WHITE    = {{1, 1, 1, 1}};
shared_storage union color4f COLOR4F_YELLOW   = {{1, 1, 0.6235, 1}};
shared_storage union color4f COLOR4F_DARKGRAY = {{0.3, 0.3, 0.3, 1}};

union color4f color4f(float r, float g, float b, float a);
bool within_screen_bounds(int x, int y, int w, int h);

float ratio_with_screen_width(float dividend);
float ratio_with_screen_height(float dividend);

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

void present_graphics_frame(void);

void report_screen_dimensions(int* dimensions);
void get_screen_dimensions(int* width, int* height);

void clear_color(union color4f color);
void set_render_scale(float scale_factor);
float get_render_scale(void);

void draw_filled_rectangle(float x, float y, float w, float h, union color4f color);
void draw_rectangle(float x, float y, float w, float h, union color4f color);
void draw_texture(texture_id texture, float x, float y, float w, float h, union color4f color);
void draw_texture_subregion(texture_id texture, float x, float y, float w, float h, int srx, int sry, int srw, int srh, union color4f color);
void draw_text(font_id font, float x, float y, const char* cstr, union color4f color);
void draw_text_right_justified(font_id font, float x, float y, float w, const char* cstr, union color4f color);
void draw_codepoint(font_id font, float x, float y, uint32_t codepoint, union color4f color);

void unload_all_textures(void);
void unload_all_fonts(void);

#endif
