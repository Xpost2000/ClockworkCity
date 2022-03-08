#include "memory_arena.h"
#define EDITOR_TILE_GRASS_MAX_COUNT        (16384)
#define EDITOR_TILE_MAX_COUNT              (32768)
#define EDITOR_ENTITY_PLACEMENTS_MAX       (8192)
#define EDITOR_TRANSITIONS_MAX_COUNT       (64)
#define EDITOR_PLAYER_SPAWN_MAX_COUNT      (EDITOR_TRANSITIONS_MAX_COUNT)
#define EDITOR_TRIGGERS_MAX_COUNT          (512)
#define EDITOR_CAMERA_FOCUS_ZONE_MAX_COUNT (256)
#define EDITOR_SOUL_ANCHOR_MAX_COUNT       (256)
#define EDITOR_ACTIVATION_SWITCH_MAX_COUNT (128)
#define EDITOR_SPRITE_PROP_MAX_COUNT       (512)
#define EDITOR_CRUSHER_PLATFORM_MAX_COUNT  (512)
#define EDITOR_DOOR_MAX_COUNT              (512)
#define EDITOR_OBSCURING_ZONE_MAX_COUNT    (512)

/* Do I rival VVVVVVV for code quality right here? Holy fuck I regret this part so much. I should've used IMGUI, or
 at least spent more than 5 minutes thinking of the editor... */

/*
 * Duplicated code macros
 * no templates in C, and cannot use polymorphic trick because types are different in memory layout!
 
 These macros are pretty evil and satanic, and rely on the fact I have a regular naming convention...
 Also breaks indenting the next line in emacs???

 Ahhhh... this must be a Hunter's Nightmare.
 */
#define Generate_Rectangle_Sizing_Code(object)          \
    do {                                                \
    if (is_key_pressed(KEY_DOWN)) {                     \
        object->h += 1;                                 \
    } else if (is_key_pressed(KEY_UP)) {                \
        object->h -= 1;                                 \
    }                                                   \
    if (is_key_pressed(KEY_RIGHT)) {                    \
        object->w += 1;                                 \
    } else if (is_key_pressed(KEY_LEFT)) {              \
        object->w -= 1;                                 \
    }                                           \ 
    if (object->w < 1) object->w = 1;         \ 
    if (object->h < 1) object->h = 1;         \
} while (0)

#define Generate_Rectangle_Drag_Or_Place_Code(object, type, default_w, default_h) \
      do {                                                              \
      if (left_click) {                                                 \
          if (object) {                                                 \
              if (!editor.dragging &&                                   \
                  rectangle_overlapping_v(object->x, object->y, object->w, object->h, mouse_position[0], mouse_position[1], 1, 1)) { \
                  editor.dragging = true;                               \
              } else if (editor.dragging) {                             \
                  object->x = mouse_position[0];                        \
                  object->y = mouse_position[1];                        \
              }                                                         \
          } else {                                                      \
              struct type * type = editor_existing_##type##_at(editor.tilemap.type##s, editor.tilemap.type##_count, mouse_position[0], mouse_position[1]); \
              if (!type) {                                              \
                  type = editor_allocate_##type();                      \
                  type->x = mouse_position[0];                          \
                  type->y = mouse_position[1];                          \
                  type->w = default_w;                                  \
                  type->h = default_h;                                  \
              }                                                         \
              editor.context = type;                                    \
          }                                                             \
      } else {                                                          \
          editor.dragging = false;                                      \
      }                                                                 \
      } while (0)

