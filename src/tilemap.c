#define TILEMAP_CURRENT_VERSION (3) 
#define GRASS_DENSITY_PER_TILE  (6) /* in blades */
#define GRASS_BLADE_WIDTH       (VPIXEL_SZ * 16) / ((GRASS_DENSITY_PER_TILE + 0.5))
#define GRASS_BLADE_MAX_HEIGHT  (12) /* in "vpixels" */

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
    TILE_SPIKE_UP,
    TILE_SPIKE_DOWN,
    TILE_SPIKE_LEFT,
    TILE_SPIKE_RIGHT,
    TILE_ID_COUNT,
};

shared_storage char* tile_id_filestrings[] = {
    0,
    [TILE_SPIKE_UP] = "assets/testtiles/spikehazard.png",
    [TILE_SPIKE_DOWN] = "assets/testtiles/spikehazard_vertical.png",
    [TILE_SPIKE_LEFT] = "assets/testtiles/spikehazard_left.png",
    [TILE_SPIKE_RIGHT] = "assets/testtiles/spikehazard_right.png",
    [TILE_SOLID] = "assets/testtiles/block.png",
    [TILE_SLOPE_BL] = "assets/testtiles/slope45degbl.png",
    [TILE_SLOPE_BR] = "assets/testtiles/slope45degbr.png",
    [TILE_SLOPE_R] = "assets/testtiles/slope45degr.png",
    [TILE_SLOPE_L] = "assets/testtiles/slope45degl.png",
};
shared_storage char* tile_type_strings[] = {
    0,
    [TILE_SPIKE_UP]    = "(hazard) spike (up)",
    [TILE_SPIKE_DOWN]  = "(hazard) spike (down)",
    [TILE_SPIKE_LEFT]  = "(hazard) spike (left)",
    [TILE_SPIKE_RIGHT] = "(hazard) spike (right)",
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

/* NOTE(jerry):
   cleverly arranged so we can use it polymorphically with struct tile* functions.
   
   Debatable reason to separate it but I'd rather not have an additional special case...
*/
/* these are animated separately*/
/* grass tiles only plant on up facing tiles for now. */
struct grass_tile {
    int32_t x; 
    int32_t y;
};

/* TODO(jerry): typo */
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

    /* 
       These are purely decorative, and do not require
       the rectangular shape since these are never used for collision
       
       Should I try to parallax these?
    */
    uint32_t foreground_tile_count;
    uint32_t background_tile_count;
    uint32_t grass_tile_count;

    struct tile* foreground_tiles;
    struct tile* background_tiles;
    struct grass_tile* grass_tiles;

    uint8_t transition_zone_count;
    uint8_t player_spawn_link_count;
    struct transition_zone* transitions;
    struct player_spawn_link* link_spawns;
};

#define GRASS_VISUAL_INFO_TABLE_SIZE (64)

shared_storage int32_t grass_visual_height_table[GRASS_VISUAL_INFO_TABLE_SIZE]                     = {};
shared_storage float   grass_visual_period_table[GRASS_VISUAL_INFO_TABLE_SIZE]                     = {};
shared_storage int32_t  grass_visual_initial_horizontal_offset_table[GRASS_VISUAL_INFO_TABLE_SIZE] = {};

void initialize_grass_visual_tables(void) {
    console_printf("grass table init\n");
    for (unsigned index = 0; index < GRASS_VISUAL_INFO_TABLE_SIZE; ++index) {
        grass_visual_height_table[index]                    = random_ranged_integer(GRASS_BLADE_MAX_HEIGHT/2, GRASS_BLADE_MAX_HEIGHT);
        grass_visual_period_table[index]                    = random_ranged_float(3.5, 6.284);
        grass_visual_initial_horizontal_offset_table[index] = random_ranged_integer(4, 7);
    }
}

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
    const unsigned TILE_PRUNE_RADIUS = 3; /*default prefetch radius*/

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

    return 0.0;
}

void draw_player_spawn(struct player_spawn* spawn) {
    draw_rectangle(spawn->x, spawn->y, 1, 2, COLOR4F_GREEN);
    draw_filled_rectangle(spawn->x+0.5-0.125, spawn->y+2, 0.25, 0.25, COLOR4F_WHITE);
}

