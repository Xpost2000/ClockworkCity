#ifndef GRAPHICS_H
#define GRAPHICS_H

union color4f {
    struct {
        float r;
        float g;
        float b;
        float a;
    };
    float rgba[4];
};
static union color4f COLOR4F_BLACK = {{0, 0, 0, 1}};
static union color4f COLOR4F_RED   = {{1, 0, 0, 1}};
static union color4f COLOR4F_GREEN = {{0, 1, 0, 1}};
static union color4f COLOR4F_BLUE  = {{0, 0, 1, 1}};
static union color4f COLOR4F_WHITE = {{1, 1, 1, 1}};
union color4f color4f(float r, float g, float b, float a);

void graphics_initialize(void* window_handle);
void graphics_deinitialize(void);

void begin_graphics_frame(void);
void end_graphics_frame(void);

void clear_color(union color4f color);
void draw_filled_rectangle(float x, float y, float w, float h, union color4f color);
void draw_rectangle(float x, float y, float w, float h, union color4f color);

#endif
