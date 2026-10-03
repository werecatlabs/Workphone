#include "workphone_edit.h"
#include "workphone_color.h"
#include "workphone_command_buffer.h"
#include "workphone_context.h"
#include "workphone_font.h"
#include "workphone_input.h"
#include "workphone_scrollbar.h"
#include "workphone_style.h"
#include "workphone_text.h"
#include "workphone_utf8.h"
#include "workphone_widget.h"

WORKPHONE_API wp_bool wp_filter_default( const struct wp_text_edit *box, wp_rune unicode )
{
    WORKPHONE_UNUSED( unicode );
    WORKPHONE_UNUSED( box );
    return wp_true;
}
WORKPHONE_API wp_bool wp_filter_ascii( const struct wp_text_edit *box, wp_rune unicode )
{
    WORKPHONE_UNUSED( box );
    if( unicode > 128 )
        return wp_false;
    else
        return wp_true;
}
WORKPHONE_API wp_bool wp_filter_wp_f32( const struct wp_text_edit *box, wp_rune unicode )
{
    WORKPHONE_UNUSED( box );
    if( ( unicode < '0' || unicode > '9' ) && unicode != '.' && unicode != '-' )
        return wp_false;
    else
        return wp_true;
}
WORKPHONE_API wp_bool wp_filter_decimal( const struct wp_text_edit *box, wp_rune unicode )
{
    WORKPHONE_UNUSED( box );
    if( ( unicode < '0' || unicode > '9' ) && unicode != '-' )
        return wp_false;
    else
        return wp_true;
}
WORKPHONE_API wp_bool wp_filter_hex( const struct wp_text_edit *box, wp_rune unicode )
{
    WORKPHONE_UNUSED( box );
    if( ( unicode < '0' || unicode > '9' ) && ( unicode < 'a' || unicode > 'f' ) &&
        ( unicode < 'A' || unicode > 'F' ) )
        return wp_false;
    else
        return wp_true;
}
WORKPHONE_API wp_bool wp_filter_oct( const struct wp_text_edit *box, wp_rune unicode )
{
    WORKPHONE_UNUSED( box );
    if( unicode < '0' || unicode > '7' )
        return wp_false;
    else
        return wp_true;
}
WORKPHONE_API wp_bool wp_filter_binary( const struct wp_text_edit *box, wp_rune unicode )
{
    WORKPHONE_UNUSED( box );
    if( unicode != '0' && unicode != '1' )
        return wp_false;
    else
        return wp_true;
}

/* ===============================================================
 *
 *                          EDIT
 *
 * ===============================================================*/
WORKPHONE_LIB void wp_edit_draw_text( struct wp_command_buffer *out, const struct wp_style_edit *style,
                                      wp_f32 pos_x, wp_f32 pos_y, wp_f32 x_offset, const wp_c8 *text,
                                      wp_s32 byte_len, wp_f32 row_height,
                                      const struct wp_user_font *font, struct wp_color background,
                                      struct wp_color foreground, wp_bool is_selected )
{
    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( font );
    WORKPHONE_ASSERT( style );
    if( !text || !byte_len || !out || !style )
        return;

