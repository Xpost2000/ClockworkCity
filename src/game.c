/*
  physics "constants"
*/

/*
  Will I ever do the vector renderer? We'll see since thankfully the "placeholder" graphics and
  final artstyle are relatively simple (monochrome "shapes", not 1 bit art per say (well characters are 1 bit art...)).
  
  Anyways, resolve to do auto-save since Dark Souls is cool
  
  Also this clearly doesn't follow the theme, but thankfully it's a guideline
  since I want to stick with something safe so it has a chance of being polished,
  not trying to make anything novel intentionally...
  
  NOTE(jerry): nope, the answer is no. Or very likely no.
*/

/*
  TODO(jerry):
  
  Coming back tomorrow actually.
  
  - Camera Focus Zones (For cinematics :) or focusing on specific things.)
  - Trigger placement for cutscenes/prompts
  - Spikes doing damage
  - Add "Rest" points (Soul Anchors)
  - Player Death Animation
  - Entity Death Animation
  - Game Save / Load (single save.)
  - (Revelation for ability achievement)
  - Add our first boss (Should be doable without any abilities, and maybe just copy the style for False Knight?)

  Consider adding props very soon. They're technically also entities but a subtype.
  add particle placements (allow line placement for rains!)
 */
/* #define DEV */
#define LAST_GROUNDED_POSITION_RECORD_TIMER_MAX (5.0f) /* seconds */

/* HARDCODED LEVELLIST ASSOCIATION */
local struct {
    char* level_name;
    char* colorscheme_name;
} global_level_colorscheme_list[] = {
    { "limbo1", "LimboScheme" },
    { "hub",    "LimboScheme" },
};

local struct camera game_camera   = {};
local struct camera editor_camera = {};

local float game_timescale = 1;

#include "colorschemes.c"

bool noclip = false;
local struct memory_arena game_memory_arena;

#include "prompt_font_info.h"

/* thankgod for being a small game. I can load all assets into memory at start up */

local texture_id playersizedblock; /* NOTE(jerry): This is exclusively for testing out a death screen! */
local texture_id test_guy;
local texture_id knight_twoview;
local texture_id test_icon;
local font_id    dynamic_scale_font;
local font_id    test_font;
local font_id    game_title_font;
local font_id    game_ui_menu_font;

local texture_id ui_health_slice;

local font_id    test2_font;
local font_id    test3_font;
local sound_id   test_sound;
local sound_id   test_sound2;

void game_load_level(struct memory_arena* arena, char* filename, char* transition_link_to_spawn_at);
void game_load_level_from_serializer(struct memory_arena* arena, struct binary_serializer* serializer, char* transition_link_to_spawn_at);

enum game_mode {
    GAME_MODE_STATUS_INTRO,
    GAME_MODE_PLAYING,
    GAME_MODE_EDITOR,
    GAME_MODE_COUNT,
};

local void unload_all_graphics_resources(void) {
    unload_all_textures();
    unload_all_fonts();
}

local int font_size_based_on_screen_height_percentage(float percentage) {
    int height;
    get_screen_dimensions(0, &height);
    return floorf(height * percentage);
}

local int font_size_based_on_screen_width_percentage(float percentage) {
    int width;
    get_screen_dimensions(&width, 0);
    return floorf(width * percentage);
}

/* doesn't base itself off a target aspect ratio. Just checking for approximate screen dimensions. */
local int font_size_aspect_ratio_independent(float percentage) {
    int dimens[2];
    get_screen_dimensions(dimens, dimens+1);

    float aspect_ratio = safe_ratio(dimens[0], dimens[1]);

    if (aspect_ratio >= 1.0) {
        /* wider aspect ratio */
        return font_size_based_on_screen_height_percentage(percentage);
    }

    /*
      If I actually scale purely on this... It doesn't look quite correct since the text may intersect with the
      window.

      This looks more like what I want.
     */
    const float SCALING_EPISILON = 0.00875;
    return font_size_based_on_screen_width_percentage(maxf(percentage - SCALING_EPISILON, 0.0f));
}

/* 
   This is seriously making me consider a types.h approach.
   Cause C can't correct references to types that haven't been declared
   yet :/
*/
local void game_activate_prompt(uint32_t id);
local void game_close_prompt(void);
#include "persistent_state_def.c"
#include "entities_def.c"
#include "game_menus_defs.c"

#include "tilemap.c"
#include "particle_systems.c"

