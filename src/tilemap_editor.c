#include "memory_arena.h"
#define EDITOR_TILE_MAX_COUNT (16384)

/*
  NOTE(jerry):
  CTRL and drag for rectangular region picking!
*/

struct editable_tilemap {
    uint32_t tile_count;
    int width;
    int height;
    struct tile* tiles;
};
struct editor_state {
    struct memory_arena arena;

    float camera_x;
    float camera_y;
    /*
      This is a slightly different layout to the ingame format.
      This is basically a source format
    */
    struct editable_tilemap tilemap;

    /*-*/
    bool selection_region_exists;
    /*for some reason I decide to store this in pixels. Change this later.*/
    struct rectangle selection_region;
    /*this is memoized separately from selection_region. Only changes on resize.*/
    struct rectangle selected_tile_region;

    enum tile_id placement_type;
};

local struct editor_state editor;

local void editor_begin_selection_region(int x, int y) {
    if (editor.selection_region_exists)
        return;

    editor.selection_region_exists = true;
    editor.selection_region.x = floorf((float)x / TILE_TEX_SIZE) * TILE_TEX_SIZE;
    editor.selection_region.y = floorf((float)y / TILE_TEX_SIZE) * TILE_TEX_SIZE;
    console_printf("started region selection at (x:%d(tx: %d), y:%d(ty: %d))\n", x, (int)editor.selection_region.x, y, (int)editor.selection_region.y);
}

local void editor_end_selection_region(void) {
    editor.selection_region_exists = false;
    editor.selection_region = (struct rectangle) {};
    editor.selected_tile_region = (struct rectangle) {};
}

local void get_mouse_location_in_camera_space(int* x, int* y) {
    get_mouse_location(x, y);
    transform_point_into_camera_space(x, y);
}

/*abstract this later.*/
struct tile* existing_block_at(struct tile* tiles, int tile_count, int grid_x, int grid_y) {
    for (unsigned index = 0; index < tile_count; ++index) {
        struct tile* t = tiles + index;

        if (t->id == TILE_NONE) continue;
        if (t->x == grid_x && t->y == grid_y) {
            return t;
        }
    }

    return NULL;
}

/*yes this is different.*/
struct tile* occupied_block_at(struct tile* tiles, int tile_count, int grid_x, int grid_y) {
    for (unsigned index = 0; index < tile_count; ++index) {
        struct tile* t = tiles + index;

        if (t->x == grid_x && t->y == grid_y) {
            return t;
        }
    }

    return NULL;
}

/* TODO(jerry): negatives are bad! */
local bool editor_has_tiles_within_selection(void) {
    struct rectangle tile_region = editor.selected_tile_region;

    for (int y = (int)tile_region.y; y <= (int)(tile_region.y+tile_region.h); ++y) {
        for (int x = (int)tile_region.x; x <= (int)(tile_region.x+tile_region.w); ++x) {
            struct tile* t = existing_block_at(editor.tilemap.tiles, editor.tilemap.tile_count, x, y);
            if (t) {
                return true;
            }
        }
    }

    return false;
}

local void editor_move_selected_tile_region(void) {
    struct rectangle tile_region     = editor.selected_tile_region;
    struct rectangle selected_region = editor.selection_region;

    /* rescale into grid coordinates */ {
        selected_region.x /= TILE_TEX_SIZE;
        selected_region.y /= TILE_TEX_SIZE;
        selected_region.w /= TILE_TEX_SIZE;
        selected_region.h /= TILE_TEX_SIZE;
    }

    assert(tile_region.w == selected_region.w);
    assert(tile_region.h == selected_region.h);

    /* copy into new area */ {
        for (int y = tile_region.y; y <= (int)(tile_region.y+tile_region.h); ++y) {
            for (int x = tile_region.x; x <= (int)(tile_region.x+tile_region.w); ++x) {
                struct tile* t = existing_block_at(editor.tilemap.tiles, editor.tilemap.tile_count, x, y);

                if (t) {
                    /*heart surgery*/{
                        int relative_position_of_tile_x = (x - tile_region.x);
                        int relative_position_of_tile_y = (y - tile_region.y);

                        struct tile* replace_target = occupied_block_at(editor.tilemap.tiles, editor.tilemap.tile_count,
                                                                        relative_position_of_tile_x + selected_region.x,
                                                                        relative_position_of_tile_y + selected_region.y);
                        if (!replace_target) {
                            replace_target = &editor.tilemap.tiles[editor.tilemap.tile_count++];
                        }

                        replace_target->id = t->id;
                        replace_target->x =  relative_position_of_tile_x + selected_region.x;
                        replace_target->y =  relative_position_of_tile_y + selected_region.y;
                    }

                    /*kill original block*/{
                        t->id = TILE_NONE;
                    }
                }
            }
        }
    }
}

