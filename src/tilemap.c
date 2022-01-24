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
    /*tiles will store relative positions (in the case of moving tile islands)*/
    /*this is redundant, but this'll allow me to use a different storage method if I want.*/
    /*also simplifies some other code later, as I don't need to derive information like the tile rectangle*/
    uint16_t x;
    uint16_t y;
    uint32_t id;
};

bool tile_is_slope(struct tile* t) {
    return ((t->id == TILE_SLOPE_BL) ||
            (t->id == TILE_SLOPE_BR) ||
            (t->id == TILE_SLOPE_L)  ||
            (t->id == TILE_SLOPE_R));
}

float tile_get_slope_height(struct tile* t, float x, float w, float h) {
    float tile_x = t->x * TILE_TEX_SIZE;
    float tile_y = t->y * TILE_TEX_SIZE;
    float tile_w = TILE_TEX_SIZE;
    float tile_h = TILE_TEX_SIZE;

    if (t->id == TILE_SLOPE_L) {
        float slope_x_offset = clampf(((x + w) - tile_x), 0, tile_w);
        return ((tile_y + tile_h) - slope_x_offset - h);
    } else {
        float slope_x_offset = clampf((x - tile_x), 0, tile_w);
        return ((tile_y + slope_x_offset) - h);
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

void DEBUG_draw_tilemap(struct tilemap* tilemap) {
    for(unsigned y = 0; y < tilemap->height; ++y) {
        for(unsigned x = 0; x < tilemap->width; ++x) {
            struct tile* t = &tilemap->tiles[y * tilemap->width + x];
            draw_texture(tile_textures[t->id], x * TILE_TEX_SIZE, y * TILE_TEX_SIZE, TILE_TEX_SIZE, TILE_TEX_SIZE, COLOR4F_WHITE);
        }
    }
}

struct tilemap global_test_tilemap = {};

local bool do_collision_response_tile_left_edge(struct tile* t, struct entity* ent) {
    assert(t->id != TILE_NONE && "uh, air tile tries collision?");

    float entity_right_edge = ent->x + ent->w;
    float tile_right_edge = (t->x + 1) * TILE_TEX_SIZE;

    if (entity_right_edge > t->x && entity_right_edge < tile_right_edge) {
        ent->x = t->x*TILE_TEX_SIZE - ent->w;
        return true;
    }

    return false;
}

local bool do_collision_response_tile_right_edge(struct tile* t, struct entity* ent) {
    assert(t->id != TILE_NONE && "uh, air tile tries collision?");

    float tile_right_edge = (t->x + 1) * TILE_TEX_SIZE;
    if (roundf(ent->x) < tile_right_edge && roundf(ent->x) > t->x) {
        ent->x = tile_right_edge;
        return true;
    }

    return false;
}

local bool do_collision_response_tile_top_edge(struct tile* t, struct entity* ent) {
    assert(t->id != TILE_NONE && "uh, air tile tries collision?");

    float entity_bottom_edge = roundf(ent->y) + ent->h;
    float tile_bottom_edge = (t->y + 1) * TILE_TEX_SIZE;

    if (entity_bottom_edge > t->y && entity_bottom_edge < tile_bottom_edge) {
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

/* maybe move to entity.c? */
void do_moving_entity_horizontal_collision_response(struct tilemap* tilemap, struct entity* entity, float dt) {
    float old_x = entity->x;
    entity->x += entity->vx * dt;

    for(unsigned y = 0; y < tilemap->height; ++y) {
        for(unsigned x = 0; x < tilemap->width; ++x) {
            struct tile* t = &tilemap->tiles[y * tilemap->width + x];

            float tile_x = x * TILE_TEX_SIZE;
            float tile_y = y * TILE_TEX_SIZE;
            float tile_w = TILE_TEX_SIZE;
            float tile_h = TILE_TEX_SIZE;

            if(t->id == TILE_NONE) continue;

            if (rectangle_intersects_v(entity->x, entity->y, entity->w, entity->h,
                                       tile_x, tile_y, tile_w, tile_h)) {
                if (tile_is_slope(t)) {
                    switch (t->id) {
                        case TILE_SLOPE_L: {
                            if (old_x >= tile_x + tile_w) {
                                entity->x = tile_x + tile_w;
                            } else {
                                float slope_x_offset = clampf(((entity->x + entity->w) - tile_x), 0, tile_w);
                                float slope_snapped_location = roundf((tile_y + tile_h) - slope_x_offset) - entity->h;

                                float delta_from_foot_to_tile_top = (slope_snapped_location - entity->y);

                                if (entity->y + entity->h <= tile_y + tile_h) {
                                    if (entity->vy >= 0 && (entity->y >= slope_snapped_location || delta_from_foot_to_tile_top <= ((float)TILE_TEX_SIZE/2.25))) {
                                        entity->y = slope_snapped_location;
                                        entity->vy = 0;
                                    }
                                } else {
                                    if (old_x + entity->w <= tile_x) {
                                        entity->x = tile_x - entity->w;
                                    } else {
                                        float delta_from_foot_to_tile_bottom = (entity->y - (tile_y + tile_h));

                                        if (fabs(delta_from_foot_to_tile_bottom) < fabs(delta_from_foot_to_tile_top)) {
                                            entity->y = tile_y + tile_h;
                                        } else {
                                            entity->y = slope_snapped_location;
                                        }

                                        entity->vy = 0;
                                    }
                                }
                            }
                        } break;
                        case TILE_SLOPE_R: {
                            if (old_x + entity->w <= tile_x) {
                                entity->x = tile_x - entity->w;
                            } else {
                                float slope_x_offset = clampf((entity->x - tile_x), 0, tile_w);
                                float slope_snapped_location = roundf(tile_y + slope_x_offset) - entity->h;

                                float delta_from_foot_to_tile_top = (entity->y - slope_snapped_location);

                                if (entity->y + entity->h <= tile_y + tile_h) {
                                    if (entity->vy >= 0 && (entity->y >= slope_snapped_location || delta_from_foot_to_tile_top <= ((float)TILE_TEX_SIZE/2.25))) {
                                        entity->y = slope_snapped_location;
                                        entity->vy = 0;
                                    }
                                } else {
                                    if (old_x >= tile_x + tile_w) {
                                        entity->x = tile_x + tile_w;
                                    } else {
                                        float delta_from_foot_to_tile_bottom = (entity->y - (tile_y + tile_h));

                                        if (fabs(delta_from_foot_to_tile_bottom) < fabs(delta_from_foot_to_tile_top)) {
                                            entity->y = tile_y + tile_h;
                                        } else {
                                            entity->y = slope_snapped_location;
                                        }

                                        entity->vy = 0;
                                    }
                                }
                            }
                        } break;
                        case TILE_SLOPE_BL: {
                            if (old_x >= tile_x + tile_w) {
                                entity->x = tile_x + tile_w;
                            } else {
                                float slope_x_offset = clampf(((entity->x + entity->w) - tile_x), 0, tile_w);
                                float slope_snapped_location = (tile_y + slope_x_offset);

                                if (entity->y > tile_y) {
                                    if (entity->y < slope_snapped_location) {
                                        entity->y = slope_snapped_location;
                                        if (entity->vy < 0)
                                            entity->vy = 0;
                                    }
                                    /* 
                                       leads to a similar issue which hasn't been fixed yet
                                       which is what happens when a slope goes into an obstacle?
                                    */
                                } else {
                                    if (old_x + entity->w <= tile_x) {
                                        entity->x = tile_x - entity->w;
                                        /* 
                                           leads to a similar issue which hasn't been fixed yet
                                           which is what happens when a slope goes into an obstacle?
                                        */
                                    } else {
                                        entity->y = tile_y - entity->h;
                                        /* 
                                           leads to a similar issue which hasn't been fixed yet
                                           which is what happens when a slope goes into an obstacle?
                                        */
                                    }
                                }
                            }
                        } break;
                        case TILE_SLOPE_BR: {
                            if (old_x + entity->w <= tile_x) {
                                entity->x = tile_x - entity->w;
                            } else {
                                if (entity->y > tile_y) {
                                    float slope_x_offset = (tile_x - entity->x);
                                    float slope_snapped_location = ((tile_y + tile_h) + slope_x_offset);
                                    float delta_from_foot_to_tile_top = (slope_snapped_location - entity->y);

                                    if (entity->y <= slope_snapped_location) {
                                        if (entity->vy < 0) {
                                            entity->vy = 0;
                                        }
                                        entity->y = slope_snapped_location;
                                    }
                                    /* 
                                       leads to a similar issue which hasn't been fixed yet
                                       which is what happens when a slope goes into an obstacle?
                                    */
                                } else {
                                    if (old_x >= tile_x + tile_w) {
                                        entity->x = tile_x + tile_w;
                                        /* 
                                           leads to a similar issue which hasn't been fixed yet
                                           which is what happens when a slope goes into an obstacle?
                                        */
                                    } else {
                                        entity->y = tile_y - entity->h;
                                        /* 
                                           leads to a similar issue which hasn't been fixed yet
                                           which is what happens when a slope goes into an obstacle?
                                        */
                                    }
                                }
                            }
                        } break;
                    }
                } else {
                    if (!do_collision_response_tile_left_edge(t, entity)) {
                        do_collision_response_tile_right_edge(t, entity);
                    }
                }

                entity->vx = 0;
            }
        }
    }
}

void do_moving_entity_vertical_collision_response(struct tilemap* tilemap, struct entity* entity, float dt) {
    float old_y = entity->y;
    entity->y += entity->vy * dt;

    for(unsigned y = 0; y < tilemap->height; ++y) {
        for(unsigned x = 0; x < tilemap->width; ++x) {
            struct tile* t = &tilemap->tiles[y * tilemap->width + x];

            float tile_x = x * TILE_TEX_SIZE;
            float tile_y = y * TILE_TEX_SIZE;
            float tile_w = TILE_TEX_SIZE;
            float tile_h = TILE_TEX_SIZE;

            if (rectangle_intersects_v(entity->x, entity->y, entity->w, entity->h, tile_x, tile_y, tile_w, tile_h)) {
                switch (t->id) {
                    case TILE_SOLID: {
                        do_collision_response_tile_top_edge(t, entity);
                        do_collision_response_tile_bottom_edge(t, entity);

                        entity->vy = 0;
                        return;
                    } break;
                    case TILE_SLOPE_R:
                    case TILE_SLOPE_L: {
                        if (entity->vy >= 0 && roundf(old_y) == (tile_get_slope_height(t, entity->x, entity->w, entity->h))) {
                            /*
                              somewhere along the line, something fucked up and on the tippy top
                              of a slope, I somehow avoid setting entity->vy to 0? Or some other
                              number magic happened. I actually really don't know why I need this.
                              
                              I probably won't know until like I forget this codebase and reread it!
                             */
                            entity->y = roundf(entity->y);
                            entity->vy = 0;
                            return;
                        }
                    } break;
                    case TILE_SLOPE_BL:
                    case TILE_SLOPE_BR: {
                        /* duplicated but okay. */
                        if (entity->y + entity->h <= tile_y) {
                            entity->y = roundf(entity->y);
                            entity->vy = 0;
                            return;
                        }
                    } break;
                }

            }
        }
    }
}

void evaluate_moving_entity_grounded_status(struct tilemap* tilemap, struct entity* entity, float dt) {
    float old_y = entity->y;
    {
        bool was_on_ground = entity->onground;
        entity->onground = false;

        for(unsigned y = 0; y < tilemap->height; ++y) {
            for(unsigned x = 0; x < tilemap->width; ++x) {
                struct tile* t = &tilemap->tiles[y * tilemap->width + x];

                float tile_x = x * TILE_TEX_SIZE;
                float tile_y = y * TILE_TEX_SIZE;
                float tile_w = TILE_TEX_SIZE;
                float tile_h = TILE_TEX_SIZE;

                if(t->id == TILE_NONE) continue;
                if (!rectangle_overlapping_v(entity->x, entity->y, entity->w, entity->h, tile_x, tile_y, tile_w, tile_h)) continue;

                /*NOTE(jerry): this is slightly... different for sloped tiles.*/

                if ((old_y + entity->h <= tile_y)) {
                    entity->onground = true;
                }

                if (tile_is_slope(t)) {
                    float slope_location = tile_get_slope_height(t, entity->x, entity->w, entity->h);

                    if (roundf(entity->y) == roundf(slope_location)) {
                        entity->onground = true;
                    } else {
                        entity->onground = false;
                    }
                }

                if (entity->onground) {
                    return;
                }
            }
        }
    }
}
