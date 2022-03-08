#ifndef CAMERA_H
#define CAMERA_H
/*
  This API really sucks. It's not really that hard to use so I guess
  I'll let it pass since it's a really small aomunt of code.
  
  TODO(jerry): (interpolate zoom)
*/
#define CAMERA_FOCUS_ZONE_IDENTIFIER_LENGTH (16)
struct camera_focus_zone { /* technically this is from the tilemap file, but I'm not making a _def.c / .h file. */
    /* does not need identifier? */
    char identifier[CAMERA_FOCUS_ZONE_IDENTIFIER_LENGTH];
    uint8_t active;

    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;

    float zoom;
    float interpolation_speed[3];
};

#define INTERPOLATION_DIMENSIONS (3)
struct camera_state {
    float target_position_x;
    float target_position_y;
    float target_zoom_level;
    float interpolation_speed[INTERPOLATION_DIMENSIONS];
};

struct camera {
    struct camera_focus_zone* active_focus_zone;
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

    struct camera_state original_camera_state;

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

    bool paused;
};

/*
  I mean I can still have a camera pool... But whatever.
*/

/*normalized values from 0 - 1.0!*/
void camera_traumatize(struct camera* camera, float amount);
void notify_camera_traumatize(struct camera* camera, float amount);
void camera_set_position(struct camera* camera, float x, float y);
void camera_set_zoom(struct camera* camera, float level);
void camera_offset_position(struct camera* camera, float x, float y);
void camera_set_focus_speed(struct camera* camera, float speed);
void camera_set_focus_speed_x(struct camera* camera, float speed);
void camera_set_focus_speed_y(struct camera* camera, float speed);
void camera_set_focus_speed_zoom(struct camera* camera, float speed);
void camera_set_focus_position(struct camera* camera, float x, float y);
void camera_set_focus_zoom_level(struct camera* camera, float level);
/* TODO(jerry): remove unused arguments. */
void camera_update(struct camera* camera, struct camera_focus_zone* focus_zones, size_t focus_zone_count, float dt);
void camera_force_clamp_to_bounds(struct camera* camera);
void camera_stop_tracking(struct camera* camera);
void camera_resume_tracking(struct camera* camera);
void camera_reset_transform(struct camera* camera);
/*based on the renderer scale / game units*/
void camera_set_bounds(struct camera* camera, float min_x, float min_y, float max_x, float max_y);
void camera_clear_bounds(struct camera* camera);
void camera_set_active_focus_zone(struct camera* camera, struct camera_focus_zone* zone);

void transform_point_into_camera_space(struct camera* camera, int* x, int* y);
struct rectangle camera_get_bounds(struct camera* camera);
#endif