local void editor_try_to_place_block(int grid_x, int grid_y) {
    assert(editor.tilemap.tile_count < EDITOR_TILE_MAX_COUNT);

    struct tile* tile = existing_block_at(editor.tilemap.tiles, editor.tilemap.tile_count, grid_x, grid_y);

    if (!tile) {
        tile = &editor.tilemap.tiles[editor.tilemap.tile_count++];
    } else {
    }

    tile->x = grid_x;
    tile->y = grid_y;

    tile->id = editor.placement_type;
}

local void editor_erase_block(int grid_x, int grid_y) {
    struct tile* tile = existing_block_at(editor.tilemap.tiles, editor.tilemap.tile_count, grid_x, grid_y);

    if (!tile) {
        return;
    }

    tile->id = TILE_NONE;
}

local void editor_clear_all(void) {
    editor.tilemap.tile_count = 0;
    editor.camera_x = 0;
    editor.camera_y = 0;
}

local void load_tilemap_editor_resources(void) {
    editor.arena = allocate_memory_arena(Megabyte(2));
    editor.tilemap.tiles = memory_arena_push(&editor.arena, EDITOR_TILE_MAX_COUNT * sizeof(*editor.tilemap.tiles));
    size_t memusage = memory_arena_total_usage(&editor.arena);
    console_printf("Arena is using %d bytes, (%d kb) (%d mb) (%d gb)\n",
                   memusage, memusage / 1024,
                   memusage / (1024 * 1024), memusage / (1024*1024*1024));
}

/*braindead code for the night
  not thinking much of how to do it or any abstractions loololol*/
local void editor_output_to_binary_file(char* filename) {
    FILE* f = fopen(filename, "wb+");

    char magic[] = "LOVEYOU!";

    fwrite(magic, 8, 1, f);

    int width, height;
    get_bounding_rectangle_for_tiles(editor.tilemap.tiles, editor.tilemap.tile_count, NULL/*x*/, NULL/*y*/, &width, &height);
    console_printf("width height: %d, %d\n", width, height);

    fwrite(&width, sizeof(width), 1, f);
    fwrite(&height, sizeof(height), 1, f);
    fwrite(&editor.tilemap.tile_count, sizeof(editor.tilemap.tile_count), 1, f);
    fwrite(editor.tilemap.tiles, sizeof(*editor.tilemap.tiles) * editor.tilemap.tile_count, 1, f);

    fclose(f);
}

local void editor_load_from_binary_file(char* filename) {
    FILE* f = fopen(filename, "rb+");

    if (!f) return;

    char magic[8] = {};

    fread(magic, 8, 1, f);
    assert(strncmp(magic, "LOVEYOU!", 8) == 0);

    fread(&editor.tilemap.width, sizeof(editor.tilemap.width), 1, f);
    fread(&editor.tilemap.height, sizeof(editor.tilemap.height), 1, f);
    fread(&editor.tilemap.tile_count, sizeof(editor.tilemap.tile_count), 1, f);
    fread(editor.tilemap.tiles, sizeof(*editor.tilemap.tiles) * editor.tilemap.tile_count, 1, f);

    fclose(f);

    editor.camera_x = 0;
    editor.camera_y = 0;
}