#define Generate_Rectangle_Deletion_Or_Unselect_Code(type)              \
      do {                                                              \
      if (right_click) {                                                \
          struct type* type = editor_existing_##type##_at(editor.tilemap.type##s, editor.tilemap.type##_count, mouse_position[0], mouse_position[1]); \
          if (type) {                                                   \
              unsigned index = (type - editor.tilemap.type##s);         \
              editor.tilemap.type##s[index] = editor.tilemap.type##s[--editor.tilemap.type##_count]; \
          } else {                                                      \
              editor.context = NULL;                                    \
          }                                                             \
      }                                                                 \
      if (is_key_pressed(KEY_ESCAPE)) {                                 \
          editor.context = NULL;                                        \
      }                                                                 \
      } while(0)
/*
  NOTE(jerry):
  Almost everything is expressed in tile coordinates at the moment.

  The game itself uses floating point math, but all entities must snap to the tile grid except
  for maybe like "decorative" things?
  
  NOTE(jerry):
  Lots of code is copied and pasted since it was way faster to do this honestly...
*/

/*
  TODO(jerry):
  editor support for 
  - Soul Anchors
  - Camera Focus Zones
  - Triggers;
 */

/*
  regions do not include entities???
  NOTE(jerry): this suffers a lot from the lack of a real UI library lol.
  I really need a UI library. Dear god.
  
  Yeah anyways, this code is REALLY REALLY bad, but whatever. The rest of the engine has better code.
*/

/*This data structure may be different from the game runtime, which is why it's duplicated*/
struct editable_tilemap {
    uint32_t tile_count;
    uint8_t  transition_zone_count;
    uint8_t  player_spawn_link_count;
    uint16_t entity_placement_count;
    uint16_t trigger_count;
    uint16_t soul_anchor_count;
    uint16_t camera_focus_zone_count;
    uint16_t activation_switch_count;
    uint16_t sprite_prop_count;
    uint16_t door_count;
    uint16_t crusher_platform_count;
    uint16_t obscuring_zone_count;

    uint32_t width;
    uint32_t height;

    uint32_t foreground_tile_count;
    uint32_t background_tile_count;
    uint32_t grass_tile_count;

    float bounds_min_x;
    float bounds_min_y;
    float bounds_max_x;
    float bounds_max_y;

    struct player_spawn_link* player_spawn_links;
    struct entity_placement*  entity_placements; /* store entity placements and some flags*/
    struct camera_focus_zone* camera_focus_zones;
    struct tile*              tiles;
    struct tile*              foreground_tiles;
    struct tile*              background_tiles;
    struct grass_tile*        grass_tiles;
    struct soul_anchor*       soul_anchors;
    struct trigger*           triggers;
    struct transition_zone*   transitions;
    struct player_spawn       default_spawn;
    struct crusher_platform*  crusher_platforms;
    struct obscuring_zone*    obscuring_zones;
    struct door*              doors;
    struct sprite_prop*       sprite_props;
    /* regular naming convention. I know it's switches */
    struct activation_switch* activation_switchs;
};
enum editor_tool_mode {
    EDITOR_TOOL_PAINT_TILE,
    EDITOR_TOOL_PAINT_TRANSITION,
    EDITOR_TOOL_PAINT_PLAYERSPAWN,
    EDITOR_TOOL_ESTABLISH_BOUNDS,
    EDITOR_TOOL_PAINT_ENTITIES, /* 
                                   There are submodes within this for specific typed entities? Like doors or something? If I get that far 
                                   Honestly I'm going to see if I can get away with omitting doors, by just making movement the only gate-keeper.
                                */
    EDITOR_TOOL_PAINT_TRIGGERS,
    EDITOR_TOOL_COUNT
};
enum entity_painting_subtype {
    ENTITY_PAINTING_SUBTYPE_ENTITIES,
    ENTITY_PAINTING_SUBTYPE_TRIGGERS,
    ENTITY_PAINTING_SUBTYPE_CAMERA_FOCUS_ZONES,
    ENTITY_PAINTING_SUBTYPE_SOUL_ANCHORS,
    ENTITY_PAINTING_SUBTYPE_DOORS,
    ENTITY_PAINTING_SUBTYPE_ACTIVATORS,
    ENTITY_PAINTING_SUBTYPE_COUNT,
};
enum editor_tile_layer {
    TILE_LAYER_PLAYABLE,
    TILE_LAYER_BACKGROUND,
    TILE_LAYER_FOREGROUND,
};
const local char* editor_tile_layer_strings[] = {
    "playable layer", "background layer", "foreground layer",
};
const local char* editor_tool_mode_strings[] = {
    "Paint Tile", "Edit Transitions", "Player Spawn Placement", "Establish Level Bounds", "Paint Entities", "Paint Triggers", "?"
};
const local char* editor_painting_subtype_strings[] = {
    "entities", "triggers", "camera focus zones", "soul anchors", "doors", "activators"
};
struct editor_text_edit {
    bool open;
    char* prompt_title;
    char* buffer_target;
    size_t buffer_length;
};
struct editor_state {
    struct memory_arena arena;
    bool initialized;

    float editor_tool_change_fade_timer;
    /*
      This is a slightly different layout to the ingame format.
      This is basically a source format
    */
    struct editable_tilemap tilemap;

    enum editor_tool_mode tool;
    enum editor_tile_layer editting_tile_layer;
    uint32_t painting_entity_type;
    uint32_t entity_painting_subtype;
    bool painting_grass;

    void* context; /*depends on mode*/
    struct editor_text_edit text_edit;

    /*unused?*/
    int last_mouse_x;
    int last_mouse_y;
    /*unused?*/
    bool dragging;

    /*-*/
    bool selection_region_exists;
    /*for some reason I decide to store this in pixels. Change this later.*/
    struct rectangle selection_region;
    /*this is memoized separately from selection_region. Only changes on resize.*/
    struct rectangle selected_tile_region;

    enum tile_id placement_type;
};

local struct editor_state editor;

local void editor_open_text_edit_prompt(char* prompt_name, char* target_buffer, size_t target_buffer_length, size_t current_length) {
    struct editor_text_edit* text_edit = &editor.text_edit;
    text_edit->prompt_title            = prompt_name;
    text_edit->buffer_target           = target_buffer;
    text_edit->buffer_length           = target_buffer_length;
    text_edit->open                    = true;
    start_text_edit(target_buffer, current_length);
}

local struct tile* editor_allocate_block(void) {
    if (editor.painting_grass) {
        assert(editor.tilemap.grass_tile_count < EDITOR_TILE_GRASS_MAX_COUNT && "overrun grass tiles?");
        return &editor.tilemap.grass_tiles[editor.tilemap.grass_tile_count++];
    } else {
        switch (editor.editting_tile_layer) {
            case TILE_LAYER_FOREGROUND: {
                assert(editor.tilemap.foreground_tile_count < EDITOR_TILE_MAX_COUNT && "overrun foreground tiles?");
                return &editor.tilemap.foreground_tiles[editor.tilemap.foreground_tile_count++];
            } break;
            case TILE_LAYER_BACKGROUND: {
                assert(editor.tilemap.tile_count < EDITOR_TILE_MAX_COUNT && "overrun background tiles?");
                return &editor.tilemap.background_tiles[editor.tilemap.background_tile_count++];
            } break;
            case TILE_LAYER_PLAYABLE: {
                assert(editor.tilemap.tile_count < EDITOR_TILE_MAX_COUNT && "overrun playable tiles?");
                return &editor.tilemap.tiles[editor.tilemap.tile_count++];
            } break;
        }
    }


    return NULL;
}

local void editor_select_appropriate_tile_array(struct tile** tiles, size_t* count) {
    if (editor.painting_grass) {
        *count = editor.tilemap.grass_tile_count;
        *tiles = editor.tilemap.grass_tiles;
    } else {
        switch (editor.editting_tile_layer) {
            case TILE_LAYER_FOREGROUND: {
                *count = editor.tilemap.foreground_tile_count;
                *tiles = editor.tilemap.foreground_tiles;
            } break;
            case TILE_LAYER_BACKGROUND: {
                *count = editor.tilemap.background_tile_count;
                *tiles = editor.tilemap.background_tiles;
            } break;
            case TILE_LAYER_PLAYABLE: {
                *count = editor.tilemap.tile_count;
                *tiles = editor.tilemap.tiles;
            } break;
        }
    }
} 

local struct transition_zone* editor_allocate_transition(void) {
    assert(editor.tilemap.transition_zone_count < EDITOR_TRANSITIONS_MAX_COUNT);
    struct transition_zone* result = &editor.tilemap.transitions[editor.tilemap.transition_zone_count++];
    zero_array(result->identifier);
    zero_array(result->zone_filename);
    zero_array(result->zone_link);
    return result;
}

local struct door* editor_allocate_door(void) {
    assert(editor.tilemap.door_count < EDITOR_DOOR_MAX_COUNT);
    struct door* result = &editor.tilemap.doors[editor.tilemap.door_count++];
    return result;
}

local struct camera_focus_zone* editor_allocate_camera_focus_zone(void) {
    assert(editor.tilemap.camera_focus_zone_count < EDITOR_CAMERA_FOCUS_ZONE_MAX_COUNT);
    struct camera_focus_zone* result = &editor.tilemap.camera_focus_zones[editor.tilemap.camera_focus_zone_count++];
    /* default init */
    result->active = 1;
    result->zoom = 1;
    result->interpolation_speed[0] = result->interpolation_speed[1] = result->interpolation_speed[2] = 1;
    return result;
}

local struct trigger* editor_allocate_trigger(void) {
    assert(editor.tilemap.trigger_count < EDITOR_TRIGGERS_MAX_COUNT);
    struct trigger* result = &editor.tilemap.triggers[editor.tilemap.trigger_count++];
    return result;
}

local struct entity_placement* editor_allocate_entity_placement(void) {
    assert(editor.tilemap.entity_placement_count < EDITOR_ENTITY_PLACEMENTS_MAX);
    struct entity_placement* result = &editor.tilemap.entity_placements[editor.tilemap.entity_placement_count++];
    zero_array(result->identifier);
    result->flags = 0;
    result->travel_distance_x = 0;
    result->travel_distance_y = 0;
    
    return result;
}

local struct player_spawn_link* editor_allocate_spawn(void) {
    assert(editor.tilemap.player_spawn_link_count < EDITOR_PLAYER_SPAWN_MAX_COUNT);
    struct player_spawn_link* result = &editor.tilemap.player_spawn_links[editor.tilemap.player_spawn_link_count++];
    zero_array(result->identifier);
    return result;
}

local struct soul_anchor* editor_allocate_soul_anchor(void) {
    assert(editor.tilemap.soul_anchor_count < EDITOR_SOUL_ANCHOR_MAX_COUNT);
    struct soul_anchor* result = &editor.tilemap.soul_anchors[editor.tilemap.soul_anchor_count++];
    zero_array(result->identifier);
    return result;
}

local struct activation_switch* editor_allocate_activation_switch(void) {
    assert(editor.tilemap.activation_switch_count < EDITOR_ACTIVATION_SWITCH_MAX_COUNT);
    struct activation_switch* result = &editor.tilemap.activation_switchs[editor.tilemap.activation_switch_count++];
    zero_array(result->targets);
    return result;
}

/*grid coordinates*/
local bool intersects_editor_selected_tile_region(int x, int y, int w, int h) {
    struct rectangle region = editor.selected_tile_region;
    return rectangle_intersects_v(region.x, region.y, region.w, region.h, x, y, w, h);
}

local void editor_begin_selection_region(int x, int y) {
    float scale_factor = editor_camera.render_scale;

    if (editor.selection_region_exists)
        return;

    editor.selection_region_exists = true;
    editor.selection_region.x = x;
    editor.selection_region.y = y;
    console_printf("started region selection at (x:%d(tx: %d), y:%d(ty: %d))\n", x, (int)editor.selection_region.x, y, (int)editor.selection_region.y);
}

local void editor_end_selection_region(void) {
    editor.selection_region_exists = false;
    editor.selection_region = (struct rectangle) {};
    editor.selected_tile_region = (struct rectangle) {};
}

local void get_mouse_location_in_camera_space(int* x, int* y) {
    get_mouse_location(x, y);
    transform_point_into_camera_space(&editor_camera, x, y);
}

/*abstract this later.*/
struct tile* existing_block_at(struct tile* tiles, int tile_count, int grid_x, int grid_y) {
    for (unsigned index = 0; index < tile_count; ++index) {
        struct tile* t = tiles + index;

        if (!editor.painting_grass) {
            if (t->id == TILE_NONE) continue;  
        } 

        if (t->x == grid_x && t->y == grid_y) {
            return t;
        }
    }

    return NULL;
}

local bool editor_any_blocks_within_region(void) {
    struct rectangle region = editor.selected_tile_region;

    size_t count;
    struct tile* tiles;
    editor_select_appropriate_tile_array(&tiles, &count);

    for (int y = 0; y < (int)region.h; ++y) {
        for (int x = 0; x < (int)region.w; ++x) {
            if (existing_block_at(tiles, count, x + region.x, y + region.y)) {
                return true;
            }
        }
    }

    return false;
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

struct camera_focus_zone* editor_existing_camera_focus_zone_at(struct camera_focus_zone* camera_focus_zones, size_t camera_focus_zone_count, int grid_x, int grid_y) {
    for (unsigned index = 0; index < camera_focus_zone_count; ++index) {
        struct camera_focus_zone* t = camera_focus_zones + index;

        if (rectangle_intersects_v(t->x, t->y, t->w, t->h, grid_x, grid_y, 0.5, 0.5)) {
            return t;
        }
    }

    return NULL;
}

struct soul_anchor* editor_existing_soul_anchor_at(struct soul_anchor* anchors, size_t anchor_count, int grid_x, int grid_y) {
    for (unsigned index = 0; index < anchor_count; ++index) {
        struct soul_anchor* t = anchors + index;

        if (rectangle_intersects_v(t->x, t->y, 1, 2, grid_x, grid_y, 0.5, 0.5)) {
            return t;
        }
    }

    return NULL;
}

struct door* editor_existing_door_at(struct door* doors, size_t door_count, int grid_x, int grid_y) {
    for (unsigned index = 0; index < door_count; ++index) {
        struct door* door = doors + index;

        if (rectangle_intersects_v(door->x, door->y, door->w, door->h, grid_x, grid_y, 0.5, 0.5)) {
            return door;
        }
    }

    return NULL;
}

struct activation_switch* editor_existing_activation_switch_at(struct activation_switch* switches, size_t switch_count, int grid_x, int grid_y) {
    for (unsigned index = 0; index < switch_count; ++index) {
        struct activation_switch* _switch = switches + index;

        if (rectangle_intersects_v(_switch->x, _switch->y, _switch->w, _switch->h, grid_x, grid_y, 0.5, 0.5)) {
            return _switch;
        }
    }

    return NULL;
}

struct transition_zone* editor_existing_transition_at(struct transition_zone* transitions, size_t transition_zone_count, int grid_x, int grid_y) {
    for (unsigned index = 0; index < transition_zone_count; ++index) {
        struct transition_zone* t = transitions + index;

        if (rectangle_intersects_v(t->x, t->y, t->w, t->h, grid_x, grid_y, 0.5, 0.5)) {
            return t;
        }
    }

    return NULL;
}

struct trigger* editor_existing_trigger_at(struct trigger* triggers, size_t trigger_count, int grid_x, int grid_y) {
    for (unsigned index = 0; index < trigger_count; ++index) {
        struct trigger* t = triggers + index;

        if (rectangle_intersects_v(t->x, t->y, t->w, t->h, grid_x, grid_y, 0.5, 0.5)) {
            return t;
        }
    }

    return NULL;
}

struct player_spawn_link* editor_existing_spawn_at(struct player_spawn_link* spawns, size_t spawn_count, int grid_x, int grid_y) {
    if (rectangle_intersects_v(editor.tilemap.default_spawn.x, editor.tilemap.default_spawn.y, 1, 2, grid_x, grid_y, 0.5, 0.5)) {
        return &editor.tilemap.default_spawn;
    }

    for (unsigned index = 0; index < spawn_count; ++index) {
        struct player_spawn_link* t = spawns + index;

        if (rectangle_intersects_v(t->x, t->y, 1, 2, grid_x, grid_y, 0.5, 0.5)) {
            return t;
        }
    }

    return NULL;
}

/* NOTE(jerry):
   Entities are technically in floating point positions.
   
   Just round to nearest integer and hope it looks okay.
*/
struct entity_placement* editor_existing_entity_at(struct entity_placement* entities, size_t entity_count, int integer_x, int integer_y) {
    for (unsigned index = 0; index < entity_count; ++index) {
        struct entity_placement* t = entities + index;

        float entity_w;
        float entity_h;

        entity_get_type_dimensions(t->type, &entity_w, &entity_h);
        if (rectangle_intersects_v(t->x, t->y, entity_w, entity_h, integer_x, integer_y, 0.5, 0.5)) {
            return t;
        }
    }

    return NULL;
}
/* exists for the macro */
struct entity_placement* editor_existing_entity_placement_at(struct entity_placement* entities, size_t entity_count, int integer_x, int integer_y) {
    return editor_existing_entity_at(entities, entity_count, integer_x, integer_y);
}

/* TODO(jerry): negatives are bad! */
local bool editor_has_tiles_within_selection(void) {
    struct rectangle tile_region = editor.selected_tile_region;

    struct tile* tiles;
    size_t tile_count;
    editor_select_appropriate_tile_array(&tiles, &tile_count);

    for (int y = (int)tile_region.y; y < (int)(tile_region.y+tile_region.h); ++y) {
        for (int x = (int)tile_region.x; x < (int)(tile_region.x+tile_region.w); ++x) {
            struct tile* t = existing_block_at(tiles, tile_count, x, y);
            if (t) {
                return true;
            }
        }
    }

    return false;
}

local struct tile* editor_yank_selected_tile_region(struct memory_arena* arena, bool cut) {
    struct rectangle tile_region     = editor.selected_tile_region;
    struct rectangle selected_region = editor.selection_region;

    float scale_factor = editor_camera.render_scale;

    struct tile* tiles;
    size_t tile_count;
    editor_select_appropriate_tile_array(&tiles, &tile_count);
    
    struct tile* selected_region_tiles = memory_arena_push(arena, sizeof(*selected_region_tiles) * selected_region.w * selected_region.h);
    /*make temporary copy of the region, also empty it out at the same time*/ {
        zero_buffer_memory(selected_region_tiles, sizeof(*selected_region_tiles) * selected_region.w * selected_region.h);

        for (int y = (int)tile_region.y; y < (int)(tile_region.y+tile_region.h); ++y) {
            for (int x = (int)tile_region.x; x < (int)(tile_region.x+tile_region.w); ++x) {
                struct tile* t = existing_block_at(tiles, tile_count, x, y);
                if (t) {
                    selected_region_tiles[(y - (int)tile_region.y) * (int)tile_region.w + (x - (int)tile_region.x)] = *t;
                    if (cut) t->id = TILE_NONE;
                }
            }
        }
    }

    return selected_region_tiles;
}

local void editor_move_selected_tile_region(struct memory_arena* arena) {
    struct rectangle tile_region     = editor.selected_tile_region;
    struct rectangle selected_region = editor.selection_region;

    float scale_factor = editor_camera.render_scale;

    assert(tile_region.w == selected_region.w);
    assert(tile_region.h == selected_region.h);

    struct tile* selected_region_tiles = editor_yank_selected_tile_region(arena, true);

    struct tile* tiles;
    size_t tile_count;
    editor_select_appropriate_tile_array(&tiles, &tile_count);

    /*copy the selected_region_tiles into the right place*/ {
        for (int y = 0; y < (int)(tile_region.h); ++y) {
            for (int x = 0; x < (int)(tile_region.w); ++x) {
                struct tile* block_to_copy = &selected_region_tiles[(y * (int)tile_region.w) + x];
                struct tile* t = occupied_block_at(tiles, tile_count, x + selected_region.x, y + selected_region.y);

                if (block_to_copy->id == TILE_NONE) continue;

                if (!t) {
                    t = editor_allocate_block();
                }

                *t = *block_to_copy;
                t->x = x + selected_region.x;
                t->y = y + selected_region.y;
            }
        }
    }
}

/* copy and paste */
local void editor_copy_selected_tile_region(struct memory_arena* arena) {
    struct rectangle tile_region     = editor.selected_tile_region;
    struct rectangle selected_region = editor.selection_region;

    float scale_factor = editor_camera.render_scale;

    assert(tile_region.w == selected_region.w);
    assert(tile_region.h == selected_region.h);

    struct tile* selected_region_tiles = editor_yank_selected_tile_region(arena, false);

    struct tile* tiles;
    size_t tile_count;
    editor_select_appropriate_tile_array(&tiles, &tile_count);

    /*copy the selected_region_tiles into the right place*/ {
        for (int y = 0; y < (int)(tile_region.h); ++y) {
            for (int x = 0; x < (int)(tile_region.w); ++x) {
                struct tile* block_to_copy = &selected_region_tiles[(y * (int)tile_region.w) + x];
                struct tile* t = occupied_block_at(tiles, tile_count, x + selected_region.x, y + selected_region.y);

                if (block_to_copy->id == TILE_NONE) continue;

                if (!t) {
                    t = editor_allocate_block();
                }

                *t = selected_region_tiles[(y * (int)tile_region.w) + x];
                t->x = x + selected_region.x;
                t->y = y + selected_region.y;
            }
        }
    }
}

local void editor_try_to_place_block(int grid_x, int grid_y) {
    if (editor.painting_grass) {
        struct tile* tile;
        
        tile = existing_block_at(editor.tilemap.grass_tiles, editor.tilemap.grass_tile_count, grid_x, grid_y);
        if (!tile) tile = editor_allocate_block();
        
        tile->x = grid_x;
        tile->y = grid_y;
    } else {
        struct tile* tile;

        switch (editor.editting_tile_layer) {
            case TILE_LAYER_FOREGROUND: {
                tile = existing_block_at(editor.tilemap.foreground_tiles, editor.tilemap.foreground_tile_count, grid_x, grid_y);
            } break;
            case TILE_LAYER_BACKGROUND: {
                tile = existing_block_at(editor.tilemap.background_tiles, editor.tilemap.background_tile_count, grid_x, grid_y);
            } break;
            case TILE_LAYER_PLAYABLE: {
                tile = existing_block_at(editor.tilemap.tiles, editor.tilemap.tile_count, grid_x, grid_y);
            } break;
        }

        if (!tile) tile = editor_allocate_block();

        tile->x = grid_x;
        tile->y = grid_y;

        tile->id = editor.placement_type;
    }

}

local void editor_erase_block(int grid_x, int grid_y) {
    if (editor.painting_grass) {
        for (unsigned to_remove = 0; to_remove < editor.tilemap.grass_tile_count; ++to_remove) {
            struct grass_tile* t = editor.tilemap.grass_tiles + to_remove;

            if (t->x == grid_x && t->y == grid_y) {
                editor.tilemap.grass_tiles[to_remove] = editor.tilemap.grass_tiles[--editor.tilemap.grass_tile_count];
                return;
            }
        }
    }

    struct tile* tile = NULL;

    int removal_index;
    switch (editor.editting_tile_layer) {
        case TILE_LAYER_FOREGROUND: {
            tile = existing_block_at(editor.tilemap.foreground_tiles, editor.tilemap.foreground_tile_count, grid_x, grid_y);
            removal_index = tile - editor.tilemap.foreground_tiles;
        } break;
        case TILE_LAYER_BACKGROUND: {
            tile = existing_block_at(editor.tilemap.background_tiles, editor.tilemap.background_tile_count, grid_x, grid_y);
            removal_index = tile - editor.tilemap.background_tiles;
        } break;
        case TILE_LAYER_PLAYABLE: {
            tile = existing_block_at(editor.tilemap.tiles, editor.tilemap.tile_count, grid_x, grid_y);
            removal_index = tile - editor.tilemap.tiles;
        } break;
    }

    if (!tile) {
        return;
    }

    tile->id = TILE_NONE;
    /* This is breaks things for now. (I should just do the manual removal loop that I did before.) */
    /* editor.tilemap.tile_count -= 1; */
}

local void editor_clear_all(void) {
    editor.tilemap.tile_count              = 0;
    editor.tilemap.foreground_tile_count   = 0;
    editor.tilemap.background_tile_count   = 0;
    editor.tilemap.transition_zone_count   = 0;
    editor.tilemap.player_spawn_link_count = 0;
    editor.tilemap.entity_placement_count  = 0;
    editor.tilemap.trigger_count           = 0;
    editor.tilemap.soul_anchor_count       = 0;
    editor.tilemap.camera_focus_zone_count = 0;
    editor.tilemap.activation_switch_count = 0;
    editor.tilemap.crusher_platform_count  = 0;
    editor.tilemap.obscuring_zone_count    = 0;
    editor.tilemap.door_count              = 0;
    editor.tilemap.sprite_prop_count       = 0;
    camera_reset_transform(&editor_camera);
    editor.tilemap.bounds_min_x            = 0;
    editor.tilemap.bounds_min_y            = 0;
    editor.tilemap.bounds_max_x            = 0;
    editor.tilemap.bounds_max_y            = 0;
}

local void load_tilemap_editor_resources(void) {
    if (!editor.initialized) {
        editor.arena = allocate_memory_arena(Megabyte(16));
        editor.arena.name = "Editor Arena";

        memory_arena_allocate_array(editor.arena, editor.tilemap.tiles,              EDITOR_TILE_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.foreground_tiles,   EDITOR_TILE_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.background_tiles,   EDITOR_TILE_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.camera_focus_zones, EDITOR_CAMERA_FOCUS_ZONE_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.transitions,        EDITOR_TRANSITIONS_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.player_spawn_links, EDITOR_PLAYER_SPAWN_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.grass_tiles,        EDITOR_TILE_GRASS_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.entity_placements,  EDITOR_ENTITY_PLACEMENTS_MAX);
        memory_arena_allocate_array(editor.arena, editor.tilemap.triggers,           EDITOR_TRIGGERS_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.soul_anchors,       EDITOR_SOUL_ANCHOR_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.obscuring_zones,    EDITOR_OBSCURING_ZONE_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.doors,              EDITOR_DOOR_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.crusher_platforms,  EDITOR_CRUSHER_PLATFORM_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.activation_switchs, EDITOR_ACTIVATION_SWITCH_MAX_COUNT);
        memory_arena_allocate_array(editor.arena, editor.tilemap.sprite_props,       EDITOR_SPRITE_PROP_MAX_COUNT);

        editor.initialized = true;
    }
}

local void editor_serialize(struct binary_serializer* serializer) {
    char magic[8] = "MVOIDLVL";
    get_bounding_rectangle_for_tiles(editor.tilemap.tiles, editor.tilemap.tile_count,
                                     NULL/*x*/, NULL/*y*/, &editor.tilemap.width, &editor.tilemap.height);
    serialize_bytes(serializer, magic, sizeof(magic));
    uint32_t version_id = TILEMAP_CURRENT_VERSION;
    serialize_u32(serializer, &version_id);
    serialize_u32(serializer, &editor.tilemap.width);
    serialize_u32(serializer, &editor.tilemap.height);
    Serialize_Structure(serializer, editor.tilemap.default_spawn);
    serialize_f32(serializer, &editor.tilemap.bounds_min_x);
    serialize_f32(serializer, &editor.tilemap.bounds_min_y);
    serialize_f32(serializer, &editor.tilemap.bounds_max_x);
    serialize_f32(serializer, &editor.tilemap.bounds_max_y);

    Serialize_Fixed_Array(serializer, u32, editor.tilemap.tile_count, editor.tilemap.tiles);
    if (version_id >= 2) {
        Serialize_Fixed_Array(serializer, u32, editor.tilemap.foreground_tile_count, editor.tilemap.foreground_tiles);
        Serialize_Fixed_Array(serializer, u32, editor.tilemap.background_tile_count, editor.tilemap.background_tiles);

        /* Probably doesn't have to be nested. */
        if (version_id >= 3) {
            Serialize_Fixed_Array(serializer, u32, editor.tilemap.grass_tile_count, editor.tilemap.grass_tiles);

            if (version_id >= 4) {
                Serialize_Fixed_Array(serializer, u16, editor.tilemap.entity_placement_count, editor.tilemap.entity_placements);

                if (version_id >= 5) {
                    Serialize_Fixed_Array(serializer, u16, editor.tilemap.trigger_count, editor.tilemap.triggers);
                    Serialize_Fixed_Array(serializer, u16, editor.tilemap.camera_focus_zone_count, editor.tilemap.camera_focus_zones);
                    Serialize_Fixed_Array(serializer, u16, editor.tilemap.soul_anchor_count, editor.tilemap.soul_anchors);

                    if (version_id >= 6) {
                        Serialize_Fixed_Array(serializer, u16, editor.tilemap.activation_switch_count, editor.tilemap.activation_switchs);
                        Serialize_Fixed_Array(serializer, u16, editor.tilemap.door_count, editor.tilemap.doors);
                    }
                }
            }
        }
    }
    Serialize_Fixed_Array(serializer,  u8, editor.tilemap.transition_zone_count, editor.tilemap.transitions);
    Serialize_Fixed_Array(serializer,  u8, editor.tilemap.player_spawn_link_count, editor.tilemap.player_spawn_links);
}

/*
  This seriously suffers when having to do versioned files.
*/
local void editor_output_to_binary_file(char* filename) {
    struct binary_serializer file = open_write_file_serializer(filename);
    editor_serialize(&file);
    serializer_finish(&file);
}

local void editor_load_from_binary_file(char* filename) {
    struct binary_serializer file = open_read_file_serializer(filename);
    editor_serialize(&file);
    serializer_finish(&file);
    camera_set_position(&editor_camera, editor.tilemap.default_spawn.x, editor.tilemap.default_spawn.y);
}

static void editor_serialize_into_game_memory(void) {
    /* The api is unfortunately not really designed for two way streaming so I have to
       separate it into two steps. Not a big deal. */
    struct binary_serializer write_memory = open_write_memory_serializer();
    editor_serialize(&write_memory);
    size_t size;
    char* finalized_memory_buffer = serializer_flatten_memory(&write_memory, &size);
    serializer_finish(&write_memory);

    struct binary_serializer read_memory = open_read_memory_serializer(finalized_memory_buffer, size);
    game_load_level_from_serializer(&game_memory_arena, &read_memory, NULL);
    system_deallocate_memory(finalized_memory_buffer);
    serializer_finish(&read_memory);
}

local void draw_grid(float x_offset, float y_offset, int rows, int cols, float thickness, union color4f color) {
    float scale_factor = editor_camera.render_scale;
    int direction_x = 1;
    int direction_y = 1;

    if (rows < 0) { direction_y *= -1; rows *= -1; }
    if (cols < 0) { direction_x *= -1; cols *= -1; }

    if (thickness == 1) {
        /*NOTE(jerry):
          one sized filled rectangles sometimes flicker out of existance with the current renderer.
         */
        for (int y = 0; y <= rows; ++y) {
            draw_rectangle(x_offset, y * (direction_y) + y_offset, direction_x * cols, 1/scale_factor, color);
        }
        for (int x = 0; x <= cols; ++x) {
            draw_rectangle(x * (direction_x) + x_offset, y_offset, 1/scale_factor, direction_y * rows, color);
        }
    } else {
        for (int y = 0; y <= rows; ++y) {
            draw_filled_rectangle(x_offset, y * (direction_y) + y_offset, direction_x * cols, thickness/scale_factor, color);
        }
        for (int x = 0; x <= cols; ++x) {
            draw_filled_rectangle(x * (direction_x) + x_offset, y_offset, thickness / scale_factor, direction_y * rows, color);
        }
    }
}

local void tilemap_editor_handle_paint_tile_mode(struct memory_arena* frame_arena, float dt);
local void tilemap_editor_handle_paint_transition_mode(struct memory_arena* frame_arena, float dt);
local void tilemap_editor_handle_paint_playerspawn_mode(struct memory_arena* frame_arena, float dt);
local void tilemap_editor_handle_bounds_establishment(struct memory_arena* frame_arena, float dt);
local void tilemap_editor_handle_paint_entities_mode(struct memory_arena* frame_arena, float dt);

/* kill any state. */
local void editor_switch_tool(enum editor_tool_mode mode) {
    editor.tool = mode;
    editor.last_mouse_y = editor.last_mouse_x = 0;
    editor.context = NULL;
    editor.dragging = false;
    editor.selection_region_exists = false;
    editor.editor_tool_change_fade_timer = 1.5f;
}

void editor_draw_entity_placements(struct entity_placement* entity_placements, size_t entity_placement_count) {
    for (unsigned index = 0; index < entity_placement_count; ++index) {
        struct entity_placement* ep = entity_placements + index;
        struct entity temporary = construct_entity_of_type(ep->type, ep->x, ep->y);
        draw_entity(&temporary, 0, 0);
    }
}

local void tilemap_editor_update_render_frame(float dt) {
    /* lazy init to avoid hogging resources */
    load_tilemap_editor_resources();

    float scale_factor = editor_camera.render_scale;
    struct temporary_arena frame_arena = begin_temporary_memory(&editor.arena, Kilobyte(800));

    if (!is_editting_text()) {
        if (is_key_pressed(KEY_F1)) {
            editor_switch_tool(EDITOR_TOOL_PAINT_TILE);
        } else if (is_key_pressed(KEY_F2)) {
            editor_switch_tool(EDITOR_TOOL_PAINT_TRANSITION);
        } else if (is_key_pressed(KEY_F3)) {
            editor_switch_tool(EDITOR_TOOL_PAINT_PLAYERSPAWN);
        } else if (is_key_pressed(KEY_F4)) {
            editor_switch_tool(EDITOR_TOOL_ESTABLISH_BOUNDS);
        } else if (is_key_pressed(KEY_F5)) {
            editor_switch_tool(EDITOR_TOOL_PAINT_ENTITIES);
        } else if (is_key_pressed(KEY_F6)) {
            editor_switch_tool(EDITOR_TOOL_PAINT_TRIGGERS);
        }

        /*camera editor movement*/
        {
            const int CAMERA_SPEED = TILES_PER_SCREEN / 2;

            if (is_key_down(KEY_W)) {
                camera_offset_position(&editor_camera, 0, -dt * CAMERA_SPEED);
            } else if (is_key_down(KEY_S)) {
                camera_offset_position(&editor_camera, 0, dt * CAMERA_SPEED);
            }

            if (is_key_down(KEY_A)) {
                camera_offset_position(&editor_camera, -dt * CAMERA_SPEED, 0);
            } else if (is_key_down(KEY_D)) {
                camera_offset_position(&editor_camera, dt * CAMERA_SPEED, 0);
            }
        }
    }

    begin_graphics_frame(NULL); {
        draw_filled_rectangle(0, 0, 9999, 9999, active_colorscheme.secondary);
    } end_graphics_frame();

    begin_graphics_frame(&editor_camera); {
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
            const int SCREENFUL_FILLS = 50;

            int row_count = (screen_dimensions[1]) / scale_factor;
            int col_count = (screen_dimensions[0]) / scale_factor;

            int x_offset = -SCREENFUL_FILLS/2 * (col_count);
            int y_offset = -SCREENFUL_FILLS/2 * (row_count);

            draw_grid(x_offset, y_offset, row_count * SCREENFUL_FILLS,
                      col_count * SCREENFUL_FILLS, 1, COLOR4F_DARKGRAY);
        }

        /* draw world */
        {

            {
                struct tile* tiles = editor.tilemap.tiles;
                struct tile* foreground_tiles = editor.tilemap.foreground_tiles;
                struct tile* background_tiles = editor.tilemap.background_tiles;

                for (unsigned index = 0; index < editor.tilemap.background_tile_count; ++index) {
                    struct tile* t = &background_tiles[index];

                    if (editor.editting_tile_layer == TILE_LAYER_BACKGROUND && editor.selection_region_exists && intersects_editor_selected_tile_region(t->x, t->y, 1, 1) && !is_key_down(KEY_Y))
                        continue;

                    union color4f color = active_colorscheme.primary_background;
                    draw_texture(tile_textures[t->id], t->x, t->y, 1, 1, color);
                } 

                for (unsigned index = 0; index < editor.tilemap.tile_count; ++index) {
                    struct tile* t = &tiles[index];

                    if (editor.editting_tile_layer == TILE_LAYER_PLAYABLE && editor.selection_region_exists && intersects_editor_selected_tile_region(t->x, t->y, 1, 1) && !is_key_down(KEY_Y))
                        continue;

                    union color4f color = active_colorscheme.primary;
                    draw_texture(tile_textures[t->id], t->x, t->y, 1, 1, color);
                } 

                draw_grass_tiles(editor.tilemap.grass_tiles, editor.tilemap.grass_tile_count, active_colorscheme.primary);

                for (unsigned index = 0; index < editor.tilemap.foreground_tile_count; ++index) {
                    struct tile* t = &foreground_tiles[index];

                    if (editor.editting_tile_layer == TILE_LAYER_FOREGROUND && editor.selection_region_exists && intersects_editor_selected_tile_region(t->x, t->y, 1, 1) && !is_key_down(KEY_Y))
                        continue;

                    union color4f color = active_colorscheme.primary_foreground;
                    if (editor.editting_tile_layer != TILE_LAYER_FOREGROUND) {
                        color.a /= 2;
                    }
                    draw_texture(tile_textures[t->id], t->x, t->y, 1, 1, color);
                } 
            }

            draw_camera_focus_zones(editor.tilemap.camera_focus_zones, editor.tilemap.camera_focus_zone_count);
            draw_transitions(editor.tilemap.transitions, editor.tilemap.transition_zone_count);
            draw_player_spawn_links(editor.tilemap.player_spawn_links, editor.tilemap.player_spawn_link_count);
            draw_soul_anchors(editor.tilemap.soul_anchors, editor.tilemap.soul_anchor_count);
            draw_triggers(editor.tilemap.triggers, editor.tilemap.trigger_count);
            draw_doors(editor.tilemap.doors, editor.tilemap.door_count);
            draw_activation_switches(editor.tilemap.activation_switchs, editor.tilemap.activation_switch_count);
            editor_draw_entity_placements(editor.tilemap.entity_placements, editor.tilemap.entity_placement_count);
            draw_player_spawn(&editor.tilemap.default_spawn);

            {
                float bounds_width  = editor.tilemap.bounds_max_x - editor.tilemap.bounds_min_x;
                float bounds_height = editor.tilemap.bounds_max_y - editor.tilemap.bounds_min_y;

                draw_rectangle(editor.tilemap.bounds_min_x, editor.tilemap.bounds_min_y,
                               bounds_width, bounds_height, COLOR4F_BLUE);
            }
        }

        /*rectangle picker */
        /* little hack since I reuse functionality there. Eh. */
        if (editor.tool != EDITOR_TOOL_ESTABLISH_BOUNDS) {
            union color4f grid_color = COLOR4F_RED;

            if (is_key_down(KEY_Y)) grid_color = COLOR4F_GREEN;

            if (editor.selection_region_exists) {
                struct rectangle region = editor.selection_region;
                draw_grid(region.x, region.y,
                          roundf(region.h), roundf(region.w),
                          ((sinf(global_elapsed_time*8) + 1)/2.0f) * 2.0f + 0.5f, grid_color);

                /* draw tiles within the region for moving regions */
                {
                    struct tile* tiles = editor.tilemap.tiles;
                    for (unsigned index = 0; index < editor.tilemap.tile_count; ++index) {
                        struct tile* t = &tiles[index];

                        /*exclude tiles in the region to avoid a copy. Would like to have a hashmap :/*/
                        if (!intersects_editor_selected_tile_region(t->x, t->y, 1, 1))
                            continue;

                        {
                            struct rectangle selected_region = editor.selected_tile_region;

                            struct rectangle region = editor.selection_region;
                            draw_texture(tile_textures[t->id],
                                         t->x + (region.x - selected_region.x),
                                         t->y + (region.y - selected_region.y),
                                         1, 1, grid_color);
                        }
                    } 
                }
            }
        }
    } end_graphics_frame();

    /* these also have drawing code. That's why they're also here. */
    switch (editor.tool) {
        case EDITOR_TOOL_PAINT_TILE: {
            tilemap_editor_handle_paint_tile_mode(&frame_arena, dt);
        } break;
        case EDITOR_TOOL_PAINT_TRANSITION: {
            tilemap_editor_handle_paint_transition_mode(&frame_arena, dt);
        } break;
        case EDITOR_TOOL_PAINT_PLAYERSPAWN: {
            tilemap_editor_handle_paint_playerspawn_mode(&frame_arena, dt);
        } break;
        case EDITOR_TOOL_ESTABLISH_BOUNDS: {
            tilemap_editor_handle_bounds_establishment(&frame_arena, dt);
        } break;
        case EDITOR_TOOL_PAINT_ENTITIES: {
            tilemap_editor_handle_paint_entities_mode(&frame_arena, dt);
        } break;
    }

    if (editor.editor_tool_change_fade_timer > 0.0) {
        begin_graphics_frame(NULL); {
            editor.editor_tool_change_fade_timer -= dt;

            // manual center justify
            {
                char* string = editor_tool_mode_strings[editor.tool];
                int dimens[2];
                int tdimens[2];

                get_screen_dimensions(dimens, dimens+1);
                get_text_dimensions(test_font, string, tdimens, tdimens+1);


                draw_text(test_font, dimens[0]/2 - tdimens[0]/2, 0, string, color4f(1, 1, 1, editor.editor_tool_change_fade_timer/1.5f));
                draw_text(test_font, dimens[0]/2 - tdimens[0]/2+2, 2, string, color4f(1, 0, 0, editor.editor_tool_change_fade_timer/1.5f));
            }
        } end_graphics_frame();
    }

    /* text widget */
    begin_graphics_frame(NULL); {
        int dimens[2];
        get_screen_dimensions(dimens, dimens+1);
        if (editor.text_edit.open) {
            draw_filled_rectangle(0, 0, dimens[0], dimens[1], color4f(0,0,0,0.5));
            {
                int tdimens[2];
                get_text_dimensions(test3_font, editor.text_edit.prompt_title, tdimens, tdimens+1);
                draw_text(test3_font, dimens[0]/2 - tdimens[0]/2, 0, editor.text_edit.prompt_title, COLOR4F_WHITE);
            }
            {
                float font_height = font_size_aspect_ratio_independent(0.04);
                int tdimens[2];
                get_text_dimensions(test3_font, current_text_buffer(), tdimens, tdimens+1);
                draw_text(test3_font, dimens[0]/2 - tdimens[0]/2, font_height, current_text_buffer(), COLOR4F_WHITE);
            }

            if (is_key_pressed(KEY_ESCAPE)) {
                if (editor.text_edit.open) {
                    end_text_edit(0, 0);
                    editor.text_edit.open = false;
                }
            } else if (is_key_pressed(KEY_RETURN)) {
                if (editor.text_edit.open) {
                    end_text_edit(editor.text_edit.buffer_target, editor.text_edit.buffer_length);
                    editor.text_edit.open = false;
                    /* hardcoded response to return */
                    void editor_on_prompt_submission(void);
                    editor_on_prompt_submission();
                }
            }
        }
    } end_graphics_frame();

    end_temporary_memory(&frame_arena);
}

local void tilemap_editor_handle_paint_tile_mode(struct memory_arena* frame_arena, float dt) {
    float scale_factor = editor_camera.render_scale;

    if (is_key_pressed(KEY_G)) {
        Toggle_Boolean(editor.painting_grass);
    }

    if (is_key_pressed(KEY_UP)) {
        editor.editting_tile_layer++;
    } else if (is_key_pressed(KEY_DOWN)) {
        editor.editting_tile_layer--;
    }

    if (!editor.painting_grass) {
        if (editor.editting_tile_layer <= TILE_LAYER_PLAYABLE)        editor.editting_tile_layer = TILE_LAYER_PLAYABLE;
        else if (editor.editting_tile_layer >= TILE_LAYER_FOREGROUND) editor.editting_tile_layer = TILE_LAYER_FOREGROUND;

        if (is_key_pressed(KEY_RIGHT)) {
            editor.placement_type++;
        } else if (is_key_pressed(KEY_LEFT)) {
            editor.placement_type--;
        }
    }

    if (editor.painting_grass) {
        /* 
           NOTE(jerry):
           Disallow region selection of grass. It just doesn't transport the same way.
         */
        editor_end_selection_region();
    } else {
        if (is_key_pressed(KEY_ESCAPE)) {
            editor_end_selection_region();
        }

        if (is_key_pressed(KEY_RETURN)) {
            if (editor_has_tiles_within_selection()) {
                if (is_key_down(KEY_Y)) {
                    editor_copy_selected_tile_region(frame_arena);
                } else {
                    editor_move_selected_tile_region(frame_arena);
                }
                editor_end_selection_region();
            }
        }

        if (is_key_down(KEY_CTRL)) {
            int mouse_position[2];
            get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
            editor_begin_selection_region(mouse_position[0], mouse_position[1]);

            /* resize region */
            if (editor.selection_region_exists) {
                int distance_delta_x = (mouse_position[0]) - editor.selection_region.x;
                int distance_delta_y = (mouse_position[1]) - editor.selection_region.y;
                editor.selection_region.w = distance_delta_x;
                editor.selection_region.h = distance_delta_y;

                editor.selected_tile_region = editor.selection_region;
            }
        } else {
            if (editor_any_blocks_within_region()) {
                editor_end_selection_region();
            }
        }

        if (editor.selection_region_exists) {
            struct rectangle region = editor.selection_region;

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
                editor.selection_region.x = mouse_position[0];
                editor.selection_region.y = mouse_position[1];
            }
        }
    }

    editor.placement_type = clampi(editor.placement_type, TILE_SOLID, TILE_ID_COUNT-1);

    begin_graphics_frame(&editor_camera); {
        /* cursor */
        {
            int mouse_position[2];
            bool left_click, right_click;

            get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
            get_mouse_buttons(&left_click, 0, &right_click);

            {
                const int SURROUNDER_VISUAL_SIZE = 5;
                draw_grid(mouse_position[0]-(SURROUNDER_VISUAL_SIZE/2),
                          mouse_position[1]-(SURROUNDER_VISUAL_SIZE/2),
                          SURROUNDER_VISUAL_SIZE, SURROUNDER_VISUAL_SIZE,
                          ((sinf(global_elapsed_time*4) + 1)/2.0f) * 2.f + 1.f, COLOR4F_BLUE);
            }

            if (editor.painting_grass) {
                for (unsigned blade_index = 0; blade_index < GRASS_DENSITY_PER_TILE; ++blade_index) {
                    float blade_x = mouse_position[0] + blade_index * (GRASS_BLADE_WIDTH);
                    float blade_y = mouse_position[1] + 1;

                    draw_bresenham_filled_rectangle_line(blade_x, blade_y,
                                                         0, 0, 0, -GRASS_BLADE_MAX_HEIGHT, VPIXEL_SZ, active_colorscheme.primary);
                }
            } else {
                draw_texture(tile_textures[editor.placement_type],
                             mouse_position[0], mouse_position[1], 1, 1,
                             color4f(1, 1, 1, 0.5 + 0.5 * ((sinf(global_elapsed_time) + 1) / 2.0f)));
            }

            if (left_click) {
                editor_try_to_place_block(mouse_position[0], mouse_position[1]);
            } else if (right_click) {
                editor_erase_block(mouse_position[0], mouse_position[1]);
            }
        }
    } end_graphics_frame();

    begin_graphics_frame(NULL);{
        int mouse_position[2];
        get_mouse_location_in_camera_space(mouse_position, mouse_position+1);

        if (!editor.painting_grass) {
            draw_text(test_font, 0, 0, format_temp("tiles present: %d\n", editor.tilemap.tile_count), active_colorscheme.text);

            /*"tool" bar*/
            {
                float font_height = font_size_aspect_ratio_independent(0.03);
                int frame_pad = 3;
                int frame_size = font_height+frame_pad;
                draw_texture(tile_textures[editor.placement_type], frame_pad, font_height+frame_pad, frame_size, frame_size, COLOR4F_WHITE);
                draw_text(test_font, frame_size * 1.5, font_height, tile_type_strings[editor.placement_type], active_colorscheme.text);
                draw_text(test_font, frame_size * 1.5, font_height*2, editor_tile_layer_strings[editor.editting_tile_layer], active_colorscheme.text);
            }
        } else {
            draw_text(test_font, 0, 0, format_temp("grass present: %d\n", editor.tilemap.grass_tile_count), active_colorscheme.text);
        }
    } end_graphics_frame();
}

