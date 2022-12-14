/*
  Entity assets
 */

/* FINAL BOSS(BOSS4) */
/* END FINAL BOSS(BOSS4) */

/*
  A vast majority of the entity code is kind of else where because the tilemaps is probably
  the vast majority of their code.
*/
enum attack_direction {
    ATTACK_DIRECTION_RIGHT,
    ATTACK_DIRECTION_LEFT,
    ATTACK_DIRECTION_DOWN,
    ATTACK_DIRECTION_UP,
};

int opposite_attack_direction(int original) {
    switch (original) {
        case ATTACK_DIRECTION_LEFT:  return ATTACK_DIRECTION_RIGHT;
        case ATTACK_DIRECTION_RIGHT: return ATTACK_DIRECTION_LEFT;
        case ATTACK_DIRECTION_UP:    return ATTACK_DIRECTION_DOWN;
        case ATTACK_DIRECTION_DOWN:  return ATTACK_DIRECTION_UP;
    }

    assert(!"impossible");
    return -1;
}

enum movement_flags { /* eh? */
    MOVEMENT_FLAG_ALLOW_WALL_JUMP = BIT(0), /*wall grinding*/
    MOVEMENT_FLAG_ALLOW_DASH      = BIT(1),
};

#define KINEMATIC_ENTITY_BASE_BODY()            \
    float x, last_x, initial_x;                 \
    float y, last_y, initial_y;                 \
    float w;                                    \
    float h;                                    \
    float vx;                                   \
    float vy;                                   \
    float ax;                                   \
    float ay;                                   \
    float last_vy;                              \
    uint8_t movement_flags;                     \
    uint8_t sliding_against_wall;               \
    uint8_t onground;

/* 
   To allow this to technically be "future proof", I'd actually
   have to pretend the ids always kept going up... Who knows?
*/

/* 
   NOTE(jerry):
   regarding animations! While technically most entities will die in the same way:
   (play death animation, then turn into particles that blow away like ash. Some enemies will
   have special animations... Like bosses or the player with more dramatic ones. Right now I'm planning
   to hardcode all of these special animations...)
*/
enum entity_type {
    ENTITY_TYPE_NONE               = 0,
    ENTITY_TYPE_PLAYER             = 1,
    /* 
       If I spent more time thinking I would've obviously just made these
       different types through behavioral settings (like flags), and not
       do type based dispatch. But this is fast and I'm not adding many more
       enemy types.
     */
    ENTITY_TYPE_HOVERING_LOST_SOUL = 2,
    ENTITY_TYPE_LOST_SOUL          = 3,
    ENTITY_TYPE_VOLATILE_LOST_SOUL = 4,

    ENTITY_TYPE_CHARGING_IMP       = 5,
    ENTITY_TYPE_MELEE_IMP          = 6,

    ENTITY_TYPE_BOSS1              = 7,
    ENTITY_TYPE_BOSS2              = 8,
    ENTITY_TYPE_BOSS3              = 9,
    ENTITY_TYPE_BOSS4              = 10,
    /* ENTITY_TYPE_MEPHIT_MINOR       = 5, */
    /* ENTITY_TYPE_MEPHIT             = 6, */

    /* 
       Entities past this point cannot be created in the
       editor, these are spawned for very specific situations.
    */
    ENTITY_TYPE_LAST_SPAWNABLE_ENTITY_TYPE,

    ENTITY_TYPE_COUNT, /* inaccurate??? */
};

/* hardcoded type baking. fun */
local bool entity_is_physics_active(uint32_t entity_type) {
    switch (entity_type) {
        case ENTITY_TYPE_PLAYER:
        case ENTITY_TYPE_CHARGING_IMP:
        case ENTITY_TYPE_MELEE_IMP:
            return true;
        default:
            return false;
    }
}

