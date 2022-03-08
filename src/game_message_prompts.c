/*
  NOTE(jerry):
  All the prompts are hard coded in here.

  I'm sure it would be nice to actually data specify these, but parsing
  mark up to generate a small splash screen is not really a good idea to me...
  
  Also it's not particularly hard to code these prompts... It worked fine for
  TreeWatcher, so it's gotta work fine here!
  
  TODO(jerry):
  add a "trigger" entity.
  
  Triggers just fire out arbitrary integers, that the engine can intercept.
  
  They will remain as arbitrary integers in the editor simply because idk
  if there will be time otherwise (says this with 25 days left...)
  
  NOTE(jerry):
  While all of these things generally follow a pattern, and could theoretically
  could just be turned into a file format.
  
  It's way easier to just code it as data.
  
  NOTE(jerry):
  I have no idea how I managed to come up an inane way of producing slide-show prompts.
  If I still wanted to retain similar behavior honestly, I could just do.
  
  enum {
  EFFECT_FADE_IN, EFFECT_FADE_OUT,
  };
  struct prompt_slide {
  float slide_duration;
  float effect_time;
  int effects;
  };
  
  struct prompt_slide slides[] = {
  something_with_fade_in,
  ... insert slides,
  something_with_fade_out,
  }
  
  int current = 0;
  
  get current slide, and render it's generic properties (fade in or whatever)
  switch (current) {
  do my thing here.
  }
  
  Oh well.
*/
enum prompt_id {
    PROMPT_ID_NONE = 0,
    PROMPT_ID_CONTROL_SCHEME_OVERVIEW,
/*
  This prompt shows an overview of all controls, but there are more "in-depth" tutorials
  when actually needed.
*/
    PROMPT_ID_EXPOSITION_STORY1,
    PROMPT_ID_FIRSTDEATH_STORY2,
    PROMPT_ID_FOUND_HUB_STORY3,
    /*
     * what it sounds like
     */


    /* developer test prompts */
    PROMPT_ID_TEST_PROMPT = 700000,
    PROMPT_ID_TEST1_PROMPT,
    PROMPT_ID_TEST2_PROMPT,

    PROMPT_ID_COUNT,
};

/* 
   prompts can pause the game here should they wish,

   this should all basically be self-contained.
*/
#define PROMPT_FADE_IN_TIMER_GENERIC (0.3)
#define PROMPT_TIME_LINGER_TIME_GENERIC (1)
#define PROMPT_TIME_PER_PAGE_GENERIC (1.3)

#define PROMPT_TOTAL_TIME_PER_PAGE_GENERIC (PROMPT_TIME_PER_PAGE_GENERIC + PROMPT_TIME_LINGER_TIME_GENERIC + PROMPT_FADE_IN_TIMER_GENERIC)

local font_id controls_prompt_font;

struct prompt_state {
    uint32_t id;

    /* any additional state that the prompts may need for some reason */
    int32_t page; /* or stage. */

    float timer;
};

local struct prompt_state global_prompt_state = {};

local void game_close_prompt(void) {
    console_printf("prompt closed\n");
    global_prompt_state.id   = PROMPT_ID_NONE;
    global_prompt_state.page = 0;
}

local void game_activate_prompt(uint32_t id) {
    console_printf("activated prompt id: %d\n", id);
    global_prompt_state.id    = id;
    global_prompt_state.timer = 0;
}

local bool game_no_prompt(void) {
    return global_prompt_state.id == PROMPT_ID_NONE;
}

/* could use xmacros...? */
#define Define_Prompt(name) local void name(struct game_controller* controller, float dt)
typedef void (*prompt_proc)(struct game_controller* controller, float dt);

/* NOTE(jerry): Example templates for prompts */
/* simple non-pausing, time based fade */

/* 
   automatically does page fading. 
   
   return true on page change. This allows you to handle things based on the page it just
   went to.
*/
bool do_prompt_page_advancing(float* prompt_alpha, float time_per_page, float fade_in_time, float linger_time, int last_page) {
    /* nocheck prompt_alpha */
    if (global_prompt_state.timer >= time_per_page) {
        /* fade out! NOTE(jerry): fade in is handled else where. */
        *prompt_alpha = interpolation_clamp((1 + linger_time)
                                            -
                                            (global_prompt_state.timer - (time_per_page)) / fade_in_time);

        if (global_prompt_state.timer >= (time_per_page + fade_in_time)) {
            if (*prompt_alpha <= 0.0) {
                global_prompt_state.page += 1;
                global_prompt_state.timer = 0;

                if (global_prompt_state.page > last_page) {
                    game_close_prompt();
                }

                return true;
            }
        }
    }

    return false;
}

