/* general entity procedures */
/*
  Camera influence is determined by distance to the player.
  
  TODO(jerry): Draw
*/
float entity_lerp_x(struct entity* entity, float t) {
    return lerp(entity->last_x, entity->x, t);
}

float entity_lerp_y(struct entity* entity, float t) {
    return lerp(entity->last_y, entity->y, t);
}

#ifdef DEV
struct lingered_hitbox { /* I don't actually need this for any data tracking, just debug viewing... */
    float x;
    float y;
    float w;
    float h;
    float lifetime;
};
local uint16_t               DEBUG_lingering_hitbox_count              =  0;
local struct lingered_hitbox DEBUG_lingering_hitboxes[HITBOX_POOL_MAX] = {};

local void push_lingering_hitbox(float x, float y, float w, float h) {
    assert(DEBUG_lingering_hitbox_count < HITBOX_POOL_MAX);
    struct lingered_hitbox* new_hitbox = &DEBUG_lingering_hitboxes[DEBUG_lingering_hitbox_count++];
    new_hitbox->x = x; new_hitbox->y = y;
    new_hitbox->w = w; new_hitbox->h = h;
    new_hitbox->lifetime = 0.15;
}

local void DEBUG_update_all_lingering_hitboxes(float dt) {
    for (int index = DEBUG_lingering_hitbox_count-1; index >= 0; --index) {
        struct lingered_hitbox* current_hitbox = DEBUG_lingering_hitboxes + index;
        current_hitbox->lifetime -= dt;

        if (current_hitbox->lifetime <= 0) {
            DEBUG_lingering_hitboxes[index] = DEBUG_lingering_hitboxes[--DEBUG_lingering_hitbox_count];
        }
    }

}

local void DEBUG_draw_all_lingering_hitboxes(void) {
    for (int index = DEBUG_lingering_hitbox_count-1; index >= 0; --index) {
        struct hitbox* current_hitbox = hitbox_pool + index;
        draw_rectangle(current_hitbox->x, current_hitbox->y, current_hitbox->w, current_hitbox->h, COLOR4F_BLUE);
    }
}
#endif

local void hitbox_push_ignored_entity(struct hitbox* hitbox, struct entity* to_ignore) {
    for (unsigned ignored_index = 0; ignored_index < array_count(hitbox->ignored_entities); ++ignored_index) {
        if (hitbox->ignored_entities[ignored_index] == NULL) {
            hitbox->ignored_entities[ignored_index] = to_ignore;
            return;
        }
    }
}

/* hitbox that operates on one frame. */
local void push_melee_hitbox(struct entity* owner,
                             float x, float y, float w, float h, int damage, int direction) {
    assert(hitbox_count < HITBOX_POOL_MAX);
    struct hitbox* new_hitbox = &hitbox_pool[hitbox_count++];
    new_hitbox->owner = owner;
    hitbox_push_ignored_entity(new_hitbox, owner);

    new_hitbox->type = HITBOX_TYPE_HURT;
    new_hitbox->direction = direction;
    new_hitbox->x = x;
    new_hitbox->y = y;
    new_hitbox->w = w;
    new_hitbox->h = h;
    new_hitbox->lifetime = 0;
    new_hitbox->damage = damage;
}

local void push_melee_with_knockback_hitbox(struct entity* owner,
                                            float x, float y, float w, float h, int damage, int direction) {
    assert(hitbox_count < HITBOX_POOL_MAX);
    struct hitbox* new_hitbox = &hitbox_pool[hitbox_count++];
    new_hitbox->owner = owner;
    hitbox_push_ignored_entity(new_hitbox, owner);

    new_hitbox->type = HITBOX_TYPE_HURT_WITH_KNOCKBACK;
    new_hitbox->direction = direction;
    new_hitbox->x = x;
    new_hitbox->y = y;
    new_hitbox->w = w;
    new_hitbox->h = h;
    new_hitbox->lifetime = 0;
    new_hitbox->damage = damage;
}

