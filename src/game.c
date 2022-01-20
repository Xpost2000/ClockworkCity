texture_id knight_twoview;
font_id    test_font;

/* 
   all of this is temporary until I come up with a more easily
   hackable entity codebase. Or a "game core",
   
   today I just wanted to refigure out my basic physics stuff and then I'll punch it into a more basic
   platformer system.
   
   NOTE(jerry):
   need to come up with basic world units. Which is probably the only game specific thing I require this early. I guess
   that implies I figure out the art style too. Oh boy, bad pixel art here we go again! How bad can it be when I have a month?
*/

/*
  physics "constants"
*/

#define VPIXELS_PER_METER (40)
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

struct world_block {
    float x;
    float y;
    float w;
    float h;
};

struct entity player = {
    // no units, prolly pixels
    .x = 380,
    .y = 0,
    .w = 20,
    .h = VPIXELS_PER_METER,
};

int block_count = 3;
struct world_block blocks[999] = {
    /*floor*/
    {250, 500, 900, 40},
    /*wall*/
    {500, 450, 40, 50},
    /*ceiling */
    {600, 400, 80, 10},
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

    /* camera_set_position(player.x - player.w/2, player.y - player.h/2); */
}

void do_player_input(float dt) {
    player.vx = 0;
    if (is_key_down(KEY_D)) {
        player.vx = VPIXELS_PER_METER * 5;
    } else if (is_key_down(KEY_A)) {
        player.vx = VPIXELS_PER_METER * -5;
    }

    if (roundf(player.vy) == 0) {
        player.jump_leniancy_timer = 0.3;
    }

    if (is_key_pressed(KEY_SPACE)) {
        if (player.onground) {
            player.vy = VPIXELS_PER_METER * -10;
            player.onground = false;
        }
    }

    player.jump_leniancy_timer -= dt;
}

void do_physics(float dt) {
    player.vy += VPIXELS_PER_METER*13 * dt;

    {
        float old_player_x = player.x;
        player.x += player.vx * dt;

        for (int index = 0; index < block_count; ++index) {
            struct world_block block = blocks[index];

            if (rectangle_intersects(player.x, player.y, player.w, player.h, block.x, block.y, block.w, block.h)) {
                if (old_player_x + player.w <= block.x) {
                    player.x = block.x - player.w;
                } else if (old_player_x >= block.x + block.w) {
                    player.x = block.x + block.w;
                }
                player.vx = 0;
                break;
            }
        }
    }

    /* 
       vertical axis check is a bit more complicated because onground is a bit iffy to deal with like this.
       Due to the way collisions are resolved to avoid the way I usually resolve them with plain AABBs (which looks
       really inaccurate on my old platformy things). The touching vs. intersection check needs to happen separately
       
       That's fine though... Not many game entities that need an onground check any how I'm sure. The rest of the physics
       is pretty standard though.
    */
    {
        float old_player_y = player.y;
        player.y += player.vy * dt;

        int first_top_intersection = 0;
        player.onground = false;

        for (int index = 0; index < block_count; ++index) {
            struct world_block block = blocks[index];

            if (rectangle_intersects(player.x, player.y, player.w, player.h, block.x, block.y, block.w, block.h)) {
                if (old_player_y + player.h <= block.y) {
                    player.y = block.y - (player.h);
                    first_top_intersection = index;
                } else if (old_player_y >= block.y + block.h) {
                    player.y = block.y + block.h;
                }
                player.vy = 0;
                break;
            }
        }

        for (int index = first_top_intersection; index < block_count && !player.onground; ++index) {
            struct world_block block = blocks[index];
            player.onground =
                rectangle_touching(player.x, player.y, player.w, player.h, block.x, block.y, block.w, block.h)
                && (old_player_y + player.h <= block.y);
        }
    }
}

void update_render_frame(float dt) {
    begin_graphics_frame(); {
        clear_color(COLOR4F_BLACK);

        camera_set_focus_speed_x(12);
        camera_set_focus_speed_y(5);
        camera_set_focus_position(player.x - player.w/2, player.y - player.h/2);

        do_player_input(dt);
        do_physics(dt);

        for (int index = 0; index < block_count; ++index) {
            struct world_block* block = blocks + index;
            union color4f colors[] = {COLOR4F_RED, COLOR4F_BLUE, COLOR4F_GREEN};
            draw_filled_rectangle(block->x, block->y, block->w, block->h, colors[index%array_count(colors)]);
        }

        draw_filled_rectangle(player.x, player.y, player.w, player.h, color4f(0.3, 0.2, 1.0, 1.0));

        /*whoops*/
        /* camera_reset_transform(); */
        draw_text(test_font, 0, 0,
                  format_temp("onground: %d\npx: %f\npy:%15.15f\npvx: %f\npvy: %f\n",
                              player.onground,
                              player.x, player.y, player.vx, player.vy),
        COLOR4F_WHITE);
    } end_graphics_frame();
}
