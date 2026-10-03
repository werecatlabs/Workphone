#include "workphone.h"
#include "workphone_cursor.h"
#include "workphone_layout.h"

void wp_style_default( struct wp_context *ctx )
{
    wp_style_from_table( ctx, 0 );
}

#define WORKPHONE_COLOR_MAP( WORKPHONE_COLOR ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_TEXT, 175, 175, 175, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_WINDOW, 45, 45, 45, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_HEADER, 40, 40, 40, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_BORDER, 65, 65, 65, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_BUTTON, 50, 50, 50, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_BUTTON_HOVER, 40, 40, 40, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_BUTTON_ACTIVE, 35, 35, 35, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_TOGGLE, 100, 100, 100, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_TOGGLE_HOVER, 120, 120, 120, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_TOGGLE_CURSOR, 45, 45, 45, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_SELECT, 45, 45, 45, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_SELECT_ACTIVE, 35, 35, 35, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_SLIDER, 38, 38, 38, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_SLIDER_CURSOR, 100, 100, 100, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_SLIDER_CURSOR_HOVER, 120, 120, 120, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_SLIDER_CURSOR_ACTIVE, 150, 150, 150, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_PROPERTY, 38, 38, 38, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_EDIT, 38, 38, 38, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_EDIT_CURSOR, 175, 175, 175, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_COMBO, 45, 45, 45, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_CHART, 120, 120, 120, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_CHART_COLOR, 45, 45, 45, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_CHART_COLOR_HIGHLIGHT, 255, 0, 0, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_SCROLLBAR, 40, 40, 40, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_SCROLLBAR_CURSOR, 100, 100, 100, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_SCROLLBAR_CURSOR_HOVER, 120, 120, 120, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_SCROLLBAR_CURSOR_ACTIVE, 150, 150, 150, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_TAB_HEADER, 40, 40, 40, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_KNOB, 38, 38, 38, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_KNOB_CURSOR, 100, 100, 100, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_KNOB_CURSOR_HOVER, 120, 120, 120, 255 ) \
    WORKPHONE_COLOR( WORKPHONE_COLOR_KNOB_CURSOR_ACTIVE, 150, 150, 150, 255 )

WORKPHONE_GLOBAL const struct wp_color wp_default_color_style[WORKPHONE_COLOR_COUNT] = {
#define WORKPHONE_COLOR( a, b, c, d, e ) { b, c, d, e },
    WORKPHONE_COLOR_MAP( WORKPHONE_COLOR )
#undef WORKPHONE_COLOR
};
WORKPHONE_GLOBAL const wp_c8 *wp_color_names[WORKPHONE_COLOR_COUNT] = {
#define WORKPHONE_COLOR( a, b, c, d, e ) #a,
    WORKPHONE_COLOR_MAP( WORKPHONE_COLOR )
#undef WORKPHONE_COLOR
};

const wp_c8 *wp_style_get_color_by_name( enum wp_style_colors c )
{
    return wp_color_names[c];
}

struct wp_style_item wp_style_item_color( struct wp_color col )
{
    struct wp_style_item i;
    i.type = WORKPHONE_STYLE_ITEM_COLOR;
    i.data.color = col;
    return i;
}
struct wp_style_item wp_style_item_image( struct wp_image img )
{
    struct wp_style_item i;
    i.type = WORKPHONE_STYLE_ITEM_IMAGE;
    i.data.image = img;
    return i;
}

struct wp_style_item wp_style_item_nine_slice( struct wp_nine_slice slice )
{
    struct wp_style_item i;
    i.type = WORKPHONE_STYLE_ITEM_NINE_SLICE;
    i.data.slice = slice;
    return i;
}

struct wp_style_item wp_style_item_hide( void )
{
    struct wp_style_item i;
    i.type = WORKPHONE_STYLE_ITEM_COLOR;
    i.data.color = wp_rgba( 0, 0, 0, 0 );
    return i;
}

