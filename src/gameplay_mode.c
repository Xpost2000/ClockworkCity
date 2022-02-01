#define QUIT_FADE_TIMER (0.35)
#define INGAME_PAN_OUT_TIMER (0.25)
#define START_MENU_ALPHA (0.85)

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

    if (is_key_down(KEY_ESCAPE) || (!gamepad->last_buttons[BUTTON_START] && gamepad->buttons[BUTTON_START])) {
        game_state->menu_mode = GAMEPLAY_UI_PAUSEMENU;
        game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_TO_PAUSE;
        game_state->ingame_transition_timer[0] = INGAME_PAN_OUT_TIMER;
        game_state->ingame_transition_timer[1] = INGAME_PAN_OUT_TIMER;
    }

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

local void do_gameplay_ui(struct game_controller* controller, float dt);
local void do_mainmenu_ui(struct game_controller* controller, float dt);
local void do_pausemenu_ui(struct game_controller* controller, float dt);
local void game_update_render_frame(float dt) {
    struct game_controller* gamepad = get_gamepad(0);

    if (game_state->menu_mode == GAMEPLAY_UI_INGAME) {
        do_player_input(dt);

        local float physics_accumulation_timer = 0;
        const int PHYSICS_FRAMERATE = 300;
        const float PHYSICS_TIMESTEP = 1.0f / (float)(PHYSICS_FRAMERATE);

        while (physics_accumulation_timer > 0.0f) {
            do_physics(PHYSICS_TIMESTEP);
            physics_accumulation_timer -= PHYSICS_TIMESTEP;
        }

        physics_accumulation_timer += dt;
    }

    /* UI is a substate, we still draw the game under the UI, so it can look nice. */
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
        camera_set_focus_position(player.x - player.w/2, player.y - player.h/2);

        draw_filled_rectangle(player.x, player.y, player.w, player.h, color4f(0.3, 0.2, 1.0, 1.0));
        draw_tilemap(game_state->loaded_level);
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
#if 0
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

local void do_gameplay_ui(struct game_controller* controller, float dt) {
    
}

/* nice fancy little immediate mode UI I guess. */
local void do_mainmenu_ui(struct game_controller* controller, float dt) {
    int dimens[2];
    get_screen_dimensions(dimens, dimens+1);

    if (game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_NONE) {
        bool previous_option = !controller->last_buttons[DPAD_UP] && controller->buttons[DPAD_UP] || controller->left_stick.axes[0] <= -0.3 || is_key_pressed(KEY_UP);
        bool next_option = !controller->last_buttons[DPAD_DOWN] && controller->buttons[DPAD_DOWN] || controller->left_stick.axes[0] >= 0.3 || is_key_pressed(KEY_DOWN);
        bool select_option = !controller->last_buttons[BUTTON_A] && controller->buttons[BUTTON_A] || is_key_pressed(KEY_RETURN);

        if (next_option) {
            game_state->selected_menu_option++;
        } else if (previous_option) {
            game_state->selected_menu_option--;
        }

        if (select_option) {
            switch (game_state->selected_menu_option) {
                case MAINMENU_UI_PLAY_GAME: {
                    game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_TO_INGAME;
                    game_state->ingame_transition_timer[0] = INGAME_PAN_OUT_TIMER;
                    game_state->ingame_transition_timer[1] = INGAME_PAN_OUT_TIMER;
                } break;
                /* case MAINMENU_UI_LOAD_GAME: { */
                /*     game_state->menu_mode = GAMEPLAY_UI_LOAD_SAVE; */
                /* } break; */
                /* case MAINMENU_UI_OPTIONS: { */
                /*     game_state->menu_mode = GAMEPLAY_UI_OPTIONS; */
                /* } break; */
                case MAINMENU_UI_QUIT: {
                    game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_TO_QUIT;
                    game_state->quit_transition_timer = QUIT_FADE_TIMER;
                } break;
            }
        }
    }

    if (game_state->selected_menu_option > array_count(menu_option_strings)-1) {
        game_state->selected_menu_option = array_count(menu_option_strings)-1;
    } else if (game_state->selected_menu_option < 0) {
        game_state->selected_menu_option = 0;
    }

    {
        float font_height = font_size_aspect_ratio_independent(DYNAMIC_FONT_SIZE);
        float x_cursor = 0;
        float alpha = START_MENU_ALPHA;

        if (game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_TO_INGAME) {
            /* stage 1 */
            if (game_state->ingame_transition_timer[0] > 0) {
                game_state->ingame_transition_timer[0] -= dt;
            } else {
                if (game_state->ingame_transition_timer[1] > 0) {
                    game_state->ingame_transition_timer[1] -= dt;
                } else {
                    game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_NONE;
                    game_state->menu_mode = GAMEPLAY_UI_INGAME;
                }
            }

            x_cursor = lerp(0, -dimens[0], 1.0 - game_state->ingame_transition_timer[0]/INGAME_PAN_OUT_TIMER);
            alpha = lerp(START_MENU_ALPHA, 0, 1.0 - game_state->ingame_transition_timer[1]/INGAME_PAN_OUT_TIMER);
        }

        begin_graphics_frame(); {
            draw_filled_rectangle(0, 0, dimens[0], dimens[1], color4f(0, 0, 0, alpha));
            float y_cursor = GAME_UI_TITLE_FONT_SIZE * 0.1;

            /* this part will be moved out of the way in the "ingame" transition code which is hardcoded. Looks good enough to
               pass as a basic menu system.*/
            draw_text(game_title_font, x_cursor, y_cursor, "Mplusplus", COLOR4F_WHITE);
            y_cursor += GAME_UI_TITLE_FONT_SIZE * 4.3;

            for (unsigned index = 0; index < array_count(menu_option_strings); ++index) {
                union color4f color = COLOR4F_WHITE;

                if (index == game_state->selected_menu_option) {
                    color = color4f(127/255.0f, 255/255.0f, 212/255.0f, 1.0f);
                }

                draw_text(game_ui_menu_font, x_cursor, y_cursor, menu_option_strings[index], color);
                y_cursor += GAME_UI_MENU_FONT_SIZE * 1.1;
            }

            if (game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_TO_QUIT) {
                draw_filled_rectangle(0, 0, dimens[0], dimens[1], color4f(0, 0, 0, 1.0 - game_state->quit_transition_timer/QUIT_FADE_TIMER));
                if (game_state->quit_transition_timer > 0) {
                    game_state->quit_transition_timer -= dt;
                } else {
                    running = false;
                    game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_NONE;
                }
            }

        } end_graphics_frame();
    }
}

/* TODO(jerry): make unique */
local void do_pausemenu_ui(struct game_controller* controller, float dt) {
    int dimens[2];
    get_screen_dimensions(dimens, dimens+1);

    if (game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_NONE) {
        bool previous_option = !controller->last_buttons[DPAD_UP] && controller->buttons[DPAD_UP] || controller->left_stick.axes[0] <= -0.3 || is_key_pressed(KEY_UP);
        bool next_option = !controller->last_buttons[DPAD_DOWN] && controller->buttons[DPAD_DOWN] || controller->left_stick.axes[0] >= 0.3 || is_key_pressed(KEY_DOWN);
        bool select_option = !controller->last_buttons[BUTTON_A] && controller->buttons[BUTTON_A] || is_key_pressed(KEY_RETURN);

        if (next_option) {
            game_state->selected_menu_option++;
        } else if (previous_option) {
            game_state->selected_menu_option--;
        }

        if (select_option) {
            switch (game_state->selected_menu_option) {
                case MAINMENU_UI_PLAY_GAME: {
                    game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_TO_INGAME;
                    game_state->ingame_transition_timer[0] = INGAME_PAN_OUT_TIMER;
                    game_state->ingame_transition_timer[1] = INGAME_PAN_OUT_TIMER;
                } break;
                /* case MAINMENU_UI_LOAD_GAME: { */
                /*     game_state->menu_mode = GAMEPLAY_UI_LOAD_SAVE; */
                /* } break; */
                /* case MAINMENU_UI_OPTIONS: { */
                /*     game_state->menu_mode = GAMEPLAY_UI_OPTIONS; */
                /* } break; */
                case (MAINMENU_UI_QUIT+1): {
                    game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_TO_QUIT;
                    game_state->quit_transition_timer = QUIT_FADE_TIMER;
                } break;
            }
        }
    }

    if (game_state->selected_menu_option > array_count(menu_pause_option_strings)-1) {
        game_state->selected_menu_option = array_count(menu_pause_option_strings)-1;
    } else if (game_state->selected_menu_option < 0) {
        game_state->selected_menu_option = 0;
    }

    {
        float font_height = font_size_aspect_ratio_independent(DYNAMIC_FONT_SIZE);
        float x_cursor = 0;
        float alpha = START_MENU_ALPHA;

        if (game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_TO_INGAME ||
            game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_TO_PAUSE) {
            /* stage 1 */
            if (game_state->ingame_transition_timer[0] > 0) {
                game_state->ingame_transition_timer[0] -= dt;
            } else {
                if (game_state->ingame_transition_timer[1] > 0) {
                    game_state->ingame_transition_timer[1] -= dt;
                } else {
                    if (game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_TO_INGAME) {
                        game_state->menu_mode = GAMEPLAY_UI_INGAME;
                        return;
                    }

                    game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_NONE;
                }
            }

            if (game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_TO_INGAME) {
                x_cursor = lerp(0, -dimens[0], 1.0 - game_state->ingame_transition_timer[0]/INGAME_PAN_OUT_TIMER);
                alpha = lerp(START_MENU_ALPHA, 0, 1.0 - game_state->ingame_transition_timer[1]/INGAME_PAN_OUT_TIMER);
            } else {
                x_cursor = lerp(-dimens[0], 0, 1.0 - game_state->ingame_transition_timer[0]/INGAME_PAN_OUT_TIMER);
                alpha = lerp(0, START_MENU_ALPHA, 1.0 - game_state->ingame_transition_timer[1]/INGAME_PAN_OUT_TIMER);
            }
        }

        begin_graphics_frame(); {
            draw_filled_rectangle(0, 0, dimens[0], dimens[1], color4f(0, 0, 0, alpha));
            float y_cursor = GAME_UI_TITLE_FONT_SIZE * 0.1;

            /* this part will be moved out of the way in the "ingame" transition code which is hardcoded. Looks good enough to
               pass as a basic menu system.*/
            draw_text(game_title_font, x_cursor, y_cursor, "Mplusplus", COLOR4F_WHITE);
            y_cursor += GAME_UI_TITLE_FONT_SIZE * 4.3;

            for (unsigned index = 0; index < array_count(menu_pause_option_strings); ++index) {
                union color4f color = COLOR4F_WHITE;

                if (index == game_state->selected_menu_option) {
                    color = color4f(127/255.0f, 255/255.0f, 212/255.0f, 1.0f);
                }

                draw_text(game_ui_menu_font, x_cursor, y_cursor, menu_pause_option_strings[index], color);
                y_cursor += GAME_UI_MENU_FONT_SIZE * 1.1;
            }

            if (game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_TO_QUIT) {
                draw_filled_rectangle(0, 0, dimens[0], dimens[1], color4f(0, 0, 0, 1.0 - game_state->quit_transition_timer/QUIT_FADE_TIMER));
                if (game_state->quit_transition_timer > 0) {
                    game_state->quit_transition_timer -= dt;
                } else {
                    running = false;
                    game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_NONE;
                }
            }

        } end_graphics_frame();
    }
}
