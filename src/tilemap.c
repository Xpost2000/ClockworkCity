/*
  TODO(jerry):
  Yep... Slopes again!
  
  Technically the right answer is to use a raycast to mount onto the slope.
  
  basically

  // in the collision code...
 */
#if 0
if (tile_is_slope(t)) {
    // extend a "ray"
    struct rectangle entity_ray = (struct rectangle) {
        .x = entity->x + entity->w/2,
        .y = entity->y + entity->h,
        .w = 0.1,
        .h = entity->h/3,
    };

    float height = tile_get_slope_height(t, entity->x, entity->w, entity->h);
    if (rectangle_overlapping(entity_ray, t)) {
        if (height <= entity_ray.h + entity_ray.y) {
            snap?
        }
    }
#endif
const int TILE_TEX_SIZE = 1.0f;
const float PHYSICS_EPSILION = 0.0345;
/*these aren't really tile ids, these are more like flags/properties*/
/*core logic is here though, so I guess it don't matter, can worry about later.*/
/*need to find test platformer tileset to try out some ideas...*/
enum tile_id {
    TILE_NONE, /*shouldn't happen but okay*/
    TILE_SOLID,
    TILE_SLOPE_BR,
    TILE_SLOPE_BL,
    TILE_SLOPE_R,
    TILE_SLOPE_L,
    TILE_SPIKE,
    TILE_ID_COUNT,
};
shared_storage char* tile_id_filestrings[] = {
    0,
    [TILE_SPIKE] = "assets/testtiles/spikehazard.png",
    [TILE_SOLID] = "assets/testtiles/block.png",
    [TILE_SLOPE_BL] = "assets/testtiles/slope45degbl.png",
    [TILE_SLOPE_BR] = "assets/testtiles/slope45degbr.png",
    [TILE_SLOPE_R] = "assets/testtiles/slope45degr.png",
    [TILE_SLOPE_L] = "assets/testtiles/slope45degl.png",
};
shared_storage char* tile_type_strings[] = {
    0,
    [TILE_SPIKE] = "(hazard) spike",
    [TILE_SOLID] = "solid",
    [TILE_SLOPE_BL] = "bottom left slope(45 deg)",
    [TILE_SLOPE_BR] = "bottom right slope(45 deg)",
    [TILE_SLOPE_R] = "right slope(45 deg)",
    [TILE_SLOPE_L] = "left slope(45 deg)",
};

local texture_id tile_textures[TILE_ID_COUNT] = {};
void DEBUG_load_all_tile_assets(void) {
    for (unsigned i = TILE_NONE+1; i < TILE_ID_COUNT; ++i) {
        char* fstring = tile_id_filestrings[i];
        if (fstring) {
            tile_textures[i] = load_texture(fstring);
        }
    }
}

struct tile {
    /*tiles will store relative positions (in the case of moving tile islands)*/
    /*this is redundant, but this'll allow me to use a different storage method if I want.*/
    /*also simplifies some other code later, as I don't need to derive information like the tile rectangle*/
    int32_t x;
    int32_t y;
    uint32_t id;
};

bool tile_is_slope(struct tile* t) {
    return ((t->id == TILE_SLOPE_BL) ||
            (t->id == TILE_SLOPE_BR) ||
            (t->id == TILE_SLOPE_L)  ||
            (t->id == TILE_SLOPE_R));
}

void get_bounding_rectangle_for_tiles(struct tile* tiles, size_t tile_count, int* x, int* y, int* w, int* h) {
    int min_x = tiles[0].x;
    int min_y = tiles[0].y;
    int max_x = min_x;
    int max_y = min_y;

    for (size_t index = 0; index < tile_count; ++index) {
        struct tile* t = tiles + index;

        if (t->x < min_x) min_x = t->x;
        if (t->y < min_y) min_y = t->y;
        if (t->x > max_x) max_x = t->x;
        if (t->y > max_y) max_y = t->y;
    }

    int width = (max_x+1) - min_x;
    int height = (max_y+1) - min_y;

    console_printf("min x: %d, max x: %d\n", min_x, max_x);
    console_printf("min y: %d, max y: %d\n", min_y, max_y);

    safe_assignment(x) = min_x;
    safe_assignment(y) = min_y;
    safe_assignment(w) = width;
    safe_assignment(h) = height;
}

float tile_get_slope_height(struct tile* t, float x, float w, float h) {
    float tile_x = t->x * TILE_TEX_SIZE;
    float tile_y = t->y * TILE_TEX_SIZE;
    float tile_w = TILE_TEX_SIZE;
    float tile_h = TILE_TEX_SIZE;

    switch (t->id) {
        case TILE_SLOPE_L: {
            float slope_x_offset = clampf(((x + w) - tile_x), 0, tile_w);
            return ((tile_y + tile_h) - slope_x_offset - h);
        } break;
        case TILE_SLOPE_R: {
            float slope_x_offset = clampf((x - tile_x), 0, tile_w);
            return ((tile_y + slope_x_offset) - h);
        } break;
        case TILE_SLOPE_BL: {
            float slope_x_offset = clampf(((x + w) - tile_x), 0, tile_w);
            return (tile_y + slope_x_offset);
        } break;
        case TILE_SLOPE_BR: {
            float slope_x_offset = clampf((tile_x - x), 0, tile_w);
            return ((tile_y + tile_h) + slope_x_offset);
        } break;
    }
}
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
struct tilemap* DEBUG_tilemap_from_file(struct memory_arena* arena, char* file) {
    /*
      allocates from the top.
      Bottom is for permenant information / persistent. Top is for levels or more temp stuff
    */
    memory_arena_clear_top(&game_memory_arena);
    struct tilemap* result = memory_arena_push_top(arena, sizeof(*result));
    char* filestring = load_entire_file(file);

    {
        result->height = count_lines_of_cstring(filestring);
        result->width = 0;

        int index = 0;
        char* current_line;

        while (current_line = get_line_starting_from(filestring, &index)){
            int current_length = strlen(current_line);
            if (result->width < current_length) {
                result->width = current_length;
            }
        }
    }

    /* result->tiles = system_allocate_zeroed_memory(result.width * result.height * sizeof(*result.tiles)); */
    result->tiles = memory_arena_push_top(arena, result->width * result->height * sizeof(*result->tiles));
    printf("%d x %d\n", result->width, result->height);

    {
        int index = 0;
        char* current_line;

        for(unsigned y = 0; y < result->height; ++y) {
            current_line = get_line_starting_from(filestring, &index);

            for(unsigned x = 0; x < result->width; ++x) {
                struct tile* t = &result->tiles[y * result->width + x];
                t->y = y;
                t->x = x;

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

void draw_tiles(struct tile* tiles, size_t count) {
    for (unsigned index = 0; index < count; ++index) {
        struct tile* t = &tiles[index];
        draw_texture(tile_textures[t->id],
                     t->x * TILE_TEX_SIZE, t->y * TILE_TEX_SIZE,
                     TILE_TEX_SIZE, TILE_TEX_SIZE, COLOR4F_WHITE);
    } 
}

void DEBUG_draw_tilemap(struct tilemap* tilemap) {
    draw_tiles(tilemap->tiles, tilemap->height * tilemap->width);
}

local bool do_collision_response_tile_left_edge(struct tile* t, struct entity* ent) {
    assert(t->id != TILE_NONE && "uh, air tile tries collision?");

    float entity_right_edge = ent->x + ent->w;
    float tile_right_edge = (t->x + 1) * TILE_TEX_SIZE;

    if (entity_right_edge > t->x * TILE_TEX_SIZE && entity_right_edge < tile_right_edge) {
        ent->x = t->x*TILE_TEX_SIZE - (ent->w + PHYSICS_EPSILION);
        return true;
    }

    return false;
}

local bool do_collision_response_tile_right_edge(struct tile* t, struct entity* ent) {
    assert(t->id != TILE_NONE && "uh, air tile tries collision?");

    float tile_right_edge  = (t->x + 1) * TILE_TEX_SIZE;

    if (ent->x < tile_right_edge && ent->x > t->x * TILE_TEX_SIZE) {
        ent->x = tile_right_edge + PHYSICS_EPSILION;
        return true;
    }

    return false;
}

local bool do_collision_response_tile_top_edge(struct tile* t, struct entity* ent) {
    assert(t->id != TILE_NONE && "uh, air tile tries collision?");

    float entity_bottom_edge = ent->y + ent->h;
    float tile_bottom_edge = (t->y + 1) * TILE_TEX_SIZE;

    if (entity_bottom_edge >= t->y && entity_bottom_edge <= tile_bottom_edge) {
        ent->y = t->y * TILE_TEX_SIZE - ent->h;
        return true;
    }

    return false;
}

local bool do_collision_response_tile_bottom_edge(struct tile* t, struct entity* ent) {
    assert(t->id != TILE_NONE && "uh, air tile tries collision?");

    float entity_bottom_edge = ent->y + ent->h;
    float tile_bottom_edge = (t->y + 1) * TILE_TEX_SIZE;

    if (ent->y <= tile_bottom_edge && entity_bottom_edge >= tile_bottom_edge) {
        ent->y = tile_bottom_edge;
        return true;
    }

    return false;
}

local bool tile_intersects_rectangle(struct tile* t, float x, float y, float w, float h) {
    float tile_x = t->x * TILE_TEX_SIZE;
    float tile_y = t->y * TILE_TEX_SIZE;
    float tile_w = TILE_TEX_SIZE;
    float tile_h = TILE_TEX_SIZE;

    if (rectangle_intersects_v(x, y, w, h, tile_x, tile_y, tile_w, tile_h)) {
        if (tile_is_slope(t)) {
            if (t->id == TILE_SLOPE_R || t->id == TILE_SLOPE_L) {
                if (y > tile_get_slope_height(t, x, w, h)) {
                    return true;
                }
            } else {
                if (y < tile_get_slope_height(t, x, w, h)) {
                    return true;
                }
            }

            return false;
        }

        return true;
    }

    return false;
}

local bool entity_intersects_any_tiles_excluding(struct entity* entity, struct tilemap* tilemap, size_t excluded_index) {
    /*kind of stupid for assuming my entities are only single rectangles but I know this is true for 90% of my games...*/
    size_t total_tile_count = tilemap->height * tilemap->width;

    struct tile* excluded_tile = &tilemap->tiles[excluded_index];

    for(unsigned other_index = 0; other_index < excluded_index; ++other_index) {
        struct tile* t = &tilemap->tiles[other_index];

        if(t->id == TILE_NONE) continue;

        if (tile_intersects_rectangle(t, entity->x, entity->y, entity->w, entity->h)) {
            /*
              This prevents us from "nicking" off slopes. This only works if we are on slopes of the
              same type.
              
              This should not prevent you from transitioning to other slopes though.
             */
            if (excluded_tile->id == TILE_SLOPE_L || excluded_tile->id == TILE_SLOPE_R) {
                if (t->id == TILE_SLOPE_L || t->id == TILE_SLOPE_R) {
                    return false;
                }
            }

            return true;
        }
    }

    for(unsigned other_index = excluded_index+1; other_index < total_tile_count; ++other_index) {
        struct tile* t = &tilemap->tiles[other_index];

        float tile_x = t->x * TILE_TEX_SIZE;
        float tile_y = t->y * TILE_TEX_SIZE;
        float tile_w = TILE_TEX_SIZE;
        float tile_h = TILE_TEX_SIZE;

        if(t->id == TILE_NONE) continue;

        if (tile_intersects_rectangle(t, entity->x, entity->y, entity->w, entity->h)) {
            if (excluded_tile->id == TILE_SLOPE_L || excluded_tile->id == TILE_SLOPE_R) {
                if (t->id == TILE_SLOPE_L || t->id == TILE_SLOPE_R) {
                    return false;
                }
            }

            return true;
        }
    }

    return false;
}

/* maybe move to entity.c? */
void do_moving_entity_horizontal_collision_response(struct tilemap* tilemap, struct entity* entity, float dt) {
    float old_x = entity->x;
    /*technically wrong because slopes should slow you down. Whatever*/
    entity->x += entity->vx * dt;

    size_t total_tile_count = tilemap->height * tilemap->width;

    for(unsigned index = 0; index < total_tile_count; ++index) {
        struct tile* t = &tilemap->tiles[index];

        float tile_x = t->x * TILE_TEX_SIZE;
        float tile_y = t->y * TILE_TEX_SIZE;
        float tile_w = TILE_TEX_SIZE;
        float tile_h = TILE_TEX_SIZE;

        if(t->id == TILE_NONE) continue;

        if (rectangle_intersects_v(entity->x, entity->y, entity->w, entity->h, tile_x, tile_y, tile_w, tile_h)) {
            if (tile_is_slope(t)) {
                /* assume this is for right facing (left) slope first. (slope = 1) */
                bool taller_edge_intersection = (entity->x > tile_x + tile_w);
                bool smaller_edge_intersection = (entity->x + entity->w < tile_x);
                float taller_edge_correction_position = tile_x + tile_w;
                float smaller_edge_correction_position = tile_x - entity->w;

                /*
                  this prevents getting nicked by other slope corners. This check on the other
                  hand is required for the BR/BL slopes.
                */
                if (t->id == TILE_SLOPE_R || t->id == TILE_SLOPE_L) {
                    taller_edge_intersection = (entity->x >= tile_x + tile_w);
                    smaller_edge_intersection = (entity->x + entity->w <= tile_x);
                }

                if (t->id == TILE_SLOPE_R || t->id == TILE_SLOPE_BR) {
                    /*just reversed.*/
                    swap(bool, taller_edge_intersection, smaller_edge_intersection);
                    swap(float, taller_edge_correction_position, smaller_edge_correction_position);
                }

                if (t->id == TILE_SLOPE_L || t->id == TILE_SLOPE_R) {
                    if (taller_edge_intersection) {
                        entity->x = taller_edge_correction_position;
                        entity->vx = 0;
                    } else {
                        float slope_snapped_location = tile_get_slope_height(t, entity->x, entity->w, entity->h) - PHYSICS_EPSILION;
                        float delta_from_foot_to_tile_top = (slope_snapped_location - entity->y);
                        float delta_from_foot_to_tile_bottom = (entity->y - (tile_y + tile_h));

                        if (entity->y + entity->h < tile_y + tile_h) {
                            /*
                              TODO(jerry):
                              this is the wrong condition, I need more data to properly determine
                              when I should "anchor on".

                              This is a very small bug compared to other shit I've fucked up so far
                              so I can sleep soundly at night knowing this is still here.
                            */
                            if (entity->vy >= 0 && (entity->y >= slope_snapped_location || delta_from_foot_to_tile_top <= ((float)entity->h * 0.6))) {
                                float old_y = entity->y;
                                entity->y = slope_snapped_location;

                                if (entity_intersects_any_tiles_excluding(entity, tilemap, index)) {
                                    entity->x = old_x;
                                    entity->y = old_y;
                                    entity->vx = 0;
                                }

                                /*eh.*/
                                const float SIN45 = 0.7071067812;
                                entity->vx -= (entity->vx * SIN45 * dt);
                                entity->vy = 0;
                            }
                        } else {
                            if (smaller_edge_intersection) {
                                entity->x = smaller_edge_correction_position;
                                entity->vx = 0;
                            } else {
                                if (fabs(delta_from_foot_to_tile_bottom) < fabs(delta_from_foot_to_tile_top)) {
                                    entity->y = tile_y + tile_h;
                                } else {
                                    entity->y = slope_snapped_location;
                                }

                                entity->vy = 0;
                            }
                        }
                    }
                } else {
                    if (taller_edge_intersection) {
                        entity->x = taller_edge_correction_position;
                    } else {
                        float slope_snapped_location = tile_get_slope_height(t, entity->x, entity->w, entity->h);
                        float delta_from_foot_to_tile_top = (slope_snapped_location - entity->y);
                        float delta_from_foot_to_tile_bottom = (entity->y - (tile_y + tile_h));

                        if (entity->y >= tile_y) {
                            if (entity->y < slope_snapped_location) {
                                float old_y = entity->y;
                                entity->y = slope_snapped_location;

                                if (entity_intersects_any_tiles_excluding(entity, tilemap, index)) {
                                    entity->x = old_x;
                                    entity->y = old_y;
                                    entity->vx = 0;
                                }

                                if (entity->vy < 0 && taller_edge_intersection) {
                                    entity->vy = 0;
                                }
                            }
                        } else {
                            if (smaller_edge_intersection) {
                                entity->x = smaller_edge_correction_position;
                                entity->vx = 0;
                            }
                        }
                    }
                }
            } else {
                bool stop_movement = false;
                if (!do_collision_response_tile_left_edge(t, entity)) {
                    if (do_collision_response_tile_right_edge(t, entity)) {
                        stop_movement = true;
                    }
                } else {
                    stop_movement = true;
                }

                if (stop_movement)
                    entity->vx = 0;
            }

        }
    }
}

void do_moving_entity_vertical_collision_response(struct tilemap* tilemap, struct entity* entity, float dt) {
    float old_y = entity->y;
    entity->y += entity->vy * dt;

    size_t total_tile_count = tilemap->height * tilemap->width;

    for(unsigned index = 0; index < total_tile_count; ++index) {
        struct tile* t = &tilemap->tiles[index];

        float tile_x = t->x * TILE_TEX_SIZE;
        float tile_y = t->y * TILE_TEX_SIZE;
        float tile_w = TILE_TEX_SIZE;
        float tile_h = TILE_TEX_SIZE;

        if (t->id == TILE_NONE) continue;
        if (rectangle_intersects_v(entity->x, entity->y, entity->w, entity->h, tile_x, tile_y, tile_w, tile_h)) {
            switch (t->id) {
                case TILE_SLOPE_R:
                case TILE_SLOPE_L: {
                    const float DISTANCE_EPSILION = 0.085;
                    if (entity->vy >= 0 && fabs(entity->y - tile_get_slope_height(t, entity->x, entity->w, entity->h)) <= DISTANCE_EPSILION) {
                        /*HACK*/
                        entity->vy = 0;
                    }
                } break;
                case TILE_SLOPE_BL:
                case TILE_SLOPE_BR: {
                    float slope_location = tile_get_slope_height(t, entity->x, entity->w, entity->h);
                    float delta_from_foot_to_slope_location = (entity->y + entity->h) - slope_location;
                    float delta_from_foot_to_tile_top = (entity->y + entity->h) - tile_y;

                    if (fabs(delta_from_foot_to_tile_top) < fabs(delta_from_foot_to_slope_location)) {
                        entity->y = tile_y - entity->h;
                        entity->vy = 0;
                    }
                } break;
                default:
                case TILE_SOLID: {
                    if(!do_collision_response_tile_top_edge(t, entity)) {
                        do_collision_response_tile_bottom_edge(t, entity);
                    }

                    entity->vy = 0;
                } break;
            }

        }
    }
}

void evaluate_moving_entity_grounded_status(struct tilemap* tilemap, struct entity* entity, float dt) {
    float old_y = entity->y;
    bool was_on_ground = entity->onground;
    entity->onground = false;

    size_t total_tile_count = tilemap->height * tilemap->width;

    for (unsigned index = 0; index < total_tile_count; ++index) {
        struct tile* t = &tilemap->tiles[index];

        float tile_x = t->x * TILE_TEX_SIZE;
        float tile_y = t->y * TILE_TEX_SIZE;
        float tile_w = TILE_TEX_SIZE;
        float tile_h = TILE_TEX_SIZE;

        if(t->id == TILE_NONE) continue;
        if (!rectangle_overlapping_v(entity->x, entity->y, entity->w, entity->h, tile_x, tile_y, tile_w, tile_h)) continue;

        /*NOTE(jerry): this is slightly... different for sloped tiles.*/

        if ((old_y + entity->h <= tile_y)) {
            entity->onground = true;
        }

        if (tile_is_slope(t)) {
            float slope_location = tile_get_slope_height(t, entity->x, entity->w, entity->h) - PHYSICS_EPSILION;
            bool is_bottom_facing_tile = (t->id == TILE_SLOPE_BR || t->id == TILE_SLOPE_BL);

            if (!is_bottom_facing_tile && roundf(entity->y) == roundf(slope_location)) {
                entity->onground = true;
            } else {
                if (is_bottom_facing_tile && entity->onground) {
                    return;
                }

                entity->onground = false;
            }
        }

        if (entity->onground) {
            return;
        }
    }
}
