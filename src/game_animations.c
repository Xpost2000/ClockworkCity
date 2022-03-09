/*
  All the special cased game animation code
  
  (I mean larger scale animations,

  boss intros if I feel like I have the gall to do them, will have to be even more special
  cased without any tools... And they may or may not go here... Based on level design???

  Oh my god. That's what happens during a jam I guess.)
*/

local void game_animation_id_change_level(float dt) {
    const float HALF_QUEUED_LEVEL_LOAD_TIME = QUEUE_LEVEL_LOAD_FADE_TIMER_MAX/2;
    const float TOTAL_ANIMATION_TIME = QUEUE_LEVEL_LOAD_FADE_TIMER_MAX + QUEUE_LEVEL_LOAD_FADE_TIMER_LINGER_MAX; 
    union color4f fade_color = COLOR4F_BLACK;

    if (queued_level_transition.fade_timer < HALF_QUEUED_LEVEL_LOAD_TIME) {
        /* fade in */
        float adjusted_time = queued_level_transition.fade_timer - QUEUE_LEVEL_LOAD_FADE_TIMER_LINGER_MAX;
        fade_color.a = interpolation_clamp(adjusted_time / HALF_QUEUED_LEVEL_LOAD_TIME);
    } else {
        /* load here and unset everything */
        if (!queued_load_level_loaded()) {
            game_load_level(queued_level_transition.arena,
                            queued_level_transition.filename,
                            queued_level_transition.transition_link_to_spawn_at);
            queued_level_transition.loaded = true;
        }

        /* fade out */
        float adjusted_time = queued_level_transition.fade_timer - HALF_QUEUED_LEVEL_LOAD_TIME;
        fade_color.a = interpolation_clamp(1.0 - (adjusted_time / HALF_QUEUED_LEVEL_LOAD_TIME));
    }


    if (queued_level_transition.fade_timer >= TOTAL_ANIMATION_TIME) {
        queued_level_transition.queued = false;
        animation_id = GAME_ANIMATION_ID_NONE;
        camera_resume_tracking(&game_camera);
    }

    if (queued_level_transition.fade_timer >= (0.523 * TOTAL_ANIMATION_TIME)) {
        camera_resume_tracking(&game_camera);
    } else {
        camera_stop_tracking(&game_camera);
    }

    begin_graphics_frame(NULL); {
        draw_filled_rectangle(0, 0, 9999, 9999, fade_color);
    } end_graphics_frame();

    queued_level_transition.fade_timer += dt;
}

local void game_animation_id_player_death(float dt) {
    /* restore to last soul anchor. */
    /* after the animation finishes */
    void game_player_revive_warp(void);
    game_player_revive_warp();
    animation_id = GAME_ANIMATION_ID_NONE;
}

local void game_animation_id_player_badfall(float dt) {
}

local void do_render_game_global_animations(float dt) {
    switch (animation_id) {
        case GAME_ANIMATION_ID_CHANGE_LEVEL: {
            game_animation_id_change_level(dt);
        } break;
        case GAME_ANIMATION_ID_PLAYER_DEATH: {
            game_animation_id_player_death(dt);
        } break;
        case GAME_ANIMATION_ID_PLAYER_BADFALL: {
            game_animation_id_player_badfall(dt);
        } break;
    }
}