char* entity_type_strings[] = {
    "?",
    [ENTITY_TYPE_PLAYER] = "Player",
    /* These two should reform. This could be a flag but whatever. */
    [ENTITY_TYPE_HOVERING_LOST_SOUL] = "Lost Soul(idle)",
    [ENTITY_TYPE_LOST_SOUL] = "Lost Soul",
    /* these can die */
    [ENTITY_TYPE_VOLATILE_LOST_SOUL] = "Volatile Lost Soul",
    [ENTITY_TYPE_CHARGING_IMP]       = "Imp (charging)",
    [ENTITY_TYPE_MELEE_IMP]       = "Imp (can melee)",

    [ENTITY_TYPE_BOSS1]       = "BOSS1",
    [ENTITY_TYPE_BOSS2]       = "BOSS2",
    [ENTITY_TYPE_BOSS3]       = "BOSS3",
    [ENTITY_TYPE_BOSS4]       = "God Eater",
    /* more flying skins */
    /* [ENTITY_TYPE_MEPHIT_MINOR] = "Lesser Mephit", */
    /* [ENTITY_TYPE_MEPHIT] = "Mephit", */
    0
};

/*
  This might just turn into an uber struct or something.
*/
typedef enum entity_flags {
    ENTITY_FLAGS_NONE = 0,
    ENTITY_FLAGS_PERMENANT = BIT(0),
    ENTITY_FLAGS_DISABLED         = BIT(1),
    ENTITY_FLAGS_RESERVED_3         = BIT(2),
    ENTITY_FLAGS_RESERVED_4         = BIT(3),
    ENTITY_FLAGS_RESERVED_5         = BIT(4),
    ENTITY_FLAGS_RESERVED_6         = BIT(5),
    ENTITY_FLAGS_RESERVED_7         = BIT(6),
    ENTITY_FLAGS_RESERVED_8         = BIT(7),
    ENTITY_FLAGS_RESERVED_9         = BIT(8),
    ENTITY_FLAGS_RESERVED_10        = BIT(9),
    ENTITY_FLAGS_RESERVED_11        = BIT(10),
    ENTITY_FLAGS_RESERVED_12        = BIT(11),
    ENTITY_FLAGS_RESERVED_13        = BIT(12),
    ENTITY_FLAGS_RESERVED_14        = BIT(13),
    ENTITY_FLAGS_RESERVED_15        = BIT(14),
    ENTITY_FLAGS_RESERVED_16        = BIT(15),
    ENTITY_FLAGS_RESERVED_17        = BIT(16),
    ENTITY_FLAGS_RESERVED_18        = BIT(17),
    ENTITY_FLAGS_RESERVED_19        = BIT(18),
    ENTITY_FLAGS_RESERVED_20        = BIT(19),
    ENTITY_FLAGS_RESERVED_21        = BIT(20),
    ENTITY_FLAGS_RESERVED_22        = BIT(21),
    ENTITY_FLAGS_RESERVED_23        = BIT(22),
    ENTITY_FLAGS_RESERVED_24        = BIT(23),
    ENTITY_FLAGS_RESERVED_25        = BIT(24),
    ENTITY_FLAGS_RESERVED_26        = BIT(25),
    ENTITY_FLAGS_RESERVED_27        = BIT(26),
    ENTITY_FLAGS_RESERVED_28        = BIT(27),
    ENTITY_FLAGS_RESERVED_29        = BIT(28),
    ENTITY_FLAGS_RESERVED_30        = BIT(29),
    ENTITY_FLAGS_RESERVED_31        = BIT(30),
    ENTITY_FLAGS_RESERVED_32        = BIT(31),
} entity_flags;

