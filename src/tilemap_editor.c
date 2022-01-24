/*
  Obviously I need this to look better and work a bit better,
  this is just a quick hack for the night.
*/
/*will be a world editor*/
#define EDITOR_TILE_MAX_COUNT (16384)
struct editor_memory_arena {
    size_t used;
    size_t capacity;
    void* memory;
};

void* editor_memory_arena_push(struct editor_memory_arena* arena, size_t amount) {
    assert(arena->used < arena->capacity && "Out of arena memory (does not grow!)");
    void* result = arena->memory + arena->used;
    arena->used += amount;
    return result;
}

struct editor_memory_arena editor_alloc_arena(size_t sz) {
    return (struct editor_memory_arena) {
        .used = 0,
        .capacity = sz,
        /*should this be virtual memory?*/
        .memory = system_allocate_zeroed_memory(sz)
    };
}

void editor_memory_arena_clear(struct editor_memory_arena* arena) {
    arena->used = 0;
}

void editor_memory_arena_deallocate(struct editor_memory_arena* arena) {
    arena->used = 0;
    arena->capacity = 0;
    system_deallocate_memory(arena->memory);
    arena->memory = NULL;
}

struct editor_state {
    struct editor_memory_arena arena;

    uint32_t tile_count;
    /*
      Contiguous storage only for this.
      
      When writing to disk, I will copy the buffer and rearrange
      it so it can be addressed like a 2D array.
      
      Kind of expensive but whatever.
      
(TODO): Technically, I could do this tonight, but I'm too lazy to.
Replace this with the tilemap struct.
    */
    struct tile* tiles;

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
    assert(editor.tile_count < EDITOR_TILE_MAX_COUNT);

    struct tile* tile = existing_block_at(editor.tiles, editor.tile_count, grid_x, grid_y);

    if (!tile) {
        tile = &editor.tiles[editor.tile_count++];
    } else {
    }

    tile->x = grid_x;
    tile->y = grid_y;

    tile->id = editor.placement_type;
}

local void load_tilemap_editor_resources(void) {
    editor.arena = editor_alloc_arena(Megabyte(64));
    editor.tiles = editor_memory_arena_push(&editor.arena, EDITOR_TILE_MAX_COUNT * sizeof(*editor.tiles));
}

/*braindead code for the night
  not thinking much of how to do it or any abstractions loololol*/
local void editor_output_to_binary_file(char* filename) {
    FILE* f = fopen(filename, "wb+");

    char magic[] = "LOVEYOU!";

    /*
      right now I'm not even doing the transformation to make it a 2D array on disk.
      
      Also I'm hoping that doing that should be fine because the 2D array should be densely packed.
    */

    fwrite(magic, 8, 1, f);
    fwrite(&editor.tile_count, sizeof(editor.tile_count), 1, f);
    fwrite(editor.tiles, sizeof(*editor.tiles) * editor.tile_count, 1, f);

    fclose(f);
}

local void editor_load_from_binary_file(char* filename) {
    FILE* f = fopen(filename, "rb+");

    char magic[8] = {};

    fread(magic, 8, 1, f);
    assert(strncmp(magic, "LOVEYOU!", 8) == 0);

    fread(&editor.tile_count, sizeof(editor.tile_count), 1, f);
    fread(editor.tiles, sizeof(*editor.tiles) * editor.tile_count, 1, f);

    fprintf(stderr, "%d\n", editor.tile_count);

    fclose(f);
}

local void tilemap_editor_update_render_frame(float dt) {
    if (is_key_pressed(KEY_RIGHT)) {
        editor.placement_type++;
    } else if (is_key_pressed(KEY_LEFT)) {
        editor.placement_type--;
    }

    if (is_key_pressed(KEY_SPACE) && is_key_down(KEY_W)) {
        editor_output_to_binary_file("test.chunkpiece");
    }

    if (is_key_pressed(KEY_SPACE) && is_key_down(KEY_S)) {
        editor_load_from_binary_file("test.chunkpiece");
    }


    editor.placement_type = clampi(editor.placement_type, TILE_SOLID, TILE_ID_COUNT-1);

    begin_graphics_frame(); {
        draw_filled_rectangle(0, 0, 640, 480, COLOR4F_BLACK);
        /*grid*/
        {
            int row_count = (480/TILE_TEX_SIZE);
            int col_count = (640/TILE_TEX_SIZE);

            for (int y = 0; y < row_count; ++y) {
                draw_rectangle(0, y * TILE_TEX_SIZE, 640, 2, COLOR4F_DARKGRAY);
            }

            for (int x = 0; x < col_count; ++x) {
                draw_rectangle(x * TILE_TEX_SIZE, 0, 2, 480, COLOR4F_DARKGRAY);
            }
        }

        /*"tool" bar*/
        #if 0
        {
            int i = 0;
            int frame_pad = 3;
            int frame_size = TILE_TEX_SIZE+frame_pad;
            for (int x = max_int(editor.placement_type-2, TILE_SOLID);
                 x < min_int(editor.placement_type + 2, TILE_ID_COUNT-1);
                 ++x, ++i) {
                if (editor.placement_type == x) {
                    draw_rectangle(i * frame_size, 16, frame_size, frame_size, COLOR4F_RED);
                } else {
                    draw_rectangle(i * frame_size, 16, frame_size, frame_size, COLOR4F_GREEN);
                }
                draw_texture(tile_textures[x], i * frame_size + frame_pad/2, 16 + frame_pad/2, TILE_TEX_SIZE, TILE_TEX_SIZE, COLOR4F_BLUE);
            }
        }
        #else
        {
            int frame_pad = 3;
            int frame_size = TILE_TEX_SIZE+frame_pad;
            int i = 0;
            draw_rectangle(i * frame_size, 16, frame_size, frame_size, COLOR4F_RED);
            draw_texture(tile_textures[editor.placement_type], i * frame_size + frame_pad/2, 16 + frame_pad/2, TILE_TEX_SIZE, TILE_TEX_SIZE, COLOR4F_BLUE);
            draw_text(test_font, 5 + (i+1) * frame_size + frame_pad/2, 16, tile_type_strings[editor.placement_type], COLOR4F_WHITE);
        }
        #endif


        /* cursor */
        {
            int mouse_position[2];
            get_mouse_location(mouse_position, mouse_position+1);

            bool left_click;
            get_mouse_buttons(&left_click, 0, 0);

            {
                mouse_position[0] = (mouse_position[0] / TILE_TEX_SIZE) * TILE_TEX_SIZE;
                mouse_position[1] = (mouse_position[1] / TILE_TEX_SIZE) * TILE_TEX_SIZE;
            }

            draw_filled_rectangle(mouse_position[0], mouse_position[1],
                                  TILE_TEX_SIZE, TILE_TEX_SIZE,
                                  color4f(1, 1, 1, (sinf(global_elapsed_time) + 1) / 2.0f));

            if (left_click) {
                editor_try_to_place_block(mouse_position[0]/TILE_TEX_SIZE,
                                          mouse_position[1]/TILE_TEX_SIZE);
            }
        }

        /* world */
        {
            draw_tiles(editor.tiles, editor.tile_count);
        }
        draw_text(test_font, 0, 0, "world edit", COLOR4F_WHITE);
        draw_text(test_font, 0, 32, format_temp("tilecount: %d", editor.tile_count), COLOR4F_WHITE);
    } end_graphics_frame();
}
