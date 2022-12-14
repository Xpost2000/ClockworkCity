#ifndef INPUT_H
#define INPUT_H
/*
  90% sure I don't need input recording at any stage of this
  gamejam. So I'm not going to have anything related to that...
  
  Not even for debugging... I'm hoping to not require anything that complicated...
  Famous last words anyways. Oh well, I've got a month to get the general engine stuff.
  
  I'm sure if I test some basic metroidvania stuff, if it comes up I'll handle it then.
 */
enum mouse_button {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_COUNT,
};

enum keyboard_button {
    KEY_UNKNOWN,
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
    KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
    KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
    KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
    KEY_F11, KEY_F12, KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT,
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_MINUS, KEY_BACKQUOTE, KEY_EQUALS,
    KEY_SEMICOLON, KEY_QUOTE, KEY_COMMA,
    KEY_PERIOD, KEY_RETURN, KEY_BACKSPACE, KEY_ESCAPE,

    KEY_INSERT, KEY_HOME, KEY_PAGEUP, KEY_PAGEDOWN, KEY_DELETE, KEY_END,
    KEY_PRINTSCREEN,

    /*
      I probably don't actually care about mapping these keys.
    */
    KEY_PAUSE, KEY_SCROLL_LOCK, KEY_NUMBER_LOCK,
    KEYPAD_0, KEYPAD_1, KEYPAD_2, KEYPAD_3, KEYPAD_4,
    KEYPAD_5, KEYPAD_6, KEYPAD_7, KEYPAD_8, KEYPAD_9,

    KEYPAD_LEFT, KEYPAD_RIGHT, KEYPAD_UP, KEYPAD_DOWN,
    KEYPAD_ASTERISK, KEYPAD_BACKSLASH,
    KEYPAD_MINUS, KEYPAD_PLUS, KEYPAD_PERIOD,

    KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET,
    KEY_FORWARDSLASH, KEY_BACKSLASH,

    KEY_TAB, KEY_SHIFT,
    KEY_META, KEY_SUPER, KEY_SPACE,

    KEY_CTRL, KEY_ALT,

    KEY_COUNT
};

enum controller_button {
    BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y,
    BUTTON_RS, BUTTON_LS,
    BUTTON_RB, BUTTON_LB,
    BUTTON_START, BUTTON_BACK,
    DPAD_UP,DPAD_DOWN,DPAD_LEFT,DPAD_RIGHT,
    BUTTON_COUNT,
};

struct game_controller_triggers {
    float left;
    float right;
};
struct game_controller_joystick {
    float axes[2];
};
struct game_controller {
    struct game_controller_triggers triggers;
    uint8_t                         buttons[BUTTON_COUNT];
    struct game_controller_joystick left_stick;
    struct game_controller_joystick right_stick;

    struct game_controller_triggers last_triggers;
    uint8_t                         last_buttons[BUTTON_COUNT];
    struct game_controller_joystick last_left_stick;
    struct game_controller_joystick last_right_stick;

    void* _internal_controller_handle;
};

/* 1.0 - 0.0 */
void controller_rumble(struct game_controller* controller, float x_magnitude, float y_magnitude, uint32_t ms);
bool controller_button_pressed(struct game_controller* controller, uint8_t button_id);

