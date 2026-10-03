/**
 * @file workphone_graphics_lightmap.c
 * @brief Implementation of the C lightmap API.
 */

#include "workphone_graphics_lightmap.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WP_LM_SEARCH_RADIUS 16

/* =========================================================================
 * Internal structures
 * ====================================================================== */

typedef struct wp_lightmap
{
    wp_s32 tex_size;
    wp_c8 name[WP_LIGHTMAP_MAX_NAME];
    wp_u8 *pixels;  // RGBA8, row-major, size tex_size*tex_size*4
    wp_u8 *mask;    // 1 if valid, 0 if not
    void *native;
} wp_lightmap;

typedef struct wp_lightmapper
{
    wp_s32 tex_size;
    wp_vec3f light_dir;
    wp_f32 ambient;
    wp_s32 auto_size;
    wp_lightmap **maps;
    wp_s32 map_count;
    wp_s32 map_cap;
    wp_s32 counter;
} wp_lightmapper;

/* =========================================================================
 * wp_lightmap implementation
 * ====================================================================== */

wp_lightmap *wp_lightmap_create( wp_s32 tex_size )
{
    if( tex_size <= 0 )
        return NULL;
    wp_lightmap *lm = (wp_lightmap *)malloc( sizeof( wp_lightmap ) );
    if( !lm )
        return NULL;
    memset( lm, 0, sizeof( wp_lightmap ) );
    lm->tex_size = tex_size;
    lm->pixels = (wp_u8 *)malloc( tex_size * tex_size * 4 );
    lm->mask = (wp_u8 *)malloc( tex_size * tex_size );
    if( !lm->pixels || !lm->mask )
    {
        free( lm->pixels );
        free( lm->mask );
        free( lm );
        return NULL;
    }
    memset( lm->pixels, 0, tex_size * tex_size * 4 );
    memset( lm->mask, 0, tex_size * tex_size );
    lm->name[0] = '\0';
    return lm;
}

void wp_lightmap_destroy( wp_lightmap *lm )
{
    if( !lm )
        return;
    free( lm->pixels );
    free( lm->mask );
    free( lm );
}

wp_s32 wp_lightmap_get_width( const wp_lightmap *lm )
{
    return lm ? lm->tex_size : 0;
}
wp_s32 wp_lightmap_get_height( const wp_lightmap *lm )
{
    return lm ? lm->tex_size : 0;
}

const wp_c8 *wp_lightmap_get_name( const wp_lightmap *lm )
{
    return lm ? lm->name : "";
}
void wp_lightmap_set_name( wp_lightmap *lm, const wp_c8 *name )
{
    if( !lm )
        return;
    if( name )
    {
        strncpy( lm->name, name, WP_LIGHTMAP_MAX_NAME - 1 );
        lm->name[WP_LIGHTMAP_MAX_NAME - 1] = '\0';
    }
    else
    {
        lm->name[0] = '\0';
    }
}

const wp_u8 *wp_lightmap_get_pixels( const wp_lightmap *lm )
{
    return lm ? lm->pixels : NULL;
}

wp_u8 wp_lightmap_get_intensity( const wp_lightmap *lm, wp_s32 x, wp_s32 y )
{
    if( !lm || x < 0 || y < 0 || x >= lm->tex_size || y >= lm->tex_size )
        return 0;
    wp_s32 idx = ( y * lm->tex_size + x ) * 4;
    return lm->pixels[idx];
}

static wp_vec3f barycentric( wp_vec2f t1, wp_vec2f t2, wp_vec2f t3, wp_vec2f p )
{
    wp_f32 x1 = t1.x, y1 = t1.y, x2 = t2.x, y2 = t2.y, x3 = t3.x, y3 = t3.y;
    wp_f32 x = p.x, y = p.y;
    wp_f32 denom = ( y2 - y3 ) * ( x1 - x3 ) + ( x3 - x2 ) * ( y1 - y3 );
    wp_f32 w1 = ( ( y2 - y3 ) * ( x - x3 ) + ( x3 - x2 ) * ( y - y3 ) ) / denom;
    wp_f32 w2 = ( ( y3 - y1 ) * ( x - x3 ) + ( x1 - x3 ) * ( y - y3 ) ) / denom;
    wp_f32 w3 = 1.0f - w1 - w2;
    wp_vec3f bc = { w1, w2, w3 };
    return bc;
}