static void editor_serialize_into_game_memory(void) {
    int min_x;
    int min_y;
    get_bounding_rectangle_for_tiles(editor.tilemap.tiles, editor.tilemap.tile_count, &min_x, &min_y,
                                     &editor.tilemap.width, &editor.tilemap.height);

    struct temporary_arena temp = begin_temporary_memory(&editor.arena, editor.tilemap.width * editor.tilemap.height * sizeof(*editor.tilemap.tiles));
    struct tile* tilemap_rectangle = memory_arena_push(&temp, editor.tilemap.width * editor.tilemap.height * sizeof(*editor.tilemap.tiles));

    {
        zero_buffer_memory(tilemap_rectangle, editor.tilemap.width * editor.tilemap.height * sizeof(*editor.tilemap.tiles));

        for (size_t index = 0; index < editor.tilemap.tile_count; ++index) {
            struct tile* t = editor.tilemap.tiles + index;
            int index_y = ((t->y) - (min_y));
            int index_x = ((t->x) - (min_x));
            assert(index_y >= 0 && index_x >= 0);
            int index_mapped = index_y * editor.tilemap.width + index_x;
            assert(index_mapped >= 0 && index_mapped < editor.tilemap.width * editor.tilemap.height);
            tilemap_rectangle[index_mapped] = *t;
        }
    }

    memory_arena_clear_top(&game_memory_arena);
    game_state->loaded_level = memory_arena_push_top(&game_memory_arena, sizeof(*game_state->loaded_level));
    game_state->loaded_level->width  = editor.tilemap.width;
    game_state->loaded_level->height = editor.tilemap.height;

    game_state->loaded_level->tiles  = memory_arena_copy_buffer_top(&game_memory_arena, tilemap_rectangle,
                                                                    editor.tilemap.width * editor.tilemap.height * sizeof(*editor.tilemap.tiles));
    end_temporary_memory(&temp);
}

local void draw_grid(float x_offset, float y_offset, int rows, int cols, float thickness, union color4f color) {
    int direction_x = 1;
    int direction_y = 1;

    if (rows < 0) { direction_y *= -1; rows *= -1; }
    if (cols < 0) { direction_x *= -1; cols *= -1; }

    if (thickness == 1) {
        /*NOTE(jerry):
          one sized filled rectangles sometimes flicker out of existance with the current renderer.
         */
        for (int y = 0; y <= rows; ++y) {
            draw_rectangle(x_offset, y * TILE_TEX_SIZE*(direction_y) + y_offset,
                           direction_x * cols * TILE_TEX_SIZE, 1, color);
        }
        for (int x = 0; x <= cols; ++x) {
            draw_rectangle(x * TILE_TEX_SIZE*(direction_x) + x_offset, y_offset, 1,
                           direction_y * rows * TILE_TEX_SIZE, color);
        }
    } else {
        for (int y = 0; y <= rows; ++y) {
            draw_filled_rectangle(x_offset, y * TILE_TEX_SIZE * (direction_y) + y_offset,
                                  direction_x * cols * TILE_TEX_SIZE, thickness, color);
        }
        for (int x = 0; x <= cols; ++x) {
            draw_filled_rectangle(x * TILE_TEX_SIZE * (direction_x) + x_offset, y_offset,
                                  thickness, direction_y * rows * TILE_TEX_SIZE, color);
        }
    }
}

