#define FORFRIENDS_DEMO

local int __active_players = 1;
local int p_idx = 0;
void _p_idx(int n) {p_idx = n;}
void _report_active_players(int n) {
    __active_players = n;
}

local float _camera_render_scale(struct camera* camera) {
    if (camera->render_scale == 0) {
        camera->render_scale = 1;
    }

    if (camera->visual_zoom_level == 0.0) {
        camera->target_zoom_level = camera->last_zoom_level = camera->visual_zoom_level = 1;
    }

#ifdef FORFRIENDS_DEMO
    return camera->render_scale * camera->visual_zoom_level;
#endif
}


local void _camera_transform_v2(struct camera* camera, float* x, float* y) {
    assert(x && y);
    assert(screen_dimensions[0] > 0 && screen_dimensions[1] > 0);

    float render_scale = _camera_render_scale(camera);

#ifdef FORFRIENDS_DEMO
    int old_screen_dimensions_w = screen_dimensions[0];
    int old_screen_dimensions_h = screen_dimensions[1];

    int active_players = __active_players;
    int split_screen_width = screen_dimensions[0] / active_players;
#endif
    if (active_players > 1) {
        *x -= (camera->visual_position_x) * render_scale;
        *x += (screen_dimensions[0]/2.0);
        if (p_idx == 0)
        *x -= (split_screen_width/2.0);
        else
        *x += (split_screen_width/2.0);
        *y -= (camera->visual_position_y) * render_scale;
        *y += (screen_dimensions[1] / 2.0f);
    } else {
        *x -= (camera->visual_position_x) * render_scale;
        *x += (screen_dimensions[0] / 2.0f);
        *y -= (camera->visual_position_y) * render_scale;
        *y += (screen_dimensions[1] / 2.0f);
    }
}

void camera_offset_position(struct camera* camera, float x, float y) {
    float new_x = camera->visual_position_x + x;
    float new_y = camera->visual_position_y + y;
    camera_set_position(camera, new_x, new_y);
}

void camera_set_position(struct camera* camera, float x, float y) {
    camera->visual_position_x = camera->target_position_x = camera->last_position_x = x;
    camera->visual_position_y = camera->target_position_y = camera->last_position_y = y;
}

void camera_set_focus_position(struct camera* camera, float x, float y) {
    camera->last_position_x = camera->visual_position_x;
    camera->target_position_x = x;

    camera->last_position_y = camera->visual_position_y;
    camera->target_position_y = y;

    camera->interpolation_time[0] = camera->interpolation_time[1] = 0;
}

void camera_set_active_focus_zone(struct camera* camera, struct camera_focus_zone* zone) {
    if (zone != camera->active_focus_zone) {
        camera->active_focus_zone = zone;

        if (zone) {
            camera->original_camera_state = (struct camera_state) {
                .target_position_x      = camera->target_position_x,
                .target_position_y      = camera->target_position_y,
                .target_zoom_level      = (camera->target_zoom_level == 0) ? 1 : camera->target_zoom_level,
                .interpolation_speed[0] = camera->interpolation_speed[0],
                .interpolation_speed[1] = camera->interpolation_speed[1],
                .interpolation_speed[2] = (camera->interpolation_speed[2] == 0) ? 1 : camera->interpolation_speed[2],
            };

            camera_set_focus_speed_x(camera, zone->interpolation_speed[0]);
            camera_set_focus_speed_y(camera, zone->interpolation_speed[1]);
            camera_set_focus_speed_zoom(camera, zone->interpolation_speed[2]);
            camera_set_focus_zoom_level(camera, zone->zoom);
        } else {
            camera_set_focus_speed_x(camera, camera->original_camera_state.interpolation_speed[0]);
            camera_set_focus_speed_y(camera, camera->original_camera_state.interpolation_speed[1]);
            camera_set_focus_speed_zoom(camera, camera->original_camera_state.interpolation_speed[2]);
            camera_set_focus_zoom_level(camera, camera->original_camera_state.target_zoom_level);
        }
    }
}

void camera_set_zoom(struct camera* camera, float level) {
    camera->visual_zoom_level = camera->target_zoom_level = camera->last_zoom_level = level;
    camera->interpolation_time[2] = 1.0;
}

