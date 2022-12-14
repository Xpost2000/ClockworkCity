#define FORFRIENDS_DEMO
/*
 * sdl "platform", not that I'm going to write a win32 one lol
 
 This is a really crufty metroidvania construction set basically.
 Hack in specific game code later.
 
 NOTE(jerry) (3/1/22): You might want to bleach your eyes after this. I'm not even done,
 but even I regret actually not spending time properly engineering lots of crap.
 
 I seemed to justify a lot of the stupid stuff I did as "I won't have enough time", or
 "It's not worth the effort it would take to do this right now since it's a jam".
 
 Some of that might be true, but things like not taking time to make UI library for my
 wysiwyg editor was clearly a mistake.
 
 Animation is also another mistake. The only real way to do it is as a state machine,
 but doing it without any tooling is really really painful. Not sprite animation, like
 the more involved cutscene animation things.
 
 Anything else that did not involve the words animation or UI was fine though.
*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include "common.h"

#include "../glad/glad.h"

#include "input.h"
#include "camera.h"
#include "graphics.h"
#include "audio.h"
#include "memory_arena.h"

#include "binary_serializer.c"

local font_id _console_font;
#define CONSOLE_IMPLEMENTATION
#include "blackiron_console_sfl.h"

#define WINDOW_NAME "ASCENSION"
#define DEFAULT_RESOLUTION_X 640
#define DEFAULT_RESOLUTION_Y 480

SDL_GLContext global_opengl_context;
SDL_Window* global_window;
local float global_elapsed_time = 0;
local SDL_GameController* global_controller_devices[4]      = {};
local SDL_Haptic*         global_haptic_devices[4] = {};
bool running = true;

local void reload_all_graphics_resources();
local void unload_all_graphics_resources(void);
local void load_static_resources(void);
local void load_graphics_resources(void);
local void update_render_frame(float dt);
local void register_console_commands();

void _console_draw_codepoint(void* context, uint32_t codepoint, float x, float y, float r, float g, float b, float a) {
    int height;
    get_text_dimensions(_console_font, "b", 0, &height);
    draw_codepoint(_console_font, x, y-height, codepoint, color4f(r,g,b,a));
}

void _console_draw_rectangle(void* context, float x, float y, float w, float h, float r, float g, float b, float a) {
    draw_filled_rectangle(x, y, w, h, color4f(r, g, b, a));
}

void _console_set_scissor_region(void* context, float x, float y, float w, float h) {
}

struct console_screen_metrics _console_get_screen_metrics(void* context) {
    int dimens[2];
    get_screen_dimensions(dimens, dimens+1);

    return (struct console_screen_metrics) {
        .width = dimens[0],
        .height = dimens[1]
    };
}

struct console_text_metrics _console_measure_text(void* context, char* text_utf8) {
    int dimens[2];
    get_text_dimensions(_console_font, text_utf8, dimens, dimens+1);

    return (struct console_text_metrics) {
        .width = dimens[0],
        .height = dimens[1],
    };
}

local int translate_sdl_scancode(int scancode) {
    local int _sdl_scancode_to_input_keycode_table[255] = {
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
    /* not_really_important_assert(mapping != KEY_UNKNOWN && "Unbound key?"); */
    return mapping;
}

local void close_all_controllers(void) {
    for (unsigned controller_index = 0; controller_index < array_count(global_controller_devices); ++controller_index) {
        if (global_controller_devices[controller_index]) {
            SDL_GameControllerClose(global_controller_devices[controller_index]);
        }
    }
}

local void poll_and_register_controllers(void) {
    for (unsigned controller_index = 0; controller_index < array_count(global_controller_devices); ++controller_index) {
        SDL_GameController* controller = global_controller_devices[controller_index];

        if (controller) {
            if (!SDL_GameControllerGetAttached(controller)) {
                SDL_GameControllerClose(controller);
                global_controller_devices[controller_index] = NULL;
                console_printf("Controller at %d is bad", controller_index);
            }
        } else {
            if (SDL_IsGameController(controller_index)) {
                global_controller_devices[controller_index] = SDL_GameControllerOpen(controller_index);
                console_printf("Opened controller index %d (%s)\n", controller_index, SDL_GameControllerNameForIndex(controller_index));
            }
        }
    }
}

