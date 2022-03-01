/*
  I want particles to be defined in terms of systems which are mostly code
  driven and not necessarily data driven.
  
  This style is probably easier for a programmer to work with than doing lots of data stuff imo.
  
  TODO(jerry): work in progress
  
  TODO(jerry):
  Specify particle direction,
  and particle velocity & acceleration.
  
  NOTE(jerry):
  Since particle emitters "own" their particles, I don't variably size them like I do for
  the tilemap or anything else. This is game "permenant state". There is a slated maximum for particle
  emitters because emitters end up using lots of memory.
  
  There will be "emitter entities", which will be limited to the amount of particle emitters in the engine?
  
  NOTE(jerry):
  I believe I want the UI to have particles as well, but this clearly wasn't designed for that.
  Fuck.
  
  (Although to be fair, this is a much more "general" game situation),
  (if I want the UI to have particles, I could just code much simpler particles since those
  one's aren't really that important and have a very likely known amount.)
  
  Worry about it later.
*/
#define MAX_PARTICLE_EMITTER_COUNT (512) /* With the new particle storage system. GO NUTS!*/

struct particle {
    KINEMATIC_ENTITY_BASE_BODY();
    float lifetime;
    float lifetime_max;

    union color4u8 color;
};

/*
  sparse storage, doubly linked list
*/
#define PARTICLE_CHUNK_SIZE (512)
struct particle_chunk {
    uint16_t               used;
    /* I made a typo earlier and that cost me 30 minutes lol, particle* instead of particle LOL*/
    struct particle        storage[PARTICLE_CHUNK_SIZE];
    struct particle_chunk* previous;
    struct particle_chunk* next;
};
struct particle_chunk particle_chunk_list_sentinel = {};

struct particle_chunk_list {
    struct particle_chunk* head;
    struct particle_chunk* tail;
};

void particle_chunk_list_push(struct particle_chunk_list* list, struct particle_chunk* chunk) {
    if (list->head == &particle_chunk_list_sentinel) {
        list->head  = list->tail = chunk;
        chunk->next = chunk->previous = &particle_chunk_list_sentinel;
    } else {
        struct particle_chunk* old_tail = list->tail;
        list->tail                      = chunk;
        chunk->previous                 = old_tail;
        old_tail->next                  = chunk;
        chunk->next                     = &particle_chunk_list_sentinel;
    }
}

struct particle_chunk* particle_chunk_list_pop_front(struct particle_chunk_list* list) {
    struct particle_chunk* result = &particle_chunk_list_sentinel;

    if (list->head != &particle_chunk_list_sentinel) {
        struct particle_chunk* head_next = list->head->next;
        result = list->head;

        list->head = head_next;
        head_next->previous = &particle_chunk_list_sentinel;
    }

    return result;
}

void particle_chunk_list_remove(struct particle_chunk_list* list, struct particle_chunk* chunk) {
    struct particle_chunk* next     = chunk->next;
    struct particle_chunk* previous = chunk->previous;

    previous->next = next;
    next->previous = previous;

    if (chunk == list->head) {
        list->head = next;
    } else if (chunk == list->tail) {
        list->tail = previous; 
    }
}

int particle_chunk_list_length(struct particle_chunk_list* list) {
    struct particle_chunk* chunk = list->head;
    int c = 0;
    while (chunk != &particle_chunk_list_sentinel) {
        struct particle_chunk* next     = chunk->next;
        c++;
        chunk = next;
    }
    return c;
}

struct particle_emitter {
    struct memory_arena* arena;
    bool alive;

    float x;
    float y;

    /*should turn these into flags?*/
    bool runs_out_of_time;
    float lifetime;
    uint16_t emissions;
    uint16_t max_emissions; /* if 0, means infinite */

    /* uint8_t emission_shape_type; */
    /* line */
    float x1;
    float y1;
    /* end of emission shapes? */

    /* I want to move properties like this into a "particle system definition" */
    union color4f particle_color;
    float particle_max_lifetime;
    /*TEMPORARY, should be under flags*/
    bool collides_with_world;

    /* second per emission */
    float emission_rate;
    float emission_timer;

    float vx_min;
    float vx_max;

    float vy_min;
    float vy_max;

    float lifetime_min;
    float lifetime_max;

    float min_w;
    float max_w;

    float min_h;
    float max_h;

    bool is_square;

    union color4f color_variance; /* probably not using? */

    /* use texture as emission source, overrides everything else, since I can't see any other way to use this for now */
    texture_id from_texture;
    texture_id particle_texture;

