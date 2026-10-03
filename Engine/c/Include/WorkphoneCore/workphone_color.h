#ifndef workphone_color_h__
#define workphone_color_h__

#include "workphone_prerequisites.h"

/**
 * @brief Simple RGBA floating-point colour.
 */
typedef struct wp_colour_f
{
    wp_f32 r;
    wp_f32 g;
    wp_f32 b;
    wp_f32 a;
} wp_colour_f;

typedef struct wp_color
{
    wp_u8 r, g, b, a;
} wp_color;

typedef struct wp_colorf
{
    wp_f32 r, g, b, a;
} wp_colorf;

WORKPHONE_GLOBAL const struct wp_color wp_red = { 255, 0, 0, 255 };
WORKPHONE_GLOBAL const struct wp_color wp_green = { 0, 255, 0, 255 };
WORKPHONE_GLOBAL const struct wp_color wp_blue = { 0, 0, 255, 255 };
WORKPHONE_GLOBAL const struct wp_color wp_white = { 255, 255, 255, 255 };
WORKPHONE_GLOBAL const struct wp_color wp_black = { 0, 0, 0, 255 };
WORKPHONE_GLOBAL const struct wp_color wp_yellow = { 255, 255, 0, 255 };

/* Function declarations */
#ifdef __cplusplus
extern "C" {
#endif

WORKPHONE_API struct wp_color wp_rgba( wp_s32 r, wp_s32 g, wp_s32 b, wp_s32 a );
WORKPHONE_API struct wp_color wp_rgb_factor( struct wp_color col, wp_f32 factor );
WORKPHONE_API struct wp_color wp_rgb( wp_s32 r, wp_s32 g, wp_s32 b );
WORKPHONE_API struct wp_color wp_rgb_hex( const wp_c8 *rgb );
WORKPHONE_API struct wp_color wp_rgba_hex( const wp_c8 *rgb );
WORKPHONE_API void wp_color_hex_rgba( wp_c8 *output, struct wp_color col );
WORKPHONE_API void wp_color_hex_rgb( wp_c8 *output, struct wp_color col );
WORKPHONE_API struct wp_color wp_rgba_iv( const wp_s32 *c );
WORKPHONE_API struct wp_color wp_rgba_bv( const wp_byte *c );
WORKPHONE_API struct wp_color wp_rgb_iv( const wp_s32 *c );
WORKPHONE_API struct wp_color wp_rgb_bv( const wp_byte *c );
WORKPHONE_API struct wp_color wp_rgba_u32( wp_u32 in );
WORKPHONE_API struct wp_color wp_rgba_f( wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 a );
WORKPHONE_API struct wp_color wp_rgba_fv( const wp_f32 *c );
WORKPHONE_API struct wp_color wp_rgba_cf( struct wp_colorf c );
WORKPHONE_API struct wp_color wp_rgb_f( wp_f32 r, wp_f32 g, wp_f32 b );
WORKPHONE_API struct wp_color wp_rgb_fv( const wp_f32 *c );
WORKPHONE_API struct wp_color wp_rgb_cf( struct wp_colorf c );
WORKPHONE_API struct wp_colorf wp_hsva_colorf( wp_f32 h, wp_f32 s, wp_f32 v, wp_f32 a );
WORKPHONE_API struct wp_colorf wp_hsva_colorfv( const wp_f32 *c );
WORKPHONE_API struct wp_color wp_hsva_f( wp_f32 h, wp_f32 s, wp_f32 v, wp_f32 a );
WORKPHONE_API struct wp_color wp_hsva_fv( const wp_f32 *c );
WORKPHONE_API struct wp_color wp_hsv( wp_s32 h, wp_s32 s, wp_s32 v );
WORKPHONE_API struct wp_color wp_hsv_iv( const wp_s32 *c );
WORKPHONE_API struct wp_color wp_hsv_bv( const wp_byte *c );
WORKPHONE_API struct wp_color wp_hsv_f( wp_f32 h, wp_f32 s, wp_f32 v );
WORKPHONE_API struct wp_color wp_hsv_fv( const wp_f32 *c );
WORKPHONE_API struct wp_color wp_hsva( wp_s32 h, wp_s32 s, wp_s32 v, wp_s32 a );
WORKPHONE_API struct wp_color wp_hsva_iv( const wp_s32 *c );
WORKPHONE_API struct wp_color wp_hsva_bv( const wp_byte *c );
WORKPHONE_API wp_u32 wp_color_u32( struct wp_color in );
WORKPHONE_API void wp_color_f( wp_f32 *r, wp_f32 *g, wp_f32 *b, wp_f32 *a, struct wp_color in );
WORKPHONE_API void wp_color_fv( wp_f32 *c, struct wp_color in );
WORKPHONE_API struct wp_colorf wp_color_cf( struct wp_color in );
WORKPHONE_API void wp_color_d( wp_f64 *r, wp_f64 *g, wp_f64 *b, wp_f64 *a, struct wp_color in );
WORKPHONE_API void wp_color_dv( wp_f64 *c, struct wp_color in );
WORKPHONE_API void wp_colorf_hsva_f( wp_f32 *out_h, wp_f32 *out_s, wp_f32 *out_v, wp_f32 *out_a,
                                     struct wp_colorf in );
WORKPHONE_API void wp_colorf_hsva_fv( wp_f32 *hsva, struct wp_colorf in );
WORKPHONE_API void wp_color_hsva_i( wp_s32 *out_h, wp_s32 *out_s, wp_s32 *out_v, wp_s32 *out_a,
                                    struct wp_color in );
WORKPHONE_API void wp_color_hsva_iv( wp_s32 *hsva, struct wp_color in );
WORKPHONE_API void wp_color_hsva_bv( wp_byte *hsva, struct wp_color in );
WORKPHONE_API void wp_color_hsva_b( wp_byte *out_h, wp_byte *out_s, wp_byte *out_v, wp_byte *out_a,
                                    struct wp_color in );
WORKPHONE_API void wp_color_hsv_i( wp_s32 *out_h, wp_s32 *out_s, wp_s32 *out_v, struct wp_color in );
WORKPHONE_API void wp_color_hsv_b( wp_byte *h, wp_byte *s, wp_byte *v, wp_byte *a, struct wp_color in );
WORKPHONE_API void wp_color_hsv_iv( wp_s32 *hsv, struct wp_color in );
WORKPHONE_API void wp_color_hsv_bv( wp_byte *hsv, struct wp_color in );
WORKPHONE_API void wp_color_hsv_f( wp_f32 *out_h, wp_f32 *out_s, wp_f32 *out_v, struct wp_color in );
WORKPHONE_API void wp_color_hsv_fv( wp_f32 *hsv, struct wp_color in );

#ifdef __cplusplus
}
#endif

#endif  // workphone_color_h__
