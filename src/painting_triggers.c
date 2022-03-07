local void tilemap_editor_painting_triggers(struct memory_arena* frame_arena, float dt) {
    int mouse_position[2];
    bool left_click, right_click;

    get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
    get_mouse_buttons(&left_click, 0, &right_click);

    begin_graphics_frame(&editor_camera); {
        /* cursor */
        {
            struct trigger* already_selected = (struct trigger*) editor.context;

            Generate_Rectangle_Drag_Or_Place_Code(already_selected, trigger, 5, 5);
            Generate_Rectangle_Deletion_Or_Unselect_Code(trigger);

            if (already_selected) draw_rectangle(already_selected->x, already_selected->y, already_selected->w, already_selected->h, color4f(0.2, 0.3, 1.0, 1.0));
        }
    } end_graphics_frame();

    /* editor ui */
    {
        struct trigger* already_selected = (struct trigger*) editor.context;
        if (already_selected)  {
            begin_graphics_frame(NULL); {
                float font_height = font_size_aspect_ratio_independent(0.03);
                int dimens[2];
                get_screen_dimensions(dimens, dimens+1);

                const int lines = 7; /* manually counted for layout reasons... */
                draw_text_right_justified(test_font, 0, 0, dimens[0],
                                          format_temp("TRIGGER PROPERTIES(TYPE: %s)\nNAME: \"%s\"\nX: %d\nY: %d\nW: %d\nH: %d\nonce only?: %s\n",
                                                      trigger_type_strings[already_selected->type],
                                                      already_selected->identifier,
                                                      already_selected->x, already_selected->y,
                                                      already_selected->w, already_selected->h, yesno[already_selected->once_only]), COLOR4F_WHITE);
                switch (already_selected->type) {
                    case TRIGGER_TYPE_PROMPT: {
                        draw_text_right_justified(test_font, 0, (lines+1) * font_height, dimens[0],
                                                  format_temp("prompt id: %d\n", already_selected->params[0]),
                                                  COLOR4F_WHITE);
                    } break;
                    case TRIGGER_TYPE_SPECIAL_EVENT: {
                        /* ??? Needed? */
                    } break;
                }
            } end_graphics_frame();

            if (is_key_pressed(KEY_TAB)) {
                already_selected->type += 1;
                already_selected->type %= TRIGGER_TYPE_COUNT;
            }

            Generate_Rectangle_Sizing_Code(already_selected);

            if (!is_editting_text()) {
                static char temporary_buffers[32][128] = {};

                switch (already_selected->type) {
                    case TRIGGER_TYPE_PROMPT: {
                        if (is_key_pressed(KEY_1)) {
                            Toggle_Boolean(already_selected->once_only);
                        }

                        if (is_key_pressed(KEY_2)) {
                            memset(temporary_buffers[0], 0, 128);
                            editor_open_text_edit_prompt("SET PROMPT ID", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                        }
                    } break;
                    case TRIGGER_TYPE_SPECIAL_EVENT: {
                        /* ??? Needed? */
                    } break;
                }
            }
        }
    }
}
