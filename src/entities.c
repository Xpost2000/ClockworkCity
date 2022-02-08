/*
  NOTE(jerry): work on this later tonight.
  
  I'm probably going to need "IMGUI" buttons, anyways hopefully this makes most of the editor
  code okay.
  
  These are really "kinematic" entities, or otherwise "living" things. So entity doesn't actually
  mean every random thing.
  
  Any entity here is explicitly something that requires the high resolution physics and is an enemy
  or NPC or what have you that can live.
  
  Anything else will have it's own dedicated typed array.
*/

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

/* NOTE(jerry):
   These procedures are isolated from the world right now. Also
 */
typedef void (*entity_physics_update_procedure)(struct entity* self, float dt);  /* not sure why this has to happen*/
typedef void (*entity_update_procedure)(struct entity* self, float dt);
/* NOTE(jerry):
   We really shouldn't have to do this but okay.
 */
typedef void (*entity_draw_procedure)(struct entity* self, float dt);

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

local entity_draw_procedure           draw_procedures[ENTITY_TYPE_COUNT] = {
    
};

local entity_update_procedure         update_procedures[ENTITY_TYPE_COUNT] = {
    
};

local entity_physics_update_procedure physics_update_procedures[ENTITY_TYPE_COUNT] = {

};

/*
  This might just turn into an uber struct or something.
*/
struct entity {
    KINEMATIC_ENITTY_BASE_BODY();

    uint32_t type;
    int32_t  health;

    /*temporary*/
    int facing_dir;
    bool dash;
    /*end temporary*/

    /* player specific */
    float jump_leniancy_timer;
};

void entity_halt_motion(struct entity* entity) {
    entity->ax = entity->ay = entity->vx = entity->vy = entity->last_vy = 0;
}

struct entity player = {
    // no units, prolly pixels
    .x = -4,
    .y = -5,
    .w = 1/2.0f,
    .h = 1,
};
