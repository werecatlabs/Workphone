#include "workphone_config.h"
#include "workphone_prerequisites.h"
#include "workphone_math.h"
#include "workphone_ui.h"

struct wp_rect wp_shrink_rect( struct wp_rect r, float amount )
{
    struct wp_rect res;
    r.w = WORKPHONE_MAX( r.w, 2 * amount );
    r.h = WORKPHONE_MAX( r.h, 2 * amount );
    res.x = r.x + amount;
    res.y = r.y + amount;
    res.w = r.w - 2 * amount;
    res.h = r.h - 2 * amount;
    return res;
}

struct wp_rect wp_pad_rect( struct wp_rect r, struct wp_vec2f pad )
{
    r.w = WORKPHONE_MAX( r.w, 2 * pad.x );
    r.h = WORKPHONE_MAX( r.h, 2 * pad.y );
    r.x += pad.x;
    r.y += pad.y;
    r.w -= 2 * pad.x;
    r.h -= 2 * pad.y;
    return r;
}

wp_f32 wp_minf( wp_f32 a, wp_f32 b )
{
    return a < b ? a : b;
}
wp_f32 wp_maxf( wp_f32 a, wp_f32 b )
{
    return a > b ? a : b;
}
wp_f32 wp_absf( wp_f32 a )
{
    return fabsf( a );
}

wp_f32 wp_clampf( wp_f32 v, wp_f32 lo, wp_f32 hi )
{
    if( v < lo )
        return lo;
    if( v > hi )
        return hi;
    return v;
}

wp_f32 wp_lerpf( wp_f32 a, wp_f32 b, wp_f32 t )
{
    return a + ( b - a ) * t;
}

wp_f32 wp_signf( wp_f32 a )
{
    if( a > 0.0f )
        return 1.0f;
    if( a < 0.0f )
        return -1.0f;
    return 0.0f;
}

wp_f32 wp_stepf( wp_f32 edge, wp_f32 x )
{
    return x < edge ? 0.0f : 1.0f;
}

wp_f32 wp_smoothstepf( wp_f32 edge0, wp_f32 edge1, wp_f32 x )
{
    wp_f32 t = wp_clampf( ( x - edge0 ) / ( edge1 - edge0 ), 0.0f, 1.0f );
    return t * t * ( 3.0f - 2.0f * t );
}

wp_f32 wp_floorf( wp_f32 a )
{
    return floorf( a );
}
wp_f32 wp_ceilf( wp_f32 a )
{
    return ceilf( a );
}
wp_f32 wp_fmodf( wp_f32 a, wp_f32 b )
{
    return fmodf( a, b );
}
wp_f32 wp_sqrtf( wp_f32 a )
{
    return sqrtf( a );
}
wp_f32 wp_cbrtf( wp_f32 a )
{
    return (wp_f32)cbrt( (wp_f64)a );
}
wp_f32 wp_powf( wp_f32 a, wp_f32 b )
{
    return powf( a, b );
}
wp_f32 wp_expf( wp_f32 a )
{
    return expf( a );
}
wp_f32 wp_logf( wp_f32 a )
{
    return logf( a );
}
wp_f32 wp_log2f( wp_f32 a )
{
    return log2f( a );
}
wp_f32 wp_sinf( wp_f32 a )
{
    return sinf( a );
}
wp_f32 wp_cosf( wp_f32 a )
{
    return cosf( a );
}
wp_f32 wp_tanf( wp_f32 a )
{
    return tanf( a );
}
wp_f32 wp_asinf( wp_f32 a )
{
    return asinf( a );
}
wp_f32 wp_acosf( wp_f32 a )
{
    return acosf( a );
}
wp_f32 wp_atanf( wp_f32 a )
{
    return atanf( a );
}
wp_f32 wp_atan2f( wp_f32 y, wp_f32 x )
{
    return atan2f( y, x );
}
wp_f32 wp_deg2radf( wp_f32 deg )
{
    return deg * WORKPHONE_DEG2RAD_F;
}
wp_f32 wp_rad2degf( wp_f32 rad )
{
    return rad * WORKPHONE_RAD2DEG_F;
}

