#ifndef CAMERA_H
#define CAMERA_H
/*
  TODO(jerry): zoom
*/
struct camera {
    float visual_zoom_level;
    float visual_position_x;
    float visual_position_y;

    float last_position_x;
    float last_position_y;
    float last_zoom_level;

    float target_position_x;
    float target_position_y;
    float target_zoom_level;

    float interpolation_time[3];
    float interpolation_speed[3];

    /*NOTE(jerry): UNUSED*/
    /* me really wants this but this requires access to transformation matrices */
    /* which sdl2's renderer sucks at. so we needs new renderer tomorrow already! */
    float trauma;
#if 0
    float rotation_radians;
#endif
};

struct camera* get_global_camera(void);
void set_active_camera(struct camera* camera); /*really only here to toggle camera usage*/

/*normalized values from 0 - 1.0!*/
void camera_traumatize(float amount);
void camera_set_position(float x, float y);
void camera_set_focus_speed(float speed);
void camera_set_focus_speed_x(float speed);
void camera_set_focus_speed_y(float speed);
void camera_set_focus_position(float x, float y);
void camera_update(float dt);
void camera_reset_transform(void);

void transform_point_into_camera_space(int* x, int* y);
#endif
