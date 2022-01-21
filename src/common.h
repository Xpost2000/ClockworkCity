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
#define safe_assignment(x) if (x) *x 
#define random_element(fixed_size_array) fixed_size_array[random_ranged_integer(0, array_count(fixed_size_array))]
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

#endif
