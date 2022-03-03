#ifndef GRAPHICS_H
#define GRAPHICS_H

typedef struct {
    uint16_t id;
} texture_id;

typedef struct {
    uint16_t id;
} font_id;

union color4u8 {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    uint32_t rgba;
    uint8_t  rgba_bytes[4];
};

union color4f {
    struct {
        float r;
        float g;
        float b;
        float a;
    };
    float rgba[4];
};

/*never change C*/
#define COLOR4F_normalize(R, G, B, A)           \
    (union color4f) {                           \
        .r = R /255.0f,                         \
            .g = G /255.0f,                     \
            .b = B /255.0f,                     \
            .a = A /255.0f,                     \
            }

/* implicitly rgba32 */
struct image_buffer {
    uint32_t width;
    uint32_t height;
    uint8_t* pixels;
};

#define COLOR4F_BLACK    (union color4f) {{0, 0, 0, 1}}
#define COLOR4F_RED      (union color4f) {{1, 0, 0, 1}}
#define COLOR4F_GREEN    (union color4f) {{0, 1, 0, 1}}
#define COLOR4F_BLUE     (union color4f) {{0, 0, 1, 1}}
#define COLOR4F_WHITE    (union color4f) {{1, 1, 1, 1}}
#define COLOR4F_YELLOW   (union color4f) {{1, 1, 0.6235, 1}}
#define COLOR4F_DARKGRAY (union color4f) {{0.3, 0.3, 0.3, 1}}
#define COLOR4F_MAGENTA  (union color4f) {{1, 0, 1, 1}}
#define COLOR4F_PURPLE   (union color4f) {{0.5, 0, 0.5, 1}}

union color4f color4f_lerp(union color4f a, union color4f b, float t);
union color4f color4f(float r, float g, float b, float a);
union color4f color4f_normalize(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
union color4f color4f_invert(union color4f base);
union color4u8 color4u8(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
union color4u8 color4u8_from_color4f(union color4f color);
bool within_screen_bounds(int x, int y, int w, int h);

float ratio_with_screen_width(float dividend);
float ratio_with_screen_height(float dividend);

void get_texture_dimensions(texture_id texture, int* width, int* height);

/*no _scaled version, you can just scale afterwards yourself.*/
void get_text_dimensions(font_id font, const char* cstr, int* width, int* height); 
void get_codepoint_dimensions(font_id font, uint32_t codepoint, int* width, int* height); 

font_id    load_font(const char* font_path, int size);

texture_id load_texture(const char* texture_path);
void       unload_font(font_id font);
void       unload_texture(texture_id texture);

struct image_buffer get_texture_buffer(texture_id texture);

void graphics_initialize(void* window_handle);
void graphics_deinitialize(void);

void present_graphics_frame(void);

void report_screen_dimensions(int* dimensions);
void get_screen_dimensions(int* width, int* height);

void clear_color(union color4f color);

void begin_graphics_frame(struct camera* camera); /*null is okay*/
void end_graphics_frame(void);

/* This is for immediately drawing, this isn't very efficient. */
void draw_filled_rectangle(float x, float y, float w, float h, union color4f color);
/* draw as a line buffer lol */
void draw_rectangle(float x, float y, float w, float h, union color4f color);
void draw_bresenham_filled_rectangle_line(float x_off, float y_off, int x1, int y1, int x2, int y2, float square_size, union color4f color);
void draw_texture(texture_id texture, float x, float y, float w, float h, union color4f color);
void draw_texture_ext(texture_id texture, float x, float y, float w, float h, union color4f color, bool flipped, float angle);
void draw_texture_subregion(texture_id texture, float x, float y, float w, float h, int srx, int sry, int srw, int srh, union color4f color);
void draw_texture_subregion_ext(texture_id texture, float x, float y, float w, float h, int srx, int sry, int srw, int srh, union color4f color, bool flipped, float angle);
void draw_texture_aligned(texture_id texture, float x, float y, float tw, float th, float scale, float cx, float cy, union color4f color, bool flipped, float angle);
void draw_text(font_id font, float x, float y, const char* cstr, union color4f color);
void draw_text_right_justified(font_id font, float x, float y, float w, const char* cstr, union color4f color);
void draw_text_center_justified(font_id font, float x, float y, float w, float h, const char* cstr, union color4f color);
void draw_text_horizontal_center_justified(font_id font, float x, float y, float w, const char* cstr, union color4f color);
void draw_text_vertical_center_justified(font_id font, float x, float y, float h, const char* cstr, union color4f color);

#if 0
/* I'm not adding an explicit shader api yet... */
void draw_mesh(mesh_id mesh);
#endif

#if 0
/* this is the batching api */
/* this is meant to be used for the game itself, to reduce rendering time. */
/* relies on a stable sort for this to work properly. */

void begin_batch(void); 

void push_filled_rectangle(float x, float y, float w, float h);
void push_texture(texture_id texture, float x, float y, float w, float h, union color4f color);
void push_texture_subregion(texture_id texture, float x, float y, float w, float h, union color4f color);
void push_texture_ext(texture_id texture, float x, float y, float w, float h, union color4f color, int layer);
void push_texture_subregion_ext(texture_id texture, float x, float y, float w, float h, union color4f color, int layer);

/* render all batches */
void end_batch(void);
#endif

/*don't need scaled*/
void draw_text_scaled(font_id font, float x, float y, const char* cstr, float scale, union color4f color);

void draw_codepoint(font_id font, float x, float y, uint32_t codepoint, union color4f color);

void unload_all_textures(void);
void unload_all_fonts(void);

#endif
