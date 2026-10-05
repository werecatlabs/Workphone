/**
 * @file wp_graphics_object.h
 * @brief C API for a renderable graphics object attachable to a scene node.
 *
 * A graphics object is the fundamental renderable unit in the scene graph. It
 * encapsulates visibility, shadowing, render ordering, bitmask-based flags,
 * a local axis-aligned bounding box (AABB), and attachment to a scene node.
 */

#ifndef WORKPHONE_GRAPHICS_OBJECT_H
#define WORKPHONE_GRAPHICS_OBJECT_H

#include <stdint.h>
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_graphics_object wp_graphics_object;
typedef struct wp_scenenode wp_scenenode;
typedef struct wp_graphics_scene wp_graphics_scene;
typedef struct wp_renderer wp_renderer;
typedef struct wp_graphics_mesh wp_graphics_mesh;
typedef struct wp_graphics_material wp_graphics_material;

/**
 * @brief Callback invoked by the graphics system to draw this object.
 * @param obj      The object being rendered.
 * @param renderer The active renderer to issue draw calls against.
 */
typedef void ( *wp_graphics_object_render_func )( wp_graphics_object *obj, wp_renderer *renderer );

/* -------------------------------------------------------------------------
 * AABB type
 * ---------------------------------------------------------------------- */

/**
 * @brief Axis-aligned bounding box expressed as a min/max pair of 3D points.
 */
typedef struct
{
    wp_vec3f min; /**< Minimum corner of the bounding box. */
    wp_vec3f max; /**< Maximum corner of the bounding box. */
} wp_aabb3f;

/* -------------------------------------------------------------------------
 * Object flags
 * ---------------------------------------------------------------------- */

/** @brief Flag marking the object as an overlay (drawn on top of scene geometry). */
#define WORKPHONE_GRAPHICS_OBJECT_FLAG_OVERLAY ( 1u << 0 )

/** @brief Flag marking the object as a UI element. */
#define WORKPHONE_GRAPHICS_OBJECT_FLAG_UI ( 1u << 1 )

/** @brief Flag marking the object as a normal scene object. */
#define WORKPHONE_GRAPHICS_OBJECT_FLAG_SCENE ( 1u << 2 )

/** @brief Internal flag indicating the object is attached to a scene node. */
#define WORKPHONE_GRAPHICS_OBJECT_FLAG_ATTACHED ( 1u << 3 )

/** @brief Internal flag indicating the object is currently visible. */
#define WORKPHONE_GRAPHICS_OBJECT_FLAG_VISIBLE ( 1u << 4 )

/** @brief Internal flag indicating the object casts shadows. */
#define WORKPHONE_GRAPHICS_OBJECT_FLAG_CAST_SHADOWS ( 1u << 5 )

/** @brief Internal flag indicating the object receives shadows. */
#define WORKPHONE_GRAPHICS_OBJECT_FLAG_RECV_SHADOWS ( 1u << 6 )

/** @brief Mask covering all defined property flags. */
#define WORKPHONE_GRAPHICS_OBJECT_ALL_PROPERTIES ( 0x7Fu )

/* -------------------------------------------------------------------------
 * Render queue groups
 * ---------------------------------------------------------------------- */

/**
 * @brief Render queue group identifiers used to control draw order.
 */
typedef enum wp_render_queue_group
{
    WORKPHONE_RENDER_QUEUE_BACKGROUND = 0, /**< Rendered before all other geometry. */
    WORKPHONE_RENDER_QUEUE_DEFAULT = 50,   /**< Standard scene geometry. */
    WORKPHONE_RENDER_QUEUE_OVERLAY = 100,  /**< Rendered after scene geometry. */
    WORKPHONE_RENDER_QUEUE_UI = 200        /**< User-interface elements, rendered last. */
} wp_render_queue_group;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a new graphics object with default state.
 * @return Pointer to the created graphics object, or NULL on failure.
 */
wp_graphics_object *wp_graphics_object_create( void );

