/**
 * @file wp_math.h
 * @brief C API for scalar math utilities (wp_f32 and wp_f64 variants).
 */

#ifndef WORKPHONE_MATH_H
#define WORKPHONE_MATH_H

#include "workphone_types.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WORKPHONE_PI (wp_f32)3.14159265358979323846
#define WORKPHONE_TWO_PI (wp_f32)6.28318530717958647692
#define WORKPHONE_HALF_PI (wp_f32)1.57079632679489661923
#define WORKPHONE_E (wp_f32)2.71828182845904523536
#define WORKPHONE_PI_F ( (wp_f32)WORKPHONE_PI )
#define WORKPHONE_TWO_PI_F ( (wp_f32)WORKPHONE_TWO_PI )
#define WORKPHONE_HALF_PI_F ( (wp_f32)WORKPHONE_HALF_PI )
#define WORKPHONE_DEG2RAD ( WORKPHONE_PI / 180.0 )
#define WORKPHONE_RAD2DEG ( 180.0 / WORKPHONE_PI )
#define WORKPHONE_DEG2RAD_F ( (wp_f32)( WORKPHONE_PI / 180.0 ) )
#define WORKPHONE_RAD2DEG_F ( (wp_f32)( 180.0 / WORKPHONE_PI ) )
#define WORKPHONE_EPSILON_F 1e-7f
#define WORKPHONE_EPSILON_D 1e-7

struct wp_scroll
{
    wp_u32 x, y;
};

struct wp_rect
{
    wp_f32 x, y, w, h;
};

struct wp_recti
{
    short x, y, w, h;
};

struct wp_vec2f;

WORKPHONE_GLOBAL const struct wp_rect wp_null_rect = { -8192.0f, -8192.0f, 16384, 16384 };

WORKPHONE_LIB struct wp_rect wp_shrink_rect( struct wp_rect r, float amount );

wp_f32 wp_minf( wp_f32 a, wp_f32 b );
wp_f32 wp_maxf( wp_f32 a, wp_f32 b );
wp_f32 wp_absf( wp_f32 a );
wp_f32 wp_clampf( wp_f32 v, wp_f32 lo, wp_f32 hi );
wp_f32 wp_lerpf( wp_f32 a, wp_f32 b, wp_f32 t );
wp_f32 wp_signf( wp_f32 a );
wp_f32 wp_stepf( wp_f32 edge, wp_f32 x );
wp_f32 wp_smoothstepf( wp_f32 edge0, wp_f32 edge1, wp_f32 x );
wp_f32 wp_floorf( wp_f32 a );
wp_f32 wp_ceilf( wp_f32 a );
wp_f32 wp_roundf( wp_f32 a );
wp_f32 wp_fmodf( wp_f32 a, wp_f32 b );
wp_f32 wp_sqrtf( wp_f32 a );
wp_f32 wp_cbrtf( wp_f32 a );
wp_f32 wp_powf( wp_f32 a, wp_f32 b );
wp_f32 wp_expf( wp_f32 a );
wp_f32 wp_logf( wp_f32 a );
wp_f32 wp_log2f( wp_f32 a );
wp_f32 wp_sinf( wp_f32 a );
wp_f32 wp_cosf( wp_f32 a );
wp_f32 wp_tanf( wp_f32 a );
wp_f32 wp_asinf( wp_f32 a );
wp_f32 wp_acosf( wp_f32 a );
wp_f32 wp_atanf( wp_f32 a );
wp_f32 wp_atan2f( wp_f32 y, wp_f32 x );
wp_f32 wp_deg2radf( wp_f32 deg );
wp_f32 wp_rad2degf( wp_f32 rad );
wp_s32 wp_equalf( wp_f32 a, wp_f32 b );
wp_s32 wp_equals_tolf( wp_f32 a, wp_f32 b, wp_f32 tolerance );
wp_s32 wp_is_zerof( wp_f32 a );
wp_s32 wp_is_nanf( wp_f32 a );
wp_s32 wp_is_inff( wp_f32 a );

wp_f64 wp_mind( wp_f64 a, wp_f64 b );
wp_f64 wp_maxd( wp_f64 a, wp_f64 b );
wp_f64 wp_absd( wp_f64 a );
wp_f64 wp_clampd( wp_f64 v, wp_f64 lo, wp_f64 hi );
wp_f64 wp_lerpd( wp_f64 a, wp_f64 b, wp_f64 t );
wp_f64 wp_signd( wp_f64 a );
wp_f64 wp_stepd( wp_f64 edge, wp_f64 x );
wp_f64 wp_smoothstepd( wp_f64 edge0, wp_f64 edge1, wp_f64 x );
wp_f64 wp_floord( wp_f64 a );
wp_f64 wp_ceild( wp_f64 a );
wp_f64 wp_roundd( wp_f64 a );
wp_f64 wp_fmodd( wp_f64 a, wp_f64 b );
wp_f64 wp_sqrtd( wp_f64 a );
wp_f64 wp_cbrtd( wp_f64 a );
wp_f64 wp_powd( wp_f64 a, wp_f64 b );
wp_f64 wp_expd( wp_f64 a );
wp_f64 wp_logd( wp_f64 a );
wp_f64 wp_log2d( wp_f64 a );
wp_f64 wp_sind( wp_f64 a );
wp_f64 wp_cosd( wp_f64 a );
wp_f64 wp_tand( wp_f64 a );
wp_f64 wp_asind( wp_f64 a );
wp_f64 wp_acosd( wp_f64 a );
wp_f64 wp_atand( wp_f64 a );
wp_f64 wp_atan2d( wp_f64 y, wp_f64 x );
wp_f64 wp_deg2radd( wp_f64 deg );
wp_f64 wp_rad2degd( wp_f64 rad );
wp_s32 wp_equald( wp_f64 a, wp_f64 b );
wp_s32 wp_equals_told( wp_f64 a, wp_f64 b, wp_f64 tolerance );
wp_s32 wp_is_zerod( wp_f64 a );
wp_s32 wp_is_nand( wp_f64 a );
wp_s32 wp_is_infd( wp_f64 a );

