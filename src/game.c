/*
  physics "constants"
*/

/*
  Will I ever do the vector renderer? We'll see since thankfully the "placeholder" graphics and
  final artstyle are relatively simple (monochrome "shapes", not 1 bit art per say (well characters are 1 bit art...)).
  
  Anyways, resolve to do auto-save since Dark Souls is cool
*/
#define VPIXELS_PER_METER (16)
#define VPIXEL_SZ          ((1.0f)/(VPIXELS_PER_METER))
#define TILES_PER_SCREEN (22)
#define GRAVITY_CONSTANT (20)

local struct camera game_camera   = {};
local struct camera editor_camera = {};

#include "colorschemes.c"

bool noclip = false;
local struct memory_arena game_memory_arena;

local texture_id playersizedblock; /* NOTE(jerry): This is exclusively for testing out a death screen! */
local texture_id test_guy;
local texture_id knight_twoview;
local texture_id test_icon;
local font_id    dynamic_scale_font;
local font_id    test_font;
local font_id    game_title_font;
local font_id    game_ui_menu_font;

local font_id    test2_font;
local font_id    test3_font;
local sound_id   test_sound;
local sound_id   test_sound2;

void game_load_level(struct memory_arena* arena, char* filename, char* transition_link_to_spawn_at);
void game_load_level_from_serializer(struct memory_arena* arena, struct binary_serializer* serializer, char* transition_link_to_spawn_at);

enum game_mode {
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
#include "persistent_state_def.c"
#include "entities_def.c"
#include "game_menus_defs.c"

#include "tilemap.c"
#include "particle_systems.c"
#include "persistent_state.c"

#define PERSISTENT_ENTITY_COUNT_MAX (256)
struct game_state {
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

enum game_mode mode = GAME_MODE_PLAYING;

#include "entities.c"
#include "game_menus.c"
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

    test2_font      = load_font("assets/Exo2Medium-aDL9.ttf", font_size_aspect_ratio_independent(0.07));

    game_title_font   = load_font("assets/Exoplanetaria-gxxJ5.ttf", GAME_UI_TITLE_FONT_SIZE);
    game_ui_menu_font = load_font("assets/Exoplanetaria-gxxJ5.ttf", GAME_UI_MENU_FONT_SIZE);

    game_camera.render_scale   = ratio_with_screen_width(TILES_PER_SCREEN);
    editor_camera.render_scale = ratio_with_screen_width(TILES_PER_SCREEN);

    load_all_particle_textures();
    load_gameplay_resources();
}

local void load_static_resources(void) {
    test_sound     = load_sound("assets/emp.wav");
    test_sound2    = load_sound("assets/explosion_b.wav");

    game_memory_arena = allocate_memory_arena(Megabyte(128));
    game_memory_arena.name = "Game Arena";

    game_state = memory_arena_push(&game_memory_arena, sizeof(*game_state));
    initialize_colorscheme_database(&game_memory_arena);
    initialize_particle_emitter_pool(&game_memory_arena);
    playersizedblock = load_texture("assets/playersizedblock.png");

    use_colorscheme("MonoDefault0");

    load_tilemap_editor_resources();
    gameplay_initialize();
    console_execute_cstr("load 1.lvl");
    /* console_execute_cstr("noclip"); */
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
    }
    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u8, game_state->loaded_level->transition_zone_count, game_state->loaded_level->transitions);
    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u8, game_state->loaded_level->player_spawn_link_count, game_state->loaded_level->link_spawns);
}

/*binary format*/
void game_load_level_from_serializer(struct memory_arena* arena, struct binary_serializer* serializer, char* transition_link_to_spawn_at) {
    memory_arena_clear_top(arena);
    game_state->loaded_level = memory_arena_push_top(arena, sizeof(*game_state->loaded_level));

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

    camera_set_position(&game_camera, player->x, player->y);
    camera_force_clamp_to_bounds(&game_camera);
}

void game_load_level(struct memory_arena* arena, char* filename, char* transition_link_to_spawn_at) {
    struct binary_serializer file = open_read_file_serializer(filename);
    game_load_level_from_serializer(arena, &file, transition_link_to_spawn_at);

    serializer_finish(&file);
}

void update_render_frame(float dt) {
    clear_color(COLOR4F_BLACK);

    switch (mode) {
        case GAME_MODE_PLAYING: game_update_render_frame(dt); break;
        case GAME_MODE_EDITOR: tilemap_editor_update_render_frame(dt); break;
    }
}
