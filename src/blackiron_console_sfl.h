/*
  Blackiron Quake Console SFL

  - Commands
  - Variables
  - Large Scrollback
  - No dynamic allocation
  - Basic theming
  - Kind of easy integration
  - Tab completion
  - basic emacs keybindings
  
  NOTE(jerry):
  Oh wait. I just realized the theming never got backported from the engine,
  since this is a slightly earlier version of the console... Which explains
  why there was some small weird bugs.
  
  Ugh... I'll backport it later but I'll hardcode some new constants.
 */
#ifndef CONSOLE_H
#define CONSOLE_H
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#define BIT(x) (1 << x)
static bool is_whitespace_character(char character) {
    return (character == '\n' ||
            character == ' '  ||
            character == '\r' ||
            character == '\t');
}

static bool is_lowercase_character(char character) {
    return (character >= 'a') && (character <= 'z');
}

static bool is_uppercase_character(char character) {
    return (character >= 'A') && (character <= 'Z');
}

static bool is_alphabetic_character(char character) {
    return (is_lowercase_character(character) ||
            is_uppercase_character(character));
}

static bool is_numeric_character(char character) {
    return (character >= '0') && (character <= '9');
}

static bool is_ascii_character(char character) {
    return (character >= 0 && character <= 127);
}

static bool is_human_readable_ascii_character(char character) {
    return (character >= 32 && character <= 127);
}
// TODO(jerry):
// if I wanted this to be a little saner we could hook into the subsystem to do stuff.

// This is the shared code for the display and subsystem/variable part

// This is a Quake style console. Everyone loves these!
// There is only one global console.

// no stdlib dependencies unless you don't define these.
#ifndef _internal_console_atof
#include <stdlib.h>
#define _internal_console_atof atof
#endif
#ifndef _internal_console_atoi
#include <stdlib.h>
#define _internal_console_atoi atoi
#endif
#ifndef _internal_console_vsnprintf
#include <stdio.h>
#define _internal_console_vsnprintf vsnprintf
#endif
#ifndef _internal_console_strncmp
#include <string.h>
#define _internal_console_strncmp strncmp
#endif
#ifndef _internal_console_strlen
#include <string.h>
#define _internal_console_strlen strlen
#endif

// The ring buffer implementation does not print whatever is at the head
// as it is being used for writing purposes. So an extra byte is added.
#define CONSOLE_SCROLLBACK_BUFFER_SIZE (16384 + 1)
#define CONSOLE_INPUT_LINE_BUFFER_SIZE (512)

#define CONSOLE_CURSOR_BLINK_TIMER_MAX (0.45)
#define CONSOLE_SCREEN_PORTION (0.45)
#define CONSOLE_INPUT_HISTORY_MAX_ENTRIES (8)
#define CONSOLE_DROPSHADOW_X_OFFSET (2.3)
#define CONSOLE_DROPSHADOW_Y_OFFSET (1)

#define CONSOLE_SYSTEM_VARIABLE_STRING_SIZE (64)
#define CONSOLE_SYSTEM_COMMANDS_MAX_ARGUMENTS (32)

#define template_fn_macro_concatenate(a, b) a##_##b
#define template_fn_name(name, type) template_fn_macro_concatenate(name, type)
#define template_fn(name) template_fn_name(name, type)
#define stringify(x) #x
#define macro_stringify(x) stringify(x)
#define tokenize(x) x
#define macro_tokenize(x) tokenize(x)

// Here are some macros to make life more pleasant,
// you still have to register them manually though.
// It will produce variables as cmd_NAME
#define Define_Console_Command(Name) \
    void template_fn_macro_concatenate(_console_command_generated, Name)(uint8_t argument_count, struct console_system_variant* parameters); \
    static struct console_system_command template_fn_macro_concatenate(cmd, Name) = { .name = stringify(Name), .procedure = template_fn_macro_concatenate(_console_command_generated, Name) }; \
    void template_fn_macro_concatenate(_console_command_generated, Name)(uint8_t argument_count, struct console_system_variant* parameters)

// initialize elsewhere because of the way strings work.
// As these strings are writable, and not read-only.
#define Define_Console_Variable(Name, Type)\
    static struct console_system_variable template_fn_macro_concatenate(convar, Name) = { stringify(Name), template_fn_macro_concatenate(CONSOLE_VARIABLE_TYPE, Type) }


enum console_system_variable_type {
    CONSOLE_VARIABLE_TYPE_NUMBER  = BIT(0),
    CONSOLE_VARIABLE_TYPE_STRING  = BIT(1),
    CONSOLE_VARIABLE_TYPE_BOOLEAN = BIT(2),
    CONSOLE_VARIABLE_TYPE_COUNT,
};
enum console_system_number_type {
    // This is setup like this so you can just do
    /*
      console_system_variable sv_cheats = { 
            .name = "sv_cheats",
            .type = CONSOLE_VARIABLE_TYPE_NUMBER | CONSOLE_VARIABLE_TYPE_NUMBER_INTEGER,
            .integer = 0
      };
     */
    CONSOLE_VARIABLE_TYPE_NUMBER_REAL    = BIT(1),
    CONSOLE_VARIABLE_TYPE_NUMBER_INTEGER = BIT(2),
    CONSOLE_VARIABLE_TYPE_NUMBER_COUNT,
};

struct console_system_variant {
    uint8_t type;
    union {
        int32_t integer;
        float   real;
        char    string[CONSOLE_SYSTEM_VARIABLE_STRING_SIZE];
        bool    boolean;
    };
};
struct console_system_variable {
    char* name;
    uint8_t type;
    union {
        int32_t integer;
        float   real;
        char    string[CONSOLE_SYSTEM_VARIABLE_STRING_SIZE];
        bool    boolean;
    };

    struct console_system_variable* next;
};

typedef void (*console_system_command_procedure)(uint8_t argument_count, struct console_system_variant* parameters);
// NOTE(jerry): remove this
// if I really wanted to typecheck... The functions could do that themselves.
typedef struct console_system_command_arguments {
    bool    typechecking;
    uint8_t types[CONSOLE_SYSTEM_COMMANDS_MAX_ARGUMENTS];
} console_system_command_arguments;