char* entity_flag_strings[] = {
    "no flags",
    "permenant entity",
    "DISABLED",
    "reserved 3",
    "reserved 4",
    "reserved 5",
    "reserved 6",
    "reserved 7",
    "reserved 8",
    "reserved 9",
    "reserved 10",
    "reserved 11",
    "reserved 12",
    "reserved 13",
    "reserved 14",
    "reserved 15",
    "reserved 16",
    "reserved 17",
    "reserved 18",
    "reserved 19",
    "reserved 20",
    "reserved 21",
    "reserved 22",
    "reserved 23",
    "reserved 24",
    "reserved 25",
    "reserved 26",
    "reserved 27",
    "reserved 28",
    "reserved 29",
    "reserved 30",
    "reserved 31",
    "reserved 32",
};

/*
  NOTE(jerry):
  Actually, EVERYTHING is fat here.

  This just gives me the most flexibility, at the cost of memory storage. Which is fine,
  we don't have many full entities usually. (the thinking kind),

  smaller insignificant entities are specialized in their own types (props, or particles, or literally anything
  else that isn't living.)
 */
enum death_state {
    DEATH_STATE_ALIVE,
    DEATH_STATE_DYING,
    DEATH_STATE_DYING_FROM_SPIKE, /* gives me permission to animate them in a fun way :) */
    DEATH_STATE_DEAD,
};
shared_storage char* death_state_strings[] = {
    [DEATH_STATE_ALIVE]            = "alive",
    [DEATH_STATE_DYING]            = "dying",
    [DEATH_STATE_DYING_FROM_SPIKE] = "badfalldeath",
    [DEATH_STATE_DEAD]             = "dead",
};

#define ENTITY_COYOTE_JUMP_TIMER_MAX               (0.12)
#define ENTITY_STRING_IDENTIFIER_MAX_LENGTH        (9) /*entity12*/ /*use for hardcoded triggers*/
#define PLAYER_VARIABLE_JUMP_TIME_LIMIT            (0.17)
#define PLAYER_VARIABLE_JUMP_ACCELERATION          (GRAVITY_CONSTANT*1.3)
#define PLAYER_ATTACK_COOLDOWN_TIMER_MAX           (0.3)
#define ENTITY_DASH_SHADOW_MAX_AMOUNT              (64)
#define ENTITY_DASH_SHADOW_MAX_LINGER_LIMIT        (0.7) /*seconds*/
#define ENTITY_DASH_SHADOW_SAMPLE_RECORD_TIMER_MAX (0.01)
#define DISTANCE_PER_DASH    (3)
#define ENTITY_DASH_TIME_MAX (0.10)
struct entity_dash_shadow {
    /* Flat data, really mostly depends on the code for better rendering */
    float x; float y; float t;
};

/* lost souls cannot collide with each other. */
/* always the player. Attack state. */
#define LOST_SOUL_TRY_TO_SEEK_TIMER_MAX (4)
#define LOST_SOUL_RETRACTION_TIMER_MAX  (2.5)
#define LOST_SOUL_NEXT_SEEK_TIMER_MIN (1.0)
#define LOST_SOUL_NEXT_SEEK_TIMER_MAX (2.0)
struct entity_lost_soul {
    float fly_time;
    float vomit_timer;
    float next_vomit_timer;
    float resurrection_timer;
    struct particle_emitter* owned_vomit_emitter;
    struct particle_emitter* owned_flames_emitter;

    bool  seeking_towards_target;
    float try_to_seek_timer;
    float direction_to_target_x;
    float direction_to_target_y;
    float retraction_timer;
    bool  idle_during_retraction;
    float next_seek_timer;
};

struct entity_imp {
    bool chasing_enemy;
    /* if not chasing anyways, decide whether to be idle or not. */
    float next_think_time;
};

/* 
   this guy is like Elder Hu from Hollow Knight (gives dash).
   
   Basically just Elder Hu, but can use dash for evasion sometimes. (if player is to be attacking in range.)
*/
struct entity_boss1 {
    
};

/* Maybe Hive Knight-ish? (gives walljump),

 mainly attacks by using wall jump and buttslaming for damage. Can melee */
struct entity_boss2 {
    
};

/* Guardian. More like false knight */
struct entity_boss3 {
    
};

