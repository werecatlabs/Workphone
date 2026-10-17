/**
 * @file workphone_graphics_scene.h
 * @brief C API for a graphics scene that manages scene nodes, graphics objects
 *        and global rendering state (lighting, fog, skybox, shadows).
 *
 * Mirrors the C++ IGraphicsScene interface.  A graphics scene owns scene
 * nodes and graphics objects, provides ambient / hemisphere lighting, fog
 * and skybox configuration, and shadow enable/disable.
 */

#ifndef WORKPHONE_GRAPHICS_SCENE_H
#define WORKPHONE_GRAPHICS_SCENE_H

#include <stdint.h>
#include "workphone_graphics_node.h"
#include "workphone_matrix.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_scenenode wp_scenenode;
typedef struct wp_graphics_object wp_graphics_object;
typedef struct wp_camera wp_camera;
typedef struct wp_renderer wp_renderer;
typedef struct wp_graphics_cull_job wp_graphics_cull_job;
typedef struct wp_graphics_cull_result wp_graphics_cull_result;
typedef struct wp_threadpool wp_threadpool;

/* -------------------------------------------------------------------------
 * Fog modes
 * ---------------------------------------------------------------------- */

/**
 * @brief Fog calculation methods.
 */
typedef enum wp_fog_mode
{
    WP_FOG_NONE = 0,
    WP_FOG_EXP = 1,
    WP_FOG_EXP2 = 2,
    WP_FOG_LINEAR = 3
} wp_fog_mode;

/* -------------------------------------------------------------------------
 * Colour helper
 * ---------------------------------------------------------------------- */

/**
 * @brief Simple RGBA floating-powp_s32 colour.
 */
typedef struct wp_colour4f
{
    wp_f32 r;
    wp_f32 g;
    wp_f32 b;
    wp_f32 a;
} wp_colour4f;

/* -------------------------------------------------------------------------
 * Capacity constants
 * ---------------------------------------------------------------------- */

#ifndef WP_SCENE_INITIAL_NODE_CAPACITY
#    define WP_SCENE_INITIAL_NODE_CAPACITY 16
#endif

#ifndef WP_SCENE_INITIAL_OBJECT_CAPACITY
#    define WP_SCENE_INITIAL_OBJECT_CAPACITY 16
#endif

/* -------------------------------------------------------------------------
 * Graphics scene structure
 * ---------------------------------------------------------------------- */

/**
 * @brief A graphics scene manages scene nodes, graphics objects and global
 *        rendering settings.
 */
typedef struct wp_graphics_scene
{
    /* -- Scene graph nodes ------------------------------------------------ */
    wp_scenenode *root_node;
    wp_scenenode **nodes;
    wp_s32 node_count;
    wp_s32 node_capacity;

    /* -- Graphics objects ------------------------------------------------- */
    wp_graphics_object **objects;
    wp_s32 object_count;
    wp_s32 object_capacity;

    /* -- Cached render queue --------------------------------------------- */
    wp_graphics_object **render_list;
    wp_graphics_object **render_scratch;
    wp_s32 render_capacity;

    /* -- Visibility ------------------------------------------------------- */
    wp_u32 visibility_mask;

    /* -- Ambient light ---------------------------------------------------- */
    wp_f32 ambient_r;
    wp_f32 ambient_g;
    wp_f32 ambient_b;

    /* -- Hemisphere lighting ---------------------------------------------- */
    wp_colour4f upper_hemisphere;
    wp_colour4f lower_hemisphere;
    wp_vec3f hemisphere_dir;
    wp_f32 envmap_scale;

    /* -- Fog -------------------------------------------------------------- */
    wp_fog_mode fog_mode;
    wp_colour4f fog_colour;
    wp_f32 fog_density;
    wp_f32 fog_start;
    wp_f32 fog_end;

    /* -- Skybox ----------------------------------------------------------- */
    wp_s32 skybox_enabled;
    wp_f32 skybox_distance;

    /* -- Shadows ---------------------------------------------------------- */
    wp_s32 shadows_enabled;
    wp_s32 depth_shadows;

    /* -- Cameras ---------------------------------------------------------- */
    wp_camera *active_camera;
    wp_camera *default_camera;

    /* -- Native ----------------------------------------------------------- */
    void *native;
    wp_u64 generation;
    wp_graphics_cull_job *cull_job;
    wp_threadpool *transform_pool; /* borrowed; synchronous preparation drains it */
    wp_u32 cull_flags;
} wp_graphics_scene;

