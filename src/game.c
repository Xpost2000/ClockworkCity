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
    float ax;
    float ay;

    float vx;
    float vy;

    bool onground;

    /*temporary*/
    int facing_dir;
    bool dash;
    /*end temporary*/

    /* player specific */
    float jump_leniancy_timer;
};

struct game_state {
    /*whoops this is an additional indirection. Fix this at the end of the night*/
    struct tilemap* loaded_level;
};

/*TODO(jerry): move some game state globals into this.*/
local struct memory_arena game_memory_arena;
local struct game_state* game_state;

local texture_id knight_twoview;
local font_id    test_font;
local font_id    test2_font;
local sound_id   test_sound;
local sound_id   test_sound2;

enum game_mode {
    GAME_MODE_PLAYING,
    GAME_MODE_EDITOR,
    GAME_MODE_COUNT,
};

/* enum game_mode mode = GAME_MODE_EDITOR; */
enum game_mode mode = GAME_MODE_PLAYING;

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
#include "gameplay_mode.c"
#include "tilemap_editor.c"

local void load_graphics_resources(void) {
    /* can change depending on resolution maybe... */
    knight_twoview = load_texture("assets/knight_twoview.png");
    test_font      = load_font("assets/Exoplanetaria-gxxJ5.ttf", font_size_aspect_ratio_independent(0.02));
    /* test2_font      = load_font("assets/Exoplanetaria-gxxJ5.ttf", 64); */
    /* test2_font      = load_font("assets/charissilbold.ttf", 64); */
    /* test2_font      = load_font("assets/Helmet-lWZV.otf", 64); */

    test2_font      = load_font("assets/Exo2Medium-aDL9.ttf", font_size_aspect_ratio_independent(0.07));
    /* test2_font      = load_font("assets/TwentyOne-nRmJ.ttf", 64); */
    load_gameplay_resources();
}

local void load_static_resources(void) {
    load_graphics_resources();

    test_sound     = load_sound("assets/emp.wav");
    test_sound2    = load_sound("assets/explosion_b.wav");

    game_memory_arena = allocate_memory_arena(Megabyte(48));
    game_state = memory_arena_push(&game_memory_arena, sizeof(*game_state));

    load_tilemap_editor_resources();

    console_execute_cstr("editor_load ts1");
    console_execute_cstr("editor");
}
/*
  This is really reloading all graphical assets... But anyways.
 */
local void reload_all_graphics_resources(void) {
    unload_all_graphics_resources();
    load_graphics_resources();
}

void update_render_frame(float dt) {
    clear_color(COLOR4F_BLACK);

    switch (mode) {
        case GAME_MODE_PLAYING: game_update_render_frame(dt); break;
        case GAME_MODE_EDITOR: tilemap_editor_update_render_frame(dt); break;
    }
}
