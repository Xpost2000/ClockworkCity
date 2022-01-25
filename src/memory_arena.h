#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H

/*
  double ended memory arena
  
  I find these are the most versatile... So yeah
*/
struct memory_arena {
    size_t used;
    size_t top_used;
    size_t capacity;
    void* memory;
};

/*This is intentionally structured like this so you can use the memory_arena functions. Ignore type problems*/
struct temporary_arena { 
    size_t used;
    size_t top_used;
    size_t capacity;
    void*  memory;

    struct memory_arena* owner;
    size_t bottom_marker;
    size_t top_marker;
};

/*assume bottom always*/
void* memory_arena_push(struct memory_arena* arena, size_t amount);
void* memory_arena_push_bottom(struct memory_arena* arena, size_t amount);
void* memory_arena_push_top(struct memory_arena* arena, size_t amount);

size_t memory_arena_total_usage(struct memory_arena* arena);

struct memory_arena allocate_memory_arena(size_t sz);

struct temporary_arena begin_temporary_memory(struct memory_arena* arena, size_t amount);
void end_temporary_memory(struct temporary_arena* temp);

void memory_arena_clear(struct memory_arena* arena);
void memory_arena_deallocate(struct memory_arena* arena);

#endif
