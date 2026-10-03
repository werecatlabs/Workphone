#include "workphone.h"
#include "workphone_prerequisites.h"

// Forward declaration to avoid redefinition error
void wp_color_hsva_f( wp_f32 *out_h, wp_f32 *out_s, wp_f32 *out_v, wp_f32 *out_a, struct wp_color in );

// Forward declaration for wp_hsva_f to ensure proper type resolution
struct wp_color wp_hsva_f( wp_f32 h, wp_f32 s, wp_f32 v, wp_f32 a );

// Forward declaration for wp_hsva to ensure proper type resolution
struct wp_color wp_hsva( wp_s32 h, wp_s32 s, wp_s32 v, wp_s32 a );

wp_s32 wp_parse_hex( const wp_c8 *p, wp_s32 length )
{
    wp_s32 i = 0;
    wp_s32 len = 0;
    while( len < length )
    {
        i <<= 4;
        if( p[len] >= 'a' && p[len] <= 'f' )
            i += ( ( p[len] - 'a' ) + 10 );
        else if( p[len] >= 'A' && p[len] <= 'F' )
            i += ( ( p[len] - 'A' ) + 10 );
        else
            i += ( p[len] - '0' );
        len++;
    }
    return i;
}
struct wp_color wp_rgb_factor( struct wp_color col, wp_f32 factor )
{
    if( factor == 1.0f )
        return col;
    col.r = (wp_byte)( col.r * factor );
    col.g = (wp_byte)( col.g * factor );
    col.b = (wp_byte)( col.b * factor );
    return col;
}
struct wp_color wp_rgba( wp_s32 r, wp_s32 g, wp_s32 b, wp_s32 a )
{
    struct wp_color ret;
    ret.r = (wp_byte)WORKPHONE_CLAMP( 0, r, 255 );
    ret.g = (wp_byte)WORKPHONE_CLAMP( 0, g, 255 );
    ret.b = (wp_byte)WORKPHONE_CLAMP( 0, b, 255 );
    ret.a = (wp_byte)WORKPHONE_CLAMP( 0, a, 255 );
    return ret;
}
struct wp_color wp_rgb_hex( const wp_c8 *rgb )
{
    struct wp_color col;
    const wp_c8 *c = rgb;
    if( *c == '#' )
        c++;
    col.r = (wp_byte)wp_parse_hex( c, 2 );
    col.g = (wp_byte)wp_parse_hex( c + 2, 2 );
    col.b = (wp_byte)wp_parse_hex( c + 4, 2 );
    col.a = 255;
    return col;
}
struct wp_color wp_rgba_hex( const wp_c8 *rgb )
{
    struct wp_color col;
    const wp_c8 *c = rgb;
    if( *c == '#' )
        c++;
    col.r = (wp_byte)wp_parse_hex( c, 2 );
    col.g = (wp_byte)wp_parse_hex( c + 2, 2 );
    col.b = (wp_byte)wp_parse_hex( c + 4, 2 );
    col.a = (wp_byte)wp_parse_hex( c + 6, 2 );
    return col;
}
void wp_color_hex_rgba( wp_c8 *output, struct wp_color col )
{
#define WORKPHONE_TO_HEX( i ) ( ( i ) <= 9 ? '0' + ( i ) : 'A' - 10 + ( i ) )
    output[0] = (wp_c8)WORKPHONE_TO_HEX( ( col.r & 0xF0 ) >> 4 );
    output[1] = (wp_c8)WORKPHONE_TO_HEX( ( col.r & 0x0F ) );
    output[2] = (wp_c8)WORKPHONE_TO_HEX( ( col.g & 0xF0 ) >> 4 );
    output[3] = (wp_c8)WORKPHONE_TO_HEX( ( col.g & 0x0F ) );
    output[4] = (wp_c8)WORKPHONE_TO_HEX( ( col.b & 0xF0 ) >> 4 );
    output[5] = (wp_c8)WORKPHONE_TO_HEX( ( col.b & 0x0F ) );
    output[6] = (wp_c8)WORKPHONE_TO_HEX( ( col.a & 0xF0 ) >> 4 );
    output[7] = (wp_c8)WORKPHONE_TO_HEX( ( col.a & 0x0F ) );
    output[8] = '\0';
#undef WORKPHONE_TO_HEX
}
void wp_color_hex_rgb( wp_c8 *output, struct wp_color col )
{
#define WORKPHONE_TO_HEX( i ) ( ( i ) <= 9 ? '0' + ( i ) : 'A' - 10 + ( i ) )
    output[0] = (wp_c8)WORKPHONE_TO_HEX( ( col.r & 0xF0 ) >> 4 );
    output[1] = (wp_c8)WORKPHONE_TO_HEX( ( col.r & 0x0F ) );
    output[2] = (wp_c8)WORKPHONE_TO_HEX( ( col.g & 0xF0 ) >> 4 );
    output[3] = (wp_c8)WORKPHONE_TO_HEX( ( col.g & 0x0F ) );
    output[4] = (wp_c8)WORKPHONE_TO_HEX( ( col.b & 0xF0 ) >> 4 );
    output[5] = (wp_c8)WORKPHONE_TO_HEX( ( col.b & 0x0F ) );
    output[6] = '\0';
#undef WORKPHONE_TO_HEX
}
struct wp_color wp_rgba_iv( const wp_s32 *c )
{
    return wp_rgba( c[0], c[1], c[2], c[3] );
}
struct wp_color wp_rgba_bv( const wp_byte *c )
{
    return wp_rgba( c[0], c[1], c[2], c[3] );
}
struct wp_color wp_rgb( wp_s32 r, wp_s32 g, wp_s32 b )
{
    struct wp_color ret;
    ret.r = (wp_byte)WORKPHONE_CLAMP( 0, r, 255 );
    ret.g = (wp_byte)WORKPHONE_CLAMP( 0, g, 255 );
    ret.b = (wp_byte)WORKPHONE_CLAMP( 0, b, 255 );
    ret.a = (wp_byte)255;
    return ret;
}
struct wp_color wp_rgb_iv( const wp_s32 *c )
{
    return wp_rgb( c[0], c[1], c[2] );
}
struct wp_color wp_rgb_bv( const wp_byte *c )
{
    return wp_rgb( c[0], c[1], c[2] );
}
struct wp_color wp_rgba_u32( wp_u32 in )
{
    struct wp_color ret;
    ret.r = ( in & 0xFF );
    ret.g = ( ( in >> 8 ) & 0xFF );
    ret.b = ( ( in >> 16 ) & 0xFF );
    ret.a = (wp_byte)( ( in >> 24 ) & 0xFF );
    return ret;
}
struct wp_color wp_rgba_f( wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 a )
{
    struct wp_color ret;
    ret.r = (wp_byte)( WORKPHONE_SATURATE( r ) * 255.0f );
    ret.g = (wp_byte)( WORKPHONE_SATURATE( g ) * 255.0f );
    ret.b = (wp_byte)( WORKPHONE_SATURATE( b ) * 255.0f );
    ret.a = (wp_byte)( WORKPHONE_SATURATE( a ) * 255.0f );
    return ret;
}
struct wp_color wp_rgba_fv( const wp_f32 *c )
{
    return wp_rgba_f( c[0], c[1], c[2], c[3] );
}
struct wp_color wp_rgba_cf( struct wp_colorf c )
{
    return wp_rgba_f( c.r, c.g, c.b, c.a );
}
struct wp_color wp_rgb_f( wp_f32 r, wp_f32 g, wp_f32 b )
{
    struct wp_color ret;
    ret.r = (wp_byte)( WORKPHONE_SATURATE( r ) * 255.0f );
    ret.g = (wp_byte)( WORKPHONE_SATURATE( g ) * 255.0f );
    ret.b = (wp_byte)( WORKPHONE_SATURATE( b ) * 255.0f );
    ret.a = 255;
    return ret;
}
struct wp_color wp_rgb_fv( const wp_f32 *c )
{
    return wp_rgb_f( c[0], c[1], c[2] );
}
struct wp_color wp_rgb_cf( struct wp_colorf c )
{
    return wp_rgb_f( c.r, c.g, c.b );
}
struct wp_colorf wp_hsva_colorf( wp_f32 h, wp_f32 s, wp_f32 v, wp_f32 a )
{
    wp_s32 i;
    wp_f32 p, q, t, f;
    struct wp_colorf out = { 0, 0, 0, 0 };
    if( s <= 0.0f )
    {
        out.r = v;
        out.g = v;
        out.b = v;
        out.a = a;
        return out;
    }
    h = h / ( 60.0f / 360.0f );
    i = (wp_s32)h;
    f = h - (wp_f32)i;
    p = v * ( 1.0f - s );
    q = v * ( 1.0f - ( s * f ) );
    t = v * ( 1.0f - s * ( 1.0f - f ) );

