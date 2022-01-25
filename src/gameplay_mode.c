struct entity player = {
    // no units, prolly pixels
    .x = -VPIXELS_PER_METER/4,
    .y = 0,
    .w = VPIXELS_PER_METER/2,
    .h = VPIXELS_PER_METER,
};

local void load_gameplay_resources(void) {
    DEBUG_load_all_tile_assets();
    game_state->loaded_level = DEBUG_tilemap_from_file(&game_memory_arena, "assets/testmap.txt");
}

local void do_physics(float dt) {
    struct tilemap* tilemap = game_state->loaded_level;
    player.vy += VPIXELS_PER_METER*20 * dt;

    do_moving_entity_horizontal_collision_response(tilemap, &player, dt);
    do_moving_entity_vertical_collision_response(tilemap, &player, dt);
    evaluate_moving_entity_grounded_status(tilemap, &player, dt);
}

local void do_player_input(float dt) {
    struct game_controller* gamepad = get_gamepad(0);

    player.vx = 0;
    const float MOVEMENT_THRESHOLD = 0.5;
    const int MAX_SPEED = 5;

    bool move_right = is_key_down(KEY_D) || gamepad->buttons[DPAD_RIGHT];
    bool move_left  = is_key_down(KEY_A) || gamepad->buttons[DPAD_LEFT];

    if (move_right) {
        player.vx = VPIXELS_PER_METER * MAX_SPEED;
    } else if (move_left) {
        player.vx = VPIXELS_PER_METER * -MAX_SPEED;
    }

    if (gamepad->left_stick.axes[0] != 0) {
        player.vx = VPIXELS_PER_METER * MAX_SPEED * gamepad->left_stick.axes[0];
    }

    /* if (gamepad->left_stick.axes[1] != 0) { */
    /*     player.vy = VPIXELS_PER_METER * MAX_SPEED * gamepad->left_stick.axes[1]; */
    /* } */

    if (roundf(player.vy) == 0) {
        player.jump_leniancy_timer = 0.3;
    }

    if (is_key_pressed(KEY_SPACE) || gamepad->buttons[BUTTON_A]) {
        if (player.onground && player.vy == 0) {
            player.vy = VPIXELS_PER_METER * -10;
            player.onground = false;
        }
    }

    player.jump_leniancy_timer -= dt;
}

local void game_update_render_frame(float dt) {
    do_player_input(dt);
    do_physics(dt);

    begin_graphics_frame(); {
        /* might need to rethink camera interface. 
           I still want it to operate under one global camera, but
           obviously you don't always want the camera. */
        set_active_camera(get_global_camera());

        camera_set_focus_speed_x(12);
        camera_set_focus_speed_y(5);
        camera_set_focus_position(player.x - player.w/2, player.y - player.h/2);

        draw_filled_rectangle(player.x, player.y, player.w, player.h, color4f(0.3, 0.2, 1.0, 1.0));
        DEBUG_draw_tilemap(game_state->loaded_level);
    } end_graphics_frame();

    begin_graphics_frame(); {
        draw_text(test_font, 0, 0,
                  format_temp("onground: %d\npx: %f\npy:%15.15f\npvx: %f\npvy: %f\n%f ms",
                              player.onground,
                              player.x, player.y, player.vx, player.vy, dt * 1000.0f),
                  COLOR4F_WHITE);
    } end_graphics_frame();
}
