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

#define MAX_PERSISTENT_CHANGE_LIST_CHANGES (350) /* changelists should be contiguous for the least amount of savegame pain. */
#define MAX_PERSISTENT_CHANGE_LISTS        (150)  /* we need to be able to account for all levels. */
#define GLOBAL_PERSISTENT_CHANGES_ID (0)

/* this might be the only type of thing I end up using these for,
   so eh. I guess it was kind of cool to do though? */
enum persistent_change_type {
    PERSISTENT_CHANGE_NONE,

    PERSISTENT_CHANGE_SOUL_ANCHOR_ACTIVATED,
    PERSISTENT_CHANGE_ADD_PLAYER_MOVEMENT_FLAG,
    PERSISTENT_CHANGE_KILLED_BOSS,

    PERSISTENT_CHANGE_COUNT,
};

struct persistent_change_killed_boss {
    uint32_t number;
};

struct persistent_change_soul_anchor_activated {
    uint32_t soul_anchor_index;
};

struct persistent_change_add_player_movement_flag {
    uint8_t flag;
};

struct persistent_change {
    uint16_t type; /* 65535 types of changes... Ought to be good enough for anybody. */
    union {
        struct persistent_change_soul_anchor_activated    soul_anchor_activated;
        struct persistent_change_add_player_movement_flag add_player_movement_flag;
        struct persistent_change_killed_boss              killed_boss;
        char reserved[16 + 64];
    };
};

struct persistent_change_list {
    char level_name[FILENAME_MAX_LENGTH];
    struct persistent_change changes[MAX_PERSISTENT_CHANGE_LIST_CHANGES];
};

struct persistent_changes {
    uint16_t list_count;
    struct persistent_change_list lists[MAX_PERSISTENT_CHANGE_LISTS+1];
};

struct game_state; /*forward decl to avoid error*/
local void persistent_changes_add_change(struct persistent_changes* changes, char* level_name, struct persistent_change change);
local void persistent_changes_apply_changes(struct persistent_changes* changes, struct game_state* state, uint32_t id);
local uint32_t persistent_changes_find_change_list_by_name(struct persistent_changes* changes, char* name);
local void persistent_changes_serialize(struct persistent_changes* changes, struct binary_serializer* serializer);
