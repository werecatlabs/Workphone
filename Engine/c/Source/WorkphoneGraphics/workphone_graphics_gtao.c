/**
 * @file wp_graphics_gtao.c
 * @brief Ground-Truth Ambient Occlusion implementation.
 */

#include "workphone_graphics_gtao.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WP_GTAO_PI 3.141592653589793f
#define WP_GTAO_IGN_A 0.06711056f
#define WP_GTAO_IGN_B 0.00583715f
#define WP_GTAO_HASH_K1 0.1031f

/* =========================================================================
 * Internal structures
 * ====================================================================== */

/**
 * GTAO context
 */
struct wp_gtao
{
    /** Render width */
    wp_s32 width;
    /** Render height */
    wp_s32 height;
    /** Total pixels */
    wp_s32 pixel_count;

    /** Configuration */
    wp_gtao_config config;

    /** Buffers */
    wp_f32 *raw_buffer; /* visibility + depth */
    wp_f32 *history_a;  /* ping-pong history buffers */
    wp_f32 *history_b;
    wp_f32 *blur_a; /* blur buffers */
    wp_f32 *blur_b;
    wp_f32 *history_current; /* current history buffer */
    wp_f32 *history_prev;    /* previous history buffer */
    wp_f32 *output;          /* final output buffer */

    /** Derived values */
    wp_f32 texel_x;
    wp_f32 texel_y;

    /** State */
    wp_s32 reset_history;
    wp_s32 frame;
    wp_mat4f proj_inv;
    wp_f32 p11;
};

static wp_f32 wp_max( wp_f32 a, wp_f32 b )
{
    return a > b ? a : b;
}
static wp_f32 wp_min( wp_f32 a, wp_f32 b )
{
    return a < b ? a : b;
}
static wp_f32 wp_abs( wp_f32 x )
{
    return x < 0 ? -x : x;
}
static wp_f32 wp_clamp( wp_f32 x, wp_f32 a, wp_f32 b )
{
    return x < a ? a : x > b ? b : x;
}
static wp_f32 wp_acos( wp_f32 x )
{
    x = wp_clamp( x, -1.0f, 1.0f );
    return (wp_f32)acos( x );
}

static wp_f32 wp_gtao_ign( wp_f32 x, wp_f32 y )
{
    return (wp_f32)fmod( 52.9829189f * (wp_f32)fmod( x * WP_GTAO_IGN_A + y * WP_GTAO_IGN_B, 1.0f ),
                         1.0f );
}

static wp_f32 wp_gtao_hash12( wp_f32 x, wp_f32 y )
{
    wp_f32 px = x * WP_GTAO_HASH_K1 + y * WP_GTAO_HASH_K1;
    wp_f32 py = y * WP_GTAO_HASH_K1 + x * WP_GTAO_HASH_K1;
    wp_f32 pz = x * 33.33f;
    px = px - (wp_f32)floor( px );
    py = py - (wp_f32)floor( py );
    pz = pz - (wp_f32)floor( pz );
    px += py + pz;
    py += px + pz;
    px += py * px;
    px = px - (wp_f32)floor( px );
    py = py - (wp_f32)floor( py );
    return px + py;
}

static void wp_decode_normal( wp_f32 nx, wp_f32 ny, wp_f32 *ox, wp_f32 *oy, wp_f32 *oz )
{
    wp_f32 z = 1.0f - wp_abs( nx ) - wp_abs( ny );
    wp_f32 tx = nx, ty = ny;
    if( z < 0.0f )
    {
        wp_f32 old_tx = tx;
        tx = ( 1.0f - wp_abs( ty ) ) * ( tx >= 0.0f ? 1.0f : -1.0f );
        ty = ( 1.0f - wp_abs( old_tx ) ) * ( ty >= 0.0f ? 1.0f : -1.0f );
    }
    wp_f32 len = (wp_f32)sqrt( tx * tx + ty * ty + z * z );
    if( len < 1e-6f )
        len = 1e-6f;
    *ox = tx / len;
    *oy = ty / len;
    *oz = z / len;
}