/* Scene membership/view revision. Mutation/capture/submission are owner-thread
 * operations. Workers never dereference the scene after capture. */
wp_u64 wp_graphics_scene_get_generation( const wp_graphics_scene * );
void wp_graphics_scene_set_transform_pool( wp_graphics_scene *, wp_threadpool * );

enum
{
    WP_GRAPHICS_CULL_SPHERE_TEST = 1u,
    WP_GRAPHICS_CULL_SAT_TEST = 2u,
    WP_GRAPHICS_CULL_GROUP_TEST = 4u,
    WP_GRAPHICS_CULL_SPATIAL_TEST = 8u
};

/* Owner-only scene A/B policy. Sphere/SAT enabled by default; blocks/spatial bins opt-in.
 * Six-plane box tests
 * remain active with zero flags. Unknown bits reject without mutation. Changes
 * invalidate prepared views; equal assignments preserve them. Pool replacement
 * and scene clear preserve the policy. Standalone jobs retain their own switches. */
wp_s32 wp_graphics_scene_set_cull_flags( wp_graphics_scene *, wp_u32 flags );
wp_u32 wp_graphics_scene_get_cull_flags( const wp_graphics_scene * );
/* Owned independent per-view result; pass is 0=colour or 1=conservative shadows.
 * A NULL result means preparation failed and the caller may use the synchronous
 * render_with_submit reference before issuing any draws. */
wp_graphics_cull_result *wp_graphics_scene_prepare_cull( wp_graphics_scene *, const wp_camera *,
                                                         wp_s32 pass );
/* Optional borrowed monotonic nanosecond clock, used only during this call.
 * NULL selects ordinary unprofiled preparation. No scene policy/generation change. */
typedef wp_u64 ( *wp_graphics_cull_clock )( void *context );
wp_graphics_cull_result *wp_graphics_scene_prepare_cull_profiled(
    wp_graphics_scene *, const wp_camera *, wp_s32 pass, wp_graphics_cull_clock, void *context );
typedef wp_s32 ( *wp_graphics_scene_snapshot_submit_func )( wp_graphics_object *, wp_renderer *,
                                                            const wp_mat4f *, void * );
/* Validates before and during consumption. Callbacks may not destroy the scene.
 * Uses frozen camera/world matrices, never changes shared object visibility. */
wp_s32 wp_graphics_scene_render_cull_result( wp_graphics_scene *, wp_renderer *,
                                             const wp_graphics_cull_result *,
                                             wp_graphics_scene_snapshot_submit_func, void * );

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_graphics_scene *wp_graphics_scene_create( void );
void wp_graphics_scene_destroy( wp_graphics_scene *scene );
void wp_graphics_scene_clear( wp_graphics_scene *scene );

/* =========================================================================
 * Scene node management
 * ====================================================================== */

wp_scenenode *wp_graphics_scene_create_node( wp_graphics_scene *scene );
void wp_graphics_scene_destroy_node( wp_graphics_scene *scene, wp_scenenode *node );
wp_scenenode *wp_graphics_scene_get_root_node( const wp_graphics_scene *scene );
wp_s32 wp_graphics_scene_get_node_count( const wp_graphics_scene *scene );

/* =========================================================================
 * Graphics object management
 * ====================================================================== */

wp_graphics_object *wp_graphics_scene_create_object( wp_graphics_scene *scene );
void wp_graphics_scene_destroy_object( wp_graphics_scene *scene, wp_graphics_object *obj );
wp_s32 wp_graphics_scene_get_object_count( const wp_graphics_scene *scene );

/* =========================================================================
 * Visibility mask
 * ====================================================================== */

void wp_graphics_scene_set_visibility_mask( wp_graphics_scene *scene, wp_u32 mask );
wp_u32 wp_graphics_scene_get_visibility_mask( const wp_graphics_scene *scene );

/* =========================================================================
 * Ambient light
 * ====================================================================== */

void wp_graphics_scene_set_ambient_light( wp_graphics_scene *scene, wp_f32 r, wp_f32 g, wp_f32 b );
void wp_graphics_scene_get_ambient_light( const wp_graphics_scene *scene, wp_f32 *r, wp_f32 *g,
                                          wp_f32 *b );