local void entity_do_ground_impact(struct entity* entity, float camera_influence) {
    if (entity->last_vy >= (15)) {
        float g_force_count = (entity->last_vy / (GRAVITY_CONSTANT));
        float shake_factor = pow(0.02356, 1.10 / (g_force_count)) * camera_influence;

        camera_traumatize(&game_camera, shake_factor);
        {
            struct particle_emitter* splatter = particle_emitter_allocate();
            splatter->x = splatter->x1 = entity->x;
            splatter->y = splatter->y1 = entity->y + entity->h;
            splatter->emission_rate = 0;
            splatter->emission_count = minf(ceilf(164 * shake_factor), 40);
            splatter->max_emissions = 1;
            splatter->particle_color = color4f(0.8, 0.8, 0.8, 1.0);
            splatter->particle_max_lifetime = 1;
        }

        entity->last_vy = 0;
    }
}

void entity_halt_motion(struct entity* entity) {
    entity->last_vy = entity->vy;
    entity->ax = entity->ay = entity->vx = entity->vy = 0;
}

/* end of general entity procedures */

/* type procedures */

/*
  Honestly... There is no reason this should be special.
*/
void do_player_entity_physics_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    if (noclip) {
        entity->vx      = entity->ax;
        entity->vy      = entity->ay;
        entity->last_vy = entity->vy;
    }

    entity->vx += entity->ax * dt;
    bool hugging = entity_hugging_wall(tilemap, entity) && !entity->onground;
    bool will_be_hugging_wall = false;
    {
        float old_x = entity->x;
        entity->x += entity->vx * dt;
        will_be_hugging_wall = entity_hugging_wall(tilemap, entity) && !entity->onground;
        entity->x = old_x;
    }
    entity->sliding_against_wall = hugging;

    const float WALL_SLIDE_VELOCITY = GRAVITY_CONSTANT/7;
    if (fabs(entity->ax) > 0 && hugging) {
        if (entity->vy < 0 && will_be_hugging_wall) {
            entity->vy = 0;  
        } 

        entity->vy += (entity->ay + WALL_SLIDE_VELOCITY) * dt;
        if (entity->vy > 0) {
            if (entity->vy > WALL_SLIDE_VELOCITY) {
                entity->vy = WALL_SLIDE_VELOCITY;
                {
                    /* TODO(jerry): sliding grinding particles */
                }
            }
        }

        entity->current_jump_count = 0;
    } else {
        entity->vy += (entity->ay + GRAVITY_CONSTANT) * dt;
    }

    if (!entity->dash) {
        entity->vx -= (entity->vx * 4 * dt);
    }

    const int MAX_SPEED = 500;
    if (fabs(entity->vx) > MAX_SPEED) {
        float sgn = float_sign(entity->vx);
        entity->vx = MAX_SPEED * sgn;
    }

    if (entity->dash) entity->vy = 0;

    do_moving_entity_horizontal_collision_response(tilemap, entity, dt);
    do_moving_entity_vertical_collision_response(tilemap, entity, dt);
}

void do_lost_soul_physics_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    entity->last_x = entity->x;
    entity->last_y = entity->y;

    switch (entity->type) {
        /* case ENTITY_TYPE_HOVERING_LOST_SOUL: {} break; */
        default: {
            /* TODO(jerry):
             make lost soul types work differently.*/
            entity->y = normalized_sinf((entity->lost_soul_info.fly_time)) * 0.67 + entity->initial_y;
        } break;
    }
    struct particle_emitter* emitter = entity->lost_soul_info.owned_emitter;
    if (entity->health == 0) {
        emitter->alive = 0;
    }
    {
        emitter->x  = entity->x - 0.25;
        emitter->y  = entity->y + 0.5;
        emitter->x1 = entity->x + 0.25;
        emitter->y1 = entity->y + 0.5;
    }

    entity->lost_soul_info.fly_time += dt * 1.5;
    /* entity->x  += entity->vx *                       dt; */
    /* entity->y  += entity->vy *                       dt; */
}

void do_lost_soul_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    /* these guys do respawn. For platforming reasons lol. */
    if (entity->death_state != DEATH_STATE_ALIVE) {
        if (entity->death_state == DEATH_STATE_DYING) {
            entity->death_state = DEATH_STATE_DEAD;

            /* death explosion. woosh */
            {
                struct particle_emitter* splatter = particle_emitter_allocate();
                splatter->x = splatter->x1 = entity->x;
                splatter->y = splatter->y1 = entity->y + entity->h;
                splatter->emission_rate = 0;
                splatter->emission_count = 16;
                splatter->max_emissions = 1;
                splatter->particle_color = active_colorscheme.primary;
                splatter->particle_max_lifetime = 1;
                splatter->collides_with_world = true;
                camera_traumatize(&game_camera, 0.0152);
            }
        }
    }
}