local void tilemap_editor_handle_paint_transition_mode(struct memory_arena* frame_arena, float dt) {
    int mouse_position[2];
    bool left_click, right_click;

    get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
    get_mouse_buttons(&left_click, 0, &right_click);

    begin_graphics_frame(&editor_camera); {
        /* cursor */
        {
            struct transition_zone* already_selected = (struct transition_zone*) editor.context;
            /* Not deduplicated, because of irregular naming convention LOL. It's okay I guess. */
            if (left_click) {
                if (already_selected) {
                    /*handle*/
                    if (!editor.dragging &&
                        rectangle_overlapping_v(already_selected->x, already_selected->y, already_selected->w, already_selected->h, mouse_position[0], mouse_position[1], 1, 1)) {
                        editor.dragging = true;
                    } else if (editor.dragging) {
                        already_selected->x = mouse_position[0];
                        already_selected->y = mouse_position[1];
                    }
                } else {
                    struct transition_zone* transition = editor_existing_transition_at(editor.tilemap.transitions, editor.tilemap.transition_zone_count, mouse_position[0], mouse_position[1]);

                    if (!transition) {
                        transition = editor_allocate_transition();
                        transition->x = mouse_position[0];
                        transition->y = mouse_position[1];
                        transition->w = 5;
                        transition->h = 5;
                    }

                    editor.context = transition;
                }
            } else {
                editor.dragging = false;
            }

            if (right_click) {
                /*
                  This only works cause it's an array. And this is actually okay.
                */
                struct transition_zone* transition = editor_existing_transition_at(editor.tilemap.transitions, editor.tilemap.transition_zone_count, mouse_position[0], mouse_position[1]);
                if (transition) {
                    unsigned index = (transition - editor.tilemap.transitions);
                    editor.tilemap.transitions[index] = editor.tilemap.transitions[--editor.tilemap.transition_zone_count];
                } else {
                    editor.context = NULL;
                }
            }

            if (is_key_pressed(KEY_ESCAPE)) {
                editor.context = NULL;
            }

            if (already_selected) draw_rectangle(already_selected->x, already_selected->y, already_selected->w, already_selected->h, color4f(0.2, 0.3, 1.0, 1.0));
        }
    } end_graphics_frame();

    /* editor ui */
    {
        struct transition_zone* already_selected = (struct transition_zone*) editor.context;
        if (already_selected)  {
            begin_graphics_frame(NULL); {
                float font_height = font_size_aspect_ratio_independent(0.03);
                int dimens[2];
                get_screen_dimensions(dimens, dimens+1);

                draw_text_right_justified(test_font, 0, 0, dimens[0],
                                          format_temp("TRANSITION PROPERTIES\nNAME: \"%s\"\nX: %d\nY: %d\nW: %d\nH: %d\nLINKFILE: \"%s\"\nIDENTIFIER: \"%s\"\n",
                                                      already_selected->identifier, already_selected->x, already_selected->y, already_selected->w, already_selected->h, already_selected->zone_filename, already_selected->zone_link), COLOR4F_WHITE);
            } end_graphics_frame();

            Generate_Rectangle_Sizing_Code(already_selected);

            if (!is_editting_text()) {
                if (is_key_down(KEY_1)) {
                    editor_open_text_edit_prompt("SET TRANSITION ZONE NAME", already_selected->identifier, TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH, strlen(already_selected->identifier));
                }
                if (is_key_down(KEY_2)) {
                    editor_open_text_edit_prompt("SET TRANSITION ZONE FILENAME", already_selected->zone_filename, FILENAME_MAX_LENGTH, strlen(already_selected->zone_filename));
                }
                if (is_key_down(KEY_3)) {
                    editor_open_text_edit_prompt("SET TRANSITION ZONE LINKNAME", already_selected->zone_link, TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH, strlen(already_selected->zone_link));
                }
            }
        }
    }
}

