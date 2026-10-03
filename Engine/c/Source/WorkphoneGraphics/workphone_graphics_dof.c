/**
 * @file wp_graphics_dof.c
 * @brief Depth of Field implementation
 * Reference: Claude-of-Duty/src/render/dof.js
 */

#include "workphone_graphics_dof.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct wp_dof
{
    wp_s32 width;
    wp_s32 height;
    wp_s32 half_w;
    wp_s32 half_h;
    wp_f32 *pre_buffer;
    wp_f32 *gather_buffer;
    wp_f32 *output_buffer;
    wp_f32 max_coc;
    wp_f32 near_ratio;
};

static wp_f32 wp_cl( wp_f32 x, wp_f32 a, wp_f32 b )
{
    return x < a ? a : x > b ? b : x;
}
static wp_f32 wp_ign( wp_f32 x, wp_f32 y )
{
    return (wp_f32)fmod( 52.9829189f * (wp_f32)fmod( x * 0.06711056f + y * 0.00583715f, 1.0f ), 1.0f );
}

static wp_f32 wp_coc( wp_f32 d, wp_f32 focus, wp_f32 far_start, wp_f32 far_range )
{
    wp_f32 fc = wp_cl( d, focus, 1e4f );
    wp_f32 far = wp_cl( ( fc - focus * far_start ) / far_range, 0, 1 );
    return far * wp_cl( 1.0f - wp_cl( fc / ( focus * 0.55f ), 0, 1 ) *
                                   wp_cl( (wp_f32)fabs( d - focus ) / ( focus * 0.4f ), 0, 1 ) *
                                   ( 1 - wp_cl( fc / ( focus * 0.4f ), 0.6f, 1 ) ),
                        0, 1 );
}

static void wp_dof_prefilter( wp_dof *c, const wp_f32 *color, const wp_f32 *depth, wp_f32 focus )
{
    for( wp_s32 y = 0; y < c->half_h; y++ )
    {
        for( wp_s32 x = 0; x < c->half_w; x++ )
        {
            wp_f32 u = ( (wp_f32)x + 0.5f ) / c->half_w;
            wp_f32 v = ( (wp_f32)y + 0.5f ) / c->half_h;
            wp_f32 tx = 0.5f / c->width, ty = 0.5f / c->height;

            wp_f32 sum_r = 0, sum_g = 0, sum_b = 0, wsum = 0, max_c = 0;
            for( wp_s32 dy = -1; dy <= 1; dy++ )
            {
                for( wp_s32 dx = -1; dx <= 1; dx++ )
                {
                    wp_f32 su = wp_cl( u + dx * tx, 0, 1 );
                    wp_f32 sv = wp_cl( v + dy * ty, 0, 1 );
                    wp_s32 sx = (wp_s32)( su * ( c->width - 1 ) );
                    wp_s32 sy = (wp_s32)( sv * ( c->height - 1 ) );
                    wp_s32 si = sy * c->width + sx;
                    wp_f32 d = depth ? depth[si] : 1e4f;
                    wp_f32 coc = wp_cl( wp_coc( d, focus, 1.2f, 20.0f ) * c->max_coc, 0, c->max_coc );
                    wp_s32 ci = si * 3;
                    wp_f32 r = wp_cl( color[ci], 0, 24.0f );
                    wp_f32 g = wp_cl( color[ci + 1], 0, 24.0f );
                    wp_f32 b = wp_cl( color[ci + 2], 0, 24.0f );
                    wp_f32 w = coc + 0.05f;
                    sum_r += r * w;
                    sum_g += g * w;
                    sum_b += b * w;
                    wsum += w;
                    if( coc > max_c )
                        max_c = coc;
                }
            }
            wp_s32 oi = ( y * c->half_w + x ) * 4;
            if( wsum > 0 )
            {
                c->pre_buffer[oi] = sum_r / wsum;
                c->pre_buffer[oi + 1] = sum_g / wsum;
                c->pre_buffer[oi + 2] = sum_b / wsum;
            }
            c->pre_buffer[oi + 3] = max_c;
        }
    }
}

