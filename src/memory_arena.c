#include "common.h"
#include "memory_arena.h"

void* memory_arena_push(struct memory_arena* arena, size_t amount) {
    assert(arena->used < arena->capacity && "Out of arena memory (does not grow!)");
    void* result = arena->memory + arena->used;
    arena->used += amount;
    return result;
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

void memory_arena_deallocate(struct memory_arena* arena) {
    arena->used = 0;
    arena->capacity = 0;
    system_deallocate_memory(arena->memory);
    arena->memory = NULL;
}

struct temporary_arena begin_temporary_memory(struct memory_arena* arena, size_t amount) {
    size_t marker = arena->used;
    void* base_address = memory_arena_push(arena, amount);

    struct temporary_arena temporary = (struct temporary_arena) {
        .used = 0,
        .capacity = amount,
        .memory = base_address,

        .owner = arena,
        .restoration_marker = marker,
    };

    return temporary;
}

void end_temporary_memory(struct temporary_arena* temp) {
    temp->used     = 0;
    temp->capacity = 0;
    temp->memory   = NULL;
    temp->owner->used = temp->restoration_marker;
}
