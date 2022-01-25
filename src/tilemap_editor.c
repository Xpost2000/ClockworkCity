#include "memory_arena.h"
#define EDITOR_TILE_MAX_COUNT (16384)

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

    enum tile_id placement_type;
};

local struct editor_state editor;

struct tile* existing_block_at(struct tile* tiles, int tile_count, int grid_x, int grid_y) {
    for (unsigned index = 0; index < tile_count; ++index) {
        struct tile* t = tiles + index;
        if (t->x == grid_x && t->y == grid_y) {
            return t;
        }
    }

    return NULL;
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
    if (thickness == 1) {
        /*NOTE(jerry):
          one sized filled rectangles sometimes flicker out of existance with the current renderer.
         */
        for (int y = 0; y <= rows; ++y) {
            draw_rectangle(x_offset, y * TILE_TEX_SIZE + y_offset, cols * TILE_TEX_SIZE, 1, color);
        }
        for (int x = 0; x <= cols; ++x) {
            draw_rectangle(x * TILE_TEX_SIZE + x_offset, y_offset, 1, rows * TILE_TEX_SIZE, color);
        }
    } else {
        for (int y = 0; y <= rows; ++y) {
            draw_filled_rectangle(x_offset, y * TILE_TEX_SIZE + y_offset,
                                  cols * TILE_TEX_SIZE, thickness, color);
        }
        for (int x = 0; x <= cols; ++x) {
            draw_filled_rectangle(x * TILE_TEX_SIZE + x_offset, y_offset,
                                  thickness, rows * TILE_TEX_SIZE, color);
        }
    }
}

local void tilemap_editor_update_render_frame(float dt) {
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
            get_mouse_location(mouse_position, mouse_position+1);

            /*center mouse position*/
            transform_point_into_camera_space(mouse_position, mouse_position+1);

            bool left_click, right_click;
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
            draw_tiles(editor.tilemap.tiles, editor.tilemap.tile_count);
        }
    } end_graphics_frame();

    begin_graphics_frame();{
        draw_text(test_font, 0, 0, "world edit", COLOR4F_WHITE);
        draw_text(test_font, 0, 32, format_temp("tilecount: %d", editor.tilemap.tile_count), COLOR4F_WHITE);

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