local void tilemap_editor_handle_bounds_establishment(struct memory_arena* frame_arena, float dt) {
    begin_graphics_frame(&editor_camera); {
        float bounds_width  = editor.tilemap.bounds_max_x - editor.tilemap.bounds_min_x;
        float bounds_height = editor.tilemap.bounds_max_y - editor.tilemap.bounds_min_y;

        draw_filled_rectangle(editor.tilemap.bounds_min_x, editor.tilemap.bounds_min_y,
                              bounds_width, bounds_height, color4f(0, 0.2, 0.4, normalized_sinf(global_elapsed_time * 3.0) - 0.1f));

        if (editor.selection_region_exists) {
            draw_filled_rectangle(editor.selection_region.x, editor.selection_region.y,
                                  editor.selection_region.w, editor.selection_region.h, color4f(0.5, 0.2, 0.4, normalized_sinf(global_elapsed_time * 5.0) - 0.1f));
        }
    } end_graphics_frame();


    {
        int mouse_position[2];
        bool left_click, right_click;

        get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
        get_mouse_buttons(&left_click, 0, &right_click);

        if (left_click) {
            editor_begin_selection_region(mouse_position[0], mouse_position[1]);

            if (editor.selection_region_exists) {
                int distance_delta_x = (mouse_position[0]) - editor.selection_region.x;
                int distance_delta_y = (mouse_position[1]) - editor.selection_region.y;

                editor.selection_region.w = distance_delta_x;
                editor.selection_region.h = distance_delta_y;
            }
        }

        /*sanity correction cause the selection_region system doesn't work with negatives*/
        {
            if (editor.tilemap.bounds_min_x > editor.tilemap.bounds_max_x) {
                swap(int, editor.tilemap.bounds_min_x, editor.tilemap.bounds_max_x);
            }

            if (editor.tilemap.bounds_min_y > editor.tilemap.bounds_max_y) {
                swap(int, editor.tilemap.bounds_min_y, editor.tilemap.bounds_max_y);
            }
        }
    }

    if (is_key_pressed(KEY_RETURN)) {
        editor.tilemap.bounds_min_x = editor.selection_region.x;
        editor.tilemap.bounds_min_y = editor.selection_region.y;
        editor.tilemap.bounds_max_x = editor.selection_region.x + editor.selection_region.w;
        editor.tilemap.bounds_max_y = editor.selection_region.y + editor.selection_region.h;
        editor_end_selection_region();
    }
}

