#include "workphone.h"
#include "workphone_font.h"
#include "workphone_utf8.h"

/* stb_textedit.h - v1.8  - public domain - Sean Barrett */
struct wp_text_find
{
    wp_f32 x, y;                /* position of n'th wp_c8acter */
    wp_f32 height;              /* height of line */
    wp_s32 first_wp_c8, length; /* first wp_c8 of row, and length */
    wp_s32 prev_first;          /*_ first wp_c8 of previous row */
};

struct wp_text_edit_row
{
    wp_f32 x0, x1;
    /* starting x location, end x location (allows for align=right, etc) */
    wp_f32 baseline_y_delta;
    /* position of baseline relative to previous row's baseline*/
    wp_f32 ymin, ymax;
    /* height of row above and below baseline */
    wp_s32 num_wp_c8s;
};

/* forward declarations */
WORKPHONE_INTERN void wp_textedit_makeundo_delete( struct wp_text_edit *, int, wp_s32 );
WORKPHONE_INTERN void wp_textedit_makeundo_insert( struct wp_text_edit *, int, wp_s32 );
WORKPHONE_INTERN void wp_textedit_makeundo_replace( struct wp_text_edit *, int, int, wp_s32 );
#define WORKPHONE_TEXT_HAS_SELECTION( s ) ( ( s )->select_start != ( s )->select_end )

