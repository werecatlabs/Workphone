#ifndef WORKPHONE_STYLE_H_
#define WORKPHONE_STYLE_H_

#include "workphone_color.h"
#include "workphone_image.h"
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Function pointer types for user font */
typedef wp_f32 ( *wp_text_width_f )( wp_handle, wp_f32 h, const wp_c8 *, wp_s32 len );
#ifdef WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
typedef void ( *wp_query_font_glyph_f )( wp_handle, wp_f32 font_height, struct wp_user_font_glyph *,
                                         wp_rune codepoint, wp_rune next_codepoint );
#endif

/* Nine-slice structure */
struct wp_nine_slice
{
    struct wp_image img;
    wp_u16 l, t, r, b;
};

/* Style item - can be a color, image, or nine-slice */
struct wp_style_item
{
    enum wp_style_item_type type;
    union
    {
        struct wp_color color;
        struct wp_image image;
        struct wp_nine_slice slice;
    } data;
};
struct wp_style_text
{
    struct wp_color color;
    struct wp_vec2f padding;
    float color_factor;
    float disabled_factor;
};

struct wp_style_button
{
    /* background */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item active;
    struct wp_color border_color;
    float color_factor_background;

    /* text */
    struct wp_color text_background;
    struct wp_color text_normal;
    struct wp_color text_hover;
    struct wp_color text_active;
    wp_flags text_alignment;
    float color_factor_text;

    /* properties */
    float border;
    float rounding;
    struct wp_vec2f padding;
    struct wp_vec2f image_padding;
    struct wp_vec2f touch_padding;
    float disabled_factor;

    /* optional user callbacks */
    wp_handle userdata;
    void ( *draw_begin )( struct wp_command_buffer *, wp_handle userdata );
    void ( *draw_end )( struct wp_command_buffer *, wp_handle userdata );
};

struct wp_style_toggle
{
    /* background */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item active;
    struct wp_color border_color;

    /* cursor */
    struct wp_style_item cursor_normal;
    struct wp_style_item cursor_hover;

    /* text */
    struct wp_color text_normal;
    struct wp_color text_hover;
    struct wp_color text_active;
    struct wp_color text_background;
    wp_flags text_alignment;

    /* properties */
    struct wp_vec2f padding;
    struct wp_vec2f touch_padding;
    float spacing;
    float border;
    float color_factor;
    float disabled_factor;

    /* optional user callbacks */
    wp_handle userdata;
    void ( *draw_begin )( struct wp_command_buffer *, wp_handle );
    void ( *draw_end )( struct wp_command_buffer *, wp_handle );
};

struct wp_style_selectable
{
    /* background (inactive) */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item pressed;

    /* background (active) */
    struct wp_style_item normal_active;
    struct wp_style_item hover_active;
    struct wp_style_item pressed_active;

    /* text color (inactive) */
    struct wp_color text_normal;
    struct wp_color text_hover;
    struct wp_color text_pressed;

    /* text color (active) */
    struct wp_color text_normal_active;
    struct wp_color text_hover_active;
    struct wp_color text_pressed_active;
    struct wp_color text_background;
    wp_flags text_alignment;

    /* properties */
    float rounding;
    struct wp_vec2f padding;
    struct wp_vec2f touch_padding;
    struct wp_vec2f image_padding;
    float color_factor;
    float disabled_factor;

    /* optional user callbacks */
    wp_handle userdata;
    void ( *draw_begin )( struct wp_command_buffer *, wp_handle );
    void ( *draw_end )( struct wp_command_buffer *, wp_handle );
};

struct wp_style_slider
{
    /* background */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item active;
    struct wp_color border_color;

    /* background bar */
    struct wp_color bar_normal;
    struct wp_color bar_hover;
    struct wp_color bar_active;
    struct wp_color bar_filled;

    /* cursor */
    struct wp_style_item cursor_normal;
    struct wp_style_item cursor_hover;
    struct wp_style_item cursor_active;

    /* properties */
    float border;
    float rounding;
    float bar_height;
    struct wp_vec2f padding;
    struct wp_vec2f spacing;
    struct wp_vec2f cursor_size;
    float color_factor;
    float disabled_factor;

    /* optional buttons */
    int show_buttons;
    struct wp_style_button inc_button;
    struct wp_style_button dec_button;
    enum wp_symbol_type inc_symbol;
    enum wp_symbol_type dec_symbol;

