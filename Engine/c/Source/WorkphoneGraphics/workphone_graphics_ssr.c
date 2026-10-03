/**
 * @file wp_graphics_ssr.c
 * @brief Screen Space Reflections implementation
 */

#include "workphone_graphics_ssr.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct wp_ssr
{
    wp_s32 width;
    wp_s32 height;
    wp_s32 half_w;
    wp_s32 half_h;
    wp_f32 *raw_buffer;
    wp_f32 *blur_buffer;
    wp_f32 max_dist;
    wp_f32 thickness;
    wp_f32 strength;
};

static wp_f32 wp_cl( wp_f32 x, wp_f32 a, wp_f32 b )
{
    return x < a ? a : x > b ? b : x;
}

static wp_f32 wp_ign( wp_f32 x, wp_f32 y )
{
    return (wp_f32)fmod( 52.9829189f * (wp_f32)fmod( x * 0.06711056f + y * 0.00583715f, 1.0f ), 1.0f );
}

static void wp_decode_normal( wp_f32 nx, wp_f32 ny, wp_f32 *ox, wp_f32 *oy, wp_f32 *oz )
{
    wp_f32 z = 1.0f - (wp_f32)fabs( nx ) - (wp_f32)fabs( ny );
    wp_f32 tx = nx, ty = ny;
    if( z < 0 )
    {
        wp_f32 ot = tx;
        tx = ( 1.0f - (wp_f32)fabs( ty ) ) * ( tx >= 0 ? 1.0f : -1.0f );
        ty = ( 1.0f - (wp_f32)fabs( ot ) ) * ( ty >= 0 ? 1.0f : -1.0f );
    }
    wp_f32 len = (wp_f32)sqrt( tx * tx + ty * ty + z * z );
    if( len < 1e-6f )
        len = 1e-6f;
    *ox = tx / len;
    *oy = ty / len;
    *oz = z / len;
}

static void wp_view_pos( wp_f32 u, wp_f32 v, wp_f32 d, const wp_mat4f *inv, wp_f32 *ox, wp_f32 *oy,
                         wp_f32 *oz )
{
    wp_f32 hx = inv->m[0][0] * ( u * 2 - 1 ) + inv->m[0][2] * d + inv->m[0][3];
    wp_f32 hy = inv->m[1][0] * ( u * 2 - 1 ) + inv->m[1][2] * d + inv->m[1][3];
    wp_f32 hz = inv->m[2][0] * ( u * 2 - 1 ) + inv->m[2][2] * d + inv->m[2][3];
    wp_f32 hw = inv->m[3][0] * ( u * 2 - 1 ) + inv->m[3][2] * d + inv->m[3][3];
    wp_f32 iw = 1.0f / hw;
    wp_f32 rx = hx * iw, ry = hy * iw, rz = hz * iw;
    wp_f32 fx = inv->m[0][0] * ( u * 2 - 1 ) + inv->m[0][2] + inv->m[0][3] * iw;
    wp_f32 fy = inv->m[1][0] * ( u * 2 - 1 ) + inv->m[1][2] + inv->m[1][3] * iw;
    wp_f32 fz = inv->m[2][0] * ( u * 2 - 1 ) + inv->m[2][2] + inv->m[2][3] * iw;
    wp_f32 len = (wp_f32)sqrt( fx * fx + fy * fy + fz * fz );
    if( len < 1e-6f )
        len = 1e-6f;
    *ox = rx / len * d;
    *oy = ry / len * d;
    *oz = -d;
}

static wp_f32 wp_sample_d( const wp_f32 *b, wp_s32 w, wp_s32 h, wp_f32 u, wp_f32 v )
{
    if( u < 0 || u >= 1 || v < 0 || v >= 1 )
        return 1e8f;
    wp_s32 ix = (wp_s32)( u * ( w - 1 ) );
    wp_s32 iy = (wp_s32)( v * ( h - 1 ) );
    return b[iy * w + ix];
}

static void wp_sample_n( const wp_f32 *b, wp_s32 w, wp_s32 h, wp_f32 u, wp_f32 v, wp_f32 *nx, wp_f32 *ny,
                         wp_f32 *cov )
{
    if( u < 0 || u >= 1 || v < 0 || v >= 1 )
    {
        *nx = 0;
        *ny = 0;
        *cov = 0;
        return;
    }
    wp_s32 ix = (wp_s32)( u * ( w - 1 ) );
    wp_s32 iy = (wp_s32)( v * ( h - 1 ) );
    wp_s32 i = ( iy * w + ix ) * 4;
    *nx = b[i];
    *ny = b[i + 1];
    *cov = b[i + 2];
}

static void wp_project( wp_f32 x, wp_f32 y, wp_f32 z, const wp_mat4f *p, wp_f32 *u, wp_f32 *v )
{
    wp_f32 c = p->m[0][0] * x + p->m[0][1] * y + p->m[0][2] * z + p->m[0][3];
    wp_f32 c2 = p->m[1][0] * x + p->m[1][1] * y + p->m[1][2] * z + p->m[1][3];
    wp_f32 cw = p->m[3][0] * x + p->m[3][1] * y + p->m[3][2] * z + p->m[3][3];
    wp_f32 iw = 1.0f / cw;
    *u = c * iw * 0.5f + 0.5f;
    *v = c2 * iw * 0.5f + 0.5f;
}

wp_ssr *wp_ssr_create( wp_s32 w, wp_s32 h )
{
    wp_ssr *c = calloc( 1, sizeof( wp_ssr ) );
    if( !c )
        return NULL;
    c->width = w;
    c->height = h;
    c->half_w = ( w + 1 ) / 2;
    c->half_h = ( h + 1 ) / 2;
    wp_s32 hp = c->half_w * c->half_h;
    c->raw_buffer = calloc( (size_t)hp * 4, sizeof( wp_f32 ) );
    c->blur_buffer = calloc( (size_t)hp * 4, sizeof( wp_f32 ) );
    c->max_dist = WP_SSR_DEFAULT_DIST;
    c->thickness = WP_SSR_DEFAULT_THICKNESS;
    c->strength = WP_SSR_DEFAULT_STRENGTH;
    return c;
}

