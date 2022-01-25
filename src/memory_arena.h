#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H

struct memory_arena {
    size_t used;
    size_t capacity;
    void* memory;
};

/*This is intentionally structured like this so you can use the memory_arena functions. Ignore type problems*/
struct temporary_arena { 
    size_t used;
    size_t capacity;
    void*  memory;

    struct memory_arena* owner;
    size_t restoration_marker;
};

void* memory_arena_push(struct memory_arena* arena, size_t amount);
struct memory_arena allocate_memory_arena(size_t sz);

struct temporary_arena begin_temporary_memory(struct memory_arena* arena, size_t amount);
void end_temporary_memory(struct temporary_arena* temp);

void memory_arena_clear(struct memory_arena* arena);
void memory_arena_deallocate(struct memory_arena* arena);

#endif
