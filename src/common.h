#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

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
#define zero_array(x) zero_buffer_memory(x, array_count(x))

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

bool rectangle_intersects_v(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
bool rectangle_overlapping_v(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
bool rectangle_intersects(struct rectangle a, struct rectangle b);
bool rectangle_overlapping(struct rectangle a, struct rectangle b);

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
int random_ranged_integer(int min, int max);

char* get_line_starting_from(char* text, int* starting_from);
char* format_temp(char* fmt, ...);

void read_file_into_buffer(char* path, char* dest, size_t length);
char* load_entire_file(char* path);

int count_lines_of_cstring(char* string);

void* system_allocate_memory(size_t amount);
void  system_deallocate_memory(void* ptr);
void* system_allocate_zeroed_memory(size_t amount);

uint64_t rdtsc(void);

#endif
