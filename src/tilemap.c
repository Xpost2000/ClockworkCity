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

struct tilemap_sample_interval {
    int min_x;
    int min_y;
    int max_x;
    int max_y;
};

struct tile {
    /*tiles will store relative positions (in the case of moving tile islands)*/
    /*this is redundant, but this'll allow me to use a different storage method if I want.*/
    /*also simplifies some other code later, as I don't need to derive information like the tile rectangle*/
    int32_t x;
    int32_t y;
    uint32_t id;
};

#define TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH (16)
struct transition_zone {
    int32_t x;
    int32_t y;

    int16_t w;
    int16_t h;
    char zone_filename[FILENAME_MAX_LENGTH]; /*cstr*/
    char identifier[TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH];
    char zone_link[TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH];
};

struct player_spawn {
    int32_t x;
    int32_t y;
};

struct player_spawn_link {
    int32_t x;
    int32_t y;

    char identifier[TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH];
};

/* should be "streamed" */
/* rename to level */
struct tilemap {
    /* this really... Shouldn't be used frequently... */
    struct player_spawn default_spawn;

    float bounds_min_x;
    float bounds_min_y;
    float bounds_max_x;
    float bounds_max_y;

    /*old*/
    uint32_t width;
    uint32_t height;
    /*old*/
    struct tile* tiles;

    uint8_t transition_zone_count;
    uint8_t player_spawn_link_count;
    struct transition_zone* transitions;
    struct player_spawn_link* link_spawns;
};

void load_all_tile_assets(void) {
    for (unsigned i = TILE_NONE+1; i < TILE_ID_COUNT; ++i) {
        char* fstring = tile_id_filestrings[i];
        if (fstring) {
            tile_textures[i] = load_texture(fstring);
        }
    }
}

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

    safe_assignment(x) = min_x;
    safe_assignment(y) = min_y;
    safe_assignment(w) = width;
    safe_assignment(h) = height;
}

local struct tilemap_sample_interval tilemap_sampling_region_around_moving_entity(struct tilemap* tilemap, struct entity* entity) {
    /* NOTE(jerry): Takes advantage of the fact we store the tilemap tiles as a 2D array. This just allows for
     faster collision without me doing any real work. */
    /* This is really just a glorified clamped interval. Infact this is longer than it should be but whatever. */
    /* NOTE(jerry): Name indicates I should extrapolate the interval through the velocity to allow for better
     "continuous" collision. Not doing this yet though. */
    const unsigned TILE_PRUNE_RADIUS = 5; /*default prefetch radius*/

    int entity_ceiled_x = ceilf(entity->x);
    int entity_ceiled_y = ceilf(entity->y);
    int entity_ceiled_w = ceilf(entity->w);
    int entity_ceiled_h = ceilf(entity->h);

    int min_tile_x;
    int min_tile_y;
    get_bounding_rectangle_for_tiles(tilemap->tiles, tilemap->height * tilemap->width, &min_tile_x, &min_tile_y, 0, 0);

    /* Coordinate space transform for correct array positions. */
    int sample_center_x = (entity_ceiled_x - min_tile_x);
    int sample_center_y = (entity_ceiled_y - min_tile_y);

    int sample_min_x = sample_center_x - TILE_PRUNE_RADIUS;
    int sample_min_y = sample_center_y - TILE_PRUNE_RADIUS;

    int sample_max_x = sample_center_x + TILE_PRUNE_RADIUS + entity_ceiled_w;
    int sample_max_y = sample_center_y + TILE_PRUNE_RADIUS + entity_ceiled_h;

    if (sample_min_x < 0) sample_min_x = 0;
    if (sample_min_y < 0) sample_min_y = 0;

    if (sample_max_x > tilemap->width)  sample_max_x = tilemap->width;
    if (sample_max_y > tilemap->height) sample_max_y = tilemap->height;

    /* minimizing bounding area */
    struct tilemap_sample_interval result = (struct tilemap_sample_interval) {
        .min_x = sample_min_x, .min_y = sample_min_y,
        .max_x = sample_max_x, .max_y = sample_max_y,
    };

#if 0
    /*
      I cannot actually say if this is faster, frametimings seem pretty close
      with or without this.

      I'll leave it in, but just not compile it cause I have no idea if this works.
     */
    {
        int best_min_x = sample_min_x;
        int best_min_y = sample_min_y;
        int best_max_x = sample_max_x;
        int best_max_y = sample_max_y;

        for (unsigned y = result.min_y; y < result.max_y; ++y) {
            for (unsigned x = result.min_x; x < result.max_x; ++x) {
                struct tile* t = &tilemap->tiles[y * tilemap->width + x];
                if (t->id == TILE_NONE) continue;

                if (x < best_min_x) best_min_x = x;
                if (x > best_max_x) best_max_x = x;
                if (y < best_min_y) best_min_y = y;
                if (y > best_max_y) best_max_y = y;
            }
        }

        result.min_x = best_min_x;
        result.min_y = best_min_y;
        result.max_x = best_max_x;
        result.max_y = best_max_y;
    }
#endif

    return result;
}