/* 
   technically physics updates for all entities should nearly be identical...
   However for now I only have the player so... I can't say.
*/
void do_generic_entity_physics_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    entity->vx += entity->ax *                      dt;
    entity->vy += (entity->ay + GRAVITY_CONSTANT) * dt;

    do_moving_entity_horizontal_collision_response(tilemap, entity, dt);
    do_moving_entity_vertical_collision_response(tilemap, entity, dt);
}

void do_generic_entity_basic_kinematic_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    entity->vx += entity->ax *                      dt;
    entity->vy += (entity->ay + GRAVITY_CONSTANT) * dt;

    entity->last_x = entity->x;
    entity->last_y = entity->y;

    entity->x  += entity->vx *                       dt;
    entity->y  += entity->vy *                       dt;
}

void do_generic_entity_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    /* entity->x += dt; */
    /* entity->y += dt; */
    /* entity->w += dt; */
    /* entity->h += dt; */
}

void do_player_entity_input(struct entity* entity, int gamepad_id, float dt) {
    struct game_controller* gamepad = get_gamepad(gamepad_id);

    bool pause      = is_key_down(KEY_ESCAPE) || controller_button_pressed(gamepad, BUTTON_START);
    bool move_right = is_key_down(KEY_D)    || gamepad->buttons[DPAD_RIGHT];
    bool move_left  = is_key_down(KEY_A)    || gamepad->buttons[DPAD_LEFT];
    bool attack     = is_key_pressed(KEY_J) || controller_button_pressed(gamepad, BUTTON_X);
    bool jump       = is_key_pressed(KEY_SPACE) || controller_button_pressed(gamepad, BUTTON_A);
    bool dash       = is_key_pressed(KEY_SHIFT) || (gamepad->triggers.right - gamepad->last_triggers.right) >= 0.75;

    /* This state, is not really stored per say. Since the
     game doesn't need to care whether I'm technically lookin up or not?
    Well I'll see soon*/
    bool aiming_down = is_key_down(KEY_S) || gamepad->buttons[DPAD_DOWN] || gamepad->left_stick.axes[1] >  0.25;
    bool aiming_up   = is_key_down(KEY_W) || gamepad->buttons[DPAD_UP]   || gamepad->left_stick.axes[1] < -0.25;

    entity->ax = 0;


    if (pause) {
        game_state->menu_mode = GAMEPLAY_UI_PAUSEMENU;
        game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_TO_PAUSE;
        game_state->ingame_transition_timer[0] = INGAME_PAN_OUT_TIMER;
        game_state->ingame_transition_timer[1] = INGAME_PAN_OUT_TIMER;
    }

    /* basic movements */
    const int MAX_ACCELERATION = 30;
    {
        if (fabs(gamepad->left_stick.axes[0]) >= 0.1) {
            entity->ax = MAX_ACCELERATION * gamepad->left_stick.axes[0];
            entity->facing_dir = (int)float_sign(entity->ax);
        }

        if (move_right) {
            entity->ax = MAX_ACCELERATION;
            entity->facing_dir = 1;
        } else if (move_left) {
            entity->ax = -MAX_ACCELERATION;
            entity->facing_dir = -1;
        }
    }


    /* debug code */
    {
        if (noclip) {
            entity->ay = 0;
            if (fabs(gamepad->left_stick.axes[1]) >= 0.1) {
                entity->ay = MAX_ACCELERATION * gamepad->left_stick.axes[1];
            }
        }
    }

    if (dash) {
        if (!entity->dash && (fabs(entity->ax) > 0 || !entity->onground)) {
            entity->vy = 0;

            if (entity->sliding_against_wall) {
                entity->facing_dir *= -1;
            }

            camera_traumatize(&game_camera, 0.0375);
            entity->dash       = true;
            entity->dash_timer = ENTITY_DASH_TIME_MAX;
        }
    }

    if (entity->dash) {
        entity->ax = 0;

        const int MAX_SPEED = (DISTANCE_PER_DASH / ENTITY_DASH_TIME_MAX);
        entity->vx = MAX_SPEED * entity->facing_dir;
        entity->dash_timer -= dt;

        if (entity->dash_timer < 0) {
            entity->dash = false;
        }
    }

    if (entity->onground) {
        entity->coyote_jump_timer         = ENTITY_COYOTE_JUMP_TIMER_MAX;
        entity->current_jump_count        = 0;
    } else {
        /* interesting coyote jump bug. Actually I don't even know what expected behavior should be... */
        entity->coyote_jump_timer         -= dt;
        entity->player_variable_jump_time -= dt;
    }

    if (jump) {
        if ((entity->onground ||
             entity->coyote_jump_timer > 0 ||
             entity->current_jump_count < entity->max_allowed_jump_count)) {
            entity->vy                  = -5.5;
            entity->player_variable_jump_time = PLAYER_VARIABLE_JUMP_TIME_LIMIT;

            entity->current_jump_count += 1;

            /* jump particles */
            if (!entity->onground) {
                struct particle_emitter* splatter = particle_emitter_allocate();
                splatter->x = splatter->x1 = entity->x;
                splatter->y = splatter->y1 = entity->y + entity->h;
                splatter->emission_rate = 0;
                splatter->emission_count = 7;
                splatter->max_emissions = 1;
                splatter->particle_color = color4f(0.8, 0.8, 0.8, 1.0);
                splatter->particle_max_lifetime = 1;
            }

            if (entity->sliding_against_wall) {
                entity->apply_wall_jump_force_timer = 0.15;
                entity->opposite_facing_direction = -entity->facing_dir;
                entity->facing_dir *= -1;
            }

            entity->onground            = false;
        }
    }

    if (attack) {
        if (entity->attack_cooldown_timer <= 0) {
            const float HITBOX_W = 0.65;

            float hitbox_x = entity->x;
            float hitbox_y = entity->y;
            float hitbox_w = HITBOX_W;
            float hitbox_h = entity->h;

            int facing_dir = entity->facing_dir;
            int direction = ATTACK_DIRECTION_RIGHT;
            if (facing_dir == 0) facing_dir = 1;

            if (aiming_up || aiming_down) {
                if (aiming_up) {
                    hitbox_h = entity->h * 1.3;
                    hitbox_y = entity->y - hitbox_h;
                    direction = ATTACK_DIRECTION_UP;
                } else {
                    hitbox_y = entity->y + entity->h;
                    direction = ATTACK_DIRECTION_DOWN;
                    hitbox_h = entity->h * 1.3;
                }
            } else {
                if (facing_dir == 1) {
                    hitbox_x = (entity->x + entity->w);
                    direction = ATTACK_DIRECTION_RIGHT;
                } else {
                    hitbox_x = (entity->x) - HITBOX_W;
                    direction = ATTACK_DIRECTION_LEFT;
                }
            }


            entity->attack_cooldown_timer = PLAYER_ATTACK_COOLDOWN_TIMER_MAX;
            push_melee_with_knockback_hitbox(entity, hitbox_x, hitbox_y, hitbox_w, hitbox_h, 1, direction);
        }
    }

    {
        /* wall jump has priority of force. */
        if (entity->apply_attack_knockback_force_timer > 0) {
            const int KNOCKDOWN_MAGNITUDE_X = 2;
            const int KNOCKDOWN_MAGNITUDE_Y = 2;

            switch (entity->attack_direction) {
                /* on left and right, cancel knockback if you're facing away.
                 I have no idea how accurate this looks, but this'll probably prevent a weird issue?*/
                case ATTACK_DIRECTION_RIGHT: {
                    if (entity->facing_dir == 1) {
                        entity->vx = -KNOCKDOWN_MAGNITUDE_X;
                    } else {
                        entity->apply_attack_knockback_force_timer = 0;   
                    }
                } break;
                case ATTACK_DIRECTION_LEFT: {
                    if (entity->facing_dir == -1) {
                        entity->vx = KNOCKDOWN_MAGNITUDE_X;
                    } else {
                        entity->apply_attack_knockback_force_timer = 0;  
                    } 
                } break;
                case ATTACK_DIRECTION_DOWN: {
                    /* allows for "bouncing" */
                    entity->vy = -KNOCKDOWN_MAGNITUDE_Y * 2;
                } break;
                case ATTACK_DIRECTION_UP: {
                    entity->vy = KNOCKDOWN_MAGNITUDE_Y;
                } break;
            }
        }

        if (entity->apply_wall_jump_force_timer > 0) {
            /* Should be a velocity to be better looking. */
            entity->ax = entity->opposite_facing_direction * 50;
            entity->vy = -8;
            entity->sliding_against_wall = false;
        }
    }


    if (is_key_down(KEY_SPACE) || gamepad->buttons[BUTTON_A]) {
        if (!entity->onground && entity->player_variable_jump_time > 0) {
            entity->vy -= PLAYER_VARIABLE_JUMP_ACCELERATION * dt;
        }
    }

    entity->attack_cooldown_timer -= dt;
}