#include "game_animations_def.c"

#define PERSISTENT_ENTITY_COUNT_MAX (256)
struct last_rest_location {
    bool     can_revive;
    char     level[FILENAME_MAX_LENGTH];
    uint32_t soul_anchor_index;
};
struct rest_prompt_state {
    bool active;
    struct soul_anchor* anchor;
    float t;
};
struct game_state {
    char current_level_filename[FILENAME_MAX_LENGTH];

    uint8_t menu_mode;
    uint8_t menu_transition_state;

    float quit_transition_timer[2];
    float ingame_transition_timer[2];

    uint8_t selected_menu_option;

    /*whoops this is an additional indirection. Fix this at the end of the night*/
    struct tilemap* loaded_level;

    /* 
       NOTE(jerry):
       These are permenant storage entities, which include the player and
       any projectiles or otherwise "generated" entities.
       
       Level entities are stored as part of the level they came from.
    */
    uint16_t entity_count;
    struct entity             persistent_entities[PERSISTENT_ENTITY_COUNT_MAX];
    struct persistent_changes persistent_changes;
    struct last_rest_location last_rest_location;
    struct rest_prompt_state  rest_prompt;

    /* use this! Then a fade out mayhaps? */
    bool  have_a_good_grounded_position;
    float last_good_grounded_position_recording_timer;
    int   last_good_grounded_position_x;
    int   last_good_grounded_position_y;
};
/*
  Entity IDs are only going to be indices, I'm not currently expecting to
  store references to anything.

  The only state I need to keep track of will be manually kept.
 */
void game_state_add_persistent_entity(struct game_state* game_state, struct entity entity) {
    if (game_state->entity_count < PERSISTENT_ENTITY_COUNT_MAX) {
        game_state->persistent_entities[game_state->entity_count++] = entity;
    }
}

void game_state_remove_persistent_entity(struct game_state* game_state, uint32_t index) {
    game_state->persistent_entities[index] = game_state->persistent_entities[--game_state->entity_count];
}

local struct game_state* game_state;

enum game_mode mode = GAME_MODE_STATUS_INTRO;
void game_queue_load_level(struct memory_arena* arena, char* filename, char* transition_link_to_spawn_at);

void game_activate_soul_anchor(struct soul_anchor* anchor) {
    if (!anchor->unlocked) {
        anchor->unlocked = true;
        {
            struct persistent_change change;
            change.type = PERSISTENT_CHANGE_SOUL_ANCHOR_ACTIVATED;
            change.soul_anchor_activated.soul_anchor_index = game_state->last_rest_location.can_revive;
            persistent_changes_add_change(&game_state->persistent_changes, game_state->current_level_filename, change);
        }
    }

    strncpy(game_state->last_rest_location.level, game_state->current_level_filename, FILENAME_MAX_LENGTH);
    game_state->last_rest_location.soul_anchor_index = anchor - game_state->loaded_level->soul_anchors;
    game_state->last_rest_location.can_revive = true;

}

void game_load_level(struct memory_arena* arena, char* filename, char* transition_link_to_spawn_at);
void game_revive_to_last_soul_anchor(void) {
    if (!game_state->last_rest_location.can_revive)
        return;

    struct entity* player = &game_state->persistent_entities[0];
    player->health = player->max_health;
    player->death_state = DEATH_STATE_ALIVE;
    /* play a little wakeup animation... */
    /* if (strncmp(game_state->current_level_filename, game_state->last_rest_location.level, FILENAME_MAX_LENGTH) != 0) { */
    game_load_level(&game_memory_arena, game_state->last_rest_location.level, NULL);
    /* } */

    struct soul_anchor* target_anchor = game_state->loaded_level->soul_anchors + game_state->last_rest_location.soul_anchor_index;
    player->x = target_anchor->x;
    player->y = target_anchor->y;
}

void game_signal_rest_prompt(struct soul_anchor* anchor) {
    if (game_state->rest_prompt.anchor != anchor) {
        game_state->rest_prompt.t      = 0;
        game_state->rest_prompt.active = true;
        game_state->rest_prompt.anchor = anchor;
    }
}

void game_cancel_rest_prompt(void) {
    game_state->rest_prompt.active = false;
    game_state->rest_prompt.anchor = 0;
}

#include "entities.c"

struct entity_iterator game_state_entity_iterator(struct game_state* game_state) {
    struct entity_iterator entities = {};

