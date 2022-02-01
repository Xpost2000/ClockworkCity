local void load_gameplay_resources(void) {
    load_all_tile_assets();
}

local void do_physics(float dt) {
    if (noclip) {
        player.x += player.vx * dt;
        player.y += player.vy * dt;
        return;   
    }

    struct tilemap* tilemap = game_state->loaded_level;

    player.vx += player.ax * dt;
    player.vx -= (player.vx * 3 * dt);

    const int MAX_SPEED = 150 * VPIXELS_PER_METER;
    if (fabs(player.vx) > MAX_SPEED) {
        float sgn = float_sign(player.vx);
        player.vx = MAX_SPEED * sgn;
    }

    if (fabs(player.vx) < (30 * VPIXELS_PER_METER)) {
        player.dash = false;
    }

    player.vy += (player.ay + VPIXELS_PER_METER*20) * dt;
    if (player.dash) player.vy = 0;

    do_moving_entity_horizontal_collision_response(tilemap, &player, dt);
    do_moving_entity_vertical_collision_response(tilemap, &player, dt);
    evaluate_moving_entity_grounded_status(tilemap, &player, dt);

    /* game collisions, not physics */
    /* check if I hit transition then change level */
    {
        for (unsigned index = 0; index < tilemap->transition_zone_count; ++index) {
            struct transition_zone t = (tilemap->transitions[index]);
            if (rectangle_intersects_v(player.x, player.y, player.w, player.h, t.x, t.y, t.w, t.h)) {
                game_load_level(&game_memory_arena, t.zone_filename, t.zone_link);
                break;
            }
        }
    }
}

local void do_player_input(float dt) {
    struct game_controller* gamepad = get_gamepad(0);

    bool move_right = is_key_down(KEY_D) || gamepad->buttons[DPAD_RIGHT];
    bool move_left  = is_key_down(KEY_A) || gamepad->buttons[DPAD_LEFT];

    player.ax = 0;

    const int MAX_ACCELERATION = 25;
    if (move_right) {
        player.ax = VPIXELS_PER_METER * MAX_ACCELERATION;
        player.facing_dir = 1;
    } else if (move_left) {
        player.ax = VPIXELS_PER_METER * -MAX_ACCELERATION;
        player.facing_dir = -1;
    }

    if (fabs(gamepad->left_stick.axes[0]) >= 0.1) {
        player.ax = VPIXELS_PER_METER * MAX_ACCELERATION * gamepad->left_stick.axes[0];
        player.facing_dir = (int)float_sign(player.ax);
    }

    if (noclip) {
        player.ay = 0;
        if (fabs(gamepad->left_stick.axes[1]) >= 0.1) {
            player.ay = VPIXELS_PER_METER * MAX_ACCELERATION * gamepad->left_stick.axes[1];
        }
    }

    if (roundf(player.vy) == 0) {
        player.jump_leniancy_timer = 0.3;
    }

    if (is_key_pressed(KEY_SHIFT) || roundf(gamepad->triggers.right) == 1.0f) {
        if (!player.dash) {
            player.vy = 0;
            const int MAX_SPEED = 90 * VPIXELS_PER_METER;
            player.vx = MAX_SPEED * player.facing_dir;
            camera_traumatize(0.0375);
            player.dash = true;
        }
    }

    if (is_key_pressed(KEY_SPACE) || gamepad->buttons[BUTTON_A]) {
        if (player.onground && player.vy == 0) {
            player.vy = VPIXELS_PER_METER * -10;
            player.onground = false;
        }
    }

    player.jump_leniancy_timer -= dt;
}

local void DEBUG_draw_debug_stuff(void) {
    /*add debug rendering code here*/
}

local void game_update_render_frame(float dt) {
    struct game_controller* gamepad = get_gamepad(0);
    do_player_input(dt);

    /*fixed physics framerate update*/ {
        local float physics_accumulation_timer = 0;
        const int PHYSICS_FRAMERATE = 300;
        const float PHYSICS_TIMESTEP = 1.0f / (float)(PHYSICS_FRAMERATE);

        while (physics_accumulation_timer > 0.0f) {
            do_physics(PHYSICS_TIMESTEP);
            physics_accumulation_timer -= PHYSICS_TIMESTEP;
        }

        physics_accumulation_timer += dt;
    }

    begin_graphics_frame(); {
        /* might need to rethink camera interface. 
           I still want it to operate under one global camera, but
           obviously you don't always want the camera. */
        set_active_camera(get_global_camera());
        set_render_scale(ratio_with_screen_width(TILES_PER_SCREEN));

        camera_set_focus_speed_x(12);
        camera_set_focus_speed_y(5);
        camera_set_bounds(game_state->loaded_level->bounds_min_x, game_state->loaded_level->bounds_min_y,
                          game_state->loaded_level->bounds_max_x, game_state->loaded_level->bounds_max_y);
/*         printf("%f, %f, %f, %f vs:\n", */
/* game_state->loaded_level->bounds_min_x, game_state->loaded_level->bounds_min_y, */
/*                           game_state->loaded_level->bounds_max_x, game_state->loaded_level->bounds_max_y */
/*         ); */
        camera_set_focus_position(player.x - player.w/2, player.y - player.h/2);

        draw_filled_rectangle(player.x, player.y, player.w, player.h, color4f(0.3, 0.2, 1.0, 1.0));
        draw_tilemap(game_state->loaded_level);
        DEBUG_draw_debug_stuff();
    } end_graphics_frame();
#if 1
    begin_graphics_frame(); {
        int dimens[2];
        get_screen_dimensions(dimens, dimens+1);
        draw_filled_rectangle(0, 0, dimens[0], dimens[1], color4f(0,0,0,0.5));

        draw_text(test_font, 0, 0,
                  format_temp("onground: %d\npx: %f\npy:%15.15f\npvx: %f\npvy: %f\n%f ms (%f fps)\n",
                              player.onground, player.x, player.y, player.vx, player.vy, dt * 1000.0f, (1.0f/dt)),
                  COLOR4F_WHITE);
        {
            int tdimens[2];
            char* text = "There's a radio stuck in my brain.\nHello Sailer!";
            get_text_dimensions(test2_font, text, tdimens, tdimens+1);
            draw_text(test2_font, dimens[0]/2 - tdimens[0]/2, dimens[1]/2 - tdimens[1]/2, text, COLOR4F_WHITE);
        }
    } end_graphics_frame();
#endif
}
