local void game_animation_id_player_death(float dt) {
    /* restore to last soul anchor. */
    /* after the animation finishes */
    void game_player_revive_warp(struct entity* player);
    game_player_revive_warp(&game_state->persistent_entities[0]);
    game_player_revive_warp(&game_state->persistent_entities[1]);
    game_player_revive_warp(&game_state->persistent_entities[2]);
    game_player_revive_warp(&game_state->persistent_entities[3]);
    animation_id = GAME_ANIMATION_ID_NONE;
}