    entity_iterator_push_array(&entities, game_state->persistent_entities, game_state->entity_count);
    /* NOTE(jerry): *sigh*, this shouldn't be a pointer and I have no idea why I made it that way before. Definitely not for a good reason */
    if (game_state->loaded_level)
        entity_iterator_push_array(&entities, game_state->loaded_level->entities, game_state->loaded_level->entity_count);

    return entities;
}

#include "game_message_prompts.c"
#include "game_menus.c"
#include "intro.c"
#include "gameplay_mode.c"
#include "tilemap_editor.c"

local void load_graphics_resources(void) {
    /* can change depending on resolution maybe... */
    knight_twoview      = load_texture("assets/knight_twoview.png");
    test_icon      = load_texture("assets/icon.png");
    test_font           = load_font("assets/LiberationMono-Regular.ttf", font_size_aspect_ratio_independent(0.03));
    test_guy  = load_texture("assets/guy.png");
    _console_font  = load_font("assets/LiberationMono-Regular.ttf", 16);
    test3_font          = load_font("assets/Exoplanetaria-gxxJ5.ttf", font_size_aspect_ratio_independent(0.04));
    ui_health_slice = load_texture("assets/ui_healthslice.png");

    test2_font      = load_font("assets/Exo2Medium-aDL9.ttf", font_size_aspect_ratio_independent(0.07));

    /* base kind? */
    for (unsigned index = 0; index < array_count(controller_prompt_font); ++index) {
        controller_prompt_font[index] = load_font("assets/promptfont/PromptFont.otf", font_size_aspect_ratio_independent(prompt_font_sizes[index]));
    }

    game_title_font   = load_font("assets/Exoplanetaria-gxxJ5.ttf", GAME_UI_TITLE_FONT_SIZE);
    game_ui_menu_font = load_font("assets/Exoplanetaria-gxxJ5.ttf", GAME_UI_MENU_FONT_SIZE);

    game_camera.render_scale   = ratio_with_screen_width(TILES_PER_SCREEN);
    editor_camera.render_scale = ratio_with_screen_width(TILES_PER_SCREEN);

    initialize_entity_graphics_assets();
    playersizedblock = load_texture("assets/playersizedblock.png");

    load_all_particle_textures();
    intro_load_resources();
    load_gameplay_resources();
}

local void load_static_resources(void) {
    initialize_entity_audio_assets();
    test_sound     = load_sound("assets/emp.wav");
    test_sound2    = load_sound("assets/explosion_b.wav");

    game_memory_arena = allocate_memory_arena(Megabyte(128));
    game_memory_arena.name = "Game Arena";

    game_state = memory_arena_push(&game_memory_arena, sizeof(*game_state));
    initialize_colorscheme_database(&game_memory_arena);
    initialize_particle_emitter_pool(&game_memory_arena);

    use_colorscheme("MonoDefault0");

    gameplay_initialize();
    initialize_grass_visual_tables();
}
/*
  This is really reloading all graphical assets... But anyways.
 */
local void reload_all_graphics_resources(void) {
    unload_all_graphics_resources();
    load_graphics_resources();
}