WORKPHONE_INTERN wp_f32 wp_textedit_get_width( const struct wp_text_edit *edit, wp_s32 line_start,
                                               wp_s32 wp_c8_id, const struct wp_user_font *font )
{
    wp_s32 len = 0;
    wp_rune unicode = 0;
    const wp_c8 *str = wp_str_at_const( &edit->string, line_start + wp_c8_id, &unicode, &len );
    return font->width( font->userdata, font->height, str, len );
}
WORKPHONE_INTERN void wp_textedit_layout_row( struct wp_text_edit_row *r, struct wp_text_edit *edit,
                                              wp_s32 line_start_id, wp_f32 row_height,
                                              const struct wp_user_font *font )
{
    wp_s32 l;
    wp_s32 glyphs = 0;
    wp_rune unicode;
    const wp_c8 *remaining;
    wp_s32 len = wp_str_len_wp_c8( &edit->string );
    const wp_c8 *end = wp_str_get_const( &edit->string ) + len;
    const wp_c8 *text = wp_str_at_const( &edit->string, line_start_id, &unicode, &l );
    const struct wp_vec2f size =
        wp_text_calculate_text_bounds( font, text, (wp_s32)( end - text ), row_height, &remaining, 0,
                                       &glyphs, WORKPHONE_STOP_ON_NEW_LINE );

    r->x0 = 0.0f;
    r->x1 = size.x;
    r->baseline_y_delta = size.y;
    r->ymin = 0.0f;
    r->ymax = size.y;
    r->num_wp_c8s = glyphs;
}
WORKPHONE_INTERN wp_s32 wp_textedit_locate_coord( struct wp_text_edit *edit, wp_f32 x, wp_f32 y,
                                                  const struct wp_user_font *font, wp_f32 row_height )
{
    struct wp_text_edit_row r;
    wp_s32 n = edit->string.len;
    wp_f32 base_y = 0, prev_x;
    wp_s32 i = 0, k;

    r.x0 = r.x1 = 0;
    r.ymin = r.ymax = 0;
    r.num_wp_c8s = 0;

    /* search rows to find one that straddles 'y' */
    while( i < n )
    {
        wp_textedit_layout_row( &r, edit, i, row_height, font );
        if( r.num_wp_c8s <= 0 )
            return n;

        if( i == 0 && y < base_y + r.ymin )
            return 0;

        if( y < base_y + r.ymax )
            break;

        i += r.num_wp_c8s;
        base_y += r.baseline_y_delta;
    }

    /* below all text, return 'after' last wp_c8acter */
    if( i >= n )
        return n;

    /* check if it's before the beginning of the line */
    if( x < r.x0 )
        return i;

    /* check if it's before the end of the line */
    if( x < r.x1 )
    {
        /* search wp_c8acters in row for one that straddles 'x' */
        k = i;
        prev_x = r.x0;
        for( i = 0; i < r.num_wp_c8s; ++i )
        {
            wp_f32 w = wp_textedit_get_width( edit, k, i, font );
            if( x < prev_x + w )
            {
                if( x < prev_x + w / 2 )
                    return k + i;
                else
                    return k + i + 1;
            }
            prev_x += w;
        }
        /* shouldn't happen, but if it does, fall through to end-of-line case */
    }

    /* if the last wp_c8acter is a newline, return that.
     * otherwise return 'after' the last wp_c8acter */
    if( wp_str_rune_at( &edit->string, i + r.num_wp_c8s - 1 ) == '\n' )
        return i + r.num_wp_c8s - 1;
    else
        return i + r.num_wp_c8s;
}
WORKPHONE_LIB void wp_textedit_click( struct wp_text_edit *state, wp_f32 x, wp_f32 y,
                                      const struct wp_user_font *font, wp_f32 row_height )
{
    /* API click: on mouse down, move the cursor to the clicked location,
     * and reset the selection */
    state->cursor = wp_textedit_locate_coord( state, x, y, font, row_height );
    state->select_start = state->cursor;
    state->select_end = state->cursor;
    state->has_preferred_x = 0;
}
WORKPHONE_LIB void wp_textedit_drag( struct wp_text_edit *state, wp_f32 x, wp_f32 y,
                                     const struct wp_user_font *font, wp_f32 row_height )
{
    /* API drag: on mouse drag, move the cursor and selection endpoint
     * to the clicked location */
    wp_s32 p = wp_textedit_locate_coord( state, x, y, font, row_height );
    if( state->select_start == state->select_end )
        state->select_start = state->cursor;
    state->cursor = state->select_end = p;
}
WORKPHONE_INTERN void wp_textedit_find_wp_c8pos( struct wp_text_find *find, struct wp_text_edit *state,
                                                 wp_s32 n, wp_s32 single_line,
                                                 const struct wp_user_font *font, wp_f32 row_height )
{
    /* find the x/y location of a wp_c8acter, and remember info about the previous
     * row in case we get a move-up event (for page up, we'll have to rescan) */
    struct wp_text_edit_row r;
    wp_s32 prev_start = 0;
    wp_s32 z = state->string.len;
    wp_s32 i = 0, first;

    wp_zero_struct( r );
    if( n == z )
    {
        /* if it's at the end, then find the last line -- simpler than trying to
        explicitly handle this case in the regular code */
        wp_textedit_layout_row( &r, state, 0, row_height, font );
        if( single_line )
        {
            find->first_wp_c8 = 0;
            find->length = z;
        }
        else
        {
            while( i < z )
            {
                prev_start = i;
                i += r.num_wp_c8s;
                wp_textedit_layout_row( &r, state, i, row_height, font );
            }

            find->first_wp_c8 = i;
            find->length = r.num_wp_c8s;
        }
        find->x = r.x1;
        find->y = r.ymin;
        find->height = r.ymax - r.ymin;
        find->prev_first = prev_start;
        return;
    }

    /* search rows to find the one that straddles wp_c8acter n */
    find->y = 0;

    for( ;; )
    {
        wp_textedit_layout_row( &r, state, i, row_height, font );
        if( n < i + r.num_wp_c8s )
            break;
        prev_start = i;
        i += r.num_wp_c8s;
        find->y += r.baseline_y_delta;
    }

    find->first_wp_c8 = first = i;
    find->length = r.num_wp_c8s;
    find->height = r.ymax - r.ymin;
    find->prev_first = prev_start;

    /* now scan to find xpos */
    find->x = r.x0;
    for( i = 0; first + i < n; ++i )
        find->x += wp_textedit_get_width( state, first, i, font );
}
WORKPHONE_INTERN void wp_textedit_clamp( struct wp_text_edit *state )
{
    /* make the selection/cursor state valid if client altered the string */
    wp_s32 n = state->string.len;
    if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
    {
        if( state->select_start > n )
            state->select_start = n;
        if( state->select_end > n )
            state->select_end = n;
        /* if clamping forced them to be equal, move the cursor to match */
        if( state->select_start == state->select_end )
            state->cursor = state->select_start;
    }
    if( state->cursor > n )
        state->cursor = n;
}
WORKPHONE_API void wp_textedit_delete( struct wp_text_edit *state, wp_s32 where, wp_s32 len )
{
    /* delete wp_c8acters while updating undo */
    wp_textedit_makeundo_delete( state, where, len );
    wp_str_delete_runes( &state->string, where, len );
    state->has_preferred_x = 0;
}
WORKPHONE_API void wp_textedit_delete_selection( struct wp_text_edit *state )
{
    /* delete the section */
    wp_textedit_clamp( state );
    if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
    {
        if( state->select_start < state->select_end )
        {
            wp_textedit_delete( state, state->select_start, state->select_end - state->select_start );
            state->select_end = state->cursor = state->select_start;
        }
        else
        {
            wp_textedit_delete( state, state->select_end, state->select_start - state->select_end );
            state->select_start = state->cursor = state->select_end;
        }
        state->has_preferred_x = 0;
    }
}
WORKPHONE_INTERN void wp_textedit_sortselection( struct wp_text_edit *state )
{
    /* canonicalize the selection so start <= end */
    if( state->select_end < state->select_start )
    {
        wp_s32 temp = state->select_end;
        state->select_end = state->select_start;
        state->select_start = temp;
    }
}
WORKPHONE_INTERN void wp_textedit_move_to_first( struct wp_text_edit *state )
{
    /* move cursor to first wp_c8acter of selection */
    if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
    {
        wp_textedit_sortselection( state );
        state->cursor = state->select_start;
        state->select_end = state->select_start;
        state->has_preferred_x = 0;
    }
}
WORKPHONE_INTERN void wp_textedit_move_to_last( struct wp_text_edit *state )
{
    /* move cursor to last wp_c8acter of selection */
    if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
    {
        wp_textedit_sortselection( state );
        wp_textedit_clamp( state );
        state->cursor = state->select_end;
        state->select_start = state->select_end;
        state->has_preferred_x = 0;
    }
}
WORKPHONE_INTERN wp_s32 wp_is_word_boundary( struct wp_text_edit *state, wp_s32 idx )
{
    wp_s32 len;
    wp_rune c;
    if( idx < 0 )
        return 1;
    if( !wp_str_at_rune( &state->string, idx, &c, &len ) )
        return 1;
#ifndef WORKPHONE_IS_WORD_BOUNDARY
    return ( c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v' || c == 0x3000 );
#else
    return WORKPHONE_IS_WORD_BOUNDARY( c );
#endif
}
WORKPHONE_INTERN wp_s32 wp_textedit_move_to_word_previous( struct wp_text_edit *state )
{
    wp_s32 c = state->cursor - 1;
    if( c > 0 )
    {
        if( wp_is_word_boundary( state, c ) )
        {
            while( c > 0 && wp_is_word_boundary( state, --c ) )
                ;
        }
        while( !wp_is_word_boundary( state, --c ) )
            ;
        c++;
    }
    else
    {
        return 0;
    }

    return c;
}
WORKPHONE_INTERN wp_s32 wp_textedit_move_to_word_next( struct wp_text_edit *state )
{
    const wp_s32 len = state->string.len;
    wp_s32 c = state->cursor;
    if( c < len )
    {
        if( !wp_is_word_boundary( state, c ) )
        {
            while( c < len && !wp_is_word_boundary( state, ++c ) )
                ;
        }
        while( c < len && wp_is_word_boundary( state, ++c ) )
            ;
    }
    else
    {
        return len;
    }

    return c;
}
WORKPHONE_INTERN void wp_textedit_prep_selection_at_cursor( struct wp_text_edit *state )
{
    /* update selection and cursor to match each other */
    if( !WORKPHONE_TEXT_HAS_SELECTION( state ) )
        state->select_start = state->select_end = state->cursor;
    else
        state->cursor = state->select_end;
}
WORKPHONE_API wp_bool wp_textedit_cut( struct wp_text_edit *state )
{
    /* API cut: delete selection */
    if( state->mode == WORKPHONE_TEXT_EDIT_MODE_VIEW )
        return 0;
    if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
    {
        wp_textedit_delete_selection( state ); /* implicitly clamps */
        state->has_preferred_x = 0;
        return 1;
    }
    return 0;
}
WORKPHONE_API wp_bool wp_textedit_paste( struct wp_text_edit *state, wp_c8 const *ctext, wp_s32 len )
{
    /* API paste: replace existing selection with passed-in text */
    wp_s32 glyphs;
    const wp_c8 *text = (const wp_c8 *)ctext;
    if( state->mode == WORKPHONE_TEXT_EDIT_MODE_VIEW )
        return 0;

    /* if there's a selection, the paste should delete it */
    wp_textedit_clamp( state );
    wp_textedit_delete_selection( state );

    /* try to insert the wp_c8acters */
    glyphs = wp_utf_len( ctext, len );
    if( wp_str_insert_text_wp_c8( &state->string, state->cursor, text, len ) )
    {
        wp_textedit_makeundo_insert( state, state->cursor, glyphs );
        state->cursor += len;
        state->has_preferred_x = 0;
        return 1;
    }
    /* remove the undo since we didn't actually insert the wp_c8acters */
    if( state->undo.undo_point )
        --state->undo.undo_point;

    return 0;
}
WORKPHONE_API void wp_textedit_text( struct wp_text_edit *state, const wp_c8 *text, wp_s32 total_len )
{
    wp_rune unicode;
    wp_s32 glyph_len;
    wp_s32 text_len = 0;

    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( text );
    if( !text || !total_len || state->mode == WORKPHONE_TEXT_EDIT_MODE_VIEW )
        return;

    glyph_len = wp_utf_decode( text, &unicode, total_len );
    while( ( text_len < total_len ) && glyph_len )
    {
        /* don't insert a backward delete, just process the event */
        if( unicode == 127 )
            goto next;
        /* can't add newline in single-line mode */
        if( unicode == '\n' && state->single_line )
            goto next;
        /* filter incoming text */
        if( state->filter && !state->filter( state, unicode ) )
            goto next;

        if( !WORKPHONE_TEXT_HAS_SELECTION( state ) && state->cursor < state->string.len )
        {
            if( state->mode == WORKPHONE_TEXT_EDIT_MODE_REPLACE )
            {
                wp_textedit_makeundo_replace( state, state->cursor, 1, 1 );
                wp_str_delete_runes( &state->string, state->cursor, 1 );
            }
            if( wp_str_insert_text_utf8( &state->string, state->cursor, text + text_len, 1 ) )
            {
                ++state->cursor;
                state->has_preferred_x = 0;
            }
        }
        else
        {
            wp_textedit_delete_selection( state ); /* implicitly clamps */
            if( wp_str_insert_text_utf8( &state->string, state->cursor, text + text_len, 1 ) )
            {
                wp_textedit_makeundo_insert( state, state->cursor, 1 );
                state->cursor = WORKPHONE_MIN( state->cursor + 1, state->string.len );
                state->has_preferred_x = 0;
            }
        }
    next:
        text_len += glyph_len;
        glyph_len = wp_utf_decode( text + text_len, &unicode, total_len - text_len );
    }
}
WORKPHONE_LIB void wp_textedit_key( struct wp_text_edit *state, enum wp_keys key, wp_s32 shift_mod,
                                    const struct wp_user_font *font, wp_f32 row_height )
{
retry:
    switch( key )
    {
    case WORKPHONE_KEY_NONE:
    case WORKPHONE_KEY_CTRL:
    case WORKPHONE_KEY_ENTER:
    case WORKPHONE_KEY_SHIFT:
    case WORKPHONE_KEY_TAB:
    case WORKPHONE_KEY_COPY:
    case WORKPHONE_KEY_CUT:
    case WORKPHONE_KEY_PASTE:
    case WORKPHONE_KEY_MAX:
    default:
        break;
    case WORKPHONE_KEY_TEXT_UNDO:
        wp_textedit_undo( state );
        state->has_preferred_x = 0;
        break;

    case WORKPHONE_KEY_TEXT_REDO:
        wp_textedit_redo( state );
        state->has_preferred_x = 0;
        break;

    case WORKPHONE_KEY_TEXT_SELECT_ALL:
        wp_textedit_select_all( state );
        state->has_preferred_x = 0;
        break;

    case WORKPHONE_KEY_TEXT_INSERT_MODE:
        state->mode = WORKPHONE_TEXT_EDIT_MODE_INSERT;
        break;
    case WORKPHONE_KEY_TEXT_REPLACE_MODE:
        state->mode = WORKPHONE_TEXT_EDIT_MODE_REPLACE;
        break;
    case WORKPHONE_KEY_TEXT_RESET_MODE:
        state->mode = WORKPHONE_TEXT_EDIT_MODE_VIEW;
        break;

    case WORKPHONE_KEY_LEFT:
        if( shift_mod )
        {
            wp_textedit_clamp( state );
            wp_textedit_prep_selection_at_cursor( state );
            /* move selection left */
            if( state->select_end > 0 )
                --state->select_end;
            state->cursor = state->select_end;
            state->has_preferred_x = 0;
        }
        else
        {
            /* if currently there's a selection,
             * move cursor to start of selection */
            if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
                wp_textedit_move_to_first( state );
            else if( state->cursor > 0 )
                --state->cursor;
            state->has_preferred_x = 0;
        }
        break;

    case WORKPHONE_KEY_RIGHT:
        if( shift_mod )
        {
            wp_textedit_prep_selection_at_cursor( state );
            /* move selection right */
            ++state->select_end;
            wp_textedit_clamp( state );
            state->cursor = state->select_end;
            state->has_preferred_x = 0;
        }
        else
        {
            /* if currently there's a selection,
             * move cursor to end of selection */
            if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
                wp_textedit_move_to_last( state );
            else
                ++state->cursor;
            wp_textedit_clamp( state );
            state->has_preferred_x = 0;
        }
        break;

    case WORKPHONE_KEY_TEXT_WORD_LEFT:
        if( shift_mod )
        {
            if( !WORKPHONE_TEXT_HAS_SELECTION( state ) )
                wp_textedit_prep_selection_at_cursor( state );
            state->cursor = wp_textedit_move_to_word_previous( state );
            state->select_end = state->cursor;
            wp_textedit_clamp( state );
        }
        else
        {
            if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
                wp_textedit_move_to_first( state );
            else
            {
                state->cursor = wp_textedit_move_to_word_previous( state );
                wp_textedit_clamp( state );
            }
        }
        break;

    case WORKPHONE_KEY_TEXT_WORD_RIGHT:
        if( shift_mod )
        {
            if( !WORKPHONE_TEXT_HAS_SELECTION( state ) )
                wp_textedit_prep_selection_at_cursor( state );
            state->cursor = wp_textedit_move_to_word_next( state );
            state->select_end = state->cursor;
            wp_textedit_clamp( state );
        }
        else
        {
            if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
                wp_textedit_move_to_last( state );
            else
            {
                state->cursor = wp_textedit_move_to_word_next( state );
                wp_textedit_clamp( state );
            }
        }
        break;

    case WORKPHONE_KEY_DOWN:
    {
        struct wp_text_find find;
        struct wp_text_edit_row row;
        wp_s32 i, sel = shift_mod;

        if( state->single_line )
        {
            /* on windows, up&down in single-line behave like left&right */
            key = WORKPHONE_KEY_RIGHT;
            goto retry;
        }

        if( sel )
            wp_textedit_prep_selection_at_cursor( state );
        else if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
            wp_textedit_move_to_last( state );

        /* compute current position of cursor powp_s32 */
        wp_textedit_clamp( state );
        wp_textedit_find_wp_c8pos( &find, state, state->cursor, state->single_line, font, row_height );

        /* now find wp_c8acter position down a row */
        if( find.length )
        {
            wp_f32 x;
            wp_f32 goal_x = state->has_preferred_x ? state->preferred_x : find.x;
            wp_s32 start = find.first_wp_c8 + find.length;

            state->cursor = start;
            wp_textedit_layout_row( &row, state, state->cursor, row_height, font );
            x = row.x0;

            for( i = 0; i < row.num_wp_c8s && x < row.x1; ++i )
            {
                wp_f32 dx = wp_textedit_get_width( state, start, i, font );
                x += dx;
                if( x > goal_x )
                    break;
                ++state->cursor;
            }
            wp_textedit_clamp( state );

            state->has_preferred_x = 1;
            state->preferred_x = goal_x;
            if( sel )
                state->select_end = state->cursor;
        }
    }
    break;

    case WORKPHONE_KEY_UP:
    {
        struct wp_text_find find;
        struct wp_text_edit_row row;
        wp_s32 i, sel = shift_mod;

        if( state->single_line )
        {
            /* on windows, up&down become left&right */
            key = WORKPHONE_KEY_LEFT;
            goto retry;
        }

        if( sel )
            wp_textedit_prep_selection_at_cursor( state );
        else if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
            wp_textedit_move_to_first( state );

        /* compute current position of cursor powp_s32 */
        wp_textedit_clamp( state );
        wp_textedit_find_wp_c8pos( &find, state, state->cursor, state->single_line, font, row_height );

        /* can only go up if there's a previous row */
        if( find.prev_first != find.first_wp_c8 )
        {
            /* now find wp_c8acter position up a row */
            wp_f32 x;
            wp_f32 goal_x = state->has_preferred_x ? state->preferred_x : find.x;

            state->cursor = find.prev_first;
            wp_textedit_layout_row( &row, state, state->cursor, row_height, font );
            x = row.x0;

            for( i = 0; i < row.num_wp_c8s && x < row.x1; ++i )
            {
                wp_f32 dx = wp_textedit_get_width( state, find.prev_first, i, font );
                x += dx;
                if( x > goal_x )
                    break;
                ++state->cursor;
            }
            wp_textedit_clamp( state );

            state->has_preferred_x = 1;
            state->preferred_x = goal_x;
            if( sel )
                state->select_end = state->cursor;
        }
    }
    break;

    case WORKPHONE_KEY_DEL:
        if( state->mode == WORKPHONE_TEXT_EDIT_MODE_VIEW )
            break;
        if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
            wp_textedit_delete_selection( state );
        else
        {
            wp_s32 n = state->string.len;
            if( state->cursor < n )
                wp_textedit_delete( state, state->cursor, 1 );
        }
        state->has_preferred_x = 0;
        break;

    case WORKPHONE_KEY_BACKSPACE:
        if( state->mode == WORKPHONE_TEXT_EDIT_MODE_VIEW )
            break;
        if( WORKPHONE_TEXT_HAS_SELECTION( state ) )
            wp_textedit_delete_selection( state );
        else
        {
            wp_textedit_clamp( state );
            if( state->cursor > 0 )
            {
                wp_textedit_delete( state, state->cursor - 1, 1 );
                --state->cursor;
            }
        }
        state->has_preferred_x = 0;
        break;

    case WORKPHONE_KEY_TEXT_START:
        if( shift_mod )
        {
            wp_textedit_prep_selection_at_cursor( state );
            state->cursor = state->select_end = 0;
            state->has_preferred_x = 0;
        }
        else
        {
            state->cursor = state->select_start = state->select_end = 0;
            state->has_preferred_x = 0;
        }
        break;

    case WORKPHONE_KEY_TEXT_END:
        if( shift_mod )
        {
            wp_textedit_prep_selection_at_cursor( state );
            state->cursor = state->select_end = state->string.len;
            state->has_preferred_x = 0;
        }
        else
        {
            state->cursor = state->string.len;
            state->select_start = state->select_end = 0;
            state->has_preferred_x = 0;
        }
        break;

    case WORKPHONE_KEY_TEXT_LINE_START:
    {
        if( shift_mod )
        {
            struct wp_text_find find;
            wp_textedit_clamp( state );
            wp_textedit_prep_selection_at_cursor( state );
            if( state->string.len && state->cursor == state->string.len )
                --state->cursor;
            wp_textedit_find_wp_c8pos( &find, state, state->cursor, state->single_line, font,
                                       row_height );
            state->cursor = state->select_end = find.first_wp_c8;
            state->has_preferred_x = 0;
        }
        else
        {
            struct wp_text_find find;
            if( state->string.len && state->cursor == state->string.len )
                --state->cursor;
            wp_textedit_clamp( state );
            wp_textedit_move_to_first( state );
            wp_textedit_find_wp_c8pos( &find, state, state->cursor, state->single_line, font,
                                       row_height );
            state->cursor = find.first_wp_c8;
            state->has_preferred_x = 0;
        }
    }
    break;

    case WORKPHONE_KEY_TEXT_LINE_END:
    {
        if( shift_mod )
        {
            struct wp_text_find find;
            wp_textedit_clamp( state );
            wp_textedit_prep_selection_at_cursor( state );
            wp_textedit_find_wp_c8pos( &find, state, state->cursor, state->single_line, font,
                                       row_height );
            state->has_preferred_x = 0;
            state->cursor = find.first_wp_c8 + find.length;
            if( find.length > 0 && wp_str_rune_at( &state->string, state->cursor - 1 ) == '\n' )
                --state->cursor;
            state->select_end = state->cursor;
        }
        else
        {
            struct wp_text_find find;
            wp_textedit_clamp( state );
            wp_textedit_move_to_first( state );
            wp_textedit_find_wp_c8pos( &find, state, state->cursor, state->single_line, font,
                                       row_height );

            state->has_preferred_x = 0;
            state->cursor = find.first_wp_c8 + find.length;
            if( find.length > 0 && wp_str_rune_at( &state->string, state->cursor - 1 ) == '\n' )
                --state->cursor;
        }
    }
    break;
    }
}

WORKPHONE_INTERN void wp_textedit_flush_redo( struct wp_text_undo_state *state )
{
    state->redo_char_point = WORKPHONE_TEXTEDIT_UNDOSTATECOUNT;
    state->redo_char_point = WORKPHONE_TEXTEDIT_UNDOCHARCOUNT;
}

WORKPHONE_INTERN void wp_textedit_discard_undo( struct wp_text_undo_state *state )
{
    /* discard the oldest entry in the undo list */
    if( state->undo_point > 0 )
    {
        /* if the 0th undo state has wp_c8acters, clean those up */
        if( state->undo_rec[0].char_storage >= 0 )
        {
            wp_s32 n = state->undo_rec[0].insert_length, i;
            /* delete n wp_c8acters from all other records */
            state->undo_char_point = (short)( state->undo_char_point - n );
            WORKPHONE_MEMCPY( state->undo_char, state->undo_char + n,
                              (wp_size)state->undo_char_point * sizeof( wp_rune ) );
            for( i = 0; i < state->undo_point; ++i )
            {
                if( state->undo_rec[i].char_storage >= 0 )
                    state->undo_rec[i].char_storage = (short)( state->undo_rec[i].char_storage - n );
            }
        }
        --state->undo_point;
        WORKPHONE_MEMCPY( state->undo_rec, state->undo_rec + 1,
                          (wp_size)( (wp_size)state->undo_point * sizeof( state->undo_rec[0] ) ) );
    }
}

WORKPHONE_INTERN void wp_textedit_discard_redo( struct wp_text_undo_state *state )
{
    /*  discard the oldest entry in the redo list--it's bad if this
        ever happens, but because undo & redo have to store the actual
        wp_c8acters in different cases, the redo wp_c8acter buffer can
        fill up even though the undo buffer didn't */
    wp_size num;
    wp_s32 k = WORKPHONE_TEXTEDIT_UNDOSTATECOUNT - 1;
    if( state->redo_char_point <= k )
    {
        /* if the k'th undo state has wp_c8acters, clean those up */
        if( state->undo_rec[k].char_storage >= 0 )
        {
            wp_s32 n = state->undo_rec[k].insert_length, i;
            /* delete n wp_c8acters from all other records */
            state->redo_char_point = (short)( state->redo_char_point + n );
            num = (wp_size)( WORKPHONE_TEXTEDIT_UNDOCHARCOUNT - state->redo_char_point );
            WORKPHONE_MEMCPY( state->undo_char + state->redo_char_point,
                              state->undo_char + state->redo_char_point - n, num * sizeof( wp_c8 ) );
            for( i = state->redo_point; i < k; ++i )
            {
                if( state->undo_rec[i].char_storage >= 0 )
                {
                    state->undo_rec[i].char_storage = (short)( state->undo_rec[i].char_storage + n );
                }
            }
        }
        ++state->redo_point;
        num = (wp_size)( WORKPHONE_TEXTEDIT_UNDOSTATECOUNT - state->redo_char_point );
        if( num )
            WORKPHONE_MEMCPY( state->undo_rec + state->redo_char_point - 1,
                              state->undo_rec + state->redo_point, num * sizeof( state->undo_rec[0] ) );
    }
}

WORKPHONE_INTERN struct wp_text_undo_record *wp_textedit_create_undo_record(
    struct wp_text_undo_state *state, wp_s32 numwp_c8s )
{
    /* any time we create a new undo record, we discard redo*/
    wp_textedit_flush_redo( state );

    /* if we have no free records, we have to make room,
     * by sliding the existing records down */
    if( state->undo_point == WORKPHONE_TEXTEDIT_UNDOSTATECOUNT )
        wp_textedit_discard_undo( state );

    /* if the wp_c8acters to store won't possibly fit in the buffer,
     * we can't undo */
    if( numwp_c8s > WORKPHONE_TEXTEDIT_UNDOCHARCOUNT )
    {
        state->undo_point = 0;
        state->undo_char_point = 0;
        return 0;
    }

    /* if we don't have enough free wp_c8acters in the buffer,
     * we have to make room */
    while( state->undo_char_point + numwp_c8s > WORKPHONE_TEXTEDIT_UNDOCHARCOUNT )
        wp_textedit_discard_undo( state );
    return &state->undo_rec[state->undo_point++];
}

WORKPHONE_INTERN wp_rune *wp_textedit_createundo( struct wp_text_undo_state *state, wp_s32 pos,
                                                  wp_s32 insert_len, wp_s32 delete_len )
{
    struct wp_text_undo_record *r = wp_textedit_create_undo_record( state, insert_len );
    if( r == 0 )
        return 0;

    r->where = pos;
    r->insert_length = (short)insert_len;
    r->delete_length = (short)delete_len;

    if( insert_len == 0 )
    {
        r->char_storage = -1;
        return 0;
    }
    else
    {
        r->char_storage = state->undo_char_point;
        state->undo_char_point = (short)( state->undo_char_point + insert_len );
        return &state->undo_char[r->char_storage];
    }
}

WORKPHONE_API void wp_textedit_undo( struct wp_text_edit *state )
{
    struct wp_text_undo_state *s = &state->undo;
    struct wp_text_undo_record u, *r;
    if( s->undo_point == 0 )
        return;

    /* we need to do two things: apply the undo record, and create a redo record */
    u = s->undo_rec[s->undo_point - 1];
    r = &s->undo_rec[s->redo_char_point - 1];
    r->char_storage = -1;

    r->insert_length = u.delete_length;
    r->delete_length = u.insert_length;
    r->where = u.where;

    if( u.delete_length )
    {
        /*   if the undo record says to delete wp_c8acters, then the redo record will
             need to re-insert the wp_c8acters that get deleted, so we need to store
             them.
             there are three cases:
                 - there's enough room to store the wp_c8acters
                 - wp_c8acters stored for *redoing* don't leave room for redo
                 - wp_c8acters stored for *undoing* don't leave room for redo
             if the last is true, we have to bail */
        if( s->undo_char_point + u.delete_length >= WORKPHONE_TEXTEDIT_UNDOCHARCOUNT )
        {
            /* the undo records take up too much wp_c8acter space; there's no space
             * to store the redo wp_c8acters */
            r->insert_length = 0;
        }
        else
        {
            wp_s32 i;
            /* there's definitely room to store the wp_c8acters eventually */
            while( s->undo_char_point + u.delete_length > s->redo_char_point )
            {
                /* there's currently not enough room, so discard a redo record */
                wp_textedit_discard_redo( s );
                /* should never happen: */
                if( s->redo_char_point == WORKPHONE_TEXTEDIT_UNDOSTATECOUNT )
                    return;
            }

            r = &s->undo_rec[s->redo_char_point - 1];
            r->char_storage = (short)( s->redo_char_point - u.delete_length );
            s->redo_char_point = (short)( s->redo_char_point - u.delete_length );

            /* now save the wp_c8acters */
            for( i = 0; i < u.delete_length; ++i )
                s->undo_char[r->char_storage + i] = wp_str_rune_at( &state->string, u.where + i );
        }
        /* now we can carry out the deletion */
        wp_str_delete_runes( &state->string, u.where, u.delete_length );
    }

    /* check type of recorded action: */
    if( u.insert_length )
    {
        /* easy case: was a deletion, so we need to insert n wp_c8acters */
        wp_str_insert_text_runes( &state->string, u.where, &s->undo_char[u.char_storage],
                                  u.insert_length );
        s->undo_char_point = (short)( s->undo_char_point - u.insert_length );
    }
    state->cursor = (short)( u.where + u.insert_length );

    s->undo_point--;
    s->redo_point--;
}

WORKPHONE_API void wp_textedit_redo( struct wp_text_edit *state )
{
    struct wp_text_undo_state *s = &state->undo;
    struct wp_text_undo_record *u, r;
    if( s->redo_char_point == WORKPHONE_TEXTEDIT_UNDOSTATECOUNT )
        return;

    /* we need to do two things: apply the redo record, and create an undo record */
    u = &s->undo_rec[s->undo_point];
    r = s->undo_rec[s->redo_point];

    /* we KNOW there must be room for the undo record, because the redo record
    was derived from an undo record */
    u->delete_length = r.insert_length;
    u->insert_length = r.delete_length;
    u->where = r.where;
    u->char_storage = -1;

    if( r.delete_length )
    {
        /* the redo record requires us to delete wp_c8acters, so the undo record
        needs to store the wp_c8acters */
        if( s->undo_char_point + u->insert_length > s->redo_char_point )
        {
            u->insert_length = 0;
            u->delete_length = 0;
        }
        else
        {
            wp_s32 i;
            u->char_storage = s->undo_char_point;
            s->undo_char_point = (short)( s->undo_char_point + u->insert_length );

            /* now save the wp_c8acters */
            for( i = 0; i < u->insert_length; ++i )
            {
                s->undo_char[u->char_storage + i] = wp_str_rune_at( &state->string, u->where + i );
            }
        }
        wp_str_delete_runes( &state->string, r.where, r.delete_length );
    }

    if( r.insert_length )
    {
        /* easy case: need to insert n wp_c8acters */
        wp_str_insert_text_runes( &state->string, r.where, &s->undo_char[r.char_storage],
                                  r.insert_length );
    }
    state->cursor = r.where + r.insert_length;

    s->undo_point++;
    s->redo_point++;
}
WORKPHONE_INTERN void wp_textedit_makeundo_insert( struct wp_text_edit *state, wp_s32 where,
                                                   wp_s32 length )
{
    wp_textedit_createundo( &state->undo, where, 0, length );
}
WORKPHONE_INTERN void wp_textedit_makeundo_delete( struct wp_text_edit *state, wp_s32 where,
                                                   wp_s32 length )
{
    wp_s32 i;
    wp_rune *p = wp_textedit_createundo( &state->undo, where, length, 0 );
    if( p )
    {
        for( i = 0; i < length; ++i )
            p[i] = wp_str_rune_at( &state->string, where + i );
    }
}
WORKPHONE_INTERN void wp_textedit_makeundo_replace( struct wp_text_edit *state, wp_s32 where,
                                                    wp_s32 old_length, wp_s32 new_length )
{
    wp_s32 i;
    wp_rune *p = wp_textedit_createundo( &state->undo, where, old_length, new_length );
    if( p )
    {
        for( i = 0; i < old_length; ++i )
            p[i] = wp_str_rune_at( &state->string, where + i );
    }
}
WORKPHONE_LIB void wp_textedit_clear_state( struct wp_text_edit *state, enum wp_text_edit_type type,
                                            wp_plugin_filter filter )
{
    /* reset the state to default */
    state->undo.undo_point = 0;
    state->undo.undo_char_point = 0;
    state->undo.redo_char_point = WORKPHONE_TEXTEDIT_UNDOSTATECOUNT;
    state->undo.redo_char_point = WORKPHONE_TEXTEDIT_UNDOCHARCOUNT;
    state->select_end = state->select_start = 0;
    state->cursor = 0;
    state->has_preferred_x = 0;
    state->preferred_x = 0;
    state->cursor_at_end_of_line = 0;
    state->initialized = 1;
    state->single_line = (wp_u8)( type == WORKPHONE_TEXT_EDIT_SINGLE_LINE );
    state->mode = WORKPHONE_TEXT_EDIT_MODE_VIEW;
    state->filter = filter;
    state->scrollbar = wp_make_vec2f( 0, 0 );
}
WORKPHONE_API void wp_textedit_init_fixed( struct wp_text_edit *state, void *memory, wp_size size )
{
    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( memory );
    if( !state || !memory || !size )
        return;
    WORKPHONE_MEMSET( state, 0, sizeof( struct wp_text_edit ) );
    wp_textedit_clear_state( state, WORKPHONE_TEXT_EDIT_SINGLE_LINE, 0 );
    wp_str_init_fixed( &state->string, memory, size );
}
WORKPHONE_API void wp_textedit_init( struct wp_text_edit *state, const struct wp_allocator *alloc,
                                     wp_size size )
{
    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( alloc );
    if( !state || !alloc )
        return;
    WORKPHONE_MEMSET( state, 0, sizeof( struct wp_text_edit ) );
    wp_textedit_clear_state( state, WORKPHONE_TEXT_EDIT_SINGLE_LINE, 0 );
    wp_str_init( &state->string, alloc, size );
}
#ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
WORKPHONE_API void wp_textedit_init_default( struct wp_text_edit *state )
{
    WORKPHONE_ASSERT( state );
    if( !state )
        return;
    WORKPHONE_MEMSET( state, 0, sizeof( struct wp_text_edit ) );
    wp_textedit_clear_state( state, WORKPHONE_TEXT_EDIT_SINGLE_LINE, 0 );
    wp_str_init_default( &state->string );
}
#endif
WORKPHONE_API void wp_textedit_select_all( struct wp_text_edit *state )
{
    WORKPHONE_ASSERT( state );
    state->select_start = 0;
    state->select_end = state->string.len;
}
WORKPHONE_API void wp_textedit_free( struct wp_text_edit *state )
{
    WORKPHONE_ASSERT( state );
    if( !state )
        return;
    wp_str_free( &state->string );
}
