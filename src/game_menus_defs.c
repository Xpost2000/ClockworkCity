#define DYNAMIC_FONT_SIZE (0.5) /* big so it scales okayly? */
#define GAME_UI_TITLE_FONT_SIZE (font_size_aspect_ratio_independent(0.15))
#define GAME_UI_MENU_FONT_SIZE (font_size_aspect_ratio_independent(0.075))

#define QUIT_FADE_TIMER (0.35)
#define QUIT_FADE_TIMER2 (0.5)
#define INGAME_PAN_OUT_TIMER (0.25)
#define START_MENU_ALPHA (0.85)

enum gameplay_ui_mode {
    GAMEPLAY_UI_MAINMENU,
    GAMEPLAY_UI_PAUSEMENU,
    GAMEPLAY_UI_INGAME,
    GAMEPLAY_UI_LOAD_SAVE,
    GAMEPLAY_UI_OPTIONS,

    GAMEPLAY_UI_MODE_COUNT,
};

/* crufty animations */
enum gameplay_ui_transition {
    GAMEPLAY_UI_TRANSITION_NONE,
    GAMEPLAY_UI_TRANSITION_TO_QUIT,
    GAMEPLAY_UI_TRANSITION_TO_INGAME,
    GAMEPLAY_UI_TRANSITION_TO_PAUSE,
};

enum {
    MAINMENU_UI_PLAY_GAME,
    /* MAINMENU_UI_LOAD_GAME, */
    /* MAINMENU_UI_OPTIONS, */
    MAINMENU_UI_QUIT,
};
char* menu_option_strings[] = {
    "Play Game",
    /* "Load Game", */
    /* "Options", */
    "Quit",
};

char* menu_pause_option_strings[] = {
    "Resume Game",
    /* "Load Game", */
    /* "Options", */
    "Return To Main Menu",
    "Quit To Desktop",
};
