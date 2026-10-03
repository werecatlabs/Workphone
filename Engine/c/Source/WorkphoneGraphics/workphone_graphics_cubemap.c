/**
 * @file workphone_graphics_cubemap.c
 * @brief Implementation of the C cubemap API.
 */

#include "workphone_graphics_cubemap.h"
#include "workphone_graphics_material.h"
#include <stdlib.h>
#include <string.h>

#ifndef WP_CUBEMAP_MAX_TEXTURES
#    define WP_CUBEMAP_MAX_TEXTURES 6
#endif

struct wp_cubemap
{
    wp_graphics_material *material;
    wp_c8 texture_names[WP_CUBEMAP_MAX_TEXTURES][WP_MATERIAL_MAX_NAME];
    wp_s32 enabled;
    void *native;
};

wp_cubemap *wp_cubemap_create( void )
{
    wp_cubemap *cubemap = (wp_cubemap *)malloc( sizeof( wp_cubemap ) );
    if( !cubemap )
        return NULL;
    memset( cubemap, 0, sizeof( wp_cubemap ) );
    cubemap->material = wp_graphics_material_create();
    wp_graphics_material_set_type( cubemap->material, WORKPHONE_MATERIAL_TYPE_SKYBOX );
    cubemap->enabled = 1;
    return cubemap;
}

void wp_cubemap_destroy( wp_cubemap *cubemap )
{
    if( !cubemap )
        return;
    wp_graphics_material_destroy( cubemap->material );
    free( cubemap );
}

void wp_cubemap_set_enabled( wp_cubemap *cubemap, wp_s32 enabled )
{
    if( !cubemap )
        return;
    cubemap->enabled = enabled ? 1 : 0;
}

wp_s32 wp_cubemap_is_enabled( const wp_cubemap *cubemap )
{
    return cubemap ? cubemap->enabled : 0;
}

void wp_cubemap_set_texture_name( wp_cubemap *cubemap, wp_s32 face, const wp_c8 *name )
{
    if( !cubemap || face < 0 || face >= WP_CUBEMAP_MAX_TEXTURES )
        return;
    if( name )
    {
        strncpy( cubemap->texture_names[face], name, WP_MATERIAL_MAX_NAME - 1 );
        cubemap->texture_names[face][WP_MATERIAL_MAX_NAME - 1] = '\0';
    }
    else
    {
        cubemap->texture_names[face][0] = '\0';
    }
    wp_graphics_material_set_texture_name( cubemap->material, face, name );
}

const wp_c8 *wp_cubemap_get_texture_name( const wp_cubemap *cubemap, wp_s32 face )
{
    if( !cubemap || face < 0 || face >= WP_CUBEMAP_MAX_TEXTURES )
        return "";
    return cubemap->texture_names[face];
}

wp_graphics_material *wp_cubemap_get_material( const wp_cubemap *cubemap )
{
    return cubemap ? cubemap->material : NULL;
}

void *wp_cubemap_get_native( const wp_cubemap *cubemap )
{
    return cubemap ? cubemap->native : NULL;
}

void wp_cubemap_set_native( wp_cubemap *cubemap, void *native )
{
    if( cubemap )
        cubemap->native = native;
}