wp_s32 wp_equalf( wp_f32 a, wp_f32 b )
{
    return fabsf( a - b ) <= WORKPHONE_EPSILON_F;
}

wp_s32 wp_equals_tolf( wp_f32 a, wp_f32 b, wp_f32 tolerance )
{
    return fabsf( a - b ) <= tolerance;
}

wp_s32 wp_is_zerof( wp_f32 a )
{
    return fabsf( a ) <= WORKPHONE_EPSILON_F;
}

wp_s32 wp_is_nanf( wp_f32 a )
{
    return a != a ? 1 : 0;
}
wp_s32 wp_is_inff( wp_f32 a )
{
    return ( a == a ) && ( ( a - a ) != 0.0f ) ? 1 : 0;
}

wp_f64 wp_mind( wp_f64 a, wp_f64 b )
{
    return a < b ? a : b;
}
wp_f64 wp_maxd( wp_f64 a, wp_f64 b )
{
    return a > b ? a : b;
}
wp_f64 wp_absd( wp_f64 a )
{
    return fabs( a );
}

wp_f64 wp_clampd( wp_f64 v, wp_f64 lo, wp_f64 hi )
{
    if( v < lo )
        return lo;
    if( v > hi )
        return hi;
    return v;
}

wp_f64 wp_lerpd( wp_f64 a, wp_f64 b, wp_f64 t )
{
    return a + ( b - a ) * t;
}

wp_f64 wp_signd( wp_f64 a )
{
    if( a > 0.0 )
        return 1.0;
    if( a < 0.0 )
        return -1.0;
    return 0.0;
}

wp_f64 wp_stepd( wp_f64 edge, wp_f64 x )
{
    return x < edge ? 0.0 : 1.0;
}

wp_f64 wp_smoothstepd( wp_f64 edge0, wp_f64 edge1, wp_f64 x )
{
    wp_f64 t = wp_clampd( ( x - edge0 ) / ( edge1 - edge0 ), 0.0, 1.0 );
    return t * t * ( 3.0 - 2.0 * t );
}

wp_f64 wp_floord( wp_f64 a )
{
    return floor( a );
}
wp_f64 wp_ceild( wp_f64 a )
{
    return ceil( a );
}
wp_f64 wp_fmodd( wp_f64 a, wp_f64 b )
{
    return fmod( a, b );
}
wp_f64 wp_sqrtd( wp_f64 a )
{
    return sqrt( a );
}
wp_f64 wp_cbrtd( wp_f64 a )
{
    return cbrt( a );
}
wp_f64 wp_powd( wp_f64 a, wp_f64 b )
{
    return pow( a, b );
}
wp_f64 wp_expd( wp_f64 a )
{
    return exp( a );
}
wp_f64 wp_logd( wp_f64 a )
{
    return log( a );
}
wp_f64 wp_log2d( wp_f64 a )
{
    return log2( a );
}
wp_f64 wp_sind( wp_f64 a )
{
    return sin( a );
}
wp_f64 wp_cosd( wp_f64 a )
{
    return cos( a );
}
wp_f64 wp_tand( wp_f64 a )
{
    return tan( a );
}
wp_f64 wp_asind( wp_f64 a )
{
    return asin( a );
}
wp_f64 wp_acosd( wp_f64 a )
{
    return acos( a );
}
wp_f64 wp_atand( wp_f64 a )
{
    return atan( a );
}
wp_f64 wp_atan2d( wp_f64 y, wp_f64 x )
{
    return atan2( y, x );
}
wp_f64 wp_deg2radd( wp_f64 deg )
{
    return deg * WORKPHONE_DEG2RAD;
}
wp_f64 wp_rad2degd( wp_f64 rad )
{
    return rad * WORKPHONE_RAD2DEG;
}

wp_s32 wp_equald( wp_f64 a, wp_f64 b )
{
    return fabs( a - b ) <= WORKPHONE_EPSILON_D;
}