    switch( i )
    {
    case 0:
    default:
        out.r = v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = v;
        out.b = t;
        break;
    case 3:
        out.r = p;
        out.g = q;
        out.b = v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = v;
        break;
    case 5:
        out.r = v;
        out.g = p;
        out.b = q;
        break;
    }
    out.a = a;
    return out;
}
struct wp_colorf wp_hsva_colorfv( const wp_f32 *c )
{
    return wp_hsva_colorf( c[0], c[1], c[2], c[3] );
}
struct wp_color wp_hsva_f( wp_f32 h, wp_f32 s, wp_f32 v, wp_f32 a )
{
    struct wp_colorf c = wp_hsva_colorf( h, s, v, a );
    return wp_rgba_f( c.r, c.g, c.b, c.a );
}
struct wp_color wp_hsva_fv( const wp_f32 *c )
{
    return wp_hsva_f( c[0], c[1], c[2], c[3] );
}
struct wp_color wp_hsv( wp_s32 h, wp_s32 s, wp_s32 v )
{
    return wp_hsva( h, s, v, 255 );
}
struct wp_color wp_hsv_iv( const wp_s32 *c )
{
    return wp_hsv( c[0], c[1], c[2] );
}
struct wp_color wp_hsv_bv( const wp_byte *c )
{
    return wp_hsv( c[0], c[1], c[2] );
}
struct wp_color wp_hsv_f( wp_f32 h, wp_f32 s, wp_f32 v )
{
    return wp_hsva_f( h, s, v, 1.0f );
}
struct wp_color wp_hsv_fv( const wp_f32 *c )
{
    return wp_hsv_f( c[0], c[1], c[2] );
}
struct wp_color wp_hsva( wp_s32 h, wp_s32 s, wp_s32 v, wp_s32 a )
{
    wp_f32 hf = ( (wp_f32)WORKPHONE_CLAMP( 0, h, 255 ) ) / 255.0f;
    wp_f32 sf = ( (wp_f32)WORKPHONE_CLAMP( 0, s, 255 ) ) / 255.0f;
    wp_f32 vf = ( (wp_f32)WORKPHONE_CLAMP( 0, v, 255 ) ) / 255.0f;
    wp_f32 af = ( (wp_f32)WORKPHONE_CLAMP( 0, a, 255 ) ) / 255.0f;
    return wp_hsva_f( hf, sf, vf, af );
}
struct wp_color wp_hsva_iv( const wp_s32 *c )
{
    return wp_hsva( c[0], c[1], c[2], c[3] );
}
struct wp_color wp_hsva_bv( const wp_byte *c )
{
    return wp_hsva( c[0], c[1], c[2], c[3] );
}
wp_u32 wp_color_u32( struct wp_color in )
{
    wp_u32 out = (wp_u32)in.r;
    out |= ( (wp_u32)in.g << 8 );
    out |= ( (wp_u32)in.b << 16 );
    out |= ( (wp_u32)in.a << 24 );
    return out;
}
void wp_color_f( wp_f32 *r, wp_f32 *g, wp_f32 *b, wp_f32 *a, struct wp_color in )
{
    WORKPHONE_STORAGE const wp_f32 s = 1.0f / 255.0f;
    *r = (wp_f32)in.r * s;
    *g = (wp_f32)in.g * s;
    *b = (wp_f32)in.b * s;
    *a = (wp_f32)in.a * s;
}
void wp_color_fv( wp_f32 *c, struct wp_color in )
{
    wp_color_f( &c[0], &c[1], &c[2], &c[3], in );
}
struct wp_colorf wp_color_cf( struct wp_color in )
{
    struct wp_colorf o;
    wp_color_f( &o.r, &o.g, &o.b, &o.a, in );
    return o;
}
void wp_color_d( wp_f64 *r, wp_f64 *g, wp_f64 *b, wp_f64 *a, struct wp_color in )
{
    WORKPHONE_STORAGE const wp_f64 s = 1.0 / 255.0;
    *r = (wp_f64)in.r * s;
    *g = (wp_f64)in.g * s;
    *b = (wp_f64)in.b * s;
    *a = (wp_f64)in.a * s;
}
void wp_color_dv( wp_f64 *c, struct wp_color in )
{
    wp_color_d( &c[0], &c[1], &c[2], &c[3], in );
}
void wp_color_hsv_f( wp_f32 *out_h, wp_f32 *out_s, wp_f32 *out_v, struct wp_color in )
{
    wp_f32 a;
    wp_color_hsva_f( out_h, out_s, out_v, &a, in );
}
void wp_color_hsv_fv( wp_f32 *out, struct wp_color in )
{
    wp_f32 a;
    wp_color_hsva_f( &out[0], &out[1], &out[2], &a, in );
}
void wp_colorf_hsva_f( wp_f32 *out_h, wp_f32 *out_s, wp_f32 *out_v, wp_f32 *out_a, struct wp_colorf in )
{
    wp_f32 chroma;
    wp_f32 K = 0.0f;
    if( in.g < in.b )
    {
        const wp_f32 t = in.g;
        in.g = in.b;
        in.b = t;
        K = -1.f;
    }
    if( in.r < in.g )
    {
        const wp_f32 t = in.r;
        in.r = in.g;
        in.g = t;
        K = -2.f / 6.0f - K;
    }
    chroma = in.r - ( ( in.g < in.b ) ? in.g : in.b );
    *out_h = WORKPHONE_ABS( K + ( in.g - in.b ) / ( 6.0f * chroma + 1e-20f ) );
    *out_s = chroma / ( in.r + 1e-20f );
    *out_v = in.r;
    *out_a = in.a;
}
void wp_colorf_hsva_fv( wp_f32 *hsva, struct wp_colorf in )
{
    wp_colorf_hsva_f( &hsva[0], &hsva[1], &hsva[2], &hsva[3], in );
}
void wp_color_hsva_f( wp_f32 *out_h, wp_f32 *out_s, wp_f32 *out_v, wp_f32 *out_a, struct wp_color in )
{
    struct wp_colorf col;
    wp_color_f( &col.r, &col.g, &col.b, &col.a, in );
    wp_colorf_hsva_f( out_h, out_s, out_v, out_a, col );
}
void wp_color_hsva_fv( wp_f32 *out, struct wp_color in )
{
    wp_color_hsva_f( &out[0], &out[1], &out[2], &out[3], in );
}
void wp_color_hsva_i( wp_s32 *out_h, wp_s32 *out_s, wp_s32 *out_v, wp_s32 *out_a, struct wp_color in )
{
    wp_f32 h, s, v, a;
    wp_color_hsva_f( &h, &s, &v, &a, in );
    *out_h = (wp_byte)( h * 255.0f );
    *out_s = (wp_byte)( s * 255.0f );
    *out_v = (wp_byte)( v * 255.0f );
    *out_a = (wp_byte)( a * 255.0f );
}
void wp_color_hsva_iv( wp_s32 *out, struct wp_color in )
{
    wp_color_hsva_i( &out[0], &out[1], &out[2], &out[3], in );
}
void wp_color_hsva_bv( wp_byte *out, struct wp_color in )
{
    wp_s32 tmp[4];
    wp_color_hsva_i( &tmp[0], &tmp[1], &tmp[2], &tmp[3], in );
    out[0] = (wp_byte)tmp[0];
    out[1] = (wp_byte)tmp[1];
    out[2] = (wp_byte)tmp[2];
    out[3] = (wp_byte)tmp[3];
}