// these are "type-checked."
struct console_system_command {
    char* name;
    console_system_command_arguments argument_definitions;
    console_system_command_procedure procedure;
    struct console_system_command* next;
};

// This is only here so I can reuse this console outside of this
// project, since I'd honestly rather not rewrite this all the time lol.

// Now this assumes that the font is baked in the procedures it uses
// to draw. IE: font configuration is left up to the procedures.
struct console_screen_metrics {
    float width;
    float height;
};
struct console_text_metrics {
    float width;
    float height;
};
struct console_render_procedures {
    // implicit context parameter
    // you might want.
    void* context;

    // Draw unicode rune/codepoint/ whatever you call it.
    // there is no need for draw_text. An internal draw text is implemented based off of draw_codepoint.
    void                          (*draw_codepoint)(void* context, uint32_t codepoint, float x, float y, float r, float g, float b, float a);
    void                          (*draw_quad)(void* context, float x, float y, float w, float h, float r, float g, float b, float a);

    // This scissor region assumes the top left is the origin
    /*
      +-----------------------------> X
      |
      |
      |
      |
      |
      |
      v

      Y
     */
    void                          (*set_scissor_region)(void* context, float x, float y, float w, float h);

    // Used to prune for resolution and setting console height and width.
    struct console_screen_metrics (*get_screen_metrics)(void* context);

    // used for line wrapping.
    struct console_text_metrics   (*measure_text)(void* context, char* text_utf8);
};

void console_initialize(struct console_render_procedures render_procedures);
void console_printf(char* text, ...);
void console_clear(void);

// ASCII only console.
void console_send_character(char character);
void console_submit(void);

void console_display(void);
void console_allow_submission(void);

// up to the game to respect this.
bool console_active(void);
void console_scroll_by(float amount);

static char* console_system_variable_type_as_string(uint8_t type);
static char* console_system_number_type_as_string(uint8_t type);

/*
  CONSOLE ACTIONS
 */
void console_toggle(void);
void console_move_forward_character(void);
void console_move_backward_character(void);
void console_move_forward_word(void);
void console_move_backward_word(void);
void console_start_of_line(void);
void console_end_of_line(void);
void console_kill_line_from_current_position(void);
void console_previous_history_entry(void);
void console_next_history_entry(void);
/*
  This is my solution to the insanity of providing keybindings...
  I basically don't?
  
  I give you actions, and you do it yourself. The only thing I do is open the console for you
  with tilde which might change to also be an action.
 */

// These iterators are identical.
typedef struct console_system_variable_iterator {
    struct console_system_variable* current;
} console_system_variable_iterator;
typedef struct console_system_command_iterator {
    struct console_system_command* current;
} console_system_command_iterator;

console_system_variable_iterator console_system_begin_iterating_variables(void);
bool                             console_system_variable_iterator_finished(console_system_variable_iterator* iterator);
struct console_system_variable*  console_system_variable_iterator_advance(console_system_variable_iterator* iterator);

console_system_command_iterator console_system_begin_iterating_commands(void);
bool                            console_system_command_iterator_finished(console_system_command_iterator* iterator);
struct console_system_command*  console_system_command_iterator_advance(console_system_command_iterator* iterator);

void                            console_system_register_command(struct console_system_command* command);
void                            console_system_register_variable(struct console_system_variable* variable);
struct console_system_variable* console_system_find_variable(char* name);


#endif

#ifdef CONSOLE_IMPLEMENTATION
// This was only designed with liberation mono in mind, so try to avoid changing the font for
// your own sanity.

// Does not do scrollback, but there is a ring buffer so it does actually scrollback.
struct _internal_console_utf8_decode_result {
    uint32_t codepoint;
    size_t   decoded_bytes;
    bool     error;
};

typedef struct decode__internal_console_utf8_iterator {
    char* buffer;
    size_t buffer_length;
    size_t decoded_count;
    uint32_t codepoint;
} decode__internal_console_utf8_iterator;
decode__internal_console_utf8_iterator decode__internal_console_utf8_from(char* buffer, size_t buffer_length);
decode__internal_console_utf8_iterator decode__internal_console_utf8_from_cstring(char* buffer);
bool                 decode__internal_console_utf8_iterator_valid(decode__internal_console_utf8_iterator* iterator);
uint32_t             decode__internal_console_utf8_iterator_advance(decode__internal_console_utf8_iterator* iterator);

struct _internal_console_utf8_decode_result _internal_console_utf8_decode_single_codepoint(char* buffer);

// DIRECT COPY AND PASTE FROM public/word_iterator.h and .c
// this is so this can just be used in one file! Cause I don't feel like making this not work without a file!
typedef struct console_word_iterator {
    char* buffer;
    size_t buffer_length;
    size_t read_cursor;

    bool done;

    char* word;
    size_t word_length;
} console_word_iterator;

console_word_iterator console_word_iterator_begin_iterating_from(char* buffer, size_t buffer_length) {
void console_word_iterator_advance(console_word_iterator* iterator);
    console_word_iterator result =  (console_word_iterator) {
        .buffer        = buffer,
        .buffer_length = buffer_length,
        .read_cursor   = 0,
        .done          = false
    };

    // Same reasoning as the UTF8 iterator.
    console_word_iterator_advance(&result);
    return result;
}

console_word_iterator console_word_iterator_begin_iterating_from_cstring(char* buffer) {
    size_t string_length = 0;
    char* cursor = buffer;

    while (*(cursor++)) {}
    string_length = cursor - buffer;

    return console_word_iterator_begin_iterating_from(buffer, string_length);
}

bool console_word_iterator_finished(console_word_iterator* iterator) {
    // Technically a index comparison should work, but the iterator advancer... Is technically
    // what actually determines if we're done or not lol.
    return iterator->done;
}

