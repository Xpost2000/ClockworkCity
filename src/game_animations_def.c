/*
  Will involve lots of special cased code.
  Not serializing this :)
 */

enum game_animation_id {
    GAME_ANIMATION_ID_NONE,
    GAME_ANIMATION_ID_CHANGE_LEVEL,
    /* 
       This is only here because I don't have an elaborate animation
       system...

       GAME_ANIMATION_ID_PLAYER_INTRO, 
    */
    GAME_ANIMATION_ID_PLAYER_DEATH,
    GAME_ANIMATION_ID_PLAYER_BADFALL,
};
char* game_animation_id_strings[] = {
    "none", "change_level", "player_death", "player_bad_fall", 
};

enum player_death_animation_state {
    DEATH_ANIMATION_NOT_STARTED,
    DEATH_ANIMATION_STAGE1,
    DEATH_ANIMATION_STAGE2,
    DEATH_ANIMATION_STAGE3,
    DEATH_ANIMATION_STATE_COUNT,
};

enum game_animation_id animation_id = GAME_ANIMATION_ID_NONE;

/*
  NOTE(jerry):
  This is measured in half times.
  
  (Fade in & Fade out)
*/
#define QUEUE_LEVEL_LOAD_FADE_TIMER_MAX (1.2)
#define QUEUE_LEVEL_LOAD_FADE_TIMER_LINGER_MAX (0.2)
struct queued_load_level {
    bool queued;
    bool loaded;

    struct memory_arena* arena;
    char transition_link_to_spawn_at[TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH];
    char filename[FILENAME_MAX_LENGTH];

    float fade_timer;
};

struct queued_load_level queued_level_transition = {};

local bool queued_load_level_loaded(void) {
    return queued_level_transition.loaded;
}


void game_queue_load_level(struct memory_arena* arena, char* filename, char* transition_link_to_spawn_at) {
    if (!queued_level_transition.queued) {
        queued_level_transition.arena                       = arena;
        strncpy(queued_level_transition.filename, filename, FILENAME_MAX_LENGTH);
        strncpy(queued_level_transition.transition_link_to_spawn_at, transition_link_to_spawn_at, TRANSITION_ZONE_IDENTIIFER_STRING_LENGTH);
        queued_level_transition.fade_timer                  = 0;
        queued_level_transition.queued                      = true;
        queued_level_transition.loaded                      = false;
        animation_id                                        = GAME_ANIMATION_ID_CHANGE_LEVEL;
    }
}
