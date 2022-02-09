/*
  Colorscheme database and loader.
*/

#define GAME_COLORSCHEME_NAME_STRING_MAX (32)
struct game_colorscheme {
    char name[GAME_COLORSCHEME_NAME_STRING_MAX];
    union color4f primary;
    union color4f secondary;
    union color4f text;
    union color4f primary_background;
    union color4f primary_foreground;
};

struct game_colorscheme test1 = {
    .primary   = COLOR4F_normalize(102, 10, 163, 255),
    .secondary = COLOR4F_normalize(204, 128, 255, 255),
    .text      = COLOR4F_normalize(210, 230, 92, 255),
};
local uint16_t colorscheme_database_size = 0;
local struct game_colorscheme* colors_database = NULL;

local struct game_colorscheme active_colorscheme;

uint8_t hex_to_uint8(char hex) {
    switch (hex) {
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
            return hex - '0';
        } break;
        case 'A': case 'a': return 10;
        case 'B': case 'b': return 11;
        case 'C': case 'c': return 12;
        case 'D': case 'd': return 13;
        case 'E': case 'e': return 14;
        case 'F': case 'f': return 15;
    }

    return 0;
}

float string_nibble_to_float(char* nibble) {
    uint8_t first  = hex_to_uint8(nibble[0]);
    uint8_t second = hex_to_uint8(nibble[1]);
    uint8_t byte   = (first << 4) | (second);
    float   result = ((float) byte) / 255.0f;
    return  result;
}
/*
  File is very whitespace sensitive,
  but thankfully you should never touch it.
  
  None of this does any error checking, and is not a secure lexical analyser. It
  will break on any wrong condition
*/
union color4f _colors_read_colorstring(char* file_buffer, size_t* read_index, union color4f* last_read_color) {
    char color_string_buffer[128] = {};
    *read_index += copy_until_next_line(file_buffer, *read_index, color_string_buffer, sizeof(color_string_buffer));

    if (color_string_buffer[0] == '*') {
        return *last_read_color;
    } else {
        assert((color_string_buffer[0] == '#' ) && "This ain't no hex string");
        return color4f(
            string_nibble_to_float(&color_string_buffer[1]),
            string_nibble_to_float(&color_string_buffer[3]),
            string_nibble_to_float(&color_string_buffer[5]),
            1.0f
        );
    }

    return COLOR4F_BLACK;
}

void load_colorscheme_database(struct memory_arena* arena, char* filepath) {
    size_t file_size  = file_length(filepath);
    char* file_buffer = load_entire_file(filepath);

    size_t read_index = 0;

    union color4f last_read_color = {};

    while (read_index < file_size) {
        switch (file_buffer[read_index]) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
                read_index++;
                break;
            default: {
                struct game_colorscheme* current_colorscheme = memory_arena_push(arena, sizeof(*current_colorscheme));
                colorscheme_database_size++;
                read_index += copy_until_next_line(file_buffer, read_index, current_colorscheme->name, GAME_COLORSCHEME_NAME_STRING_MAX);
                console_printf("\"%s\" colorscheme loaded\n", current_colorscheme->name);

                size_t colors_per_struct = (sizeof(struct game_colorscheme) - GAME_COLORSCHEME_NAME_STRING_MAX) / sizeof(union color4f);
                size_t written_colors = 0;
                union color4f* current_color_location = &current_colorscheme->primary;

                /*
                  NOTE(jerry):
                  Colorschemes are just lists/arrays of colors, and I'm going to take advantage of that and just write this...
                  It's not very clear code, but this allows me to grow the struct and add colors without really changing much code.
                  
                  Takes advantage of the fact the struct is sanely packed (4 byte alignment, and name string must be a POT in order for
                  this to work.)
                 */
                while (file_buffer[read_index] != '\n' && file_buffer[read_index] != '\r' && read_index < file_size) {
                    union color4f current_read_color = _colors_read_colorstring(file_buffer, &read_index, &last_read_color);;

                    if (written_colors < colors_per_struct) {
                        *current_color_location = current_read_color; 
                        written_colors++;
                        current_color_location++;
                    } else {
                        console_printf("(NOTE): Read extra color (%f, %f, %f) from theme!\n",
                                       current_read_color.r, current_read_color.g, current_read_color.b);
                    }
                }

                console_printf("Done reading color\n");
            } break;
        }
    }

    system_deallocate_memory(file_buffer);
}

void initialize_colorscheme_database(struct memory_arena* arena) {
    /* Just for a base address. */
    colors_database = memory_arena_push(arena, 0);
    load_colorscheme_database(arena, "colors.txt");

    use_colorscheme("MonoRed0");
}


void use_colorscheme(char* name) {
    bool found = false;
    struct game_colorscheme* colors = 0;

    for (unsigned index = 0; index < colorscheme_database_size && !found; ++index) {
        colors = colors_database + index;
        if (strncmp(colors->name, name, GAME_COLORSCHEME_NAME_STRING_MAX) == 0) {
            found = true;
        }
    }

    if (!found) console_printf("Missing colorscheme?\n");
    active_colorscheme = *colors;
}