local void update_all_controller_inputs(void) {
    for (unsigned controller_index = 0; controller_index < array_count(global_controller_devices); ++controller_index) {
        SDL_GameController* controller = global_controller_devices[controller_index];

        if (!controller) {
            continue;
        }

        struct game_controller* gamepad = get_gamepad(controller_index);
        gamepad->_internal_controller_handle = controller;

        {
            gamepad->triggers.left  = (float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / (32767.0f);
            gamepad->triggers.right = (float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / (32767.0f);
        }

        {
            gamepad->buttons[BUTTON_RB] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
            gamepad->buttons[BUTTON_LB] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        }

        {
            gamepad->buttons[BUTTON_A] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
            gamepad->buttons[BUTTON_B] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B);
            gamepad->buttons[BUTTON_X] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X);
            gamepad->buttons[BUTTON_Y] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y);
        }

        {
            gamepad->buttons[DPAD_UP]    = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
            gamepad->buttons[DPAD_DOWN]  = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            gamepad->buttons[DPAD_LEFT]  = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT); 
            gamepad->buttons[DPAD_RIGHT] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT); 
        }

        {
            gamepad->buttons[BUTTON_RS] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
            gamepad->buttons[BUTTON_LS]  = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSTICK);
        }

        {
            gamepad->buttons[BUTTON_START] = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START);
            gamepad->buttons[BUTTON_BACK]  = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK);
        }

        {
            {
                float axis_x = (float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) / (32767.0f);
                float axis_y = (float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) / (32767.0f);

                const float DEADZONE_X = 0.01;
                const float DEADZONE_Y = 0.03;
                if (fabs(axis_x) < DEADZONE_X) axis_x = 0;
                if (fabs(axis_y) < DEADZONE_Y) axis_y = 0;

                gamepad->left_stick.axes[0] = axis_x;
                gamepad->left_stick.axes[1] = axis_y;
            }

            {
                float axis_x = (float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) / (32767.0f);
                float axis_y = (float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) / (32767.0f);

                const float DEADZONE_X = 0.01;
                const float DEADZONE_Y = 0.03;
                if (fabs(axis_x) < DEADZONE_X) axis_x = 0;
                if (fabs(axis_y) < DEADZONE_Y) axis_y = 0;
                
                gamepad->right_stick.axes[0] = axis_x;
                gamepad->right_stick.axes[1] = axis_y;
            }
        }
    }
}

void controller_rumble(struct game_controller* controller, float x_magnitude, float y_magnitude, uint32_t ms) {
    SDL_GameController* sdl_controller = controller->_internal_controller_handle;
    x_magnitude = clampf(x_magnitude, 0, 1);
    y_magnitude = clampf(y_magnitude, 0, 1);
    SDL_GameControllerRumble(sdl_controller, (0xFFFF * x_magnitude), (0xFFFF * y_magnitude), ms);
}

local void set_window_transparency(float transparency) {
    SDL_SetWindowOpacity(global_window, transparency);
}