/**
 * @brief Destroys a graphics object and frees its resources.
 * @param obj Pointer to the graphics object to destroy.
 */
void wp_graphics_object_destroy( wp_graphics_object *obj );

/* -------------------------------------------------------------------------
 * Visibility
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the coarse visible state of the object.
 * @param obj Pointer to the graphics object.
 * @param visible Non-zero to make the object visible; zero to hide it.
 */
void wp_graphics_object_set_visible( wp_graphics_object *obj, wp_s32 visible );

/**
 * @brief Queries the coarse visible state of the object.
 * @param obj Pointer to the graphics object.
 * @return Non-zero if the object is currently visible, zero otherwise.
 */
wp_s32 wp_graphics_object_is_visible( const wp_graphics_object *obj );

/* -------------------------------------------------------------------------
 * Shadow casting and receiving
 * ---------------------------------------------------------------------- */

/**
 * @brief Enables or disables shadow casting for the object.
 * @param obj Pointer to the graphics object.
 * @param cast Non-zero to enable shadow casting; zero to disable.
 */
void wp_graphics_object_set_cast_shadows( wp_graphics_object *obj, wp_s32 cast );

/**
 * @brief Queries whether the object casts shadows.
 * @param obj Pointer to the graphics object.
 * @return Non-zero if shadow casting is enabled.
 */
wp_s32 wp_graphics_object_get_cast_shadows( const wp_graphics_object *obj );

/**
 * @brief Enables or disables shadow receiving for the object.
 * @param obj Pointer to the graphics object.
 * @param receive Non-zero to enable shadow receiving; zero to disable.
 */
void wp_graphics_object_set_receive_shadows( wp_graphics_object *obj, wp_s32 receive );

/**
 * @brief Queries whether the object receives shadows.
 * @param obj Pointer to the graphics object.
 * @return Non-zero if shadow receiving is enabled.
 */
wp_s32 wp_graphics_object_get_receive_shadows( const wp_graphics_object *obj );

/* -------------------------------------------------------------------------
 * Visibility flags (bitmask)
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the visibility flags bitmask for camera/layer-based culling.
 * @param obj Pointer to the graphics object.
 * @param flags Bitmask that is ANDed against the scene manager visibility mask.
 *
 * @note Final visibility is (objectFlags & sceneVisibilityMask) != 0.
 */
void wp_graphics_object_set_visibility_flags( wp_graphics_object *obj, wp_u32 flags );

/**
 * @brief Gets the visibility flags bitmask.
 * @param obj Pointer to the graphics object.
 * @return Current visibility flags bitmask.
 */
wp_u32 wp_graphics_object_get_visibility_flags( const wp_graphics_object *obj );

/* -------------------------------------------------------------------------
 * Z-order and render queue
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the Z-order used for draw-call ordering within the same render queue.
 * @param obj Pointer to the graphics object.
 * @param z_order Renderer-specific Z-order value.
 */
void wp_graphics_object_set_z_order( wp_graphics_object *obj, wp_u32 z_order );

/**
 * @brief Gets the current Z-order value.
 * @param obj Pointer to the graphics object.
 * @return Current Z-order value.
 */
wp_u32 wp_graphics_object_get_z_order( const wp_graphics_object *obj );

/**
 * @brief Sets the render queue group that schedules this object for drawing.
 * @param obj Pointer to the graphics object.
 * @param queue_id Renderer-specific queue group identifier.
 */
void wp_graphics_object_set_render_queue_group( wp_graphics_object *obj, wp_u32 queue_id );

/**
 * @brief Gets the current render queue group identifier.
 * @param obj Pointer to the graphics object.
 * @return Current render queue group identifier.
 */
wp_u32 wp_graphics_object_get_render_queue_group( const wp_graphics_object *obj );

/* -------------------------------------------------------------------------
 * Render technique
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the render technique by its hash identifier.
 * @param obj Pointer to the graphics object.
 * @param technique Hash value identifying the technique to apply.
 */
