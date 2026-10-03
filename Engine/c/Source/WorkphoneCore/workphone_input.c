#include "workphone.h"
#include "workphone_utf8.h"

void wp_input_begin( struct wp_context *ctx )
{
    wp_s32 i;
    struct wp_input *in;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    in = &ctx->input;
    for( i = 0; i < WORKPHONE_BUTTON_MAX; ++i )
        in->mouse.buttons[i].clicked = 0;

    in->keyboard.text_len = 0;
    in->mouse.scroll_delta = wp_make_vec2f( 0, 0 );
    in->mouse.prev.x = in->mouse.pos.x;
    in->mouse.prev.y = in->mouse.pos.y;
    in->mouse.delta.x = 0;
    in->mouse.delta.y = 0;
    for( i = 0; i < WORKPHONE_KEY_MAX; i++ )
        in->keyboard.keys[i].clicked = 0;
}
void wp_input_end( struct wp_context *ctx )
{
    struct wp_input *in;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    in = &ctx->input;
    if( in->mouse.grab )
        in->mouse.grab = 0;
    if( in->mouse.ungrab )
    {
        in->mouse.grabbed = 0;
        in->mouse.ungrab = 0;
        in->mouse.grab = 0;
    }
}
void wp_input_motion( struct wp_context *ctx, wp_s32 x, wp_s32 y )
{
    struct wp_input *in;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    in = &ctx->input;
    in->mouse.pos.x = (wp_f32)x;
    in->mouse.pos.y = (wp_f32)y;
    in->mouse.delta.x = in->mouse.pos.x - in->mouse.prev.x;
    in->mouse.delta.y = in->mouse.pos.y - in->mouse.prev.y;
}
void wp_input_key( struct wp_context *ctx, enum wp_keys key, wp_bool down )
{
    struct wp_input *in;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    in = &ctx->input;
#ifdef WORKPHONE_KEYSTATE_BASED_INPUT
    if( in->keyboard.keys[key].down != down )
        in->keyboard.keys[key].clicked++;
#else
    in->keyboard.keys[key].clicked++;
#endif
    in->keyboard.keys[key].down = down;
}
void wp_input_button( struct wp_context *ctx, enum wp_buttons id, wp_s32 x, wp_s32 y, wp_bool down )
{
    struct wp_mouse_button *btn;
    struct wp_input *in;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    in = &ctx->input;
    if( in->mouse.buttons[id].down == down )
        return;

    btn = &in->mouse.buttons[id];
    btn->clicked_pos.x = (wp_f32)x;
    btn->clicked_pos.y = (wp_f32)y;
    btn->down = down;
    btn->clicked++;

    /* Fix Click-Drag for touch events. */
    in->mouse.delta.x = 0;
    in->mouse.delta.y = 0;
#ifdef WORKPHONE_BUTTON_TRIGGER_ON_RELEASE
    if( down == 1 && id == WORKPHONE_BUTTON_LEFT )
    {
        in->mouse.down_pos.x = btn->clicked_pos.x;
        in->mouse.down_pos.y = btn->clicked_pos.y;
    }
#endif
}
void wp_input_scroll( struct wp_context *ctx, struct wp_vec2f val )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    ctx->input.mouse.scroll_delta.x += val.x;
    ctx->input.mouse.scroll_delta.y += val.y;
}
void wp_input_glyph( struct wp_context *ctx, const wp_glyph glyph )
{
    wp_s32 len = 0;
    wp_rune unicode;
    struct wp_input *in;

    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    in = &ctx->input;

    len = wp_utf_decode( glyph, &unicode, WORKPHONE_UTF_SIZE );
    if( len && ( ( in->keyboard.text_len + len ) < WORKPHONE_INPUT_MAX ) )
    {
        wp_utf_encode( unicode, &in->keyboard.text[in->keyboard.text_len],
                       WORKPHONE_INPUT_MAX - in->keyboard.text_len );
        in->keyboard.text_len += len;
    }
}
void wp_input_wp_c8( struct wp_context *ctx, wp_c8 c )
{
    wp_glyph glyph = { 0 };
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    glyph[0] = c;
    wp_input_glyph( ctx, glyph );
}
void wp_input_unicode( struct wp_context *ctx, wp_rune unicode )
{
    wp_glyph rune;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    wp_utf_encode( unicode, rune, WORKPHONE_UTF_SIZE );
    wp_input_glyph( ctx, rune );
}
wp_bool wp_input_has_mouse_click( const struct wp_input *i, enum wp_buttons id )
{
    const struct wp_mouse_button *btn;
    if( !i )
        return wp_false;
    btn = &i->mouse.buttons[id];
    return ( btn->clicked && btn->down == wp_false ) ? wp_true : wp_false;
}
wp_bool wp_input_has_mouse_click_in_rect( const struct wp_input *i, enum wp_buttons id,
                                          struct wp_rect b )
{
    const struct wp_mouse_button *btn;
    if( !i )
        return wp_false;
    btn = &i->mouse.buttons[id];
    if( !WORKPHONE_INBOX( btn->clicked_pos.x, btn->clicked_pos.y, b.x, b.y, b.w, b.h ) )
        return wp_false;
    return wp_true;
}
wp_bool wp_input_has_mouse_click_in_button_rect( const struct wp_input *i, enum wp_buttons id,
                                                 struct wp_rect b )
{
    const struct wp_mouse_button *btn;
    if( !i )
        return wp_false;
    btn = &i->mouse.buttons[id];
#ifdef WORKPHONE_BUTTON_TRIGGER_ON_RELEASE
    if( !WORKPHONE_INBOX( btn->clicked_pos.x, btn->clicked_pos.y, b.x, b.y, b.w, b.h ) ||
        !WORKPHONE_INBOX( i->mouse.down_pos.x, i->mouse.down_pos.y, b.x, b.y, b.w, b.h ) )
#else
    if( !WORKPHONE_INBOX( btn->clicked_pos.x, btn->clicked_pos.y, b.x, b.y, b.w, b.h ) )
#endif
        return wp_false;
    return wp_true;
}
wp_bool wp_input_has_mouse_click_down_in_rect( const struct wp_input *i, enum wp_buttons id,
                                               struct wp_rect b, wp_bool down )
{
    const struct wp_mouse_button *btn;
    if( !i )
        return wp_false;
    btn = &i->mouse.buttons[id];
    return wp_input_has_mouse_click_in_rect( i, id, b ) && ( btn->down == down );
}
wp_bool wp_input_is_mouse_click_in_rect( const struct wp_input *i, enum wp_buttons id, struct wp_rect b )
{
    const struct wp_mouse_button *btn;
    if( !i )
        return wp_false;
    btn = &i->mouse.buttons[id];
    return ( wp_input_has_mouse_click_down_in_rect( i, id, b, wp_false ) && btn->clicked ) ? wp_true
                                                                                           : wp_false;
}
wp_bool wp_input_is_mouse_click_down_in_rect( const struct wp_input *i, enum wp_buttons id,
                                              struct wp_rect b, wp_bool down )
{
    const struct wp_mouse_button *btn;
    if( !i )
        return wp_false;
    btn = &i->mouse.buttons[id];
    return ( wp_input_has_mouse_click_down_in_rect( i, id, b, down ) && btn->clicked ) ? wp_true
                                                                                       : wp_false;
}
wp_bool wp_input_any_mouse_click_in_rect( const struct wp_input *in, struct wp_rect b )
{
    wp_s32 i, down = 0;
    for( i = 0; i < WORKPHONE_BUTTON_MAX; ++i )
        down = down || wp_input_is_mouse_click_in_rect( in, (enum wp_buttons)i, b );
    return down;
}
wp_bool wp_input_is_mouse_hovering_rect( const struct wp_input *i, struct wp_rect rect )
{
    if( !i )
        return wp_false;
    return WORKPHONE_INBOX( i->mouse.pos.x, i->mouse.pos.y, rect.x, rect.y, rect.w, rect.h );
}
wp_bool wp_input_is_mouse_prev_hovering_rect( const struct wp_input *i, struct wp_rect rect )
{
    if( !i )
        return wp_false;
    return WORKPHONE_INBOX( i->mouse.prev.x, i->mouse.prev.y, rect.x, rect.y, rect.w, rect.h );
}
wp_bool wp_input_mouse_clicked( const struct wp_input *i, enum wp_buttons id, struct wp_rect rect )
{
    if( !i )
        return wp_false;
    if( !wp_input_is_mouse_hovering_rect( i, rect ) )
        return wp_false;
    return wp_input_is_mouse_click_in_rect( i, id, rect );
}
wp_bool wp_input_is_mouse_down( const struct wp_input *i, enum wp_buttons id )
{
    if( !i )
        return wp_false;
    return i->mouse.buttons[id].down;
}
wp_bool wp_input_is_mouse_pressed( const struct wp_input *i, enum wp_buttons id )
{
    const struct wp_mouse_button *b;
    if( !i )
        return wp_false;
    b = &i->mouse.buttons[id];
    if( b->down && b->clicked )
        return wp_true;
    return wp_false;
}
wp_bool wp_input_is_mouse_released( const struct wp_input *i, enum wp_buttons id )
{
    if( !i )
        return wp_false;
    return ( !i->mouse.buttons[id].down && i->mouse.buttons[id].clicked );
}
wp_bool wp_input_is_mouse_moved( const struct wp_input *i )
{
    if( !i )
        return wp_false;
    return i->mouse.delta.x != 0 || i->mouse.delta.y != 0;
}
wp_bool wp_input_is_key_pressed( const struct wp_input *i, enum wp_keys key )
{
    const struct wp_key *k;
    if( !i )
        return wp_false;
    k = &i->keyboard.keys[key];
    if( ( k->down && k->clicked ) || ( !k->down && k->clicked >= 2 ) )
        return wp_true;
    return wp_false;
}
wp_bool wp_input_is_key_released( const struct wp_input *i, enum wp_keys key )
{
    const struct wp_key *k;
    if( !i )
        return wp_false;
    k = &i->keyboard.keys[key];
    if( ( !k->down && k->clicked ) || ( k->down && k->clicked >= 2 ) )
        return wp_true;
    return wp_false;
}
wp_bool wp_input_is_key_down( const struct wp_input *i, enum wp_keys key )
{
    const struct wp_key *k;
    if( !i )
        return wp_false;
    k = &i->keyboard.keys[key];
    if( k->down )
        return wp_true;
    return wp_false;
}