float tile_get_slope_height(struct tile* t, float x, float w, float h) {
    float tile_x = t->x;
    float tile_y = t->y;
    float tile_w = 1;
    float tile_h = 1;

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
            float slope_x_offset = clampf((x - tile_x), 0, tile_w);
            return ((tile_y + tile_h) - slope_x_offset);
        } break;
    }
}

void draw_player_spawn(struct player_spawn* spawn) {
    draw_rectangle(spawn->x, spawn->y, 1, 2, COLOR4F_GREEN);
    draw_filled_rectangle(spawn->x+0.5-0.125, spawn->y+2, 0.25, 0.25, COLOR4F_WHITE);
}

void draw_tiles(struct tile* tiles, size_t count) {
    for (unsigned index = 0; index < count; ++index) {
        struct tile* t = &tiles[index];
        draw_texture(tile_textures[t->id],
                     t->x, t->y,
                     1, 1,
                     active_colorscheme.primary);
    } 
}

void draw_transitions(struct transition_zone* transitions, size_t count) {
    for (unsigned index = 0; index < count; ++index) {
        struct transition_zone* t = &transitions[index];
        draw_rectangle(t->x, t->y, t->w, t->h, COLOR4F_RED);
    } 
}

void draw_player_spawn_links(struct player_spawn_link* spawns, size_t count) {
    for (unsigned index = 0; index < count; ++index) {
        struct player_spawn_link* spawn = spawns + index;
        draw_player_spawn(spawn);
    }
}

void draw_tilemap(struct tilemap* tilemap) {
    draw_tiles(tilemap->tiles, tilemap->height * tilemap->width);
}