/* An overglorified clamp of some kind. */
float do_prompt_multipage_alpha(float original_alpha, float fade_in_time, int last_page) {
    if (global_prompt_state.page == 0) {
        if (global_prompt_state.timer >= PROMPT_FADE_IN_TIMER_GENERIC) {
            return 1.0;
        } else {
            return original_alpha;
        }
    } else if (global_prompt_state.page == last_page) {
        if (global_prompt_state.timer <= PROMPT_FADE_IN_TIMER_GENERIC) {
            return 1.0;
        } else {
            return original_alpha;
        }
    }

    return 1.0;
}

Define_Prompt(DEVTEST_prompt1) {
    float prompt_alpha = interpolation_clamp(global_prompt_state.timer / PROMPT_FADE_IN_TIMER_GENERIC);
    union color4f prompt_color = COLOR4F_BLACK;
    union color4f text_color   = COLOR4F_WHITE;

    do_prompt_page_advancing(&prompt_alpha, PROMPT_TIME_PER_PAGE_GENERIC, PROMPT_FADE_IN_TIMER_GENERIC, PROMPT_TIME_LINGER_TIME_GENERIC, 0);
    if (game_no_prompt()) return;

    prompt_color.a = prompt_alpha * 0.6;
    text_color.a   = prompt_alpha;

    begin_graphics_frame(NULL); {
        draw_filled_rectangle(0, 0, 9999, 9999, prompt_color);
        draw_text(test3_font, 0, 0, "Hello World", text_color);
    } end_graphics_frame();

}

/* confirm with pause */
Define_Prompt(DEVTEST_prompt2) {
    game_close_prompt();
}

/* kind with multiple pages */
Define_Prompt(DEVTEST_prompt3) {
    float prompt_alpha = interpolation_clamp(global_prompt_state.timer / PROMPT_FADE_IN_TIMER_GENERIC);

    const int LAST_PAGE = 1;
    union color4f prompt_color = COLOR4F_BLACK;
    union color4f text_color   = COLOR4F_WHITE;

    do_prompt_page_advancing(&prompt_alpha, PROMPT_TIME_PER_PAGE_GENERIC, PROMPT_FADE_IN_TIMER_GENERIC, PROMPT_TIME_LINGER_TIME_GENERIC, LAST_PAGE);
    prompt_color.a = do_prompt_multipage_alpha(prompt_alpha, PROMPT_FADE_IN_TIMER_GENERIC, LAST_PAGE) * 0.6;

    if (game_no_prompt()) return;

    text_color.a   = prompt_alpha;

    begin_graphics_frame(NULL); {
        draw_filled_rectangle(0, 0, 9999, 9999, prompt_color);
        draw_text(test3_font, 0, 0, "Hello World", text_color);
    } end_graphics_frame();

}

struct control_scheme_description {
    uint32_t button_glyph[2]; /* 2 for joystick or dpad */
    uint32_t key;
    char*    description;
};

/* this is really for polish reasons, not really much. */
local struct control_scheme_description default_controls_prompt[] = {
    {
        .button_glyph[0] = FONT_GLYPH_DPAD_LEFT,
        .button_glyph[1] = FONT_GLYPH_LEFT_ANALOG_LEFT,
        .key = 'A',
        .description = "Move Left"
    },
    {
        .button_glyph[0] = FONT_GLYPH_DPAD_RIGHT,
        .button_glyph[1] = FONT_GLYPH_LEFT_ANALOG_RIGHT,
        .key = 'D',
        .description = "Move Right"
    },
    {
        .button_glyph[0] = FONT_GLYPH_BUTTON_A,
        .key = FONT_GLYPH_SPACE,
        .description = "Jump"
    },
    {
        .button_glyph[0] = FONT_GLYPH_RT,
        .key = FONT_GLYPH_SHIFT,
        .description = "Dash"
    },
    {
        .button_glyph[0] = FONT_GLYPH_BUTTON_X,
        .key = 'J',
        .description = "Attack"
    },
    {
        .button_glyph[0] = FONT_GLYPH_BUTTON_Y,
        .key = 'R',
        .description = "(hold) Recall"
    },
};

