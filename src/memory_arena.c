#include "common.h"
#include "memory_arena.h"

void* memory_arena_push(struct memory_arena* arena, size_t amount) {
    return memory_arena_push_bottom(arena, amount);
}

local void _arena_diagonistic(struct memory_arena* arena) {
    {
        size_t memusage = memory_arena_total_usage(arena);
        console_printf("(memory arena \"%s\") is using %d bytes, (%d kb) (%d mb) (%d gb)\n",
                       arena->name, memusage, memusage / 1024, memusage / (1024 * 1024), memusage / (1024*1024*1024));
    }
}

void* memory_arena_push_top(struct memory_arena* arena, size_t amount) {
    assert(arena->used+arena->top_used <= arena->capacity && "Out of arena memory (does not grow!)");
    arena->top_used += amount;
    void* result = (arena->memory + arena->capacity) - arena->top_used;
    zero_buffer_memory(result, amount);
    return result;
}

void* memory_arena_push_bottom(struct memory_arena* arena, size_t amount) {
    assert(arena->used+arena->top_used <= arena->capacity && "Out of arena memory (does not grow!)");
    void* result = arena->memory + arena->used;
    arena->used += amount;
    zero_buffer_memory(result, amount);
    return result;
}

void* memory_arena_copy_buffer(struct memory_arena* arena, void* buffer, size_t buffer_length) {
    return memory_arena_copy_buffer_bottom(arena, buffer, buffer_length);
}

void* memory_arena_copy_buffer_top(struct memory_arena* arena, void* buffer, size_t buffer_length) {
    void* result = memory_arena_push_top(arena, buffer_length);
    memcpy(result, buffer, buffer_length);
    return result;
}

void* memory_arena_copy_buffer_bottom(struct memory_arena* arena, void* buffer, size_t buffer_length) {
    void* result = memory_arena_push_bottom(arena, buffer_length);
    memcpy(result, buffer, buffer_length);
    return result;
}

size_t memory_arena_total_usage(struct memory_arena* arena) {
    return arena->used + arena->top_used;
}

/*NOTE(jerry): should I allow a growing arena?*/
struct memory_arena allocate_memory_arena(size_t sz) {
    return (struct memory_arena) {
        .used = 0,
        .capacity = sz,
        .memory = system_allocate_zeroed_memory(sz)
    };
}

void memory_arena_clear(struct memory_arena* arena) {
    arena->used = 0;
}

void memory_arena_clear_top(struct memory_arena* arena) {
    arena->top_used = 0;
}

void memory_arena_deallocate(struct memory_arena* arena) {
    arena->used = 0;
    arena->capacity = 0;
    system_deallocate_memory(arena->memory);
    arena->memory = NULL;
}

struct temporary_arena begin_temporary_memory(struct memory_arena* arena, size_t amount) {
    size_t bottom_marker = arena->used;
    size_t top_marker = arena->top_used;
    void* base_address = memory_arena_push(arena, amount);

    struct temporary_arena temporary = (struct temporary_arena) {
        .used = 0,
        .capacity = amount,
        .memory = base_address,

        .owner = arena,
        .bottom_marker = bottom_marker,
        .top_marker = top_marker,
    };

    return temporary;
}

void end_temporary_memory(struct temporary_arena* temp) {
    temp->used            = 0;
    temp->capacity        = 0;
    temp->memory          = NULL;
    temp->owner->used     = temp->bottom_marker;
    temp->owner->top_used = temp->top_marker;
}
