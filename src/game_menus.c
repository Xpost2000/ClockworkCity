/*
  Menu code or any menu system that comes out of this.

  Right now this is a minimal flashy menu, that might actually survive
  to ship considering how cool it looks.
*/

#define DYNAMIC_FONT_SIZE (0.5) /* big so it scales okayly? */
#define GAME_UI_TITLE_FONT_SIZE (font_size_aspect_ratio_independent(0.15))
#define GAME_UI_MENU_FONT_SIZE (font_size_aspect_ratio_independent(0.075))

#define QUIT_FADE_TIMER (0.35)
#define QUIT_FADE_TIMER2 (0.5)
#define INGAME_PAN_OUT_TIMER (0.25)
#define START_MENU_ALPHA (0.85)

enum gameplay_ui_mode {
    GAMEPLAY_UI_MAINMENU,
    GAMEPLAY_UI_PAUSEMENU,
    GAMEPLAY_UI_INGAME,
    GAMEPLAY_UI_LOAD_SAVE,
    GAMEPLAY_UI_OPTIONS,

    GAMEPLAY_UI_MODE_COUNT,
};

/* crufty animations */
enum gameplay_ui_transition {
    GAMEPLAY_UI_TRANSITION_NONE,
    GAMEPLAY_UI_TRANSITION_TO_QUIT,
    GAMEPLAY_UI_TRANSITION_TO_INGAME,
    GAMEPLAY_UI_TRANSITION_TO_PAUSE,
};

enum {
    MAINMENU_UI_PLAY_GAME,
    /* MAINMENU_UI_LOAD_GAME, */
    /* MAINMENU_UI_OPTIONS, */
    MAINMENU_UI_QUIT,
};
char* menu_option_strings[] = {
    "Play Game",
    /* "Load Game", */
    /* "Options", */
    "Quit",
};

char* menu_pause_option_strings[] = {
    "Resume Game",
    /* "Load Game", */
    /* "Options", */
    "Return To Main Menu",
    "Quit To Desktop",
};


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
            draw_text(game_title_font, x_cursor, y_cursor, "Mplusplus", active_colorscheme.text);
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
            draw_text(game_title_font, x_cursor, y_cursor, "Mplusplus", active_colorscheme.text);
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