local bool block_player_input = false;
void do_player_entity_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    if (entity->death_state != DEATH_STATE_ALIVE)
        return;

    if (entity->max_allowed_jump_count <= 0) entity->max_allowed_jump_count = 1;

    if (animation_id == GAME_ANIMATION_ID_NONE && !block_player_input) {
        do_player_entity_input(entity, 0, dt);
    }
    /* game collisions, not physics */

    /* check for spike hits */
    {
        struct tilemap_sample_interval sample_region = tilemap_sampling_region_around_moving_entity(tilemap, entity);
        for (unsigned y = sample_region.min_y; y < sample_region.max_y; ++y) {
            for (unsigned x = sample_region.min_x; x < sample_region.max_x; ++x) {
                struct tile* t = &tilemap->tiles[y * tilemap->width + x];

                if (t->id == TILE_NONE) continue;

                if (rectangle_intersects_v(
                        entity->x, entity->y, entity->w, entity->h,
                        t->x, t->y, 1, 1
                    )) {
                    if (t->id == TILE_SPIKE_LEFT  ||
                        t->id == TILE_SPIKE_RIGHT ||
                        t->id == TILE_SPIKE_DOWN  ||
                        t->id == TILE_SPIKE_UP) {
                        /* for now just restore a bad fall */
                        /* NOTE(jerry): global. Shit! */
                        restore_player_to_last_good_grounded();
                    }
                }
            }
        }
    }
    /* check if I hit transition then change level */
    {
        /* NOTE(jerry): I store string pointers in the "queue"... So this has to be a pointer inside of the loop. */
        for (unsigned index = 0; index < tilemap->transition_zone_count; ++index) {
            struct transition_zone* t = (&tilemap->transitions[index]);
            if (rectangle_intersects_v(entity->x, entity->y, entity->w, entity->h, t->x, t->y, t->w, t->h)) {
                game_queue_load_level(&game_memory_arena,
                                      t->zone_filename,
                                      t->zone_link);
                break;
            }
        }
    }
}

