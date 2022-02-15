local struct particle_emitter* test_emitter = 0x12345;
local struct particle_emitter* test_emitter2 = 0x12345;

#define TEST_timer1_max 2
float TEST_timer1 = 0;
bool TEST_bool1 = 0;

local void load_gameplay_resources(void) {
    load_all_tile_assets();
}

local void gameplay_initialize(void) {
    game_state_add_persistent_entity(game_state, entity_create_player(0, 0));

    test_emitter  = particle_emitter_allocate();
    test_emitter2 = particle_emitter_allocate();

    {
        test_emitter->emission_rate = 0.01;
        test_emitter->emission_count = 16;
        test_emitter->particle_color = color4f(1.0, 0.0, 0.0, 1.0);
        test_emitter->particle_max_lifetime = 1;
        test_emitter->collides_with_world = true;
    }

    {
        test_emitter2->emission_rate = 0.01;
        test_emitter2->emission_count = 16;
        test_emitter2->particle_color = color4f(0.12, 0.2, 0.85, 1.0);
        test_emitter2->particle_max_lifetime = 8;
        test_emitter2->collides_with_world = false;

        int a = 0;
        int b = -8;
        test_emitter2->x = a;
        test_emitter2->y = b;

        test_emitter2->x1 = a + 30;
        test_emitter2->y1 = b;
    }
}

struct entity_iterator game_state_entity_iterator(struct game_state* game_state) {
    struct entity_iterator entities = {};

    entity_iterator_push_array(&entities, game_state->persistent_entities, game_state->entity_count);

    return entities;
}

local void DEBUG_draw_debug_stuff(void) {
    /*add debug rendering code here*/

    /*boundaries of the loaded world*/
    {
        struct tilemap* tilemap = game_state->loaded_level;
        float bounds_width  = tilemap->bounds_max_x - tilemap->bounds_min_x;
        float bounds_height = tilemap->bounds_max_y - tilemap->bounds_min_y;

        draw_rectangle(tilemap->bounds_min_x, tilemap->bounds_min_y,
                       bounds_width, bounds_height, COLOR4F_BLUE);
    }

    /* Camera bounds */
    #if 0
    {
        struct camera* camera = get_global_camera();
        struct rectangle bounds = camera_get_bounds(camera);

        draw_rectangle(bounds.x, bounds.y, bounds.w, bounds.h, COLOR4F_BLUE);
    }
    #endif
}

local void DEBUG_draw_debug_ui_stuff(void) {
    begin_graphics_frame(NULL); {
        {
            struct entity* player = &game_state->persistent_entities[0];
            struct memory_arena* arena = &game_memory_arena;
            size_t memusage = memory_arena_total_usage(arena);
            char* arena_msg = format_temp("(memory arena \"%s\") is using %d bytes\n(%d kb)\n(%d mb)\n(%d gb)\n(onground: %d)\n(player %f, %f, %f, %f)\n",
                                          arena->name, memusage, memusage / 1024, memusage / (1024 * 1024), memusage / (1024*1024*1024), player->onground, player->x, player->y, player->vx, player->vy);
            draw_text(_console_font, 0, 0, arena_msg, COLOR4F_GREEN);
        }
    } end_graphics_frame();
}

local void game_update_render_frame(float dt) {
    struct game_controller* gamepad = get_gamepad(0);
    struct entity* player = &game_state->persistent_entities[0];
    struct tilemap* tilemap = game_state->loaded_level;

    if (game_state->menu_mode == GAMEPLAY_UI_INGAME) {
        {
            local float physics_accumulation_timer = 0;
            const int PHYSICS_FRAMERATE = 300;
            const float PHYSICS_TIMESTEP = 1.0f / (float)(PHYSICS_FRAMERATE);

            struct entity_iterator entities = game_state_entity_iterator(game_state);
            while (physics_accumulation_timer > 0.0f) {
                do_entity_physics_updates(&entities, tilemap, PHYSICS_TIMESTEP);
                physics_accumulation_timer -= PHYSICS_TIMESTEP;
            }

            physics_accumulation_timer += dt;
        }
        {
            struct entity_iterator entities = game_state_entity_iterator(game_state);

            do_entity_updates(&entities, tilemap, dt);
            update_all_particle_systems(game_state->loaded_level, dt);
        }
    }

    {
        test_emitter->x = test_emitter->x1 = player->x;
        test_emitter->y = test_emitter->y1 = player->y;
    }

    camera_set_focus_speed_zoom(&game_camera, 5);
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
        {
            draw_tiles(tilemap->background_tiles, tilemap->background_tile_count, active_colorscheme.primary_background);
        }

        {
            draw_filled_rectangle(player->x, player->y, player->w, player->h, active_colorscheme.primary);
            /* draw_texture(test_guy, player.x, player.y+player.h - (32.0f/16.0f), 16/16, (32.0f/16), active_colorscheme.primary); */
        }

        draw_all_particle_systems();
        {
            draw_tiles(tilemap->tiles, tilemap->height * tilemap->width, active_colorscheme.primary);
            draw_tiles(tilemap->foreground_tiles, tilemap->foreground_tile_count, active_colorscheme.primary_foreground);
        }

        DEBUG_draw_debug_stuff();
    } end_graphics_frame();

    /* draw and update UI */
    {
        switch (game_state->menu_mode) {
            case GAMEPLAY_UI_INGAME: {
                do_gameplay_ui(gamepad, dt);
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
