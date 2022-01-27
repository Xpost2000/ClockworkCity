#ifndef CAMERA_H
#define CAMERA_H
/*
  one global active camera object. Is the idea anyways,
  but I expose the structure here incase I change my mind.
  
  The way this camera is setup is that it can follow things, but I've never
  tried this with room based things.
  
  I'll have to see. Or I can just have one camera always? probably looks bad though
*/
struct camera {
    float visual_position_x;
    float visual_position_y;

    float last_position_x;
    float last_position_y;

    float target_position_x;
    float target_position_y;

    float interpolation_time[2];
    float interpolation_speed[2];

    /*NOTE(jerry): UNUSED*/
    /* me really wants this but this requires access to transformation matrices */
    /* which sdl2's renderer sucks at. so we needs new renderer tomorrow already! */
    float trauma;
#if 0
    float zoom_level;
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
