/*
  A vast majority of the entity code is kind of else where because the tilemaps is probably
  the vast majority of their code.
*/

#define KINEMATIC_ENTITY_BASE_BODY()            \
    float x, last_x;                            \
    float y, last_y;                            \
    float w;                                    \
    float h;                                    \
    float vx;                                   \
    float vy;                                   \
    float ax;                                   \
    float ay;                                   \
    float last_vy;                              \
    uint8_t onground

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
    ENTITY_TYPE_NONE = 0,
    ENTITY_TYPE_PLAYER = 1,
    ENTITY_TYPE_DUMMYA = 2,

    ENTITY_TYPE_COUNT,
};
char* entity_type_strings[] = {
    "?",
    "Player",
    "Dummy(1) Entity",
    0
};

/*
  This might just turn into an uber struct or something.
*/
typedef enum entity_flags {
    ENTITY_FLAGS_NONE = 0,
    ENTITY_FLAGS_PERMENANT = BIT(0),
    ENTITY_FLAGS_RESERVED_2         = BIT(1),
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
    "reserved 2",
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
    DEATH_STATE_DEAD,
};
shared_storage char* death_state_strings[] = {
    [DEATH_STATE_ALIVE] = "alive",
    [DEATH_STATE_DYING] = "dying",
    [DEATH_STATE_DEAD] = "dead",
};

#define ENTITY_COYOTE_JUMP_TIMER_MAX        (0.12)
#define ENTITY_STRING_IDENTIFIER_MAX_LENGTH (8) /*entity12*/ /*use for hardcoded triggers*/
#define PLAYER_VARIABLE_JUMP_TIME_LIMIT     (0.17)
#define PLAYER_VARIABLE_JUMP_ACCELERATION   (GRAVITY_CONSTANT*1.3)
struct entity {
    KINEMATIC_ENTITY_BASE_BODY();
    char identifier[ENTITY_STRING_IDENTIFIER_MAX_LENGTH];

    /*
      NOTE(jerry):
      These two are unusually fat, but this gives me lots of flexibility,
      (as in I don't need to deprecate any entities per say. Since I have lots of bits to play with.
      (IE I don't need to change the binary format when serializing, which is the most important part since I don't
      wanna spend time updating my old level designs to accommodate new features very much. IE: Full backwards compat without any
      work.))
     */
    uint32_t flags;
    uint32_t type; 

    int32_t  health;
    uint8_t  death_state; /* use this for animation setting. */

    /*temporary*/
    int facing_dir;
    bool dash;
    /*end temporary*/

    float   coyote_jump_timer;
    float   player_variable_jump_time;
    uint8_t current_jump_count;
    uint8_t max_allowed_jump_count;
};

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
    struct entity_iterator_list* new_list = &iterator->lists[iterator->list_count++];
    new_list->list_length = count;
    new_list->list        = list;
}

struct entity* entity_iterator_begin(struct entity_iterator* iterator) {
    iterator->current_list_item_index = iterator->current_list_index = 0;
    struct entity* result = &iterator->lists[iterator->current_list_index].list[iterator->current_list_item_index];
    return result;
}

struct entity* entity_iterator_next(struct entity_iterator* iterator) {
    {
        struct entity_iterator_list* current_list = &iterator->lists[iterator->current_list_index];

        if (iterator->current_list_item_index < current_list->list_length) {
            iterator->current_list_item_index += 1; 
        } else {
            iterator->current_list_item_index = 0;
            iterator->current_list_index     += 1;
        }
    }

    struct entity* result = &iterator->lists[iterator->current_list_index].list[iterator->current_list_item_index];
    return result;
}

bool entity_iterator_done(struct entity_iterator* iterator) {
    if (iterator->current_list_index >= iterator->list_count) {
        struct entity_iterator_list* current_list = &iterator->lists[iterator->current_list_index];
        if (iterator->current_list_item_index >= current_list->list_length) {
            return true;
        }
    }

    return false;
}