void wp_graphics_object_set_render_technique( wp_graphics_object *obj, wp_u32 technique );

/**
 * @brief Gets the hash identifier of the active render technique.
 * @param obj Pointer to the graphics object.
 * @return Hash value of the current render technique.
 */
wp_u32 wp_graphics_object_get_render_technique( const wp_graphics_object *obj );

/* -------------------------------------------------------------------------
 * Object flags (bitmask)
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets or clears a single flag bit in the object's flags.
 * @param obj Pointer to the graphics object.
 * @param flag Single flag bit to modify (use WORKPHONE_GRAPHICS_OBJECT_FLAG_* constants).
 * @param value Non-zero to set the bit; zero to clear it.
 */
void wp_graphics_object_set_flag( wp_graphics_object *obj, wp_u32 flag, wp_s32 value );

/**
 * @brief Queries a single flag bit.
 * @param obj Pointer to the graphics object.
 * @param flag Flag bit to test.
 * @return Non-zero if the flag bit is set.
 */
wp_s32 wp_graphics_object_get_flag( const wp_graphics_object *obj, wp_u32 flag );

/**
 * @brief Gets the full flags bitmask.
 * @param obj Pointer to the graphics object.
 * @return Bitmask of all currently set flags.
 */
wp_u32 wp_graphics_object_get_flags( const wp_graphics_object *obj );

/**
 * @brief Replaces the full flags bitmask.
 * @param obj Pointer to the graphics object.
 * @param flags New flags bitmask.
 */
void wp_graphics_object_set_flags( wp_graphics_object *obj, wp_u32 flags );

/* -------------------------------------------------------------------------
 * Local AABB
 * ---------------------------------------------------------------------- */

/**
 * @brief Gets the object's local axis-aligned bounding box.
 * @param obj Pointer to the graphics object.
 * @return AABB in the object's local coordinate space.
 *
 * @note World-space bounds are computed by combining the local AABB with the
 *       owner scene node's world transform.
 */
wp_aabb3f wp_graphics_object_get_local_aabb( const wp_graphics_object *obj );

/**
 * @brief Sets the object's local axis-aligned bounding box.
 * @param obj Pointer to the graphics object.
 * @param aabb Local-space AABB enclosing the object's visible geometry.
 */
void wp_graphics_object_set_local_aabb( wp_graphics_object *obj, wp_aabb3f aabb );

/* -------------------------------------------------------------------------
 * Scene node attachment
 * ---------------------------------------------------------------------- */

/**
 * @brief Attaches the graphics object to a parent scene node.
 * @param obj Pointer to the graphics object.
 * @param parent Pointer to the parent scene node.
 */
void wp_graphics_object_attach_to_parent( wp_graphics_object *obj, wp_scenenode *parent );

/**
 * @brief Detaches the graphics object from a parent scene node.
 * @param obj Pointer to the graphics object.
 * @param parent Pointer to the parent scene node to detach from.
 *
 * @note If the object is not attached to the specified parent this call is a no-op.
 */
void wp_graphics_object_detach_from_parent( wp_graphics_object *obj, wp_scenenode *parent );

/**
 * @brief Queries whether the object is currently attached to any scene node.
 * @param obj Pointer to the graphics object.
 * @return Non-zero if attached, zero otherwise.
 */
wp_s32 wp_graphics_object_is_attached( const wp_graphics_object *obj );

/**
 * @brief Explicitly sets the attached state flag.
 * @param obj Pointer to the graphics object.
 * @param attached Non-zero to mark as attached; zero to mark as detached.
 *
 * @note Prefer wp_graphics_object_attach_to_parent / wp_graphics_object_detach_from_parent
 *       to keep parent references consistent.
 */
void wp_graphics_object_set_attached( wp_graphics_object *obj, wp_s32 attached );

/**
 * @brief Gets the scene node that currently owns this object.
 * @param obj Pointer to the graphics object.
 * @return Pointer to the owner scene node, or NULL if not attached.
 */