static void wp_dof_gather( wp_dof *c, wp_s32 frame )
{
    for( wp_s32 y = 0; y < c->half_h; y++ )
    {
        for( wp_s32 x = 0; x < c->half_w; x++ )
        {
            wp_s32 i = ( y * c->half_w + x ) * 4;
            wp_f32 cr = c->pre_buffer[i], cg = c->pre_buffer[i + 1], cb = c->pre_buffer[i + 2],
                   ca = c->pre_buffer[i + 3];
            wp_f32 max_coc = ca * 0.5f;
            wp_f32 radius = wp_cl( c->max_coc * 0.5f, 1, 32 );

            wp_f32 sum_r = cr, sum_g = cg, sum_b = cb;
            wp_f32 wsum = 1.0f;
            wp_f32 rot = wp_ign( (wp_f32)x + 5.371f * (wp_f32)( frame % 64 ), (wp_f32)y ) *
                         3.141592653589793f * 2;

            for( wp_s32 t = 0; t < WP_DOF_TAPS; t++ )
            {
                wp_f32 ang = (wp_f32)t * 2.39996323f + rot;
                wp_f32 r = (wp_f32)( t + 1 ) / (wp_f32)WP_DOF_TAPS;
                wp_f32 dist = (wp_f32)sqrt( r ) * radius;
                wp_f32 ox = (wp_f32)cos( ang ) * dist / c->half_w;
                wp_f32 oy = (wp_f32)sin( ang ) * dist / c->half_h;
                wp_f32 gu = wp_cl( ( (wp_f32)x + 0.5f ) / c->half_w + ox, 0, 1 );
                wp_f32 gv = wp_cl( ( (wp_f32)y + 0.5f ) / c->half_h + oy, 0, 1 );
                wp_s32 gx = (wp_s32)( gu * ( c->half_w - 1 ) );
                wp_s32 gy = (wp_s32)( gv * ( c->half_h - 1 ) );
                wp_s32 gi = ( gy * c->half_w + gx ) * 4;
                wp_f32 gc = c->pre_buffer[gi + 3];
                wp_f32 w = wp_cl( gc * 0.5f - dist + 1.0f, 0, 1 );
                sum_r += c->pre_buffer[gi] * w;
                sum_g += c->pre_buffer[gi + 1] * w;
                sum_b += c->pre_buffer[gi + 2] * w;
                wsum += w;
                if( gc > max_coc )
                    max_coc = gc;
            }
            c->gather_buffer[i] = sum_r / wsum;
            c->gather_buffer[i + 1] = sum_g / wsum;
            c->gather_buffer[i + 2] = sum_b / wsum;
            c->gather_buffer[i + 3] = max_coc;
        }
    }
}

