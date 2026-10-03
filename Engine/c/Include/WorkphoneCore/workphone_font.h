#ifndef workphone_font_h__
#define workphone_font_h__

#include "workphone_cursor.h"
#include "workphone_memory.h"
#include "workphone_vector.h"

#ifdef WORKPHONE_INCLUDE_FONT_BAKING

struct wp_user_font_glyph;
typedef wp_f32 ( *wp_text_width_f )( wp_handle, wp_f32 h, const wp_c8 *, wp_s32 len );
typedef void ( *wp_query_font_glyph_f )( wp_handle handle, wp_f32 font_height,
                                         struct wp_user_font_glyph *glyph, wp_rune codepoint,
                                         wp_rune next_codepoint );

#    if defined( WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT ) || defined( WORKPHONE_INCLUDE_SOFTWARE_FONT )
struct wp_user_font_glyph
{
    struct wp_vec2f uv[2];  /**!< texture coordinates */
    struct wp_vec2f offset; /**!< offset between top left and glyph */
    wp_f32 width, height;  /**!< size of the glyph  */
    wp_f32 xadvance;       /**!< offset to the next glyph */
};
#    endif

/* User font */
struct wp_user_font
{
    wp_handle userdata;
    wp_f32 height;
    wp_text_width_f width;
#    ifdef WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
    wp_query_font_glyph_f query;
    wp_handle texture;
#    endif
};

#    ifdef WORKPHONE_INCLUDE_FONT_BAKING
enum wp_font_coord_type
{
    WORKPHONE_COORD_UV,   /**!< texture coordinates inside font glyphs are clamped between 0-1 */
    WORKPHONE_COORD_PIXEL /**!< texture coordinates inside font glyphs are in absolute pixel */
};

struct wp_font;
struct wp_baked_font
{
    wp_f32 height;         /**!< height of the font  */
    wp_f32 ascent;         /**!< font glyphs ascent and descent  */
    wp_f32 descent;        /**!< font glyphs ascent and descent  */
    wp_rune glyph_offset;  /**!< glyph array offset inside the font glyph baking output array  */
    wp_rune glyph_count;   /**!< number of glyphs of this font inside the glyph baking array output */
    const wp_rune *ranges; /**!< font codepoint ranges as pairs of (from/to) and 0 as last element */
};

struct wp_font_config
{
    struct wp_font_config *next; /**!< NOTE: only used internally */
    void *
        ttf_blob; /**!< pointer to loaded TTF file memory block.  * \note not needed for wp_font_atlas_add_from_memory and wp_font_atlas_add_from_file. */
    wp_size
        ttf_size; /**!< size of the loaded TTF file memory block * \note not needed for wp_font_atlas_add_from_memory and wp_font_atlas_add_from_file. */

    unsigned char ttf_data_owned_by_atlas; /**!< used inside font atlas: default to: 0*/
    unsigned char merge_mode;              /**!< merges this font into the last font */
    unsigned char
        pixel_snap; /**!< align every character to pixel boundary (if true set oversample (1,1)) */
    unsigned char oversample_v, oversample_h; /**!< rasterize at high quality for sub-pixel position */
    unsigned char padding[3];

    wp_f32 size; /**!< baked pixel height of the font */
    enum wp_font_coord_type
        coord_type;         /**!< texture coordinate format with either pixel or UV coordinates */
    struct wp_vec2f spacing; /**!< extra pixel spacing between glyphs  */
    const wp_rune *range;   /**!< list of unicode ranges (2 values per range, zero terminated) */
    struct wp_baked_font
        *font;              /**!< font to setup in the baking process: NOTE: not needed for font atlas */
    wp_rune fallback_glyph; /**!< fallback glyph to use if a given rune is not found */
    struct wp_font_config *n;
    struct wp_font_config *p;
};

struct wp_font_glyph
{
    wp_rune codepoint;
    wp_f32 xadvance;
    wp_f32 x0, y0, x1, y1, w, h;
    wp_f32 u0, v0, u1, v1;
};