wp_scenenode *wp_graphics_object_get_owner( const wp_graphics_object *obj );

/**
 * @brief Sets the owner scene node for this object.
 * @param obj Pointer to the graphics object.
 * @param node Pointer to the new owner scene node, or NULL to clear ownership.
 */
void wp_graphics_object_set_owner( wp_graphics_object *obj, wp_scenenode *node );

/* -------------------------------------------------------------------------
 * Creator / graphics scene
 * ---------------------------------------------------------------------- */

/**
 * @brief Gets the graphics scene that created or manages this object.
 * @param obj Pointer to the graphics object.
 * @return Pointer to the creator graphics scene, or NULL if none.
 */
wp_graphics_scene *wp_graphics_object_get_creator( const wp_graphics_object *obj );

/**
 * @brief Sets the creator graphics scene for this object.
 * @param obj Pointer to the graphics object.
 * @param scene Pointer to the graphics scene that created this object.
 */
void wp_graphics_object_set_creator( wp_graphics_object *obj, wp_graphics_scene *scene );

/* -------------------------------------------------------------------------
 * Render callback
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the per-object render callback.
 *
 * The graphics system calls this function for every visible object during
 * wp_graphics_system_render.  The callback is responsible for issuing the
 * actual draw calls via the supplied renderer.
 *
 * @param obj Pointer to the graphics object.
 * @param fn  Render callback, or NULL to clear.
 */
void wp_graphics_object_set_render_func( wp_graphics_object *obj, wp_graphics_object_render_func fn );

/**
 * @brief Gets the current render callback.
 * @param obj Pointer to the graphics object.
 * @return The registered callback, or NULL if none is set.
 */
wp_graphics_object_render_func wp_graphics_object_get_render_func( const wp_graphics_object *obj );

/* -------------------------------------------------------------------------
 * Built-in mesh renderable

 * * ---------------------------------------------------------------------- */

/**
 * @brief Associates a non-owning mesh with this object.
 *
 * A non-NULL mesh installs the library's built-in mesh submission callback.
 * The caller retains ownership and must clear the association before destroying
 * the mesh.
 */
void wp_graphics_object_set_mesh( wp_graphics_object *obj, wp_graphics_mesh *mesh );

/** @brief Returns the associated mesh, or NULL. */
wp_graphics_mesh *wp_graphics_object_get_mesh( const wp_graphics_object *obj );

/** @brief Associates a non-owning material with the built-in mesh renderable. */
void wp_graphics_object_set_material( wp_graphics_object *obj, wp_graphics_material *material );

/** @brief Returns the associated material, or NULL. */
wp_graphics_material *wp_graphics_object_get_material( const wp_graphics_object *obj );

/* -------------------------------------------------------------------------
 * State management
 * ---------------------------------------------------------------------- */

/**
 * @brief Marks the object as dirty to signal that cached CPU/GPU data needs updating.
 * @param obj Pointer to the graphics object.
 *
 * @details Implementations use this to schedule re-uploads of GPU buffers,
 *          recalculation of derived bounds, or rebuilding of render batches.
 */
void wp_graphics_object_make_dirty( wp_graphics_object *obj );

/* -------------------------------------------------------------------------
 * Native object access
 * ---------------------------------------------------------------------- */

/**
 * @brief Retrieves the underlying renderer-native object pointer.
 * @param obj Pointer to the graphics object.
 * @param pp_object Output pointer that receives the native object pointer.
 *
 * @note The concrete type and ownership semantics of the native pointer are
 *       renderer-dependent. The value may be NULL if no native object exists.
 */
void wp_graphics_object_get_native( const wp_graphics_object *obj, void **pp_object );

/** Borrowed per-object submission context. The wrapper must clear this before
 * releasing its owner; native rendering and scene ownership do not use it. */
void wp_graphics_object_set_submit_data( wp_graphics_object *obj, void *data );
void *wp_graphics_object_get_submit_data( const wp_graphics_object *obj );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_OBJECT_H */