Define_Prompt(prompt_control_scheme_overview) {
    float prompt_alpha = interpolation_clamp(global_prompt_state.timer / PROMPT_FADE_IN_TIMER_GENERIC);

    const int LAST_PAGE = 0;
    union color4f prompt_color = COLOR4F_BLACK;
    union color4f text_color   = COLOR4F_WHITE;

    do_prompt_page_advancing(&prompt_alpha, PROMPT_TIME_PER_PAGE_GENERIC*3, PROMPT_FADE_IN_TIMER_GENERIC*1.4, PROMPT_TIME_LINGER_TIME_GENERIC*1.2, LAST_PAGE);
    prompt_color.a = do_prompt_multipage_alpha(prompt_alpha, PROMPT_FADE_IN_TIMER_GENERIC*1.4, LAST_PAGE) * 0.6;

    if (game_no_prompt()) return;

    text_color.a   = prompt_alpha;

    float y_cursor = 0;
    begin_graphics_frame(NULL); {
        draw_filled_rectangle(0, 0, 9999, 9999, prompt_color);
        {
            char* title_text = "Controls";
            int dimens[2];
            get_screen_dimensions(dimens, dimens+1);
            int tdimens[2];
            get_text_dimensions(game_title_font, title_text, tdimens, tdimens+1);
            draw_text(game_title_font, dimens[0]/2 - tdimens[0]/2, 0, title_text, text_color);

            y_cursor += tdimens[1] * 1.35;
        }

        int space_width;
        int text_height;
        get_text_dimensions(controls_prompt_font, " ", &space_width, &text_height);

        int widest_advance = 0;
        for (unsigned index = 0; index < array_count(default_controls_prompt); ++index) {
            struct control_scheme_description* control_description = default_controls_prompt + index;
            int current_widest_advance = 0;
            if (control_description->button_glyph[0]) {
                int advance;
                get_codepoint_dimensions(controller_prompt_font[PROMPT_FONT_SIZE_SMALL], control_description->button_glyph[0], &advance, 0);
                current_widest_advance += advance * 1.1;
            }

            if (control_description->button_glyph[1]) {
                int advance;
                get_codepoint_dimensions(controller_prompt_font[PROMPT_FONT_SIZE_SMALL], control_description->button_glyph[1], &advance, 0);
                current_widest_advance += advance * 1.1;
            }

            if (control_description->key) {
                int advance;
                get_codepoint_dimensions(controller_prompt_font[PROMPT_FONT_SIZE_SMALL], control_description->key, &advance, 0);
                current_widest_advance += advance * 1.3;
            }

            if (widest_advance < current_widest_advance) widest_advance = current_widest_advance;
        }

        for (unsigned index = 0; index < array_count(default_controls_prompt); ++index) {
            struct control_scheme_description* control_description = default_controls_prompt + index;
            float x_cursor = space_width * 4;

            if (control_description->button_glyph[0]) {
                int advance;
                get_codepoint_dimensions(controller_prompt_font[PROMPT_FONT_SIZE_SMALL], control_description->button_glyph[0], &advance, 0);
                draw_codepoint(controller_prompt_font[PROMPT_FONT_SIZE_SMALL], x_cursor, y_cursor - text_height*0.5, control_description->button_glyph[0], text_color);
                x_cursor += advance * 1.1;
            }

            if (control_description->button_glyph[1]) {
                int advance;
                get_codepoint_dimensions(controller_prompt_font[PROMPT_FONT_SIZE_SMALL], control_description->button_glyph[1], &advance, 0);
                draw_codepoint(controller_prompt_font[PROMPT_FONT_SIZE_SMALL], x_cursor, y_cursor - text_height*0.5, control_description->button_glyph[1], text_color);
                x_cursor += advance * 1.1;
            }

            if (control_description->key) {
                int advance;
                get_codepoint_dimensions(controller_prompt_font[PROMPT_FONT_SIZE_SMALL], control_description->key, &advance, 0);
                draw_codepoint(controller_prompt_font[PROMPT_FONT_SIZE_SMALL], x_cursor, y_cursor - text_height*0.5, control_description->key, text_color);
                x_cursor += advance * 1.3;
            }

            x_cursor = space_width * 6 + widest_advance;
            draw_text(controls_prompt_font, x_cursor, y_cursor, control_description->description, text_color);
            y_cursor += text_height * 1.5;
        }
    } end_graphics_frame();
}

/* byebye localization efforts. */
/* code as data? data as code? who knows anymore */

