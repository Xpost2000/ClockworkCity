/*
((PLAYER_IDLE1 TEXTURE assets/player/1.png)
 (LOSTSOUL_IDLE1 TEXTURE assets/lostsoul/1idle.png)
 (LOSTSOUL_CLOSEDIDLE1 TEXTURE assets/lostsoul/1idleclosed.png)
 (LOSTSOUL_IDLE1_CRACKED TEXTURE assets/lostsoul/1idlea.png)
 (LOSTSOUL_CLOSEDIDLE1_CRACKED TEXTURE assets/lostsoul/1idlecloseda.png))
((PLAYER_IDLE1 TEXTURE assets/player/1.png)
 (LOSTSOUL_IDLE1 TEXTURE assets/lostsoul/1idle.png)
 (LOSTSOUL_CLOSEDIDLE1 TEXTURE assets/lostsoul/1idleclosed.png)
 (LOSTSOUL_IDLE1_CRACKED TEXTURE assets/lostsoul/1idlea.png)
 (LOSTSOUL_CLOSEDIDLE1_CRACKED TEXTURE assets/lostsoul/1idlecloseda.png))
NIL
*/
// 5 textures?
local texture_id player_idle1 = {};
local texture_id lostsoul_idle1 = {};
local texture_id lostsoul_closedidle1 = {};
local texture_id lostsoul_idle1_cracked = {};
local texture_id lostsoul_closedidle1_cracked = {};
local void initialize_entity_graphics_assets(void) {
player_idle1 = load_texture("assets/player/1.png");
lostsoul_idle1 = load_texture("assets/lostsoul/1idle.png");
lostsoul_closedidle1 = load_texture("assets/lostsoul/1idleclosed.png");
lostsoul_idle1_cracked = load_texture("assets/lostsoul/1idlea.png");
lostsoul_closedidle1_cracked = load_texture("assets/lostsoul/1idlecloseda.png");
}
// 0 sounds?
local void initialize_entity_audio_assets(void) {
}
