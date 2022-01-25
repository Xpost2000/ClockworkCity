struct game_state {
    struct tilemap* loaded_level;
};

/*TODO(jerry): move some game state globals into this.*/
local struct memory_arena game_memory_arena;
local struct game_state* game_state;

local texture_id knight_twoview;
local font_id    test_font;
local sound_id   test_sound;
local sound_id   test_sound2;

/*
  physics "constants"
*/

#define VPIXELS_PER_METER (16)

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
    bool onground;

    /* player specific */
    float jump_leniancy_timer;
};

enum game_mode {
    GAME_MODE_PLAYING,
    GAME_MODE_EDITOR,
    GAME_MODE_COUNT,
};

enum game_mode mode = GAME_MODE_EDITOR;
/* enum game_mode mode = GAME_MODE_PLAYING; */

#include "tilemap.c"
#include "gameplay_mode.c"
#include "tilemap_editor.c"

local void load_static_resources(void) {
    knight_twoview = load_texture("assets/knight_twoview.png");
    test_font      = load_font("assets/pxplusvga8.ttf", 16);

    test_sound     = load_sound("assets/emp.wav");
    test_sound2    = load_sound("assets/explosion_b.wav");

    game_memory_arena = allocate_memory_arena(Megabyte(8));

    game_state = memory_arena_push(&game_memory_arena, sizeof(*game_state));

    load_gameplay_resources();
    load_tilemap_editor_resources();
}

void update_render_frame(float dt) {
    clear_color(COLOR4F_BLACK);

    switch (mode) {
        case GAME_MODE_PLAYING: game_update_render_frame(dt); break;
        case GAME_MODE_EDITOR: tilemap_editor_update_render_frame(dt); break;
    }
}