    uint16_t emission_count;
    /* add more randomness entropy things here. */

    struct particle_chunk_list chunks;
};

local size_t particle_emitter_count = 0;
local struct particle_emitter* particle_emitter_pool = NULL;
local struct particle_chunk_list particle_chunk_freelist = {};

void initialize_particle_emitter_pool(struct memory_arena* arena) {
    particle_emitter_pool  = memory_arena_push(arena, sizeof(*particle_emitter_pool) * MAX_PARTICLE_EMITTER_COUNT);
    zero_buffer_memory(particle_emitter_pool, sizeof(*particle_emitter_pool) * MAX_PARTICLE_EMITTER_COUNT);
    particle_emitter_count = MAX_PARTICLE_EMITTER_COUNT;

    for (size_t index = 0; index < MAX_PARTICLE_EMITTER_COUNT; ++index) {
        struct particle_emitter* emitter = particle_emitter_pool + index;
        emitter->chunks.head = emitter->chunks.tail = &particle_chunk_list_sentinel;
    }

    particle_chunk_freelist.head = particle_chunk_freelist.tail = &particle_chunk_list_sentinel;
}

size_t particle_emitter_active_particles(struct particle_emitter* emitter) {
    struct particle_chunk_list* list = &emitter->chunks;
    struct particle_chunk* chunk = list->head;
    size_t count = 0;

    while (chunk != &particle_chunk_list_sentinel) {
        count += chunk->used;
        chunk = chunk->next;
    }

    return count;
}

struct particle* particle_emitter_allocate_particle(struct particle_emitter* emitter) {
    struct particle_chunk* free_chunk = &particle_chunk_list_sentinel;
    /* check if we have any good chunks. */
    {
        free_chunk = emitter->chunks.head;
        while (free_chunk != &particle_chunk_list_sentinel) {
            if (free_chunk->used == PARTICLE_CHUNK_SIZE) {
                free_chunk = free_chunk->next;
            } else {
                break;
            }
        }
    }

    /* check if we have something in the particle_chunk_freelist */
    if (free_chunk == &particle_chunk_list_sentinel) {
        free_chunk = particle_chunk_list_pop_front(&particle_chunk_freelist);

        if (free_chunk == &particle_chunk_list_sentinel) {
            /* allocate a new chunk if nothing remains. */
            free_chunk = memory_arena_push(emitter->arena, sizeof(*free_chunk));
            zero_buffer_memory(free_chunk, sizeof(*free_chunk));
            free_chunk->next = free_chunk->previous = &particle_chunk_list_sentinel;
        }

        /* push onto emitter list */
        particle_chunk_list_push(&emitter->chunks, free_chunk);
    }

    assert(free_chunk != &particle_chunk_list_sentinel && "We ran out of memory?");
    struct particle* new_particle = &free_chunk->storage[free_chunk->used++];
    return new_particle;
}

struct particle_emitter* particle_emitter_allocate(void) {
    assert(particle_emitter_pool);

    for (unsigned index = 0; index < particle_emitter_count; ++index) {
        struct particle_emitter* emitter = particle_emitter_pool + index;

        if (!emitter->alive && particle_emitter_active_particles(emitter) == 0) {
            zero_buffer_memory(emitter, sizeof(*emitter));
            emitter->chunks.head = emitter->chunks.tail = &particle_chunk_list_sentinel;
            emitter->arena = &game_memory_arena; /* hack for now */
            emitter->alive = true;

            /* these are the default settings when all settings were hard coded :) */
            emitter->color_variance = COLOR4F_BLACK;
            emitter->min_h = 0.1;
            emitter->min_w = 0.1;
            emitter->max_h = 0.2;
            emitter->min_w = 0.2;

            emitter->vx_min = -3;
            emitter->vx_max = 2;
            emitter->vy_min = -1;
            emitter->vy_max = -6;
            emitter->is_square = true;

            /* LOL */
            /* TODO(jerry): reorganize later. */
            emitter->lifetime_min = emitter->particle_max_lifetime;
            emitter->lifetime_max = emitter->particle_max_lifetime + 0.4;

            return emitter;
        }
    }

    return NULL;
}

