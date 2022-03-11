local void load_gameplay_resources(void) {
    load_all_tile_assets();
    load_all_resources_for_prompts();
}

local void gameplay_initialize(void) {
    game_state_add_persistent_entity(game_state, construct_entity_of_type(ENTITY_TYPE_PLAYER, 0, 0));
#ifdef FORFRIENDS_DEMO
    game_state_add_persistent_entity(game_state, construct_entity_of_type(ENTITY_TYPE_PLAYER, 0, 0));
    game_state->persistent_entities[1].flags |= ENTITY_FLAGS_DISABLED;
    game_state_add_persistent_entity(game_state, construct_entity_of_type(ENTITY_TYPE_PLAYER, 0, 0));
    game_state->persistent_entities[2].flags |= ENTITY_FLAGS_DISABLED;
    game_state_add_persistent_entity(game_state, construct_entity_of_type(ENTITY_TYPE_PLAYER, 0, 0));
    game_state->persistent_entities[3].flags |= ENTITY_FLAGS_DISABLED;
#endif
}

local void DEBUG_draw_debug_stuff(void) {
#ifdef DEV
    /*add debug rendering code here*/

    /*boundaries of the loaded world*/
    {
        struct tilemap* tilemap = game_state->loaded_level;
        float bounds_width  = tilemap->bounds_max_x - tilemap->bounds_min_x;
        float bounds_height = tilemap->bounds_max_y - tilemap->bounds_min_y;

        draw_rectangle(tilemap->bounds_min_x, tilemap->bounds_min_y,
                       bounds_width, bounds_height, COLOR4F_BLUE);

        if (game_state->have_a_good_grounded_position) {
            draw_rectangle(game_state->last_good_grounded_position_x, game_state->last_good_grounded_position_y, 0.5, 1.5, COLOR4F_RED);
        }
    }

    /* hitboxes */
    {
        DEBUG_draw_all_hitboxes();
    }

    /* Camera bounds */
    #if 0
    {
        struct camera* camera = get_global_camera();
        struct rectangle bounds = camera_get_bounds(camera);

        draw_rectangle(bounds.x, bounds.y, bounds.w, bounds.h, COLOR4F_BLUE);
    }
    #endif
#endif
}

local void DEBUG_draw_debug_ui_stuff(void) {
#ifdef DEV
    begin_graphics_frame(NULL); {
        {
            struct entity* player = &game_state->persistent_entities[0];
            struct memory_arena* arena = &game_memory_arena;
            size_t memusage = memory_arena_total_usage(arena);
            char* arena_msg = format_temp("(memory arena \"%s\") is using %d bytes\n(%d kb)\n(%d mb)\n(%d gb)\n(onground: %d)\n(player %f, %f, %f, %f lastvy: %f, \ndeathstate: %d, facing_dir: %d\n)(%s anim state)\n",
                                          arena->name, memusage, memusage / 1024, memusage / (1024 * 1024), memusage / (1024*1024*1024), player->onground, player->x, player->y, player->vx, player->vy, player->last_vy, player->death_state, player->facing_dir, game_animation_id_strings[animation_id]);
            draw_text(_console_font, 0, 0, arena_msg, COLOR4F_GREEN);
        }
    } end_graphics_frame();
#endif
}

local void try_and_record_player_grounded_position(float dt) {
    /* 
       This should really just be based off of global elapsed time 
       so we can just eliminate the parameter, but out of habit I do it in terms
       of dt when I actually don't need to in this case.
    */
    struct entity* player = &game_state->persistent_entities[0];

    if (player->onground) {
        if (game_state->last_good_grounded_position_recording_timer <= 0) {
            /* TODO(jerry):
               I should scan surrounding area for a better position
               If it's an edge then it has to be special cased. Otherwise we can do whatever we want really.
             */
            game_state->last_good_grounded_position_x = (int32_t)floorf(player->x);
            game_state->last_good_grounded_position_y = (int32_t)floorf(player->y);
            game_state->last_good_grounded_position_recording_timer = LAST_GROUNDED_POSITION_RECORD_TIMER_MAX;
            game_state->have_a_good_grounded_position = true;
        } else {
            game_state->last_good_grounded_position_recording_timer -= dt;
        }
    } else {
        game_state->last_good_grounded_position_recording_timer = 0;
    }
}

void restore_player_to_last_good_grounded(struct entity* player) {
    if (!game_state->have_a_good_grounded_position)
        return;

    player->vx       = 0;
    player->vy       = 0;
    player->onground = true;
    player->x        = game_state->last_good_grounded_position_x;
    player->y        = game_state->last_good_grounded_position_y;
    game_state->last_good_grounded_position_recording_timer = 0;
}