struct entity entity_create_player(float x, float y) {
    struct entity result = {
        .x = x, .y = y,
        .type = ENTITY_TYPE_PLAYER,
        .w = (0.5),
        .h = 1,
        .health = 3,
        .max_health = 3,
        .flags = ENTITY_FLAGS_PERMENANT,
        .max_allowed_jump_count = 2,
    };

    return result;
}

struct particle_emitter* entity_lost_soul_particles(float x, float y) {
    struct particle_emitter* emitter = particle_emitter_allocate();

    emitter->emission_rate = 0.002;
    emitter->emission_count = 8;
    emitter->particle_color = color4f(0.7, 0.1, 0.2, 1.0);
    emitter->particle_texture = particle_textures[0];
    emitter->particle_max_lifetime = 1;
    emitter->collides_with_world = true;

    {
        emitter->x  = x - 0.25;
        emitter->y  = y + 0.25;
        emitter->x1 = x + 0.25;
        emitter->y1 = y + 0.25;
    }

    return emitter;
}

/* For now I'm hardcoding immortality into these things... */
struct entity entity_create_hovering_lost_soul(float x, float y) {
    struct entity result = {
        .x = x, .y = y,
        .type = ENTITY_TYPE_HOVERING_LOST_SOUL,
        .w = (0.5), .h = (0.5),
        .health = 2, .max_health = 2,
    };

    return result;
}

