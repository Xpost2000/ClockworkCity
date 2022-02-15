#include "game_menus.c"

local struct particle_emitter* test_emitter = 0x12345;
local struct particle_emitter* test_emitter2 = 0x12345;

#define TEST_timer1_max 2
float TEST_timer1 = 0;
bool TEST_bool1 = 0;

local void load_gameplay_resources(void) {
    load_all_tile_assets();
}

local void gameplay_initialize(void) {
    test_emitter  = particle_emitter_allocate();
    test_emitter2 = particle_emitter_allocate();

    {
        test_emitter->emission_rate = 0.01;
        test_emitter->emission_count = 16;
        test_emitter->particle_color = color4f(1.0, 0.0, 0.0, 1.0);
        test_emitter->particle_max_lifetime = 1;
        test_emitter->collides_with_world = true;
    }

    {
        test_emitter2->emission_rate = 0.01;
        test_emitter2->emission_count = 16;
        test_emitter2->particle_color = color4f(0.12, 0.2, 0.85, 1.0);
        test_emitter2->particle_max_lifetime = 8;
        test_emitter2->collides_with_world = false;

        int a = 0;
        int b = -8;
        test_emitter2->x = a;
        test_emitter2->y = b;

        test_emitter2->x1 = a + 30;
        test_emitter2->y1 = b;
    }
}

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

local void do_physics(float dt) {
    if (noclip) {
        /*stupid*/
        player.vx = player.ax;
        player.vy = player.ay;
        /* player.x += player.vx * dt; */
        /* player.y += player.vy * dt; */
    }

    struct tilemap* tilemap = game_state->loaded_level;

    player.vx += player.ax * dt;
    if (player.dash) {
        player.vx -= (player.vx * 40 * dt);
    } else {
        player.vx -= (player.vx * 3 * dt);
    }

    const int MAX_SPEED = 500;
    if (fabs(player.vx) > MAX_SPEED) {
        float sgn = float_sign(player.vx);
        player.vx = MAX_SPEED * sgn;
    }

    if (fabs(player.vx) < (30)) {
        player.dash = false;
    }

    player.vy += (player.ay + GRAVITY_CONSTANT) * dt;
    if (player.dash) player.vy = 0;

    do_moving_entity_horizontal_collision_response(tilemap, &player, dt);
    do_moving_entity_vertical_collision_response(tilemap, &player, dt);

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

    if (player.last_vy > 0 && !noclip) {
        entity_do_ground_impact(&player, 1.0f);
    }
}

local void do_player_input(float dt) {
    struct game_controller* gamepad = get_gamepad(0);

    bool move_right = is_key_down(KEY_D) || gamepad->buttons[DPAD_RIGHT];
    bool move_left  = is_key_down(KEY_A) || gamepad->buttons[DPAD_LEFT];

    player.ax = 0;

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
        player.ax = MAX_ACCELERATION;
        player.facing_dir = 1;
    } else if (move_left) {
        player.ax = -MAX_ACCELERATION;
        player.facing_dir = -1;
    }

    if (fabs(gamepad->left_stick.axes[0]) >= 0.1) {
        player.ax = MAX_ACCELERATION * gamepad->left_stick.axes[0];
        player.facing_dir = (int)float_sign(player.ax);
    }

    if (noclip) {
        player.ay = 0;
        if (fabs(gamepad->left_stick.axes[1]) >= 0.1) {
            player.ay = MAX_ACCELERATION * gamepad->left_stick.axes[1];
        }
    }

    if (roundf(player.vy) == 0) {
        player.jump_leniancy_timer = 0.3;
    }

    if (is_key_pressed(KEY_SHIFT) || roundf(gamepad->triggers.right) == 1.0f) {
        if (!player.dash) {
            player.vy = 0;
            const int MAX_SPEED = 300;
            player.vx = MAX_SPEED * player.facing_dir;
            player.vy = MAX_SPEED * player.facing_dir;
            camera_traumatize(&game_camera, 0.0675);
            player.dash = true;
        }
    }

    if (is_key_pressed(KEY_SPACE) || gamepad->buttons[BUTTON_A]) {
        if (player.onground) {
            /* player.vy = -GRAVITY_CONSTANT; */
            player.vy = -10;
            player.onground = false;
        }
    }

    player.jump_leniancy_timer -= dt;
}

