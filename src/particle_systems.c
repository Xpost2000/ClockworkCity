/*
  I want particles to be defined in terms of systems which are mostly code
  driven and not necessarily data driven.
  
  This style is probably easier for a programmer to work with than doing lots of data stuff imo.
  
  TODO(jerry): work in progress
  
  I want to do particles from images, but I don't know what the conversion is for unit to pixel unfortunately. I mean I guess
  it is actually just 
  
  NOTE(jerry):
  Since particle emitters "own" their particles, I don't variably size them like I do for
  the tilemap or anything else. This is game "permenant state". There is a slated maximum for particle
  emitters because emitters end up using lots of memory.
  
  There will be "emitter entities", which will be limited to the amount of particle emitters in the engine?
*/
#define MAX_PARTICLES_PER_EMITTER (16384) /* 32x32 filled(all non transparent) sprite. Which is incredibly unlikely since who the fuck just makes a 32x32 white square. Also I can't draw 32x32 tilesets :) */
#define MAX_PARTICLE_EMITTER_COUNT (16) /*Most levels will probably never reach this number?*/

struct particle {
    KINEMATIC_ENITTY_BASE_BODY();

    /* particle specific */
    texture_id texture;

    float last_x;
    float last_y;

    float lifetime;
    float lifetime_max;

    /*TEMPORARY*/
    bool colliding_with_world;

    union color4u8 color;
};

/*
  sparse storage, doubly linked list
*/
#define PARTICLE_CHUNK_SIZE (1024)
struct particle_chunk {
    uint16_t               used;
    /* I made a typo earlier and that cost me 30 minutes lol, particle* instead of particle LOL*/
    struct particle        storage[PARTICLE_CHUNK_SIZE];
    struct particle_chunk* previous;
    struct particle_chunk* next;
};
struct particle_chunk list_sentinel = {};

struct particle_chunk_list {
    struct particle_chunk* head;
    struct particle_chunk* tail;
};

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

    /* use texture as emission source, overrides everything else, since I can't see any other way to use this for now */
    texture_id from_texture;
    texture_id particle_texture;

    uint16_t emission_count;
    /* add more randomness entropy things here. */

    struct particle_chunk_list chunks;
};

local size_t particle_emitter_count = 0;
local struct particle_emitter* particle_emitter_pool = NULL;
local struct particle_chunk_list freelist = {};

void initialize_particle_emitter_pool(struct memory_arena* arena) {
    particle_emitter_pool  = memory_arena_push(arena, sizeof(*particle_emitter_pool) * MAX_PARTICLE_EMITTER_COUNT);
    zero_buffer_memory(particle_emitter_pool, sizeof(*particle_emitter_pool) * MAX_PARTICLE_EMITTER_COUNT);
    particle_emitter_count = MAX_PARTICLE_EMITTER_COUNT;

    for (size_t index = 0; index < MAX_PARTICLE_EMITTER_COUNT; ++index) {
        struct particle_emitter* emitter = particle_emitter_pool + index;
        emitter->chunks.head = emitter->chunks.tail = &list_sentinel;
    }

    freelist.head = freelist.tail = &list_sentinel;
}

size_t particle_emitter_active_particles(struct particle_emitter* emitter) {
    struct particle_chunk_list* list = &emitter->chunks;
    struct particle_chunk* chunk = list->head;
    size_t count = 0;

    while (chunk != &list_sentinel) {
        count += chunk->used;
        chunk = chunk->next;
    }

    return count;
}

