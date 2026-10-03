#ifndef workphone_text_h__
#define workphone_text_h__

#include "workphone_color.h"
#include "workphone_string.h"
#include "workphone_vector.h"

struct wp_clipboard
{
    wp_handle userdata;
    wp_plugin_paste paste;
    wp_plugin_copy copy;
};

/* text */
struct wp_text
{
    struct wp_vec2f padding;
    struct wp_color background;
    struct wp_color text;
};

struct wp_text_undo_record
{
    int where;
    short insert_length;
    short delete_length;
    short char_storage;
};

struct wp_text_undo_state
{
    struct wp_text_undo_record undo_rec[WORKPHONE_TEXTEDIT_UNDOSTATECOUNT];
    wp_rune undo_char[WORKPHONE_TEXTEDIT_UNDOCHARCOUNT];
    short undo_point;
    short redo_point;
    short undo_char_point;
    short redo_char_point;
};

struct wp_text_edit
{
    struct wp_clipboard clip;
    struct wp_str string;
    wp_plugin_filter filter;
    struct wp_vec2f scrollbar;

    int cursor;
    int select_start;
    int select_end;
    unsigned char mode;
    unsigned char cursor_at_end_of_line;
    unsigned char initialized;
    unsigned char has_preferred_x;
    unsigned char single_line;
    unsigned char active;
    unsigned char padding1;
    wp_f32 preferred_x;
    struct wp_text_undo_state undo;
};

WORKPHONE_API void wp_widget_text( struct wp_command_buffer *o, struct wp_rect b, const char *string,
                                   int len, const struct wp_text *t, wp_flags a,
                                   const struct wp_user_font *f );
WORKPHONE_API void wp_widget_text_wrap( struct wp_command_buffer *o, struct wp_rect b,
                                        const char *string, int len, const struct wp_text *t,
                                        const struct wp_user_font *f );

/** filter function */
WORKPHONE_API wp_bool wp_filter_default( const struct wp_text_edit *, wp_rune unicode );
WORKPHONE_API wp_bool wp_filter_ascii( const struct wp_text_edit *, wp_rune unicode );
WORKPHONE_API wp_bool wp_filter_wp_f32( const struct wp_text_edit *, wp_rune unicode );
WORKPHONE_API wp_bool wp_filter_decimal( const struct wp_text_edit *, wp_rune unicode );
WORKPHONE_API wp_bool wp_filter_hex( const struct wp_text_edit *, wp_rune unicode );
WORKPHONE_API wp_bool wp_filter_oct( const struct wp_text_edit *, wp_rune unicode );
WORKPHONE_API wp_bool wp_filter_binary( const struct wp_text_edit *, wp_rune unicode );

/** text editor */
#ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
WORKPHONE_API void wp_textedit_init_default( struct wp_text_edit * );
#endif
WORKPHONE_API void wp_textedit_init( struct wp_text_edit *, const struct wp_allocator *, wp_size size );
WORKPHONE_API void wp_textedit_init_fixed( struct wp_text_edit *, void *memory, wp_size size );
WORKPHONE_API void wp_textedit_free( struct wp_text_edit * );
WORKPHONE_API void wp_textedit_text( struct wp_text_edit *, const wp_c8 *, wp_s32 total_len );
WORKPHONE_API void wp_textedit_delete( struct wp_text_edit *, wp_s32 where, wp_s32 len );
WORKPHONE_API void wp_textedit_delete_selection( struct wp_text_edit * );
WORKPHONE_API void wp_textedit_select_all( struct wp_text_edit * );
WORKPHONE_API wp_bool wp_textedit_cut( struct wp_text_edit * );
WORKPHONE_API wp_bool wp_textedit_paste( struct wp_text_edit *, wp_c8 const *, wp_s32 len );
WORKPHONE_API void wp_textedit_undo( struct wp_text_edit * );
WORKPHONE_API void wp_textedit_redo( struct wp_text_edit * );

/* text editor */
WORKPHONE_API void wp_textedit_clear_state( struct wp_text_edit *state, enum wp_text_edit_type type,
                                            wp_plugin_filter filter );
WORKPHONE_API void wp_textedit_click( struct wp_text_edit *state, wp_f32 x, wp_f32 y,
                                      const struct wp_user_font *font, wp_f32 row_height );
WORKPHONE_API void wp_textedit_drag( struct wp_text_edit *state, wp_f32 x, wp_f32 y,
                                     const struct wp_user_font *font, wp_f32 row_height );
WORKPHONE_API void wp_textedit_key( struct wp_text_edit *state, enum wp_keys key, wp_s32 shift_mod,
                                    const struct wp_user_font *font, wp_f32 row_height );

WORKPHONE_API void wp_text( struct wp_context *, const wp_c8 *, int, wp_flags );
WORKPHONE_API void wp_text_colored( struct wp_context *, const wp_c8 *, int, wp_flags, struct wp_color );
WORKPHONE_API void wp_text_wrap( struct wp_context *, const wp_c8 *, wp_s32 );
WORKPHONE_API void wp_text_wrap_colored( struct wp_context *, const wp_c8 *, int, struct wp_color );
WORKPHONE_API void wp_label( struct wp_context *, const wp_c8 *, wp_flags align );
WORKPHONE_API void wp_label_colored( struct wp_context *, const wp_c8 *, wp_flags align,
                                     struct wp_color );
WORKPHONE_API void wp_label_wrap( struct wp_context *, const wp_c8 * );
WORKPHONE_API void wp_label_colored_wrap( struct wp_context *, const wp_c8 *, struct wp_color );

#endif  // workphone_text_h__
