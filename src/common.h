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

/*beh*/
#define safe_assignment(x) if(x) *x 

/*
  "string" functions working from temporary functions.
  Never expect to keep these, unless you clone
*/
local char* clone_cstring(char* cstr) {
    size_t length = strlen(cstr);
    char* buffer = malloc(length+1);

    buffer[length] = 0;
    memcpy(buffer, cstr, length);

    return buffer;
}

/* starting from receives next line index.
   only unix line endings. fuck windows
 */
local char* get_line_starting_from(char* text, int* starting_from) {
    shared_storage char temporary_line_buffer[TEMPORARY_STORAGE_BUFFER_SIZE];
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

local char* format_temp(char* fmt, ...) {
    shared_storage int current_buffer = 0;
    shared_storage char temporary_text_buffer[TEMPORARY_STORAGE_BUFFER_COUNT][TEMPORARY_STORAGE_BUFFER_SIZE] = {};

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

#endif