wp_s32 wp_equals_told( wp_f64 a, wp_f64 b, wp_f64 tolerance )
{
    return fabs( a - b ) <= tolerance;
}

wp_s32 wp_is_zerod( wp_f64 a )
{
    return fabs( a ) <= WORKPHONE_EPSILON_D;
}

wp_s32 wp_is_nand( wp_f64 a )
{
    return a != a ? 1 : 0;
}
wp_s32 wp_is_infd( wp_f64 a )
{
    return ( a == a ) && ( ( a - a ) != 0.0 ) ? 1 : 0;
}

wp_f32 wp_inv_sqrt( wp_f32 n )
{
    wp_f32 x2;
    const wp_f32 threehalfs = 1.5f;
    union
    {
        wp_u32 i;
        wp_f32 f;
    } conv = { 0 };
    conv.f = n;
    x2 = n * 0.5f;
    conv.i = 0x5f375A84 - ( conv.i >> 1 );
    conv.f = conv.f * ( threehalfs - ( x2 * conv.f * conv.f ) );
    return conv.f;
}

wp_f32 wp_sin( wp_f32 x )
{
    WORKPHONE_STORAGE const wp_f32 a0 = +1.91059300966915117e-31f;
    WORKPHONE_STORAGE const wp_f32 a1 = +1.00086760103908896f;
    WORKPHONE_STORAGE const wp_f32 a2 = -1.21276126894734565e-2f;
    WORKPHONE_STORAGE const wp_f32 a3 = -1.38078780785773762e-1f;
    WORKPHONE_STORAGE const wp_f32 a4 = -2.67353392911981221e-2f;
    WORKPHONE_STORAGE const wp_f32 a5 = +2.08026600266304389e-2f;
    WORKPHONE_STORAGE const wp_f32 a6 = -3.03996055049204407e-3f;
    WORKPHONE_STORAGE const wp_f32 a7 = +1.38235642404333740e-4f;
    return a0 + x * ( a1 + x * ( a2 + x * ( a3 + x * ( a4 + x * ( a5 + x * ( a6 + x * a7 ) ) ) ) ) );
}

wp_f32 wp_cos( wp_f32 x )
{
    /* New implementation. Also generated using lolremez. */
    /* Old version significantly deviated from expected results. */
    WORKPHONE_STORAGE const wp_f32 a0 = 9.9995999154986614e-1f;
    WORKPHONE_STORAGE const wp_f32 a1 = 1.2548995793001028e-3f;
    WORKPHONE_STORAGE const wp_f32 a2 = -5.0648546280678015e-1f;
    WORKPHONE_STORAGE const wp_f32 a3 = 1.2942246466519995e-2f;
    WORKPHONE_STORAGE const wp_f32 a4 = 2.8668384702547972e-2f;
    WORKPHONE_STORAGE const wp_f32 a5 = 7.3726485210586547e-3f;
    WORKPHONE_STORAGE const wp_f32 a6 = -3.8510875386947414e-3f;
    WORKPHONE_STORAGE const wp_f32 a7 = 4.7196604604366623e-4f;
    WORKPHONE_STORAGE const wp_f32 a8 = -1.8776444013090451e-5f;
    return a0 +
           x * ( a1 +
                 x * ( a2 + x * ( a3 + x * ( a4 + x * ( a5 + x * ( a6 + x * ( a7 + x * a8 ) ) ) ) ) ) );
}

wp_f32 wp_atan( wp_f32 x )
{
    /* ./lolremez --progress --wp_f32 -d 9 -r "0:pi*2" "atan(x)" */
    wp_f32 u = -1.0989005e-05f;
    WORKPHONE_ASSERT( x >= 0.0f && "TODO support negative wp_f32s" );
    u = u * x + 0.00034117949f;
    u = u * x + -0.0044932296f;
    u = u * x + 0.032596264f;
    u = u * x + -0.14088021f;
    u = u * x + 0.36040401f;
    u = u * x + -0.47017866f;
    u = u * x + 0.00050198776f;
    u = u * x + 1.0077682f;
    u = u * x + -0.0004765437f;
    return u;
}