void console_word_iterator_advance(console_word_iterator* iterator) {
    // This is done here to make sure the for loop will work as expected.
    if (iterator->read_cursor > iterator->buffer_length) {
        iterator->done = true;
        return;
    }

    char* start_of_word = iterator->buffer + iterator->read_cursor;
    char* end_of_word   = start_of_word;

    bool found_string = false;

    while (iterator->read_cursor < iterator->buffer_length) {
        char current = iterator->buffer[iterator->read_cursor];

        if (!found_string && current == '\"') {
            found_string = true;
            start_of_word++;
            current = iterator->buffer[++iterator->read_cursor];
        }

        if (!found_string) {
            if (is_whitespace_character(current)) {
                break;
            }
        } else {
            if (current == '\"') {
                break;
            }
        }

        iterator->read_cursor++;
    }

    // This actually almost looks like a mistake, because it looks like a buffer overrun.
    // I mean, it actually IS a buffer overrun, but this is still fine since I don't read or write
    // and it does give an honest length reading lol.

    // The buffer overrun never happens within the loop as far as I'm aware so there's no issue.
    end_of_word = iterator->buffer + iterator->read_cursor++;

    size_t word_length    = end_of_word - start_of_word;
    iterator->word        = start_of_word;
    iterator->word_length = word_length;

    // NOTE(jerry):
    // This is an incredibly lazy solution.
    // To solve accidently tokenizing whitespace.

    // This replicates the same solution I would have done, which is just eat whitespace, but this is way
    // shorter... Also I hope this is actually tail recursive like I think it is.
    if (end_of_word != start_of_word) {
        return;
    }

    console_word_iterator_advance(iterator);   
}

size_t console_word_iterator_length(console_word_iterator* iterator) {
    console_word_iterator clone = *iterator;
    size_t result = 0;
    while (!console_word_iterator_finished(&clone)) {
        result++;
        console_word_iterator_advance(&clone);
    }
    return result;
}

// TODO(jerry):
// make this configurable to the outside world.
// I mean I'm releasing source... So I mean you kind of already can configure
// this anytime you want... So eh?

#ifdef _MSC_VER
#define Force_inline __forceinline
#else
#define Force_inline __attribute__((always_inline))
#endif

enum console_animation_state {
    CONSOLE_ANIMATION_STATE_DEFAULT,
    CONSOLE_ANIMATION_STATE_OPENING,
    CONSOLE_ANIMATION_STATE_CLOSING,
};

// duplicated from public/common.h
Force_inline
static void _internal_console_decode_rgba_from_uint32(uint32_t encoded_rgba, float* rgba) {
    rgba[0] = (float)((encoded_rgba & 0xFF000000) >> 24) / 255.0f;
    rgba[1] = (float)((encoded_rgba & 0x00FF0000) >> 16) / 255.0f;
    rgba[2] = (float)((encoded_rgba & 0x0000FF00) >> 8)  / 255.0f;
    rgba[3] = (float)((encoded_rgba & 0x000000FF))       / 255.0f;
}

struct console {
    char   scrollback_buffer[CONSOLE_SCROLLBACK_BUFFER_SIZE];
    size_t scrollback_buffer_read_cursor;
    size_t scrollback_buffer_write_cursor;

    // since this should be monospaced this is completely fine to use
    struct console_text_metrics      character_metrics;
    struct console_render_procedures render_procedures;

    float scroll_y;
    
    float width;
    float height;

    uint32_t lines_per_page;

    uint32_t input_line_count;
    uint32_t input_line_write_cursor_location;
    char     input_line[CONSOLE_INPUT_LINE_BUFFER_SIZE];

    int8_t selected_history_buffer_index; // read index
    int8_t history_buffer_index;
    uint8_t history_buffer_count;
    struct {
        size_t count;
        char   buffer[CONSOLE_INPUT_LINE_BUFFER_SIZE];
    } input_history_buffers[CONSOLE_INPUT_HISTORY_MAX_ENTRIES];

    bool shown;
    float slide_timer;

    float cursor_blink_timer;
    bool  cursor_show;

    uint8_t animation_state;
};

struct console_system {
    uint64_t variable_count;
    struct {
        struct console_system_variable* head;
        struct console_system_variable* tail;
        struct console_system_variable null;
    } variable_list;

    uint64_t command_count;
    struct {
        struct console_system_command* head;
        struct console_system_command* tail;
        struct console_system_command null;
    } command_list;
};

static struct console_system _global_console_system = {};
static struct console        _global_console        = {};

void console_clear(void) {
    memset(_global_console.scrollback_buffer, 0, CONSOLE_SCROLLBACK_BUFFER_SIZE);
}

static inline void _console_update_resolution(void) {
    struct console_screen_metrics screen_dimensions = _global_console.render_procedures.get_screen_metrics(_global_console.render_procedures.context);

    _global_console.width          = screen_dimensions.width;
    _global_console.height         = screen_dimensions.height * CONSOLE_SCREEN_PORTION;
    _global_console.lines_per_page = _global_console.height / (_global_console.character_metrics.height);
}

void console_initialize(struct console_render_procedures render_procedures) {
    _global_console.render_procedures = render_procedures;
    
    // hardcoded path.
    // please make sure this is a monospaced font.
    /* _global_console.font              = graphics->load_font_from_file(CONSOLE_DEFAULT_FONT_PATH, CONSOLE_DEFAULT_FONT_SIZE); */
    _global_console.character_metrics = render_procedures.measure_text(_global_console.render_procedures.context, "#");

    _global_console_system.variable_list.head = &_global_console_system.variable_list.null;
    _global_console_system.variable_list.tail = &_global_console_system.variable_list.null;

    _global_console_system.command_list.head = &_global_console_system.command_list.null;
    _global_console_system.command_list.tail = &_global_console_system.command_list.null;

    _console_update_resolution();
    console_clear();
}