local void tilemap_editor_update_render_frame(float dt) {
    /*
      TODO(jerry):
      hack, cause my camera api is shitty.

      I'll fix it at the end of the week so this is okay for now.
     */
    set_active_camera(get_global_camera());
    camera_set_position(editor.camera_x, editor.camera_y);

    if (is_key_pressed(KEY_RIGHT)) {
        editor.placement_type++;
    } else if (is_key_pressed(KEY_LEFT)) {
        editor.placement_type--;
    }

    /*camera editor movement*/
    {
        const int CAMERA_SPEED = 350;

        if (is_key_down(KEY_W)) {
            editor.camera_y -= dt * CAMERA_SPEED;
        } else if (is_key_down(KEY_S)) {
            editor.camera_y += dt * CAMERA_SPEED;
        }

        if (is_key_down(KEY_A)) {
            editor.camera_x -= dt * CAMERA_SPEED;
        } else if (is_key_down(KEY_D)) {
            editor.camera_x += dt * CAMERA_SPEED;
        }
    }

    /*editor rectangular region related stuff*/
    {

        if (is_key_pressed(KEY_ESCAPE)) {
            editor_end_selection_region();
        }

        if (is_key_pressed(KEY_RETURN)) {
            if (editor_has_tiles_within_selection()) {
                editor_move_selected_tile_region();
                editor_end_selection_region();
            }
        }

        if (is_key_down(KEY_CTRL)) {
            int mouse_position[2];
            get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
            editor_begin_selection_region(mouse_position[0], mouse_position[1]);

            /* resize region */
            if (editor.selection_region_exists) {
                int distance_delta_x = ((mouse_position[0] / TILE_TEX_SIZE) * TILE_TEX_SIZE) - editor.selection_region.x;
                int distance_delta_y = ((mouse_position[1] / TILE_TEX_SIZE) * TILE_TEX_SIZE) - editor.selection_region.y;
                editor.selection_region.w = distance_delta_x;
                editor.selection_region.h = distance_delta_y;

                editor.selected_tile_region = editor.selection_region;
                editor.selected_tile_region.x /= TILE_TEX_SIZE;
                editor.selected_tile_region.y /= TILE_TEX_SIZE;
                editor.selected_tile_region.w /= TILE_TEX_SIZE;
                editor.selected_tile_region.h /= TILE_TEX_SIZE;
            }
        } else {
            struct rectangle region = editor.selected_tile_region;

            bool should_stop = true;
            for (int y = 0; y < (int)region.h; ++y) {
                for (int x = 0; x < (int)region.w; ++x) {
                    if (existing_block_at(editor.tilemap.tiles, editor.tilemap.tile_count, x + region.x, y + region.y)) {
                        should_stop = false;
                        goto end_of_loop_1;
                    }
                }
            }
        end_of_loop_1:
            if (should_stop) editor_end_selection_region();
        }

        if (editor.selection_region_exists) {
            struct rectangle region = editor.selection_region;

            region.y /= TILE_TEX_SIZE;
            region.x /= TILE_TEX_SIZE;
            region.w /= TILE_TEX_SIZE;
            region.h /= TILE_TEX_SIZE;

            if (is_key_pressed(KEY_F)) {
                for (int y = 0; y < (int)region.h; ++y) {
                    for (int x = 0; x < (int)region.w; ++x) {
                        editor_try_to_place_block(x + (int)region.x, y + (int)region.y);
                    }
                }

                editor_end_selection_region();
            } else if (is_key_pressed(KEY_X)) {
                for (int y = 0; y < (int)region.h; ++y) {
                    for (int x = 0; x < (int)region.w; ++x) {
                        editor_erase_block(x + (int)region.x, y + (int)region.y);
                    }
                }

                editor_end_selection_region();
            }

            if (is_key_down(KEY_M)) {
                int mouse_position[2];
                get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
                editor.selection_region.x = floorf((float)mouse_position[0] / TILE_TEX_SIZE) * TILE_TEX_SIZE;
                editor.selection_region.y = floorf((float)mouse_position[1] / TILE_TEX_SIZE) * TILE_TEX_SIZE;
            }
        }
    }

    editor.placement_type = clampi(editor.placement_type, TILE_SOLID, TILE_ID_COUNT-1);

    begin_graphics_frame(); {
        set_active_camera(get_global_camera());
        camera_set_position(editor.camera_x, editor.camera_y);

        /*grid*/
        {
            int screen_dimensions[2];
            get_screen_dimensions(screen_dimensions, screen_dimensions+1);

            /*
              TODO(jerry):
              too lazy to do a real infinite grid tonight.

              It'd just be repositioning constantly basically. But this works fine too.
              I'm just drawing multiple screens of grids that you can expect to "see".
              
              It's whatever for this.
             */
            const int SCREENFUL_FILLS = 100;

            int row_count = (screen_dimensions[1]) / TILE_TEX_SIZE;
            int col_count = (screen_dimensions[0]) / TILE_TEX_SIZE;

            int x_offset = -SCREENFUL_FILLS/2 * (col_count * TILE_TEX_SIZE);
            int y_offset = -SCREENFUL_FILLS/2 * (row_count * TILE_TEX_SIZE);

            draw_grid(x_offset, y_offset, row_count * SCREENFUL_FILLS,
                      col_count * SCREENFUL_FILLS, 1, COLOR4F_DARKGRAY);
        }

        /* cursor */
        {
            int mouse_position[2];
            bool left_click, right_click;

            get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
            get_mouse_buttons(&left_click, 0, &right_click);

            {
                mouse_position[0] = floorf((float)mouse_position[0] / TILE_TEX_SIZE) * TILE_TEX_SIZE;
                mouse_position[1] = floorf((float)mouse_position[1] / TILE_TEX_SIZE) * TILE_TEX_SIZE;
            }

            {
                const int SURROUNDER_VISUAL_SIZE = 5;
                draw_grid(mouse_position[0]-(SURROUNDER_VISUAL_SIZE/2)*TILE_TEX_SIZE,
                          mouse_position[1]-(SURROUNDER_VISUAL_SIZE/2)*TILE_TEX_SIZE,
                          SURROUNDER_VISUAL_SIZE, SURROUNDER_VISUAL_SIZE,
                          ((sinf(global_elapsed_time*4) + 1)/2.0f) * 2.0f + 0.2f, COLOR4F_BLUE);
            }

            draw_texture(tile_textures[editor.placement_type],
                         mouse_position[0], mouse_position[1],
                         TILE_TEX_SIZE, TILE_TEX_SIZE,
                         color4f(1, 1, 1, 0.5 + 0.5 * ((sinf(global_elapsed_time) + 1) / 2.0f)));

            if (left_click) {
                editor_try_to_place_block(mouse_position[0]/TILE_TEX_SIZE,
                                          mouse_position[1]/TILE_TEX_SIZE);
            } else if (right_click) {
                editor_erase_block(mouse_position[0]/TILE_TEX_SIZE,
                                   mouse_position[1]/TILE_TEX_SIZE);
            }
        }

        /* world */
        {
            struct tile* tiles = editor.tilemap.tiles;
            for (unsigned index = 0; index < editor.tilemap.tile_count; ++index) {
                struct tile* t = &tiles[index];

                /*exclude tiles in the region to avoid a copy. Would like to have a hashmap :/*/
                if (editor.selection_region_exists) {
                    float tile_x = t->x;
                    float tile_y = t->y;
                    float tile_w = 1;
                    float tile_h = 1;

                    struct rectangle region = editor.selected_tile_region;
                    if (rectangle_intersects_v(tile_x, tile_y, tile_w, tile_h,
                                               region.x, region.y, region.w, region.h)) {
                        continue;
                    }
                }

                draw_texture(tile_textures[t->id],
                             t->x * TILE_TEX_SIZE, t->y * TILE_TEX_SIZE,
                             TILE_TEX_SIZE, TILE_TEX_SIZE, COLOR4F_WHITE);
            } 
        }

        /*rectangle picker */
        {
            if (editor.selection_region_exists) {
                struct rectangle region = editor.selection_region;
                draw_grid(region.x, region.y,
                          roundf(region.h / TILE_TEX_SIZE), roundf(region.w / TILE_TEX_SIZE),
                          ((sinf(global_elapsed_time*8) + 1)/2.0f) * 2.0f + 0.5f, COLOR4F_RED);

                /* draw tiles within the region for moving regions */
                {
                    struct tile* tiles = editor.tilemap.tiles;
                    for (unsigned index = 0; index < editor.tilemap.tile_count; ++index) {
                        struct tile* t = &tiles[index];

                        /*exclude tiles in the region to avoid a copy. Would like to have a hashmap :/*/
                        float tile_x = t->x;
                        float tile_y = t->y;
                        float tile_w = 1;
                        float tile_h = 1;

                        {
                            struct rectangle region = editor.selected_tile_region;
                            if (!rectangle_intersects_v(tile_x, tile_y, tile_w, tile_h,
                                                        region.x, region.y, region.w, region.h)) {
                                continue;
                            }
                        }

                        {
                            struct rectangle selected_region = editor.selected_tile_region;
                            selected_region.x *= TILE_TEX_SIZE;
                            selected_region.y *= TILE_TEX_SIZE;

                            struct rectangle region = editor.selection_region;
                            draw_texture(tile_textures[t->id],
                                         t->x * TILE_TEX_SIZE + (region.x - selected_region.x),
                                         t->y * TILE_TEX_SIZE + (region.y - selected_region.y),
                                         TILE_TEX_SIZE, TILE_TEX_SIZE, COLOR4F_RED);
                        }
                    } 
                }
            }
        }
    } end_graphics_frame();

    begin_graphics_frame();{
        int mouse_position[2];
        get_mouse_location_in_camera_space(mouse_position, mouse_position+1);

        draw_text(test_font, 0, 0, "world edit", COLOR4F_WHITE);
        draw_text(test_font, 0, 32, format_temp("tilecount: %d\n(mx: %d, my: %d)\n", editor.tilemap.tile_count, mouse_position[0], mouse_position[1]), COLOR4F_WHITE);

        /*"tool" bar*/
        {
            int frame_pad = 3;
            int frame_size = TILE_TEX_SIZE+frame_pad;
            int i = 0;
            draw_rectangle(i * frame_size, 16, frame_size, frame_size, COLOR4F_RED);
            draw_texture(tile_textures[editor.placement_type], i * frame_size + frame_pad/2, 16 + frame_pad/2, TILE_TEX_SIZE, TILE_TEX_SIZE, COLOR4F_BLUE);
            draw_text(test_font, 5 + (i+1) * frame_size + frame_pad/2, 16, tile_type_strings[editor.placement_type], COLOR4F_WHITE);
        }
    } end_graphics_frame();
}