local void tilemap_editor_handle_paint_playerspawn_mode(struct memory_arena* frame_arena, float dt) {
    begin_graphics_frame(&editor_camera); {
        {
            int mouse_position[2];
            bool left_click, right_click;

            get_mouse_location_in_camera_space(mouse_position, mouse_position+1);
            get_mouse_buttons(&left_click, 0, &right_click);

            struct player_spawn_link* already_selected = (struct player_spawn_link*) editor.context;

            if (left_click) {
                if (already_selected) {
                    if (!editor.dragging &&
                        rectangle_overlapping_v(already_selected->x, already_selected->y, 1, 2, mouse_position[0], mouse_position[1], 1, 1)) {
                        editor.dragging = true;
                    } else if (editor.dragging) {
                        already_selected->x = mouse_position[0];
                        already_selected->y = mouse_position[1];
                    }
                } else {
                    struct player_spawn_link* spawn = editor_existing_spawn_at(editor.tilemap.player_spawn_links, editor.tilemap.player_spawn_link_count, mouse_position[0], mouse_position[1]);

                    if (!spawn) {
                        spawn = editor_allocate_spawn();
                        spawn->x = mouse_position[0];
                        spawn->y = mouse_position[1];
                    }

                    editor.context = spawn;
                }
            } else {
                editor.dragging = false;
            }

            /* cannot be deduplicated. */
            if (right_click) {
                struct player_spawn_link* spawn = editor_existing_spawn_at(editor.tilemap.player_spawn_links, editor.tilemap.player_spawn_link_count, mouse_position[0], mouse_position[1]);

                if (spawn) {
                    if (spawn != &editor.tilemap.default_spawn) {
                        unsigned index = (spawn - editor.tilemap.player_spawn_links);
                        editor.tilemap.player_spawn_links[index] = editor.tilemap.player_spawn_links[--editor.tilemap.player_spawn_link_count];
                    }
                } else {
                    editor.context = NULL;
                }
            }

            if (is_key_pressed(KEY_ESCAPE)) {
                editor.context = NULL;
            }

            if (already_selected) {
                if (!is_editting_text()) {
                    if (is_key_down(KEY_1) && already_selected != &editor.tilemap.default_spawn) {
                        editor_open_text_edit_prompt("SET NAME", already_selected->identifier, TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH, strlen(already_selected->identifier));
                    }
                }
                
                draw_rectangle(already_selected->x, already_selected->y, 1, 2, color4f(0.2, 0.3, 1.0, 1.0));
            }
        }
    } end_graphics_frame();
    {
        struct player_spawn_link* already_selected = (struct player_spawn_link*) editor.context;

        if (already_selected) {
            begin_graphics_frame(NULL); {
                float font_height = font_size_aspect_ratio_independent(0.03);
                int dimens[2];
                get_screen_dimensions(dimens, dimens+1);

                if (already_selected == &editor.tilemap.default_spawn) {
                    draw_text_right_justified(test_font, 0, 0, dimens[0], format_temp("DEFAULT PLAYER SPAWN\nX: %d\nY: %d", already_selected->x, already_selected->y), COLOR4F_WHITE);
                } else {
                    draw_text_right_justified(test_font, 0, 0, dimens[0], format_temp("SPAWN PROPERTIES\nNAME: \"%s\"\nX: %d\nY: %d", already_selected->identifier, already_selected->x, already_selected->y), COLOR4F_WHITE);
                }
            } end_graphics_frame();
        }
    }
}

