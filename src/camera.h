#ifndef CAMERA_H
#define CAMERA_H
/*
  This API really sucks. It's not really that hard to use so I guess
  I'll let it pass since it's a really small aomunt of code.
  
  TODO(jerry): (interpolate zoom)
*/
struct camera {
    /*So cameras now dictate the rendering scale. This will probably break lots of shit.*/
    float render_scale; 

    float visual_zoom_level;
    float visual_position_x;
    float visual_position_y;

    float last_position_x;
    float last_position_y;
    float last_zoom_level;

    float target_position_x;
    float target_position_y;
    float target_zoom_level;

#define INTERPOLATION_DIMENSIONS (3)
    float interpolation_time[INTERPOLATION_DIMENSIONS];
    float interpolation_speed[INTERPOLATION_DIMENSIONS];
#undef INTERPOLATION_DIMENSIONS

    float bounds_min_x;
    float bounds_min_y;
    float bounds_max_x;
    float bounds_max_y;

    /*NOTE(jerry): UNUSED*/
    /* me really wants this but this requires access to transformation matrices */
    /* which sdl2's renderer sucks at. so we needs new renderer tomorrow already! */
    float trauma;
#if 0
    float rotation_radians;
#endif
};

/*
  I mean I can still have a camera pool... But whatever.
*/

/*normalized values from 0 - 1.0!*/
void camera_traumatize(struct camera* camera, float amount);
void camera_set_position(struct camera* camera, float x, float y);
void camera_offset_position(struct camera* camera, float x, float y);
void camera_set_focus_speed(struct camera* camera, float speed);
void camera_set_focus_speed_x(struct camera* camera, float speed);
void camera_set_focus_speed_y(struct camera* camera, float speed);
void camera_set_focus_position(struct camera* camera, float x, float y);
void camera_update(struct camera* camera, float dt);
void camera_reset_transform(struct camera* camera);
/*based on the renderer scale / game units*/
void camera_set_bounds(struct camera* camera, float min_x, float min_y, float max_x, float max_y);
void camera_clear_bounds(struct camera* camera);

void transform_point_into_camera_space(struct camera* camera, int* x, int* y);
struct rectangle camera_get_bounds(struct camera* camera);
#endif