#include "game_animations.c"

local void game_update_render_frame(float dt) {
    struct game_controller* gamepad = get_gamepad(0);
    struct entity* player = &game_state->persistent_entities[0];
    struct tilemap* tilemap = game_state->loaded_level;

    float physics_interpolation_value;
    float particle_interpolation_value;

    if (game_state->menu_mode == GAMEPLAY_UI_INGAME) {
        {
            local float physics_accumulation_timer = 0;
            const int PHYSICS_FRAMERATE = 250;
            const float PHYSICS_TIMESTEP = 1.0f / (float)(PHYSICS_FRAMERATE);

            struct entity_iterator entities = game_state_entity_iterator(game_state);

            physics_accumulation_timer += dt * game_timescale;
            while (physics_accumulation_timer >= PHYSICS_TIMESTEP) {
                // Interesting...

                do_entity_physics_updates(&entities, tilemap, PHYSICS_TIMESTEP);
                physics_accumulation_timer -= PHYSICS_TIMESTEP;
            }

            physics_interpolation_value = physics_accumulation_timer / PHYSICS_TIMESTEP;
        }
        {
            struct entity_iterator entities = game_state_entity_iterator(game_state);
            do_entity_updates(&entities, tilemap, dt * game_timescale);
        }
        {
            struct entity_iterator entities = game_state_entity_iterator(game_state);
            update_all_hitboxes(&entities, tilemap, dt);
        }
        {
            update_all_doors(tilemap, dt);
        }
        {
            local float particle_accumulation_timer = 0;
            const int PARTICLES_FRAMERATE = 30;
            const float PARTICLES_TIMESTEP = 1.0f / (float)(PARTICLES_FRAMERATE);

            particle_accumulation_timer += dt * game_timescale;
            while (particle_accumulation_timer >= PARTICLES_TIMESTEP) {
                update_all_particle_systems(game_state->loaded_level, PARTICLES_TIMESTEP);
                particle_accumulation_timer -= PARTICLES_TIMESTEP;
            }

            particle_interpolation_value = particle_accumulation_timer / PARTICLES_TIMESTEP;
        }
    }

    camera_set_focus_speed_x(&game_camera, 3);
    camera_set_focus_speed_y(&game_camera, 2);

#ifdef FORFRIENDS_DEMO
    camera_set_focus_speed_x(&game1_camera, 3);
    camera_set_focus_speed_y(&game1_camera, 2);

    camera_set_focus_speed_x(&game2_camera, 3);
    camera_set_focus_speed_y(&game2_camera, 2);

    camera_set_focus_speed_x(&game3_camera, 3);
    camera_set_focus_speed_y(&game3_camera, 2);
#endif

    camera_set_bounds(&game_camera, game_state->loaded_level->bounds_min_x, game_state->loaded_level->bounds_min_y, game_state->loaded_level->bounds_max_x, game_state->loaded_level->bounds_max_y);
#ifdef FORFRIENDS_DEMO
    camera_set_bounds(&game1_camera, game_state->loaded_level->bounds_min_x, game_state->loaded_level->bounds_min_y, game_state->loaded_level->bounds_max_x, game_state->loaded_level->bounds_max_y);
    camera_set_bounds(&game2_camera, game_state->loaded_level->bounds_min_x, game_state->loaded_level->bounds_min_y, game_state->loaded_level->bounds_max_x, game_state->loaded_level->bounds_max_y);
    camera_set_bounds(&game3_camera, game_state->loaded_level->bounds_min_x, game_state->loaded_level->bounds_min_y, game_state->loaded_level->bounds_max_x, game_state->loaded_level->bounds_max_y);
#endif
    {
        struct entity* player2 = &game_state->persistent_entities[1];
        struct entity* player3 = &game_state->persistent_entities[2];
        struct entity* player4 = &game_state->persistent_entities[3];
        camera_set_focus_position(&game_camera, player->x - player->w/2, player->y - player->h/2);
        camera_set_focus_position(&game1_camera, player2->x - player2->w/2, player2->y - player2->h/2);
        camera_set_focus_position(&game2_camera, player3->x - player3->w/2, player3->y - player3->h/2);
        camera_set_focus_position(&game3_camera, player4->x - player4->w/2, player4->y - player4->h/2);
    }

    /* UI is a substate, we still draw the game under the UI, so it can look nice. */
    int active_players = 0;
    int screen_dimens[2];
    {
        {
            if (!(game_state->persistent_entities[0].flags & ENTITY_FLAGS_DISABLED)) {active_players++;}
            if (!(game_state->persistent_entities[1].flags & ENTITY_FLAGS_DISABLED)) {active_players++;}
            if (!(game_state->persistent_entities[2].flags & ENTITY_FLAGS_DISABLED)) {active_players++;}
            if (!(game_state->persistent_entities[3].flags & ENTITY_FLAGS_DISABLED)) {active_players++;}
        }

        get_screen_dimensions(screen_dimens, screen_dimens+1);
        _report_active_players(active_players);
    }
    int split_screen_width = screen_dimens[0] / active_players;
    _p_idx(0);
    begin_graphics_frame(NULL); {
        draw_filled_rectangle(0, 0, 9999, 9999, active_colorscheme.secondary);
    } end_graphics_frame();
    if (active_players == 1) {
        clip_rect(0, 0, 0, 0, 0);
        begin_graphics_frame(&game_camera); {
            draw_all_background_particle_systems(particle_interpolation_value);

            {
                draw_tiles(tilemap->background_tiles, tilemap->background_tile_count, active_colorscheme.primary_background);
            }

            struct entity_iterator entities = game_state_entity_iterator(game_state);
            draw_soul_anchors(tilemap->soul_anchors, tilemap->soul_anchor_count);
            draw_all_particle_systems(particle_interpolation_value);
            {
                draw_doors(tilemap->doors, tilemap->door_count, false);
                draw_activation_switches(tilemap->activation_switches, tilemap->activation_switch_count);
                draw_tiles(tilemap->tiles, tilemap->height * tilemap->width, active_colorscheme.primary);
                draw_all_entities(&entities, dt, physics_interpolation_value);
                draw_grass_tiles(tilemap->grass_tiles, tilemap->grass_tile_count, active_colorscheme.primary);
                draw_tiles(tilemap->foreground_tiles, tilemap->foreground_tile_count, active_colorscheme.primary_foreground);
            }
            DEBUG_draw_debug_stuff();
        } end_graphics_frame();
    } else {
        int split_x = 0;
        for (int i = 0; i < active_players; ++i) {
            clip_rect(true, split_x, 0, split_screen_width, screen_dimens[1]);
            split_x += split_screen_width;

            struct camera* target = &game_camera;
            if (i == 1) target = &game1_camera;
            if (i == 2) target = &game2_camera;
            if (i == 3) target = &game3_camera;
            _p_idx(i);

            begin_graphics_frame(target); {
                draw_all_background_particle_systems(particle_interpolation_value);

                {
                    draw_tiles(tilemap->background_tiles, tilemap->background_tile_count, active_colorscheme.primary_background);
                }

                struct entity_iterator entities = game_state_entity_iterator(game_state);
                draw_soul_anchors(tilemap->soul_anchors, tilemap->soul_anchor_count);
                draw_all_particle_systems(particle_interpolation_value);
                {
                    draw_doors(tilemap->doors, tilemap->door_count, false);
                    draw_activation_switches(tilemap->activation_switches, tilemap->activation_switch_count);
                    draw_tiles(tilemap->tiles, tilemap->height * tilemap->width, active_colorscheme.primary);
                    draw_all_entities(&entities, dt, physics_interpolation_value);
                    draw_grass_tiles(tilemap->grass_tiles, tilemap->grass_tile_count, active_colorscheme.primary);
                    draw_tiles(tilemap->foreground_tiles, tilemap->foreground_tile_count, active_colorscheme.primary_foreground);
                }
                DEBUG_draw_debug_stuff();
            } end_graphics_frame();
        }
    }

    /* draw and update UI */
    {
        if (animation_id != GAME_ANIMATION_ID_NONE) {
            do_render_game_global_animations(dt);
        }

        clip_rect(0,0,0,0,0);
        _p_idx(0);
        _report_active_players(1);
        switch (game_state->menu_mode) {
            case GAMEPLAY_UI_INGAME: {
                do_gameplay_ui(gamepad, dt);
                try_and_record_player_grounded_position(dt);
            } break;
            case GAMEPLAY_UI_MAINMENU: {
                do_mainmenu_ui(gamepad, dt);
            } break;
            case GAMEPLAY_UI_PAUSEMENU: {
                do_pausemenu_ui(gamepad, dt);
            } break;
        }
    }

    /* colorscheme fluctuates brightness */
    {
        float ambient = 0.9;
        float alpha = ambient + normalized_sinf(global_elapsed_time * 2) * (1 - ambient);
        for (unsigned index = 0; index < array_count(loaded_colorscheme.colors); ++index) {
            active_colorscheme.colors[index].r = loaded_colorscheme.colors[index].r * alpha;
            active_colorscheme.colors[index].g = loaded_colorscheme.colors[index].g * alpha;
            active_colorscheme.colors[index].b = loaded_colorscheme.colors[index].b * alpha;
        }
    }
    DEBUG_draw_debug_ui_stuff();
}
