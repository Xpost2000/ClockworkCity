/*
 * sdl "platform", not that I'm going to write a win32 one lol
 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "common.h"

#include "input.h"
#include "camera.h"
#include "graphics.h"

#define WINDOW_NAME "Metroidvania Jam 15"

SDL_Window* global_window;
local float global_elapsed_time = 0;
bool running = true;

void load_static_resources(void);
void update_render_frame(float dt);

static void initialize(void) {
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    load_static_resources();

    global_window = SDL_CreateWindow(
        WINDOW_NAME,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP
    );
    {
        int screen_dimensions[2];
        SDL_GL_GetDrawableSize(global_window, screen_dimensions, screen_dimensions+1);
        report_screen_dimensions(screen_dimensions);
    }

    graphics_initialize(global_window);
}

static void deinitialize(void) {
    graphics_deinitialize();
    SDL_DestroyWindow(global_window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

static int translate_sdl_scancode(int scancode) {
    static int _sdl_scancode_to_input_keycode_table[255] = {
        // top row
        [SDL_SCANCODE_ESCAPE]    = KEY_ESCAPE,
        [SDL_SCANCODE_1]         = KEY_1,
        [SDL_SCANCODE_2]         = KEY_2,
        [SDL_SCANCODE_3]         = KEY_3,
        [SDL_SCANCODE_4]         = KEY_4,
        [SDL_SCANCODE_5]         = KEY_5,
        [SDL_SCANCODE_6]         = KEY_6,
        [SDL_SCANCODE_7]         = KEY_7,
        [SDL_SCANCODE_8]         = KEY_8,
        [SDL_SCANCODE_9]         = KEY_9,
        [SDL_SCANCODE_0]         = KEY_0,
        [SDL_SCANCODE_MINUS]     = KEY_MINUS,
        [SDL_SCANCODE_EQUALS]    = KEY_EQUALS,
        [SDL_SCANCODE_BACKSPACE] = KEY_BACKSPACE,

        [SDL_SCANCODE_TAB] = KEY_TAB,

        [SDL_SCANCODE_Q]            = KEY_Q,
        [SDL_SCANCODE_W]            = KEY_W,
        [SDL_SCANCODE_E]            = KEY_E,
        [SDL_SCANCODE_R]            = KEY_R,
        [SDL_SCANCODE_T]            = KEY_T,
        [SDL_SCANCODE_Y]            = KEY_Y,
        [SDL_SCANCODE_U]            = KEY_U,
        [SDL_SCANCODE_I]            = KEY_I,
        [SDL_SCANCODE_O]            = KEY_O,
        [SDL_SCANCODE_P]            = KEY_P,
        [SDL_SCANCODE_LEFTBRACKET]  = KEY_LEFT_BRACKET,
        [SDL_SCANCODE_RIGHTBRACKET] = KEY_RIGHT_BRACKET,
        [SDL_SCANCODE_RETURN]       = KEY_RETURN,

        [SDL_SCANCODE_LCTRL] = KEY_CTRL,
        [SDL_SCANCODE_RCTRL] = KEY_CTRL,

        [SDL_SCANCODE_A] = KEY_A,
        [SDL_SCANCODE_S] = KEY_S,
        [SDL_SCANCODE_D] = KEY_D,
        [SDL_SCANCODE_F] = KEY_F,
        [SDL_SCANCODE_G] = KEY_G,
        [SDL_SCANCODE_H] = KEY_H,
        [SDL_SCANCODE_J] = KEY_J,
        [SDL_SCANCODE_K] = KEY_K,
        [SDL_SCANCODE_L] = KEY_L,
        [SDL_SCANCODE_SEMICOLON] = KEY_SEMICOLON,
        [SDL_SCANCODE_APOSTROPHE] = KEY_QUOTE,
        [SDL_SCANCODE_GRAVE] = KEY_BACKQUOTE,

        [SDL_SCANCODE_LSHIFT] = KEY_SHIFT, // left shift
        [SDL_SCANCODE_RSHIFT] = KEY_SHIFT, // left shift
        [SDL_SCANCODE_BACKSLASH] = KEY_BACKSLASH,

        [SDL_SCANCODE_Z]      = KEY_Z,
        [SDL_SCANCODE_X]      = KEY_X,
        [SDL_SCANCODE_C]      = KEY_C,
        [SDL_SCANCODE_V]      = KEY_V,
        [SDL_SCANCODE_B]      = KEY_B,
        [SDL_SCANCODE_N]      = KEY_N,
        [SDL_SCANCODE_M]      = KEY_M,
        [SDL_SCANCODE_COMMA]  = KEY_COMMA,
        [SDL_SCANCODE_PERIOD] = KEY_PERIOD,
        [SDL_SCANCODE_SLASH]  = KEY_FORWARDSLASH,

        [SDL_SCANCODE_PRINTSCREEN] = KEY_PRINTSCREEN,
        [SDL_SCANCODE_LALT]        = KEY_ALT,
        [SDL_SCANCODE_RALT]        = KEY_ALT,
        [SDL_SCANCODE_SPACE]       = KEY_SPACE,

        [SDL_SCANCODE_F1]  = KEY_F1,
        [SDL_SCANCODE_F2]  = KEY_F2,
        [SDL_SCANCODE_F3]  = KEY_F3,
        [SDL_SCANCODE_F4]  = KEY_F4,
        [SDL_SCANCODE_F5]  = KEY_F5,
        [SDL_SCANCODE_F6]  = KEY_F6,
        [SDL_SCANCODE_F7]  = KEY_F7,
        [SDL_SCANCODE_F8]  = KEY_F8,
        [SDL_SCANCODE_F9]  = KEY_F9,
        [SDL_SCANCODE_F10] = KEY_F10,
        [SDL_SCANCODE_F11] = KEY_F11,
        [SDL_SCANCODE_F12] = KEY_F12,

        [SDL_SCANCODE_NUMLOCKCLEAR] = KEY_NUMBER_LOCK,
        [SDL_SCANCODE_SCROLLLOCK]   = KEY_SCROLL_LOCK,


        [SDL_SCANCODE_PAGEUP]   = KEY_PAGEUP,
        [SDL_SCANCODE_HOME]     = KEY_HOME,
        [SDL_SCANCODE_PAGEDOWN] = KEY_PAGEDOWN,
        [SDL_SCANCODE_INSERT]   = KEY_INSERT,
        [SDL_SCANCODE_DELETE]   = KEY_DELETE,
        [SDL_SCANCODE_END]      = KEY_END,

        [SDL_SCANCODE_UP]    = KEY_UP,
        [SDL_SCANCODE_DOWN]  = KEY_DOWN,
        [SDL_SCANCODE_LEFT]  = KEY_LEFT,
        [SDL_SCANCODE_RIGHT] = KEY_RIGHT,
    };

    int mapping = _sdl_scancode_to_input_keycode_table[scancode];
    not_really_important_assert(mapping != KEY_UNKNOWN && "Unbound key?");
    return mapping;
}

static void handle_sdl_event(SDL_Event event) {
    switch (event.type) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        case SDL_WINDOWEVENT_RESIZED: {
            int screen_dimensions[2];
            SDL_GL_GetDrawableSize(global_window, screen_dimensions, screen_dimensions+1);
            report_screen_dimensions(screen_dimensions);
        } break;
        case SDL_QUIT: {
            running = false;
        } break;
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            /* 
               game should be allowed to intercept this itself, since I don't have my own
               input event system, and I don't think I'd want one. I usually seem to only care
               about keystate, so it's no biggie I guess.

               I'll find out later.
             */
            if (event.type == SDL_KEYDOWN) {
                register_key_down(translate_sdl_scancode(event.key.keysym.scancode));
            } else {
                register_key_up(translate_sdl_scancode(event.key.keysym.scancode));
            }
        } break;
        /*TODO(jerry): handle mouse relative mode later*/
        case SDL_MOUSEMOTION: {
            register_mouse_position(event.motion.x, event.motion.y);
        } break;
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
        {
            int button_id;
            switch (event.button.button) {
                case SDL_BUTTON_LEFT: {
                    button_id = MOUSE_BUTTON_LEFT;
                } break;
                case SDL_BUTTON_MIDDLE: {
                    button_id = MOUSE_BUTTON_MIDDLE;
                } break;
                case SDL_BUTTON_RIGHT: {
                    button_id = MOUSE_BUTTON_RIGHT; 
                } break;
            }

            register_mouse_position(event.button.x, event.button.y);
            register_mouse_button(button_id, event.button.state == SDL_PRESSED);
        }
        break;
    }
}

#include "game.c"

int main(int argc, char** argv) {
    unused_expression(argc);
    unused_expression(argv);

    initialize();

    uint32_t frame_start_tick = 0;
    float dt = 0.0f;

    while (running) {
        begin_input_frame();
        frame_start_tick = SDL_GetTicks();
        {
            SDL_Event event;

            while (SDL_PollEvent(&event)) {
                handle_sdl_event(event);
            }
        }


        /*NOTE(jerry): camera should operate on "game"/"graphics" time
         not IRL time ticks*/
        camera_update(dt);
        update_render_frame(dt);

        end_input_frame();
        dt = (float) (SDL_GetTicks() - frame_start_tick) / 1000.0f;
        global_elapsed_time += dt;
    }

    deinitialize();
    return 0;
}
