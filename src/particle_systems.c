/*
  I want particles to be defined in terms of systems which are mostly code
  driven and not necessarily data driven.
  
  This style is probably easier for a programmer to work with than doing lots of data stuff imo.
  
  TODO(jerry): work in progress
*/
#define MAX_PARTICLES_PER_EMITTER (2048)
#define MAX_PARTICLE_EMITTER_COUNT (256)

struct particle {
    /* shared with struct entity intentionally to allow pointer cast reuse */
    float x;
    float y;

    float w;
    float h;

    float vx;
    float vy;

    float ax;
    float ay;

    float last_vy;
    bool onground;

    /* particle specific */
    float last_x;
    float last_y;

    float lifetime;
    float lifetime_max;
};

struct particle_emitter {
    bool alive;

    float x;
    float y;

    /* second per emission */
    float emission_rate;
    float emission_timer;

    uint16_t emission_count;
    /* add more randomness entropy things here. */

    uint16_t count;
    struct particle particles[MAX_PARTICLES_PER_EMITTER];
};

local size_t particle_emitter_count = 0;
local struct particle_emitter* particle_emitter_pool = NULL;

void initialize_particle_emitter_pool(struct memory_arena* arena, size_t count) {
    particle_emitter_pool  = memory_arena_push(arena, sizeof(*particle_emitter_pool) * count);
    zero_buffer_memory(particle_emitter_pool, sizeof(*particle_emitter_pool) * count);
    particle_emitter_count = count;
}

struct particle_emitter* particle_emitter_allocate(void) {
    assert(particle_emitter_pool);

    for (unsigned index = 0; index < particle_emitter_count; ++index) {
        struct particle_emitter* emitter = particle_emitter_pool + index;
        if (!emitter->alive) {
            emitter->alive = true;
            return emitter;
        }
    }

    return NULL;
}

local void draw_particle_emitter_particles(struct particle_emitter* emitter) {
    for (unsigned index = 0; index < emitter->count; ++index) {
        struct particle* particle = emitter->particles + index;
        /* just draw a square I suppose */
        draw_filled_rectangle(particle->x, particle->y, particle->w, particle->h,
                              color4f(1.0, 0.2, 0.3, particle->lifetime / particle->lifetime_max));
    }
}

local void update_particle_emitter(struct particle_emitter* emitter, float dt) {
    if (emitter->emission_timer <= 0.0f) {
        /* emit some particles */
        for (int emitted = 0; emitted < emitter->emission_count && emitter->count < MAX_PARTICLES_PER_EMITTER; ++emitted) {
            struct particle* emitted_particle = &emitter->particles[emitter->count++];
            /* lots of randomness :D */
            emitted_particle->x = emitter->x + random_ranged_float(-1, 1);
            emitted_particle->y = emitter->y + random_ranged_float(-1, 1);
            emitted_particle->h = emitted_particle->w = 0.1 + random_float() * 0.1;
            emitted_particle->vx = (random_float() * 5) - 3;
            emitted_particle->vy = -1 - (random_float() * 5);
            emitted_particle->lifetime_max = emitted_particle->lifetime = 0.5 + random_float() * 0.4;
        }

        emitter->emission_timer = emitter->emission_rate;
    } else {
        emitter->emission_timer -= dt;
    }

    for (int index = emitter->count-1; index >= 0; --index) {
        struct particle* particle = emitter->particles + index;

        particle->vx += particle->ax * dt;
        particle->vy += particle->ay * dt;
        particle->vy += 20 * dt;

        particle->x += particle->vx * dt;
        particle->y += particle->vy * dt;

        particle->lifetime -= dt;

        if (particle->lifetime <= 0.0) {
            emitter->particles[index] = emitter->particles[--emitter->count];
        }
    }
}

void update_all_particle_systems(float dt) {
    for (unsigned index = 0; index < particle_emitter_count; ++index) {
        struct particle_emitter* emitter = particle_emitter_pool + index;

        if (emitter->alive) {
            update_particle_emitter(emitter, dt);
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
