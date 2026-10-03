#ifndef workphone_input_h__
#define workphone_input_h__

#include "workphone_vector.h"

struct wp_mouse_button
{
    wp_bool down;
    unsigned int clicked;
    struct wp_vec2f clicked_pos;
};

struct wp_mouse
{
    struct wp_mouse_button buttons[WORKPHONE_BUTTON_MAX];
    struct wp_vec2f pos;
#ifdef WORKPHONE_BUTTON_TRIGGER_ON_RELEASE
    struct wp_vec2f down_pos;
#endif
    struct wp_vec2f prev;
    struct wp_vec2f delta;
    struct wp_vec2f scroll_delta;
    unsigned char grab;
    unsigned char grabbed;
    unsigned char ungrab;
};

struct wp_key
{
    wp_bool down;
    unsigned int clicked;
};

struct wp_keyboard
{
    struct wp_key keys[WORKPHONE_KEY_MAX];
    char text[WORKPHONE_INPUT_MAX];
    int text_len;
};

struct wp_input
{
    struct wp_keyboard keyboard;
    struct wp_mouse mouse;
};

WORKPHONE_API wp_bool wp_input_has_mouse_click( const struct wp_input *, enum wp_buttons );
WORKPHONE_API wp_bool wp_input_has_mouse_click_in_rect( const struct wp_input *, enum wp_buttons,
                                                        struct wp_rect );
WORKPHONE_API wp_bool wp_input_has_mouse_click_in_button_rect( const struct wp_input *, enum wp_buttons,
                                                               struct wp_rect );
WORKPHONE_API wp_bool wp_input_has_mouse_click_down_in_rect( const struct wp_input *, enum wp_buttons,
                                                             struct wp_rect, wp_bool down );
WORKPHONE_API wp_bool wp_input_is_mouse_click_in_rect( const struct wp_input *, enum wp_buttons,
                                                       struct wp_rect );
WORKPHONE_API wp_bool wp_input_is_mouse_click_down_in_rect( const struct wp_input *i, enum wp_buttons id,
                                                            struct wp_rect b, wp_bool down );
WORKPHONE_API wp_bool wp_input_any_mouse_click_in_rect( const struct wp_input *, struct wp_rect );
WORKPHONE_API wp_bool wp_input_is_mouse_prev_hovering_rect( const struct wp_input *, struct wp_rect );
WORKPHONE_API wp_bool wp_input_is_mouse_hovering_rect( const struct wp_input *, struct wp_rect );
WORKPHONE_API wp_bool wp_input_is_mouse_moved( const struct wp_input * );
WORKPHONE_API wp_bool wp_input_mouse_clicked( const struct wp_input *, enum wp_buttons, struct wp_rect );
WORKPHONE_API wp_bool wp_input_is_mouse_down( const struct wp_input *, enum wp_buttons );
WORKPHONE_API wp_bool wp_input_is_mouse_pressed( const struct wp_input *, enum wp_buttons );
WORKPHONE_API wp_bool wp_input_is_mouse_released( const struct wp_input *, enum wp_buttons );
WORKPHONE_API wp_bool wp_input_is_key_pressed( const struct wp_input *, enum wp_keys );
WORKPHONE_API wp_bool wp_input_is_key_released( const struct wp_input *, enum wp_keys );
WORKPHONE_API wp_bool wp_input_is_key_down( const struct wp_input *, enum wp_keys );

WORKPHONE_API void wp_input_begin( struct wp_context * );
WORKPHONE_API void wp_input_motion( struct wp_context *, int x, int y );
WORKPHONE_API void wp_input_key( struct wp_context *, enum wp_keys, wp_bool down );
WORKPHONE_API void wp_input_button( struct wp_context *, enum wp_buttons, int x, int y, wp_bool down );
WORKPHONE_API void wp_input_scroll( struct wp_context *, struct wp_vec2f val );
WORKPHONE_API void wp_input_char( struct wp_context *, char );
WORKPHONE_API void wp_input_glyph( struct wp_context *, const wp_glyph );
WORKPHONE_API void wp_input_unicode( struct wp_context *, wp_rune );
WORKPHONE_API void wp_input_end( struct wp_context * );

#endif  // workphone_input_h__
