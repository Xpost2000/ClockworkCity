local float _camera_render_scale(struct camera* camera) {
    if (camera->render_scale == 0) {
        camera->render_scale = 1;
    }

    return camera->render_scale;
}

local void _camera_transform_v2(struct camera* camera, float* x, float* y) {
    assert(x && y);
    assert(screen_dimensions[0] > 0 && screen_dimensions[1] > 0);

    float render_scale = _camera_render_scale(camera);

    *x -= (camera->visual_position_x) * render_scale;
    *x += (screen_dimensions[0] / 2.0f);
    *y -= (camera->visual_position_y) * render_scale;
    *y += (screen_dimensions[1] / 2.0f);
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

    zero_array(camera->interpolation_time);
}

void camera_set_focus_speed_x(struct camera* camera, float speed) {
    camera->interpolation_speed[0] = speed;
}

void camera_set_focus_speed_y(struct camera* camera, float speed) {
    camera->interpolation_speed[1] = speed;
}

void camera_set_focus_speed(struct camera* camera, float speed) {
    for (int index = 0; index < array_count(camera->interpolation_time); ++index) {
        camera->interpolation_speed[index] = speed;
    }
}

void camera_traumatize(struct camera* camera, float amount) {
    camera->trauma += amount;
}

void camera_update(struct camera* camera, float dt) {
    camera->trauma = clampf(camera->trauma, 0.0, 1.0);

    /*
      Because of the frequency of set_focus_position... This is actually not linear but
      quadratic. So this is technically the *wrong* way to use lerp.

      But whatever.
     */

    /*hardcoded for now*/
    float trauma_shake_x = (8 * random_float() - 4.5) * camera->trauma;
    float trauma_shake_y = (8 * random_float() - 4.5) * camera->trauma;

    /* think of some better metrics on small levels */
    {
        struct rectangle screen_bounds = camera_get_bounds(camera);
        float area = ((camera->bounds_max_x - camera->bounds_min_x) * (camera->bounds_max_y - camera->bounds_min_y));

        if (area > 0) {
            if ((screen_bounds.x + screen_bounds.w) > (camera->bounds_max_x)) {
                camera->target_position_x = camera->bounds_max_x - (screen_bounds.w/2);
                camera->interpolation_time[0] = 0;
            }
            if ((screen_bounds.x) < (camera->bounds_min_x)) {
                camera->target_position_x = camera->bounds_min_x + (screen_bounds.w/2);
                camera->interpolation_time[0] = 0;
            }

            if ((screen_bounds.y + screen_bounds.h) > (camera->bounds_max_y)) {
                camera->target_position_y = camera->bounds_max_y - (screen_bounds.h/2);
                camera->interpolation_time[1] = 0;
            }
        
            if ((screen_bounds.y) < (camera->bounds_min_y)) {
                camera->target_position_y = camera->bounds_min_y + (screen_bounds.h/2);
                camera->interpolation_time[1] = 0;
            }
        }
    }

    for (int index = 0; index < array_count(camera->interpolation_time); ++index) {
        if (camera->interpolation_time[index] < 1.0) {
            camera->interpolation_time[index] += dt * camera->interpolation_speed[index];
        }
    }

    camera->visual_position_x =
        lerp(camera->last_position_x, camera->target_position_x,
             clampf(camera->interpolation_time[0], 0, 1)) + trauma_shake_x;
    camera->visual_position_y =
        lerp(camera->last_position_y, camera->target_position_y,
             clampf(camera->interpolation_time[1], 0, 1)) + trauma_shake_y;

    camera->trauma -= dt * 0.078;
}

void camera_reset_transform(struct camera* camera) {
    (*camera) = (struct camera) {
        .interpolation_speed[0] = 1.0, .interpolation_speed[1] = 1.0
    };
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
