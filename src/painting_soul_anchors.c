
local void tilemap_editor_painting_soul_anchors(struct memory_arena* frame_arena, float dt) {
    begin_graphics_frame(&editor_camera); {
        {
            int mouse_position[2];
            bool left_click, right_click;

            get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
            get_mouse_buttons(&left_click, 0, &right_click);

            struct soul_anchor* already_selected = (struct soul_anchor*) editor.context;

            /* cannot be deduplicated since width/height is *known*. */
            if (left_click) {
                if (already_selected) {
                    float entity_w = 1;
                    float entity_h = 2;

                    if (!editor.dragging &&
                        rectangle_overlapping_v(already_selected->x, already_selected->y,
                                                entity_w, entity_h, mouse_position[0], mouse_position[1], 1, 1)) {
                        editor.dragging = true;
                    } else if (editor.dragging) {
                        already_selected->x = mouse_position[0];
                        already_selected->y = mouse_position[1];
                    }
                } else {
                    struct soul_anchor* entity = editor_existing_soul_anchor_at(editor.tilemap.soul_anchors, editor.tilemap.entity_placement_count, mouse_position[0], mouse_position[1]);

                    if (!entity) {
                        entity       = editor_allocate_soul_anchor();
                        entity->x    = mouse_position[0];
                        entity->y    = mouse_position[1];
                    }

                    editor.context = entity;
                }
            } else {
                editor.dragging = false;
            }

            Generate_Rectangle_Deletion_Or_Unselect_Code(soul_anchor);

            if (already_selected) {
                {
                    float entity_w = 1;
                    float entity_h = 2;

                    draw_rectangle(already_selected->x, already_selected->y, entity_w, entity_h, color4f(0.2, 0.3, 1.0, 1.0));
                }
            }
        }
    } end_graphics_frame();

    begin_graphics_frame(NULL); {
        {
            draw_text(test_font, 0, 0, format_temp("soul anchors present: %d\n", editor.tilemap.soul_anchor_count), active_colorscheme.text);
        }
        {
            struct soul_anchor* already_selected = (struct soul_anchor*) editor.context;

            if (already_selected) {
                float font_height = font_size_aspect_ratio_independent(0.03);
                int dimens[2];
                get_screen_dimensions(dimens, dimens+1);

                draw_text_right_justified(test_font, 0, 0, dimens[0],
                                          format_temp("SOUL ANCHOR PROPERTIES\nNAME: \"%s\"\nX: %d\nY: %d\n",
                                                      already_selected->identifier,
                                                      already_selected->x,
                                                      already_selected->y),
                                          COLOR4F_WHITE);

                if (!is_editting_text()) {
                    if (is_key_down(KEY_1)) {
                        editor_open_text_edit_prompt("SET NAME", already_selected->identifier, SOUL_ANCHOR_IDENTIFIER_STRING_LENGTH, strlen(already_selected->identifier));
                    }
                }
            }
        }
    } end_graphics_frame();
}