void game_serialize_level(struct memory_arena* arena, struct binary_serializer* serializer) {
    char magic[8] = "MVOIDLVL";
    serialize_bytes(serializer, magic, 8);
    assert(strncmp(magic, "MVOIDLVL", 8) == 0);

    uint32_t version_id = TILEMAP_CURRENT_VERSION;
    serialize_u32(serializer, &version_id);

    serialize_u32(serializer, &game_state->loaded_level->width);
    serialize_u32(serializer, &game_state->loaded_level->height);

    Serialize_Structure(serializer, game_state->loaded_level->default_spawn);

    serialize_f32(serializer, &game_state->loaded_level->bounds_min_x);
    serialize_f32(serializer, &game_state->loaded_level->bounds_min_y);
    serialize_f32(serializer, &game_state->loaded_level->bounds_max_x);
    serialize_f32(serializer, &game_state->loaded_level->bounds_max_y);

    /* 
       NOTE(jerry):
       the only reason why the serialization code is different between editor and game is this,
       otherwise, I could use the same function.
       
       This is required because the game runtime assumes the tilemap is a 2D array for simplicity reasons.
    */
    {
        uint32_t tile_count;
        serialize_u32(serializer, &tile_count); assert(tile_count > 0);
        game_state->loaded_level->tiles = memory_arena_push_top(arena, game_state->loaded_level->width * game_state->loaded_level->height * sizeof(struct tile));;

        {
            struct temporary_arena temp = begin_temporary_memory(arena, tile_count);
            struct tile* tiles = memory_arena_push(&temp, sizeof(*tiles) * tile_count);
            serialize_bytes(serializer, tiles, sizeof(*tiles) * tile_count);

            zero_buffer_memory(game_state->loaded_level->tiles, game_state->loaded_level->width * game_state->loaded_level->height * sizeof(struct tile));

            int min_x, min_y, e, f;
            get_bounding_rectangle_for_tiles(tiles, tile_count, &min_x, &min_y, &e, &f);

            for (size_t index = 0; index < tile_count; ++index) {
                struct tile* t = tiles + index;
                int index_y = ((t->y) - (min_y));
                int index_x = ((t->x) - (min_x));
                assert(index_y >= 0 && index_x >= 0);
                int index_mapped = index_y * game_state->loaded_level->width + index_x;
                assert(index_mapped >= 0 && index_mapped < game_state->loaded_level->width * game_state->loaded_level->height);
                game_state->loaded_level->tiles[index_mapped] = *t;
            }

            end_temporary_memory(&temp);
        }
    }

    if (version_id >= 2) {
        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u32, game_state->loaded_level->foreground_tile_count, game_state->loaded_level->foreground_tiles);
        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u32, game_state->loaded_level->background_tile_count, game_state->loaded_level->background_tiles);

        if (version_id >= 3) {
            Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u32, game_state->loaded_level->grass_tile_count, game_state->loaded_level->grass_tiles);

            if (version_id >= 4) {
                console_printf("new version!\n");
                /*
                  The editor data is smaller, we unpack it here.
                */
                uint16_t entity_placement_count;
                serialize_u16(serializer, &entity_placement_count);
                console_printf("%d entities to unpack\n", entity_placement_count);
                struct temporary_arena conversion_arena = begin_temporary_memory(arena, entity_placement_count * sizeof(struct entity_placement)); 
                struct entity_placement* entity_placements = memory_arena_push(&conversion_arena, entity_placement_count * sizeof(*entity_placements));
                serialize_bytes(serializer, entity_placements, entity_placement_count * sizeof(*entity_placements));

                {
                    game_state->loaded_level->entity_count = entity_placement_count;
                    struct entity* unpacked_entities = memory_arena_push_top(arena, sizeof(*unpacked_entities) * entity_placement_count);

                    /* unpack entities */

                    console_printf("Trying to unpack %d entities\n", entity_placement_count);
                    for (unsigned index = 0; index < entity_placement_count; ++index) {
                        struct entity_placement* ep = entity_placements + index;
                        struct entity*           unpack_into = unpacked_entities + index;

                        *unpack_into = construct_entity_of_type(ep->type, ep->x, ep->y);
                        unpack_into->flags      = ep->flags;
                        unpack_into->facing_dir = ep->facing_direction;
                        console_printf("Unpacked a %s (%f, %f)\n", entity_type_strings[ep->type], unpack_into->x, unpack_into->y);

                        initialize_entity(unpack_into);
                    }

                    game_state->loaded_level->entities = unpacked_entities;

                    if (version_id >= 5) {
                        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u16, game_state->loaded_level->trigger_count, game_state->loaded_level->triggers);
                        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u16, game_state->loaded_level->camera_focus_zone_count, game_state->loaded_level->camera_focus_zones);
                        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u16, game_state->loaded_level->soul_anchor_count, game_state->loaded_level->soul_anchors);

                        if (version_id >= 6) {
                            Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u16, game_state->loaded_level->activation_switch_count, game_state->loaded_level->activation_switches);
                            Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u16, game_state->loaded_level->door_count, game_state->loaded_level->doors);
                        }
                    }
                }

                end_temporary_memory(&conversion_arena);
            }
        }
    }
    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u8, game_state->loaded_level->transition_zone_count, game_state->loaded_level->transitions);
    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u8, game_state->loaded_level->player_spawn_link_count, game_state->loaded_level->link_spawns);
}

/*binary format*/
local void cleanup_current_entities(void) {
    struct entity_iterator entities = game_state_entity_iterator(game_state);
    struct entity* player = &game_state->persistent_entities[0];

    for (struct entity* current_entity = entity_iterator_begin(&entities); !entity_iterator_done(&entities); current_entity = entity_iterator_next(&entities)) {
        if (current_entity != player)
            cleanup_for_entity(current_entity);
    }
}