void camera_set_focus_zoom_level(struct camera* camera, float level) {
    camera->last_zoom_level = camera->visual_zoom_level;
    camera->target_zoom_level = level;

    camera->interpolation_time[2] = 0.0;
}

void camera_set_focus_speed_x(struct camera* camera, float speed) {
    camera->interpolation_speed[0] = speed;
}

void camera_set_focus_speed_y(struct camera* camera, float speed) {
    camera->interpolation_speed[1] = speed;
}

void camera_set_focus_speed_zoom(struct camera* camera, float speed) {
    camera->interpolation_speed[2] = speed;
}

void camera_set_focus_speed(struct camera* camera, float speed) {
    for (int index = 0; index < array_count(camera->interpolation_time); ++index) {
        camera->interpolation_speed[index] = speed;
    }
}

void camera_traumatize(struct camera* camera, float amount) {
    camera->trauma += amount;
    /* hack for controller rumble :) */
    notify_camera_traumatize(camera, amount);
}

void camera_force_clamp_to_bounds(struct camera* camera) {
    struct rectangle screen_bounds = camera_get_bounds(camera);
    float area = ((camera->bounds_max_x - camera->bounds_min_x) * (camera->bounds_max_y - camera->bounds_min_y));

    if (area > 0) {
        if ((screen_bounds.x + screen_bounds.w) > (camera->bounds_max_x)) {
            camera->visual_position_x = camera->target_position_x = camera->bounds_max_x - (screen_bounds.w/2);
        }
        if ((screen_bounds.x) < (camera->bounds_min_x)) {
            camera->visual_position_x = camera->target_position_x = camera->bounds_min_x + (screen_bounds.w/2);
        }

        if ((screen_bounds.y + screen_bounds.h) > (camera->bounds_max_y)) {
            camera->visual_position_y = camera->target_position_y = camera->bounds_max_y - (screen_bounds.h/2);
        }
        
        if ((screen_bounds.y) < (camera->bounds_min_y)) {
            camera->visual_position_y = camera->target_position_y = camera->bounds_min_y + (screen_bounds.h/2);
        }
    }
}

local void camera_constrict_view_to_rectangle(struct camera* camera, float min_x, float max_x, float min_y, float max_y) {
    struct rectangle screen_bounds = camera_get_bounds(camera);

    if ((screen_bounds.x + screen_bounds.w) > (max_x)) {
        camera->target_position_x = max_x - (screen_bounds.w/2);
        camera->interpolation_time[0] = 0;
    }

    if ((screen_bounds.x) < (min_x)) {
        camera->target_position_x = min_x + (screen_bounds.w/2);
        camera->interpolation_time[0] = 0;
    }

    if ((screen_bounds.y + screen_bounds.h) > (max_y)) {
        camera->target_position_y = max_y - (screen_bounds.h/2);
        camera->interpolation_time[1] = 0;
    }
        
    if ((screen_bounds.y) < (min_y)) {
        camera->target_position_y = min_y + (screen_bounds.h/2);
        camera->interpolation_time[1] = 0;
    }
}