/* copy and pastes of the transition editting code since both of these are just editting rectangles */
#include "painting_soul_anchors.c"
#include "painting_triggers.c"
#include "painting_entities.c"
#include "painting_camera_focus_zones.c"
#include "painting_doors.c"
#include "painting_activators.c"

local void tilemap_editor_handle_paint_entities_mode(struct memory_arena* frame_arena, float dt) {
    if (is_key_pressed(KEY_MINUS)) {
        editor.entity_painting_subtype -= 1;
    } else if (is_key_pressed(KEY_EQUALS)) {
        editor.entity_painting_subtype += 1;
    }

    editor.entity_painting_subtype =
        clampi(editor.entity_painting_subtype,
               ENTITY_PAINTING_SUBTYPE_ENTITIES,
               ENTITY_PAINTING_SUBTYPE_COUNT-1);

    switch (editor.entity_painting_subtype) {
        case ENTITY_PAINTING_SUBTYPE_ENTITIES: {
            tilemap_editor_painting_entities(frame_arena, dt);
        } break;
        case ENTITY_PAINTING_SUBTYPE_TRIGGERS: {
            tilemap_editor_painting_triggers(frame_arena, dt);
        } break;
        case ENTITY_PAINTING_SUBTYPE_CAMERA_FOCUS_ZONES: {
            tilemap_editor_painting_camera_focus_zones(frame_arena, dt);
        } break;
        case ENTITY_PAINTING_SUBTYPE_SOUL_ANCHORS: {
            tilemap_editor_painting_soul_anchors(frame_arena, dt);
        } break;
        case ENTITY_PAINTING_SUBTYPE_DOORS: {
            tilemap_editor_painting_doors(frame_arena, dt);
        } break;
        case ENTITY_PAINTING_SUBTYPE_ACTIVATORS: {
            tilemap_editor_painting_activators(frame_arena, dt);
        } break;
    }

    begin_graphics_frame(NULL); {
        begin_graphics_frame(NULL); {
            float font_height = font_size_aspect_ratio_independent(0.03);
            int dimens[2];
            get_screen_dimensions(dimens, dimens+1);

            draw_text_right_justified(test_font, 0, 0, dimens[0], editor_painting_subtype_strings[editor.entity_painting_subtype], COLOR4F_WHITE);
        } end_graphics_frame();
    } end_graphics_frame();
}

