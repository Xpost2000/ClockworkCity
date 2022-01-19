#include "common.h"

#include "input.h"

struct {
    struct input_state current_state;
    struct input_state last_state;
} global_input = {};

void register_key_down(int keyid) {
    global_input.current_state.keys[keyid] = true;
}

void register_key_up(int keyid) {
    global_input.current_state.keys[keyid] = false;
}

bool is_key_down(int keyid) {
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


void begin_input_frame(void) {}
void end_input_frame(void) {
    global_input.last_state = global_input.current_state;
}
