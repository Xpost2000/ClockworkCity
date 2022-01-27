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

    *x -= global_camera.visual_position_x;
    *x += screen_dimensions[0] / 2.0f;
    *y -= global_camera.visual_position_y;
    *y += screen_dimensions[1] / 2.0f;

    *x = floorf(*x);
    *y = floorf(*y);
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
    float trauma_shake_x = (300 * random_float() - 150) * global_camera.trauma;
    float trauma_shake_y = (300 * random_float() - 150) * global_camera.trauma;

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

void transform_point_into_camera_space(int* x, int* y) {
    if (!active_camera) return;

    if (x) {
        *x += active_camera->visual_position_x - screen_dimensions[0]/2;
    }

    if (y) {
        *y += active_camera->visual_position_y - screen_dimensions[1]/2;
    }
}
