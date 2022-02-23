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
*/
enum prompt_id {
    PROMPT_ID_NONE = 0,

    PROMPT_ID_STANDARD_START,
    PROMPT_ID_TUTORIAL_PROMPT_START,

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
   
   (ONLY WORKS IF TIMER >= time_per_page),

   so it results in kind of weird looking code but whatever.
   
   outputs alpha and whether a page advance is possible
*/
bool do_prompt_page_advancing(float* prompt_alpha, float time_per_page, float fade_in_time, float linger_time) {
    /* nocheck prompt_alpha */
    if (global_prompt_state.timer >= time_per_page) {
        /* fade out! */
        *prompt_alpha = interpolation_clamp((1 + linger_time)
                                            -
                                            (global_prompt_state.timer - (time_per_page)) / fade_in_time);

        if (global_prompt_state.timer >= (time_per_page + fade_in_time)) {
            if (*prompt_alpha <= 0.0) {
                return true;
            }
        }
    }

    return false;
}

Define_Prompt(DEVTEST_prompt1) {
    float prompt_alpha = interpolation_clamp(global_prompt_state.timer / PROMPT_FADE_IN_TIMER_GENERIC);
    union color4f prompt_color = COLOR4F_BLACK;
    union color4f text_color   = COLOR4F_WHITE;

    if (do_prompt_page_advancing(&prompt_alpha, PROMPT_TIME_PER_PAGE_GENERIC, PROMPT_FADE_IN_TIMER_GENERIC, PROMPT_TIME_LINGER_TIME_GENERIC)) {
        game_close_prompt();
    }

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
    game_close_prompt();
}

local prompt_proc game_prompt_update_renders[PROMPT_ID_COUNT] = {
    [PROMPT_ID_TEST_PROMPT]  = DEVTEST_prompt1,
    [PROMPT_ID_TEST1_PROMPT] = DEVTEST_prompt2,
    [PROMPT_ID_TEST2_PROMPT] = DEVTEST_prompt3,
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