static void wp_dof_combine( wp_dof *c, const wp_f32 *color, const wp_f32 *depth, wp_f32 focus )
{
    for( wp_s32 y = 0; y < c->height; y++ )
    {
        for( wp_s32 x = 0; x < c->width; x++ )
        {
            wp_s32 i = ( y * c->width + x ) * 3;
            wp_f32 cr = wp_cl( color[i], 0, 24.0f );
            wp_f32 cg = wp_cl( color[i + 1], 0, 24.0f );
            wp_f32 cb = wp_cl( color[i + 2], 0, 24.0f );
            wp_f32 d = depth ? depth[y * c->width + x] : 1e4f;

            /* Bilinear sample gather buffer */
            wp_f32 gu = ( (wp_f32)x / 2 + 0.5f ) / c->half_w;
            wp_f32 gv = ( (wp_f32)y / 2 + 0.5f ) / c->half_h;
            wp_f32 gfrac_x = gu * ( c->half_w - 1 ) - (wp_s32)( gu * ( c->half_w - 1 ) );
            wp_f32 gfrac_y = gv * ( c->half_h - 1 ) - (wp_s32)( gv * ( c->half_h - 1 ) );
            wp_s32 gx0 = (wp_s32)( gu * ( c->half_w - 1 ) ), gy0 = (wp_s32)( gv * ( c->half_h - 1 ) );
            wp_s32 gx1 = gx0 + 1 < c->half_w ? gx0 + 1 : c->half_w - 1;
            wp_s32 gy1 = gy0 + 1 < c->half_h ? gy0 + 1 : c->half_h - 1;
            wp_s32 gi00 = ( gy0 * c->half_w + gx0 ) * 4, gi10 = ( gy0 * c->half_w + gx1 ) * 4;
            wp_s32 gi01 = ( gy1 * c->half_w + gx0 ) * 4, gi11 = ( gy1 * c->half_w + gx1 ) * 4;
            wp_f32 gr = c->gather_buffer[gi00] * ( 1 - gfrac_x ) * ( 1 - gfrac_y ) +
                        c->gather_buffer[gi10] * gfrac_x * ( 1 - gfrac_y ) +
                        c->gather_buffer[gi01] * ( 1 - gfrac_x ) * gfrac_y +
                        c->gather_buffer[gi11] * gfrac_x * gfrac_y;
            wp_f32 gg = c->gather_buffer[gi00 + 1] * ( 1 - gfrac_x ) * ( 1 - gfrac_y ) +
                        c->gather_buffer[gi10 + 1] * gfrac_x * ( 1 - gfrac_y ) +
                        c->gather_buffer[gi01 + 1] * ( 1 - gfrac_x ) * gfrac_y +
                        c->gather_buffer[gi11 + 1] * gfrac_x * gfrac_y;
            wp_f32 gb = c->gather_buffer[gi00 + 2] * ( 1 - gfrac_x ) * ( 1 - gfrac_y ) +
                        c->gather_buffer[gi10 + 2] * gfrac_x * ( 1 - gfrac_y ) +
                        c->gather_buffer[gi01 + 2] * ( 1 - gfrac_x ) * gfrac_y +
                        c->gather_buffer[gi11 + 2] * gfrac_x * gfrac_y;
            wp_f32 gc = c->gather_buffer[gi00 + 3] * ( 1 - gfrac_x ) * ( 1 - gfrac_y ) +
                        c->gather_buffer[gi10 + 3] * gfrac_x * ( 1 - gfrac_y ) +
                        c->gather_buffer[gi01 + 3] * ( 1 - gfrac_x ) * gfrac_y +
                        c->gather_buffer[gi11 + 3] * gfrac_x * gfrac_y;

            wp_f32 coc = wp_cl( wp_coc( d, focus, 1.2f, 20.0f ) * c->max_coc, 0, c->max_coc );
            wp_f32 m =
                wp_cl( wp_cl( coc * 0.85f, gc * 0.85f, c->max_coc ) / ( c->max_coc * 0.85f ), 0, 1 );

            c->output_buffer[i] = cr * ( 1 - m ) + gr * m;
            c->output_buffer[i + 1] = cg * ( 1 - m ) + gg * m;
            c->output_buffer[i + 2] = cb * ( 1 - m ) + gb * m;
        }
    }
}

wp_dof *wp_dof_create( wp_s32 w, wp_s32 h )
{
    wp_dof *c = calloc( 1, sizeof( wp_dof ) );
    if( !c )
        return NULL;
    c->width = w;
    c->height = h;
    c->half_w = ( w + 1 ) / 2;
    c->half_h = ( h + 1 ) / 2;
    wp_s32 hp = c->half_w * c->half_h;
    c->pre_buffer = calloc( (size_t)hp * 4, sizeof( wp_f32 ) );
    c->gather_buffer = calloc( (size_t)hp * 4, sizeof( wp_f32 ) );
    c->output_buffer = calloc( (size_t)w * h * 3, sizeof( wp_f32 ) );
    c->max_coc = WP_DOF_DEFAULT_COC;
    c->near_ratio = WP_DOF_DEFAULT_NEAR_RATIO;
    return c;
}

void wp_dof_destroy( wp_dof *c )
{
    if( !c )
        return;
    free( c->pre_buffer );
    free( c->gather_buffer );
    free( c->output_buffer );
    free( c );
}

void wp_dof_render( wp_dof *c, const wp_f32 *color, const wp_f32 *depth, wp_s32 frame,
                    wp_f32 focus_dist )
{
    if( !c )
        return;
    wp_dof_prefilter( c, color, depth, focus_dist );
    wp_dof_gather( c, frame );
    wp_dof_combine( c, color, depth, focus_dist );
}

const wp_f32 *wp_dof_get_texture( const wp_dof *c )
{
    return c ? c->output_buffer : NULL;
}

void wp_dof_resize( wp_dof *c, wp_s32 w, wp_s32 h )
{
    if( !c )
        return;
    c->width = w;
    c->height = h;
    c->half_w = ( w + 1 ) / 2;
    c->half_h = ( h + 1 ) / 2;
    wp_s32 hp = c->half_w * c->half_h;
    c->pre_buffer = realloc( c->pre_buffer, (size_t)hp * 4 * sizeof( wp_f32 ) );
    c->gather_buffer = realloc( c->gather_buffer, (size_t)hp * 4 * sizeof( wp_f32 ) );
    c->output_buffer = realloc( c->output_buffer, (size_t)w * h * 3 * sizeof( wp_f32 ) );
}
