local void tilemap_editor_painting_camera_focus_zones(struct memory_arena* frame_arena, float dt) {
    int mouse_position[2];
    bool left_click, right_click;

    get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
    get_mouse_buttons(&left_click, 0, &right_click);

    begin_graphics_frame(&editor_camera); {
        /* cursor */
        {
            struct camera_focus_zone* already_selected = (struct camera_focus_zone*) editor.context;

            Generate_Rectangle_Drag_Or_Place_Code(already_selected, camera_focus_zone, 5, 5);
            Generate_Rectangle_Deletion_Or_Unselect_Code(camera_focus_zone);

            if (already_selected) draw_rectangle(already_selected->x, already_selected->y, already_selected->w, already_selected->h, color4f(0.2, 0.3, 1.0, 1.0));
        }
    } end_graphics_frame();

    /* editor ui */
    {
        struct camera_focus_zone* already_selected = (struct camera_focus_zone*) editor.context;
        if (already_selected)  {
            begin_graphics_frame(NULL); {
                float font_height = font_size_aspect_ratio_independent(0.03);
                int dimens[2];
                get_screen_dimensions(dimens, dimens+1);

                draw_text_right_justified(test_font, 0, 0, dimens[0],
                                          format_temp("FOCUS ZONE PROPERTIES\nNAME: \"%s\"\nX: %d\nY: %d\nW: %d\nH: %d\ntarget zoom: %f\ninterp speeds (%f, %f, %f)\n",
                                                      already_selected->identifier,
                                                      already_selected->x, already_selected->y,
                                                      already_selected->w, already_selected->h,
                                                      already_selected->zoom, already_selected->interpolation_speed[0], already_selected->interpolation_speed[1], already_selected->interpolation_speed[2]), COLOR4F_WHITE);
            } end_graphics_frame();

            Generate_Rectangle_Sizing_Code(already_selected);

            if (!is_editting_text()) {
                static char temporary_buffers[32][128] = {};

                if (is_key_pressed(KEY_1)) {
                    memset(temporary_buffers[0], 0, 128);
                    editor_open_text_edit_prompt("FOCUS ZONE ZOOM", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                }

                if (is_key_pressed(KEY_2)) {
                    memset(temporary_buffers[0], 0, 128);
                    editor_open_text_edit_prompt("FOCUS ZONE INTERPOLATION SPEED X", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                }

                if (is_key_pressed(KEY_3)) {
                    memset(temporary_buffers[0], 0, 128);
                    editor_open_text_edit_prompt("FOCUS ZONE INTERPOLATION SPEED Y", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                }

                if (is_key_pressed(KEY_4)) {
                    memset(temporary_buffers[0], 0, 128);
                    editor_open_text_edit_prompt("FOCUS ZONE INTERPOLATION SPEED ZOOM", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                }
            }
        }
    }
}
