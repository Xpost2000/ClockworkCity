/*
  Menu code or any menu system that comes out of this.

  Right now this is a minimal flashy menu, that might actually survive
  to ship considering how cool it looks.
*/
#define GAME_TITLE_CSTR "ASCENSION"

local void do_gameplay_ui(struct game_controller* controller, float dt) {
    struct entity* player = &game_state->persistent_entities[0];
    {
        float square_size = font_size_aspect_ratio_independent(0.075);
        float x_cursor = square_size * 1.2;

        begin_graphics_frame(0); {
            /* should be active colorshceme later */
#ifdef FORFRIENDS_DEMO
            {
                struct entity* players[4] = {
                    &game_state->persistent_entities[0],
                    &game_state->persistent_entities[1],
                    &game_state->persistent_entities[2],
                    &game_state->persistent_entities[3],
                };
                for (unsigned k = 0; k < array_count(players); ++k) {
                    if (players[k]->flags & ENTITY_FLAGS_DISABLED) continue;

                    x_cursor = square_size * 1.2;
                    for (int i = 0; i < players[k]->health; ++i) {
                        draw_texture(ui_health_slice, x_cursor, square_size * (1+k) + ( normalized_sinf(global_elapsed_time * (i+1)) * (square_size/4)), square_size, square_size, active_colorscheme.text);
                        x_cursor += square_size;
                    }
                }
            }
#else
            {
                for (int i = 0; i < player->health; ++i) {
                    draw_texture(ui_health_slice, x_cursor, square_size + ( normalized_sinf(global_elapsed_time * (i+1)) * (square_size/4)), square_size, square_size, active_colorscheme.text);
                    x_cursor += square_size;
                }
            }
#endif

            /* TODO(jerry): Signal generic prompt system. */
            {
                const float REST_PROMPT_MAX_TIME_FADE = 0.6;
                union color4f color = active_colorscheme.text;
                float alpha;

                alpha = game_state->rest_prompt.t / REST_PROMPT_MAX_TIME_FADE;
                if (game_state->rest_prompt.active) {
                    game_state->rest_prompt.t += dt;
                    if (game_state->rest_prompt.t >= REST_PROMPT_MAX_TIME_FADE) game_state->rest_prompt.t = REST_PROMPT_MAX_TIME_FADE;
                } else {
                    game_state->rest_prompt.t -= dt;
                    if (game_state->rest_prompt.t <= 0) game_state->rest_prompt.t = 0;
                }

                color.a = alpha;

                if (game_state->rest_prompt.t != 0) {
                    char* cstr = "Bind to Soul Anchor";
                    if (game_state->rest_prompt.anchor && game_state->rest_prompt.anchor->unlocked) {
                        cstr = "Align to Soul Anchor";
                    }
                    
                    int screen_dimens[2];
                    get_screen_dimensions(screen_dimens, screen_dimens+1);
                    int text_dimens[2];
                    get_text_dimensions(game_ui_menu_font, cstr, text_dimens, text_dimens+1);

                    draw_text(game_ui_menu_font, screen_dimens[0]/2 - text_dimens[0]/2, screen_dimens[1] * 0.8 - text_dimens[1], cstr, color);
                }
            }
        } end_graphics_frame();
    }
    update_render_game_prompt(controller, dt);
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
                case (MAINMENU_UI_QUIT-1): {
                    game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_TO_QUIT;
                    game_state->quit_transition_timer[0] = QUIT_FADE_TIMER;
                    game_state->quit_transition_timer[1] = QUIT_FADE_TIMER2;
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

            x_cursor = lerp(0, -dimens[0], clampf(1.0 - game_state->ingame_transition_timer[0]/INGAME_PAN_OUT_TIMER, 0, 1));
            alpha    = lerp(START_MENU_ALPHA, 0, clampf(1.0 - game_state->ingame_transition_timer[1]/INGAME_PAN_OUT_TIMER, 0, 1));
        }

        begin_graphics_frame(NULL); {
            draw_filled_rectangle(0, 0, dimens[0], dimens[1], color4f(0, 0, 0, alpha));
            float y_cursor = GAME_UI_TITLE_FONT_SIZE * 0.1;

            /* this part will be moved out of the way in the "ingame" transition code which is hardcoded. Looks good enough to
               pass as a basic menu system.*/
            draw_text(game_title_font, x_cursor, y_cursor, GAME_TITLE_CSTR, active_colorscheme.text);
            y_cursor += GAME_UI_TITLE_FONT_SIZE * 4.3;

            for (unsigned index = 0; index < array_count(menu_option_strings); ++index) {
                union color4f color = active_colorscheme.text;

                if (index == game_state->selected_menu_option) {
                    float brightness = clampf(normalized_sinf(global_elapsed_time * 3.15) + 0.5, 0, 1.0);
                    color = color4f((127/255.0f) * brightness, (255/255.0f) * brightness, (212/255.0f) * brightness, 1.0f);
                }

                draw_text(game_ui_menu_font, x_cursor, y_cursor, menu_option_strings[index], color);
                y_cursor += GAME_UI_MENU_FONT_SIZE * 1.1;
            }

            if (game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_TO_QUIT) {
                draw_filled_rectangle(0, 0, dimens[0], dimens[1], color4f(0, 0, 0, 1.0 - game_state->quit_transition_timer[0]/QUIT_FADE_TIMER));
                if (game_state->quit_transition_timer[0] > 0) {
                    game_state->quit_transition_timer[0] -= dt;
                } else {
                    if (game_state->quit_transition_timer[0] > 0) {
                        game_state->quit_transition_timer[0] -= dt;
                    } else {
                        if (game_state->quit_transition_timer[1] > 0) {
                            game_state->quit_transition_timer[1] -= dt;
                            set_window_transparency(game_state->quit_transition_timer[1] / QUIT_FADE_TIMER2);
                        } else {
                            running = false;
                            game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_NONE;
                        }
                    }
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
                case MAINMENU_UI_RETURN_TO_MENU: {
                    /* no transition FUCK */
                    game_state->menu_mode = GAMEPLAY_UI_MAINMENU;
                    console_execute_cstr("load limbo1");
                } break;
                /* case MAINMENU_UI_LOAD_GAME: { */
                /*     game_state->menu_mode = GAMEPLAY_UI_LOAD_SAVE; */
                /* } break; */
                /* case MAINMENU_UI_OPTIONS: { */
                /*     game_state->menu_mode = GAMEPLAY_UI_OPTIONS; */
                /* } break; */
                case (MAINMENU_UI_QUIT+1): {
                    game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_TO_QUIT;
                    game_state->quit_transition_timer[0] = QUIT_FADE_TIMER;
                    game_state->quit_transition_timer[1] = QUIT_FADE_TIMER2;
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
                x_cursor = lerp(0, -dimens[0], clampf(1.0 - game_state->ingame_transition_timer[0]/INGAME_PAN_OUT_TIMER, 0, 1));
                alpha    = lerp(START_MENU_ALPHA, 0, clampf(1.0 - game_state->ingame_transition_timer[1]/INGAME_PAN_OUT_TIMER, 0, 1));
            } else {
                x_cursor = lerp(-dimens[0], 0, clampf(1.0 - game_state->ingame_transition_timer[0]/INGAME_PAN_OUT_TIMER, 0, 1));
                alpha    = lerp(0, START_MENU_ALPHA, clampf(1.0 - game_state->ingame_transition_timer[1]/INGAME_PAN_OUT_TIMER, 0, 1));
            }
        }

        begin_graphics_frame(NULL); {
            draw_filled_rectangle(0, 0, dimens[0], dimens[1], color4f(0, 0, 0, alpha));
            float y_cursor = GAME_UI_TITLE_FONT_SIZE * 0.1;

            /* this part will be moved out of the way in the "ingame" transition code which is hardcoded. Looks good enough to
               pass as a basic menu system.*/
            draw_text(game_title_font, x_cursor, y_cursor, GAME_TITLE_CSTR, active_colorscheme.text);
            y_cursor += GAME_UI_TITLE_FONT_SIZE * 4.3;

            for (unsigned index = 0; index < array_count(menu_pause_option_strings); ++index) {
                union color4f color = active_colorscheme.text;

                if (index == game_state->selected_menu_option) {
                    float brightness = clampf(normalized_sinf(global_elapsed_time * 3.15) + 0.5, 0, 1.0);
                    color = color4f((127/255.0f) * brightness, (255/255.0f) * brightness, (212/255.0f) * brightness, 1.0f);
                }

                draw_text(game_ui_menu_font, x_cursor, y_cursor, menu_pause_option_strings[index], color);
                y_cursor += GAME_UI_MENU_FONT_SIZE * 1.1;
            }

            if (game_state->menu_transition_state == GAMEPLAY_UI_TRANSITION_TO_QUIT) {
                draw_filled_rectangle(0, 0, dimens[0], dimens[1], color4f(0, 0, 0, 1.0 - game_state->quit_transition_timer[0]/QUIT_FADE_TIMER));
                if (game_state->quit_transition_timer[0] > 0) {
                    game_state->quit_transition_timer[0] -= dt;
                } else {
                    if (game_state->quit_transition_timer[1] > 0) {
                        game_state->quit_transition_timer[1] -= dt;
                        set_window_transparency(game_state->quit_transition_timer[1] / QUIT_FADE_TIMER2);
                    } else {
                        running = false;
                        game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_NONE;
                    }
                }
            }

        } end_graphics_frame();
    }
}