struct particle* particle_emitter_allocate_particle(struct particle_emitter* emitter) {
    struct particle_chunk* free_chunk = &list_sentinel;
    /* check if we have any good chunks. */
    {
        free_chunk = emitter->chunks.head;
        while (free_chunk != &list_sentinel) {
            if (free_chunk->used == PARTICLE_CHUNK_SIZE) {
                free_chunk = free_chunk->next;
            } else {
                break;
            }
        }
    }
    /* check if we have something in the freelist */
    if (free_chunk == &list_sentinel) {

        if (freelist.head != &list_sentinel) {
            struct particle_chunk* head = freelist.head;
            struct particle_chunk* head_next = head->next;

            free_chunk = freelist.head;
            freelist.head = head_next;
        }

        if (free_chunk == &list_sentinel) {
            /* allocate a new chunk if nothing remains. */
            free_chunk = memory_arena_push(emitter->arena, sizeof(*free_chunk));
            zero_buffer_memory(free_chunk, sizeof(*free_chunk));
            free_chunk->next = free_chunk->previous = &list_sentinel;
        }

        /* push onto emitter list */
        if (emitter->chunks.head == &list_sentinel) {
            emitter->chunks.head = emitter->chunks.tail = free_chunk;
            free_chunk->next = free_chunk->previous = &list_sentinel;
        } else {
            struct particle_chunk* old_tail = emitter->chunks.tail;
            emitter->chunks.tail = free_chunk;
            free_chunk->previous = old_tail;
            old_tail->next       = free_chunk;
            free_chunk->next = &list_sentinel;
        }
    }

    assert(free_chunk != &list_sentinel && "We ran out of memory?");
    struct particle* new_particle = &free_chunk->storage[free_chunk->used++];
    return new_particle;
}

struct particle_emitter* particle_emitter_allocate(void) {
    assert(particle_emitter_pool);

    for (unsigned index = 0; index < particle_emitter_count; ++index) {
        struct particle_emitter* emitter = particle_emitter_pool + index;

        if (!emitter->alive && particle_emitter_active_particles(emitter) == 0) {
            zero_buffer_memory(emitter, sizeof(*emitter));
            emitter->chunks.head = emitter->chunks.tail = &list_sentinel;
            emitter->arena = &game_memory_arena; /* hack for now */
            emitter->alive = true;
            return emitter;
        }
    }

    return NULL;
}

local void draw_particle_emitter_particles(struct particle_emitter* emitter) {
    struct particle_chunk_list* list = &emitter->chunks;
    struct particle_chunk* chunk = list->head;

    while (chunk != &list_sentinel) {
        for (unsigned index = 0; index < chunk->used; ++index) {
            struct particle* particle = chunk->storage + index;
            if (particle->texture.id) {
                draw_texture(particle->texture, particle->x, particle->y, particle->w, particle->h,
                             color4f((float)particle->color.r / 256.0f,
                                     (float)particle->color.g / 256.0f,
                                     (float)particle->color.b / 256.0f,
                                     particle->lifetime / particle->lifetime_max
                                     /* 1.0 */
                             ));
            } else {
                draw_filled_rectangle(particle->x, particle->y, particle->w, particle->h,
                                      color4f((float)particle->color.r / 256.0f,
                                              (float)particle->color.g / 256.0f,
                                              (float)particle->color.b / 256.0f,
                                              particle->lifetime / particle->lifetime_max
                                              /* 1.0 */
                                      ));
            }
        }

        chunk = chunk->next;
    }
}

/* TODO(jerry): more parameterization */

local emit_particles_from_image_source(struct particle_emitter* emitter) {
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
                struct particle* emitted_particle = particle_emitter_allocate_particle(emitter);
                emitted_particle->colliding_with_world = emitter->collides_with_world;
                emitted_particle->texture = emitter->particle_texture;
                {
                    emitted_particle->x = emitter->x + (float)x * pixel_scale_factor;
                    emitted_particle->y = emitter->y + (float)y * pixel_scale_factor;
                }
            
                {
                    uint8_t r = image_buffer[y * (image_width * 4) + (x * 4) + 0] * emitter->particle_color.r;
                    uint8_t g = image_buffer[y * (image_width * 4) + (x * 4) + 1] * emitter->particle_color.g;
                    uint8_t b = image_buffer[y * (image_width * 4) + (x * 4) + 2] * emitter->particle_color.b;
                    uint8_t a = image_buffer[y * (image_width * 4) + (x * 4) + 3] * emitter->particle_color.a;

                    emitted_particle->color = color4u8(r, g, b, a);
                }
                emitted_particle->h = pixel_scale_factor;
                emitted_particle->w = pixel_scale_factor;

                emitted_particle->vx = (random_float() * 5) - 3;
                emitted_particle->vy = -1 - (random_float() * 5);

                emitted_particle->lifetime_max = emitted_particle->lifetime = emitter->particle_max_lifetime + random_float() * 0.4;
            }
        }
    }
}