local bool tile_intersects_rectangle(struct tile* t, float x, float y, float w, float h) {
    float tile_x = t->x;
    float tile_y = t->y;
    float tile_w = 1;
    float tile_h = 1;

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

/* NOTE(jerry): scans all tiles, when I don't have to anymore. This only happens on tiles though... So okay. */
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
            /* if (excluded_tile->id == TILE_SLOPE_BL || excluded_tile->id == TILE_SLOPE_BR) { */
            /*     if (t->id == TILE_SLOPE_BL || t->id == TILE_SLOPE_BR) { */
            /*         return false; */
            /*     } */
            /* } */

            return true;
        }
    }

    for(unsigned other_index = excluded_index+1; other_index < total_tile_count; ++other_index) {
        struct tile* t = &tilemap->tiles[other_index];

        float tile_x = t->x;
        float tile_y = t->y;

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

bool entity_already_in_slope(struct tilemap* tilemap, struct entity* entity) {
    struct tilemap_sample_interval sample_region = tilemap_sampling_region_around_moving_entity(tilemap, entity);
    
    for (unsigned y = sample_region.min_y; y < sample_region.max_y; ++y) {
        for (unsigned x = sample_region.min_x; x < sample_region.max_x; ++x) {
            struct tile* t = &tilemap->tiles[y * tilemap->width + x];

            if (t->id == TILE_NONE) continue;
            if (rectangle_intersects_v(entity->x, entity->y, entity->w, entity->h, t->x, t->y, 1, 1)) {
                switch (t->id) {
                    case TILE_SLOPE_R:
                    case TILE_SLOPE_L: {
                        const float DISTANCE_EPSILION = 0.4;
                        if (fabs(entity->y - tile_get_slope_height(t, entity->x, entity->w, entity->h)) <= DISTANCE_EPSILION) {
                            return true;
                        }
                    } break;
                    default: break;
                }
            }
        }
    }

    return false;
}

void do_moving_entity_horizontal_collision_response(struct tilemap* tilemap, struct entity* entity, float dt) {
    float old_x = entity->x;
    float old_vy = entity->vy;
    entity->onground = false;
    struct tilemap_sample_interval sample_region = tilemap_sampling_region_around_moving_entity(tilemap, entity);

    bool in_slope = entity_already_in_slope(tilemap, entity);

    const int CONTINUOUS_SUBSTEPS = 16;
    for (unsigned substep = 0; substep < CONTINUOUS_SUBSTEPS; ++substep) {
        float vx_substep = entity->vx / CONTINUOUS_SUBSTEPS;

        if (in_slope) {
            entity->x += vx_substep * sinf(degrees_to_radians(45)) * dt;
        } else {
            entity->x += vx_substep * dt;
        }

        for (unsigned y = sample_region.min_y; y < sample_region.max_y; ++y) {
            for (unsigned x = sample_region.min_x; x < sample_region.max_x; ++x) {
                struct tile* t = &tilemap->tiles[y * tilemap->width + x];
                if(t->id == TILE_NONE) continue;

                float old_y = entity->y;

                if (rectangle_intersects_v(entity->x, entity->y, entity->w, entity->h, t->x, t->y, 1, 1)) {
                    if (tile_is_slope(t)) {
                        if (t->id == TILE_SLOPE_L || t->id == TILE_SLOPE_R) {
                            float slope_snapped_location = tile_get_slope_height(t, entity->x, entity->w, entity->h) - PHYSICS_EPSILION;

                            if (t->id == TILE_SLOPE_L) {
                                switch (rectangle_closest_intersection_edge_v(entity->x, entity->y, entity->w, entity->h, t->x, t->y, 1, 1)) {
                                    case INTERSECTION_EDGE_LEFT:
                                    case INTERSECTION_EDGE_TOP: {
                                        if (entity->y + entity->h <= (t->y + 1)) {
                                            if (entity->y > slope_snapped_location) {
                                                entity->y = slope_snapped_location;
                                                entity->onground = true;

                                                if (entity_intersects_any_tiles_excluding(entity, tilemap, y * tilemap->width + x)) {
                                                    entity->x = old_x;
                                                    entity->y = old_y;
                                                    entity->vx = 0;
                                                }
                                            }
                                        } else {
                                            entity->x = t->x - entity->w;
                                            entity->vx = 0;
                                        }
                                    } break;
                                    case INTERSECTION_EDGE_BOTTOM: {
                                        entity->y = t->y + 1;
                                        entity->last_vy = old_vy;
                                        entity->vy = 0;
                                    } break;
                                    case INTERSECTION_EDGE_RIGHT: {
                                        entity->x = t->x + 1;
                                        entity->vx = 0;
                                    } break;
                                        invalid_cases();
                                }
                            } else if (t->id == TILE_SLOPE_R) {
                                switch (rectangle_closest_intersection_edge_v(entity->x, entity->y, entity->w, entity->h, t->x, t->y, 1, 1)) {
                                    case INTERSECTION_EDGE_RIGHT:
                                    case INTERSECTION_EDGE_TOP: {
                                        if (entity->y + entity->h <= (t->y + 1)) {
                                            if (entity->y > slope_snapped_location) {
                                                entity->y = slope_snapped_location;
                                                entity->onground = true;

                                                if (entity_intersects_any_tiles_excluding(entity, tilemap, y * tilemap->width + x)) {
                                                    entity->x = old_x;
                                                    entity->y = old_y;
                                                    entity->vx = 0;
                                                }
                                            }
                                        } else {
                                            entity->x = t->x + 1;
                                            entity->vx = 0;
                                        }
                                    } break;
                                    case INTERSECTION_EDGE_BOTTOM: {
                                        entity->y = t->y + 1;
                                        entity->last_vy = old_vy;
                                        entity->vy = 0;
                                    } break;
                                    case INTERSECTION_EDGE_LEFT: {
                                        entity->x = t->x - entity->w;
                                        entity->vx = 0;
                                    } break;
                                        invalid_cases();
                                }
                            }
                        } else {
                            float slope_snapped_location = tile_get_slope_height(t, entity->x, entity->w, entity->h) - PHYSICS_EPSILION;
                            if (t->id == TILE_SLOPE_BR) {
                                switch (rectangle_closest_intersection_edge_v(entity->x, entity->y, entity->w, entity->h, t->x, t->y, 1, 1)) {
                                    case INTERSECTION_EDGE_TOP: {
                                        entity->onground = true;
                                        entity->y = t->y - entity->h;
                                        entity->last_vy = old_vy;
                                        entity->vy = 0;
                                    } break;
                                    case INTERSECTION_EDGE_RIGHT:
                                    case INTERSECTION_EDGE_BOTTOM: {
                                        if (entity->y > t->y) {
                                            if (entity->y < slope_snapped_location) {
                                                entity->y = slope_snapped_location;

                                                if (entity_intersects_any_tiles_excluding(entity, tilemap, y * tilemap->width + x)) {
                                                    entity->x = old_x;
                                                    entity->y = old_y;
                                                    entity->vx = 0;
                                                }

                                                if (entity->vy < 0) {
                                                    entity->last_vy = old_vy;
                                                    entity->vy = 0;
                                                }
                                            }
                                        } else {
                                            entity->x = t->x + 1;
                                        }
                                    } break;
                                    case INTERSECTION_EDGE_LEFT: {
                                        entity->x = t->x - entity->w;
                                        entity->vx = 0;
                                    } break;
                                        invalid_cases();
                                }
                            } else if (t->id == TILE_SLOPE_BL) {
                                switch (rectangle_closest_intersection_edge_v(entity->x, entity->y, entity->w, entity->h, t->x, t->y, 1, 1)) {
                                    case INTERSECTION_EDGE_TOP: {
                                        entity->y = t->y - entity->h;
                                        entity->onground = true;
                                        entity->last_vy = old_vy;
                                        entity->vy = 0;
                                    } break;
                                    case INTERSECTION_EDGE_LEFT:
                                    case INTERSECTION_EDGE_BOTTOM: {
                                        if (entity->y > t->y) {
                                            if (entity->y < slope_snapped_location) {
                                                entity->y = slope_snapped_location;

                                                if (entity_intersects_any_tiles_excluding(entity, tilemap, y * tilemap->width + x)) {
                                                    entity->x = old_x;
                                                    entity->y = old_y;
                                                    entity->vx = 0;
                                                }

                                                if (entity->vy < 0) {
                                                    entity->last_vy = old_vy;
                                                    entity->vy = 0;
                                                }
                                            }
                                        } else {
                                            entity->x = t->x - (entity->w);
                                            entity->vx = 0;
                                        }
                                    } break;
                                    case INTERSECTION_EDGE_RIGHT: {
                                        entity->x = t->x + 1;
                                        entity->vx = 0;
                                    } break;
                                        invalid_cases();
                                }
                            }
                        }
                    } else {
                        float entity_right_edge = entity->x + entity->w;
                        float tile_right_edge = (t->x + 1);

                        if (entity_right_edge > t->x && entity_right_edge < tile_right_edge) {
                            entity->x = t->x - (entity->w);
                            entity->vx = 0;
                        } else if (entity->x < tile_right_edge && entity->x > t->x) {
                            entity->x = tile_right_edge;
                            entity->vx = 0;
                        }
                    }
                }
            }
        }
    }
}

void do_moving_entity_vertical_collision_response(struct tilemap* tilemap, struct entity* entity, float dt) {
    float old_vy = entity->vy;
    float old_y = entity->y;
    struct tilemap_sample_interval sample_region = tilemap_sampling_region_around_moving_entity(tilemap, entity);

    entity->y += entity->vy * dt;

    {
        for (unsigned y = sample_region.min_y; y < sample_region.max_y; ++y) {
            for (unsigned x = sample_region.min_x; x < sample_region.max_x; ++x) {
                struct tile* t = &tilemap->tiles[y * tilemap->width + x];

                if (t->id == TILE_NONE) continue;
                if (rectangle_intersects_v(entity->x, entity->y, entity->w, entity->h, t->x, t->y, 1, 1)) {
                    switch (t->id) {
                        case TILE_SOLID: {
                            float entity_bottom_edge = entity->y + entity->h;
                            float tile_bottom_edge = (t->y + 1);

                            if (entity_bottom_edge >= t->y && entity_bottom_edge <= tile_bottom_edge) {
                                entity->y = t->y - entity->h;
                                entity->onground = true;
                            } else if (entity->y <= tile_bottom_edge && entity_bottom_edge >= tile_bottom_edge) {
                                entity->y = tile_bottom_edge;
                            }

                            entity->last_vy = old_vy;
                            entity->vy = 0;
                        } break;
                        default: break;
                    }
                }
            }
        }
    }

    /* handle slope snapping as a separate step. */
    if (entity->vy >= 0) {
        float extended_height = entity->h;
        float extended_vy     = GRAVITY_CONSTANT;

        extended_height += extended_vy * dt;

        for (unsigned y = sample_region.min_y; y < sample_region.max_y; ++y) {
            for (unsigned x = sample_region.min_x; x < sample_region.max_x; ++x) {
                struct tile* t = &tilemap->tiles[y * tilemap->width + x];

                if (t->id == TILE_NONE) continue;
                if (rectangle_intersects_v(entity->x, entity->y, entity->w, extended_height, t->x, t->y, 1, 1)) {
                    switch (t->id) {
                        case TILE_SLOPE_R:
                        case TILE_SLOPE_L: {
                            if (fabs(tile_get_slope_height(t, entity->x, entity->w, entity->h)+entity->h - t->y) <= PHYSICS_EPSILION *2) {
                                entity->onground = true;
                                entity->last_vy = old_vy;
                                entity->vy = 0;
                            }

                            if (entity->vy >= 0 && fabs(entity->y - tile_get_slope_height(t, entity->x, entity->w, extended_height)) <= PHYSICS_EPSILION *2) {
                                entity->y = tile_get_slope_height(t, entity->x, entity->w, entity->h);
                                entity->onground = true;
                                entity->last_vy = old_vy;
                                entity->vy = 0;
                            }
                        } break;
                    }
                }
            }
        }
    }
}