local void draw_particle_emitter_particles(struct particle_emitter* emitter, float interpolation) {
    struct particle_chunk_list* list = &emitter->chunks;

    if (emitter->particle_texture.id) {
        for (struct particle_chunk* chunk = list->head; chunk != &particle_chunk_list_sentinel; chunk = chunk->next) {
            for (unsigned index = 0; index < chunk->used; ++index) {
                struct particle* particle = chunk->storage + index;
                draw_texture(emitter->particle_texture,
                             entity_lerp_x(particle, interpolation),
                             entity_lerp_y(particle, interpolation),
                             particle->w, particle->h,
                             color4f((float)particle->color.r / 256.0f,
                                     (float)particle->color.g / 256.0f,
                                     (float)particle->color.b / 256.0f,
                                     particle->lifetime / particle->lifetime_max));
            }
        }
    } else {
        for (struct particle_chunk* chunk = list->head; chunk != &particle_chunk_list_sentinel; chunk = chunk->next) {
            for (unsigned index = 0; index < chunk->used; ++index) {
                struct particle* particle = chunk->storage + index;
                draw_filled_rectangle(entity_lerp_x(particle, interpolation),
                                      entity_lerp_y(particle, interpolation),
                                      particle->w, particle->h,
                                      color4f((float)particle->color.r / 256.0f,
                                              (float)particle->color.g / 256.0f,
                                              (float)particle->color.b / 256.0f,
                                              particle->lifetime / particle->lifetime_max));
            }
        }
    }
}

/* TODO(jerry): more parameterization */

local void emit_particles_from_image_source(struct particle_emitter* emitter) {
    /* image source particles will ignore most parameters except for particle kinematics. */
    /* I'm only just going to make sure we don't run out of particles though. */
    /* again assume 32 bit rgba image */
    {
        struct image_buffer texture_buffer = get_texture_buffer(emitter->from_texture);
        float pixel_scale_factor = 1.0f/VPIXELS_PER_METER;

        uint8_t* image_buffer = texture_buffer.pixels;
        uint32_t image_width  = texture_buffer.width;
        uint32_t image_height = texture_buffer.height;

        for (unsigned y = 0; y < image_height; ++y) {
            for (unsigned x = 0; x < image_width; ++x) {
                uint8_t r = image_buffer[y * (image_width * 4) + (x * 4) + 0] * emitter->particle_color.r;
                uint8_t g = image_buffer[y * (image_width * 4) + (x * 4) + 1] * emitter->particle_color.g;
                uint8_t b = image_buffer[y * (image_width * 4) + (x * 4) + 2] * emitter->particle_color.b;
                uint8_t a = image_buffer[y * (image_width * 4) + (x * 4) + 3] * emitter->particle_color.a;

                if (a != 0) {
                    struct particle* emitted_particle = particle_emitter_allocate_particle(emitter);
                    {
                        emitted_particle->x = emitter->x + (float)x * pixel_scale_factor;
                        emitted_particle->y = emitter->y + (float)y * pixel_scale_factor;
                    }
            
                    emitted_particle->color = color4u8(r, g, b, a);
                    emitted_particle->h = pixel_scale_factor;
                    emitted_particle->w = pixel_scale_factor;

                    emitted_particle->vx = lerp(emitter->vx_min, emitter->vx_max, random_float());
                    emitted_particle->vy = lerp(emitter->vy_min, emitter->vy_max, random_float());

                    emitted_particle->lifetime_max = emitted_particle->lifetime = emitter->particle_max_lifetime + random_float() * 0.4;
                }
            }
        }
    }
}

local void emit_particles(struct particle_emitter* emitter) {
    for (int emitted = 0; emitted < emitter->emission_count; ++emitted) {
        struct particle* emitted_particle = particle_emitter_allocate_particle(emitter);
        /* lots of randomness :D */
        {
            emitted_particle->x = lerp(emitter->x, emitter->x1, random_float());
            emitted_particle->y = lerp(emitter->y, emitter->y1, random_float());
        }
            
        /* TODO(jerry)  color variance*/
        emitted_particle->color = color4u8_from_color4f(emitter->particle_color);
            
        if (emitter->is_square) {
            emitted_particle->h = emitted_particle->w = lerp(emitter->min_h, emitter->max_h, random_float());
        } else {
            emitted_particle->h = lerp(emitter->min_h, emitter->max_h, random_float());
            emitted_particle->w = lerp(emitter->min_w, emitter->max_w, random_float());
        }

        emitted_particle->vx = lerp(emitter->vx_min, emitter->vx_max, random_float());
        emitted_particle->vy = lerp(emitter->vy_min, emitter->vy_max, random_float());
        /* emitted_particle->lifetime_max = emitted_particle->lifetime = lerp(emitter->lifetime_min, emitter->lifetime_max, random_float()); */
        emitted_particle->lifetime_max = emitted_particle->lifetime = emitter->particle_max_lifetime + random_float() * 0.4;
    }
}

