/* general entity procedures */
/*
  Camera influence is determined by distance to the player.
*/
local void entity_do_ground_impact(struct entity* entity, float camera_influence) {
    if (entity->last_vy >= (20)) {
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
    entity->ax = entity->ay = entity->vx = entity->vy = entity->last_vy = 0;
}

/* end of general entity procedures */

/* type procedures */
void do_player_entity_physics_update(struct entity* entity, struct tilemap* tilemap, float dt) {
    if (noclip) {
        entity->vx = entity->ax;
        entity->vy = entity->ay;
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
    } else if (is_key_down(KEY_G)) {
        camera_set_focus_zoom_level(&game_camera, 1.0);
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

    if (entity->last_vy > 0 && !noclip) {
        entity_do_ground_impact(entity, 1.0f);
    }
}
/* end of type procedures */

struct entity player = {
    // no units, prolly pixels
    .x = -4,
    .y = -5,
    .w = 1/2.0f,
    .h = 1,
};