/*
  NOTE(jerry):
  Will source from a randomized table. I don't know if I have the time to make decent looking "wind".
  It just need to not look terribly stupid and this'll probably pass the test.
*/
void draw_grass_tiles(struct grass_tile* tiles, size_t count, union color4f color) {
    for (unsigned index = 0; index < count; ++index) {
        struct grass_tile* t = &tiles[index];

        /* ignore direction for now. */
        for (unsigned blade_index = 0; blade_index < GRASS_DENSITY_PER_TILE; ++blade_index) {
            float blade_x = t->x + blade_index * (GRASS_BLADE_WIDTH);
            float blade_y = t->y + 1;

            uint32_t visual_table_index;
            {
                int temporary_bytes[4] = {blade_x, blade_y, blade_index, index};
                visual_table_index = hash_bytes32(temporary_bytes, sizeof(temporary_bytes));
            }
            visual_table_index %= GRASS_VISUAL_INFO_TABLE_SIZE; /* please be power of two */

            float period_amplitude        = grass_visual_period_table[visual_table_index];
            int initial_horizontal_offset = grass_visual_initial_horizontal_offset_table[visual_table_index];
            int blade_height              = -grass_visual_height_table[visual_table_index];

            draw_bresenham_filled_rectangle_line(blade_x, blade_y,
                                                 0, 0,
                                                 initial_horizontal_offset + normalized_sinf(global_elapsed_time * period_amplitude) * 5.0f,
                                                 blade_height, VPIXEL_SZ, color);
        }
    }
}

void draw_tiles(struct tile* tiles, size_t count, union color4f color) {
    for (unsigned index = 0; index < count; ++index) {
        struct tile* t = &tiles[index];

        if (t->id == TILE_NONE)
            continue;

        draw_texture(tile_textures[t->id],
                     t->x, t->y,
                     1, 1,
                     color);
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
        draw_player_spawn((struct player_spawn*) spawn);
    }
}

