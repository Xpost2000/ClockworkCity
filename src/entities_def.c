/*
  A vast majority of the entity code is kind of else where because the tilemaps is probably
  the vast majority of their code.
*/

#define KINEMATIC_ENITTY_BASE_BODY()            \
    float x;                                    \
    float y;                                    \
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
struct entity {
    KINEMATIC_ENITTY_BASE_BODY();

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

    /*temporary*/
    int facing_dir;
    bool dash;
    /*end temporary*/

    /* player specific */
    float jump_leniancy_timer;
};
