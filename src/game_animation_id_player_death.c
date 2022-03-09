local void game_animation_id_player_death(float dt) {
    /* restore to last soul anchor. */
    /* after the animation finishes */
    void game_player_revive_warp(void);
    game_player_revive_warp();
    animation_id = GAME_ANIMATION_ID_NONE;
}
