local void activation_target_cycle_through_types(struct activation_switch_target* target) {
    target->type++;
    target->type %= ACTIVATION_TARGET_TYPE_COUNT;
}

local void tilemap_editor_painting_activators(struct memory_arena* frame_arena, float dt) {
    int mouse_position[2];
    bool left_click, right_click;

    get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
    get_mouse_buttons(&left_click, 0, &right_click);

    begin_graphics_frame(&editor_camera); {
        /* cursor */
        {
            struct activation_switch* already_selected = (struct activation_switch*) editor.context;

            Generate_Rectangle_Drag_Or_Place_Code(already_selected, activation_switch, 5, 5);
            Generate_Rectangle_Deletion_Or_Unselect_Code(activation_switch);

            if(already_selected) {
                already_selected->w = 1; already_selected->h = 1;
            }
            
            if (already_selected) draw_rectangle(already_selected->x, already_selected->y, already_selected->w, already_selected->h, color4f(0.2, 0.3, 1.0, 1.0));
        }
    } end_graphics_frame();

    /* editor ui */
    {
        struct activation_switch* already_selected = (struct activation_switch*) editor.context;
        if (already_selected)  {
            begin_graphics_frame(NULL); {
                float font_height = font_size_aspect_ratio_independent(0.03);
                int dimens[2];
                get_screen_dimensions(dimens, dimens+1);

                const int lines = 7; /* manually counted for layout reasons... */
#define target_line(i) activation_target_strings[already_selected->targets[i].type], already_selected->targets[i].identifier
                draw_text_right_justified(test_font, 0, 0, dimens[0],
                                          format_temp("LEVER PROPERTIES\nNAME: \"%s\"\nX: %d\nY: %d\nW: %d\nH: %d\n"
                                                      /* evil */
                                                      "TARGET1(%s): %s\n"
                                                      "TARGET2(%s): %s\n"
                                                      "TARGET3(%s): %s\n"
                                                      "TARGET4(%s): %s\n"
                                                      "TARGET5(%s): %s\n"
                                                      "TARGET6(%s): %s\n"
                                                      "TARGET7(%s): %s\n"
                                                      "TARGET8(%s): %s\n"
                                                      "TARGET9(%s): %s\n"
                                                      "TARGET10(%s): %s\nonce_only? : %s\n",
                                                      already_selected->identifier,
                                                      already_selected->x, already_selected->y,
                                                      already_selected->w, already_selected->h,
                                                      target_line(0),
                                                      target_line(1),
                                                      target_line(2),
                                                      target_line(3),
                                                      target_line(4),
                                                      target_line(5),
                                                      target_line(6),
                                                      target_line(7),
                                                      target_line(8),
                                                      target_line(9),
                                                      yesno[already_selected->once_only]
                                          ), COLOR4F_WHITE);
#undef target_line
            } end_graphics_frame();

            Generate_Rectangle_Sizing_Code(already_selected);

            for (unsigned index = 0; index < 10; ++index) {
                struct activation_switch_target* target = already_selected->targets + index;
                if (target->type >= ACTIVATION_TARGET_TYPE_COUNT) target->type = ACTIVATION_TARGET_TYPE_COUNT-1;
            }

            if (!is_editting_text()) {
                static char temporary_buffers[32][128] = {};
                if (is_key_down(KEY_SHIFT)) {
                    if (is_key_pressed(KEY_1)) {
                        activation_target_cycle_through_types(&already_selected->targets[0]);
                    }
                    if (is_key_pressed(KEY_2)) {
                        activation_target_cycle_through_types(&already_selected->targets[1]);
                    }
                    if (is_key_pressed(KEY_3)) {
                        activation_target_cycle_through_types(&already_selected->targets[2]);
                    }
                    if (is_key_pressed(KEY_4)) {
                        activation_target_cycle_through_types(&already_selected->targets[3]);
                    }
                    if (is_key_pressed(KEY_5)) {
                        activation_target_cycle_through_types(&already_selected->targets[4]);
                    }
                    if (is_key_pressed(KEY_6)) {
                        activation_target_cycle_through_types(&already_selected->targets[5]);
                    }
                    if (is_key_pressed(KEY_7)) {
                        activation_target_cycle_through_types(&already_selected->targets[6]);
                    }
                    if (is_key_pressed(KEY_8)) {
                        activation_target_cycle_through_types(&already_selected->targets[7]);
                    }
                    if (is_key_pressed(KEY_9)) {
                        activation_target_cycle_through_types(&already_selected->targets[8]);
                    }
                    if (is_key_pressed(KEY_0)) {
                        activation_target_cycle_through_types(&already_selected->targets[9]);
                    }
                } else {
                    if (is_key_pressed(KEY_1)) {
                        if (is_key_down(KEY_SPACE)) {
                            Toggle_Boolean(already_selected->once_only);
                        } else {
                            memset(temporary_buffers[0], 0, 128);
                            editor_open_text_edit_prompt("set target1 name", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                        }
                    }
                    if (is_key_pressed(KEY_2)) {
                        memset(temporary_buffers[0], 0, 128);
                        editor_open_text_edit_prompt("set target2 name", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                    }
                    if (is_key_pressed(KEY_3)) {
                        memset(temporary_buffers[0], 0, 128);
                        editor_open_text_edit_prompt("set target3 name", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                    }
                    if (is_key_pressed(KEY_4)) {
                        memset(temporary_buffers[0], 0, 128);
                        editor_open_text_edit_prompt("set target4 name", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                    }
                    if (is_key_pressed(KEY_5)) {
                        memset(temporary_buffers[0], 0, 128);
                        editor_open_text_edit_prompt("set target5 name", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                    }
                    if (is_key_pressed(KEY_6)) {
                        memset(temporary_buffers[0], 0, 128);
                        editor_open_text_edit_prompt("set target6 name", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                    }
                    if (is_key_pressed(KEY_7)) {
                        memset(temporary_buffers[0], 0, 128);
                        editor_open_text_edit_prompt("set target7 name", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                    }
                    if (is_key_pressed(KEY_8)) {
                        memset(temporary_buffers[0], 0, 128);
                        editor_open_text_edit_prompt("set target8 name", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                    }
                    if (is_key_pressed(KEY_9)) {
                        memset(temporary_buffers[0], 0, 128);
                        editor_open_text_edit_prompt("set target9 name", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                    }
                    if (is_key_pressed(KEY_0)) {
                        memset(temporary_buffers[0], 0, 128);
                        editor_open_text_edit_prompt("set target10 name", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                    }
                }
            }
        }
    }
}
