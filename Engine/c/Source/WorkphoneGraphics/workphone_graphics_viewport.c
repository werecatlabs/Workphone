#include "workphone_graphics_viewport.h"
#include <stdlib.h>
#include <string.h>

wp_graphics_viewport *wp_graphics_viewport_create( void *camera, void *target, wp_f32 left, wp_f32 top,
                                                   wp_f32 width, wp_f32 height, wp_s32 z_order )
{
    wp_graphics_viewport *viewport = (wp_graphics_viewport *)malloc( sizeof( wp_graphics_viewport ) );
    if( !viewport )
    {
        return NULL;
    }

    viewport->camera = camera;
    viewport->target = target;
    viewport->rel_rect.left = left;
    viewport->rel_rect.top = top;
    viewport->rel_rect.right = left + width;
    viewport->rel_rect.bottom = top + height;
    viewport->z_order = z_order;

    viewport->back_colour.r = 0.0f;
    viewport->back_colour.g = 0.0f;
    viewport->back_colour.b = 0.0f;
    viewport->back_colour.a = 1.0f;

    viewport->depth_clear_value = 1.0f;
    viewport->clear_every_frame = 1;
    viewport->clear_buffers = 0; /* FBT_COLOUR | FBT_DEPTH equivalent */
    viewport->updated = 0;
    viewport->show_overlays = 1;
    viewport->show_skies = 1;
    viewport->show_shadows = 1;
    viewport->visibility_mask = 0xFFFFFFFF;
    viewport->material_scheme_name = NULL;
    viewport->is_auto_updated = 1;
    viewport->colour_buffer = 0;

    wp_graphics_viewport_update_dimensions( viewport );

    return viewport;
}

void wp_graphics_viewport_destroy( wp_graphics_viewport *viewport )
{
    if( !viewport )
    {
        return;
    }

    if( viewport->material_scheme_name )
    {
        free( viewport->material_scheme_name );
    }

    free( viewport );
}

void wp_graphics_viewport_update_dimensions( wp_graphics_viewport *viewport )
{
    /* In a real implementation, target dimensions would be retrieved from the target object.
       Here we simulate the logic from OgreViewport.cpp */
    wp_f32 target_width = 800.0f;  /* Placeholder: should come from viewport->target */
    wp_f32 target_height = 600.0f; /* Placeholder: should come from viewport->target */

    viewport->act_rect.x = (wp_s32)( viewport->rel_rect.left * target_width );
    viewport->act_rect.y = (wp_s32)( viewport->rel_rect.top * target_height );
    viewport->act_rect.width =
        (wp_s32)( ( viewport->rel_rect.right - viewport->rel_rect.left ) * target_width );
    viewport->act_rect.height =
        (wp_s32)( ( viewport->rel_rect.bottom - viewport->rel_rect.top ) * target_height );

    viewport->updated = 1;
}

void wp_graphics_viewport_update( wp_graphics_viewport *viewport )
{
    if( !viewport || !viewport->camera )
    {
        return;
    }
    /* Mirror OgreViewport::update: notify camera and render scene */
    /* Camera*_notifyViewport(viewport->camera, viewport); */
    /* Camera*_renderScene(viewport->camera, viewport); */
}

void wp_graphics_viewport_clear( wp_graphics_viewport *viewport, wp_u32 buffers, wp_colour_value col,
                                 wp_f32 depth, wp_u16 stencil )
{
    if( !viewport )
    {
        return;
    }
    /* Mirror OgreViewport::clear: uses RenderSystem to clear the frame buffer */
    /* RenderSystem* rs = Root::getSingleton().getRenderSystem(); ... */
}

void wp_graphics_viewport_set_camera( wp_graphics_viewport *viewport, void *camera )
{
    if( !viewport )
    {
        return;
    }
    viewport->camera = camera;
}

void wp_graphics_viewport_set_dimensions( wp_graphics_viewport *viewport, wp_f32 left, wp_f32 top,
                                          wp_f32 width, wp_f32 height )
{
    if( !viewport )
    {
        return;
    }
    viewport->rel_rect.left = left;
    viewport->rel_rect.top = top;
    viewport->rel_rect.right = left + width;
    viewport->rel_rect.bottom = top + height;
    wp_graphics_viewport_update_dimensions( viewport );
}