void game_load_level_from_serializer(struct memory_arena* arena, struct binary_serializer* serializer, char* transition_link_to_spawn_at) {
    cleanup_current_entities();

    memory_arena_clear_top(arena);
    game_state->loaded_level = memory_arena_push_top(arena, sizeof(*game_state->loaded_level));
    game_state->have_a_good_grounded_position = false;

    game_serialize_level(arena, serializer);
    struct entity* player = &game_state->persistent_entities[0];

    /*level is loaded, now setup player spawns*/
    if (transition_link_to_spawn_at) {
        for (unsigned index = 0; index < game_state->loaded_level->player_spawn_link_count; ++index) {
            struct player_spawn_link* spawn = game_state->loaded_level->link_spawns + index;
            if (strncmp(spawn->identifier, transition_link_to_spawn_at, TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH) == 0) {
                player->x = spawn->x;
                player->y = spawn->y;
                break;
            }
        }
    } else {
        player->x = game_state->loaded_level->default_spawn.x;
        player->y = game_state->loaded_level->default_spawn.y;
    }

    player->last_x = player->x;
    player->last_y = player->y;
    sane_init_all_doors(game_state->loaded_level->doors, game_state->loaded_level->door_count);

    camera_set_position(&game_camera, player->x, player->y);
    camera_force_clamp_to_bounds(&game_camera);
}

void game_load_level(struct memory_arena* arena, char* filename, char* transition_link_to_spawn_at) {
    struct binary_serializer file = open_read_file_serializer(filename);
    game_load_level_from_serializer(arena, &file, transition_link_to_spawn_at);
    game_close_prompt();
    block_player_input = false;
    strncpy(game_state->current_level_filename, filename, FILENAME_MAX_LENGTH);

    /* load by serializer does not work because it does not know the filename, which is probably a weird oversight. Oh well! */
    uint32_t changelist_id = persistent_changes_find_change_list_by_name(&game_state->persistent_changes, game_state->current_level_filename);

    persistent_changes_apply_changes(&game_state->persistent_changes, game_state, 0);
    persistent_changes_apply_changes(&game_state->persistent_changes, game_state, changelist_id);

    /* apply_colorscheme_to_current_level */
    {
        for (unsigned index = 0; index < array_count(global_level_colorscheme_list); ++index) {
            if (!strcmp(global_level_colorscheme_list[index].level_name, game_state->current_level_filename)) {
                use_colorscheme(global_level_colorscheme_list[index].colorscheme_name);
                break; 
            }
        }
    }

    serializer_finish(&file);
}

/* without the animation */
void game_player_revive_warp(void) {
    struct entity* player = &game_state->persistent_entities[0];
    player->health = player->max_health;
    player->death_state = DEATH_STATE_ALIVE;

    if (game_state->last_rest_location.can_revive) {
        game_revive_to_last_soul_anchor();
    } else {
        game_load_level(&game_memory_arena, game_state->current_level_filename, 0 );
    }
} 

void update_render_frame(float dt) {
    clear_color(COLOR4F_BLACK);

    switch (mode) {
        case GAME_MODE_STATUS_INTRO: intro_update_render_frame(dt); break;
        case GAME_MODE_PLAYING:      game_update_render_frame(dt); break;
        case GAME_MODE_EDITOR:       tilemap_editor_update_render_frame(dt); break;
    }

    {
        struct camera_focus_zone* focus_zones      = 0;
        size_t                    focus_zone_count = 0;

        if (game_state->loaded_level) {
            focus_zones      = game_state->loaded_level->camera_focus_zones;
            focus_zone_count = game_state->loaded_level->camera_focus_zone_count;
        }

        camera_update(&game_camera, focus_zones, focus_zone_count, dt);
    }
    camera_update(&editor_camera, 0, 0, dt);
}

/* heavily exagerrated controller rumble */
void notify_camera_traumatize(struct camera* camera, float amount) {
    struct game_controller* gamepad = get_gamepad(0);
    /* some bs equation */
    /* controller_rumble(gamepad, 1, 1, 170 + 485*(5 * (amount+0.35))); */
    /* rumble does not account for accumulated trauma :/ */
    controller_rumble(gamepad, 1, 1, (amount * 9.8) * 1000);
}

#include "persistent_state.c"
