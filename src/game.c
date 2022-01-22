texture_id knight_twoview;
font_id    test_font;
sound_id   test_sound;
sound_id   test_sound2;

/*
  TODO(jerry):
  Patch up and clean up the slopes implementation
  FIXME(jerry):
  Slopes ignore the rest of the world when doing their advancing thing.

  IE: when you slope upto a wall. You go through it.

  Uh this shouldn't really be difficult to do. The map is already stored as a 2D array,
  I just need to grab the neighbors within the direction of movement
  
  0 = slope
  
                XXX
                 0
  This should be all I need to grab... (of course this depends on the entity climbing!
  If it's a big entity... Well, we'll be grabbing lots of tiles to make sure we don't smash into shit I guess.
  As long as most entities are close to the players size though I don't have to worry about this.)
*/

/*
  world_tilemap temporary experimental work
  
  using a "fat format", as I try to figure out what works okay, and I clean up everything after I have
  something that's a little more acceptable since I have so little code to help write anything right now lololo.
*/

const int TILE_TEX_SIZE = 16;
enum tile_id {
    TILE_NONE, /*shouldn't happen but okay*/
    TILE_SOLID,
    TILE_SLOPE_BL,
    TILE_SLOPE_BR,
    TILE_SLOPE_R,
    TILE_SLOPE_L,
    TILE_ID_COUNT,
};
char* tile_id_filestrings[] = {
    0,
    "assets/testtiles/block.png",
    "assets/testtiles/slope45degbl.png",
    "assets/testtiles/slope45degbr.png",
    "assets/testtiles/slope45degr.png",
    "assets/testtiles/slope45degl.png",
};
texture_id tile_textures[TILE_ID_COUNT] = {};
void DEBUG_load_all_tile_assets(void) {
    for (unsigned i = 0; i < TILE_ID_COUNT; ++i) {
        char* fstring = tile_id_filestrings[i];
        if (fstring) {
            tile_textures[i] = load_texture(fstring);
        }
    }
}

struct tile {
    uint32_t id;
};
/* should be "streamed" */
struct tilemap {
    uint16_t width;
    uint16_t height;
    struct tile* tiles;
};

/* partially stupid but whatever, just something to try out tonight */
/*
  really bad ascii format for debugging reasons, could use getline or something
  or split into line buffers?

  ehh....
*/
struct tilemap DEBUG_tilemap_from_file(char* file) {
    struct tilemap result = {};
    char* filestring = load_entire_file(file);

    {
        result.height = count_lines_of_cstring(filestring);
        result.width = 0;

        int index = 0;
        char* current_line;

        while (current_line = get_line_starting_from(filestring, &index)){
            int current_length = strlen(current_line);
            if (result.width < current_length) {
                result.width = current_length;
            }
        }
    }

    result.tiles = system_allocate_zeroed_memory(result.width * result.height * sizeof(*result.tiles));
    printf("%d x %d\n", result.width, result.height);

    {

        int index = 0;
        char* current_line;

        for(unsigned y = 0; y < result.height; ++y) {
            current_line = get_line_starting_from(filestring, &index);

            for(unsigned x = 0; x < result.width; ++x) {
                struct tile* t = &result.tiles[y * result.width + x];

                switch (current_line[x]) {
                    case '.': {
                    } break;
                    case '#': {
                        t->id = TILE_SOLID;
                    } break;
                    case '>': {
                        t->id = TILE_SLOPE_BR;
                    } break;
                    case '<': {
                        t->id = TILE_SLOPE_BL;
                    } break;
                    case '/': {
                        t->id = TILE_SLOPE_L;
                    } break;
                    case '\\': {
                        t->id = TILE_SLOPE_R;
                    } break;
                }
            }
        }
    }

    system_deallocate_memory(filestring);
    return result;
}

void DEBUG_draw_tilemap(struct tilemap* tilemap) {
    for(unsigned y = 0; y < tilemap->height; ++y) {
        for(unsigned x = 0; x < tilemap->width; ++x) {
            struct tile* t = &tilemap->tiles[y * tilemap->width + x];
            draw_texture(tile_textures[t->id], x * TILE_TEX_SIZE, y * TILE_TEX_SIZE, TILE_TEX_SIZE, TILE_TEX_SIZE, COLOR4F_WHITE);
        }
    }
}

struct tilemap global_test_tilemap = {};

/*
  physics "constants"
*/

#define VPIXELS_PER_METER (16)

struct entity {
    float x;
    float y;
    float w;
    float h;

    /* only acceleration is gravity for now. Don't care about other forces atm */
    float vx;
    float vy;
    bool onground;
    float jump_leniancy_timer;
};

struct entity player = {
    // no units, prolly pixels
    .x = -VPIXELS_PER_METER/4,
    .y = 0,
    .w = VPIXELS_PER_METER/2,
    .h = VPIXELS_PER_METER,
};

bool rectangle_intersects(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    float x1min = x1;
    float x1max = x1 + w1;

    float y1min = y1;
    float y1max = y1 + h1;

    float x2min = x2;
    float x2max = x2 + w2;

    float y2min = y2;
    float y2max = y2 + h2;

    return (x1min < x2max && x1max > x2min) && (y1min < y2max && y1max > y2min);
}

