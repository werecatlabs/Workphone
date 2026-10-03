/**
 * @file workphone_graphics_skybox.c
 * @brief Implementation of the C skybox API.
 */

#include "workphone_graphics_skybox.h"
#include "workphone_graphics_material.h"
#include <stdlib.h>
#include <string.h>

#ifndef WP_SKYBOX_MAX_TEXTURES
#    define WP_SKYBOX_MAX_TEXTURES 6
#endif

struct wp_skybox
{
    wp_graphics_material *material;
    wp_c8 texture_names[WP_SKYBOX_MAX_TEXTURES][WP_MATERIAL_MAX_NAME];
    wp_s32 enabled;
    wp_f32 distance;
    void *native;
};

wp_skybox *wp_skybox_create( void )
{
    wp_skybox *skybox = (wp_skybox *)malloc( sizeof( wp_skybox ) );
    if( !skybox )
        return NULL;
    memset( skybox, 0, sizeof( wp_skybox ) );
    skybox->material = wp_graphics_material_create();
    wp_graphics_material_set_type( skybox->material, WORKPHONE_MATERIAL_TYPE_SKYBOX );
    skybox->enabled = 1;
    skybox->distance = 100.0f;
    return skybox;
}

void wp_skybox_destroy( wp_skybox *skybox )
{
    if( !skybox )
        return;

    wp_graphics_material_destroy( skybox->material );
    free( skybox );
}

void wp_skybox_set_enabled( wp_skybox *skybox, wp_s32 enabled )
{
    if( !skybox )
        return;

    skybox->enabled = enabled ? 1 : 0;
}

wp_s32 wp_skybox_is_enabled( const wp_skybox *skybox )
{
    return skybox ? skybox->enabled : 0;
}

void wp_skybox_set_distance( wp_skybox *skybox, float distance )
{
    if( !skybox )
        return;

    skybox->distance = distance;
}

float wp_skybox_get_distance( const wp_skybox *skybox )
{
    return skybox ? skybox->distance : 0.0f;
}

void wp_skybox_set_texture_name( wp_skybox *skybox, wp_s32 face, const wp_c8 *name )
{
    if( !skybox || face < 0 || face >= WP_SKYBOX_MAX_TEXTURES )
        return;

    if( name )
    {
        strncpy( skybox->texture_names[face], name, WP_MATERIAL_MAX_NAME - 1 );
        skybox->texture_names[face][WP_MATERIAL_MAX_NAME - 1] = '\0';
    }
    else
    {
        skybox->texture_names[face][0] = '\0';
    }

    wp_graphics_material_set_texture_name( skybox->material, face, name );
}

const wp_c8 *wp_skybox_get_texture_name( const wp_skybox *skybox, wp_s32 face )
{
    if( !skybox || face < 0 || face >= WP_SKYBOX_MAX_TEXTURES )
        return "";
    return skybox->texture_names[face];
}

wp_graphics_material *wp_skybox_get_material( const wp_skybox *skybox )
{
    return skybox ? skybox->material : NULL;
}

void *wp_skybox_get_native( const wp_skybox *skybox )
{
    return skybox ? skybox->native : NULL;
}

void wp_skybox_set_native( wp_skybox *skybox, void *native )
{
    if( skybox )
        skybox->native = native;
}
