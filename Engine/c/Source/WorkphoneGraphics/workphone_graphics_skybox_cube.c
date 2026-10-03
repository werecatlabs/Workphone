/**
 * @file workphone_graphics_skybox_cube.c
 * @brief Implementation of the cube skybox rendering.
 */

#include "workphone_graphics_skybox_cube.h"
#include <stdlib.h>
#include <string.h>

/* --- Internal Structure --- */

struct wp_skybox_cube
{
    void *scene_mgr;             /**< Pointer to Ogre::SceneManager */
    void *camera;                /**< Pointer to Ogre::Camera */
    float size;                  /**< Skybox dimension */
    int has_renderable_material; /**< Material validity flag */
    void *sky_native;            /**< Pointer to Ogre::Rectangle2D */
};

/* --- Lifecycle Implementation --- */

wp_skybox_cube *wp_skybox_cube_create( void )
{
    wp_skybox_cube *skybox = (wp_skybox_cube *)malloc( sizeof( struct wp_skybox_cube ) );
    if( skybox )
    {
        memset( skybox, 0, sizeof( struct wp_skybox_cube ) );
        skybox->size = 5000.0f;
    }
    return skybox;
}

void wp_skybox_cube_destroy( wp_skybox_cube *skybox )
{
    if( skybox )
    {
        wp_skybox_cube_unload( skybox );
        free( skybox );
    }
}

void wp_skybox_cube_load( wp_skybox_cube *skybox, void *scene_mgr, void *camera, float size )
{
    if( !skybox )
        return;

    /* Preserve native geometry and material when refreshing the same binding. */
    if( skybox->scene_mgr == scene_mgr && skybox->camera == camera && skybox->size == size )
        return;

    /* In a real C implementation, we would call C-wrappers for Ogre functions.
       The following is a structural translation of CSkyboxCubeOgreNext::load. */

    wp_skybox_cube_unload( skybox );

    skybox->scene_mgr = scene_mgr;
    skybox->camera = camera;
    skybox->size = size;
    skybox->has_renderable_material = 0;

    if( !skybox->scene_mgr )
    {
        /* WP_LOG_ERROR equivalent */
        return;
    }

    /*
       MOCK CALLS to OgreNext C-API (representing the C++ logic):
       mSky = sceneMgr->createRectangle2D(SCENE_STATIC);
       mSky->initialize(...);
       mSky->setGeometry(...);
       mSky->setRenderQueueGroup(212);
       mSky->setVisible(false);
       sceneMgr->getRootSceneNode(SCENE_STATIC)->attachObject(mSky);
    */

    /* Since the actual C-API for OgreNext is not fully provided, we implement
       the state tracking. In the real project, these void* would be cast to
       internal types and native Ogre functions called. */

    wp_skybox_cube_update( skybox );
}

void wp_skybox_cube_load_data( wp_skybox_cube *skybox, void *data )
{
    /* Structural translation of load(SmartPtr<ISharedObject> data) */
    /* This typically involves extracting scene, camera, and distance from 'data'
       and then calling wp_skybox_cube_load. */
    if( !skybox || !data )
        return;

    /* Mock logic: retrieve parameters from data and call load */
    /* void *scene_mgr = ...; void *camera = ...; float distance = ...; */
    /* wp_skybox_cube_load(skybox, scene_mgr, camera, distance); */
}

void wp_skybox_cube_unload( wp_skybox_cube *skybox )
{
    if( !skybox )
        return;

    /*
       MOCK CALLS to OgreNext C-API:
       destroySky();
       destroySkyMaterial();
    */

    skybox->scene_mgr = NULL;
    skybox->camera = NULL;
    skybox->size = 0.0f;
    skybox->has_renderable_material = 0;
    skybox->sky_native = NULL;
}

void wp_skybox_cube_apply_material( wp_skybox_cube *skybox, wp_graphics_material *material )
{
    if( !skybox || !skybox->sky_native )
        return;

    if( !material )
    {
        skybox->has_renderable_material = 0;
        /* mSky->setVisible(false); */
        return;
    }

    /*
       Structural translation of CSkyboxCubeOgreNext::applyMaterial:
       1. Extract cube textures from material.
       2. Create Ogre cubemap.
       3. Bind to skybox native object.
    */

    skybox->has_renderable_material = 1;
    /* mSky->setVisible(isVisible() && has_renderable_material); */
    wp_skybox_cube_update( skybox );
}

void wp_skybox_cube_update( wp_skybox_cube *skybox )
{
    if( !skybox || !skybox->sky_native )
        return;

    /*
       Structural translation of CSkyboxCubeOgreNext::updateSkyFrustum:
       Updates the frustum of the skybox rectangle to keep it centered on the camera.
    */
}
