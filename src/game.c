/*
  physics "constants"
*/
#define VPIXELS_PER_METER (1)
#define TILES_PER_SCREEN (48)
/*
  This might just turn into an uber struct or something.
*/
struct entity {
    float x;
    float y;
    
    float w;
    float h;

    /* only acceleration is gravity for now. Don't care about other forces atm */
    float vx;
    float vy;

    float ax;
    float ay;

    /* for falling reasons */
    float last_vy;
    bool onground;

    /*temporary*/
    int facing_dir;
    bool dash;
    /*end temporary*/

    /* player specific */
    float jump_leniancy_timer;
};

bool noclip = false;
struct entity player = {
    // no units, prolly pixels
    .x = -VPIXELS_PER_METER/4,
    .y = -5,
    .w = VPIXELS_PER_METER/2.0f,
    .h = VPIXELS_PER_METER,
};

/*TODO(jerry): move some game state globals into this.*/
local struct memory_arena game_memory_arena;

local texture_id knight_twoview;
local font_id    dynamic_scale_font;
local font_id    test_font;
local font_id    game_title_font;
local font_id    game_ui_menu_font;

local font_id    test2_font;
local font_id    test3_font;
local sound_id   test_sound;
local sound_id   test_sound2;

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

#include "tilemap.c"
#include "particle_systems.c"

struct game_state {
    uint8_t menu_mode;
    uint8_t menu_transition_state;

    float quit_transition_timer[2];
    float ingame_transition_timer[2];

    uint8_t selected_menu_option;

    /*whoops this is an additional indirection. Fix this at the end of the night*/
    struct tilemap* loaded_level;
};

local struct camera game_camera   = {};
local struct camera editor_camera = {};
local struct game_state* game_state;

enum game_mode mode = GAME_MODE_PLAYING;

#include "gameplay_mode.c"
#include "tilemap_editor.c"

void load_graphics_resources(void) {
    /* can change depending on resolution maybe... */
    knight_twoview      = load_texture("assets/knight_twoview.png");
    test_font           = load_font("assets/Exoplanetaria-gxxJ5.ttf", font_size_aspect_ratio_independent(0.03));
    _console_font  = load_font("assets/LiberationMono-Regular.ttf", 16);
    test3_font          = load_font("assets/Exoplanetaria-gxxJ5.ttf", font_size_aspect_ratio_independent(0.04));
    /* test2_font      = load_font("assets/Exoplanetaria-gxxJ5.ttf", 64); */
    /* test2_font      = load_font("assets/charissilbold.ttf", 64); */
    /* test2_font      = load_font("assets/Helmet-lWZV.otf", 64); */

    test2_font      = load_font("assets/Exo2Medium-aDL9.ttf", font_size_aspect_ratio_independent(0.07));

    game_title_font   = load_font("assets/Exoplanetaria-gxxJ5.ttf", GAME_UI_TITLE_FONT_SIZE);
    game_ui_menu_font = load_font("assets/Exoplanetaria-gxxJ5.ttf", GAME_UI_MENU_FONT_SIZE);

    game_camera.render_scale   = ratio_with_screen_width(TILES_PER_SCREEN);
    editor_camera.render_scale = ratio_with_screen_width(TILES_PER_SCREEN);

    /* dynamic_scale_font = load_font("assets/Exoplanetaria-gxxJ5.ttf", font_size_aspect_ratio_independent(DYNAMIC_FONT_SIZE)); */
    /* test2_font      = load_font("assets/TwentyOne-nRmJ.ttf", 64); */
    load_gameplay_resources();
}

local void load_static_resources(void) {
    test_sound     = load_sound("assets/emp.wav");
    test_sound2    = load_sound("assets/explosion_b.wav");

    game_memory_arena = allocate_memory_arena(Megabyte(64));
    game_memory_arena.name = "Game Arena";

    game_state = memory_arena_push(&game_memory_arena, sizeof(*game_state));
    initialize_particle_emitter_pool(&game_memory_arena, MAX_PARTICLE_EMITTER_COUNT);

    load_tilemap_editor_resources();
    gameplay_initialize();
    console_execute_cstr("load 1.lvl");
}
/*
  This is really reloading all graphical assets... But anyways.
 */
