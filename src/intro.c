/*
  Basic attract mode intro
*/
local texture_id tarkus_icon;

enum {
    INTRO_PHASE_PRELUDE,
    INTRO_PHASE_BANNER,
    INTRO_PHASE_DISCLAIMER,
    INTRO_PHASE_DISCLAIMER2,
    INTRO_PHASE_DONE,
    INTRO_PHASE_COUNT,
};

local float intro_phase_times[] = {
    [INTRO_PHASE_PRELUDE]     = 2,
    [INTRO_PHASE_BANNER]      = 3.5,
    [INTRO_PHASE_DISCLAIMER]  = 5,
    [INTRO_PHASE_DISCLAIMER2] = 4,
    [INTRO_PHASE_DONE]        = 0.0
};

struct {
    int   phase;
    float timer;
} intro_state = {};

void intro_load_resources(void) {
    tarkus_icon = load_texture("assets/banner.png");
}

void advance_intro_state(void) {
    intro_state.timer = 0;
    intro_state.phase += 1;
}

void intro_update_render_frame(float dt) {
    intro_state.timer += dt;

    /* float alpha = clampf(intro_state.timer / intro_phase_times[intro_state.phase], 0, 1); */
    float phase_time_limit = intro_phase_times[intro_state.phase];

    int screen_dimensions[2];
    get_screen_dimensions(screen_dimensions, screen_dimensions+1);

    if (any_key_down() || controller_any_button_down(get_gamepad(0))) {
        advance_intro_state();
    }

    begin_graphics_frame(NULL); {
        switch (intro_state.phase) {
            case INTRO_PHASE_BANNER: {
                int desired_width  = 2238 / (screen_dimensions[0]/850);
                int desired_height = 2004 / (screen_dimensions[0]/850);

                union color4f color = COLOR4F_WHITE;

                if (intro_state.timer < (phase_time_limit-0.5)/2 + 0.3) {
                    color.a = (intro_state.timer / (phase_time_limit/2));
                } else if (intro_state.timer > (phase_time_limit-0.5)/2 + 0.3) {
                    color.a = 1 - ((intro_state.timer-(phase_time_limit-0.3)/2) / ((phase_time_limit-0.5)/2));
                }

                color.a = clampf(color.a, 0, 1);

                float render_x = screen_dimensions[0]/2 - desired_width/2;
                float render_y = screen_dimensions[1]/2 - desired_height/2;
                float render_w = desired_width;
                float render_h = desired_height;

                draw_texture(tarkus_icon, render_x, render_y, render_w, render_h, color);
            } break;
            case INTRO_PHASE_DISCLAIMER: {
                const char* text = "THIS IS A JAM GAME\nEXPECT A LITTLE BIT OF BUGGYNESS\nOR UNFINISHEDNESS!\n";

                int text_dimens[2];
                get_text_dimensions(game_ui_menu_font, text, text_dimens, text_dimens+1);

                float render_x = screen_dimensions[0]/2 - text_dimens[0]/2;
                float render_y = screen_dimensions[1]/2 - text_dimens[1]/2;

                draw_text(game_ui_menu_font, render_x, render_y, text, COLOR4F_WHITE);
            } break;
            case INTRO_PHASE_DISCLAIMER2: {
                /* real yakuza play with a controller */
                const char* text = "THERE IS ALSO CONTROLLER SUPPORT :)\nINCLUDING RUMBLE (IF SUPPORTED)\n\nTHIS IS PROBABLY BEST PLAYED\nWITH A CONTROLLER IF POSSIBLE\n";

                int text_dimens[2];
                get_text_dimensions(game_ui_menu_font, text, text_dimens, text_dimens+1);

                float render_x = screen_dimensions[0]/2 - text_dimens[0]/2;
                float render_y = screen_dimensions[1]/2 - text_dimens[1]/2;

                draw_text(game_ui_menu_font, render_x, render_y, text, COLOR4F_WHITE);
            } break;
            case INTRO_PHASE_DONE: {
                /* allow gameplay fade transition? */
                console_execute_cstr("load limbo1");
                mode = GAME_MODE_PLAYING;
            } break;
        }  
    } end_graphics_frame();
    
    if (intro_state.timer >= phase_time_limit) {
        advance_intro_state();
    }
}