/*yes there's a difference*/
bool rectangle_touching(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    float x1min = x1;
    float x1max = x1 + w1;

    float y1min = y1;
    float y1max = y1 + h1;

    float x2min = x2;
    float x2max = x2 + w2;

    float y2min = y2;
    float y2max = y2 + h2;

    return (x1min <= x2max && x1max >= x2min) && (y1min <= y2max && y1max >= y2min);
}

void load_static_resources(void) {
    knight_twoview = load_texture("assets/knight_twoview.png");
    test_font      = load_font("assets/pxplusvga8.ttf", 16);
    test_sound     = load_sound("assets/emp.wav");
    test_sound2    = load_sound("assets/explosion_b.wav");

    /* camera_set_position(player.x - player.w/2, player.y - player.h/2); */
    DEBUG_load_all_tile_assets();
    global_test_tilemap = DEBUG_tilemap_from_file("assets/testmap.txt");
}

void do_player_input(float dt) {
    struct game_controller* gamepad = get_gamepad(0);

    player.vx = 0;
    const float MOVEMENT_THRESHOLD = 0.5;

    bool move_right = is_key_down(KEY_D) || gamepad->buttons[DPAD_RIGHT] || gamepad->left_stick.axes[0] >= MOVEMENT_THRESHOLD;
    bool move_left  = is_key_down(KEY_A) || gamepad->buttons[DPAD_LEFT] || gamepad->left_stick.axes[0] <= -MOVEMENT_THRESHOLD;

    if (move_right) {
        player.vx = VPIXELS_PER_METER * 5;
    } else if (move_left) {
        player.vx = VPIXELS_PER_METER * -5;
    }

    if (roundf(player.vy) == 0) {
        player.jump_leniancy_timer = 0.3;
    }

    if (is_key_pressed(KEY_SPACE) || gamepad->buttons[BUTTON_A]) {
        if (player.onground) {
            player.vy = VPIXELS_PER_METER * -10;
            player.onground = false;
            play_sound(test_sound);
        }
    }

    player.jump_leniancy_timer -= dt;
}