/* =========================================================================
 * Hemisphere lighting
 * ====================================================================== */

void wp_graphics_scene_set_upper_hemisphere( wp_graphics_scene *scene, wp_colour4f colour );
wp_colour4f wp_graphics_scene_get_upper_hemisphere( const wp_graphics_scene *scene );

void wp_graphics_scene_set_lower_hemisphere( wp_graphics_scene *scene, wp_colour4f colour );
wp_colour4f wp_graphics_scene_get_lower_hemisphere( const wp_graphics_scene *scene );

void wp_graphics_scene_set_hemisphere_dir( wp_graphics_scene *scene, wp_vec3f dir );
wp_vec3f wp_graphics_scene_get_hemisphere_dir( const wp_graphics_scene *scene );

void wp_graphics_scene_set_envmap_scale( wp_graphics_scene *scene, wp_f32 scale );
wp_f32 wp_graphics_scene_get_envmap_scale( const wp_graphics_scene *scene );

/* =========================================================================
 * Fog
 * ====================================================================== */

void wp_graphics_scene_set_fog( wp_graphics_scene *scene, wp_fog_mode mode, wp_colour4f colour,
                                wp_f32 density, wp_f32 start, wp_f32 end );

wp_fog_mode wp_graphics_scene_get_fog_mode( const wp_graphics_scene *scene );
wp_colour4f wp_graphics_scene_get_fog_colour( const wp_graphics_scene *scene );
wp_f32 wp_graphics_scene_get_fog_density( const wp_graphics_scene *scene );
wp_f32 wp_graphics_scene_get_fog_start( const wp_graphics_scene *scene );
wp_f32 wp_graphics_scene_get_fog_end( const wp_graphics_scene *scene );

/* =========================================================================
 * Skybox
 * ====================================================================== */

void wp_graphics_scene_set_skybox( wp_graphics_scene *scene, wp_s32 enable, wp_f32 distance );
wp_s32 wp_graphics_scene_get_skybox_enabled( const wp_graphics_scene *scene );
wp_f32 wp_graphics_scene_get_skybox_distance( const wp_graphics_scene *scene );

/* =========================================================================
 * Shadows
 * ====================================================================== */

void wp_graphics_scene_set_shadows( wp_graphics_scene *scene, wp_s32 enable, wp_s32 depth_shadows );
wp_s32 wp_graphics_scene_get_shadows_enabled( const wp_graphics_scene *scene );
wp_s32 wp_graphics_scene_get_depth_shadows( const wp_graphics_scene *scene );

/* =========================================================================
 * Camera
 * ====================================================================== */

wp_camera *wp_graphics_scene_get_active_camera( const wp_graphics_scene *scene );
void wp_graphics_scene_set_active_camera( wp_graphics_scene *scene, wp_camera *camera );

wp_camera *wp_graphics_scene_get_default_camera( const wp_graphics_scene *scene );
void wp_graphics_scene_set_default_camera( wp_graphics_scene *scene, wp_camera *camera );

/**
 * @brief Submits the scene's visible objects to a renderer.
 *
 * Objects are filtered by the scene and active/default camera visibility masks,
 * then submitted in ascending render-queue and Z-order. Equal keys retain scene
 * creation order. Meshes with usable local bounds are conservatively culled
 * against the camera; callback-only objects and unknown bounds are retained.
 * Object callbacks must not add or remove scene objects while
 * this function is running.
 *
 * @param scene Scene to render.
 * @param renderer Renderer passed to each object's C render callback.
 */
void wp_graphics_scene_render( wp_graphics_scene *scene, wp_renderer *renderer );

/** @brief Optional submission hook for visible objects in sorted render order.
 * Return nonzero when handled, or zero to use the object's normal render callback.
 */
typedef wp_s32 ( *wp_graphics_scene_submit_func )( wp_graphics_object *object, wp_renderer *renderer,
                                                   void *user_data );
void wp_graphics_scene_render_with_submit( wp_graphics_scene *scene, wp_renderer *renderer,
                                           wp_graphics_scene_submit_func submit, void *user_data );

/* =========================================================================
 * Native access
 * ====================================================================== */

void wp_graphics_scene_get_native( const wp_graphics_scene *scene, void **pp_object );
void wp_graphics_scene_set_native( wp_graphics_scene *scene, void *native );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_SCENE_H */
