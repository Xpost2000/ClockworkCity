/*
  physics "constants"
*/

/*
  As I'm converting to vector based rendering soon, the definition
  of "pixel" is kind of arbitrary.

  However I want characters of 16x16 to be one unit tall, one unit wide, which
  is probably the best my artistic ability will allow me to draw, and that might even
  be stretching it.
  
  If I such at pixel art even more... Just decrease the number!
*/
#define VPIXELS_PER_METER (16)
#define TILES_PER_SCREEN (33)
#define GRAVITY_CONSTANT (20)

/* Let's work on this soon. */
struct game_colorscheme {
    /* will get more complicated overtime I suppose, as I think of more colors to add */
    union color4f primary; // foreground
    union color4f secondary; // background

    union color4f text; // what it sounds like.
};

struct game_colorscheme test1 = {
    .primary   = COLOR4F_normalize(102, 10, 163, 255),
    .secondary = COLOR4F_normalize(204, 128, 255, 255),
    .text      = COLOR4F_normalize(210, 230, 92, 255),
};
struct game_colorscheme DEFAULT_mono = {
    .secondary = COLOR4F_BLACK,
    .primary   = COLOR4F_WHITE,
    .text = COLOR4F_WHITE,
};
bool transitioning = false;
struct game_colorscheme transition_to;
float transition_t = 0;
struct game_colorscheme active_colorscheme;

/*
  Particles are theoretically a type of entity, so this allows me to
  make entities without 'inheritance'.
  
  This is like 40 bytes btw! So a bit chunky.
*/
#define KINEMATIC_ENITTY_BASE_BODY()            \
    float x;                                    \
    float y;                                    \
    float w;                                    \
    float h;                                    \
    float vx;                                   \
    float vy;                                   \
    float ax;                                   \
    float ay;                                   \
    float last_vy;                              \
    bool onground

/*
  This might just turn into an uber struct or something.
*/
struct entity {
    KINEMATIC_ENITTY_BASE_BODY();

    /*temporary*/
    int facing_dir;
    bool dash;
    /*end temporary*/

    /* player specific */
    float jump_leniancy_timer;
};

void entity_halt_motion(struct entity* entity) {
    entity->ax = entity->ay = entity->vx = entity->vy = entity->last_vy = 0;
}

bool noclip = false;
struct entity player = {
    // no units, prolly pixels
    .x = -4,
    .y = -5,
    .w = 1/2.0f,
    .h = 1,
};

/*TODO(jerry): move some game state globals into this.*/
local struct memory_arena game_memory_arena;

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
    test_icon      = load_texture("assets/icon.png");
    test_font           = load_font("assets/Exoplanetaria-gxxJ5.ttf", font_size_aspect_ratio_independent(0.03));
    test_guy  = load_texture("assets/guy.png");
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

    game_memory_arena = allocate_memory_arena(Megabyte(128));
    game_memory_arena.name = "Game Arena";

    game_state = memory_arena_push(&game_memory_arena, sizeof(*game_state));
    initialize_particle_emitter_pool(&game_memory_arena);
    active_colorscheme = DEFAULT_mono;

    load_tilemap_editor_resources();
    gameplay_initialize();
    console_execute_cstr("load e");
    console_execute_cstr("noclip");
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

            console_printf("sz: %d, %d : %d %d\n", min_x, min_y, e, f);
            end_temporary_memory(&temp);
        }
    }

    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u8, game_state->loaded_level->transition_zone_count, game_state->loaded_level->transitions);
    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, u8, game_state->loaded_level->player_spawn_link_count, game_state->loaded_level->link_spawns);
}

/*binary format*/
void game_load_level_from_serializer(struct memory_arena* arena, struct binary_serializer* serializer, char* transition_link_to_spawn_at) {
    memory_arena_clear_top(arena);
    game_state->loaded_level = memory_arena_push_top(arena, sizeof(*game_state->loaded_level));

    game_serialize_level(arena, serializer);

    /*level is loaded, now setup player spawns*/
    if (transition_link_to_spawn_at) {
        for (unsigned index = 0; index < game_state->loaded_level->player_spawn_link_count; ++index) {
            struct player_spawn_link* spawn = game_state->loaded_level->link_spawns + index;
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
    camera_force_clamp_to_bounds(&game_camera);
}

void game_load_level(struct memory_arena* arena, char* filename, char* transition_link_to_spawn_at) {
    struct binary_serializer file = open_read_file_serializer(filename);
    game_load_level_from_serializer(arena, &file, transition_link_to_spawn_at);

    serializer_finish(&file);
    transitioning = true;
    transition_t = 0;
    transition_to = test1;
}

void update_render_frame(float dt) {
    clear_color(COLOR4F_BLACK);

    switch (mode) {
        case GAME_MODE_PLAYING: game_update_render_frame(dt); break;
        case GAME_MODE_EDITOR: tilemap_editor_update_render_frame(dt); break;
    }
}
