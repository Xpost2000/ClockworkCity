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
*/
enum prompt_id {
    PROMPT_ID_NONE,

    PROMPT_ID_STANDARD_START,
    PROMPT_ID_TUTORIAL_PROMPT_START,

    PROMPT_ID_COUNT,
};

/* 
   prompts can pause the game here should they wish,

   this should all basically be self-contained.
*/
struct prompt_state {
    uint32_t id;

    /* any additional state that the prompts may need for some reason */
    int32_t page;
};

local struct prompt_state global_prompt_state = {};

void game_close_prompt(void) {
    global_prompt_state.id   = PROMPT_ID_NONE;
    global_prompt_state.page = 0;
}

void game_activate_prompt(uint32_t id) {
    assert(id > PROMPT_ID_NONE && id < PROMPT_ID_COUNT);
    global_prompt_state.id = id;
}

/* BEGIN PROMPTS */
/* END PROMPTS */

void update_render_game_prompt(struct game_controller* controller, float dt) {
    if (global_prompt_state.id != PROMPT_ID_NONE) {
        switch (global_prompt_state.id) {
            default: {
                printf("Prompt not implemented!\n");
                unimplemented();
            } break;
        }
    }
}