    /* optional user callbacks */
    wp_handle userdata;
    void ( *draw_begin )( struct wp_command_buffer *, wp_handle );
    void ( *draw_end )( struct wp_command_buffer *, wp_handle );
};

struct wp_style_knob
{
    /* background */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item active;
    struct wp_color border_color;

    /* knob */
    struct wp_color knob_normal;
    struct wp_color knob_hover;
    struct wp_color knob_active;
    struct wp_color knob_border_color;

    /* cursor */
    struct wp_color cursor_normal;
    struct wp_color cursor_hover;
    struct wp_color cursor_active;

    /* properties */
    float border;
    float knob_border;
    struct wp_vec2f padding;
    struct wp_vec2f spacing;
    float cursor_width;
    float color_factor;
    float disabled_factor;

    /* optional user callbacks */
    wp_handle userdata;
    void ( *draw_begin )( struct wp_command_buffer *, wp_handle );
    void ( *draw_end )( struct wp_command_buffer *, wp_handle );
};

struct wp_style_progress
{
    /* background */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item active;
    struct wp_color border_color;

    /* cursor */
    struct wp_style_item cursor_normal;
    struct wp_style_item cursor_hover;
    struct wp_style_item cursor_active;
    struct wp_color cursor_border_color;

    /* properties */
    float rounding;
    float border;
    float cursor_border;
    float cursor_rounding;
    struct wp_vec2f padding;
    float color_factor;
    float disabled_factor;

    /* optional user callbacks */
    wp_handle userdata;
    void ( *draw_begin )( struct wp_command_buffer *, wp_handle );
    void ( *draw_end )( struct wp_command_buffer *, wp_handle );
};

struct wp_style_scrollbar
{
    /* background */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item active;
    struct wp_color border_color;

    /* cursor */
    struct wp_style_item cursor_normal;
    struct wp_style_item cursor_hover;
    struct wp_style_item cursor_active;
    struct wp_color cursor_border_color;

    /* properties */
    float border;
    float rounding;
    float border_cursor;
    float rounding_cursor;
    struct wp_vec2f padding;
    float color_factor;
    float disabled_factor;

    /* optional buttons */
    int show_buttons;
    struct wp_style_button inc_button;
    struct wp_style_button dec_button;
    enum wp_symbol_type inc_symbol;
    enum wp_symbol_type dec_symbol;

    /* optional user callbacks */
    wp_handle userdata;
    void ( *draw_begin )( struct wp_command_buffer *, wp_handle );
    void ( *draw_end )( struct wp_command_buffer *, wp_handle );
};

struct wp_style_edit
{
    /* background */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item active;
    struct wp_color border_color;
    struct wp_style_scrollbar scrollbar;

    /* cursor  */
    struct wp_color cursor_normal;
    struct wp_color cursor_hover;
    struct wp_color cursor_text_normal;
    struct wp_color cursor_text_hover;

    /* text (unselected) */
    struct wp_color text_normal;
    struct wp_color text_hover;
    struct wp_color text_active;

    /* text (selected) */
    struct wp_color selected_normal;
    struct wp_color selected_hover;
    struct wp_color selected_text_normal;
    struct wp_color selected_text_hover;

    /* properties */
    float border;
    float rounding;
    float cursor_size;
    struct wp_vec2f scrollbar_size;
    struct wp_vec2f padding;
    float row_padding;
    float color_factor;
    float disabled_factor;
};

struct wp_style_property
{
    /* background */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item active;
    struct wp_color border_color;

    /* text */
    struct wp_color label_normal;
    struct wp_color label_hover;
    struct wp_color label_active;

    /* symbols */
    enum wp_symbol_type sym_left;
    enum wp_symbol_type sym_right;

    /* properties */
    float border;
    float rounding;
    struct wp_vec2f padding;
    float color_factor;
    float disabled_factor;

    struct wp_style_edit edit;
    struct wp_style_button inc_button;
    struct wp_style_button dec_button;

    /* optional user callbacks */
    wp_handle userdata;
    void ( *draw_begin )( struct wp_command_buffer *, wp_handle );
    void ( *draw_end )( struct wp_command_buffer *, wp_handle );
};

struct wp_style_chart
{
    /* colors */
    struct wp_style_item background;
    struct wp_color border_color;
    struct wp_color selected_color;
    struct wp_color color;

    /* properties */
    float border;
    float rounding;
    struct wp_vec2f padding;
    float color_factor;
    float disabled_factor;
    wp_bool show_markers;
};