local void particle_emitter_clear_all_particles(struct particle_emitter* emitter) {
    struct particle_chunk_list* list = &emitter->chunks;

    for (struct particle_chunk* chunk = list->head; chunk != &particle_chunk_list_sentinel; chunk = chunk->next) {
        chunk->used = 0;
        particle_chunk_list_remove(list, chunk);
        particle_chunk_list_push(&particle_chunk_freelist, chunk);
    }
}

local void update_particle_emitter(struct particle_emitter* emitter, struct tilemap* world, float dt) {
    if (emitter->alive && emitter->emission_timer <= 0.0f && (emitter->max_emissions == 0 || emitter->emissions < emitter->max_emissions)) {
        if (emitter->from_texture.id) {
            emit_particles_from_image_source(emitter);
        } else {
            emit_particles(emitter);
        }

        emitter->emission_timer = emitter->emission_rate;
        if (emitter->max_emissions != 0) emitter->emissions++;
    } else {
        emitter->emission_timer -= dt;
    }

    struct particle_chunk_list* list = &emitter->chunks;

    if (emitter->collides_with_world) {
        for (struct particle_chunk* chunk = list->head; chunk != &particle_chunk_list_sentinel; chunk = chunk->next) {
            for (int index = chunk->used-1; index >= 0; --index) {
                struct particle* particle = chunk->storage + index;

                particle->vx += particle->ax * dt;
                particle->vy += particle->ay * dt;
                particle->vy += GRAVITY_CONSTANT * dt;

                do_particle_horizontal_collision_response(world, (struct entity*) particle, dt);
                do_particle_vertical_collision_response(world, (struct entity*) particle, dt);

                particle->lifetime -= dt;

                if (particle->lifetime <= 0.0) {
                    chunk->storage[index] = chunk->storage[--chunk->used];
                }
            }
        }
    } else {
        for (struct particle_chunk* chunk = list->head; chunk != &particle_chunk_list_sentinel; chunk = chunk->next) {
            for (int index = chunk->used-1; index >= 0; --index) {
                struct particle* particle = chunk->storage + index;

                particle->vx += particle->ax * dt;
                particle->vy += particle->ay * dt;
                particle->vy += GRAVITY_CONSTANT * dt;

                particle->last_x = particle->x;
                particle->x += particle->vx * dt;
                particle->last_y = particle->y;
                particle->y += particle->vy * dt;

                particle->lifetime -= dt;

                if (particle->lifetime <= 0.0) {
                    chunk->storage[index] = chunk->storage[--chunk->used];
                }
            }
        }
    }

    /* "garbage collection" */
    for (struct particle_chunk* chunk = list->head; chunk != &particle_chunk_list_sentinel; chunk = chunk->next) {
        if (chunk->used == 0) {
            /* This order does matter because push modifies the chunk */
            particle_chunk_list_remove(list, chunk);
            particle_chunk_list_push(&particle_chunk_freelist, chunk);
        }
    }

    if (emitter->emissions > emitter->max_emissions) {
        if (particle_emitter_active_particles(emitter) == 0) {
            emitter->alive = false;
        } 
    } else {
        if (emitter->runs_out_of_time) {
            if (emitter->lifetime <= 0) {
                emitter->alive = false;
            }

            emitter->lifetime -= dt;
        }
    }
}

local texture_id particle_textures[3]       = {};
local const char* particle_texture_paths[3] = {
    "assets/testtiles/particle4.png",
    "assets/testtiles/particle8.png",
    "assets/testtiles/particle16.png",
};
void load_all_particle_textures(void) {
    for (unsigned index = 0; index < array_count(particle_textures); ++index) {
        particle_textures[index] = load_texture(particle_texture_paths[index]);
    }
}

/*Allow per entity collision? Pixel collision detection *crying**/
void update_all_particle_systems(struct tilemap* world, float dt) {
    for (unsigned index = 0; index < particle_emitter_count; ++index) {
        struct particle_emitter* emitter = particle_emitter_pool + index;

        if (emitter->alive) {
            update_particle_emitter(emitter, world, dt);
        }
    }
}

void draw_all_particle_systems(float interpolation) {
    for (unsigned index = 0; index < particle_emitter_count; ++index) {
        struct particle_emitter* emitter = particle_emitter_pool + index;

        if (emitter->alive) {
            draw_particle_emitter_particles(emitter, interpolation);
        }
    }
}
