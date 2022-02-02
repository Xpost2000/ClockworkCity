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

bool rectangle_intersects(struct rectangle a, struct rectangle b) {
    return rectangle_intersects_v(a.x, a.y, a.w, a.h, b.x, b.y, b.w, b.h);
}

bool rectangle_overlapping(struct rectangle a, struct rectangle b) {
    return rectangle_overlapping_v(a.x, a.y, a.w, a.h, b.x, b.y, b.w, b.h);
}

/* starting from receives next line index.
   only unix line endings. fuck windows
 */
char* get_line_starting_from(char* text, int* starting_from) {
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

char* format_temp(char* fmt, ...) {
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


size_t file_length(char* path) {
    size_t result = 0;
    FILE* f = fopen(path, "rb+");

    if (f) {
        fseek(f, 0, SEEK_END);
        result = ftell(f);
        fclose(f);
    }

    return result;
}

void read_file_into_buffer(char* path, char* dest, size_t length) {
    FILE* f = fopen(path, "rb+");

    if (f) {
        fread(dest, 1, length, f);
        fclose(f);
    }
}

char* load_entire_file(char* path) {
    size_t file_size = file_length(path);
    char* new_buffer = system_allocate_zeroed_memory(file_size+1);

    read_file_into_buffer(path, new_buffer, file_size);

    return new_buffer;
}

int count_lines_of_cstring(char* string) {
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

void* system_allocate_zeroed_memory(size_t amount) {
    void* ptr = system_allocate_memory(amount);
    zero_buffer_memory(ptr, amount);
    return ptr;
}

uint64_t rdtsc(void) {
    return __rdtsc();
}