void wp_lightmap_light_triangle( wp_lightmap *lm, wp_vec3f p1, wp_vec3f p2, wp_vec3f p3, wp_vec3f n1,
                                 wp_vec3f n2, wp_vec3f n3, wp_vec2f t1, wp_vec2f t2, wp_vec2f t3,
                                 wp_vec3f light_dir, wp_f32 ambient )
{
    if( !lm )
        return;
    wp_s32 size = lm->tex_size;
    for( wp_s32 y = 0; y < size; ++y )
    {
        for( wp_s32 x = 0; x < size; ++x )
        {
            wp_vec2f uv = { ( x + 0.5f ) / size, ( y + 0.5f ) / size };
            wp_vec3f bc = barycentric( t1, t2, t3, uv );
            if( bc.x < -0.001f || bc.y < -0.001f || bc.z < -0.001f )
                continue;
            // Interpolate position and normal
            wp_vec3f pos = { bc.x * p1.x + bc.y * p2.x + bc.z * p3.x,
                             bc.x * p1.y + bc.y * p2.y + bc.z * p3.y,
                             bc.x * p1.z + bc.y * p2.z + bc.z * p3.z };
            wp_vec3f norm = { bc.x * n1.x + bc.y * n2.x + bc.z * n3.x,
                              bc.x * n1.y + bc.y * n2.y + bc.z * n3.y,
                              bc.x * n1.z + bc.y * n2.z + bc.z * n3.z };
            // Normalize normal
            wp_f32 len = sqrtf( norm.x * norm.x + norm.y * norm.y + norm.z * norm.z );
            if( len > 1e-6f )
            {
                norm.x /= len;
                norm.y /= len;
                norm.z /= len;
            }
            // Compute intensity
            wp_f32 dot = norm.x * light_dir.x + norm.y * light_dir.y + norm.z * light_dir.z;
            wp_f32 intensity = ambient + fmaxf( 0.0f, dot );
            if( intensity > 1.0f )
                intensity = 1.0f;
            wp_u8 val = (wp_u8)( intensity * 255.0f );
            wp_s32 idx = ( y * size + x ) * 4;
            lm->pixels[idx + 0] = val;
            lm->pixels[idx + 1] = val;
            lm->pixels[idx + 2] = val;
            lm->pixels[idx + 3] = 255;
            lm->mask[y * size + x] = 1;
        }
    }
}

void wp_lightmap_fill_invalid_pixels( wp_lightmap *lm )
{
    if( !lm )
        return;
    wp_s32 size = lm->tex_size;
    for( wp_s32 y = 0; y < size; ++y )
    {
        for( wp_s32 x = 0; x < size; ++x )
        {
            if( lm->mask[y * size + x] )
                continue;
            // Spiral search for nearest valid pixel
            wp_s32 found = 0;
            for( wp_s32 r = 1; r <= WP_LM_SEARCH_RADIUS && !found; ++r )
            {
                for( wp_s32 dy = -r; dy <= r && !found; ++dy )
                {
                    for( wp_s32 dx = -r; dx <= r && !found; ++dx )
                    {
                        wp_s32 nx = x + dx, ny = y + dy;
                        if( nx < 0 || ny < 0 || nx >= size || ny >= size )
                            continue;
                        if( lm->mask[ny * size + nx] )
                        {
                            wp_s32 src = ( ny * size + nx ) * 4;
                            wp_s32 dst = ( y * size + x ) * 4;
                            lm->pixels[dst + 0] = lm->pixels[src + 0];
                            lm->pixels[dst + 1] = lm->pixels[src + 1];
                            lm->pixels[dst + 2] = lm->pixels[src + 2];
                            lm->pixels[dst + 3] = 255;
                            lm->mask[y * size + x] = 1;
                            found = 1;
                        }
                    }
                }
            }
        }
    }
}

void *wp_lightmap_get_native( const wp_lightmap *lm )
{
    return lm ? lm->native : NULL;
}
void wp_lightmap_set_native( wp_lightmap *lm, void *native )
{
    if( lm )
        lm->native = native;
}

/* =========================================================================
 * wp_lightmapper implementation
 * ====================================================================== */

wp_lightmapper *wp_lightmapper_create( void )
{
    wp_lightmapper *lmr = (wp_lightmapper *)malloc( sizeof( wp_lightmapper ) );
    if( !lmr )
        return NULL;
    memset( lmr, 0, sizeof( wp_lightmapper ) );
    lmr->tex_size = 256;
    lmr->ambient = 0.2f;
    lmr->light_dir = (wp_vec3f){ 0.0f, 1.0f, 0.0f };
    lmr->auto_size = 0;
    lmr->map_cap = 4;
    lmr->maps = (wp_lightmap **)malloc( sizeof( wp_lightmap * ) * lmr->map_cap );
    lmr->map_count = 0;
    lmr->counter = 0;
    return lmr;
}