local void reload_all_graphics_resources(void) {
    unload_all_graphics_resources();
    load_graphics_resources();
}

/*binary format*/
void game_load_level(struct memory_arena* arena, char* filename, char* transition_link_to_spawn_at) {
    memory_arena_clear_top(arena);
    FILE* f = fopen(filename, "rb+");
    game_state->loaded_level = memory_arena_push_top(arena, sizeof(*game_state->loaded_level));

    char magic[8] = {};
    fread(magic, 8, 1, f);
    assert(strncmp(magic, "MVOIDLVL", 8) == 0);

    fread(&game_state->loaded_level->width, sizeof(game_state->loaded_level->width), 1, f);
    fread(&game_state->loaded_level->height, sizeof(game_state->loaded_level->height), 1, f);
    fread(&game_state->loaded_level->default_spawn, sizeof(game_state->loaded_level->default_spawn), 1, f);
    fread(&game_state->loaded_level->bounds_min_x, sizeof(float), 1, f);
    fread(&game_state->loaded_level->bounds_min_y, sizeof(float), 1, f);
    fread(&game_state->loaded_level->bounds_max_x, sizeof(float), 1, f);
    fread(&game_state->loaded_level->bounds_max_y, sizeof(float), 1, f);

    {
        uint32_t tile_count; 
        fread(&tile_count, sizeof(tile_count), 1, f);
        assert(tile_count > 0);

        game_state->loaded_level->tiles = memory_arena_push_top(arena, game_state->loaded_level->width * game_state->loaded_level->height * sizeof(struct tile));;
        {
            struct temporary_arena temp = begin_temporary_memory(arena, tile_count);
            struct tile* tiles = memory_arena_push(&temp, sizeof(*tiles) * tile_count);
            fread(tiles, sizeof(*tiles) * tile_count, 1, f);


            zero_buffer_memory(game_state->loaded_level->tiles, game_state->loaded_level->width * game_state->loaded_level->height * sizeof(struct tile));

            int min_x, min_y;
            int a, b;
            get_bounding_rectangle_for_tiles(tiles, tile_count, &min_x, &min_y, &a, &b);

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
    {
        fread(&game_state->loaded_level->transition_zone_count, sizeof(game_state->loaded_level->transition_zone_count), 1, f);
        game_state->loaded_level->transitions = memory_arena_push_top(arena, game_state->loaded_level->transition_zone_count * sizeof(*game_state->loaded_level->transitions));;
        fread(game_state->loaded_level->transitions, sizeof(*game_state->loaded_level->transitions) * game_state->loaded_level->transition_zone_count, 1, f);
    }
    {
        fread(&game_state->loaded_level->player_spawn_link_count, sizeof(game_state->loaded_level->player_spawn_link_count), 1, f);
        game_state->loaded_level->link_spawns = memory_arena_push_top(arena, game_state->loaded_level->player_spawn_link_count * sizeof(*game_state->loaded_level->link_spawns));
        fread(game_state->loaded_level->link_spawns, sizeof(*game_state->loaded_level->link_spawns) * game_state->loaded_level->player_spawn_link_count, 1, f);
    }

    /*level is loaded, now setup player spawns*/
    if (transition_link_to_spawn_at) {
        for (unsigned index = 0; index < game_state->loaded_level->player_spawn_link_count; ++index) {
            struct player_spawn_link* spawn = game_state->loaded_level->link_spawns + index;
            console_printf("%s vs %s\n", transition_link_to_spawn_at, spawn->identifier);
            if (strncmp(spawn->identifier, transition_link_to_spawn_at, TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH) == 0) {
                player.x = spawn->x;
                player.y = spawn->y;
                break;
            }
        }
    } else {
        player.x = game_state->loaded_level->default_spawn.x;
        player.y = game_state->loaded_level->default_spawn.y;
    }

    camera_set_position(&game_camera, player.x, player.y);
}

void update_render_frame(float dt) {
    clear_color(COLOR4F_BLACK);

    switch (mode) {
        case GAME_MODE_PLAYING: game_update_render_frame(dt); break;
        case GAME_MODE_EDITOR: tilemap_editor_update_render_frame(dt); break;
    }
}