void wp_style_from_table( struct wp_context *ctx, const struct wp_color *table )
{
    struct wp_style *style;
    struct wp_style_text *text;
    struct wp_style_button *button;
    struct wp_style_toggle *toggle;
    struct wp_style_selectable *select;
    struct wp_style_slider *slider;
    struct wp_style_knob *knob;
    struct wp_style_progress *prog;
    struct wp_style_scrollbar *scroll;
    struct wp_style_edit *edit;
    struct wp_style_property *property;
    struct wp_style_combo *combo;
    struct wp_style_chart *chart;
    struct wp_style_tab *tab;
    struct wp_style_window *win;

    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    style = &ctx->style;
    table = ( !table ) ? wp_default_color_style : table;

    /* default text */
    text = &style->text;
    text->color = table[WORKPHONE_COLOR_TEXT];
    text->padding = wp_make_vec2f( 0, 0 );
    text->color_factor = 1.0f;
    text->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;

    /* default button */
    button = &style->button;
    wp_zero_struct( *button );
    button->normal = wp_style_item_color( table[WORKPHONE_COLOR_BUTTON] );
    button->hover = wp_style_item_color( table[WORKPHONE_COLOR_BUTTON_HOVER] );
    button->active = wp_style_item_color( table[WORKPHONE_COLOR_BUTTON_ACTIVE] );
    button->border_color = table[WORKPHONE_COLOR_BORDER];
    button->text_background = table[WORKPHONE_COLOR_BUTTON];
    button->text_normal = table[WORKPHONE_COLOR_TEXT];
    button->text_hover = table[WORKPHONE_COLOR_TEXT];
    button->text_active = table[WORKPHONE_COLOR_TEXT];
    button->padding = wp_make_vec2f( 2.0f, 2.0f );
    button->image_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 1.0f;
    button->rounding = 4.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;

    /* contextual button */
    button = &style->contextual_button;
    wp_zero_struct( *button );
    button->normal = wp_style_item_color( table[WORKPHONE_COLOR_WINDOW] );
    button->hover = wp_style_item_color( table[WORKPHONE_COLOR_BUTTON_HOVER] );
    button->active = wp_style_item_color( table[WORKPHONE_COLOR_BUTTON_ACTIVE] );
    button->border_color = table[WORKPHONE_COLOR_WINDOW];
    button->text_background = table[WORKPHONE_COLOR_WINDOW];
    button->text_normal = table[WORKPHONE_COLOR_TEXT];
    button->text_hover = table[WORKPHONE_COLOR_TEXT];
    button->text_active = table[WORKPHONE_COLOR_TEXT];
    button->padding = wp_make_vec2f( 2.0f, 2.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 0.0f;
    button->rounding = 0.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;

    /* menu button */
    button = &style->menu_button;
    wp_zero_struct( *button );
    button->normal = wp_style_item_color( table[WORKPHONE_COLOR_WINDOW] );
    button->hover = wp_style_item_color( table[WORKPHONE_COLOR_WINDOW] );
    button->active = wp_style_item_color( table[WORKPHONE_COLOR_WINDOW] );
    button->border_color = table[WORKPHONE_COLOR_WINDOW];
    button->text_background = table[WORKPHONE_COLOR_WINDOW];
    button->text_normal = table[WORKPHONE_COLOR_TEXT];
    button->text_hover = table[WORKPHONE_COLOR_TEXT];
    button->text_active = table[WORKPHONE_COLOR_TEXT];
    button->padding = wp_make_vec2f( 2.0f, 2.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 0.0f;
    button->rounding = 1.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;

    /* checkbox toggle */
    toggle = &style->checkbox;
    wp_zero_struct( *toggle );
    toggle->normal = wp_style_item_color( table[WORKPHONE_COLOR_TOGGLE] );
    toggle->hover = wp_style_item_color( table[WORKPHONE_COLOR_TOGGLE_HOVER] );
    toggle->active = wp_style_item_color( table[WORKPHONE_COLOR_TOGGLE_HOVER] );
    toggle->cursor_normal = wp_style_item_color( table[WORKPHONE_COLOR_TOGGLE_CURSOR] );
    toggle->cursor_hover = wp_style_item_color( table[WORKPHONE_COLOR_TOGGLE_CURSOR] );
    toggle->userdata = wp_handle_ptr( 0 );
    toggle->text_background = table[WORKPHONE_COLOR_WINDOW];
    toggle->text_normal = table[WORKPHONE_COLOR_TEXT];
    toggle->text_hover = table[WORKPHONE_COLOR_TEXT];
    toggle->text_active = table[WORKPHONE_COLOR_TEXT];
    toggle->padding = wp_make_vec2f( 2.0f, 2.0f );
    toggle->touch_padding = wp_make_vec2f( 0, 0 );
    toggle->border_color = wp_rgba( 0, 0, 0, 0 );
    toggle->border = 0.0f;
    toggle->spacing = 4;
    toggle->color_factor = 1.0f;
    toggle->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;

    /* option toggle */
    toggle = &style->option;
    wp_zero_struct( *toggle );
    toggle->normal = wp_style_item_color( table[WORKPHONE_COLOR_TOGGLE] );
    toggle->hover = wp_style_item_color( table[WORKPHONE_COLOR_TOGGLE_HOVER] );
    toggle->active = wp_style_item_color( table[WORKPHONE_COLOR_TOGGLE_HOVER] );
    toggle->cursor_normal = wp_style_item_color( table[WORKPHONE_COLOR_TOGGLE_CURSOR] );
    toggle->cursor_hover = wp_style_item_color( table[WORKPHONE_COLOR_TOGGLE_CURSOR] );
    toggle->userdata = wp_handle_ptr( 0 );
    toggle->text_background = table[WORKPHONE_COLOR_WINDOW];
    toggle->text_normal = table[WORKPHONE_COLOR_TEXT];
    toggle->text_hover = table[WORKPHONE_COLOR_TEXT];
    toggle->text_active = table[WORKPHONE_COLOR_TEXT];
    toggle->padding = wp_make_vec2f( 3.0f, 3.0f );
    toggle->touch_padding = wp_make_vec2f( 0, 0 );
    toggle->border_color = wp_rgba( 0, 0, 0, 0 );
    toggle->border = 0.0f;
    toggle->spacing = 4;
    toggle->color_factor = 1.0f;
    toggle->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;

    /* selectable */
    select = &style->selectable;
    wp_zero_struct( *select );
    select->normal = wp_style_item_color( table[WORKPHONE_COLOR_SELECT] );
    select->hover = wp_style_item_color( table[WORKPHONE_COLOR_SELECT] );
    select->pressed = wp_style_item_color( table[WORKPHONE_COLOR_SELECT] );
    select->normal_active = wp_style_item_color( table[WORKPHONE_COLOR_SELECT_ACTIVE] );
    select->hover_active = wp_style_item_color( table[WORKPHONE_COLOR_SELECT_ACTIVE] );
    select->pressed_active = wp_style_item_color( table[WORKPHONE_COLOR_SELECT_ACTIVE] );
    select->text_normal = table[WORKPHONE_COLOR_TEXT];
    select->text_hover = table[WORKPHONE_COLOR_TEXT];
    select->text_pressed = table[WORKPHONE_COLOR_TEXT];
    select->text_normal_active = table[WORKPHONE_COLOR_TEXT];
    select->text_hover_active = table[WORKPHONE_COLOR_TEXT];
    select->text_pressed_active = table[WORKPHONE_COLOR_TEXT];
    select->padding = wp_make_vec2f( 2.0f, 2.0f );
    select->image_padding = wp_make_vec2f( 2.0f, 2.0f );
    select->touch_padding = wp_make_vec2f( 0, 0 );
    select->userdata = wp_handle_ptr( 0 );
    select->rounding = 0.0f;
    select->color_factor = 1.0f;
    select->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    select->draw_begin = 0;
    select->draw_end = 0;

    /* slider */
    slider = &style->slider;
    wp_zero_struct( *slider );
    slider->normal = wp_style_item_hide();
    slider->hover = wp_style_item_hide();
    slider->active = wp_style_item_hide();
    slider->bar_normal = table[WORKPHONE_COLOR_SLIDER];
    slider->bar_hover = table[WORKPHONE_COLOR_SLIDER];
    slider->bar_active = table[WORKPHONE_COLOR_SLIDER];
    slider->bar_filled = table[WORKPHONE_COLOR_SLIDER_CURSOR];
    slider->cursor_normal = wp_style_item_color( table[WORKPHONE_COLOR_SLIDER_CURSOR] );
    slider->cursor_hover = wp_style_item_color( table[WORKPHONE_COLOR_SLIDER_CURSOR_HOVER] );
    slider->cursor_active = wp_style_item_color( table[WORKPHONE_COLOR_SLIDER_CURSOR_ACTIVE] );
    slider->inc_symbol = WORKPHONE_SYMBOL_TRIANGLE_RIGHT;
    slider->dec_symbol = WORKPHONE_SYMBOL_TRIANGLE_LEFT;
    slider->cursor_size = wp_make_vec2f( 16, 16 );
    slider->padding = wp_make_vec2f( 2, 2 );
    slider->spacing = wp_make_vec2f( 2, 2 );
    slider->userdata = wp_handle_ptr( 0 );
    slider->show_buttons = wp_false;
    slider->bar_height = 8;
    slider->rounding = 0;
    slider->color_factor = 1.0f;
    slider->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    slider->draw_begin = 0;
    slider->draw_end = 0;

    /* slider buttons */
    button = &style->slider.inc_button;
    button->normal = wp_style_item_color( wp_rgb( 40, 40, 40 ) );
    button->hover = wp_style_item_color( wp_rgb( 42, 42, 42 ) );
    button->active = wp_style_item_color( wp_rgb( 44, 44, 44 ) );
    button->border_color = wp_rgb( 65, 65, 65 );
    button->text_background = wp_rgb( 40, 40, 40 );
    button->text_normal = wp_rgb( 175, 175, 175 );
    button->text_hover = wp_rgb( 175, 175, 175 );
    button->text_active = wp_rgb( 175, 175, 175 );
    button->padding = wp_make_vec2f( 8.0f, 8.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 1.0f;
    button->rounding = 0.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;
    style->slider.dec_button = style->slider.inc_button;

    /* knob */
    knob = &style->knob;
    wp_zero_struct( *knob );
    knob->normal = wp_style_item_hide();
    knob->hover = wp_style_item_hide();
    knob->active = wp_style_item_hide();
    knob->knob_normal = table[WORKPHONE_COLOR_KNOB];
    knob->knob_hover = table[WORKPHONE_COLOR_KNOB];
    knob->knob_active = table[WORKPHONE_COLOR_KNOB];
    knob->cursor_normal = table[WORKPHONE_COLOR_KNOB_CURSOR];
    knob->cursor_hover = table[WORKPHONE_COLOR_KNOB_CURSOR_HOVER];
    knob->cursor_active = table[WORKPHONE_COLOR_KNOB_CURSOR_ACTIVE];

    knob->knob_border_color = table[WORKPHONE_COLOR_BORDER];
    knob->knob_border = 1.0f;

    knob->padding = wp_make_vec2f( 2, 2 );
    knob->spacing = wp_make_vec2f( 2, 2 );
    knob->cursor_width = 2;
    knob->color_factor = 1.0f;
    knob->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;

    knob->userdata = wp_handle_ptr( 0 );
    knob->draw_begin = 0;
    knob->draw_end = 0;

    /* progressbar */
    prog = &style->progress;
    wp_zero_struct( *prog );
    prog->normal = wp_style_item_color( table[WORKPHONE_COLOR_SLIDER] );
    prog->hover = wp_style_item_color( table[WORKPHONE_COLOR_SLIDER] );
    prog->active = wp_style_item_color( table[WORKPHONE_COLOR_SLIDER] );
    prog->cursor_normal = wp_style_item_color( table[WORKPHONE_COLOR_SLIDER_CURSOR] );
    prog->cursor_hover = wp_style_item_color( table[WORKPHONE_COLOR_SLIDER_CURSOR_HOVER] );
    prog->cursor_active = wp_style_item_color( table[WORKPHONE_COLOR_SLIDER_CURSOR_ACTIVE] );
    prog->border_color = wp_rgba( 0, 0, 0, 0 );
    prog->cursor_border_color = wp_rgba( 0, 0, 0, 0 );
    prog->userdata = wp_handle_ptr( 0 );
    prog->padding = wp_make_vec2f( 4, 4 );
    prog->rounding = 0;
    prog->border = 0;
    prog->cursor_rounding = 0;
    prog->cursor_border = 0;
    prog->color_factor = 1.0f;
    prog->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    prog->draw_begin = 0;
    prog->draw_end = 0;

    /* scrollbars */
    scroll = &style->scrollh;
    wp_zero_struct( *scroll );
    scroll->normal = wp_style_item_color( table[WORKPHONE_COLOR_SCROLLBAR] );
    scroll->hover = wp_style_item_color( table[WORKPHONE_COLOR_SCROLLBAR] );
    scroll->active = wp_style_item_color( table[WORKPHONE_COLOR_SCROLLBAR] );
    scroll->cursor_normal = wp_style_item_color( table[WORKPHONE_COLOR_SCROLLBAR_CURSOR] );
    scroll->cursor_hover = wp_style_item_color( table[WORKPHONE_COLOR_SCROLLBAR_CURSOR_HOVER] );
    scroll->cursor_active = wp_style_item_color( table[WORKPHONE_COLOR_SCROLLBAR_CURSOR_ACTIVE] );
    scroll->dec_symbol = WORKPHONE_SYMBOL_CIRCLE_SOLID;
    scroll->inc_symbol = WORKPHONE_SYMBOL_CIRCLE_SOLID;
    scroll->userdata = wp_handle_ptr( 0 );
    scroll->border_color = table[WORKPHONE_COLOR_SCROLLBAR];
    scroll->cursor_border_color = table[WORKPHONE_COLOR_SCROLLBAR];
    scroll->padding = wp_make_vec2f( 0, 0 );
    scroll->show_buttons = wp_false;
    scroll->border = 0;
    scroll->rounding = 0;
    scroll->border_cursor = 0;
    scroll->rounding_cursor = 0;
    scroll->color_factor = 1.0f;
    scroll->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    scroll->draw_begin = 0;
    scroll->draw_end = 0;
    style->scrollv = style->scrollh;

    /* scrollbars buttons */
    button = &style->scrollh.inc_button;
    button->normal = wp_style_item_color( wp_rgb( 40, 40, 40 ) );
    button->hover = wp_style_item_color( wp_rgb( 42, 42, 42 ) );
    button->active = wp_style_item_color( wp_rgb( 44, 44, 44 ) );
    button->border_color = wp_rgb( 65, 65, 65 );
    button->text_background = wp_rgb( 40, 40, 40 );
    button->text_normal = wp_rgb( 175, 175, 175 );
    button->text_hover = wp_rgb( 175, 175, 175 );
    button->text_active = wp_rgb( 175, 175, 175 );
    button->padding = wp_make_vec2f( 4.0f, 4.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 1.0f;
    button->rounding = 0.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;
    style->scrollh.dec_button = style->scrollh.inc_button;
    style->scrollv.inc_button = style->scrollh.inc_button;
    style->scrollv.dec_button = style->scrollh.inc_button;

    /* edit */
    edit = &style->edit;
    wp_zero_struct( *edit );
    edit->normal = wp_style_item_color( table[WORKPHONE_COLOR_EDIT] );
    edit->hover = wp_style_item_color( table[WORKPHONE_COLOR_EDIT] );
    edit->active = wp_style_item_color( table[WORKPHONE_COLOR_EDIT] );
    edit->cursor_normal = table[WORKPHONE_COLOR_TEXT];
    edit->cursor_hover = table[WORKPHONE_COLOR_TEXT];
    edit->cursor_text_normal = table[WORKPHONE_COLOR_EDIT];
    edit->cursor_text_hover = table[WORKPHONE_COLOR_EDIT];
    edit->border_color = table[WORKPHONE_COLOR_BORDER];
    edit->text_normal = table[WORKPHONE_COLOR_TEXT];
    edit->text_hover = table[WORKPHONE_COLOR_TEXT];
    edit->text_active = table[WORKPHONE_COLOR_TEXT];
    edit->selected_normal = table[WORKPHONE_COLOR_TEXT];
    edit->selected_hover = table[WORKPHONE_COLOR_TEXT];
    edit->selected_text_normal = table[WORKPHONE_COLOR_EDIT];
    edit->selected_text_hover = table[WORKPHONE_COLOR_EDIT];
    edit->scrollbar_size = wp_make_vec2f( 10, 10 );
    edit->scrollbar = style->scrollv;
    edit->padding = wp_make_vec2f( 4, 4 );
    edit->row_padding = 2;
    edit->cursor_size = 4;
    edit->border = 1;
    edit->rounding = 0;
    edit->color_factor = 1.0f;
    edit->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;

    /* property */
    property = &style->property;
    wp_zero_struct( *property );
    property->normal = wp_style_item_color( table[WORKPHONE_COLOR_PROPERTY] );
    property->hover = wp_style_item_color( table[WORKPHONE_COLOR_PROPERTY] );
    property->active = wp_style_item_color( table[WORKPHONE_COLOR_PROPERTY] );
    property->border_color = table[WORKPHONE_COLOR_BORDER];
    property->label_normal = table[WORKPHONE_COLOR_TEXT];
    property->label_hover = table[WORKPHONE_COLOR_TEXT];
    property->label_active = table[WORKPHONE_COLOR_TEXT];
    property->sym_left = WORKPHONE_SYMBOL_TRIANGLE_LEFT;
    property->sym_right = WORKPHONE_SYMBOL_TRIANGLE_RIGHT;
    property->userdata = wp_handle_ptr( 0 );
    property->padding = wp_make_vec2f( 4, 4 );
    property->border = 1;
    property->rounding = 10;
    property->draw_begin = 0;
    property->draw_end = 0;
    property->color_factor = 1.0f;
    property->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;

    /* property buttons */
    button = &style->property.dec_button;
    wp_zero_struct( *button );
    button->normal = wp_style_item_color( table[WORKPHONE_COLOR_PROPERTY] );
    button->hover = wp_style_item_color( table[WORKPHONE_COLOR_PROPERTY] );
    button->active = wp_style_item_color( table[WORKPHONE_COLOR_PROPERTY] );
    button->border_color = wp_rgba( 0, 0, 0, 0 );
    button->text_background = table[WORKPHONE_COLOR_PROPERTY];
    button->text_normal = table[WORKPHONE_COLOR_TEXT];
    button->text_hover = table[WORKPHONE_COLOR_TEXT];
    button->text_active = table[WORKPHONE_COLOR_TEXT];
    button->padding = wp_make_vec2f( 0.0f, 0.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 0.0f;
    button->rounding = 0.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;
    style->property.inc_button = style->property.dec_button;

    /* property edit */
    edit = &style->property.edit;
    wp_zero_struct( *edit );
    edit->normal = wp_style_item_color( table[WORKPHONE_COLOR_PROPERTY] );
    edit->hover = wp_style_item_color( table[WORKPHONE_COLOR_PROPERTY] );
    edit->active = wp_style_item_color( table[WORKPHONE_COLOR_PROPERTY] );
    edit->border_color = wp_rgba( 0, 0, 0, 0 );
    edit->cursor_normal = table[WORKPHONE_COLOR_TEXT];
    edit->cursor_hover = table[WORKPHONE_COLOR_TEXT];
    edit->cursor_text_normal = table[WORKPHONE_COLOR_EDIT];
    edit->cursor_text_hover = table[WORKPHONE_COLOR_EDIT];
    edit->text_normal = table[WORKPHONE_COLOR_TEXT];
    edit->text_hover = table[WORKPHONE_COLOR_TEXT];
    edit->text_active = table[WORKPHONE_COLOR_TEXT];
    edit->selected_normal = table[WORKPHONE_COLOR_TEXT];
    edit->selected_hover = table[WORKPHONE_COLOR_TEXT];
    edit->selected_text_normal = table[WORKPHONE_COLOR_EDIT];
    edit->selected_text_hover = table[WORKPHONE_COLOR_EDIT];
    edit->padding = wp_make_vec2f( 0, 0 );
    edit->cursor_size = 8;
    edit->border = 0;
    edit->rounding = 0;
    edit->color_factor = 1.0f;
    edit->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;

    /* wp_c8t */
    chart = &style->chart;
    wp_zero_struct( *chart );
    chart->background = wp_style_item_color( table[WORKPHONE_COLOR_CHART] );
    chart->border_color = table[WORKPHONE_COLOR_BORDER];
    chart->selected_color = table[WORKPHONE_COLOR_CHART_COLOR_HIGHLIGHT];
    chart->color = table[WORKPHONE_COLOR_CHART_COLOR];
    chart->padding = wp_make_vec2f( 4, 4 );
    chart->border = 0;
    chart->rounding = 0;
    chart->color_factor = 1.0f;
    chart->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    chart->show_markers = wp_true;

    /* combo */
    combo = &style->combo;
    combo->normal = wp_style_item_color( table[WORKPHONE_COLOR_COMBO] );
    combo->hover = wp_style_item_color( table[WORKPHONE_COLOR_COMBO] );
    combo->active = wp_style_item_color( table[WORKPHONE_COLOR_COMBO] );
    combo->border_color = table[WORKPHONE_COLOR_BORDER];
    combo->label_normal = table[WORKPHONE_COLOR_TEXT];
    combo->label_hover = table[WORKPHONE_COLOR_TEXT];
    combo->label_active = table[WORKPHONE_COLOR_TEXT];
    combo->sym_normal = WORKPHONE_SYMBOL_TRIANGLE_DOWN;
    combo->sym_hover = WORKPHONE_SYMBOL_TRIANGLE_DOWN;
    combo->sym_active = WORKPHONE_SYMBOL_TRIANGLE_DOWN;
    combo->content_padding = wp_make_vec2f( 4, 4 );
    combo->button_padding = wp_make_vec2f( 0, 4 );
    combo->spacing = wp_make_vec2f( 4, 0 );
    combo->border = 1;
    combo->rounding = 0;
    combo->color_factor = 1.0f;
    combo->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;

    /* combo button */
    button = &style->combo.button;
    wp_zero_struct( *button );
    button->normal = wp_style_item_color( table[WORKPHONE_COLOR_COMBO] );
    button->hover = wp_style_item_color( table[WORKPHONE_COLOR_COMBO] );
    button->active = wp_style_item_color( table[WORKPHONE_COLOR_COMBO] );
    button->border_color = wp_rgba( 0, 0, 0, 0 );
    button->text_background = table[WORKPHONE_COLOR_COMBO];
    button->text_normal = table[WORKPHONE_COLOR_TEXT];
    button->text_hover = table[WORKPHONE_COLOR_TEXT];
    button->text_active = table[WORKPHONE_COLOR_TEXT];
    button->padding = wp_make_vec2f( 2.0f, 2.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 0.0f;
    button->rounding = 0.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;

    /* tab */
    tab = &style->tab;
    tab->background = wp_style_item_color( table[WORKPHONE_COLOR_TAB_HEADER] );
    tab->border_color = table[WORKPHONE_COLOR_BORDER];
    tab->text = table[WORKPHONE_COLOR_TEXT];
    tab->sym_minimize = WORKPHONE_SYMBOL_TRIANGLE_RIGHT;
    tab->sym_maximize = WORKPHONE_SYMBOL_TRIANGLE_DOWN;
    tab->padding = wp_make_vec2f( 4, 4 );
    tab->spacing = wp_make_vec2f( 4, 4 );
    tab->indent = 10.0f;
    tab->border = 1;
    tab->rounding = 0;
    tab->color_factor = 1.0f;
    tab->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;

    /* tab button */
    button = &style->tab.tab_minimize_button;
    wp_zero_struct( *button );
    button->normal = wp_style_item_color( table[WORKPHONE_COLOR_TAB_HEADER] );
    button->hover = wp_style_item_color( table[WORKPHONE_COLOR_TAB_HEADER] );
    button->active = wp_style_item_color( table[WORKPHONE_COLOR_TAB_HEADER] );
    button->border_color = wp_rgba( 0, 0, 0, 0 );
    button->text_background = table[WORKPHONE_COLOR_TAB_HEADER];
    button->text_normal = table[WORKPHONE_COLOR_TEXT];
    button->text_hover = table[WORKPHONE_COLOR_TEXT];
    button->text_active = table[WORKPHONE_COLOR_TEXT];
    button->padding = wp_make_vec2f( 2.0f, 2.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 0.0f;
    button->rounding = 0.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;
    style->tab.tab_maximize_button = *button;

    /* node button */
    button = &style->tab.node_minimize_button;
    wp_zero_struct( *button );
    button->normal = wp_style_item_color( table[WORKPHONE_COLOR_WINDOW] );
    button->hover = wp_style_item_color( table[WORKPHONE_COLOR_WINDOW] );
    button->active = wp_style_item_color( table[WORKPHONE_COLOR_WINDOW] );
    button->border_color = wp_rgba( 0, 0, 0, 0 );
    button->text_background = table[WORKPHONE_COLOR_TAB_HEADER];
    button->text_normal = table[WORKPHONE_COLOR_TEXT];
    button->text_hover = table[WORKPHONE_COLOR_TEXT];
    button->text_active = table[WORKPHONE_COLOR_TEXT];
    button->padding = wp_make_vec2f( 2.0f, 2.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 0.0f;
    button->rounding = 0.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;
    style->tab.node_maximize_button = *button;

    /* window header */
    win = &style->window;
    win->header.align = WORKPHONE_HEADER_RIGHT;
    win->header.close_symbol = WORKPHONE_SYMBOL_X;
    win->header.minimize_symbol = WORKPHONE_SYMBOL_MINUS;
    win->header.maximize_symbol = WORKPHONE_SYMBOL_PLUS;
    win->header.normal = wp_style_item_color( table[WORKPHONE_COLOR_HEADER] );
    win->header.hover = wp_style_item_color( table[WORKPHONE_COLOR_HEADER] );
    win->header.active = wp_style_item_color( table[WORKPHONE_COLOR_HEADER] );
    win->header.label_normal = table[WORKPHONE_COLOR_TEXT];
    win->header.label_hover = table[WORKPHONE_COLOR_TEXT];
    win->header.label_active = table[WORKPHONE_COLOR_TEXT];
    win->header.label_padding = wp_make_vec2f( 4, 4 );
    win->header.padding = wp_make_vec2f( 4, 4 );
    win->header.spacing = wp_make_vec2f( 0, 0 );

    /* window header close button */
    button = &style->window.header.close_button;
    wp_zero_struct( *button );
    button->normal = wp_style_item_color( table[WORKPHONE_COLOR_HEADER] );
    button->hover = wp_style_item_color( table[WORKPHONE_COLOR_HEADER] );
    button->active = wp_style_item_color( table[WORKPHONE_COLOR_HEADER] );
    button->border_color = wp_rgba( 0, 0, 0, 0 );
    button->text_background = table[WORKPHONE_COLOR_HEADER];
    button->text_normal = table[WORKPHONE_COLOR_TEXT];
    button->text_hover = table[WORKPHONE_COLOR_TEXT];
    button->text_active = table[WORKPHONE_COLOR_TEXT];
    button->padding = wp_make_vec2f( 0.0f, 0.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 0.0f;
    button->rounding = 0.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;

    /* window header minimize button */
    button = &style->window.header.minimize_button;
    wp_zero_struct( *button );
    button->normal = wp_style_item_color( table[WORKPHONE_COLOR_HEADER] );
    button->hover = wp_style_item_color( table[WORKPHONE_COLOR_HEADER] );
    button->active = wp_style_item_color( table[WORKPHONE_COLOR_HEADER] );
    button->border_color = wp_rgba( 0, 0, 0, 0 );
    button->text_background = table[WORKPHONE_COLOR_HEADER];
    button->text_normal = table[WORKPHONE_COLOR_TEXT];
    button->text_hover = table[WORKPHONE_COLOR_TEXT];
    button->text_active = table[WORKPHONE_COLOR_TEXT];
    button->padding = wp_make_vec2f( 0.0f, 0.0f );
    button->touch_padding = wp_make_vec2f( 0.0f, 0.0f );
    button->userdata = wp_handle_ptr( 0 );
    button->text_alignment = WORKPHONE_TEXT_CENTERED;
    button->border = 0.0f;
    button->rounding = 0.0f;
    button->color_factor_text = 1.0f;
    button->color_factor_background = 1.0f;
    button->disabled_factor = WORKPHONE_WIDGET_DISABLED_FACTOR;
    button->draw_begin = 0;
    button->draw_end = 0;

    /* window */
    win->background = table[WORKPHONE_COLOR_WINDOW];
    win->fixed_background = wp_style_item_color( table[WORKPHONE_COLOR_WINDOW] );
    win->border_color = table[WORKPHONE_COLOR_BORDER];
    win->popup_border_color = table[WORKPHONE_COLOR_BORDER];
    win->combo_border_color = table[WORKPHONE_COLOR_BORDER];
    win->contextual_border_color = table[WORKPHONE_COLOR_BORDER];
    win->menu_border_color = table[WORKPHONE_COLOR_BORDER];
    win->group_border_color = table[WORKPHONE_COLOR_BORDER];
    win->tooltip_border_color = table[WORKPHONE_COLOR_BORDER];
    win->scaler = wp_style_item_color( table[WORKPHONE_COLOR_TEXT] );

    win->rounding = 0.0f;
    win->spacing = wp_make_vec2f( 4, 4 );
    win->scrollbar_size = wp_make_vec2f( 10, 10 );
    win->min_size = wp_make_vec2f( 64, 64 );

    win->combo_border = 1.0f;
    win->contextual_border = 1.0f;
    win->menu_border = 1.0f;
    win->group_border = 1.0f;
    win->tooltip_border = 1.0f;
    win->popup_border = 1.0f;
    win->border = 2.0f;
    win->min_row_height_padding = 8;

    win->padding = wp_make_vec2f( 4, 4 );
    win->group_padding = wp_make_vec2f( 4, 4 );
    win->popup_padding = wp_make_vec2f( 4, 4 );
    win->combo_padding = wp_make_vec2f( 4, 4 );
    win->contextual_padding = wp_make_vec2f( 4, 4 );
    win->menu_padding = wp_make_vec2f( 4, 4 );
    win->tooltip_padding = wp_make_vec2f( 4, 4 );
}

void wp_style_set_font( struct wp_context *ctx, const struct wp_user_font *font )
{
    struct wp_style *style;
    WORKPHONE_ASSERT( ctx );

    if( !ctx )
        return;
    style = &ctx->style;
    style->font = font;
    ctx->stacks.fonts.head = 0;
    if( ctx->current )
        wp_layout_reset_min_row_height( ctx );
}

wp_bool wp_style_push_font( struct wp_context *ctx, const struct wp_user_font *font )
{
    struct wp_config_stack_user_font *font_stack;
    struct wp_config_stack_user_font_element *element;

    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;

    font_stack = &ctx->stacks.fonts;
    WORKPHONE_ASSERT( font_stack->head < (wp_s32)WORKPHONE_LEN( font_stack->elements ) );
    if( font_stack->head >= (wp_s32)WORKPHONE_LEN( font_stack->elements ) )
        return 0;

    element = &font_stack->elements[font_stack->head++];
    element->address = &ctx->style.font;
    element->old_value = ctx->style.font;
    ctx->style.font = font;
    return 1;
}

wp_bool wp_style_pop_font( struct wp_context *ctx )
{
    struct wp_config_stack_user_font *font_stack;
    struct wp_config_stack_user_font_element *element;

    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;

    font_stack = &ctx->stacks.fonts;
    WORKPHONE_ASSERT( font_stack->head > 0 );
    if( font_stack->head < 1 )
        return 0;

    element = &font_stack->elements[--font_stack->head];
    *element->address = element->old_value;
    return 1;
}

#define WORKPHONE_STYLE_PUSH_IMPLEMENATION( name, prefix, type, stack ) \
    wp_style_push_##name( struct wp_context *ctx, prefix##_##type *address, prefix##_##type value ) \
    { \
        struct wp_config_stack_##name *type_stack; \
        struct wp_config_stack_##name##_element *element; \
        WORKPHONE_ASSERT( ctx ); \
        if( !ctx ) \
            return 0; \
        type_stack = &ctx->stacks.stack; \
        WORKPHONE_ASSERT( type_stack->head < (wp_s32)WORKPHONE_LEN( type_stack->elements ) ); \
        if( type_stack->head >= (wp_s32)WORKPHONE_LEN( type_stack->elements ) ) \
            return 0; \
        element = &type_stack->elements[type_stack->head++]; \
        element->address = address; \
        element->old_value = *address; \
        *address = value; \
        return 1; \
    }

#define WORKPHONE_STYLE_POP_IMPLEMENATION( type, stack ) \
    wp_style_pop_##type( struct wp_context *ctx ) \
    { \
        struct wp_config_stack_##type *type_stack; \
        struct wp_config_stack_##type##_element *element; \
        WORKPHONE_ASSERT( ctx ); \
        if( !ctx ) \
            return 0; \
        type_stack = &ctx->stacks.stack; \
        WORKPHONE_ASSERT( type_stack->head > 0 ); \
        if( type_stack->head < 1 ) \
            return 0; \
        element = &type_stack->elements[--type_stack->head]; \
        *element->address = element->old_value; \
        return 1; \
    }

wp_bool WORKPHONE_STYLE_PUSH_IMPLEMENATION( style_item, struct wp, style_item, style_items )
wp_bool WORKPHONE_STYLE_PUSH_IMPLEMENATION( wp_f32, wp, wp_f32, wp_f32s )
wp_bool WORKPHONE_STYLE_PUSH_IMPLEMENATION( vec2, wp, vec2f, vectors )
wp_bool WORKPHONE_STYLE_PUSH_IMPLEMENATION( flags, wp, flags, flags )
wp_bool WORKPHONE_STYLE_PUSH_IMPLEMENATION( color, struct wp, color, colors )

wp_bool WORKPHONE_STYLE_POP_IMPLEMENATION( style_item, style_items )
wp_bool WORKPHONE_STYLE_POP_IMPLEMENATION( wp_f32, wp_f32s )
wp_bool WORKPHONE_STYLE_POP_IMPLEMENATION( vec2, vectors )
wp_bool WORKPHONE_STYLE_POP_IMPLEMENATION( flags, flags )
wp_bool WORKPHONE_STYLE_POP_IMPLEMENATION( color, colors )

wp_bool wp_style_set_cursor( struct wp_context *ctx, enum wp_cursor_type c )
{
    struct wp_style *style;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;
    style = &ctx->style;
    if( style->cursors[c] )
    {
        style->cursor_active = style->cursors[c];
        return 1;
    }
    return 0;
}

void wp_style_show_cursor( struct wp_context *ctx )
{
    ctx->style.cursor_visible = wp_true;
}

void wp_style_hide_cursor( struct wp_context *ctx )
{
    ctx->style.cursor_visible = wp_false;
}

void wp_style_load_cursor( struct wp_context *ctx, enum wp_cursor_type cursor,
                           const struct wp_cursor *c )
{
    struct wp_style *style;

    WORKPHONE_ASSERT( ctx );

    if( !ctx )
        return;

    style = &ctx->style;
    style->cursors[cursor] = c;
}

void wp_style_load_all_cursors( struct wp_context *ctx, const struct wp_cursor *cursors )
{
    wp_s32 i = 0;
    struct wp_style *style;

    WORKPHONE_ASSERT( ctx );

    if( !ctx )
        return;

    style = &ctx->style;
    for( i = 0; i < WORKPHONE_CURSOR_COUNT; ++i )
        style->cursors[i] = &cursors[i];

    style->cursor_visible = wp_true;
}