static void wp_view_pos( wp_f32 u, wp_f32 v, wp_f32 depth, const wp_mat4f *inv, wp_f32 *ox, wp_f32 *oy,
                         wp_f32 *oz )
{
    wp_f32 hx = inv->m[0][0] * ( u * 2 - 1 ) + inv->m[0][2] * depth + inv->m[0][3];
    wp_f32 hy = inv->m[1][0] * ( u * 2 - 1 ) + inv->m[1][2] * depth + inv->m[1][3];
    wp_f32 hz = inv->m[2][0] * ( u * 2 - 1 ) + inv->m[2][2] * depth + inv->m[2][3];
    wp_f32 hw = inv->m[3][0] * ( u * 2 - 1 ) + inv->m[3][2] * depth + inv->m[3][3];
    wp_f32 iw = 1.0f / hw;
    wp_f32 rx = hx * iw, ry = hy * iw, rz = hz * iw;
    wp_f32 fx = inv->m[0][0] * ( u * 2 - 1 ) + inv->m[0][2] + inv->m[0][3] * iw;
    wp_f32 fy = inv->m[1][0] * ( u * 2 - 1 ) + inv->m[1][2] + inv->m[1][3] * iw;
    wp_f32 fz = inv->m[2][0] * ( u * 2 - 1 ) + inv->m[2][2] + inv->m[2][3] * iw;
    wp_f32 len = (wp_f32)sqrt( fx * fx + fy * fy + fz * fz );
    if( len < 1e-6f )
        len = 1e-6f;
    *ox = rx / len * depth;
    *oy = ry / len * depth;
    *oz = -depth;
}

static wp_f32 wp_sample_depth( const wp_f32 *b, wp_s32 w, wp_s32 h, wp_f32 u, wp_f32 v )
{
    if( u < 0 || u >= 1 || v < 0 || v >= 1 )
        return 1e8f;
    return b[( (wp_s32)( v * ( h - 1 ) ) * w + (wp_s32)( u * ( w - 1 ) ) )];
}

static void wp_sample_normal( const wp_f32 *b, wp_s32 w, wp_s32 h, wp_f32 u, wp_f32 v, wp_f32 *nx,
                              wp_f32 *ny, wp_f32 *cov )
{
    if( u < 0 || u >= 1 || v < 0 || v >= 1 )
    {
        *nx = 0;
        *ny = 0;
        *cov = 0;
        return;
    }
    wp_s32 i = ( (wp_s32)( v * ( h - 1 ) ) * w + (wp_s32)( u * ( w - 1 ) ) ) * 3;
    *nx = b[i];
    *ny = b[i + 1];
    *cov = b[i + 2];
}

// Forward declarations for functions called before definition
static void wp_gtao_temporal( wp_gtao *c );
static void wp_gtao_blur( wp_gtao *c );

wp_gtao *wp_gtao_create( wp_s32 w, wp_s32 h, const wp_gtao_config *cfg )
{
    wp_gtao *c = calloc( 1, sizeof( wp_gtao ) );
    if( !c )
        return NULL;
    c->width = w;
    c->height = h;
    c->pixel_count = w * h;
    if( cfg )
    {
        c->config = *cfg;
    }
    else
    {
        c->config.radius = 0.9f;
        c->config.intensity = 1.35f;
        c->config.thickness = 0.4f;
        c->config.quality = 2;
    }
    wp_s32 px = c->pixel_count;
    c->raw_buffer = calloc( (size_t)px * 2, sizeof( wp_f32 ) );
    c->history_a = calloc( (size_t)px * 2, sizeof( wp_f32 ) );
    c->history_b = calloc( (size_t)px * 2, sizeof( wp_f32 ) );
    c->blur_a = calloc( (size_t)px * 2, sizeof( wp_f32 ) );
    c->blur_b = calloc( (size_t)px * 2, sizeof( wp_f32 ) );
    c->history_current = c->history_a;
    c->history_prev = c->history_b;
    c->output = c->raw_buffer;
    c->texel_x = 1.0f / w;
    c->texel_y = 1.0f / h;
    c->reset_history = 1;
    return c;
}

