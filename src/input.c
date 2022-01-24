#include "common.h"

#include "input.h"

struct {
    struct input_state current_state;
    struct input_state last_state;
} global_input = {};
local struct game_controller global_controllers[4];

void register_key_down(int keyid) {
    global_input.current_state.keys[keyid] = true;
}

void register_key_up(int keyid) {
    global_input.current_state.keys[keyid] = false;
}

void register_mouse_position(int x, int y) {
    global_input.current_state.mouse_x = x;
    global_input.current_state.mouse_y = y;
}

void register_mouse_button(int button_id, bool state) {
    assert((button_id >= 0 && button_id < MOUSE_BUTTON_COUNT) && "wtf?");
    global_input.current_state.mouse_buttons[button_id] = state;
}

void get_mouse_location(int* mx, int* my) {
    safe_assignment(mx) = global_input.current_state.mouse_x;
    safe_assignment(my) = global_input.current_state.mouse_y;
}

void get_mouse_buttons(bool* left, bool* middle, bool* right) {
    safe_assignment(left)   = global_input.current_state.mouse_buttons[MOUSE_BUTTON_LEFT];
    safe_assignment(middle) = global_input.current_state.mouse_buttons[MOUSE_BUTTON_MIDDLE];
    safe_assignment(right)  = global_input.current_state.mouse_buttons[MOUSE_BUTTON_RIGHT];
}

bool is_key_down(int keyid) {
    assert(keyid < KEY_COUNT && "invalid key id?");
    return global_input.current_state.keys[keyid];
}

bool is_key_pressed(int keyid) {
    bool current_keystate = global_input.current_state.keys[keyid];
    bool last_keystate = global_input.last_state.keys[keyid];

    if (current_keystate == last_keystate) {
        return false;
    }

    if (current_keystate == true && last_keystate == false) {
        return true;
    }

    return false;
}

struct game_controller* get_gamepad(int index) {
    assert(index >= 0 && index < array_count(global_controllers) && "Bad controller index");
    return global_controllers + index;
}

void begin_input_frame(void) {
}
void end_input_frame(void) {
    global_input.last_state = global_input.current_state;
    zero_buffer_memory(&global_input.current_state, sizeof(global_input.current_state));
}