void wp_color_hsva_b( wp_byte *h, wp_byte *s, wp_byte *v, wp_byte *a, struct wp_color in )
{
    wp_s32 tmp[4];
    wp_color_hsva_i( &tmp[0], &tmp[1], &tmp[2], &tmp[3], in );
    *h = (wp_byte)tmp[0];
    *s = (wp_byte)tmp[1];
    *v = (wp_byte)tmp[2];
    *a = (wp_byte)tmp[3];
}

void wp_color_hsv_i( wp_s32 *out_h, wp_s32 *out_s, wp_s32 *out_v, struct wp_color in )
{
    wp_s32 a;
    wp_color_hsva_i( out_h, out_s, out_v, &a, in );
}

void wp_color_hsv_b( wp_byte *h, wp_byte *s, wp_byte *v, wp_byte *a, struct wp_color in )
{
    wp_s32 tmp[4];
    wp_color_hsva_i( &tmp[0], &tmp[1], &tmp[2], &tmp[3], in );
    *h = (wp_byte)tmp[0];
    *s = (wp_byte)tmp[1];
    *v = (wp_byte)tmp[2];
    *a = (wp_byte)tmp[3];
}

void wp_color_hsv_iv( wp_s32 *out, struct wp_color in )
{
    wp_color_hsv_i( &out[0], &out[1], &out[2], in );
}

void wp_color_hsv_bv( wp_byte *out, struct wp_color in )
{
    wp_s32 tmp[4];
    wp_color_hsv_i( &tmp[0], &tmp[1], &tmp[2], in );
    out[0] = (wp_byte)tmp[0];
    out[1] = (wp_byte)tmp[1];
    out[2] = (wp_byte)tmp[2];
}
