/*
  This will be used for game state persistence in the world.
  
  It's kind of an alternative to a flag variable system (which may still be used),
  but basically this is just taking the idea of a delta system and using it to store gamestate.
  
  IE:
  Load Level
  Play through level. Things happen, record persistent changes to buffer (permenant enemy death? New entity spawn?)
  Load Level again
  read the change list, and apply it to the level before we get to play.
*/

#define MAX_PERSISTENT_CHANGE_LIST_CHANGES (512) /* changelists should be contiguous for the least amount of savegame pain. */
#define MAX_PERSISTENT_CHANGE_LISTS        (256)  /* we need to be able to account for all levels. */
#define GLOBAL_PERSISTENT_CHANGES_ID (0)

/* this might be the only type of thing I end up using these for,
   so eh. I guess it was kind of cool to do though? */
enum persistent_change_type {
    PERSISTENT_CHANGE_NONE,
    PERSISTENT_CHANGE_SET_ENTITY_STATUS,
    PERSISTENT_CHANGE_SET_ENEMY_STATUS,    /* Probably only used on bosses. */
    /* 
       It's a waste to queue up lots of block changes since I can do it wholesale...
       In certain special cases

       like permenantly removing a path back, to force you to progress through an area.
    */
    PERSISTENT_CHANGE_SET_BLOCK_REGION_STATUS,
    PERSISTENT_CHANGE_SET_BLOCK_REGION_FLAGS,

    PERSISTENT_CHANGE_SET_ENTITY_FLAGS,
    PERSISTENT_CHANGE_SET_ENEMY_FLAGS,

    /* use these for late game status */
    PERSISTENT_CHANGE_SET_LEVEL_COLORSCHEME,
    PERSISTENT_CHANGE_SET_LEVEL_MUSIC,

    PERSISTENT_CHANGE_COUNT,
};

struct persistent_change {
    uint16_t type; /* 65535 types of changes... Ought to be good enough for anybody. */
    union {
        /*
          It is possible we want to change target locations for whatever reason based on game event
          ids. Which are going to be hardcoded, cause I don't know if I have enough time to figure out elegant
          architecture, for something I'm not really able to use to it's full potential due to jam time (even if it is
          a month, the game is designed to be intentionally simple. So I don't know of how to use it in more ways anyhow.)
         
          16 chars is the size of a "string reference" in this engine,
          and we reserve enough for 8 64 bit integers for some reason.
          
          This should be reasonably robust. (or 4 rectangles with 32bit floats/integers.)
         */
        char reserved[16 + 64];
    };
};

struct persistent_change_list {
    uint32_t hash_id; /* hash for a string id */ 
    struct persistent_change changes[MAX_PERSISTENT_CHANGE_LIST_CHANGES];
};

struct persistent_changes {
    uint16_t list_count;
    struct persistent_change_list lists[MAX_PERSISTENT_CHANGE_LISTS];
};

struct game_state; /*forward decl to avoid error*/
local void persistent_changes_add_change(struct persistent_changes* changes, uint32_t id, struct persistent_change change);
local void persistent_changes_apply_changes(struct persistent_changes* changes, struct game_state* state, uint32_t id);