    {
        wp_s32 glyph_len = 0;
        wp_rune unicode = 0;
        wp_s32 text_len = 0;
        wp_f32 line_width = 0;
        wp_f32 glyph_width;
        const wp_c8 *line = text;
        wp_f32 line_offset = 0;
        wp_s32 line_count = 0;

        struct wp_text txt;
        txt.padding = wp_make_vec2f( 0, 0 );
        txt.background = background;
        txt.text = foreground;

        foreground = wp_rgb_factor( foreground, style->color_factor );
        background = wp_rgb_factor( background, style->color_factor );

        glyph_len = wp_utf_decode( text + text_len, &unicode, byte_len - text_len );
        if( !glyph_len )
            return;
        while( ( text_len < byte_len ) && glyph_len )
        {
            if( unicode == '\n' )
            {
                /* new line separator so draw previous line */
                struct wp_rect label;
                label.y = pos_y + line_offset;
                label.h = row_height;
                label.w = line_width;
                label.x = pos_x;
                if( !line_count )
                    label.x += x_offset;

                if( is_selected ) /* selection needs to draw different background color */
                    wp_fill_rect( out, label, 0, background );
                wp_widget_text( out, label, line, (wp_s32)( ( text + text_len ) - line ), &txt,
                                WORKPHONE_TEXT_CENTERED, font );

                text_len++;
                line_count++;
                line_width = 0;
                line = text + text_len;
                line_offset += row_height;
                glyph_len = wp_utf_decode( text + text_len, &unicode, (wp_s32)( byte_len - text_len ) );
                continue;
            }
            if( unicode == '\r' )
            {
                text_len++;
                glyph_len = wp_utf_decode( text + text_len, &unicode, byte_len - text_len );
                continue;
            }
            glyph_width = font->width( font->userdata, font->height, text + text_len, glyph_len );
            line_width += (wp_f32)glyph_width;
            text_len += glyph_len;
            glyph_len = wp_utf_decode( text + text_len, &unicode, byte_len - text_len );
            continue;
        }
        if( line_width > 0 )
        {
            /* draw last line */
            struct wp_rect label;
            label.y = pos_y + line_offset;
            label.h = row_height;
            label.w = line_width;
            label.x = pos_x;
            if( !line_count )
                label.x += x_offset;

            if( is_selected )
                wp_fill_rect( out, label, 0, background );
            wp_widget_text( out, label, line, (wp_s32)( ( text + text_len ) - line ), &txt,
                            WORKPHONE_TEXT_LEFT, font );
        }
    }
}
WORKPHONE_LIB wp_flags wp_do_edit( wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds,
                                   wp_flags flags, wp_plugin_filter filter, struct wp_text_edit *edit,
                                   const struct wp_style_edit *style, struct wp_input *in,
                                   const struct wp_user_font *font )
{
    struct wp_rect area;
    wp_flags ret = 0;
    wp_f32 row_height;
    wp_c8 prev_state = 0;
    wp_c8 is_hovered = 0;
    wp_c8 select_all = 0;
    wp_c8 cursor_follow = 0;
    struct wp_rect old_clip;
    struct wp_rect clip;

    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( style );
    if( !state || !out || !style )
        return ret;

    /* visible text area calculation */
    area.x = bounds.x + style->padding.x + style->border;
    area.y = bounds.y + style->padding.y + style->border;
    area.w = bounds.w - ( 2.0f * style->padding.x + 2 * style->border );
    area.h = bounds.h - ( 2.0f * style->padding.y + 2 * style->border );
    if( flags & WORKPHONE_EDIT_MULTILINE )
        area.w = WORKPHONE_MAX( 0, area.w - style->scrollbar_size.x );
    row_height = ( flags & WORKPHONE_EDIT_MULTILINE ) ? font->height + style->row_padding : area.h;

    /* calculate clipping rectangle */
    old_clip = out->clip;
    wp_unify( &clip, &old_clip, area.x, area.y, area.x + area.w, area.y + area.h );

    /* update edit state */
    prev_state = (wp_c8)edit->active;
    if( in && in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked &&
        in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down )
    {
        edit->active =
            WORKPHONE_INBOX( in->mouse.pos.x, in->mouse.pos.y, bounds.x, bounds.y, bounds.w, bounds.h );
    }

    /* (de)activate text editor */
    if( !prev_state && edit->active )
    {
        const enum wp_text_edit_type type = ( flags & WORKPHONE_EDIT_MULTILINE )
                                                ? WORKPHONE_TEXT_EDIT_MULTI_LINE
                                                : WORKPHONE_TEXT_EDIT_SINGLE_LINE;
        /* keep scroll position when re-activating edit widget */
        struct wp_vec2f oldscrollbar = edit->scrollbar;
        wp_textedit_clear_state( edit, type, filter );
        edit->scrollbar = oldscrollbar;
        if( flags & WORKPHONE_EDIT_AUTO_SELECT )
            select_all = wp_true;
        if( flags & WORKPHONE_EDIT_GOTO_END_ON_ACTIVATE )
        {
            edit->cursor = edit->string.len;
            in = 0;
        }
    }
    else if( !edit->active )
        edit->mode = WORKPHONE_TEXT_EDIT_MODE_VIEW;
    if( flags & WORKPHONE_EDIT_READ_ONLY )
        edit->mode = WORKPHONE_TEXT_EDIT_MODE_VIEW;
    else if( flags & WORKPHONE_EDIT_ALWAYS_INSERT_MODE )
        edit->mode = WORKPHONE_TEXT_EDIT_MODE_INSERT;

    ret = ( edit->active ) ? WORKPHONE_EDIT_ACTIVE : WORKPHONE_EDIT_INACTIVE;
    if( prev_state != edit->active )
        ret |= ( edit->active ) ? WORKPHONE_EDIT_ACTIVATED : WORKPHONE_EDIT_DEACTIVATED;

    /* handle user input */
    if( edit->active && in )
    {
        wp_s32 shift_mod = in->keyboard.keys[WORKPHONE_KEY_SHIFT].down;
        const wp_f32 mouse_x = ( in->mouse.pos.x - area.x ) + edit->scrollbar.x;
        const wp_f32 mouse_y = ( in->mouse.pos.y - area.y ) + edit->scrollbar.y;

        /* mouse click handler */
        is_hovered = (wp_c8)wp_input_is_mouse_hovering_rect( in, area );
        if( select_all )
        {
            wp_textedit_select_all( edit );
        }
        else if( is_hovered && in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down &&
                 in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked )
        {
            wp_textedit_click( edit, mouse_x, mouse_y, font, row_height );
        }
        else if( is_hovered && in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down &&
                 wp_input_is_mouse_moved( in ) )
        {
            wp_textedit_drag( edit, mouse_x, mouse_y, font, row_height );
            cursor_follow = wp_true;
        }
        else if( is_hovered && in->mouse.buttons[WORKPHONE_BUTTON_RIGHT].clicked &&
                 in->mouse.buttons[WORKPHONE_BUTTON_RIGHT].down )
        {
            wp_textedit_key( edit, WORKPHONE_KEY_TEXT_WORD_LEFT, wp_false, font, row_height );
            wp_textedit_key( edit, WORKPHONE_KEY_TEXT_WORD_RIGHT, wp_true, font, row_height );
            cursor_follow = wp_true;
        }

        {
            wp_s32 i; /* keyboard input */
            wp_s32 old_mode = edit->mode;
            for( i = 0; i < WORKPHONE_KEY_MAX; ++i )
            {
                if( i == WORKPHONE_KEY_ENTER || i == WORKPHONE_KEY_TAB )
                    continue; /* special case */
                if( wp_input_is_key_pressed( in, (enum wp_keys)i ) )
                {
                    wp_textedit_key( edit, (enum wp_keys)i, shift_mod, font, row_height );
                    cursor_follow = wp_true;
                }
            }
            if( old_mode != edit->mode )
            {
                in->keyboard.text_len = 0;
            }
        }

        /* text input */
        edit->filter = filter;
        if( in->keyboard.text_len )
        {
            wp_textedit_text( edit, in->keyboard.text, in->keyboard.text_len );
            cursor_follow = wp_true;
            in->keyboard.text_len = 0;
        }

        /* enter key handler */
        if( wp_input_is_key_pressed( in, WORKPHONE_KEY_ENTER ) )
        {
            cursor_follow = wp_true;
            if( flags & WORKPHONE_EDIT_CTRL_ENTER_NEWLINE && shift_mod )
                wp_textedit_text( edit, "\n", 1 );
            else if( flags & WORKPHONE_EDIT_SIG_ENTER )
                ret |= WORKPHONE_EDIT_COMMITED;
            else
                wp_textedit_text( edit, "\n", 1 );
        }

        /* cut & copy handler */
        {
            wp_s32 copy = wp_input_is_key_pressed( in, WORKPHONE_KEY_COPY );
            wp_s32 cut = wp_input_is_key_pressed( in, WORKPHONE_KEY_CUT );
            if( ( copy || cut ) && ( flags & WORKPHONE_EDIT_CLIPBOARD ) )
            {
                wp_s32 glyph_len;
                wp_rune unicode;
                const wp_c8 *text;
                wp_s32 b = edit->select_start;
                wp_s32 e = edit->select_end;

                wp_s32 begin = WORKPHONE_MIN( b, e );
                wp_s32 end = WORKPHONE_MAX( b, e );
                text = wp_str_at_const( &edit->string, begin, &unicode, &glyph_len );
                if( edit->clip.copy )
                    edit->clip.copy( edit->clip.userdata, text, end - begin );
                if( cut && !( flags & WORKPHONE_EDIT_READ_ONLY ) )
                {
                    wp_textedit_cut( edit );
                    cursor_follow = wp_true;
                }
            }
        }

        /* paste handler */
        {
            wp_s32 paste = wp_input_is_key_pressed( in, WORKPHONE_KEY_PASTE );
            if( paste && ( flags & WORKPHONE_EDIT_CLIPBOARD ) && edit->clip.paste )
            {
                edit->clip.paste( edit->clip.userdata, edit );
                cursor_follow = wp_true;
            }
        }

        /* tab handler */
        {
            wp_s32 tab = wp_input_is_key_pressed( in, WORKPHONE_KEY_TAB );
            if( tab && ( flags & WORKPHONE_EDIT_ALLOW_TAB ) )
            {
                wp_textedit_text( edit, "    ", 4 );
                cursor_follow = wp_true;
            }
        }
    }

    /* set widget state */
    if( edit->active )
        *state = WORKPHONE_WIDGET_STATE_ACTIVE;
    else
        wp_widget_state_reset( state );

    if( is_hovered )
        *state |= WORKPHONE_WIDGET_STATE_HOVERED;

    /* DRAW EDIT */
    {
        const wp_c8 *text = wp_str_get_const( &edit->string );
        wp_s32 len = wp_str_len_wp_c8( &edit->string );

        { /* select background colors/images  */
            const struct wp_style_item *background;
            if( *state & WORKPHONE_WIDGET_STATE_ACTIVED )
                background = &style->active;
            else if( *state & WORKPHONE_WIDGET_STATE_HOVER )
                background = &style->hover;
            else
                background = &style->normal;

            /* draw background frame */
            switch( background->type )
            {
            case WORKPHONE_STYLE_ITEM_IMAGE:
                wp_draw_image( out, bounds, &background->data.image,
                               wp_rgb_factor( wp_white, style->color_factor ) );
                break;
            case WORKPHONE_STYLE_ITEM_NINE_SLICE:
                wp_draw_nine_slice( out, bounds, &background->data.slice,
                                    wp_rgb_factor( wp_white, style->color_factor ) );
                break;
            case WORKPHONE_STYLE_ITEM_COLOR:
                wp_fill_rect( out, bounds, style->rounding,
                              wp_rgb_factor( background->data.color, style->color_factor ) );
                wp_stroke_rect( out, bounds, style->rounding, style->border,
                                wp_rgb_factor( style->border_color, style->color_factor ) );
                break;
            }
        }

        area.w = WORKPHONE_MAX( 0, area.w - style->cursor_size );
        if( edit->active )
        {
            wp_s32 total_lines = 1;
            struct wp_vec2f text_size = wp_make_vec2f( 0, 0 );

            /* text pointer positions */
            const wp_c8 *cursor_ptr = 0;
            const wp_c8 *select_begin_ptr = 0;
            const wp_c8 *select_end_ptr = 0;

            /* 2D pixel positions */
            struct wp_vec2f cursor_pos = wp_make_vec2f( 0, 0 );
            struct wp_vec2f selection_offset_start = wp_make_vec2f( 0, 0 );
            struct wp_vec2f selection_offset_end = wp_make_vec2f( 0, 0 );

            wp_s32 selection_begin = WORKPHONE_MIN( edit->select_start, edit->select_end );
            wp_s32 selection_end = WORKPHONE_MAX( edit->select_start, edit->select_end );

            /* calculate total line count + total space + cursor/selection position */
            wp_f32 line_width = 0.0f;
            if( text && len )
            {
                /* utf8 encoding */
                wp_f32 glyph_width;
                wp_s32 glyph_len = 0;
                wp_rune unicode = 0;
                wp_s32 text_len = 0;
                wp_s32 glyphs = 0;
                wp_s32 row_begin = 0;

                glyph_len = wp_utf_decode( text, &unicode, len );
                glyph_width = font->width( font->userdata, font->height, text, glyph_len );
                line_width = 0;

                /* iterate all lines */
                while( ( text_len < len ) && glyph_len )
                {
                    /* set cursor 2D position and line */
                    if( !cursor_ptr && glyphs == edit->cursor )
                    {
                        wp_s32 glyph_offset;
                        struct wp_vec2f out_offset;
                        struct wp_vec2f row_size;
                        const wp_c8 *remaining;

                        /* calculate 2d position */
                        cursor_pos.y = (wp_f32)( total_lines - 1 ) * row_height;
                        row_size = wp_text_calculate_text_bounds(
                            font, text + row_begin, text_len - row_begin, row_height, &remaining,
                            &out_offset, &glyph_offset, WORKPHONE_STOP_ON_NEW_LINE );
                        cursor_pos.x = row_size.x;
                        cursor_ptr = text + text_len;
                    }

                    /* set start selection 2D position and line */
                    if( !select_begin_ptr && edit->select_start != edit->select_end &&
                        glyphs == selection_begin )
                    {
                        wp_s32 glyph_offset;
                        struct wp_vec2f out_offset;
                        struct wp_vec2f row_size;
                        const wp_c8 *remaining;

                        /* calculate 2d position */
                        selection_offset_start.y =
                            (wp_f32)( WORKPHONE_MAX( total_lines - 1, 0 ) ) * row_height;
                        row_size = wp_text_calculate_text_bounds(
                            font, text + row_begin, text_len - row_begin, row_height, &remaining,
                            &out_offset, &glyph_offset, WORKPHONE_STOP_ON_NEW_LINE );
                        selection_offset_start.x = row_size.x;
                        select_begin_ptr = text + text_len;
                    }

                    /* set end selection 2D position and line */
                    if( !select_end_ptr && edit->select_start != edit->select_end &&
                        glyphs == selection_end )
                    {
                        wp_s32 glyph_offset;
                        struct wp_vec2f out_offset;
                        struct wp_vec2f row_size;
                        const wp_c8 *remaining;

                        /* calculate 2d position */
                        selection_offset_end.y = (wp_f32)( total_lines - 1 ) * row_height;
                        row_size = wp_text_calculate_text_bounds(
                            font, text + row_begin, text_len - row_begin, row_height, &remaining,
                            &out_offset, &glyph_offset, WORKPHONE_STOP_ON_NEW_LINE );
                        selection_offset_end.x = row_size.x;
                        select_end_ptr = text + text_len;
                    }
                    if( unicode == '\n' )
                    {
                        text_size.x = WORKPHONE_MAX( text_size.x, line_width );
                        total_lines++;
                        line_width = 0;
                        text_len++;
                        glyphs++;
                        row_begin = text_len;
                        glyph_len = wp_utf_decode( text + text_len, &unicode, len - text_len );
                        glyph_width =
                            font->width( font->userdata, font->height, text + text_len, glyph_len );
                        continue;
                    }

                    glyphs++;
                    text_len += glyph_len;
                    line_width += (wp_f32)glyph_width;

                    glyph_len = wp_utf_decode( text + text_len, &unicode, len - text_len );
                    glyph_width =
                        font->width( font->userdata, font->height, text + text_len, glyph_len );
                    continue;
                }
                text_size.y = (wp_f32)total_lines * row_height;

                /* handle case when cursor is at end of text buffer */
                if( !cursor_ptr && edit->cursor == edit->string.len )
                {
                    cursor_pos.x = line_width;
                    cursor_pos.y = text_size.y - row_height;
                }
            }
            {
                /* scrollbar */
                if( cursor_follow )
                {
                    /* update scrollbar to follow cursor */
                    if( !( flags & WORKPHONE_EDIT_NO_HORIZONTAL_SCROLL ) )
                    {
                        /* horizontal scroll */
                        const wp_f32 scroll_increment = area.w * 0.25f;
                        if( cursor_pos.x < edit->scrollbar.x )
                            edit->scrollbar.x =
                                (wp_f32)(wp_s32)WORKPHONE_MAX( 0.0f, cursor_pos.x - scroll_increment );
                        if( cursor_pos.x >= edit->scrollbar.x + area.w )
                            edit->scrollbar.x = (wp_f32)(wp_s32)WORKPHONE_MAX(
                                0.0f, cursor_pos.x - area.w + scroll_increment );
                    }
                    else
                        edit->scrollbar.x = 0;

                    if( flags & WORKPHONE_EDIT_MULTILINE )
                    {
                        /* vertical scroll: like horizontal, it only adjusts if the
                         * cursor leaves the visible area, and then only just enough
                         * to keep it visible */
                        if( cursor_pos.y < edit->scrollbar.y )
                            edit->scrollbar.y = WORKPHONE_MAX( 0.0f, cursor_pos.y );
                        if( cursor_pos.y > edit->scrollbar.y + area.h - row_height )
                            edit->scrollbar.y = edit->scrollbar.y + row_height;
                    }
                    else
                        edit->scrollbar.y = 0;
                }

                /* scrollbar widget */
                if( flags & WORKPHONE_EDIT_MULTILINE )
                {
                    wp_flags ws;
                    struct wp_rect scroll;
                    wp_f32 scroll_target;
                    wp_f32 scroll_offset;
                    wp_f32 scroll_step;
                    wp_f32 scroll_inc;

                    scroll = area;
                    scroll.x = ( bounds.x + bounds.w - style->border ) - style->scrollbar_size.x;
                    scroll.w = style->scrollbar_size.x;

                    scroll_offset = edit->scrollbar.y;
                    scroll_step = scroll.h * 0.10f;
                    scroll_inc = scroll.h * 0.01f;
                    scroll_target = text_size.y;
                    edit->scrollbar.y =
                        wp_do_scrollbarv( &ws, out, scroll, is_hovered, scroll_offset, scroll_target,
                                          scroll_step, scroll_inc, &style->scrollbar, in, font );
                    /* Eat mouse scroll if we're active */
                    if( is_hovered && in->mouse.scroll_delta.y )
                    {
                        in->mouse.scroll_delta.y = 0;
                    }
                }
            }

            /* draw text */
            {
                struct wp_color background_color;
                struct wp_color text_color;
                struct wp_color sel_background_color;
                struct wp_color sel_text_color;
                struct wp_color cursor_color;
                struct wp_color cursor_text_color;
                const struct wp_style_item *background;
                wp_push_scissor( out, clip );

                /* select correct colors to draw */
                if( *state & WORKPHONE_WIDGET_STATE_ACTIVED )
                {
                    background = &style->active;
                    text_color = style->text_active;
                    sel_text_color = style->selected_text_hover;
                    sel_background_color = style->selected_hover;
                    cursor_color = style->cursor_hover;
                    cursor_text_color = style->cursor_text_hover;
                }
                else if( *state & WORKPHONE_WIDGET_STATE_HOVER )
                {
                    background = &style->hover;
                    text_color = style->text_hover;
                    sel_text_color = style->selected_text_hover;
                    sel_background_color = style->selected_hover;
                    cursor_text_color = style->cursor_text_hover;
                    cursor_color = style->cursor_hover;
                }
                else
                {
                    background = &style->normal;
                    text_color = style->text_normal;
                    sel_text_color = style->selected_text_normal;
                    sel_background_color = style->selected_normal;
                    cursor_color = style->cursor_normal;
                    cursor_text_color = style->cursor_text_normal;
                }
                if( background->type == WORKPHONE_STYLE_ITEM_IMAGE )
                    background_color = wp_rgba( 0, 0, 0, 0 );
                else
                    background_color = background->data.color;

                cursor_color = wp_rgb_factor( cursor_color, style->color_factor );
                cursor_text_color = wp_rgb_factor( cursor_text_color, style->color_factor );

                if( edit->select_start == edit->select_end )
                {
                    /* no selection so just draw the complete text */
                    const wp_c8 *begin = wp_str_get_const( &edit->string );
                    wp_s32 l = wp_str_len_wp_c8( &edit->string );
                    wp_edit_draw_text( out, style, area.x - edit->scrollbar.x,
                                       area.y - edit->scrollbar.y, 0, begin, l, row_height, font,
                                       background_color, text_color, wp_false );
                }
                else
                {
                    /* edit has selection so draw 1-3 text chunks */
                    if( edit->select_start != edit->select_end && selection_begin > 0 )
                    {
                        /* draw unselected text before selection */
                        const wp_c8 *begin = wp_str_get_const( &edit->string );
                        WORKPHONE_ASSERT( select_begin_ptr );
                        wp_edit_draw_text( out, style, area.x - edit->scrollbar.x,
                                           area.y - edit->scrollbar.y, 0, begin,
                                           (wp_s32)( select_begin_ptr - begin ), row_height, font,
                                           background_color, text_color, wp_false );
                    }
                    if( edit->select_start != edit->select_end )
                    {
                        /* draw selected text */
                        WORKPHONE_ASSERT( select_begin_ptr );
                        if( !select_end_ptr )
                        {
                            const wp_c8 *begin = wp_str_get_const( &edit->string );
                            select_end_ptr = begin + wp_str_len_wp_c8( &edit->string );
                        }
                        wp_edit_draw_text( out, style, area.x - edit->scrollbar.x,
                                           area.y + selection_offset_start.y - edit->scrollbar.y,
                                           selection_offset_start.x, select_begin_ptr,
                                           (wp_s32)( select_end_ptr - select_begin_ptr ), row_height,
                                           font, sel_background_color, sel_text_color, wp_true );
                    }
                    if( ( edit->select_start != edit->select_end && selection_end < edit->string.len ) )
                    {
                        /* draw unselected text after selected text */
                        const wp_c8 *begin = select_end_ptr;
                        const wp_c8 *end =
                            wp_str_get_const( &edit->string ) + wp_str_len_wp_c8( &edit->string );
                        WORKPHONE_ASSERT( select_end_ptr );
                        wp_edit_draw_text( out, style, area.x - edit->scrollbar.x,
                                           area.y + selection_offset_end.y - edit->scrollbar.y,
                                           selection_offset_end.x, begin, (wp_s32)( end - begin ),
                                           row_height, font, background_color, text_color, wp_true );
                    }
                }

                /* cursor */
                if( edit->select_start == edit->select_end )
                {
                    if( edit->cursor >= wp_str_len( &edit->string ) ||
                        ( cursor_ptr && *cursor_ptr == '\n' ) )
                    {
                        /* draw cursor at end of line */
                        struct wp_rect cursor;
                        cursor.w = style->cursor_size;
                        cursor.h = font->height;
                        cursor.x = area.x + cursor_pos.x - edit->scrollbar.x;
                        cursor.y = area.y + cursor_pos.y + row_height / 2.0f - cursor.h / 2.0f;
                        cursor.y -= edit->scrollbar.y;
                        wp_fill_rect( out, cursor, 0, cursor_color );
                    }
                    else
                    {
                        /* draw cursor inside text */
                        wp_s32 glyph_len;
                        struct wp_rect label;
                        struct wp_text txt;

                        wp_rune unicode;
                        WORKPHONE_ASSERT( cursor_ptr );
                        glyph_len = wp_utf_decode( cursor_ptr, &unicode, 4 );

                        label.x = area.x + cursor_pos.x - edit->scrollbar.x;
                        label.y = area.y + cursor_pos.y - edit->scrollbar.y;
                        label.w = font->width( font->userdata, font->height, cursor_ptr, glyph_len );
                        label.h = row_height;

                        txt.padding = wp_make_vec2f( 0, 0 );
                        txt.background = cursor_color;
                        ;
                        txt.text = cursor_text_color;
                        wp_fill_rect( out, label, 0, cursor_color );
                        wp_widget_text( out, label, cursor_ptr, glyph_len, &txt, WORKPHONE_TEXT_LEFT,
                                        font );
                    }
                }
            }
        }
        else
        {
            /* not active so just draw text */
            wp_s32 l = wp_str_len_wp_c8( &edit->string );
            const wp_c8 *begin = wp_str_get_const( &edit->string );

            const struct wp_style_item *background;
            struct wp_color background_color;
            struct wp_color text_color;
            wp_push_scissor( out, clip );
            if( *state & WORKPHONE_WIDGET_STATE_ACTIVED )
            {
                background = &style->active;
                text_color = style->text_active;
            }
            else if( *state & WORKPHONE_WIDGET_STATE_HOVER )
            {
                background = &style->hover;
                text_color = style->text_hover;
            }
            else
            {
                background = &style->normal;
                text_color = style->text_normal;
            }
            if( background->type == WORKPHONE_STYLE_ITEM_IMAGE )
                background_color = wp_rgba( 0, 0, 0, 0 );
            else
                background_color = background->data.color;

            background_color = wp_rgb_factor( background_color, style->color_factor );
            text_color = wp_rgb_factor( text_color, style->color_factor );

            wp_edit_draw_text( out, style, area.x - edit->scrollbar.x, area.y - edit->scrollbar.y, 0,
                               begin, l, row_height, font, background_color, text_color, wp_false );
        }
        wp_push_scissor( out, old_clip );
    }
    return ret;
}
WORKPHONE_API void wp_edit_focus( struct wp_context *ctx, wp_flags flags )
{
    wp_hash hash;
    struct wp_window *win;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return;

    win = ctx->current;
    hash = win->edit.seq;
    win->edit.active = wp_true;
    win->edit.name = hash;
    if( flags & WORKPHONE_EDIT_ALWAYS_INSERT_MODE )
        win->edit.mode = WORKPHONE_TEXT_EDIT_MODE_INSERT;
}
WORKPHONE_API void wp_edit_unfocus( struct wp_context *ctx )
{
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return;

    win = ctx->current;
    win->edit.active = wp_false;
    win->edit.name = 0;
}
WORKPHONE_API wp_flags wp_edit_string( struct wp_context *ctx, wp_flags flags, wp_c8 *memory,
                                       wp_s32 *len, wp_s32 max, wp_plugin_filter filter )
{
    wp_hash hash;
    wp_flags state;
    struct wp_text_edit *edit;
    struct wp_window *win;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( memory );
    WORKPHONE_ASSERT( len );
    if( !ctx || !memory || !len )
        return 0;

    filter = ( !filter ) ? wp_filter_default : filter;
    win = ctx->current;
    hash = win->edit.seq;
    edit = &ctx->text_edit;
    wp_textedit_clear_state( &ctx->text_edit,
                             ( flags & WORKPHONE_EDIT_MULTILINE ) ? WORKPHONE_TEXT_EDIT_MULTI_LINE
                                                                  : WORKPHONE_TEXT_EDIT_SINGLE_LINE,
                             filter );

    if( win->edit.active && hash == win->edit.name )
    {
        if( flags & WORKPHONE_EDIT_NO_CURSOR )
            edit->cursor = wp_utf_len( memory, *len );
        else
            edit->cursor = win->edit.cursor;
        if( !( flags & WORKPHONE_EDIT_SELECTABLE ) )
        {
            edit->select_start = win->edit.cursor;
            edit->select_end = win->edit.cursor;
        }
        else
        {
            edit->select_start = win->edit.sel_start;
            edit->select_end = win->edit.sel_end;
        }
        edit->mode = win->edit.mode;
        edit->scrollbar.x = (wp_f32)win->edit.scrollbar.x;
        edit->scrollbar.y = (wp_f32)win->edit.scrollbar.y;
        edit->active = wp_true;
    }
    else
        edit->active = wp_false;

    max = WORKPHONE_MAX( 1, max );
    *len = WORKPHONE_MIN( *len, max - 1 );
    wp_str_init_fixed( &edit->string, memory, (wp_size)max );
    edit->string.buffer.allocated = (wp_size)*len;
    edit->string.len = wp_utf_len( memory, *len );
    state = wp_edit_buffer( ctx, flags, edit, filter );
    *len = (wp_s32)edit->string.buffer.allocated;

    if( edit->active )
    {
        win->edit.cursor = edit->cursor;
        win->edit.sel_start = edit->select_start;
        win->edit.sel_end = edit->select_end;
        win->edit.mode = edit->mode;
        win->edit.scrollbar.x = (wp_u32)edit->scrollbar.x;
        win->edit.scrollbar.y = (wp_u32)edit->scrollbar.y;
    }
    return state;
}
WORKPHONE_API wp_flags wp_edit_buffer( struct wp_context *ctx, wp_flags flags, struct wp_text_edit *edit,
                                       wp_plugin_filter filter )
{
    struct wp_window *win;
    struct wp_style *style;
    struct wp_input *in;

    enum wp_widget_layout_states state;
    struct wp_rect bounds;

    wp_flags ret_flags = 0;
    wp_u8 prev_state;
    wp_hash hash;

    /* make sure correct values */
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( edit );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    state = wp_widget( &bounds, ctx );
    if( !state )
        return state;
    else if( state == WORKPHONE_WIDGET_DISABLED )
        flags |= WORKPHONE_EDIT_READ_ONLY;
    in = ( win->layout->flags & WORKPHONE_WINDOW_ROM ) ? 0 : &ctx->input;

    /* check if edit is currently hot item */
    hash = win->edit.seq++;
    if( win->edit.active && hash == win->edit.name )
    {
        if( flags & WORKPHONE_EDIT_NO_CURSOR )
            edit->cursor = edit->string.len;
        if( !( flags & WORKPHONE_EDIT_SELECTABLE ) )
        {
            edit->select_start = edit->cursor;
            edit->select_end = edit->cursor;
        }
        if( flags & WORKPHONE_EDIT_CLIPBOARD )
            edit->clip = ctx->clip;
        edit->active = (wp_u8)win->edit.active;
    }
    else
        edit->active = wp_false;
    edit->mode = win->edit.mode;

    filter = ( !filter ) ? wp_filter_default : filter;
    prev_state = (wp_u8)edit->active;
    in = ( flags & WORKPHONE_EDIT_READ_ONLY ) ? 0 : in;
    ret_flags = wp_do_edit( &ctx->last_widget_state, &win->buffer, bounds, flags, filter, edit,
                            &style->edit, in, style->font );

    if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
        ctx->style.cursor_active = ctx->style.cursors[WORKPHONE_CURSOR_TEXT];
    if( edit->active && prev_state != edit->active )
    {
        /* current edit is now hot */
        win->edit.active = wp_true;
        win->edit.name = hash;
    }
    else if( prev_state && !edit->active )
    {
        /* current edit is now cold */
        win->edit.active = wp_false;
    }
    return ret_flags;
}
WORKPHONE_API wp_flags wp_edit_string_zero_terminated( struct wp_context *ctx, wp_flags flags,
                                                       wp_c8 *buffer, wp_s32 max,
                                                       wp_plugin_filter filter )
{
    wp_flags result;
    wp_s32 len = wp_strlen( buffer );
    result = wp_edit_string( ctx, flags, buffer, &len, max, filter );
    buffer[WORKPHONE_MIN( WORKPHONE_MAX( max - 1, 0 ), len )] = '\0';
    return result;
}