void console_printf(char* text, ...) {
    static char _temporary_format_buffer[2048] = {};
    memset(_temporary_format_buffer, 0, 2048);

    va_list variadic_arguments;
    va_start(variadic_arguments, text);

    size_t written = _internal_console_vsnprintf(_temporary_format_buffer, 2048, text, variadic_arguments);
    va_end(variadic_arguments);

    // ring buffer writing
    for (size_t character_index = 0; character_index < written; ++character_index) {
        _global_console.scrollback_buffer[_global_console.scrollback_buffer_write_cursor++] = _temporary_format_buffer[character_index];

        // Ring buffer wrap around writing.
        if (_global_console.scrollback_buffer_write_cursor >= CONSOLE_SCROLLBACK_BUFFER_SIZE) {
            _global_console.scrollback_buffer_write_cursor = 0;
            _global_console.scrollback_buffer_read_cursor  += 1;
        }

        // We have overstepped capacity so we will bump the read cursor forward which will "discard" the last
        // character. (This is basically just a queue.)
        if (_global_console.scrollback_buffer_write_cursor == _global_console.scrollback_buffer_read_cursor) {
            _global_console.scrollback_buffer_read_cursor += 1;
        }

        if (_global_console.scrollback_buffer_read_cursor >= CONSOLE_SCROLLBACK_BUFFER_SIZE) {
            _global_console.scrollback_buffer_read_cursor = 0;
        }
    }
}

// This does not need to change for the ring buffer implementation.
// This will always be fine.
static inline uint32_t _console_count_lines(void) {
    uint32_t counted_lines = 0;
    float glyph_width  = _global_console.character_metrics.width;
    float console_width  = _global_console.width;

    char* scrollback_buffer = _global_console.scrollback_buffer;
    float x_cursor = 0;

    for (size_t character_index = 0; character_index < CONSOLE_SCROLLBACK_BUFFER_SIZE; ++character_index) {
        bool should_line_break = false;

        if (scrollback_buffer[character_index] > 0) {
            x_cursor += glyph_width;

            if (x_cursor >= console_width) {
                should_line_break = true;
            }

            if (scrollback_buffer[character_index] == '\n' || scrollback_buffer[character_index] == '\r') {
                should_line_break = true;
            }

            if (should_line_break) {
                x_cursor = 0;
                counted_lines++;
            }
        }
    }

    return counted_lines;
}

void console_scroll_by(float amount) {
    float glyph_height = _global_console.character_metrics.height;

    uint32_t line_count     = _console_count_lines();
    uint32_t lines_per_page = _global_console.lines_per_page;
    float    present_pages  = (float)line_count / lines_per_page;

    float scroll_displacement   = (_global_console.scroll_y + amount);
    float scrolled_pages_height = scroll_displacement + lines_per_page * glyph_height;

    _global_console.scroll_y += amount;
}

//please inline
Force_inline
static void _console_display_codepoint(uint32_t codepoint, float* x_cursor, float* y_cursor, float glyph_width, float glyph_height, float console_width, float slide_offset_y) {
    bool should_line_break = false;

    if (codepoint != ' ' && is_whitespace_character(codepoint)) {
        switch (codepoint) {
            case '\r':
            case '\n': {
                should_line_break = true;
            } break;
        }
    } else if (codepoint > 0) {
        _global_console.render_procedures.draw_codepoint(_global_console.render_procedures.context, codepoint, *x_cursor+CONSOLE_DROPSHADOW_X_OFFSET, *y_cursor+glyph_height+slide_offset_y+CONSOLE_DROPSHADOW_Y_OFFSET, 0.0, 0.0, 0.0, 0.8);
        _global_console.render_procedures.draw_codepoint(_global_console.render_procedures.context, codepoint, *x_cursor, *y_cursor+glyph_height+slide_offset_y, 1.0, 1.0, 1.0, 1.0);
        *x_cursor += (glyph_width);
    }

    {
        float next_glyph_start = *x_cursor + glyph_width;

        if ((next_glyph_start) >= console_width) {
            should_line_break = true;
        }
    }

    if (should_line_break) {
        *x_cursor = 0;
        *y_cursor += (glyph_height);
    }
}

static float _internal_console_linear_interpolate_float(float a, float b, float t) {
    if (t <= 0.0f) {
        t = 0.0f;
    }

    if (t >= 1.0f) {
        t = 1.0f;
    }

    return (a) * (t - 1.0) + (b * t);
}