local void initialize(void) {
    srand(time(0));

    SDL_Init(SDL_INIT_EVERYTHING);
    Mix_Init(MIX_INIT_OGG);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    global_window = SDL_CreateWindow(
        WINDOW_NAME,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DEFAULT_RESOLUTION_X, DEFAULT_RESOLUTION_Y,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    {
        int screen_dimensions[2];
        SDL_GL_GetDrawableSize(global_window, screen_dimensions, screen_dimensions+1);
        report_screen_dimensions(screen_dimensions);
    }

#if 0
    {
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
        global_opengl_context = SDL_GL_CreateContext(global_window);
        assert(gladLoadGLLoader(SDL_GL_GetProcAddress) && "OpenGL functions not loadable?");
    }
#endif

    graphics_initialize(global_window);
    audio_initialize();

    load_graphics_resources();
    console_initialize(
        (struct console_render_procedures) {
            .context            = NULL,
            .draw_codepoint     = _console_draw_codepoint,
            .draw_quad          = _console_draw_rectangle,
            .set_scissor_region = _console_set_scissor_region,
            .get_screen_metrics = _console_get_screen_metrics,
            .measure_text       = _console_measure_text,
        });
    console_printf("Welcome to xvania\na C metroidvania game engine thing for\nMetroidvania Jam 15.\n");
    register_console_commands();
    load_static_resources();
    SDL_ShowWindow(global_window);
}

local void deinitialize(void) {
    close_all_controllers();
    audio_deinitialize();
    graphics_deinitialize();
    SDL_GL_DeleteContext(global_opengl_context);
    SDL_DestroyWindow(global_window);
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

#include "game.c"

local void handle_sdl_event(SDL_Event event) {
    switch (event.type) {
        case SDL_WINDOWEVENT: {
            switch (event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_RESIZED: {
                    int screen_dimensions[2];
                    SDL_GL_GetDrawableSize(global_window, screen_dimensions, screen_dimensions+1);
                    report_screen_dimensions(screen_dimensions);
                    reload_all_graphics_resources();
                } break;
            }
        } break;
        case SDL_QUIT: {
            if (mode == GAME_MODE_PLAYING) {
                game_state->menu_mode = GAMEPLAY_UI_MAINMENU;
                /*TODO(jerry): reduce code duplication*/
                game_state->menu_transition_state = GAMEPLAY_UI_TRANSITION_TO_QUIT;
                game_state->quit_transition_timer[0] = QUIT_FADE_TIMER;
                game_state->quit_transition_timer[1] = QUIT_FADE_TIMER2;
            } else {
                /* 
                   I need a proper transition system. I wanna do it for all modes but
                   eh...
                */
                running = false;
            }
        } break;
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            /* 
               game should be allowed to intercept this itself, since I don't have my own
               input event system, and I don't think I'd want one. I usually seem to only care
               about keystate, so it's no biggie I guess.

               I'll find out later.
             */
            if (console_active()) {
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
                        console_send_character('\b');
                    } else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                        console_move_backward_character();
                    } else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                        console_move_forward_character();
                    } else if (event.key.keysym.scancode == SDL_SCANCODE_F1) {
                        console_kill_line_from_current_position();
                    } else if (event.key.keysym.scancode == SDL_SCANCODE_UP) {
                        /* console_scroll_by(-1); */
                    } else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
                        /* console_scroll_by(1); */
                    }

                    if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                        console_submit();
                    }
                }
            } else {
                if (event.type == SDL_KEYDOWN) {
                    register_key_down(translate_sdl_scancode(event.key.keysym.scancode));
                } else {
                    register_key_up(translate_sdl_scancode(event.key.keysym.scancode));
                }
            }
        } break;
        /*TODO(jerry): handle mouse relative mode later*/
        case SDL_MOUSEMOTION: {
            register_mouse_position(event.motion.x, event.motion.y);
        } break;
        case SDL_MOUSEWHEEL: {
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
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_CONTROLLERDEVICEADDED: {
            poll_and_register_controllers();
        } break;
        case SDL_TEXTINPUT: {
            char* input_text = event.text.text;
            size_t input_length = strlen(input_text);
#if 1
            send_text_input(input_text, input_length);
#endif
            console_send_character(input_text[0]);
        } break;
    }
}

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

            update_all_controller_inputs();
        }

        /*NOTE(jerry): camera should operate on "game"/"graphics" time
         not IRL time ticks*/
        update_render_frame(dt);

        begin_graphics_frame(NULL);{
            console_frame(dt);
        } end_graphics_frame();

        present_graphics_frame();
        end_input_frame();
        dt = (float) (SDL_GetTicks() - frame_start_tick) / 1000.0f;
        global_elapsed_time += dt;
    }

    deinitialize();
    return 0;
}

#include "console_commands.c"