/* KEYPAD keys are left out because I have not mapped them yet. */
shared_storage char* keyboard_key_strings[] = {
    [KEY_UNKNOWN] = "Unknown Key?",

    [KEY_A] = "A",
    [KEY_B] = "B",
    [KEY_C] = "C",
    [KEY_D] = "D",
    [KEY_E] = "E",
    [KEY_F] = "F",
    [KEY_G] = "G",
    [KEY_H] = "H",
    [KEY_I] = "I",
    [KEY_J] = "J",
    [KEY_K] = "K",
    [KEY_L] = "L",
    [KEY_M] = "M",
    [KEY_N] = "N",
    [KEY_O] = "O",
    [KEY_P] = "P",
    [KEY_Q] = "Q",
    [KEY_R] = "R",
    [KEY_S] = "S",
    [KEY_T] = "T",
    [KEY_U] = "U",
    [KEY_V] = "V",
    [KEY_W] = "W",
    [KEY_X] = "X",
    [KEY_Y] = "Y",
    [KEY_Z] = "Z",

    [KEY_F1]  = "F1",
    [KEY_F2]  = "F2",
    [KEY_F3]  = "F3",
    [KEY_F4]  = "F4",
    [KEY_F5]  = "F5",
    [KEY_F6]  = "F6",
    [KEY_F7]  = "F7",
    [KEY_F8]  = "F8",
    [KEY_F9]  = "F9",
    [KEY_F10] = "F10",
    [KEY_F11] = "F11",
    [KEY_F12] = "F12",

    [KEY_MINUS]     = "-",
    [KEY_BACKQUOTE] = "`",
    [KEY_EQUALS]    = "=",
    [KEY_SEMICOLON] = ";",
    [KEY_QUOTE]     = "\'",
    [KEY_COMMA]     = ",",
    [KEY_PERIOD]    = ".",

    [KEY_RETURN]    = "Return",
    [KEY_BACKSPACE] = "Backspace",
    [KEY_ESCAPE]    = "Escape",

    [KEY_INSERT]   = "Insert",
    [KEY_HOME]     = "Home",
    [KEY_PAGEUP]   = "Page Up",
    [KEY_PAGEDOWN] = "Page Down",
    [KEY_DELETE]   = "Delete",
    [KEY_END]      = "End",

    [KEY_PRINTSCREEN] = "Print Screen",
    [KEY_PAUSE]       = "Pause",
    
    [KEY_SCROLL_LOCK] = "Scroll Lock",
    [KEY_NUMBER_LOCK] = "Number Lock",

    [KEY_LEFT_BRACKET]  = "[",
    [KEY_RIGHT_BRACKET] = "]",

    [KEY_FORWARDSLASH] = "/",
    [KEY_BACKSLASH]    = "\\",

    [KEY_TAB]   = "Tab",
    [KEY_SHIFT] = "Shift",
    [KEY_CTRL]  = "Control",
    [KEY_ALT]   = "Alt",

    [KEY_SPACE] = "Space",
};

struct input_state {
    bool keys[KEY_COUNT];
    int mouse_x;
    int mouse_y;
    bool mouse_buttons[MOUSE_BUTTON_COUNT];

    bool editting_text;
    int text_edit_cursor;
    char text[1024];
};

struct game_controller* get_gamepad(int index);

enum {
    CONTROLLER_JOYSTICK_LEFT,
    CONTROLLER_JOYSTICK_RIGHT,
};
local float angle_formed_by_joystick(struct game_controller* controller, int which) {
    struct game_controller_joystick target;
    switch (which) {
        case CONTROLLER_JOYSTICK_LEFT: {
            target = controller->left_stick;
        } break;
        case CONTROLLER_JOYSTICK_RIGHT: {
            target = controller->right_stick;
        } break;
    }

    float angle = atan2(target.axes[1], target.axes[0]);
    return angle;
}

void register_key_down(int keyid);
void register_key_up(int keyid);

void register_mouse_position(int x, int y);
void register_mouse_button(int button_id, bool state);

bool is_key_down(int keyid);
bool is_key_pressed(int keyid);
bool any_key_down(void);
bool controller_any_button_down(struct game_controller* controller);

void get_mouse_location(int* mx, int* my);
void get_mouse_buttons(bool* left, bool* middle, bool* right);

void begin_input_frame(void);
void end_input_frame(void);

void start_text_edit(char* target, size_t length);
void end_text_edit(char* target, size_t amount); /*copies all text input into target buffer. Not necessarily unicode aware. whoops!*/

void send_text_input(char* text, size_t text_length);
bool is_editting_text(void);
char* current_text_buffer(void);

#endif