struct entity entity_create_lost_soul(float x, float y) {
    struct entity result = {
        .x = x, .y = y,
        .type = ENTITY_TYPE_LOST_SOUL,
        .w = (0.5), .h = (0.5),
        .health = 2, .max_health = 2,
    };

    return result;
}

struct entity entity_create_volatile_lost_soul(float x, float y) {
    struct entity result = {
        .x = x, .y = y,
        .type = ENTITY_TYPE_VOLATILE_LOST_SOUL,
        .w = (0.5), .h = (0.5),
        .health = 2, .max_health = 2,
    };

    return result;
}

/* TODO(jerry):
   A nice way would just be to create a table of "templates", which would
   also work nicely for avoiding duplication.
*/

/* use for construction */
void entity_get_type_dimensions(uint32_t type, float* width, float* height) {
    float out_width  = 1;
    float out_height = 1;

    /* NOTE(jerry): lots of duplication. Maybe clean up later? */
    switch (type) {
        case ENTITY_TYPE_HOVERING_LOST_SOUL:
        case ENTITY_TYPE_LOST_SOUL:
        case ENTITY_TYPE_VOLATILE_LOST_SOUL:
        {
            out_width = 0.5;
            out_height = 0.5;
        } break;
        default: {} break;
    }

    safe_assignment(width)  = out_width;
    safe_assignment(height) = out_height;
}

/* factory, does not initialize them. It basically only allocates them */
struct entity construct_entity_of_type(uint32_t type, float x, float y) {
    struct entity result;
    switch (type) {
        case ENTITY_TYPE_PLAYER: {
            result = entity_create_player(x, y);
        } break;
        case ENTITY_TYPE_HOVERING_LOST_SOUL: {
            result = entity_create_hovering_lost_soul(x, y);
        } break;
        case ENTITY_TYPE_VOLATILE_LOST_SOUL: {
            result = entity_create_volatile_lost_soul(x, y);
        } break;
        case ENTITY_TYPE_LOST_SOUL: {
            result = entity_create_lost_soul(x, y);
        } break;
    }

    /* 
       NOTE(jerry):
       Fix problems related to interpolation.

       The editor doesn't play animation, so it never updates the state that
       interpolated rendering requires.
     */
    result.last_x = result.initial_x = result.x;
    result.last_y = result.initial_y = result.y;
    return result;
}

void initialize_entity(struct entity* entity) {
    /* 
       secondary step only during loading 
       
       This is for any entities that have "extra" parts.
    */
    switch (entity->type) {
        case ENTITY_TYPE_LOST_SOUL:
        case ENTITY_TYPE_VOLATILE_LOST_SOUL:
        case ENTITY_TYPE_HOVERING_LOST_SOUL: {
            entity->lost_soul_info.owned_emitter = entity_lost_soul_particles(entity->x, entity->y);
        } break;
    }
}

void entity_record_locations_for_linger_shadows(struct entity* entity) {
    if (entity->linger_shadow_count < ENTITY_DASH_SHADOW_MAX_AMOUNT && entity->linger_shadow_sample_record_timer < 0) {
        struct entity_dash_shadow* next_shadow = &entity->linger_shadows[entity->linger_shadow_count++];
        next_shadow->x = entity->x;
        next_shadow->y = entity->y;
        next_shadow->t = ENTITY_DASH_SHADOW_MAX_LINGER_LIMIT;
        entity->linger_shadow_sample_record_timer = ENTITY_DASH_SHADOW_SAMPLE_RECORD_TIMER_MAX;
    }
}

