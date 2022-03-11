/*
((PLAYER_IDLE1 TEXTURE assets/player/1.png)
 (LOSTSOUL_IDLE1 TEXTURE assets/lostsoul/1idle.png)
 (LOSTSOUL_CLOSEDIDLE1 TEXTURE assets/lostsoul/1idleclosed.png)
 (LOSTSOUL_IDLE1_CRACKED TEXTURE assets/lostsoul/1idlea.png)
 (LOSTSOUL_CLOSEDIDLE1_CRACKED TEXTURE assets/lostsoul/1idlecloseda.png)
 (BOSS4_HAND_OPEN TEXTURE assets/boss4/hand.png)
 (BOSS4_HAND_CLOSED TEXTURE assets/boss4/hand_fist.png)
 (BOSS4_HEADTOP TEXTURE assets/boss4/headtop.png)
 (BOSS4_HEADTOP1 TEXTURE assets/boss4/headtop_dmg1.png)
 (BOSS4_HEADTOP2 TEXTURE assets/boss4/headtop_dmg2.png)
 (IMP_IDLE TEXTURE assets/enemy1/k1.png)
 (IMP_ATTACK1 TEXTURE assets/enemy1/k1a1.png)
 (IMP_WALK1 TEXTURE assets/enemy1/k1w1.png)
 (IMP_WALK2 TEXTURE assets/enemy1/k1w2.png)
 (HERO_IDLE1 TEXTURE assets/player1/player.png)
 (HERO_IDLE2 TEXTURE assets/player1/player1.png)
 (HERO_SLASH1 TEXTURE assets/player1/player_a3.png)
 (HERO_SLASH2 TEXTURE assets/player1/player_a4.png)
 (HERO_WALK1 TEXTURE assets/player1/player_w1.png)
 (HERO_WALK2 TEXTURE assets/player1/player_w2.png)
 (HERO_WALK3 TEXTURE assets/player1/player_w3.png)
 (ENTITY_WALK1 SOUND assets/sounds/walk1.wav)
 (ENTITY_WALK2 SOUND assets/sounds/walk2.wav)
 (ENTITY_WALK3 SOUND assets/sounds/walk3.wav)
 (ENTITY_WALK4 SOUND assets/sounds/walk4.wav)
 (ENTITY_IMPACT1 SOUND assets/sounds/impact1.wav)
 (ENTITY_IMPACT2 SOUND assets/sounds/impact2.wav)
 (ENTITY_SPIRIT_IMPACT1 SOUND assets/sounds/impactb1.wav)
 (ENTITY_SPIRIT_IMPACT2 SOUND assets/sounds/impactb2.wav))
((PLAYER_IDLE1 TEXTURE assets/player/1.png)
 (LOSTSOUL_IDLE1 TEXTURE assets/lostsoul/1idle.png)
 (LOSTSOUL_CLOSEDIDLE1 TEXTURE assets/lostsoul/1idleclosed.png)
 (LOSTSOUL_IDLE1_CRACKED TEXTURE assets/lostsoul/1idlea.png)
 (LOSTSOUL_CLOSEDIDLE1_CRACKED TEXTURE assets/lostsoul/1idlecloseda.png)
 (BOSS4_HAND_OPEN TEXTURE assets/boss4/hand.png)
 (BOSS4_HAND_CLOSED TEXTURE assets/boss4/hand_fist.png)
 (BOSS4_HEADTOP TEXTURE assets/boss4/headtop.png)
 (BOSS4_HEADTOP1 TEXTURE assets/boss4/headtop_dmg1.png)
 (BOSS4_HEADTOP2 TEXTURE assets/boss4/headtop_dmg2.png)
 (IMP_IDLE TEXTURE assets/enemy1/k1.png)
 (IMP_ATTACK1 TEXTURE assets/enemy1/k1a1.png)
 (IMP_WALK1 TEXTURE assets/enemy1/k1w1.png)
 (IMP_WALK2 TEXTURE assets/enemy1/k1w2.png)
 (HERO_IDLE1 TEXTURE assets/player1/player.png)
 (HERO_IDLE2 TEXTURE assets/player1/player1.png)
 (HERO_SLASH1 TEXTURE assets/player1/player_a3.png)
 (HERO_SLASH2 TEXTURE assets/player1/player_a4.png)
 (HERO_WALK1 TEXTURE assets/player1/player_w1.png)
 (HERO_WALK2 TEXTURE assets/player1/player_w2.png)
 (HERO_WALK3 TEXTURE assets/player1/player_w3.png))
((ENTITY_WALK1 SOUND assets/sounds/walk1.wav)
 (ENTITY_WALK2 SOUND assets/sounds/walk2.wav)
 (ENTITY_WALK3 SOUND assets/sounds/walk3.wav)
 (ENTITY_WALK4 SOUND assets/sounds/walk4.wav)
 (ENTITY_IMPACT1 SOUND assets/sounds/impact1.wav)
 (ENTITY_IMPACT2 SOUND assets/sounds/impact2.wav)
 (ENTITY_SPIRIT_IMPACT1 SOUND assets/sounds/impactb1.wav)
 (ENTITY_SPIRIT_IMPACT2 SOUND assets/sounds/impactb2.wav))
*/
// 21 textures?
local texture_id player_idle1 = {};
local texture_id lostsoul_idle1 = {};
local texture_id lostsoul_closedidle1 = {};
local texture_id lostsoul_idle1_cracked = {};
local texture_id lostsoul_closedidle1_cracked = {};
local texture_id boss4_hand_open = {};
local texture_id boss4_hand_closed = {};
local texture_id boss4_headtop = {};
local texture_id boss4_headtop1 = {};
local texture_id boss4_headtop2 = {};
local texture_id imp_idle = {};
local texture_id imp_attack1 = {};
local texture_id imp_walk1 = {};
local texture_id imp_walk2 = {};
local texture_id hero_idle1 = {};
local texture_id hero_idle2 = {};
local texture_id hero_slash1 = {};
local texture_id hero_slash2 = {};
local texture_id hero_walk1 = {};
local texture_id hero_walk2 = {};
local texture_id hero_walk3 = {};
local void initialize_entity_graphics_assets(void) {
player_idle1 = load_texture("assets/player/1.png");
lostsoul_idle1 = load_texture("assets/lostsoul/1idle.png");
lostsoul_closedidle1 = load_texture("assets/lostsoul/1idleclosed.png");
lostsoul_idle1_cracked = load_texture("assets/lostsoul/1idlea.png");
lostsoul_closedidle1_cracked = load_texture("assets/lostsoul/1idlecloseda.png");
boss4_hand_open = load_texture("assets/boss4/hand.png");
boss4_hand_closed = load_texture("assets/boss4/hand_fist.png");
boss4_headtop = load_texture("assets/boss4/headtop.png");
boss4_headtop1 = load_texture("assets/boss4/headtop_dmg1.png");
boss4_headtop2 = load_texture("assets/boss4/headtop_dmg2.png");
imp_idle = load_texture("assets/enemy1/k1.png");
imp_attack1 = load_texture("assets/enemy1/k1a1.png");
imp_walk1 = load_texture("assets/enemy1/k1w1.png");
imp_walk2 = load_texture("assets/enemy1/k1w2.png");
hero_idle1 = load_texture("assets/player1/player.png");
hero_idle2 = load_texture("assets/player1/player1.png");
hero_slash1 = load_texture("assets/player1/player_a3.png");
hero_slash2 = load_texture("assets/player1/player_a4.png");
hero_walk1 = load_texture("assets/player1/player_w1.png");
hero_walk2 = load_texture("assets/player1/player_w2.png");
hero_walk3 = load_texture("assets/player1/player_w3.png");
}
// 8 sounds?
local sound_id entity_walk1 = {};
local sound_id entity_walk2 = {};
local sound_id entity_walk3 = {};
local sound_id entity_walk4 = {};
local sound_id entity_impact1 = {};
local sound_id entity_impact2 = {};
local sound_id entity_spirit_impact1 = {};
local sound_id entity_spirit_impact2 = {};
local void initialize_entity_audio_assets(void) {
entity_walk1 = load_sound("assets/sounds/walk1.wav");
entity_walk2 = load_sound("assets/sounds/walk2.wav");
entity_walk3 = load_sound("assets/sounds/walk3.wav");
entity_walk4 = load_sound("assets/sounds/walk4.wav");
entity_impact1 = load_sound("assets/sounds/impact1.wav");
entity_impact2 = load_sound("assets/sounds/impact2.wav");
entity_spirit_impact1 = load_sound("assets/sounds/impactb1.wav");
entity_spirit_impact2 = load_sound("assets/sounds/impactb2.wav");
}