void do_physics(float dt) {
    struct tilemap* tilemap = &global_test_tilemap;
    player.vy += VPIXELS_PER_METER*13 * dt;

    {
        float old_player_x = player.x;
        player.x += player.vx * dt;

        for(unsigned y = 0; y < tilemap->height; ++y) {
            for(unsigned x = 0; x < tilemap->width; ++x) {
                struct tile* t = &tilemap->tiles[y * tilemap->width + x];

                float tile_x = x * TILE_TEX_SIZE;
                float tile_y = y * TILE_TEX_SIZE;
                float tile_w = TILE_TEX_SIZE;
                float tile_h = TILE_TEX_SIZE;

                if(t->id == TILE_NONE) continue;

                if (rectangle_intersects(player.x, player.y, player.w, player.h, tile_x, tile_y, tile_w, tile_h)) {
                    switch (t->id) {
                        case TILE_SOLID: {
                            if (old_player_x + player.w <= tile_x) {
                                player.x = tile_x - player.w;
                            } else if (old_player_x >= tile_x + tile_w) {
                                player.x = tile_x + tile_w;
                            }
                        } break;
                        case TILE_SLOPE_L: {
                            if (old_player_x >= tile_x + tile_w) {
                                player.x = tile_x + tile_w;
                            } else {
                                float slope_x_offset = ((player.x + player.w) - tile_x);

                                float player_slope_snapped_location = ((tile_y + tile_h) - slope_x_offset) - player.h;
                                float delta_from_foot_to_tile_top = (player_slope_snapped_location - player.y);

                                if (player.y + player.h <= tile_y + tile_h) {
                                    if (player.vy >= 0 && (player.y >= player_slope_snapped_location || delta_from_foot_to_tile_top <= ((float)TILE_TEX_SIZE/2.25))) {
                                        player.y = player_slope_snapped_location;
                                    }
                                } else {
                                    /*this is technically still wrong. It looks okay enough. I'll probably refine it later or design
                                     levels that avoid this physics quirk... Until I can figure it out*/
                                    if (old_player_x + player.w <= tile_x) {
                                        player.x = tile_x - player.w;
                                    } else {
                                        player.y = tile_y + tile_h;
                                        player.vy = 0;
                                    }
                                }
                            }
                        } break;
                        case TILE_SLOPE_R: {
                            if (old_player_x + player.w <= tile_x) {
                                player.x = tile_x - player.w;
                            } else {
                                float slope_x_offset = (tile_x - player.x);

                                float player_slope_snapped_location = (tile_y - slope_x_offset) - player.h;
                                float delta_from_foot_to_tile_top = (player.y - player_slope_snapped_location);

                                if (player.y + player.h <= tile_y + tile_h) {
                                    if (player.vy >= 0 && (player.y >= player_slope_snapped_location || delta_from_foot_to_tile_top <= ((float)TILE_TEX_SIZE/2.25))) {
                                        player.y = player_slope_snapped_location;
                                    }
                                } else {
                                    if (old_player_x >= tile_x + tile_w) {
                                        player.x = tile_x + tile_w;
                                    } else {
                                        player.y = tile_y + tile_h;
                                        player.vy = 0;
                                    }
                                }
                            }
                        } break;
                    }

                    player.vx = 0;
                    goto finished_horizontal_tile_collision;
                }
            }
        }
    finished_horizontal_tile_collision:
    }

    {
        float old_player_y = player.y;
        player.y += player.vy * dt;

        for(unsigned y = 0; y < tilemap->height; ++y) {
            for(unsigned x = 0; x < tilemap->width; ++x) {
                struct tile* t = &tilemap->tiles[y * tilemap->width + x];

                float tile_x = x * TILE_TEX_SIZE;
                float tile_y = y * TILE_TEX_SIZE;
                float tile_w = TILE_TEX_SIZE;
                float tile_h = TILE_TEX_SIZE;

                if (rectangle_intersects(player.x, player.y, player.w, player.h, tile_x, tile_y, tile_w, tile_h)) {
                    switch (t->id) {
                        case TILE_SOLID: {
                            if (old_player_y + player.h <= tile_y) {
                                player.y = tile_y - (player.h);
                            } else if (old_player_y >= tile_y + tile_h) {
                                player.y = tile_y + tile_h;
                            }
                            player.vy = 0;
                            goto confirmed_tile_collision;
                        } break;
                        case TILE_SLOPE_L: {
                            float player_slope_snapped_location = ((tile_y + tile_h) - ((player.x + player.w) - tile_x)) - player.h;

                            if (player.vy >= 0 && roundf(old_player_y) == roundf(player_slope_snapped_location)) {
                                player.vy = 0;
                                goto confirmed_tile_collision;
                            }
                        } break;
                        case TILE_SLOPE_R: {
                            float player_slope_snapped_location = (tile_y - (tile_x - player.x)) - player.h;
                            if (player.vy >= 0 && roundf(old_player_y) == roundf(player_slope_snapped_location)) {
                                player.vy = 0;
                                goto confirmed_tile_collision;
                            }
                        } break;
                    }

                }
            }
        }
    confirmed_tile_collision:
        {
            bool was_on_ground = player.onground;
            player.onground = false;
            /*stupid double scan*/
            for(unsigned y = 0; y < tilemap->height; ++y) {
                for(unsigned x = 0; x < tilemap->width; ++x) {
                    struct tile* t = &tilemap->tiles[y * tilemap->width + x];

                    float tile_x = x * TILE_TEX_SIZE;
                    float tile_y = y * TILE_TEX_SIZE;
                    float tile_w = TILE_TEX_SIZE;
                    float tile_h = TILE_TEX_SIZE;

                    if(t->id == TILE_NONE) continue;
                    if (!rectangle_touching(player.x, player.y, player.w, player.h, tile_x, tile_y, tile_w, tile_h)) continue;

                    /*NOTE(jerry): this is slightly... different for sloped tiles.*/

                    if ((old_player_y + player.h <= tile_y)) {
                        player.onground = true;
                    }

                    switch (t->id) {
                        case TILE_SLOPE_L: {
                            float player_slope_snapped_location = ((tile_y + tile_h) - ((player.x + player.w) - tile_x)) - player.h;

                            if (roundf(player.y) == roundf(player_slope_snapped_location)) {
                                player.onground = true;
                            } else {
                                player.onground = false;
                            }
                        } break;
                        case TILE_SLOPE_R: {
                            float player_slope_snapped_location = (tile_y - (tile_x - player.x)) - player.h;
                            if (roundf(player.y) == roundf(player_slope_snapped_location)) {
                                player.onground = true;
                            } else {
                                player.onground = false;
                            } 
                        } break;
                    }

                    if (player.onground) {
                        goto done;
                    }
                }
            }

        done:
        }
    }
}

void update_render_frame(float dt) {
    clear_color(COLOR4F_BLACK);

    do_player_input(dt);
    do_physics(dt);

    begin_graphics_frame(); {
        /* might need to rethink camera interface. 
           I still want it to operate under one global camera, but
           obviously you don't always want the camera. */
        set_active_camera(get_global_camera());

        camera_set_focus_speed_x(12);
        camera_set_focus_speed_y(5);
        camera_set_focus_position(player.x - player.w/2, player.y - player.h/2);

        draw_filled_rectangle(player.x, player.y, player.w, player.h, color4f(0.3, 0.2, 1.0, 1.0));
        DEBUG_draw_tilemap(&global_test_tilemap);
    } end_graphics_frame();

    begin_graphics_frame(); {
        draw_text(test_font, 0, 0,
                  format_temp("onground: %d\npx: %f\npy:%15.15f\npvx: %f\npvy: %f\n",
                              player.onground,
                              player.x, player.y, player.vx, player.vy),
                  COLOR4F_WHITE);
    } end_graphics_frame();
}