wp_f32 wp_atan2( wp_f32 y, wp_f32 x )
{
    wp_f32 ax = wp_absf( x ), ay = wp_absf( y );
    /* 0 = +y +x    1 = -y +x
       2 = +y -x    3 = -y -x */
    wp_u32 signs = ( y < 0 ) | ( ( x < 0 ) << 1 );

    wp_f32 a;
    if( y == 0.0 && x == 0.0 )
        return 0.0f;
    a = ( ay > ax ) ? WORKPHONE_PI_HALF - wp_atan( ax / ay ) : wp_atan( ay / ax );

    switch( signs )
    {
    case 0:
        return a;
    case 1:
        return -a;
    case 2:
        return -a + WORKPHONE_PI;
    case 3:
        return a - WORKPHONE_PI;
    }
    return 0.0f; /* prevents warning */
}

wp_u32 wp_round_up_pow2( wp_u32 v )
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

wp_f64 wp_pow( wp_f64 x, wp_s32 n )
{
    /*  check the sign of n */
    wp_f64 r = 1;
    wp_s32 plus = n >= 0;
    n = ( plus ) ? n : -n;
    while( n > 0 )
    {
        if( ( n & 1 ) == 1 )
            r *= x;
        n /= 2;
        x *= x;
    }
    return plus ? r : 1.0 / r;
}

wp_s32 wp_ifloord( wp_f64 x )
{
    x = (wp_f64)( (wp_s32)x - ( ( x < 0.0 ) ? 1 : 0 ) );
    return (wp_s32)x;
}

wp_s32 wp_ifloorf( wp_f32 x )
{
    x = (wp_f32)( (wp_s32)x - ( ( x < 0.0f ) ? 1 : 0 ) );
    return (wp_s32)x;
}

wp_s32 wp_iceilf( wp_f32 x )
{
    if( x >= 0 )
    {
        wp_s32 i = (wp_s32)x;
        return ( x > i ) ? i + 1 : i;
    }
    else
    {
        wp_s32 t = (wp_s32)x;
        wp_f32 r = x - (wp_f32)t;
        return ( r > 0.0f ) ? t + 1 : t;
    }
}

wp_s32 wp_log10( wp_f64 n )
{
    wp_s32 neg;
    wp_s32 ret;
    wp_s32 exp = 0;

    neg = ( n < 0 ) ? 1 : 0;
    ret = ( neg ) ? (wp_s32)-n : (wp_s32)n;
    while( ( ret / 10 ) > 0 )
    {
        ret /= 10;
        exp++;
    }
    if( neg )
        exp = -exp;
    return exp;
}

wp_f32 wp_roundf( wp_f32 x )
{
    return ( x >= 0.0f ) ? (wp_f32)wp_ifloorf( x + 0.5f ) : (wp_f32)wp_iceilf( x - 0.5f );
}

struct wp_rect wp_get_null_rect( void )
{
    return wp_null_rect;
}