local bool tile_intersects_rectangle(struct tile* t, float x, float y, float w, float h) {
    if (rectangle_intersects_v(x, y, w, h, t->x, t->y, 1, 1)) {
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
                        const float DISTANCE_EPSILION = 0.2;
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

bool entity_hugging_wall(struct tilemap* tilemap, struct entity* entity) {
    struct tilemap_sample_interval sample_region = tilemap_sampling_region_around_moving_entity(tilemap, entity);

    float entity_center_x = entity->x + entity->w/2;
    float entity_center_y = entity->y + entity->h/2;

    int facing_dir = entity->facing_dir;
    if (facing_dir == 0) {facing_dir = 1;}

    const float RAY_LENGTH = 0.0589;
    /* hahahaha, not really a ray, but whatever */
    const float RAY_HEIGHT = 0.6;

    for (unsigned y = sample_region.min_y; y < sample_region.max_y; ++y) {
        for (unsigned x = sample_region.min_x; x < sample_region.max_x; ++x) {
            struct tile* t = &tilemap->tiles[y * tilemap->width + x];
            if(t->id == TILE_NONE) continue;

            enum intersection_edge closest_edge;

            /* NOTE(jerry): weird quirk. */
            if (facing_dir == 1) {
                closest_edge = rectangle_closest_intersection_edge_v(entity_center_x, entity_center_y - (RAY_HEIGHT/2),
                                                                     entity->w/2 + RAY_LENGTH, RAY_HEIGHT, t->x, t->y, 1, 1);
            } else {
                closest_edge = rectangle_closest_intersection_edge_v(entity_center_x - (entity->w/2 + 0.3), entity_center_y - (RAY_HEIGHT/2),
                                                                     entity->w/2 + RAY_LENGTH, RAY_HEIGHT, t->x, t->y, 1, 1);
            }

            /* NOTE(jerry): technically this I can do it on right/left edges of certain slope types but whatever. */
            if (!tile_is_slope(t)) {
                if (closest_edge == INTERSECTION_EDGE_RIGHT ||
                    closest_edge == INTERSECTION_EDGE_LEFT) {
                    return true;
                }
            }
        }
    }

    return false;
}

void _do_moving_entity_horizontal_collision_response(struct tilemap* tilemap, struct entity* entity, float dt, int substeps) {
    float old_x = entity->x;
    float old_vy = entity->vy;

    entity->last_x = old_x;
    entity->onground = false;

    struct tilemap_sample_interval sample_region = tilemap_sampling_region_around_moving_entity(tilemap, entity);

    bool in_slope = entity_already_in_slope(tilemap, entity);

    for (unsigned substep = 0; substep < substeps; ++substep) {
        float vx_substep = entity->vx / substeps;

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
                        float slope_snapped_location = tile_get_slope_height(t, entity->x, entity->w, entity->h) - PHYSICS_EPSILION;
                        enum intersection_edge closest_edge = rectangle_closest_intersection_edge_v(entity->x, entity->y, entity->w, entity->h, t->x, t->y, 1, 1);

                        if (t->id == TILE_SLOPE_L || t->id == TILE_SLOPE_R) {
                            if (t->id == TILE_SLOPE_L) {
                                switch (closest_edge) {
                                    case INTERSECTION_EDGE_BOTTOM: {
                                        entity->y       = t->y + 1;
                                        entity->last_vy = old_vy;
                                        entity->vy      = 0;
                                        continue;
                                    } break;
                                    case INTERSECTION_EDGE_RIGHT: {
                                        entity->x  = t->x + 1;
                                        entity->vx = 0;
                                        continue;
                                    } break;
                                    default: break;
                                }
                            } else if (t->id == TILE_SLOPE_R) {
                                switch (closest_edge) {
                                    case INTERSECTION_EDGE_BOTTOM: {
                                        entity->y       = t->y + 1;
                                        entity->last_vy = old_vy;
                                        entity->vy      = 0;
                                        continue;
                                    } break;
                                    case INTERSECTION_EDGE_LEFT: {
                                        entity->x  = t->x - entity->w;
                                        entity->vx = 0;
                                        continue;
                                    } break;
                                    default: break;
                                }
                            }

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
                                if (closest_edge == INTERSECTION_EDGE_LEFT) {
                                    entity->x = t->x - entity->w;
                                } else {
                                    entity->x = t->x + 1;
                                }

                                entity->vx = 0;
                            }
                        } else {
                            if (t->id == TILE_SLOPE_BR) {
                                switch (closest_edge) {
                                    case INTERSECTION_EDGE_TOP: {
                                        entity->onground = true;
                                        entity->y        = t->y - entity->h;
                                        entity->last_vy  = old_vy;
                                        entity->vy       = 0;
                                        continue;
                                    } break;
                                    case INTERSECTION_EDGE_LEFT: {
                                        entity->x  = t->x - entity->w;
                                        entity->vx = 0;
                                        continue;
                                    } break;
                                    default: break;
                                }
                            } else if (t->id == TILE_SLOPE_BL) {
                                switch (closest_edge) {
                                    case INTERSECTION_EDGE_TOP: {
                                        entity->onground = true;
                                        entity->y        = t->y - entity->h;
                                        entity->last_vy  = old_vy;
                                        entity->vy       = 0;
                                        continue;
                                    } break;
                                    case INTERSECTION_EDGE_RIGHT: {
                                        entity->x  = t->x + 1;
                                        entity->vx = 0;
                                        continue;
                                    } break;
                                    default: break;
                                }
                            }

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
                                if (closest_edge == INTERSECTION_EDGE_LEFT) {
                                    entity->x = t->x - entity->w;
                                } else {
                                    entity->x = t->x + 1;
                                }

                                entity->vx = 0;
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
    entity->last_y = old_y;
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

    /* 
       handle slope snapping as a separate step. 

       With a weird "swept" check.
     */
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
                            /*
                              NOTE(jerry):
                              This is actually a weird hack, this is just for checking if the tippy top of the slope is where
                              the entity's "foot" is.
                              
                              For whatever reason, it was only this spot that still accelerated due to "gravity". This seems to fix it.
                             */
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

void do_moving_entity_horizontal_collision_response(struct tilemap* tilemap, struct entity* entity, float dt) {
    _do_moving_entity_horizontal_collision_response(tilemap, entity, dt, 16);
}

void do_particle_horizontal_collision_response(struct tilemap* tilemap, struct entity* entity, float dt) {
    _do_moving_entity_horizontal_collision_response(tilemap, entity, dt, 1);
}

void do_particle_vertical_collision_response(struct tilemap* tilemap, struct entity* entity, float dt) {
    do_moving_entity_vertical_collision_response(tilemap, entity, dt);
}