/* this makes me cry. */
#define Define_Text_Based_Prompt(name, text_array, prompt_time_per_page, prompt_fade_in_timer, prompt_time_Linger_time, black_strength, blocks_player_input) Define_Prompt(name) {                                               \ 
        float prompt_alpha = interpolation_clamp(global_prompt_state.timer / PROMPT_FADE_IN_TIMER_GENERIC); \
    \
    const int LAST_PAGE = array_count(text_array)-1;                    \
    union color4f prompt_color = COLOR4F_BLACK;                         \
    union color4f text_color   = COLOR4F_WHITE;                         \
    \
    do_prompt_page_advancing(&prompt_alpha, prompt_time_per_page*3, prompt_fade_in_timer*1, prompt_time_Linger_time, LAST_PAGE); \
    prompt_color.a = do_prompt_multipage_alpha(prompt_alpha, prompt_fade_in_timer*1, LAST_PAGE) * black_strength; \
    \
    if (blocks_player_input) {                                          \
        if (game_no_prompt()) {                                         \
            block_player_input = false;                                 \
            return;                                                     \
        } else {                                                        \
            block_player_input = true;                                  \
        }                                                               \
    }                                                                   \
    \
    text_color.a   = prompt_alpha;                                      \
    \
    begin_graphics_frame(NULL); {                                       \
        draw_filled_rectangle(0, 0, 9999, 9999, prompt_color);          \
        {                                                               \
            char* text = text_array[global_prompt_state.page];          \
            int dimens[2];                                              \
            get_screen_dimensions(dimens, dimens+1);                    \
            int tdimens[2];                                             \
            get_text_dimensions(game_ui_menu_font, text, tdimens, tdimens+1); \
            draw_text(game_ui_menu_font, dimens[0]/2 - tdimens[0]/2, dimens[1]/2 - tdimens[1]/2, text, text_color); \
        }                                                               \
    } end_graphics_frame();                                             \
}

local char* story1_prompt_text[] = {
    "Death is a new beginning.",
    "A fresh start.",
    "Destroy the guardians.",
    "Become whole.",
    "Ascend.",
};

local char*  comfort1_prompt_text[] = {
    "Comfort.",
    "Safety.",
    "Venture into the unknown to ascend.",
};
local char* death1_prompt_text[] = {
    "Do not fear your death.",
    "Return stronger."
};

Define_Text_Based_Prompt(prompt_story1, story1_prompt_text, PROMPT_TIME_PER_PAGE_GENERIC, PROMPT_FADE_IN_TIMER_GENERIC, PROMPT_TIME_LINGER_TIME_GENERIC, 0.85, 1);
/*( TODO)*/
Define_Text_Based_Prompt(prompt_comfort1, comfort1_prompt_text, PROMPT_TIME_PER_PAGE_GENERIC, PROMPT_FADE_IN_TIMER_GENERIC, PROMPT_TIME_LINGER_TIME_GENERIC, 0.85, 1);
/*( TODO)*/
Define_Text_Based_Prompt(prompt_first_death, death1_prompt_text, PROMPT_TIME_PER_PAGE_GENERIC, PROMPT_FADE_IN_TIMER_GENERIC, PROMPT_TIME_LINGER_TIME_GENERIC, 0.85, 1);

local prompt_proc game_prompt_update_renders[PROMPT_ID_COUNT] = {
    [PROMPT_ID_TEST_PROMPT]             = DEVTEST_prompt1,
    [PROMPT_ID_TEST1_PROMPT]            = DEVTEST_prompt2,
    [PROMPT_ID_TEST2_PROMPT]            = DEVTEST_prompt3,
    [PROMPT_ID_CONTROL_SCHEME_OVERVIEW] = prompt_control_scheme_overview,
    [PROMPT_ID_EXPOSITION_STORY1]       = prompt_story1,
    [PROMPT_ID_FIRSTDEATH_STORY2]       = prompt_first_death,
    [PROMPT_ID_FOUND_HUB_STORY3]        = prompt_comfort1,
};

local void update_render_game_prompt(struct game_controller* controller, float dt) {
    if (!game_no_prompt()) {
        prompt_proc prompt_procedure = game_prompt_update_renders[global_prompt_state.id];

        if (!prompt_procedure) {
            fprintf(stderr, "prompt id (%d) not implemented\n", global_prompt_state.id);
            unimplemented();
        }

        prompt_procedure(controller, dt);
        global_prompt_state.timer += dt;
    }
}

void load_all_resources_for_prompts(void) {
    /* TODO(jerry): fonts... Get rid of test fonts */
    controls_prompt_font = load_font("assets/Exoplanetaria-gxxJ5.ttf", font_size_aspect_ratio_independent(prompt_font_sizes[PROMPT_FONT_SIZE_SMALL]* 0.7));
}