void console_display(void) {
    _console_update_resolution();
    // To avoid the complexities of supporting virtual resolution,
    // the console will be drawn in real screen resolutions always.
    // cool sliding visual effects I guess
    float slide_offset_y = 0;
    {
        if (_global_console.animation_state == CONSOLE_ANIMATION_STATE_OPENING) {
            slide_offset_y = _internal_console_linear_interpolate_float(_global_console.height, 0, _global_console.slide_timer);
        } else if (_global_console.animation_state == CONSOLE_ANIMATION_STATE_CLOSING) {
            slide_offset_y = _internal_console_linear_interpolate_float(0, -_global_console.height, _global_console.slide_timer);
        }
    }

    // clamping the console to console height.
    float    glyph_height = _global_console.character_metrics.height;
    float    glyph_width  = _global_console.character_metrics.width;
    uint32_t line_count   = _console_count_lines();

    // Reserve 1 line for the input line.
    uint32_t lines_per_page = _global_console.lines_per_page-1;

    // clamp the console scroller.
    float max_scroll_y;
    {
        {
            if (line_count < lines_per_page) {
                line_count = lines_per_page;
            }

            max_scroll_y = (line_count-lines_per_page) * glyph_height;
        }

        if (_global_console.scroll_y < 0) {
            _global_console.scroll_y = 0;
        } else if (_global_console.scroll_y + lines_per_page * glyph_height > line_count * glyph_height) {
            _global_console.scroll_y = max_scroll_y;
        }
    }

    float console_width  = _global_console.width;
    float console_height = _global_console.height;
    float input_line_y   = _global_console.height - glyph_height*1.1;

    {
        float rgba[4];
        /* _internal_console_decode_rgba_from_uint32(0x008C77FF, rgba); */
        _internal_console_decode_rgba_from_uint32(0xF50F22FF, rgba);
        
        _global_console.render_procedures.draw_quad(_global_console.render_procedures.context, 0, slide_offset_y, console_width, console_height, rgba[0], rgba[1], rgba[2], 0.93);
    }
    {
        float rgba[4];
        /* _internal_console_decode_rgba_from_uint32(0x005A44FF, rgba); */
        _internal_console_decode_rgba_from_uint32(0x752A30FF, rgba);
        _global_console.render_procedures.draw_quad(_global_console.render_procedures.context, 0, input_line_y + slide_offset_y, _global_console.width, glyph_height*1.1, rgba[0], rgba[1], rgba[2], 1.0);
    }

    float x_cursor = 0;
    float y_cursor = -(max_scroll_y);

    _global_console.render_procedures.set_scissor_region(_global_console.render_procedures.context, 0, 0, console_width, input_line_y);

    // ring buffer code makes this a little nastier to do.
    if (_global_console.scrollback_buffer_write_cursor > _global_console.scrollback_buffer_read_cursor) {
        for (decode__internal_console_utf8_iterator iterator = decode__internal_console_utf8_from(_global_console.scrollback_buffer, CONSOLE_SCROLLBACK_BUFFER_SIZE);
             decode__internal_console_utf8_iterator_valid(&iterator);
             decode__internal_console_utf8_iterator_advance(&iterator)) {
            _console_display_codepoint(iterator.codepoint, &x_cursor, &y_cursor, glyph_width, glyph_height, console_width, slide_offset_y);
        }
    } else if (_global_console.scrollback_buffer_write_cursor < _global_console.scrollback_buffer_read_cursor) {
        // nasty wrap around handling...

        // first half (tail -> end)
        for (decode__internal_console_utf8_iterator iterator = decode__internal_console_utf8_from(_global_console.scrollback_buffer + _global_console.scrollback_buffer_read_cursor, CONSOLE_SCROLLBACK_BUFFER_SIZE - _global_console.scrollback_buffer_read_cursor);
             decode__internal_console_utf8_iterator_valid(&iterator);
             decode__internal_console_utf8_iterator_advance(&iterator)) {
            _console_display_codepoint(iterator.codepoint, &x_cursor, &y_cursor, glyph_width, glyph_height, console_width, slide_offset_y);
        }

        // second half (begin -> tail)
        for (decode__internal_console_utf8_iterator iterator = decode__internal_console_utf8_from(_global_console.scrollback_buffer, _global_console.scrollback_buffer_write_cursor);
             decode__internal_console_utf8_iterator_valid(&iterator);
             decode__internal_console_utf8_iterator_advance(&iterator)) {
            _console_display_codepoint(iterator.codepoint, &x_cursor, &y_cursor, glyph_width, glyph_height, console_width, slide_offset_y);
        }
    }

    {
        {
            float x_cursor  = 0;
            float y_cursor  = input_line_y;

            for (decode__internal_console_utf8_iterator iterator = decode__internal_console_utf8_from(_global_console.input_line, _global_console.input_line_count+1);
                 decode__internal_console_utf8_iterator_valid(&iterator);
                 decode__internal_console_utf8_iterator_advance(&iterator)) {
                _console_display_codepoint(iterator.codepoint, &x_cursor, &y_cursor, glyph_width, glyph_height, console_width, slide_offset_y);
            }
        }

        if (_global_console.cursor_show) {
            _global_console.render_procedures.draw_quad(_global_console.render_procedures.context, _global_console.input_line_write_cursor_location * glyph_width, input_line_y + slide_offset_y, glyph_width, glyph_height, 0.0, 1.0, 0.0, 0.8);
        }
    }
}

void console_system_execute(char* line, size_t line_size);
void console_submit(void) {
    if (_global_console.input_line_count > 0) {
        console_system_execute(_global_console.input_line, _global_console.input_line_count);

        memcpy(_global_console.input_history_buffers[_global_console.history_buffer_index].buffer, _global_console.input_line, CONSOLE_INPUT_LINE_BUFFER_SIZE);
        _global_console.input_history_buffers[_global_console.history_buffer_index++].count = _global_console.input_line_count;
        _global_console.history_buffer_index &= (CONSOLE_INPUT_HISTORY_MAX_ENTRIES-1);

        if (_global_console.history_buffer_count < CONSOLE_INPUT_HISTORY_MAX_ENTRIES) {
            _global_console.history_buffer_count++;
        }

        _global_console.input_line_count = 0;
        memset(_global_console.input_line, 0, CONSOLE_INPUT_LINE_BUFFER_SIZE);
    }
}

bool console_active(void) {
    return _global_console.shown;
}

void console_toggle(void) {
    // only allow actions when the animation is finished.
    // Or we are not animating.
    if (_global_console.slide_timer == 0.0 || _global_console.animation_state == CONSOLE_ANIMATION_STATE_DEFAULT) {
        if (_global_console.shown == false) {
            _global_console.shown = true;
            _global_console.animation_state = CONSOLE_ANIMATION_STATE_OPENING;
        } else if (_global_console.shown == true) {
            _global_console.shown = false;
            _global_console.animation_state = CONSOLE_ANIMATION_STATE_CLOSING;
        }
            
        _global_console.slide_timer = 0;
    }
}

void console_move_forward_character(void) {
    if (_global_console.input_line_write_cursor_location+1 <= _global_console.input_line_count) {
        _global_console.input_line_write_cursor_location++;
    }
}

void console_move_backward_character(void) {
    if (_global_console.input_line_write_cursor_location > 0) {
        _global_console.input_line_write_cursor_location--;
    }
}


void console_move_forward_word(void) {
    // First finish the current word, by running into whitespace.
    while (_global_console.input_line_write_cursor_location < _global_console.input_line_count && !is_whitespace_character(_global_console.input_line[_global_console.input_line_write_cursor_location])) {
        console_move_forward_character();
    }

    // Then find the next word by looking for the first next non-whitespace character.
    while (_global_console.input_line_write_cursor_location < _global_console.input_line_count && is_whitespace_character(_global_console.input_line[_global_console.input_line_write_cursor_location])) {
        console_move_forward_character();
    }
}

void console_move_backward_word(void) {
    // same as the above. Just in reverse.
    while (_global_console.input_line_write_cursor_location > 0 && !is_whitespace_character(_global_console.input_line[_global_console.input_line_write_cursor_location])) {
        console_move_backward_character();
    }

    while (_global_console.input_line_write_cursor_location > 0 && is_whitespace_character(_global_console.input_line[_global_console.input_line_write_cursor_location])) {
        console_move_backward_character();
    }
}