struct wp_style_combo
{
    /* background */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item active;
    struct wp_color border_color;

    /* label */
    struct wp_color label_normal;
    struct wp_color label_hover;
    struct wp_color label_active;

    /* symbol */
    struct wp_color symbol_normal;
    struct wp_color symbol_hover;
    struct wp_color symbol_active;

    /* button */
    struct wp_style_button button;
    enum wp_symbol_type sym_normal;
    enum wp_symbol_type sym_hover;
    enum wp_symbol_type sym_active;

    /* properties */
    float border;
    float rounding;
    struct wp_vec2f content_padding;
    struct wp_vec2f button_padding;
    struct wp_vec2f spacing;
    float color_factor;
    float disabled_factor;
};

struct wp_style_tab
{
    /* background */
    struct wp_style_item background;
    struct wp_color border_color;
    struct wp_color text;

    /* button */
    struct wp_style_button tab_maximize_button;
    struct wp_style_button tab_minimize_button;
    struct wp_style_button node_maximize_button;
    struct wp_style_button node_minimize_button;
    enum wp_symbol_type sym_minimize;
    enum wp_symbol_type sym_maximize;

    /* properties */
    float border;
    float rounding;
    float indent;
    struct wp_vec2f padding;
    struct wp_vec2f spacing;
    float color_factor;
    float disabled_factor;
};

struct wp_style_window_header
{
    /* background */
    struct wp_style_item normal;
    struct wp_style_item hover;
    struct wp_style_item active;

    /* button */
    struct wp_style_button close_button;
    struct wp_style_button minimize_button;
    enum wp_symbol_type close_symbol;
    enum wp_symbol_type minimize_symbol;
    enum wp_symbol_type maximize_symbol;

    /* title */
    struct wp_color label_normal;
    struct wp_color label_hover;
    struct wp_color label_active;

    /* properties */
    enum wp_style_header_align align;
    struct wp_vec2f padding;
    struct wp_vec2f label_padding;
    struct wp_vec2f spacing;
};

struct wp_style_window
{
    struct wp_style_window_header header;
    struct wp_style_item fixed_background;
    struct wp_color background;

    struct wp_color border_color;
    struct wp_color popup_border_color;
    struct wp_color combo_border_color;
    struct wp_color contextual_border_color;
    struct wp_color menu_border_color;
    struct wp_color group_border_color;
    struct wp_color tooltip_border_color;
    struct wp_style_item scaler;

    float border;
    float combo_border;
    float contextual_border;
    float menu_border;
    float group_border;
    float tooltip_border;
    float popup_border;
    float min_row_height_padding;

    float rounding;
    struct wp_vec2f spacing;
    struct wp_vec2f scrollbar_size;
    struct wp_vec2f min_size;

    struct wp_vec2f padding;
    struct wp_vec2f group_padding;
    struct wp_vec2f popup_padding;
    struct wp_vec2f combo_padding;
    struct wp_vec2f contextual_padding;
    struct wp_vec2f menu_padding;
    struct wp_vec2f tooltip_padding;
};

struct wp_style
{
    const struct wp_user_font *font;
    const struct wp_cursor *cursors[WORKPHONE_CURSOR_COUNT];
    const struct wp_cursor *cursor_active;
    struct wp_cursor *cursor_last;
    int cursor_visible;

    struct wp_style_text text;
    struct wp_style_button button;
    struct wp_style_button contextual_button;
    struct wp_style_button menu_button;
    struct wp_style_toggle option;
    struct wp_style_toggle checkbox;
    struct wp_style_selectable selectable;
    struct wp_style_slider slider;
    struct wp_style_knob knob;
    struct wp_style_progress progress;
    struct wp_style_property property;
    struct wp_style_edit edit;
    struct wp_style_chart chart;
    struct wp_style_scrollbar scrollh;
    struct wp_style_scrollbar scrollv;
    struct wp_style_tab tab;
    struct wp_style_combo combo;
    struct wp_style_window window;
};

/* Style item creation functions */
WORKPHONE_API struct wp_style_item wp_style_item_color( struct wp_color col );
WORKPHONE_API struct wp_style_item wp_style_item_image( struct wp_image img );
WORKPHONE_API struct wp_style_item wp_style_item_nine_slice( struct wp_nine_slice slice );
WORKPHONE_API struct wp_style_item wp_style_item_hide( void );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_STYLE_H_ */