/*
  The final boss.
  
  Giant thing. Damage the hands enough and it'll fall down for hitting?
*/

struct entity_boss4_hand {
    int health;

    float x;
    float y;

    bool clenched;
};
/* NOTE(jerry): root x,y is the head */
struct entity_boss4 {
    struct entity_boss4_hand left_hand;
    struct entity_boss4_hand right_hand;

    bool stunned;
    float stunned_animation_timer[2];
    float head_downtime;

    /* animating this... Is a going to be a bitch! Thankfully this thing doesn't have to move. */
    float attack_hand_timer;
    float attack_hand_linger_timer;

#if 0
    /* in second phase, just attack regardless of what enemies are still present. */
    int phase;
    /* check if summoned entities are still alive */
    int  summoned_enemies[6];
#endif
};

/* TODO(jerry): do something about this */
/*
  While the original plan was to use a special trigger in the starting area.
  The shorter plan is to just change recall to do something else in the end.
  
  :)
*/
local bool already_played_finale_prompt   = false;
local bool globally_tracked_boss_kills[4] = {};
local bool can_face_final_boss(void) {
    if (globally_tracked_boss_kills[0] &&
        globally_tracked_boss_kills[1] &&
        globally_tracked_boss_kills[2]) {
        return true;
    }

    return false;
}

struct entity_boss_kind {
    bool active;
    union {
        struct entity_boss1 boss1_info;
        struct entity_boss2 boss2_info;
        struct entity_boss3 boss3_info;
    };
};

struct entity {
    KINEMATIC_ENTITY_BASE_BODY();
    char identifier[ENTITY_STRING_IDENTIFIER_MAX_LENGTH];

    /*
      NOTE(jerry):
      (jesus, this thing has diabetes.)
      These two are unusually fat, but this gives me lots of flexibility,
      (as in I don't need to deprecate any entities per say. Since I have lots of bits to play with.
      (IE I don't need to change the binary format when serializing, which is the most important part since I don't
      wanna spend time updating my old level designs to accommodate new features very much. IE: Full backwards compat without any
      work.))
     */
    uint32_t flags;
    uint32_t type; 

    int32_t  max_health;
    int32_t  health;
    uint8_t  death_state; /* use this for animation setting. */

    /* LOOLOLOLOLOLOLOL FML, FOR NOT HAVING SPRITE ANIMATION LOLOLOLOLOLOLOLOLOL */
    float animation_timer;
    int animation_frame;

    /*temporary*/
    int8_t facing_dir; /* don't know why this is not an enum either... */
    int8_t opposite_facing_direction; /* this state is needed for wall jump. Weird, I know. */

    float   coyote_jump_timer;
    float   player_variable_jump_time;
    uint8_t current_jump_count;
    uint8_t max_allowed_jump_count;
    /*end temporary*/

    float linger_shadow_sample_record_timer;
    uint8_t                   linger_shadow_count;
    struct entity_dash_shadow linger_shadows[ENTITY_DASH_SHADOW_MAX_AMOUNT];

    /* delay per attack input. */
    float attack_cooldown_timer;

    /* qfix */
    /* A wiser man would've probably just did
       
       uint8_t applied_force_count;
       force* forces;
       
       But who said I was a wise man?
       
       NOTE(jerry): all fields here are related to impulses/forces. In case I ever clean it up.
     */
    bool dash;
    float dash_timer; /* may phase out bool dash, since if this dash_timer > 0, implies dashing */
    /* NOTE(jerry): ughhhh more copy and paste */
    uint8_t attack_direction;
    uint8_t damage_direction;
    float apply_wall_jump_force_timer;
    float apply_attack_knockback_force_timer;
    float apply_damage_knockback_force_timer;

    struct trigger* occupied_trigger;

    union {
        struct entity_lost_soul lost_soul_info;
        struct entity_imp       imp_info;
        struct entity_boss_kind boss_info;
    };
};
shared_storage const char* get_facing_direction_string(int x) {
    switch (x) {
        case 1: return "right";
        case -1: return "left";
    }

    return "?";
}

