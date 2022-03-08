local void tilemap_editor_painting_doors(struct memory_arena* frame_arena, float dt) {
    int mouse_position[2];
    bool left_click, right_click;

    get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
    get_mouse_buttons(&left_click, 0, &right_click);

    begin_graphics_frame(&editor_camera); {
        /* cursor */
        {
            struct door* already_selected = (struct door*) editor.context;

            Generate_Rectangle_Drag_Or_Place_Code(already_selected, door, 1, 2);
            Generate_Rectangle_Deletion_Or_Unselect_Code(door);

            if (already_selected) draw_rectangle(already_selected->x, already_selected->y, already_selected->w, already_selected->h, color4f(0.2, 0.3, 1.0, 1.0));
        }
    } end_graphics_frame();

    /* editor ui */
    {
        struct door* already_selected = (struct door*) editor.context;
        if (already_selected)  {
            begin_graphics_frame(NULL); {
                float font_height = font_size_aspect_ratio_independent(0.03);
                int dimens[2];
                get_screen_dimensions(dimens, dimens+1);

                const int lines = 7; /* manually counted for layout reasons... */
                draw_text_right_justified(test_font, 0, 0, dimens[0],
                                          format_temp("DOOR PROPERTIES\nNAME: \"%s\"\nX: %d\nY: %d\nW: %d\nH: %d\listens to boss_death?: %s\nhorizontal: %s\n",
                                                      already_selected->identifier,
                                                      already_selected->x, already_selected->y,
                                                      already_selected->w, already_selected->h,
                                                      yesno[already_selected->listens_for_boss_death], yesno[already_selected->horizontal]), COLOR4F_WHITE);
            } end_graphics_frame();

            Generate_Rectangle_Sizing_Code(already_selected);

            if (!is_editting_text()) {
                static char temporary_buffers[32][128] = {};
                if (is_key_pressed(KEY_1)) {
                    Toggle_Boolean(already_selected->listens_for_boss_death);
                }

                if (is_key_pressed(KEY_2)) {
                    Toggle_Boolean(already_selected->horizontal);
                }

                if (is_key_pressed(KEY_3)) {
                    memset(temporary_buffers[0], 0, 128);
                    editor_open_text_edit_prompt("SET BOSS TYPE ID", temporary_buffers[0], 128, strlen(temporary_buffers[0]));
                }
            }
        }
    }
}