void camera_update(struct camera* camera, struct camera_focus_zone* focus_zones, size_t focus_zone_count, float dt) {
    camera->trauma = clampf(camera->trauma, 0.0, 1.0);

    /*
      Because of the frequency of set_focus_position... This is actually not linear but
      quadratic. So this is technically the *wrong* way to use lerp.

      But whatever.
     */

    /*hardcoded for now*/
    const float CAMERA_SHAKE_RADIUS_X = 3;
    const float CAMERA_SHAKE_RADIUS_Y = 3;
    float trauma_shake_x = random_ranged_float(-CAMERA_SHAKE_RADIUS_X, CAMERA_SHAKE_RADIUS_X) * camera->trauma;
    float trauma_shake_y = random_ranged_float(-CAMERA_SHAKE_RADIUS_Y, CAMERA_SHAKE_RADIUS_Y) * camera->trauma;

    /* think of some better metrics on small levels */
    {
        struct rectangle screen_bounds = camera_get_bounds(camera);
        float area = ((camera->bounds_max_x - camera->bounds_min_x) * (camera->bounds_max_y - camera->bounds_min_y));

        struct camera_focus_zone* focus_zone = camera->active_focus_zone;
        /* focus zones provide area to the camera */
        if (area > 0 || focus_zone) {
            if (focus_zone) {
                if (rectangle_overlapping_v(screen_bounds.x, screen_bounds.y,
                                            screen_bounds.w, screen_bounds.h,
                                            (float)focus_zone->x, (float)focus_zone->y,
                                            (float)focus_zone->w, (float)focus_zone->h)) {
                    camera_constrict_view_to_rectangle(camera,
                                                       (float)focus_zone->x, (float)(focus_zone->x + focus_zone->w),
                                                       (float)focus_zone->y, (float)(focus_zone->y + focus_zone->h));
                }
            } else {
                camera_constrict_view_to_rectangle(camera, camera->bounds_min_x, camera->bounds_max_x, camera->bounds_min_y, camera->bounds_max_y);
            }
        }
    }

    if (!camera->paused) {
        for (int index = 0; index < array_count(camera->interpolation_time); ++index) {
            if (camera->interpolation_time[index] < 1.0) {
                camera->interpolation_time[index] += dt * camera->interpolation_speed[index];
            }
        }
    }

    camera->visual_position_x = lerp(camera->last_position_x, camera->target_position_x, interpolation_clamp(camera->interpolation_time[0])) + trauma_shake_x;
    camera->visual_position_y = lerp(camera->last_position_y, camera->target_position_y, interpolation_clamp(camera->interpolation_time[1])) + trauma_shake_y;
    /* camera->visual_zoom_level = lerp(camera->last_zoom_level, camera->target_zoom_level, interpolation_clamp(camera->interpolation_time[2])); */
    camera->visual_zoom_level = cubic_ease_out(camera->last_zoom_level, camera->target_zoom_level - camera->last_zoom_level, 1, interpolation_clamp(camera->interpolation_time[2]));

    camera->trauma -= dt * 0.098;
}

void camera_reset_transform(struct camera* camera) {
    float old_render_scale = camera->render_scale;
    (*camera) = (struct camera) {
        .interpolation_speed[0] = 1.0, .interpolation_speed[1] = 1.0, .interpolation_speed[2] = 1.0,
        .render_scale = old_render_scale
    };
}

void camera_stop_tracking(struct camera* camera) {
    camera->paused = true;
}

void camera_resume_tracking(struct camera* camera) {
    camera->paused = false;
}

void camera_set_bounds(struct camera* camera, float min_x, float min_y, float max_x, float max_y) {
    camera->bounds_min_x = min_x;
    camera->bounds_min_y = min_y;
    camera->bounds_max_x = max_x;
    camera->bounds_max_y = max_y;
}

void camera_clear_bounds(struct camera* camera) {
    camera_set_bounds(camera, 0, 0, 0, 0);
}

/* as the full screen. Does not get actual clamped bounds, since I don't really need them? */
struct rectangle camera_get_bounds(struct camera* camera) {
    float render_scale = _camera_render_scale(camera);

    int dimens[2];
    get_screen_dimensions(dimens, dimens+1);

    dimens[0] /= render_scale;
    dimens[1] /= render_scale;

    float camera_half_width  = (dimens[0]/2);
    float camera_half_height = (dimens[1]/2);

    float camera_center_x = camera->target_position_x;
    float camera_center_y = camera->target_position_y;

    return rectangle_centered(camera_center_x, camera_center_y, camera_half_width, camera_half_height);
}

void transform_point_into_camera_space(struct camera* camera, int* x, int* y) {
    float render_scale = _camera_render_scale(camera);

    if (x) {
        float x_transform = *x;
        x_transform += camera->visual_position_x * render_scale - screen_dimensions[0]/2;
        x_transform /= render_scale;

        *x = floorf(x_transform);
    }

    if (y) {
        float y_transform = *y;
        y_transform += camera->visual_position_y * render_scale - screen_dimensions[1]/2;
        y_transform /= render_scale;

        *y = floorf(y_transform);
    }
}
