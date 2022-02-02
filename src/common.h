#ifndef COMMON_H
#define COMMON_H

#define FILENAME_MAX_LENGTH (260)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <time.h>

#ifndef M_PI
#define M_PI (3.14159265358979)
#endif

#include <stdarg.h>

#define unused_expression(x) (void)(x)
#define unimplemented() assert(!"not implemented yet")

#define not_really_important_assert(x) assert(x)

#define local static
#define shared_storage static

#define TEMPORARY_STORAGE_BUFFER_SIZE (4096)
#define TEMPORARY_STORAGE_BUFFER_COUNT (4)


#define array_count(x) (sizeof(x) / sizeof(*x))
#define zero_buffer_memory(x, l) memset(x, 0, l)
#define zero_array(x) zero_buffer_memory(x, array_count(x) * sizeof(*x))

#define swap(t, a, b)                           \
    do {                                        \
        t _tmp = a;                             \
        a = b;                                  \
        b = _tmp;                               \
    } while(0)

struct rectangle {
    float x;
    float y;
    float w;
    float h;
};

shared_storage struct rectangle rectangle(float x, float y, float w, float h) {
    return (struct rectangle) {
        .x = x, .y = y, .w = w, .h = h,
    };
}

shared_storage struct rectangle rectangle_centered(float cx, float cy, float half_width, float half_height) {
    float min_x = cx - half_width;
    float min_y = cy - half_height;

    float max_x = cx + half_width;
    float max_y = cy + half_height;

    float width  = max_x - min_x;
    float height = max_y - min_y;

    return rectangle(min_x, min_y, width, height);
}

bool rectangle_intersects_v(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
bool rectangle_overlapping_v(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
bool rectangle_intersects(struct rectangle a, struct rectangle b);
bool rectangle_overlapping(struct rectangle a, struct rectangle b);

local float lerp(float a, float b, float t) {
    return (1.0 - t) * a + (b * t);
}

/* old stuff
   format =

   min, range, max_time, time
 */
local float cubic_ease_in(float b, float c, float d, float t){
    t /= d;

    return c * t * t * t + b;
}

local float cubic_ease_out(float b, float c, float d, float t){
    t /= d;

    t--;

    return c * ( t * t * t + 1 ) + b;
}

local float quadratic_ease_in(float b, float c, float d, float t){
    t /= d;

    return c * t * t + b;
}

local float quadratic_ease_out(float b, float c, float d, float t){
    t /= d;

    return (-c) * t * (t - 2) + b;
}

shared_storage float normalized_sinf(float p) {
    return (sinf(p) + 1) / 2.0f;
}

shared_storage float normalized_cosf(float p) {
    return (cosf(p) + 1) / 2.0f;
}

shared_storage float maxf(float a, float b) {
    if (a > b) return a;
    return b;
}

shared_storage float minf(float a, float b) {
    if (a < b) return a;
    return b;
}

shared_storage float clampf(float v, float a, float b) {
    if (v < a) v = a;
    else if (v > b) v = b;

    return v;
}

shared_storage int clampi(int v, int a, int b) {
    if (v < a) v = a;
    else if (v > b) v = b;

    return v;
}

shared_storage int max_int(int a, int b) {
    if (a > b) return a;
    return b;
}

shared_storage int min_int(int a, int b) {
    if (a < b) return a;
    return b;
}

shared_storage float float_sign(float x) {
    if (x > 0) return 1.0f;
    else if (x < 0) return -1.0f;

    return 0.0f;
}

/*beh*/
#define safe_assignment(x) if (x) *x 
#define random_element(fixed_size_array) fixed_size_array[random_ranged_integer(0, array_count(fixed_size_array))]

#define Kilobyte(x) (x * 1024)
#define Megabyte(x) (Kilobyte(x) * 1024)
#define Gigabyte(x) (Megabyte(x) * 1024)
#define Terabyte(x) (Gigabyte(x) * 1024)

char* clone_cstring(char* cstr);
float random_float(void);
float random_ranged_float(float min, float max);
int random_ranged_integer(int min, int max);

char* get_line_starting_from(char* text, int* starting_from);
char* format_temp(char* fmt, ...);

void read_file_into_buffer(char* path, char* dest, size_t length);
char* load_entire_file(char* path);

int count_lines_of_cstring(char* string);

void* system_allocate_memory(size_t amount);
void  system_deallocate_memory(void* ptr);
void* system_allocate_zeroed_memory(size_t amount);

inline shared_storage float degrees_to_radians(float deg) {
    return (deg * M_PI/180.0f);
}

inline shared_storage float safe_ratio(float a, float b) {
    if (b == 0.0f) return 0.0f;
    return (a/b);
}

inline shared_storage float degree_slope(float deg) {
    float radians = degrees_to_radians(deg);
    float y = sinf(radians);
    float x = cosf(radians);

    return safe_ratio(y, x);
}

uint64_t rdtsc(void);

#endif