void wp_ssr_destroy( wp_ssr *c )
{
    if( !c )
        return;
    free( c->raw_buffer );
    free( c->blur_buffer );
    free( c );
}

void wp_ssr_render( wp_ssr *c, const wp_f32 *color, const wp_f32 *depth, const wp_f32 *normal,
                    const wp_f32 *vel, const wp_mat4f *proj, const wp_mat4f *proj_inv, wp_s32 frame )
{
    if( !c )
        return;
    for( wp_s32 y = 0; y < c->half_h; y++ )
    {
        for( wp_s32 x = 0; x < c->half_w; x++ )
        {
            wp_f32 u = ( (wp_f32)x + 0.5f ) / c->half_w;
            wp_f32 v = ( (wp_f32)y + 0.5f ) / c->half_h;
            wp_s32 hi = ( y * c->half_w + x ) * 4;

            wp_f32 d = wp_sample_d( depth, c->width, c->height, u, v );
            wp_f32 nx, ny, cov;
            wp_sample_n( normal, c->width, c->height, u, v, &nx, &ny, &cov );

            if( cov < 0.5f || d > 1e7f )
            {
                c->raw_buffer[hi] = 0;
                c->raw_buffer[hi + 1] = 0;
                c->raw_buffer[hi + 2] = 0;
                c->raw_buffer[hi + 3] = 0;
                continue;
            }

            wp_f32 px, py, pz;
            wp_view_pos( u, v, d, proj_inv, &px, &py, &pz );
            wp_f32 nnx, nny, nnz;
            wp_decode_normal( nx, ny, &nnx, &nny, &nnz );

            wp_f32 vx = -px, vy = -py, vz = -pz;
            wp_f32 vlen = (wp_f32)sqrt( vx * vx + vy * vy + vz * vz );
            if( vlen > 1e-6f )
            {
                vx /= vlen;
                vy /= vlen;
                vz /= vlen;
            }

            wp_f32 NdV = nnx * ( -vx ) + nny * ( -vy ) + nnz * ( -vz );
            if( NdV > 0.94f )
            {
                c->raw_buffer[hi] = 0;
                c->raw_buffer[hi + 1] = 0;
                c->raw_buffer[hi + 2] = 0;
                c->raw_buffer[hi + 3] = 0;
                continue;
            }

            wp_f32 rx = 2.0f * ( nnx * vx + nny * vy + nnz * vz ) * nnx - vx;
            wp_f32 ry = 2.0f * ( nnx * vx + nny * vy + nnz * vz ) * nny - vy;
            wp_f32 rz = 2.0f * ( nnx * vx + nny * vy + nnz * vz ) * nnz - vz;

            wp_f32 j = wp_ign( (wp_f32)x + 7.331f * (wp_f32)( frame % 64 ), (wp_f32)y );
            wp_f32 t = 0.06f + j * 0.06f;
            wp_f32 scale = (wp_f32)pow( c->max_dist / 0.06f, 1.0f / WP_SSR_STEPS );
            wp_f32 hit_u = 0, hit_v = 0;
            wp_s32 hit = 0;

            for( wp_s32 i = 0; i < WP_SSR_STEPS; i++ )
            {
                wp_f32 spx = px + rx * t, spy = py + ry * t, spz = pz + rz * t;
                if( spz > -0.05f )
                    break;
                wp_project( spx, spy, spz, proj, &hit_u, &hit_v );
                if( hit_u <= 0 || hit_u >= 1 || hit_v <= 0 || hit_v >= 1 )
                    break;
                wp_f32 sd = wp_sample_d( depth, c->width, c->height, hit_u, hit_v );
                wp_f32 sc;
                wp_sample_n( normal, c->width, c->height, hit_u, hit_v, &nx, &ny, &sc );
                if( sc > 0.5f && -spz - sd > 0 && -spz - sd < c->thickness + t * 0.06f )
                {
                    hit = 1;
                    break;
                }
                t *= scale;
                if( t > c->max_dist )
                    break;
            }

            wp_f32 conf = hit ? wp_cl( hit_u / 0.12f, 0, 1 ) * wp_cl( ( 1.0f - hit_u ) / 0.12f, 0, 1 ) *
                                    wp_cl( hit_v / 0.12f, 0, 1 ) *
                                    wp_cl( ( 1.0f - hit_v ) / 0.12f, 0, 1 ) * c->strength
                              : 0;
            c->raw_buffer[hi] = hit_u;
            c->raw_buffer[hi + 1] = hit_v;
            c->raw_buffer[hi + 2] = 0;
            c->raw_buffer[hi + 3] = wp_cl( conf, 0, 1 );
        }
    }
}

const wp_f32 *wp_ssr_get_texture( const wp_ssr *c )
{
    return c ? c->raw_buffer : NULL;
}

void wp_ssr_resize( wp_ssr *c, wp_s32 w, wp_s32 h )
{
    if( !c )
        return;
    c->width = w;
    c->height = h;
    c->half_w = ( w + 1 ) / 2;
    c->half_h = ( h + 1 ) / 2;
    wp_s32 hp = c->half_w * c->half_h;
    c->raw_buffer = realloc( c->raw_buffer, (size_t)hp * 4 * sizeof( wp_f32 ) );
    c->blur_buffer = realloc( c->blur_buffer, (size_t)hp * 4 * sizeof( wp_f32 ) );
}
