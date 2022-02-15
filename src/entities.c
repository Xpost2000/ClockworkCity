/* general entity procedures */
/*
  Camera influence is determined by distance to the player.
  
  TODO(jerry): Draw
*/
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
void do_player_entity_physics_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    if (noclip) {
        entity->vx      = entity->ax;
        entity->vy      = entity->ay;
        entity->last_vy = entity->vy;
    }

    entity->vx += entity->ax * dt;
    entity->vy += (entity->ay + GRAVITY_CONSTANT) * dt;

    if (entity->dash) {
        entity->vx -= (entity->vx * 40 * dt);
    } else {
        entity->vx -= (entity->vx * 3 * dt);
    }

    const int MAX_SPEED = 500;
    if (fabs(entity->vx) > MAX_SPEED) {
        float sgn = float_sign(entity->vx);
        entity->vx = MAX_SPEED * sgn;
    }

    if (fabs(entity->vx) < (30)) {
        entity->dash = false;
    }

    if (entity->dash) entity->vy = 0;

    do_moving_entity_horizontal_collision_response(tilemap, entity, dt);
    do_moving_entity_vertical_collision_response(tilemap, entity, dt);
}

/* 
   technically physics updates for all entities should nearly be identical...
   However for now I only have the player so... I can't say.
*/
void do_generic_entity_physics_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    entity->vx += entity->ax * dt;
    entity->vy += (entity->ay + GRAVITY_CONSTANT) * dt;

    do_moving_entity_horizontal_collision_response(tilemap, entity, dt);
    do_moving_entity_vertical_collision_response(tilemap, entity, dt);
}

void do_generic_entity_death_animation(struct entity* entity, float dt) {
    entity->death_state = DEATH_STATE_DEAD;
}

void do_generic_entity_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    
}

void do_player_entity_input(struct entity* entity, int gamepad_id, float dt) {
    struct game_controller* gamepad = get_gamepad(gamepad_id);

    bool move_right = is_key_down(KEY_D) || gamepad->buttons[DPAD_RIGHT];
    bool move_left  = is_key_down(KEY_A) || gamepad->buttons[DPAD_LEFT];

    entity->ax = 0;

    const int MAX_ACCELERATION = 30;

    if (is_key_down(KEY_ESCAPE) || (!gamepad->last_buttons[BUTTON_START] && gamepad->buttons[BUTTON_START])) {
        game_state->menu_mode = GAMEPLAY_UI_PAUSEMENU;
        game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_TO_PAUSE;
        game_state->ingame_transition_timer[0] = INGAME_PAN_OUT_TIMER;
        game_state->ingame_transition_timer[1] = INGAME_PAN_OUT_TIMER;
    }

    if (is_key_down(KEY_T)) {
        camera_set_focus_zoom_level(&game_camera, 0.5);
        camera_set_focus_speed_zoom(&game_camera, 0.75);
    } else if (is_key_down(KEY_G)) {
        camera_set_focus_zoom_level(&game_camera, 1.0);
        camera_set_focus_speed_zoom(&game_camera, 1.15);
    } else if (is_key_down(KEY_H)) {
        camera_set_focus_zoom_level(&game_camera, 1.5);
        camera_set_focus_speed_zoom(&game_camera, 1.15);
    }

    if (move_right) {
        entity->ax = MAX_ACCELERATION;
        entity->facing_dir = 1;
    } else if (move_left) {
        entity->ax = -MAX_ACCELERATION;
        entity->facing_dir = -1;
    }

    if (fabs(gamepad->left_stick.axes[0]) >= 0.1) {
        entity->ax = MAX_ACCELERATION * gamepad->left_stick.axes[0];
        entity->facing_dir = (int)float_sign(entity->ax);
    }

    if (noclip) {
        entity->ay = 0;
        if (fabs(gamepad->left_stick.axes[1]) >= 0.1) {
            entity->ay = MAX_ACCELERATION * gamepad->left_stick.axes[1];
        }
    }

    if (roundf(entity->vy) == 0) {
        entity->jump_leniancy_timer = 0.3;
    }

    if (is_key_pressed(KEY_SHIFT) || roundf(gamepad->triggers.right) == 1.0f) {
        if (!entity->dash) {
            entity->vy = 0;
            const int MAX_SPEED = 300;
            entity->vx = MAX_SPEED * entity->facing_dir;
            entity->vy = MAX_SPEED * entity->facing_dir;
            camera_traumatize(&game_camera, 0.0675);
            entity->dash = true;
        }
    }

    if (is_key_pressed(KEY_SPACE) || gamepad->buttons[BUTTON_A]) {
        if (entity->onground) {
            entity->vy = -10;
            entity->onground = false;
        }
    }

    entity->jump_leniancy_timer -= dt;
}

void do_player_entity_death_animation(struct entity* entity, float dt) {
    /*
      NOTE(jerry):
      Because of how I want this to work. This heavily hooks into the gamestate to control
      stuff.
      
      and will requires lots of intrusive code to work properly. Sorry! I can't think of anything
      else to do.
    */
    do_generic_entity_death_animation(entity, dt);
}

void do_player_entity_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    do_player_entity_input(entity, 0, dt);
    /* game collisions, not physics */
    /* check if I hit transition then change level */
    {
        for (unsigned index = 0; index < tilemap->transition_zone_count; ++index) {
            struct transition_zone t = (tilemap->transitions[index]);
            if (rectangle_intersects_v(entity->x, entity->y, entity->w, entity->h, t.x, t.y, t.w, t.h)) {
                game_load_level(&game_memory_arena, t.zone_filename, t.zone_link);
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
        .health = 5,
        .flags = ENTITY_FLAGS_PERMENANT,
    };

    return result;
}

void do_entity_physics_updates(struct entity_iterator* entities, struct tilemap* tilemap, float dt) {
    const float IMPACT_INFLUENCE_MAX_DISTANCE_SQ = 12.0f;
    struct entity* player = &game_state->persistent_entities[0];

    for (struct entity* current_entity = entity_iterator_begin(entities);
         !entity_iterator_done(entities);
         current_entity = entity_iterator_next(entities)) {
        switch (current_entity->type) {
            case ENTITY_TYPE_PLAYER:
                do_player_entity_physics_update(current_entity, tilemap, dt);
                break;
            default:
                do_generic_entity_physics_update(current_entity, tilemap, dt);
                break;
        }

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

            if (current_entity->death_state == DEATH_STATE_DYING) {
                switch (current_entity->type) {
                    case ENTITY_TYPE_PLAYER:
                        do_player_entity_death_animation(current_entity, dt);
                        break;
                    default:
                        do_generic_entity_death_animation(current_entity, dt);
                        break;
                }
            }
        } else {
            current_entity->death_state = DEATH_STATE_ALIVE;
            switch (current_entity->type) {
                case ENTITY_TYPE_PLAYER:
                    do_player_entity_update(current_entity, tilemap, dt);
                    break;
                default:
                    do_generic_entity_update(current_entity, tilemap, dt);
                    break;
            }
        }
    }
}