void do_entity_physics_updates(struct entity_iterator* entities, struct tilemap* tilemap, float dt) {
    const float IMPACT_INFLUENCE_MAX_DISTANCE_SQ = 12.0f;
    struct entity* player = &game_state->persistent_entities[0];

    for (struct entity* current_entity = entity_iterator_begin(entities);
         !entity_iterator_done(entities);
         current_entity = entity_iterator_next(entities)) {
        switch (current_entity->type) {
            case ENTITY_TYPE_PLAYER: {
                do_player_entity_physics_update(current_entity, tilemap, dt);
            } break;
            case ENTITY_TYPE_HOVERING_LOST_SOUL:
            case ENTITY_TYPE_LOST_SOUL:
            case ENTITY_TYPE_VOLATILE_LOST_SOUL: {
                do_lost_soul_physics_update(current_entity, tilemap, dt);
            } break;
            default:
                if (entity_is_physics_active(current_entity->type)) {
                    do_generic_entity_physics_update(current_entity, tilemap, dt);
                } else {
                    do_generic_entity_basic_kinematic_update(current_entity, tilemap, dt);
                }
                break;
        }

        if (current_entity->dash) {
            entity_record_locations_for_linger_shadows(current_entity);
        }

        current_entity->linger_shadow_sample_record_timer  -= dt;
        current_entity->apply_wall_jump_force_timer        -= dt;
        current_entity->apply_attack_knockback_force_timer -= dt;

        {
            float impact_influence = distance_sq(current_entity->x, current_entity->y, player->x, player->y);
            impact_influence = clampf(impact_influence, 0, IMPACT_INFLUENCE_MAX_DISTANCE_SQ);
            impact_influence = (IMPACT_INFLUENCE_MAX_DISTANCE_SQ - impact_influence) / IMPACT_INFLUENCE_MAX_DISTANCE_SQ;
            entity_do_ground_impact(current_entity, impact_influence);
        }
    }
}

void do_entity_updates(struct entity_iterator* entities, struct tilemap* tilemap, float dt) {
    for (struct entity* current_entity = entity_iterator_begin(entities);
         !entity_iterator_done(entities);
         current_entity = entity_iterator_next(entities)) {

        if (current_entity->health <= 0)  {
            if (current_entity->death_state == DEATH_STATE_ALIVE) {
                current_entity->death_state = DEATH_STATE_DYING;
            }
        } 

        switch (current_entity->type) {
            case ENTITY_TYPE_PLAYER:
                do_player_entity_update(current_entity, tilemap, dt);
                break;
            case ENTITY_TYPE_HOVERING_LOST_SOUL:
            case ENTITY_TYPE_LOST_SOUL:
            case ENTITY_TYPE_VOLATILE_LOST_SOUL:
                do_lost_soul_update(current_entity, tilemap, dt);
            break;
            default:
                do_generic_entity_update(current_entity, tilemap, dt);
                break;
        }
    }
}

/* technically... Entities should know their visual position as separate state instead of
 having this here but whatever.*/
local void draw_entity(struct entity* current_entity, float dt, float interpolation_value) {
    switch (current_entity->type) {
        case ENTITY_TYPE_PLAYER: {
            if (current_entity->death_state == DEATH_STATE_ALIVE) {
                for (int shadow_index = current_entity->linger_shadow_count-1; shadow_index >= 0; --shadow_index) {
                    struct entity_dash_shadow* shadow = current_entity->linger_shadows + shadow_index;

                    union color4f shadow_color = active_colorscheme.primary;
                    shadow_color.r /= 3;
                    shadow_color.g /= 3;
                    shadow_color.b /= 3;
                    shadow_color.a = interpolation_clamp(shadow->t / ENTITY_DASH_SHADOW_MAX_LINGER_LIMIT);
                    shadow->t -= dt;

                    draw_filled_rectangle(shadow->x, shadow->y, current_entity->w, current_entity->h, shadow_color);

                    if (shadow->t <= 0.0) {
                        current_entity->linger_shadows[shadow_index] = current_entity->linger_shadows[--current_entity->linger_shadow_count];
                    }
                }

                draw_filled_rectangle(entity_lerp_x(current_entity, interpolation_value),
                                      entity_lerp_y(current_entity, interpolation_value),
                                      current_entity->w, current_entity->h, active_colorscheme.primary);
            }
        } break;
        default: {
            if (current_entity->death_state == DEATH_STATE_ALIVE) {
                draw_filled_rectangle(entity_lerp_x(current_entity, interpolation_value),
                                      entity_lerp_y(current_entity, interpolation_value),
                                      current_entity->w, current_entity->h, active_colorscheme.primary);
            }
        } break;
    }
}

void draw_all_entities(struct entity_iterator* entities, float dt, float interpolation_value) {
    for (struct entity* current_entity = entity_iterator_begin(entities);
         !entity_iterator_done(entities);
         current_entity = entity_iterator_next(entities)) {
        draw_entity(current_entity, dt, interpolation_value);
    }
}

