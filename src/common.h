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

#define local static
#define shared_storage static

#define TEMPORARY_STORAGE_BUFFER_SIZE (4096)
#define TEMPORARY_STORAGE_BUFFER_COUNT (4)


#define array_count(x) (x / sizeof(*x))
#define zero_buffer_memory(x, l) memset(x, 0, l)
#define zero_array(x) zero_buffer_memory(x, array_count(x))

local char* format_temp(char* fmt, ...) {
    shared_storage int current_buffer = 0;
    shared_storage char temporary_text_buffer[TEMPORARY_STORAGE_BUFFER_COUNT][TEMPORARY_STORAGE_BUFFER_SIZE] = {};
    char* target_buffer = temporary_text_buffer[current_buffer++];
    zero_buffer_memory(target_buffer, TEMPORARY_STORAGE_BUFFER_SIZE);

    va_list args;

    va_start(args, fmt);
    snprintf(target_buffer, TEMPORARY_STORAGE_BUFFER_SIZE, fmt, args);
    va_end(args);

    if (current_buffer >= TEMPORARY_STORAGE_BUFFER_COUNT) {
        current_buffer = 0;
    }

    return target_buffer;
}

#endif