void console_start_of_line(void) {
    _global_console.input_line_write_cursor_location = 0;
}

void console_end_of_line(void) {
    _global_console.input_line_write_cursor_location = _global_console.input_line_count;
}

void console_kill_line_from_current_position(void) {
    if (_global_console.input_line_write_cursor_location <= _global_console.input_line_count) {
        memset(_global_console.input_line + _global_console.input_line_write_cursor_location, 0, _global_console.input_line_count - _global_console.input_line_write_cursor_location);
        _global_console.input_line_count = _internal_console_strlen(_global_console.input_line);
    }
}

void console_previous_history_entry(void) {
    if (_global_console.selected_history_buffer_index < 0) {
        _global_console.selected_history_buffer_index = _global_console.history_buffer_count - 1;
        if (_global_console.selected_history_buffer_index < 0) {
            return;
        }
    }
    memcpy(_global_console.input_line, _global_console.input_history_buffers[_global_console.selected_history_buffer_index].buffer, CONSOLE_INPUT_LINE_BUFFER_SIZE);
    _global_console.input_line_write_cursor_location = 0;
    _global_console.input_line_count                 = _global_console.input_history_buffers[_global_console.selected_history_buffer_index--].count;
}

void console_next_history_entry(void) {
    if (_global_console.selected_history_buffer_index >= _global_console.history_buffer_count) {
        _global_console.selected_history_buffer_index = 0;
    }
    memcpy(_global_console.input_line, _global_console.input_history_buffers[_global_console.selected_history_buffer_index].buffer, CONSOLE_INPUT_LINE_BUFFER_SIZE);
    _global_console.input_line_write_cursor_location = 0;
    _global_console.input_line_count                 = _global_console.input_history_buffers[_global_console.selected_history_buffer_index++].count;
}

void console_send_character(char character) {
    if (character == '`' || character == '~') {
        console_toggle();
        return;
    }

    if (!console_active()) {
        return;
    }

    if (_global_console.input_line_count >= CONSOLE_INPUT_LINE_BUFFER_SIZE || character == '\r' || character == '\n') {
        return;
    }

    // This looks a little nasty, but I'm not using a gap-buffer or anything
    // this is just a normal "string" operation, so it's technically slow, but practically fast!
    if (character == '\b') {
        if (_global_console.input_line_count > 0) {
            for (size_t cursor_location = _global_console.input_line_write_cursor_location-1; cursor_location < _global_console.input_line_count; ++cursor_location) {
                _global_console.input_line[cursor_location] = _global_console.input_line[cursor_location+1];
            }
            _global_console.input_line[--_global_console.input_line_count] = 0;
            --_global_console.input_line_write_cursor_location;
        } else if (_global_console.input_line_count == 0) {
            _global_console.input_line[0] = 0;
        }
    } else if (is_human_readable_ascii_character(character)) {
        for (size_t cursor_location = _global_console.input_line_count; cursor_location > _global_console.input_line_write_cursor_location; --cursor_location) {
            _global_console.input_line[cursor_location] = _global_console.input_line[cursor_location-1];
        }
        _global_console.input_line_count++;
        _global_console.input_line[_global_console.input_line_write_cursor_location++] = character;
    }
}

static void console_frame(float delta_time) {
    _global_console.cursor_blink_timer += delta_time;
    _global_console.slide_timer        += delta_time * 2.15;

    if (_global_console.input_line_write_cursor_location > _global_console.input_line_count) {
        _global_console.input_line_write_cursor_location = _global_console.input_line_count;
    }

    if (_global_console.cursor_blink_timer >= CONSOLE_CURSOR_BLINK_TIMER_MAX) {
        _global_console.cursor_blink_timer = 0;
        _global_console.cursor_show       ^= true;
    }

    if (_global_console.slide_timer >= 1.0) {
        _global_console.slide_timer = 1.0;
        _global_console.animation_state = CONSOLE_ANIMATION_STATE_DEFAULT;
    }

    // When we are no longer animating,
    // and we are not trying to show ourselves, we don't do anything else.
    if (!_global_console.shown && _global_console.animation_state == CONSOLE_ANIMATION_STATE_DEFAULT) {
        return;
    }

    console_display();
    /* console_submit(); */
}

static char* console_system_variable_type_as_string(uint8_t type) {
    switch (type) {
        case CONSOLE_VARIABLE_TYPE_NUMBER:  return "number";
        case CONSOLE_VARIABLE_TYPE_STRING:  return "string";
        case CONSOLE_VARIABLE_TYPE_BOOLEAN: return "boolean";
    } 
    return "unknown";
}
static char* console_system_number_type_as_string(uint8_t type) {
    switch (type) {
        case CONSOLE_VARIABLE_TYPE_NUMBER_REAL:    return "real";
        case CONSOLE_VARIABLE_TYPE_NUMBER_INTEGER: return "integer";
    }
    return "unknown";
}

console_system_variable_iterator console_system_begin_iterating_variables(void) {
    return (console_system_variable_iterator) {
        .current = _global_console_system.variable_list.head
    };
}
bool console_system_variable_iterator_finished(console_system_variable_iterator* iterator) {
    if (iterator->current == &_global_console_system.variable_list.null) {
        return true;
    }

    return false;
}
struct console_system_variable* console_system_variable_iterator_advance(console_system_variable_iterator* iterator) {
    struct console_system_variable* current = iterator->current;
    iterator->current = current->next;
    return current;
}

// duped.
console_system_command_iterator console_system_begin_iterating_commands(void) {
    return (console_system_command_iterator) {
        .current = _global_console_system.command_list.head
    };
}
bool console_system_command_iterator_finished(console_system_command_iterator* iterator) {
    if (iterator->current == &_global_console_system.command_list.null) {
        return true;
    }

    return false;
}
struct console_system_command* console_system_command_iterator_advance(console_system_command_iterator* iterator) {
    struct console_system_command* current = iterator->current;
    iterator->current = current->next;
    return current;
}