struct wp_rect wp_make_rect( wp_f32 x, wp_f32 y, wp_f32 w, wp_f32 h )
{
    struct wp_rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

struct wp_rect wp_make_recti( wp_s32 x, wp_s32 y, wp_s32 w, wp_s32 h )
{
    struct wp_rect r;
    r.x = (wp_f32)x;
    r.y = (wp_f32)y;
    r.w = (wp_f32)w;
    r.h = (wp_f32)h;
    return r;
}

struct wp_rect wp_make_recta( struct wp_vec2f pos, struct wp_vec2f size )
{
    return wp_make_rect( pos.x, pos.y, size.x, size.y );
}

struct wp_rect wp_make_rectv( const wp_f32 *r )
{
    return wp_make_rect( r[0], r[1], r[2], r[3] );
}

struct wp_rect wp_make_rectiv( const wp_s32 *r )
{
    return wp_make_recti( r[0], r[1], r[2], r[3] );
}

struct wp_vec2f wp_rect_pos( struct wp_rect r )
{
    struct wp_vec2f ret;
    ret.x = r.x;
    ret.y = r.y;
    return ret;
}

struct wp_vec2f wp_rect_size( struct wp_rect r )
{
    struct wp_vec2f ret;
    ret.x = r.w;
    ret.y = r.h;
    return ret;
}

struct wp_rect wp_shrink_make_rect( struct wp_rect r, wp_f32 amount )
{
    struct wp_rect res;
    r.w = WORKPHONE_MAX( r.w, 2 * amount );
    r.h = WORKPHONE_MAX( r.h, 2 * amount );
    res.x = r.x + amount;
    res.y = r.y + amount;
    res.w = r.w - 2 * amount;
    res.h = r.h - 2 * amount;
    return res;
}

struct wp_vec2f wp_make_vec2( wp_f32 x, wp_f32 y )
{
    struct wp_vec2f ret;
    ret.x = x;
    ret.y = y;
    return ret;
}

struct wp_vec2f wp_make_vec2f( wp_f32 x, wp_f32 y )
{
    struct wp_vec2f ret;
    ret.x = x;
    ret.y = y;
    return ret;
}

struct wp_vec2f wp_make_vec2i( wp_s32 x, wp_s32 y )
{
    struct wp_vec2f ret;
    ret.x = (wp_f32)x;
    ret.y = (wp_f32)y;
    return ret;
}
struct wp_vec2f wp_make_vec2v( const wp_f32 *v )
{
    return wp_make_vec2( v[0], v[1] );
}
struct wp_vec2f wp_make_vec2iv( const wp_s32 *v )
{
    return wp_make_vec2i( v[0], v[1] );
}

void wp_unify( struct wp_rect *clip, const struct wp_rect *a, wp_f32 x0, wp_f32 y0, wp_f32 x1,
               wp_f32 y1 )
{
    WORKPHONE_ASSERT( a );
    WORKPHONE_ASSERT( clip );
    clip->x = WORKPHONE_MAX( a->x, x0 );
    clip->y = WORKPHONE_MAX( a->y, y0 );
    clip->w = WORKPHONE_MIN( a->x + a->w, x1 ) - clip->x;
    clip->h = WORKPHONE_MIN( a->y + a->h, y1 ) - clip->y;
    clip->w = WORKPHONE_MAX( 0, clip->w );
    clip->h = WORKPHONE_MAX( 0, clip->h );
}

void wp_triangle_from_direction( struct wp_vec2f *result, struct wp_rect r, wp_f32 pad_x, wp_f32 pad_y,
                                 enum wp_heading direction )
{
    wp_f32 w_half, h_half;
    WORKPHONE_ASSERT( result );

    r.w = WORKPHONE_MAX( 2 * pad_x, r.w );
    r.h = WORKPHONE_MAX( 2 * pad_y, r.h );
    r.w = r.w - 2 * pad_x;
    r.h = r.h - 2 * pad_y;

    r.x = r.x + pad_x;
    r.y = r.y + pad_y;

    w_half = r.w / 2.0f;
    h_half = r.h / 2.0f;

    if( direction == WORKPHONE_UP )
    {
        result[0] = wp_make_vec2( r.x + w_half, r.y );
        result[1] = wp_make_vec2( r.x + r.w, r.y + r.h );
        result[2] = wp_make_vec2( r.x, r.y + r.h );
    }
    else if( direction == WORKPHONE_RIGHT )
    {
        result[0] = wp_make_vec2( r.x, r.y );
        result[1] = wp_make_vec2( r.x + r.w, r.y + h_half );
        result[2] = wp_make_vec2( r.x, r.y + r.h );
    }
    else if( direction == WORKPHONE_DOWN )
    {
        result[0] = wp_make_vec2( r.x, r.y );
        result[1] = wp_make_vec2( r.x + r.w, r.y );
        result[2] = wp_make_vec2( r.x + w_half, r.y + r.h );
    }
    else
    {
        result[0] = wp_make_vec2( r.x, r.y + h_half );
        result[1] = wp_make_vec2( r.x + r.w, r.y );
        result[2] = wp_make_vec2( r.x + r.w, r.y + r.h );
    }
}