#include "generated_initialize_entity_assets.c"
/* 
   NOTE(jerry):
   This was the solution I found that would be quickest to implement and least amount of effort,
   since I wouldn't need to worry about entities as much if I used this method.
 */
#define HITBOX_POOL_MAX                 (128)
#define HITBOX_MAX_IGNORE_ENTITIES      (8)
#define HITBOX_MAX_ALREADY_HIT_ENTITIES (16)
#define HITBOX_MAX_IGNORE_TYPES         (16)
/*
  Under the assumption that it would be impossible for all entities to hit something and create
  this many hitboxes.

  Projectiles are entities, and their bounds is the hitbox.

  This is just for melee.
  
  This is also lightly fat.
*/
enum hitbox_type {
    HITBOX_TYPE_HURT,
    HITBOX_TYPE_HURT_WITH_KNOCKBACK,
};
struct hitbox {
    struct entity* owner;
    struct entity* ignored_entities[HITBOX_MAX_IGNORE_ENTITIES];

    /* not sure if needed yet.*/
#if 0
    struct entity* already_hit_entities[HITBOX_MAX_ALREADY_HIT_ENTITIES]; /* lingering hitboxes */
    uint32_t       ignored_entity_types[HITBOX_MAX_IGNORE_TYPES];
#endif
    enum hitbox_type type;
    /* not necessarily attack, but relative direction to the entity when it was made */
    enum attack_direction direction;

    float x;
    float y;
    float w;
    float h;

    float lifetime;

    int damage;
};

shared_storage uint16_t      hitbox_count                 =  0;
shared_storage struct hitbox hitbox_pool[HITBOX_POOL_MAX] = {};

float entity_lerp_x(struct entity* entity, float t);
float entity_lerp_y(struct entity* entity, float t);

/*
  A more optimal thing to do would have been to make an intrusive linked list,
  but this is infinitely simpler.
  
  If you have more than this amount of disparate lists... Something might seriously be wrong with
  the development system.
*/
#define ENTITY_ITERATOR_MAX_LISTS (16)
struct entity_iterator_list {
    uint32_t list_length;
    struct entity* list;
};

struct entity_iterator {
    uint8_t list_count;
    uint8_t current_list_index;
    uint32_t current_list_item_index;
    struct entity_iterator_list lists[ENTITY_ITERATOR_MAX_LISTS];
};

void entity_iterator_push_array(struct entity_iterator* iterator, struct entity* list, size_t count) {
    /* nocheck */
    if (list && (count > 0)) {
        struct entity_iterator_list* new_list = &iterator->lists[iterator->list_count++];
        new_list->list_length = count;
        new_list->list        = list;
    }
}

struct entity* entity_iterator_begin(struct entity_iterator* iterator) {
    iterator->current_list_item_index = iterator->current_list_index = 0;
    struct entity* result = &iterator->lists[iterator->current_list_index].list[iterator->current_list_item_index];
    return result;
}

struct entity _entity_sentinel = {};
struct entity* entity_iterator_next(struct entity_iterator* iterator) {
    {
        struct entity_iterator_list* current_list = &iterator->lists[iterator->current_list_index];

        if (iterator->current_list_item_index < current_list->list_length) {
            iterator->current_list_item_index += 1; 
        }

        if (iterator->current_list_item_index >= current_list->list_length) {
            iterator->current_list_item_index = 0;
            if (iterator->current_list_index < iterator->list_count) {
                iterator->current_list_index     += 1;
            } else {
                return &_entity_sentinel;
            }
        }
    }


    struct entity* result = &iterator->lists[iterator->current_list_index].list[iterator->current_list_item_index];
    return result;
}

bool entity_iterator_done(struct entity_iterator* iterator) {
    if (iterator->current_list_index >= iterator->list_count) {
        return true;
    }

    return false;
}