void console_system_register_command(struct console_system_command* command) {
    struct console_system_command* sentinel = &_global_console_system.command_list.null;
    _global_console_system.command_count++;

    if (_global_console_system.command_list.head == sentinel) {
        _global_console_system.command_list.head = command;
    } else {
        struct console_system_command* old_tail = _global_console_system.command_list.tail;
        old_tail->next = command;
    }

    _global_console_system.command_list.tail = command;
    command->next = sentinel; 
}

void console_system_register_variable(struct console_system_variable* variable) {
    struct console_system_variable* sentinel = &_global_console_system.variable_list.null;
    _global_console_system.variable_count++;

    if (_global_console_system.variable_list.head == sentinel) {
        _global_console_system.variable_list.head = variable;
    } else {
        struct console_system_variable* old_tail = _global_console_system.variable_list.tail;
        old_tail->next = variable;
    }

    _global_console_system.variable_list.tail = variable;
    variable->next = sentinel; 
}
    
struct console_system_variable* console_system_find_variable(char* name) {
    for (console_system_variable_iterator iterator = console_system_begin_iterating_variables();
         !console_system_variable_iterator_finished(&iterator);
         console_system_variable_iterator_advance(&iterator)) {
        struct console_system_variable* current = iterator.current;

        if (strcmp(current->name, name) == 0) {
            return current;
        }
    }

    return &_global_console_system.variable_list.null;
}

// NOTE(jerry): this isn't exposed because you shouldn't have to read functions...
// TODO(jerry): differs from find_variable which uses a C string. This is because we use the tokenizer which
// provides a length encoded string! I need to make up my mind on whether to use C strings or go length encoded!
struct console_system_command* console_system_find_command(char* name, size_t name_length) {
    for (console_system_command_iterator iterator = console_system_begin_iterating_commands();
         !console_system_command_iterator_finished(&iterator);
         console_system_command_iterator_advance(&iterator)) {
        struct console_system_command* current = iterator.current;

        size_t length_of_name = _internal_console_strlen(current->name);

        if (name_length == length_of_name) {
            if (_internal_console_strncmp(current->name, name, name_length) == 0) {
                return current;
            }
        }
    }

    return &_global_console_system.command_list.null;
}

static void _console_system_call_procedure_and_do_typechecking(struct console_system_command* command, uint8_t parameter_count, struct console_system_variant* parameters) {
    bool allow_procedure_call = true;

    if (command->argument_definitions.typechecking) {
        // TODO(jerry):
        // type checking I guess.
    }

    if (allow_procedure_call) {
        command->procedure(parameter_count, parameters);
    }
}

// Technically, just like Quake. This can become a state manager in the most disturbing way.
// Just a slower one because it's a linked list (and these are not pooled.)

/*
  I could allow a hook to replace this so you can just have a scripting language replace this if desired... but
  this is going to be an explicit quake console for now I guess.
 */
void console_system_execute(char* line, size_t line_size) {
    struct console_system_command* command = &_global_console_system.command_list.null;
    console_word_iterator          words   = console_word_iterator_begin_iterating_from(line, line_size);

    uint8_t                       parameter_count                                   = 0;
    struct console_system_variant parameters[CONSOLE_SYSTEM_COMMANDS_MAX_ARGUMENTS] = {};

    if (console_word_iterator_length(&words)) {
        command = console_system_find_command(words.word, words.word_length);

        // bad command. very very bad.
        if (command == &_global_console_system.command_list.null) {
            console_printf("No command by the name of \"%.*s\"\n", words.word_length, words.word);
            return;
        }

        console_word_iterator_advance(&words);
        while (!console_word_iterator_finished(&words) && parameter_count < CONSOLE_SYSTEM_COMMANDS_MAX_ARGUMENTS) {
            struct console_system_variant* current_parameter = &parameters[parameter_count++];

            // inner string validation to check for the type I want,
            // this kind of thing has appeared several times already in minischeme.c. Just ignore it.
            uint8_t type = CONSOLE_VARIABLE_TYPE_STRING;
            {
                bool numeric     = true;
                bool real_number = false;

                for (size_t character_index = 0; character_index < words.word_length; ++character_index) {
                    // should really be doing more rigorous input validation.
                    if (!is_numeric_character(words.word[character_index]) && words.word[character_index] != '.') {
                        numeric = false;
                    } else if (words.word[character_index] == '.' && numeric == true) {
                        real_number = true;
                    }
                }

                if (numeric) {
                    type = CONSOLE_VARIABLE_TYPE_NUMBER;

                    if (real_number) {
                        type |= CONSOLE_VARIABLE_TYPE_NUMBER_REAL;
                    } else {
                        type |= CONSOLE_VARIABLE_TYPE_NUMBER_INTEGER;
                    }
                }
            }

            switch (type) {
                case CONSOLE_VARIABLE_TYPE_NUMBER: {
                    // I'm assuming that atof and atoi actually stop at the end of a valid number
                    if (type & CONSOLE_VARIABLE_TYPE_NUMBER_REAL) {
                        float value = _internal_console_atof(words.word);
                        current_parameter->real = value;
                    } else if (type & CONSOLE_VARIABLE_TYPE_NUMBER_INTEGER) {
                        int value = _internal_console_atoi(words.word);
                        current_parameter->integer = value;
                    }
                } break;
                case CONSOLE_VARIABLE_TYPE_STRING: {
                    // not case insensitive.
                    // whoops.
                    bool is_true  = _internal_console_strncmp(words.word, "true", CONSOLE_SYSTEM_VARIABLE_STRING_SIZE) == 0;
                    bool is_false = _internal_console_strncmp(words.word, "false", CONSOLE_SYSTEM_VARIABLE_STRING_SIZE) == 0;

                    if (is_true || is_false) {
                        type = CONSOLE_VARIABLE_TYPE_BOOLEAN;

                        bool value;
                        // LOL, pretty sure this doesn't need to be like that but hahaha.
                        if (is_false) {
                            value = false;                            
                        } else {
                            value = true;
                        }

                        current_parameter->boolean = value;
                    } else {
                        size_t copy_length = words.word_length;
                        if (copy_length > CONSOLE_SYSTEM_VARIABLE_STRING_SIZE) {
                            copy_length = CONSOLE_SYSTEM_VARIABLE_STRING_SIZE;
                        }

                        strncpy(current_parameter->string, words.word, words.word_length);
                    }
                } break;
            }

            current_parameter->type = type;
            console_word_iterator_advance(&words);
        }
        
        _console_system_call_procedure_and_do_typechecking(command, parameter_count, parameters);
    } else {
        return;
    }
}

