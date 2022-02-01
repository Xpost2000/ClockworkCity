local struct camera global_camera = {};
local struct camera* active_camera = NULL;
local struct camera dummy_camera = {};

struct camera* get_global_camera(void) {
    return &global_camera;
}

void set_active_camera(struct camera* camera) {
    if (camera == NULL) {
        active_camera = &dummy_camera;
    } else {
        active_camera = camera;
    }
}

local float _camera_lerp(float a, float b, float t) {
    return (1.0 - t) * a + (b * t);
}

local void _camera_transform_v2(float* x, float* y) {
    if (active_camera == &dummy_camera)
        return;

    assert(x && y);
    assert(screen_dimensions[0] > 0 && screen_dimensions[1] > 0);

    *x -= (global_camera.visual_position_x) * DEBUG_scale;
    *x += (screen_dimensions[0] / 2.0f);
    *y -= (global_camera.visual_position_y) * DEBUG_scale;
    *y += (screen_dimensions[1] / 2.0f);
}

void camera_set_position(float x, float y) {
    active_camera->visual_position_x = active_camera->target_position_x = active_camera->last_position_x = x;
    active_camera->visual_position_y = active_camera->target_position_y = active_camera->last_position_y = y;
}

void camera_set_focus_position(float x, float y) {
    active_camera->last_position_x = active_camera->visual_position_x;
    active_camera->last_position_y = active_camera->visual_position_y;

    active_camera->target_position_x = x;
    active_camera->target_position_y = y;

    active_camera->interpolation_time[0] = active_camera->interpolation_time[1] = 0.0;
}

void camera_set_focus_speed_x(float speed) {
    active_camera->interpolation_speed[0] = speed;
}

void camera_set_focus_speed_y(float speed) {
    active_camera->interpolation_speed[1] = speed;
}

void camera_set_focus_speed(float speed) {
    active_camera->interpolation_speed[0] = active_camera->interpolation_speed[1] = speed;
}

void camera_traumatize(float amount) {
    global_camera.trauma += amount;
}

void camera_update(float dt) {
    global_camera.trauma = clampf(global_camera.trauma, 0.0, 1.0);

    /*
      this doesn't touch active camera. All cameras should be free to update
      on their own. It really shouldn't matter, and it probably looks more consistent
      this way.
     */
    for (int index = 0; index < 2; ++index) {
        if (global_camera.interpolation_time[index] < 1.0) {
            global_camera.interpolation_time[index] += dt * global_camera.interpolation_speed[index];
        }
    }

    /*
      Because of the frequency of set_focus_position... This is actually not linear but
      quadratic. So this is technically the *wrong* way to use lerp.

      But whatever.
     */

    /*hardcoded for now*/
    float trauma_shake_x = (8 * random_float() - 4.5) * global_camera.trauma;
    float trauma_shake_y = (8 * random_float() - 4.5) * global_camera.trauma;

    {
        struct rectangle screen_bounds = camera_get_bounds(&global_camera);

        if ((screen_bounds.x + screen_bounds.w) > (global_camera.bounds_max_x)) {
            global_camera.target_position_x = global_camera.bounds_max_x - (screen_bounds.w/2);
        }

        if ((screen_bounds.x) < (global_camera.bounds_min_x)) {
            global_camera.target_position_x = global_camera.bounds_min_x + (screen_bounds.w/2);
        }

        if ((screen_bounds.y + screen_bounds.h) > (global_camera.bounds_max_y)) {
            global_camera.target_position_y = global_camera.bounds_max_y - (screen_bounds.h/2);
        }

        if ((screen_bounds.y) < (global_camera.bounds_min_y)) {
            global_camera.target_position_y = global_camera.bounds_min_y + (screen_bounds.h/2);
        }
    }

    global_camera.visual_position_x =
        _camera_lerp(global_camera.last_position_x, global_camera.target_position_x,
                     global_camera.interpolation_time[0]) + trauma_shake_x;
    global_camera.visual_position_y =
        _camera_lerp(global_camera.last_position_y, global_camera.target_position_y,
                     global_camera.interpolation_time[1]) + trauma_shake_y;

    global_camera.trauma -= dt * 0.078;
}

void camera_reset_transform(void) {
    global_camera = (struct camera) {
        .interpolation_speed[0] = 1.0, .interpolation_speed[1] = 1.0
    };
}

void camera_set_bounds(float min_x, float min_y, float max_x, float max_y) {
    /*lol global_camera*/
    global_camera.bounds_min_x = min_x;
    global_camera.bounds_min_y = min_y;
    global_camera.bounds_max_x = max_x;
    global_camera.bounds_max_y = max_y;
}

void camera_clear_bounds(void) {
    camera_set_bounds(0, 0, 0, 0);
}

/* as the full screen. Does not get actual clamped bounds, since I don't really need them? */
struct rectangle camera_get_bounds(struct camera* camera) {
    /* float render_scale = DEBUG_scale; */
    /* hard code until I figure out how to restructure this. Just get it to work. */
    float render_scale = ratio_with_screen_width(48);

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

void transform_point_into_camera_space(int* x, int* y) {
    if (!active_camera) return;

    if (x) {
        float x_transform = *x;
        x_transform += active_camera->visual_position_x*DEBUG_scale - screen_dimensions[0]/2;
        x_transform /= DEBUG_scale;

        *x = floorf(x_transform);
    }

    if (y) {
        float y_transform = *y;
        y_transform += active_camera->visual_position_y*DEBUG_scale - screen_dimensions[1]/2;
        y_transform /= DEBUG_scale;

        *y = floorf(y_transform);
    }
}