local bool hitbox_check_is_entity_ignored(struct hitbox* hitbox, struct entity* entity) {
    for (unsigned ignored_entity_index = 0; ignored_entity_index < array_count(hitbox->ignored_entities); ++ignored_entity_index) {
        struct entity* ignored_entity = hitbox->ignored_entities[ignored_entity_index];

        if (entity == ignored_entity)
            return true;
    }

    return false;
}

void hitbox_try_and_apply_knockback(struct hitbox* hitbox, float time) {
    if (hitbox->type == HITBOX_TYPE_HURT_WITH_KNOCKBACK && hitbox->owner) {
        hitbox->owner->attack_direction                   = hitbox->direction;
        hitbox->owner->apply_attack_knockback_force_timer = time;
    }
}

void hitbox_handle_entity_interactions(struct hitbox* hitbox, struct entity_iterator* entities) {
    /* O(n^2) really hurts... Hmmm yikes! */
    for (struct entity* current_entity = entity_iterator_begin(entities);
         !entity_iterator_done(entities);
         current_entity = entity_iterator_next(entities)) {

        if (current_entity->death_state == DEATH_STATE_ALIVE) {
            bool ignore_entity = hitbox_check_is_entity_ignored(hitbox, current_entity);

            if (!ignore_entity) {
                /* do hit response on entities! For now just remove damage */
                if (rectangle_intersects_v(
                        hitbox->x, hitbox->y, hitbox->w, hitbox->h,
                        current_entity->x, current_entity->y, current_entity->w, current_entity->h
                    )) {
                    current_entity->health -= hitbox->damage;
                    camera_traumatize(&game_camera, 0.01275);
                    hitbox_try_and_apply_knockback(hitbox, 0.1);
                }
            }
        }
    }
}

void hitbox_handle_tilemap_interactions(struct hitbox* hitbox, struct tilemap* tilemap) {
    unsigned total_tile_count = tilemap->width * tilemap->height;
    for (unsigned index = 0; index < total_tile_count; ++index) {
        struct tile* t = &tilemap->tiles[index];

        if (t->id == TILE_NONE) continue;

        if (rectangle_intersects_v(
                hitbox->x, hitbox->y, hitbox->w, hitbox->h,
                t->x, t->y, 1, 1
            )) {
            switch (t->id) {
                case TILE_SPIKE_LEFT:
                case TILE_SPIKE_RIGHT:
                case TILE_SPIKE_DOWN:
                case TILE_SPIKE_UP: {
                    hitbox_try_and_apply_knockback(hitbox, 0.16);
                    camera_traumatize(&game_camera, 0.01275);
                    return;
                } break;
                default: {
                    /* create hit particle. otherwise bye bye */
                    continue;
                } break;
            }
        }
    }
}

void update_all_hitboxes(struct entity_iterator* entities, struct tilemap* tilemap, float dt) {
    for (int index = hitbox_count-1; index >= 0; --index) {
        struct hitbox* current_hitbox = hitbox_pool + index;

        hitbox_handle_entity_interactions(current_hitbox, entities);
        hitbox_handle_tilemap_interactions(current_hitbox, tilemap);

        current_hitbox->lifetime -= dt;

        if (current_hitbox->lifetime <= 0) {
#ifdef DEV
            push_lingering_hitbox(current_hitbox->x, current_hitbox->y, current_hitbox->w, current_hitbox->h);
#endif
            hitbox_pool[index] = hitbox_pool[--hitbox_count];
        }
    }
#ifdef DEV
    DEBUG_update_all_lingering_hitboxes(dt);
#endif
}

void cleanup_for_entity(struct entity* entity) {
    if (entity->lost_soul_info.owned_emitter) {
        particle_emitter_clear_all_particles(entity->lost_soul_info.owned_emitter);
        entity->lost_soul_info.owned_emitter->alive = false;
        entity->lost_soul_info.owned_emitter = 0;
    }
}

void DEBUG_draw_all_hitboxes(void) {
#ifdef DEV
    for (int index = hitbox_count-1; index >= 0; --index) {
        struct hitbox* current_hitbox = hitbox_pool + index;
        draw_rectangle(current_hitbox->x, current_hitbox->y, current_hitbox->w, current_hitbox->h, COLOR4F_RED);
    }

    DEBUG_draw_all_lingering_hitboxes();
#endif
}