void editor_on_prompt_submission(void) {
    assert(editor.context != NULL && "Hmm, this should be impossible");
    char* prompt_title = editor.text_edit.prompt_title;

    if (editor.tool == EDITOR_TOOL_PAINT_ENTITIES) {
        if (editor.entity_painting_subtype == ENTITY_PAINTING_SUBTYPE_TRIGGERS) {
            struct trigger* already_selected = (struct trigger*) editor.context;
            if (already_selected->type == TRIGGER_TYPE_PROMPT) {
                /* pointer comparison */
                if (prompt_title == "SET PROMPT ID") {
                    already_selected->params[0] = atoi(editor.text_edit.buffer_target);
                }
            }
        } else if (editor.entity_painting_subtype == ENTITY_PAINTING_SUBTYPE_CAMERA_FOCUS_ZONES) {
            struct camera_focus_zone* already_selected = (struct camera_focus_zone*) editor.context;
            if (prompt_title == "FOCUS ZONE ZOOM") {
                already_selected->zoom = atof(editor.text_edit.buffer_target);
            } else if (prompt_title == "FOCUS ZONE INTERPOLATION SPEED X") {
                already_selected->interpolation_speed[0] = atof(editor.text_edit.buffer_target);
            } else if (prompt_title == "FOCUS ZONE INTERPOLATION SPEED Y") {
                already_selected->interpolation_speed[1] = atof(editor.text_edit.buffer_target);
            } else if (prompt_title == "FOCUS ZONE INTERPOLATION SPEED ZOOM") {
                already_selected->interpolation_speed[2] = atof(editor.text_edit.buffer_target);
            }
        } else if (editor.entity_painting_subtype == ENTITY_PAINTING_SUBTYPE_DOORS) {
            struct door* already_selected = (struct door*) editor.context;

            if (prompt_title == "SET BOSS TYPE ID") {
                already_selected->boss_type_id = atoi(editor.text_edit.buffer_target);
            }
        } else if (editor.entity_painting_subtype == ENTITY_PAINTING_SUBTYPE_ACTIVATORS) {
            struct activation_switch* already_selected = (struct activation_switch*) editor.context;

            if (prompt_title == "set target1 name") {
                strncpy(already_selected->targets[0].identifier, editor.text_edit.buffer_target, 16);
            }
            if (prompt_title == "set target2 name") {
                strncpy(already_selected->targets[1].identifier, editor.text_edit.buffer_target, 16);
            }
            if (prompt_title == "set target3 name") {
                strncpy(already_selected->targets[2].identifier, editor.text_edit.buffer_target, 16);
            }
            if (prompt_title == "set target4 name") {
                strncpy(already_selected->targets[3].identifier, editor.text_edit.buffer_target, 16);
            }
            if (prompt_title == "set target5 name") {
                strncpy(already_selected->targets[4].identifier, editor.text_edit.buffer_target, 16);
            }
            if (prompt_title == "set target6 name") {
                strncpy(already_selected->targets[5].identifier, editor.text_edit.buffer_target, 16);
            }
            if (prompt_title == "set target7 name") {
                strncpy(already_selected->targets[6].identifier, editor.text_edit.buffer_target, 16);
            }
            if (prompt_title == "set target8 name") {
                strncpy(already_selected->targets[7].identifier, editor.text_edit.buffer_target, 16);
            }
            if (prompt_title == "set target9 name") {
                strncpy(already_selected->targets[8].identifier, editor.text_edit.buffer_target, 16);
            }
            if (prompt_title == "set target10 name") {
                strncpy(already_selected->targets[9].identifier, editor.text_edit.buffer_target, 16);
            }
        }
    }
}