struct wp_font
{
    struct wp_font *next;
    struct wp_user_font handle;
    struct wp_baked_font info;
    wp_f32 scale;
    struct wp_font_glyph *glyphs;
    const struct wp_font_glyph *fallback;
    wp_rune fallback_codepoint;
    wp_handle texture;
    struct wp_font_config *config;
};

enum wp_font_atlas_format
{
    WORKPHONE_FONT_ATLAS_ALPHA8,
    WORKPHONE_FONT_ATLAS_RGBA32
};

struct wp_font_atlas
{
    void *pixel;
    int tex_width;
    int tex_height;

    struct wp_allocator permanent;
    struct wp_allocator temporary;

    struct wp_recti custom;
    struct wp_cursor cursors[WORKPHONE_CURSOR_COUNT];

    int glyph_count;
    struct wp_font_glyph *glyphs;
    struct wp_font *default_font;
    struct wp_font *fonts;
    struct wp_font_config *config;
    int font_num;
};

/** some language glyph codepoint ranges */
WORKPHONE_API const wp_rune *wp_font_default_glyph_ranges( void );
WORKPHONE_API const wp_rune *wp_font_chinese_glyph_ranges( void );
WORKPHONE_API const wp_rune *wp_font_cyrillic_glyph_ranges( void );
WORKPHONE_API const wp_rune *wp_font_korean_glyph_ranges( void );

#        ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
WORKPHONE_API void wp_font_atlas_init_default( struct wp_font_atlas * );
#        endif
WORKPHONE_API void wp_font_atlas_init( struct wp_font_atlas *, const struct wp_allocator * );
WORKPHONE_API void wp_font_atlas_init_custom( struct wp_font_atlas *,
                                              const struct wp_allocator *persistent,
                                              const struct wp_allocator *transient );
WORKPHONE_API void wp_font_atlas_begin( struct wp_font_atlas * );
WORKPHONE_API struct wp_font_config wp_font_config( wp_f32 pixel_height );
WORKPHONE_API struct wp_font *wp_font_atlas_add( struct wp_font_atlas *, const struct wp_font_config * );
#        ifdef WORKPHONE_INCLUDE_DEFAULT_FONT
WORKPHONE_API struct wp_font *wp_font_atlas_add_default( struct wp_font_atlas *, wp_f32 height,
                                                         const struct wp_font_config * );
#        endif
WORKPHONE_API struct wp_font *wp_font_atlas_add_from_memory( struct wp_font_atlas *atlas, void *memory,
                                                             wp_size size, wp_f32 height,
                                                             const struct wp_font_config *config );
#        ifdef WORKPHONE_INCLUDE_STANDARD_IO
WORKPHONE_API struct wp_font *wp_font_atlas_add_from_file( struct wp_font_atlas *atlas,
                                                           const char *file_path, wp_f32 height,
                                                           const struct wp_font_config * );
#        endif
WORKPHONE_API struct wp_font *wp_font_atlas_add_compressed( struct wp_font_atlas *, void *memory,
                                                            wp_size size, wp_f32 height,
                                                            const struct wp_font_config * );
WORKPHONE_API struct wp_font *wp_font_atlas_add_compressed_base85( struct wp_font_atlas *,
                                                                   const char *data, wp_f32 height,
                                                                   const struct wp_font_config *config );
WORKPHONE_API const void *wp_font_atlas_bake( struct wp_font_atlas *, int *width, int *height,
                                              enum wp_font_atlas_format );
WORKPHONE_API void wp_font_atlas_end( struct wp_font_atlas *, wp_handle tex,
                                      struct wp_draw_null_texture * );
WORKPHONE_API const struct wp_font_glyph *wp_font_find_glyph( const struct wp_font *, wp_rune unicode );
WORKPHONE_API void wp_font_atlas_cleanup( struct wp_font_atlas *atlas );
WORKPHONE_API void wp_font_atlas_clear( struct wp_font_atlas * );

#    endif

#endif  // WORKPHONE_INCLUDE_FONT_BAKING

#endif  // workphone_font_h__
