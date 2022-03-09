/*
  All the special cased game animation code
  
  (I mean larger scale animations,

  boss intros if I feel like I have the gall to do them, will have to be even more special
  cased without any tools... And they may or may not go here... Based on level design???

  Oh my god. That's what happens during a jam I guess.)
*/


#include "game_animation_id_change_level.c"
#include "game_animation_id_player_death.c"
#include "game_animation_id_player_badfall.c"

local void do_render_game_global_animations(float dt) {
    switch (animation_id) {
        case GAME_ANIMATION_ID_CHANGE_LEVEL: {
            game_animation_id_change_level(dt);
        } break;
        case GAME_ANIMATION_ID_PLAYER_DEATH: {
            game_animation_id_player_death(dt);
        } break;
        case GAME_ANIMATION_ID_PLAYER_BADFALL: {
            game_animation_id_player_badfall(dt);
        } break;
    }
}