wp_u32 wp_round_up_pow2( wp_u32 v );
wp_s32 wp_ifloorf( wp_f32 x );
wp_s32 wp_iceilf( wp_f32 x );
wp_f32 wp_roundf( wp_f32 a );

#ifdef WP_DTOA_NEEDED
wp_f64 wp_pow( wp_f64 x, wp_s32 n );
wp_s32 wp_ifloord( wp_f64 x );
wp_s32 wp_log10( wp_f64 n );
#endif

/* math */
WORKPHONE_LIB wp_f32 wp_inv_sqrt( wp_f32 n );
WORKPHONE_LIB wp_f32 wp_sin( wp_f32 x );
WORKPHONE_LIB wp_f32 wp_cos( wp_f32 x );
WORKPHONE_LIB wp_f32 wp_atan( wp_f32 x );
WORKPHONE_LIB wp_f32 wp_atan2( wp_f32 y, wp_f32 x );

WORKPHONE_LIB wp_u32 wp_round_up_pow2( wp_u32 v );
WORKPHONE_LIB struct wp_rect wp_shrink_make_rect( struct wp_rect r, wp_f32 amount );
WORKPHONE_LIB struct wp_rect wp_pad_rect( struct wp_rect r, struct wp_vec2f pad );
WORKPHONE_LIB void wp_unify( struct wp_rect *clip, const struct wp_rect *a, wp_f32 x0, wp_f32 y0,
                             wp_f32 x1, wp_f32 y1 );
WORKPHONE_LIB wp_s32 wp_ifloorf( wp_f32 x );
WORKPHONE_LIB wp_s32 wp_iceilf( wp_f32 x );
WORKPHONE_LIB wp_f32 wp_roundf( wp_f32 x );

WORKPHONE_API struct wp_vec2f wp_make_vec2( wp_f32 x, wp_f32 y );
WORKPHONE_API struct wp_vec2f wp_make_vec2f( wp_f32 x, wp_f32 y );
WORKPHONE_API struct wp_vec2f wp_make_vec2i( wp_s32 x, wp_s32 y );
WORKPHONE_API struct wp_vec2f wp_make_vec2v( const wp_f32 *xy );
WORKPHONE_API struct wp_vec2f wp_make_vec2iv( const wp_s32 *xy );

WORKPHONE_API struct wp_rect wp_get_null_rect( void );
WORKPHONE_API struct wp_rect wp_make_rect( wp_f32 x, wp_f32 y, wp_f32 w, wp_f32 h );
WORKPHONE_API struct wp_rect wp_make_recti( wp_s32 x, wp_s32 y, wp_s32 w, wp_s32 h );
WORKPHONE_API struct wp_rect wp_make_recta( struct wp_vec2f pos, struct wp_vec2f size );
WORKPHONE_API struct wp_rect wp_make_rectv( const wp_f32 *xywh );
WORKPHONE_API struct wp_rect wp_make_rectiv( const wp_s32 *xywh );
WORKPHONE_API struct wp_vec2f wp_rect_pos( struct wp_rect );
WORKPHONE_API struct wp_vec2f wp_rect_size( struct wp_rect );

#define WORKPHONE_PI_HALF WORKPHONE_HALF_PI_F
#define WORKPHONE_MAX_FLOAT_PRECISION 2

#define WORKPHONE_UNUSED( x ) ( (void)( x ) )
#define WORKPHONE_SATURATE( x ) ( WORKPHONE_MAX( 0, WORKPHONE_MIN( 1.0f, x ) ) )
#define WORKPHONE_LEN( a ) ( sizeof( a ) / sizeof( a )[0] )
#define WORKPHONE_ABS( a ) ( ( ( a ) < 0 ) ? -( a ) : ( a ) )
#define WORKPHONE_BETWEEN( x, a, b ) ( ( a ) <= ( x ) && ( x ) < ( b ) )
#define WORKPHONE_INBOX( px, py, x, y, w, h ) \
    ( WORKPHONE_BETWEEN( px, x, x + w ) && WORKPHONE_BETWEEN( py, y, y + h ) )
#define WORKPHONE_INTERSECT( x0, y0, w0, h0, x1, y1, w1, h1 ) \
    ( ( x1 < ( x0 + w0 ) ) && ( x0 < ( x1 + w1 ) ) && ( y1 < ( y0 + h0 ) ) && ( y0 < ( y1 + h1 ) ) )
#define WORKPHONE_CONTAINS( x, y, w, h, bx, by, bw, bh ) \
    ( WORKPHONE_INBOX( x, y, bx, by, bw, bh ) && WORKPHONE_INBOX( x + w, y + h, bx, by, bw, bh ) )

#define wp_vec2_sub( a, b ) wp_make_vec2( ( a ).x - ( b ).x, ( a ).y - ( b ).y )
#define wp_vec2_add( a, b ) wp_make_vec2( ( a ).x + ( b ).x, ( a ).y + ( b ).y )
#define wp_vec2_len_sqr( a ) ( ( a ).x * ( a ).x + ( a ).y * ( a ).y )
#define wp_vec2_muls( a, t ) wp_make_vec2( ( a ).x *( t ), ( a ).y *( t ) )

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_MATH_H */
