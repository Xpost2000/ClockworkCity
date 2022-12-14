/* avoid multiple linkage */
#include "common.h"
#include <x86intrin.h>

char* clone_cstring(char* cstr) {
    size_t length = strlen(cstr);
    char* buffer = system_allocate_zeroed_memory(length+1);

    memcpy(buffer, cstr, length);

    return buffer;
}

float random_float(void) {
    return ((float)rand() / (float)(RAND_MAX));
}

int random_ranged_integer(int min, int max) {
    return (rand() % (max - min)) + min;
}

float random_ranged_float(float min, float max) {
    float range = (max - min);
    return (random_float() * range) + min;
}

bool rectangle_intersects_v(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    float x1min = x1;
    float x1max = x1 + w1;

    float y1min = y1;
    float y1max = y1 + h1;

    float x2min = x2;
    float x2max = x2 + w2;

    float y2min = y2;
    float y2max = y2 + h2;

    return (x1min < x2max && x1max > x2min) && (y1min < y2max && y1max > y2min);
}

bool rectangle_overlapping_v(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    float x1min = x1;
    float x1max = x1 + w1;

    float y1min = y1;
    float y1max = y1 + h1;

    float x2min = x2;
    float x2max = x2 + w2;

    float y2min = y2;
    float y2max = y2 + h2;

    return (x1min <= x2max && x1max >= x2min) && (y1min <= y2max && y1max >= y2min);
}

enum intersection_edge opposite_edge_of(enum intersection_edge edge) {
    switch (edge) {
        case INTERSECTION_EDGE_TOP:    return INTERSECTION_EDGE_BOTTOM;
        case INTERSECTION_EDGE_BOTTOM: return INTERSECTION_EDGE_TOP;
        case INTERSECTION_EDGE_LEFT:   return INTERSECTION_EDGE_LEFT;
        case INTERSECTION_EDGE_RIGHT:  return INTERSECTION_EDGE_RIGHT;
    }

    return INTERSECTION_EDGE_NONE;
}

enum intersection_edge rectangle_closest_intersection_edge_v(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    if (!rectangle_intersects_v(x1,y1,w1,h1, x2,y2,w2,h2)) {
        return INTERSECTION_EDGE_NONE;
    }

    float intersection_left_edge   = fabs(x2 - (x1 + w1));
    float intersection_right_edge  = fabs(x1 - (x2 + w2));
    float intersection_top_edge    = fabs(y2 - (y1 + h1));
    float intersection_bottom_edge = fabs(y1 - (y2 + h2));

    float                  minimum_intersection = INFINITY;
    enum intersection_edge best_edge            = INTERSECTION_EDGE_NONE;

    if (intersection_left_edge && intersection_left_edge < minimum_intersection) {
        minimum_intersection = intersection_left_edge;
        best_edge = INTERSECTION_EDGE_LEFT;
    }

    if (intersection_right_edge && intersection_right_edge < minimum_intersection) {
        minimum_intersection = intersection_right_edge;
        best_edge = INTERSECTION_EDGE_RIGHT;
    }

    if (intersection_top_edge && intersection_top_edge < minimum_intersection) {
        minimum_intersection = intersection_top_edge;
        best_edge = INTERSECTION_EDGE_TOP;
    }

    if (intersection_bottom_edge && intersection_bottom_edge < minimum_intersection) {
        minimum_intersection = intersection_bottom_edge;
        best_edge = INTERSECTION_EDGE_BOTTOM;
    }

    return best_edge;
}

bool rectangle_intersects(struct rectangle a, struct rectangle b) {
    return rectangle_intersects_v(a.x, a.y, a.w, a.h, b.x, b.y, b.w, b.h);
}

bool rectangle_overlapping(struct rectangle a, struct rectangle b) {
    return rectangle_overlapping_v(a.x, a.y, a.w, a.h, b.x, b.y, b.w, b.h);
}

enum intersection_edge rectangle_closest_intersection_edge(struct rectangle testing, struct rectangle against) {
    return rectangle_closest_intersection_edge_v(testing.x, testing.y, testing.w, testing.h, against.x, against.y, against.w, against.h);
}

/* starting from receives next line index.
   only unix line endings. fuck windows
 */
char* get_line_starting_from(const char* text, int* starting_from) {
    local char temporary_line_buffer[TEMPORARY_STORAGE_BUFFER_SIZE];
    zero_array(temporary_line_buffer);

    size_t string_length = strlen(text);
    if (*starting_from >= string_length) {
        return NULL;
    }

    int index;
    for (index = *starting_from; index < string_length; ++index) {
        if (text[index] == '\n') {
            break;
        }
    }

    memcpy(temporary_line_buffer, text + *starting_from,
           index - *starting_from);
    *starting_from = (index+1);

    return temporary_line_buffer;
}

size_t copy_until_next_line(const char* text, size_t starting_from, char* into, size_t into_buffer_size) {
    size_t written = 0;
    size_t text_size = strlen(text);

    while (true) {
        char current_character = text[starting_from++];

        if (text_size == starting_from) return ++written; /* finish off the buffer. */
        if (current_character == '\r' || current_character == '\n') return ++written; /* skip the newline */

        if (written < into_buffer_size) {
            into[written++] = current_character;
        }
    }

    return written;
}

char* format_temp(const char* fmt, ...) {
    local int current_buffer = 0;
    local char temporary_text_buffer[TEMPORARY_STORAGE_BUFFER_COUNT][TEMPORARY_STORAGE_BUFFER_SIZE] = {};

    char* target_buffer = temporary_text_buffer[current_buffer++];
    zero_buffer_memory(target_buffer, TEMPORARY_STORAGE_BUFFER_SIZE+1);
    {
        va_list args;
        va_start(args, fmt);
        int written = vsnprintf(target_buffer, TEMPORARY_STORAGE_BUFFER_SIZE-1, fmt, args);
        va_end(args);
    }

    if (current_buffer >= TEMPORARY_STORAGE_BUFFER_COUNT) {
        current_buffer = 0;
    }

    return target_buffer;
}


size_t file_length(const char* path) {
    size_t result = 0;
    FILE* f = fopen(path, "rb+");

    if (f) {
        fseek(f, 0, SEEK_END);
        result = ftell(f);
        fclose(f);
    }

    return result;
}

void read_file_into_buffer(const char* path, char* dest, size_t length) {
    FILE* f = fopen(path, "rb+");

    if (f) {
        fread(dest, 1, length, f);
        fclose(f);
    }
}

char* load_entire_file(const char* path) {
    size_t file_size = file_length(path);
    char* new_buffer = system_allocate_zeroed_memory(file_size+1);

    read_file_into_buffer(path, new_buffer, file_size);

    return new_buffer;
}

int count_lines_of_cstring(const char* string) {
    size_t length = strlen(string);
    int count = 0;

    for (size_t index = 0; index < length; ++index) {
        if (string[index] == '\n') {
            count++;
        }
    }

    return count;
}

void* system_allocate_memory(size_t amount) {
    return malloc(amount);
}

void system_deallocate_memory(void* ptr) {
    free(ptr);
}

void* system_clone_buffer(const void* buffer, size_t buffer_size) {
    void* clone = system_allocate_memory(buffer_size);
    memcpy(clone, buffer, buffer_size);
    return clone;
}

void* system_allocate_zeroed_memory(size_t amount) {
    void* ptr = system_allocate_memory(amount);
    zero_buffer_memory(ptr, amount);
    return ptr;
}

uint64_t rdtsc(void) {
    return __rdtsc();
}
