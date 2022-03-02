local void load_gameplay_resources(void) {
    load_all_tile_assets();
    load_all_resources_for_prompts();
}

local void gameplay_initialize(void) {
    game_state_add_persistent_entity(game_state, construct_entity_of_type(ENTITY_TYPE_PLAYER, 0, 0));
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
            char* arena_msg = format_temp("(memory arena \"%s\") is using %d bytes\n(%d kb)\n(%d mb)\n(%d gb)\n(onground: %d)\n(player %f, %f, %f, %f lastvy: %f, \ndeathstate: %d, facing_dir: %d\n)\n",
                                          arena->name, memusage, memusage / 1024, memusage / (1024 * 1024), memusage / (1024*1024*1024), player->onground, player->x, player->y, player->vx, player->vy, player->last_vy, player->death_state, player->facing_dir);
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

    /* TODO(jerry): This is a little bugged, (okay the onground state is technically. Check later.)*/
    if (player->onground) {
        if (game_state->last_good_grounded_position_recording_timer <= 0) {
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

void restore_player_to_last_good_grounded(void) {
    if (!game_state->have_a_good_grounded_position)
        return;

    struct entity* player = &game_state->persistent_entities[0];
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
            const int PHYSICS_FRAMERATE = 300;
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
    camera_set_bounds(&game_camera, game_state->loaded_level->bounds_min_x, game_state->loaded_level->bounds_min_y,
                      game_state->loaded_level->bounds_max_x, game_state->loaded_level->bounds_max_y);
    camera_set_focus_position(&game_camera, player->x - player->w/2, player->y - player->h/2);

    /* UI is a substate, we still draw the game under the UI, so it can look nice. */
    begin_graphics_frame(NULL); {
        draw_filled_rectangle(0, 0, 9999, 9999, active_colorscheme.secondary);
    } end_graphics_frame();
    begin_graphics_frame(&game_camera); {
        draw_all_background_particle_systems(particle_interpolation_value);

        {
            draw_tiles(tilemap->background_tiles, tilemap->background_tile_count, active_colorscheme.primary_background);
        }

        struct entity_iterator entities = game_state_entity_iterator(game_state);
        draw_all_entities(&entities, dt, physics_interpolation_value);
        draw_all_particle_systems(particle_interpolation_value);
        {
            draw_tiles(tilemap->tiles, tilemap->height * tilemap->width, active_colorscheme.primary);
            draw_grass_tiles(tilemap->grass_tiles, tilemap->grass_tile_count, active_colorscheme.primary);
            draw_tiles(tilemap->foreground_tiles, tilemap->foreground_tile_count, active_colorscheme.primary_foreground);
        }
        DEBUG_draw_debug_stuff();
    } end_graphics_frame();

    /* draw and update UI */
    {
        if (animation_id != GAME_ANIMATION_ID_NONE) {
            do_render_game_global_animations(dt);
        }

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
    DEBUG_draw_debug_ui_stuff();
}
