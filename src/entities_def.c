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
    
} entity_flags;

struct entity {
    KINEMATIC_ENITTY_BASE_BODY();

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
