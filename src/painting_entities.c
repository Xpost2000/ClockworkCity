local void tilemap_editor_painting_entities(struct memory_arena* frame_arena, float dt) {
    begin_graphics_frame(&editor_camera); {
        {
            int mouse_position[2];
            bool left_click, right_click;

            get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
            get_mouse_buttons(&left_click, 0, &right_click);

            struct entity_placement* already_selected = (struct entity_placement*) editor.context;

            /* cannot be deduplicated since width/height is *known*. */
            if (left_click) {
                if (already_selected) {
                    float entity_w;
                    float entity_h;

                    entity_get_type_dimensions(already_selected->type, &entity_w, &entity_h);

                    if (!editor.dragging &&
                        rectangle_overlapping_v(already_selected->x, already_selected->y,
                                                entity_w, entity_h, mouse_position[0], mouse_position[1], 1, 1)) {
                        editor.dragging = true;
                    } else if (editor.dragging) {
                        already_selected->x = mouse_position[0];
                        already_selected->y = mouse_position[1];
                    }
                } else {
                    struct entity_placement* entity = editor_existing_entity_at(editor.tilemap.entity_placements, editor.tilemap.entity_placement_count, mouse_position[0], mouse_position[1]);

                    if (!entity) {
                        entity       = editor_allocate_entity_placement();
                        entity->x    = mouse_position[0];
                        entity->y    = mouse_position[1];
                        entity->type = editor.painting_entity_type;
                    }

                    editor.context = entity;
                }
            } else {
                editor.dragging = false;
            }

            Generate_Rectangle_Deletion_Or_Unselect_Code(entity_placement);

#if 0
            /*
              If I were to do this again, if I REALLY didn't want to use a IMGUI style thing... I could've just done this for the UI without
              making this a complete PITA
              
              It's still a form of IMGUI but whatever. It's basically a waste of time to do this now...
             */

            /* fill in property_type */
            struct property {
                enum property_type type;
                char* name;

                bool editable;
                union {
                    void*     generic;

                    int16_t*  as_int16;
                    uint16_t* as_uint16;
                    int32_t*  as_int32;
                    uint32_t* as_uint32;
                    int64_t*  as_int64;
                    uint64_t* as_uint64;
                    float*    as_float;
                    struct {
                        uint32_t max_capacity;
                        char* string_ptr;
                    } as_string;
                };
            };
            struct property_editor {
                uint32_t        property_count;
                struct property properties[512];
                int selected_property;
            };

            /* this is done every frame... */
            void push_editable_property(struct property_editor* editor, char* name, enum property_type type, void* ptr) {
                /* ... */
            }

            void do_property_editor(struct property_editor* editor, float dt) {
                for (each property of editor->properties) {
                    if (property_index == editor->selected_property) {
                        if (is_key_pressed(KEY_RETURN) && property->editable) {
                            open_text_edit_prompt_for_editor... /* This would actually make this crap not be completely horrible per say.... */
                        }
                    }
                    /* of course draw them */
                    char* print_string = format_temp(...);
                } 
            }

            /* I recognize I basically just pseudo coded it here. While I would love to have this, I don't feel like rewriting anything... Just keep it in mind... */
#endif

            if (already_selected) {
                if (!is_editting_text()) {
                    if (is_key_down(KEY_1)) {
                        editor_open_text_edit_prompt("SET NAME", already_selected->identifier, ENTITY_STRING_IDENTIFIER_MAX_LENGTH, strlen(already_selected->identifier));
                    }
                }
                
                {
                    float entity_w;
                    float entity_h;

                    entity_get_type_dimensions(already_selected->type, &entity_w, &entity_h);
                    draw_rectangle(already_selected->x, already_selected->y, entity_w, entity_h, color4f(0.2, 0.3, 1.0, 1.0));
                }
            }
        }
    } end_graphics_frame();

    if (is_key_pressed(KEY_UP)) {
        editor.painting_entity_type++;
    } else if (is_key_pressed(KEY_DOWN)) {
        editor.painting_entity_type--;
    }

    /* disallow placing multiple players. */
    editor.painting_entity_type = clampi(editor.painting_entity_type, ENTITY_TYPE_PLAYER+1, ENTITY_TYPE_LAST_SPAWNABLE_ENTITY_TYPE-1);

    {
        struct entity_placement* already_selected = (struct entity_placement*) editor.context;
        if (already_selected) {
            if (already_selected->type != editor.painting_entity_type) {
                already_selected->type = editor.painting_entity_type;
            }
        }
    }

    begin_graphics_frame(NULL); {
        {
            draw_text(test_font, 0, 0,
                      format_temp("entities present: %d\nplacement type: %s\n",
                                  editor.tilemap.entity_placement_count,
                                  entity_type_strings[editor.painting_entity_type]),
                      active_colorscheme.text);
        }
        {
            struct entity_placement* already_selected = (struct entity_placement*) editor.context;

            if (already_selected) {
                float font_height = font_size_aspect_ratio_independent(0.03);
                int dimens[2];
                get_screen_dimensions(dimens, dimens+1);

                draw_text_right_justified(test_font, 0, 0, dimens[0],
                                          format_temp("ENTITY PROPERTIES\nNAME: \"%s\"\nX: %f\nY: %f\nTYPE: %d (%s)\nFLAGS(RAW): %d\nFACING DIR: %s\n",
                                                      already_selected->identifier,
                                                      already_selected->x,
                                                      already_selected->y,
                                                      already_selected->type, entity_type_strings[already_selected->type],
                                                      already_selected->flags,
                                                      get_facing_direction_string(already_selected->facing_direction)),
                                          COLOR4F_WHITE);
            }
        }
    } end_graphics_frame();
}