local emit_particles(struct particle_emitter* emitter) {
    for (int emitted = 0; emitted < emitter->emission_count; ++emitted) {
        struct particle* emitted_particle = particle_emitter_allocate_particle(emitter);

        emitted_particle->texture = emitter->particle_texture;
        emitted_particle->colliding_with_world = emitter->collides_with_world;
        /* lots of randomness :D */
        {
            emitted_particle->x = lerp(emitter->x, emitter->x1, random_float());
            emitted_particle->y = lerp(emitter->y, emitter->y1, random_float());
        }
            
        emitted_particle->color = color4u8_from_color4f(emitter->particle_color);
            
        emitted_particle->h = emitted_particle->w = 0.1 + random_float() * 0.1;
        emitted_particle->vx = (random_float() * 5) - 3;
        emitted_particle->vy = -1 - (random_float() * 5);
        emitted_particle->lifetime_max = emitted_particle->lifetime = emitter->particle_max_lifetime + random_float() * 0.4;
    }
}

local void update_particle_emitter(struct particle_emitter* emitter, struct tilemap* world, float dt) {
    if (emitter->alive && emitter->emission_timer <= 0.0f && emitter->emissions <= emitter->max_emissions) {
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
    {
        struct particle_chunk* chunk = list->head;

        while (chunk != &list_sentinel) {
            for (int index = chunk->used-1; index >= 0; --index) {
                struct particle* particle = chunk->storage + index;

                particle->vx += particle->ax * dt;
                particle->vy += particle->ay * dt;
                particle->vy += GRAVITY_CONSTANT * dt;
                /*
                  Threading would help I suppose.
                */

                if (particle->colliding_with_world) {
                    do_moving_entity_horizontal_collision_response(world, particle, dt);
                    do_moving_entity_vertical_collision_response(world, particle, dt);
                } else {
                    particle->x += particle->vx * dt;
                    particle->y += particle->vy * dt;
                }

                particle->lifetime -= dt;

                if (particle->lifetime <= 0.0) {
                    chunk->storage[index] = chunk->storage[--chunk->used];
                }
            }

            chunk = chunk->next;
        }
    }
    /* "garbage collection" */
    {
        struct particle_chunk* chunk = list->head;

        while (chunk != &list_sentinel) {
            /* frelist adding if we can reuse it. */
            struct particle_chunk* next     = chunk->next;
            struct particle_chunk* previous = chunk->previous;
            {
                if (chunk->used == 0) {
                    if (freelist.head == &list_sentinel) {
                        freelist.head = freelist.tail = chunk;
                        chunk->next = chunk->previous = &list_sentinel;
                    } else {
                        struct particle_chunk* old_tail = freelist.tail;
                        freelist.tail = chunk;
                        chunk->previous = old_tail;
                        old_tail->next = chunk;
                        chunk->next = &list_sentinel;
                    }

                    previous->next = next;
                    next->previous = previous;
                }
            }
            chunk = next;
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

/*Allow per entity collision? Pixel collision detection *crying**/
void update_all_particle_systems(struct tilemap* world, float dt) {
    for (unsigned index = 0; index < particle_emitter_count; ++index) {
        struct particle_emitter* emitter = particle_emitter_pool + index;

        if (emitter->alive) {
            update_particle_emitter(emitter, world, dt);
        }
    }
}

void draw_all_particle_systems(void) {
    for (unsigned index = 0; index < particle_emitter_count; ++index) {
        struct particle_emitter* emitter = particle_emitter_pool + index;

        if (emitter->alive) {
            draw_particle_emitter_particles(emitter);
        }
    }
}
