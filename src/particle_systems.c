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

/*
  I can circumvent this limited amount by making the particle storage sparse, and just making
  all particles store a reference to the particle emitter they came from (which can now be safely stored
  as a variably sized array inside of a level technically, since now only the engine owns the particle storage but not necessarily
  the particle emitters), and that can work?
  
  That's more complicated although it's more robust (since now particles can theoretically just use as many particles as they want (upto the
  still fixed particle limit, which is dependent on how much memory I chomp up still.))
*/
#define MAX_PARTICLES_PER_EMITTER (16384) /* 32x32 filled(all non transparent) sprite. Which is incredibly unlikely since who the fuck just makes a 32x32 white square. Also I can't draw 32x32 tilesets :) */

/*
  May bump this number up, although I can reasonably assume this number shouldn't be reached often...
  
  15 enemies per level, say they each use like 2 or 3 particle systems for some reason, that's like 45 particle emitters
  the player has a special effect so that's another emitter, (46).
  
  That leaves like a good 50/60 for level design placement, because some particle systems can spawn spontaneously (like getting hit
  or landing really hard on the ground, but those particle emitters tend to die incredibly fast...)
 */
#define MAX_PARTICLE_EMITTER_COUNT (16) /*Most levels will probably never reach this number?*/

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
    texture_id texture;

    float last_x;
    float last_y;

    float lifetime;
    float lifetime_max;

    /*TEMPORARY*/
    bool colliding_with_world;

    union color4u8 color;
};

struct particle_emitter {
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

        if (!emitter->alive && emitter->count == 0) {
            zero_buffer_memory(emitter, sizeof(*emitter));
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
                if (emitter->count >= MAX_PARTICLES_PER_EMITTER)
                    return;

                struct particle* emitted_particle = &emitter->particles[emitter->count++];
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
    for (int emitted = 0; emitted < emitter->emission_count && emitter->count < MAX_PARTICLES_PER_EMITTER; ++emitted) {
        struct particle* emitted_particle = &emitter->particles[emitter->count++];
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

    for (int index = emitter->count-1; index >= 0; --index) {
        struct particle* particle = emitter->particles + index;

        particle->vx += particle->ax * dt;
        particle->vy += particle->ay * dt;
        particle->vy += GRAVITY_CONSTANT * dt;

        if (particle->colliding_with_world) {
            do_moving_entity_horizontal_collision_response(world, particle, dt);
            do_moving_entity_vertical_collision_response(world, particle, dt);
            /* particle->x += particle->vx * dt; */
            /* particle->y += particle->vy * dt; */
        } else {
            particle->x += particle->vx * dt;
            particle->y += particle->vy * dt;
        }

        particle->lifetime -= dt;

        if (particle->lifetime <= 0.0) {
            emitter->particles[index] = emitter->particles[--emitter->count];
        }
    }

    if (emitter->emissions > emitter->max_emissions) {
        if (emitter->count == 0) {
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