// This is most definitely not a fully compliant _INTERNAL_CONSOLE_UTF8 decoder,
// but it's good enough I guess.
static const uint8_t _internal_console_utf8_sequence_len[0x100] =
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0x00-0x0F */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0x10-0x1F */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0x20-0x2F */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0x30-0x3F */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0x40-0x4F */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0x50-0x5F */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0x60-0x6F */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0x70-0x7F */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x80-0x8F */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x90-0x9F */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xA0-0xAF */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xB0-0xBF */
    0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 0xC0-0xCF */
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 0xD0-0xDF */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, /* 0xE0-0xEF */
    4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0, /* 0xF0-0xFF */
};

// There are more specific rules but this is a good litmus test?
static size_t _internal_console_utf8_required_bytes(uint8_t leading_byte) {
    struct {
        int mask;
        size_t count;
    } table[] = {
        {0x0,  1},
        {0xC2, 2},
        {0xE0, 3},
        {0xF0, 4},
    };

    for (size_t testing_index = sizeof(table)/sizeof(*table)-1; testing_index != (size_t)(-1); testing_index--) {
        if ((leading_byte & table[testing_index].mask) == table[testing_index].mask) {
            return table[testing_index].count;
        }
    }

    return 0;
}

struct _internal_console_utf8_decode_result _internal_console_utf8_decode_single_codepoint(char* buffer) {
    struct _internal_console_utf8_decode_result decoded_packet = (struct _internal_console_utf8_decode_result){
        .codepoint     = 0,
        .decoded_bytes = 0,
        .error         = false,
    };

    size_t byte_length = _internal_console_utf8_required_bytes((uint8_t)buffer[0]);

    decoded_packet.decoded_bytes = byte_length;
    
    // NOTE(jerry):
    // This... May depend on endianness. I'm kind of hoping it doesn't.
    // but I can always force little endianness later
    uint8_t byte0 = buffer[0];
    uint8_t byte1 = buffer[1];
    uint8_t byte2 = buffer[2];
    uint8_t byte3 = buffer[3];

    switch (byte_length) {
        case 1: {
            decoded_packet.codepoint = byte0;
        } break;
        case 2: {
            // BYTE 1        |      BYTE 2
            // 110xxxxx            10xxxxxx
      // MASK  00011111            00111111
      // HEX   0x1F                0x3F
            decoded_packet.codepoint =
                (byte0 & 0x1F /* 0b00011111 */) << 6 |
                (byte1 & 0x3F /* 0b00111111 */);
        } break;
        case 3: {
            // BYTE 1        |      BYTE 2     |       BYTE 3
            // 110xxxxx            10xxxxxx         10xxxxxx
      // MASK  00011111            00111111         00111111
      // HEX   0x1F                0x3F
            decoded_packet.codepoint =
                (byte0 & 0x1F /* 0b00011111 */) << 12 |
                (byte1 & 0x3F /* 0b00111111 */) << 6 |
                (byte2 & 0x3F /* 0b00111111 */);
        } break;
        case 4: {
            // BYTE 1        |      BYTE 2     |       BYTE 3    |  BYTE 4
            // 110xxxxx            10xxxxxx         10xxxxxx       10xxxxxx
      // MASK  00011111            00111111         00111111       00111111
      // HEX   0x1F                0x3F
            decoded_packet.codepoint =
                (byte0 & 0x1F /* 0b00011111*/) << 18 |
                (byte1 & 0x3F /* 0b00111111*/) << 12 |
                (byte2 & 0x3F /* 0b00111111*/) << 6  |
                (byte3 & 0x3F /* 0b00111111*/);
        } break;
        default: {
            decoded_packet.error = true;
        } break;
    }

    return decoded_packet;
}

uint32_t decode__internal_console_utf8_iterator_advance(decode__internal_console_utf8_iterator* iterator);
decode__internal_console_utf8_iterator decode__internal_console_utf8_from(char* buffer, size_t buffer_length) {
    decode__internal_console_utf8_iterator iterator = (decode__internal_console_utf8_iterator) {
        .buffer        = buffer,
        .buffer_length = buffer_length,
        .decoded_count = 0,
        .codepoint     = 0,
    };

    // This is done because to get the correct initial expected state, which
    // is the first character.
    decode__internal_console_utf8_iterator_advance(&iterator);
    return iterator;
}

decode__internal_console_utf8_iterator decode__internal_console_utf8_from_cstring(char* buffer) {
    size_t string_length = 0;
    char* cursor = buffer;

    while (*(cursor++)) {}
    string_length = cursor - buffer;

    return decode__internal_console_utf8_from(buffer, string_length);
}

bool decode__internal_console_utf8_iterator_valid(decode__internal_console_utf8_iterator* iterator) {
    if (iterator->decoded_count < iterator->buffer_length) {
        return true;
    }

    return false;
}

uint32_t decode__internal_console_utf8_iterator_advance(decode__internal_console_utf8_iterator* iterator) {
    char*  buffer        = iterator->buffer;
    size_t decoded_count = iterator->decoded_count;

    struct _internal_console_utf8_decode_result decoded_packet = _internal_console_utf8_decode_single_codepoint(&buffer[decoded_count]);

    if (decoded_packet.error) {
        return 0;
    } else {
        iterator->decoded_count += decoded_packet.decoded_bytes;
        iterator->codepoint      = decoded_packet.codepoint;
    }

    return iterator->codepoint;
}
#undef CONSOLE_IMPLEMENTATION
#endif