local void DEBUG_draw_debug_stuff(void) {
    /*add debug rendering code here*/

    /*boundaries of the loaded world*/
    {
        struct tilemap* tilemap = game_state->loaded_level;
        float bounds_width  = tilemap->bounds_max_x - tilemap->bounds_min_x;
        float bounds_height = tilemap->bounds_max_y - tilemap->bounds_min_y;

        draw_rectangle(tilemap->bounds_min_x, tilemap->bounds_min_y,
                       bounds_width, bounds_height, COLOR4F_BLUE);
    }

    /* Camera bounds */
    #if 0
    {
        struct camera* camera = get_global_camera();
        struct rectangle bounds = camera_get_bounds(camera);

        draw_rectangle(bounds.x, bounds.y, bounds.w, bounds.h, COLOR4F_BLUE);
    }
    #endif
}

local void DEBUG_draw_debug_ui_stuff(void) {
    begin_graphics_frame(NULL); {
        {
            struct memory_arena* arena = &game_memory_arena;
            size_t memusage = memory_arena_total_usage(arena);
            char* arena_msg = format_temp("(memory arena \"%s\") is using %d bytes\n(%d kb)\n(%d mb)\n(%d gb)\n(onground: %d)\n(player %f, %f, %f, %f)\n",
                                          arena->name, memusage, memusage / 1024, memusage / (1024 * 1024), memusage / (1024*1024*1024), player.onground, player.x, player.y, player.vx, player.vy);
            draw_text(_console_font, 0, 0, arena_msg, COLOR4F_GREEN);
        }
    } end_graphics_frame();
}

local void game_update_render_frame(float dt) {
    struct game_controller* gamepad = get_gamepad(0);

    if (game_state->menu_mode == GAMEPLAY_UI_INGAME) {
        do_player_input(dt);

        {
            local float physics_accumulation_timer = 0;
            const int PHYSICS_FRAMERATE = 300;
            const float PHYSICS_TIMESTEP = 1.0f / (float)(PHYSICS_FRAMERATE);

            while (physics_accumulation_timer > 0.0f) {
                do_physics(PHYSICS_TIMESTEP);
                physics_accumulation_timer -= PHYSICS_TIMESTEP;
            }

            physics_accumulation_timer += dt;
        }

        {
            update_all_particle_systems(game_state->loaded_level, dt);
        }
    }

    {
        test_emitter->x = test_emitter->x1 = player.x;
        test_emitter->y = test_emitter->y1 = player.y;
    }

    camera_set_focus_speed_zoom(&game_camera, 5);
    camera_set_focus_speed_x(&game_camera, 3);
    camera_set_focus_speed_y(&game_camera, 2);
    camera_set_bounds(&game_camera, game_state->loaded_level->bounds_min_x, game_state->loaded_level->bounds_min_y,
                      game_state->loaded_level->bounds_max_x, game_state->loaded_level->bounds_max_y);
    camera_set_focus_position(&game_camera, player.x - player.w/2, player.y - player.h/2);

    /* UI is a substate, we still draw the game under the UI, so it can look nice. */
    begin_graphics_frame(NULL); {
        draw_filled_rectangle(0, 0, 9999, 9999, active_colorscheme.secondary);
    } end_graphics_frame();
    begin_graphics_frame(&game_camera); {
        {
            struct tilemap* tilemap = game_state->loaded_level;
            draw_tiles(tilemap->background_tiles, tilemap->background_tile_count, active_colorscheme.primary_background);
        }

        {
            draw_filled_rectangle(player.x, player.y, player.w, player.h, active_colorscheme.primary);
            /* draw_texture(test_guy, player.x, player.y+player.h - (32.0f/16.0f), 16/16, (32.0f/16), active_colorscheme.primary); */
        }

        draw_all_particle_systems();
        {
            struct tilemap* tilemap = game_state->loaded_level;
            draw_tiles(tilemap->tiles, tilemap->height * tilemap->width, active_colorscheme.primary);
            draw_tiles(tilemap->foreground_tiles, tilemap->foreground_tile_count, active_colorscheme.primary_foreground);
        }

        DEBUG_draw_debug_stuff();
    } end_graphics_frame();

    /* draw and update UI */
    {
        switch (game_state->menu_mode) {
            case GAMEPLAY_UI_INGAME: {
                do_gameplay_ui(gamepad, dt);
            } break;
            case GAMEPLAY_UI_MAINMENU: {
                do_mainmenu_ui(gamepad, dt);
            } break;
            case GAMEPLAY_UI_PAUSEMENU: {
                do_pausemenu_ui(gamepad, dt);
            } break;
        }
    }
    DEBUG_draw_debug_ui_stuff();
}