void wp_gtao_destroy( wp_gtao *c )
{
    if( !c )
        return;
    free( c->raw_buffer );
    free( c->history_a );
    free( c->history_b );
    free( c->blur_a );
    free( c->blur_b );
    free( c );
}

static wp_f32 wp_arc( wp_f32 h, wp_f32 n, wp_f32 cos_n, wp_f32 sin_n )
{
    return 0.25f * ( -(wp_f32)cos( 2.0f * h - n ) + cos_n + 2.0f * h * sin_n );
}

void wp_gtao_render( wp_gtao *c, const wp_f32 *depth, const wp_f32 *normal, const wp_mat4f *proj_inv,
                     const wp_mat4f *proj, wp_s32 frame, wp_s32 temporal )
{
    if( !c || !depth || !normal )
        return;
    c->frame = frame;
    c->proj_inv = *proj_inv;
    c->p11 = proj->m[1][1];

    for( wp_s32 y = 0; y < c->height; y++ )
    {
        for( wp_s32 x = 0; x < c->width; x++ )
        {
            wp_f32 u = ( (wp_f32)x + 0.5f ) / c->width;
            wp_f32 v = ( (wp_f32)y + 0.5f ) / c->height;
            wp_s32 pi = y * c->width + x;
            wp_s32 bi = pi * 2;

            wp_f32 d = depth[pi];
            wp_f32 nx, ny, cov;
            wp_sample_normal( normal, c->width, c->height, u, v, &nx, &ny, &cov );
            if( cov < 0.5f )
            {
                c->raw_buffer[bi] = 1.0f;
                c->raw_buffer[bi + 1] = 1e4f;
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

            wp_f32 radius_px = c->config.radius * c->p11 * 0.5f * (wp_f32)c->height / wp_max( 0.2f, d );
            radius_px = wp_clamp( radius_px, 6.0f, 128.0f );

            wp_f32 noise = wp_gtao_ign( (wp_f32)x + (wp_f32)( frame % 64 ) * 5.588238f, (wp_f32)y );
            wp_f32 noise2 = wp_gtao_hash12( (wp_f32)x * 0.371f + (wp_f32)( frame % 64 ), (wp_f32)y );
            wp_f32 inv_r2 = 1.0f / ( c->config.radius * c->config.radius );
            wp_f32 visibility = 0.0f;

            for( wp_s32 s = 0; s < 3; s++ )
            {
                wp_f32 phi = ( (wp_f32)s + noise ) * ( WP_GTAO_PI / 3.0f );
                wp_f32 dir_x = (wp_f32)cos( phi ), dir_y = (wp_f32)sin( phi );

                wp_f32 ax = dir_y * vz - 0.0f * vy, ay = 0.0f * vx - dir_x * vz,
                       az = dir_x * vy - dir_y * vx;
                wp_f32 al = (wp_f32)sqrt( ax * ax + ay * ay + az * az );
                if( al < 1e-4f )
                    continue;
                ax /= al;
                ay /= al;
                az /= al;

                wp_f32 pxn = nnx - ax * ( nnx * ax + nny * ay + nnz * az );
                wp_f32 pyn = nny - ay * ( nnx * ax + nny * ay + nnz * az );
                wp_f32 pzn = nnz - az * ( nnx * ax + nny * ay + nnz * az );
                wp_f32 pl = (wp_f32)sqrt( pxn * pxn + pyn * pyn + pzn * pzn );
                if( pl < 1e-4f )
                    continue;
                pxn /= pl;
                pyn /= pl;
                pzn /= pl;

                wp_f32 ox = dir_y * az - 0.0f * ay, oy = 0.0f * ax - dir_x * az,
                       oz = dir_x * ay - dir_y * ax;
                wp_f32 ol = (wp_f32)sqrt( ox * ox + oy * oy + oz * oz );
                if( ol < 1e-6f )
                    ol = 1e-6f;
                ox /= ol;
                oy /= ol;
                oz /= ol;

                wp_f32 cos_n = pxn * vx + pyn * vy + pzn * vz;
                cos_n = wp_clamp( cos_n, -1.0f, 1.0f );
                wp_f32 n_angle = (wp_f32)acos( cos_n );
                wp_f32 sign_n = ( ox * pxn + oy * pyn + oz * pzn ) >= 0.0f ? 1.0f : -1.0f;
                n_angle *= sign_n;
                wp_f32 sin_n = (wp_f32)sin( n_angle );

                wp_f32 cos_h_pos = -1.0f, cos_h_neg = -1.0f;

                for( wp_s32 t = 0; t < 8; t++ )
                {
                    wp_f32 ft = ( (wp_f32)t + noise2 ) / 8.0f;
                    wp_f32 off = radius_px * ft * ft + 1.0f;
                    wp_f32 du = dir_x * off * c->texel_x;
                    wp_f32 dv = dir_y * off * c->texel_y;

                    wp_f32 su1 = u + du, sv1 = v + dv;
                    if( su1 > 0 && su1 < 1 && sv1 > 0 && sv1 < 1 )
                    {
                        wp_f32 d1 = wp_sample_depth( depth, c->width, c->height, su1, sv1 );
                        wp_f32 c1;
                        wp_sample_normal( normal, c->width, c->height, su1, sv1, &nx, &ny, &c1 );
                        if( c1 > 0.5f )
                        {
                            wp_f32 dsx, dsy, dsz;
                            wp_view_pos( su1, sv1, d1, proj_inv, &dsx, &dsy, &dsz );
                            dsx -= px;
                            dsy -= py;
                            dsz -= pz;
                            wp_f32 len2 = dsx * dsx + dsy * dsy + dsz * dsz;
                            if( len2 > 2e-5f )
                            {
                                wp_f32 inv = 1.0f / (wp_f32)sqrt( len2 );
                                wp_f32 cc = ( dsx * vx + dsy * vy + dsz * vz ) * inv;
                                wp_f32 fall = wp_clamp( len2 * inv_r2, 0.0f, 1.0f );
                                fall *= fall;
                                cos_h_pos = wp_max( cos_h_pos, wp_max( cc, cos_n ) * ( 1.0f - fall ) +
                                                                   cos_h_pos * fall );
                            }
                        }
                    }

                    wp_f32 su2 = u - du, sv2 = v - dv;
                    if( su2 > 0 && su2 < 1 && sv2 > 0 && sv2 < 1 )
                    {
                        wp_f32 d2 = wp_sample_depth( depth, c->width, c->height, su2, sv2 );
                        wp_f32 c2;
                        wp_sample_normal( normal, c->width, c->height, su2, sv2, &nx, &ny, &c2 );
                        if( c2 > 0.5f )
                        {
                            wp_f32 dsx, dsy, dsz;
                            wp_view_pos( su2, sv2, d2, proj_inv, &dsx, &dsy, &dsz );
                            dsx -= px;
                            dsy -= py;
                            dsz -= pz;
                            wp_f32 len2 = dsx * dsx + dsy * dsy + dsz * dsz;
                            if( len2 > 2e-5f )
                            {
                                wp_f32 inv = 1.0f / (wp_f32)sqrt( len2 );
                                wp_f32 cc = ( dsx * vx + dsy * vy + dsz * vz ) * inv;
                                wp_f32 fall = wp_clamp( len2 * inv_r2, 0.0f, 1.0f );
                                fall *= fall;
                                cos_h_neg = wp_max( cos_h_neg, wp_max( cc, cos_n ) * ( 1.0f - fall ) +
                                                                   cos_h_neg * fall );
                            }
                        }
                    }
                }

                wp_f32 h1 = -wp_acos( wp_clamp( cos_h_pos, -1.0f, 1.0f ) );
                wp_f32 h2 = -wp_acos( wp_clamp( cos_h_neg, -1.0f, 1.0f ) );
                visibility += wp_arc( h1, n_angle, cos_n, sin_n );
                visibility += wp_arc( -h2, -n_angle, cos_n, sin_n );
            }

            visibility = 1.0f - visibility / ( 3.0f * WP_GTAO_PI );
            visibility = wp_clamp( visibility, 0.0f, 1.0f );
            c->raw_buffer[bi] = visibility;
            c->raw_buffer[bi + 1] = d;
        }
    }

    if( temporal )
        wp_gtao_temporal( c );
    wp_gtao_blur( c );
}

void wp_gtao_temporal( wp_gtao *c )
{
    if( !c )
        return;
    wp_f32 feedback = 0.92f;

    for( wp_s32 y = 0; y < c->height; y++ )
    {
        for( wp_s32 x = 0; x < c->width; x++ )
        {
            wp_s32 pi = y * c->width + x;
            wp_s32 bi = pi * 2;
            wp_f32 cur = c->raw_buffer[bi];
            wp_f32 cur_d = c->raw_buffer[bi + 1];
            wp_f32 hist = c->history_prev[bi];
            wp_f32 hist_d = c->history_prev[bi + 1];

            wp_f32 rel_diff = wp_abs( hist_d - cur_d ) / wp_max( 0.05f, cur_d );
            feedback = 0.92f * (wp_f32)exp( -rel_diff * 30.0f );

            wp_f32 mn = cur, mx = cur;
            for( wp_s32 ny = -1; ny <= 1; ny++ )
            {
                for( wp_s32 nx = -1; nx <= 1; nx++ )
                {
                    if( nx == 0 && ny == 0 )
                        continue;
                    wp_s32 sx = x + nx, sy = y + ny;
                    if( sx < 0 || sx >= c->width || sy < 0 || sy >= c->height )
                        continue;
                    wp_f32 s = c->raw_buffer[( sy * c->width + sx ) * 2];
                    mn = wp_min( mn, s );
                    mx = wp_max( mx, s );
                }
            }

            wp_f32 clamped = wp_clamp( hist, mn - 0.45f, mx + 0.45f );
            wp_f32 result = cur * ( 1.0f - feedback ) + clamped * feedback;
            result = wp_clamp( result, 0.0f, 1.0f );
            c->history_current[bi] = result;
            c->history_current[bi + 1] = cur_d;
        }
    }

    wp_f32 *tmp = c->history_prev;
    c->history_prev = c->history_current;
    c->history_current = tmp;
}

void wp_gtao_blur( wp_gtao *c )
{
    if( !c )
        return;

    for( wp_s32 y = 0; y < c->height; y++ )
    {
        for( wp_s32 x = 0; x < c->width; x++ )
        {
            wp_s32 pi = y * c->width + x;
            wp_s32 bi = pi * 2;
            wp_f32 center = c->history_prev[bi];
            wp_f32 center_d = c->history_prev[bi + 1];
            wp_f32 sum = center * 0.4f, wsum = 0.4f;

            for( wp_s32 i = 1; i <= 3; i++ )
            {
                if( x - i >= 0 )
                {
                    wp_f32 s = c->history_prev[( y * c->width + x - i ) * 2];
                    wp_f32 d = c->history_prev[( y * c->width + x - i ) * 2 + 1];
                    wp_f32 w = 0.4f / ( i + 1 );
                    wp_f32 dw =
                        (wp_f32)exp( -wp_abs( d - center_d ) * 22.0f / wp_max( 0.1f, center_d ) );
                    sum += s * w * dw;
                    wsum += w * dw;
                }
                if( x + i < c->width )
                {
                    wp_f32 s = c->history_prev[( y * c->width + x + i ) * 2];
                    wp_f32 d = c->history_prev[( y * c->width + x + i ) * 2 + 1];
                    wp_f32 w = 0.4f / ( i + 1 );
                    wp_f32 dw =
                        (wp_f32)exp( -wp_abs( d - center_d ) * 22.0f / wp_max( 0.1f, center_d ) );
                    sum += s * w * dw;
                    wsum += w * dw;
                }
            }
            c->blur_a[bi] = sum / wsum;
            c->blur_a[bi + 1] = center_d;
        }
    }

    for( wp_s32 y = 0; y < c->height; y++ )
    {
        for( wp_s32 x = 0; x < c->width; x++ )
        {
            wp_s32 pi = y * c->width + x;
            wp_s32 bi = pi * 2;
            wp_f32 center = c->blur_a[bi];
            wp_f32 center_d = c->blur_a[bi + 1];
            wp_f32 sum = center * 0.4f, wsum = 0.4f;

            for( wp_s32 i = 1; i <= 3; i++ )
            {
                if( y - i >= 0 )
                {
                    wp_f32 s = c->blur_a[( ( y - i ) * c->width + x ) * 2];
                    wp_f32 d = c->blur_a[( ( y - i ) * c->width + x ) * 2 + 1];
                    wp_f32 w = 0.4f / ( i + 1 );
                    wp_f32 dw =
                        (wp_f32)exp( -wp_abs( d - center_d ) * 22.0f / wp_max( 0.1f, center_d ) );
                    sum += s * w * dw;
                    wsum += w * dw;
                }
                if( y + i < c->height )
                {
                    wp_f32 s = c->blur_a[( ( y + i ) * c->width + x ) * 2];
                    wp_f32 d = c->blur_a[( ( y + i ) * c->width + x ) * 2 + 1];
                    wp_f32 w = 0.4f / ( i + 1 );
                    wp_f32 dw =
                        (wp_f32)exp( -wp_abs( d - center_d ) * 22.0f / wp_max( 0.1f, center_d ) );
                    sum += s * w * dw;
                    wsum += w * dw;
                }
            }

            wp_f32 val = sum / wsum;
            val = (wp_f32)pow( wp_clamp( val, 0.0f, 1.0f ), c->config.intensity );
            c->blur_b[bi] = val;
            c->blur_b[bi + 1] = center_d;
        }
    }

    c->output = c->blur_b;
}

const wp_f32 *wp_gtao_get_buffer( const wp_gtao *c )
{
    return c ? c->output : NULL;
}
void wp_gtao_get_size( const wp_gtao *c, wp_s32 *w, wp_s32 *h )
{
    if( c )
    {
        if( w )
            *w = c->width;
        if( h )
            *h = c->height;
    }
    else
    {
        if( w )
            *w = 0;
        if( h )
            *h = 0;
    }
}
void wp_gtao_set_radius( wp_gtao *c, wp_f32 r )
{
    if( c )
        c->config.radius = r;
}
void wp_gtao_set_intensity( wp_gtao *c, wp_f32 i )
{
    if( c )
        c->config.intensity = i;
}
void wp_gtao_resize( wp_gtao *c, wp_s32 w, wp_s32 h )
{
    if( !c || w <= 0 || h <= 0 )
        return;
    c->width = w;
    c->height = h;
    c->pixel_count = w * h;
    c->raw_buffer = realloc( c->raw_buffer, (size_t)w * h * 2 * sizeof( wp_f32 ) );
    c->history_a = realloc( c->history_a, (size_t)w * h * 2 * sizeof( wp_f32 ) );
    c->history_b = realloc( c->history_b, (size_t)w * h * 2 * sizeof( wp_f32 ) );
    c->blur_a = realloc( c->blur_a, (size_t)w * h * 2 * sizeof( wp_f32 ) );
    c->blur_b = realloc( c->blur_b, (size_t)w * h * 2 * sizeof( wp_f32 ) );
    c->texel_x = 1.0f / w;
    c->texel_y = 1.0f / h;
    c->reset_history = 1;
    c->history_current = c->history_a;
    c->history_prev = c->history_b;
}
wp_f32 wp_gtao_sample( const wp_gtao *c, wp_f32 u, wp_f32 v )
{
    if( !c )
        return 1.0f;
    wp_s32 x = (wp_s32)( u * ( c->width - 1 ) ), y = (wp_s32)( v * ( c->height - 1 ) );
    return c->output[( y * c->width + x ) * 2];
}
void wp_gtao_reset_history( wp_gtao *c )
{
    if( c )
        c->reset_history = 1;
}
