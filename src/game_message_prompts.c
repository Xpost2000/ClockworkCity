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
#define PROMPT_FADE_IN_TIMER_GENERIC (0.2)
#define PROMPT_TIME_PER_PAGE_GENERIC (1.0)

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

/* could use xmacros...? */
#define Define_Prompt(name) local void name(struct game_controller* controller, float dt)
typedef void (*prompt_proc)(struct game_controller* controller, float dt);

/* NOTE(jerry): Example templates for prompts */
/* simple non-pausing, time based fade */
Define_Prompt(DEVTEST_prompt1) {
    console_printf("PROMPT1\n");
    float prompt_alpha = interpolation_clamp(global_prompt_state.timer / PROMPT_FADE_IN_TIMER_GENERIC);
    union color4f prompt_color = COLOR4F_BLACK;
    union color4f text_color   = COLOR4F_WHITE;

    if (global_prompt_state.timer >= PROMPT_TIME_PER_PAGE_GENERIC) {
        /* end here */
        prompt_alpha = interpolation_clamp(1.2 - global_prompt_state.timer - (PROMPT_TIME_PER_PAGE_GENERIC));

        if (prompt_alpha <= 0.0) {
            game_close_prompt();
        }
    }

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
    if (global_prompt_state.id != PROMPT_ID_NONE) {
        prompt_proc prompt_procedure = game_prompt_update_renders[global_prompt_state.id];

        if (!prompt_procedure) {
            fprintf(stderr, "prompt id (%d) not implemented\n", global_prompt_state.id);
            unimplemented();
        }

        prompt_procedure(controller, dt);
        global_prompt_state.timer += dt;
    }
}

