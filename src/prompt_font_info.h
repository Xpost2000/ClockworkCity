/*
  Useful constants and globals for prompt font usage.
*/
enum prompt_font_size {
    PROMPT_FONT_SIZE_SMALL,
    PROMPT_FONT_SIZE_MEDIUM,
    PROMPT_FONT_SIZE_LARGE,
    PROMPT_FONT_SIZE_COUNT,
};
local const float prompt_font_sizes[PROMPT_FONT_SIZE_COUNT] = {
    0.06*3, 0.09*3, 0.12*3,
};
local font_id controller_prompt_font[PROMPT_FONT_SIZE_COUNT];

/* 
   Some useful glyph constants, mostly the ones I'm expecting to use, which
   are primarily game controllers. 
*/
enum {
    FONT_GLYPH_GAMEPAD            = 0x243C,
    FONT_GLYPH_KEYBOARD           = 0x243D,
    FONT_GLYPH_MOUSE              = 0x243E,
    FONT_GLYPH_MOUSE_AND_KEYBOARD = 0x243F,

    /* Glyph provides playstation alternatives. Only use xbox */
    FONT_GLYPH_LT = 0x2196,
    FONT_GLYPH_RT = 0x2197,

    FONT_GLYPH_LB = 0x2198,
    FONT_GLYPH_RB = 0x2199,

    FONT_GLYPH_DPAD_LEFT  = 0x219E,
    FONT_GLYPH_DPAD_UP    = 0x219F,
    FONT_GLYPH_DPAD_RIGHT = 0x21A0,
    FONT_GLYPH_DPAD_DOWN  = 0x21A1,

    FONT_GLYPH_RIGHT_ANALOG_UP    = 0x21BF,
    FONT_GLYPH_RIGHT_ANALOG_LEFT  = 0x21BD,
    FONT_GLYPH_RIGHT_ANALOG_RIGHT = 0x21C1,
    FONT_GLYPH_RIGHT_ANALOG_DOWN  = 0x21C3,

    FONT_GLYPH_LEFT_ANALOG_UP    = 0x21BE,
    FONT_GLYPH_LEFT_ANALOG_LEFT  = 0x21BC,
    FONT_GLYPH_LEFT_ANALOG_RIGHT = 0x21C0,
    FONT_GLYPH_LEFT_ANALOG_DOWN  = 0x21C2,
    
    /* generic buttons */
    FONT_GLYPH_BUTTON_LEFT  = 0x21A4,
    FONT_GLYPH_BUTTON_UP    = 0x21A5,
    FONT_GLYPH_BUTTON_RIGHT = 0x21A6,
    FONT_GLYPH_BUTTON_DOWN  = 0x21A7,

    /* xbox buttons */
    FONT_GLYPH_BUTTON_X = 0x21D0,
    FONT_GLYPH_BUTTON_Y = 0x21D1,
    FONT_GLYPH_BUTTON_B = 0x21D2,
    FONT_GLYPH_BUTTON_A = 0x21D3,
};