void wp_lightmapper_destroy( wp_lightmapper *lmr )
{
    if( !lmr )
        return;
    for( wp_s32 i = 0; i < lmr->map_count; ++i )
        wp_lightmap_destroy( lmr->maps[i] );
    free( lmr->maps );
    free( lmr );
}

wp_s32 wp_lightmapper_get_tex_size( const wp_lightmapper *lmr )
{
    return lmr ? lmr->tex_size : 0;
}
void wp_lightmapper_set_tex_size( wp_lightmapper *lmr, wp_s32 tex_size )
{
    if( lmr )
        lmr->tex_size = tex_size;
}

wp_vec3f wp_lightmapper_get_light_dir( const wp_lightmapper *lmr )
{
    return lmr ? lmr->light_dir : (wp_vec3f){ 0, 0, 0 };
}
void wp_lightmapper_set_light_dir( wp_lightmapper *lmr, wp_vec3f dir )
{
    if( lmr )
        lmr->light_dir = dir;
}

wp_f32 wp_lightmapper_get_ambient( const wp_lightmapper *lmr )
{
    return lmr ? lmr->ambient : 0.0f;
}
void wp_lightmapper_set_ambient( wp_lightmapper *lmr, wp_f32 ambient )
{
    if( lmr )
        lmr->ambient = ambient;
}

wp_s32 wp_lightmapper_get_auto_size( const wp_lightmapper *lmr )
{
    return lmr ? lmr->auto_size : 0;
}
void wp_lightmapper_set_auto_size( wp_lightmapper *lmr, wp_s32 enabled )
{
    if( lmr )
        lmr->auto_size = enabled;
}

wp_lightmap *wp_lightmapper_create_lightmap( wp_lightmapper *lmr, const wp_c8 *name, wp_s32 tex_size )
{
    if( !lmr )
        return NULL;
    if( tex_size <= 0 )
        tex_size = lmr->tex_size;
    wp_lightmap *lm = wp_lightmap_create( tex_size );
    if( !lm )
        return NULL;
    if( name )
        wp_lightmap_set_name( lm, name );
    if( lmr->map_count >= lmr->map_cap )
    {
        wp_s32 new_cap = lmr->map_cap * 2;
        wp_lightmap **new_maps = (wp_lightmap **)realloc( lmr->maps, sizeof( wp_lightmap * ) * new_cap );
        if( !new_maps )
        {
            wp_lightmap_destroy( lm );
            return NULL;
        }
        lmr->maps = new_maps;
        lmr->map_cap = new_cap;
    }
    lmr->maps[lmr->map_count++] = lm;
    return lm;
}

wp_s32 wp_lightmapper_get_lightmap_count( const wp_lightmapper *lmr )
{
    return lmr ? lmr->map_count : 0;
}
wp_lightmap *wp_lightmapper_get_lightmap( const wp_lightmapper *lmr, wp_s32 index )
{
    if( !lmr || index < 0 || index >= lmr->map_count )
        return NULL;
    return lmr->maps[index];
}
void wp_lightmapper_remove_lightmap( wp_lightmapper *lmr, wp_lightmap *lm )
{
    if( !lmr || !lm )
        return;
    for( wp_s32 i = 0; i < lmr->map_count; ++i )
    {
        if( lmr->maps[i] == lm )
        {
            wp_lightmap_destroy( lm );
            for( wp_s32 j = i + 1; j < lmr->map_count; ++j )
                lmr->maps[j - 1] = lmr->maps[j];
            lmr->map_count--;
            break;
        }
    }
}
void wp_lightmapper_clear_lightmaps( wp_lightmapper *lmr )
{
    if( !lmr )
        return;
    for( wp_s32 i = 0; i < lmr->map_count; ++i )
        wp_lightmap_destroy( lmr->maps[i] );
    lmr->map_count = 0;
}
void wp_lightmapper_generate( wp_lightmapper *lmr )
{
    if( !lmr )
        return;
    for( wp_s32 i = 0; i < lmr->map_count; ++i )
        wp_lightmap_fill_invalid_pixels( lmr->maps[i] );
}
void wp_lightmapper_reset_counter( void )
{
    // This is a global, so we use a static variable
    static wp_s32 *counter = NULL;
    if( !counter )
    {
        static wp_s32 c = 0;
        counter = &c;
    }
    *counter = 0;
}
wp_s32 wp_lightmapper_next_counter( void )
{
    static wp_s32 counter = 0;
    return counter++;
}
