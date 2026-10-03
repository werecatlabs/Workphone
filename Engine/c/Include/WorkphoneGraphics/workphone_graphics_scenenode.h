/**
 * @file wp_scenenode.h
 * @brief C API for a graphics scene node.
 *
 * A scene node is a positional entity in the scene graph that carries a
 * spatial transform (position, orientation, scale).  Nodes form a parent/child
 * hierarchy and act as attachment points for graphics objects.
 */

#ifndef WORKPHONE_SCENENODE_H
#define WORKPHONE_SCENENODE_H

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
typedef struct wp_graphics_scene wp_graphics_scene;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a new scene node with an identity transform.
 * @return Pointer to the created scene node, or NULL on failure.
 */
wp_scenenode *wp_scenenode_create( void );

/**
 * @brief Destroys a scene node and releases its resources.
 *
 * @note Child nodes and attached graphics objects are NOT destroyed; the
 *       caller is responsible for managing their lifetimes.
 *
 * @param node Pointer to the scene node to destroy. Ignored if NULL.
 */
void wp_scenenode_destroy( wp_scenenode *node );

/* -------------------------------------------------------------------------
 * Parent / child hierarchy
 * ---------------------------------------------------------------------- */

/**
 * @brief Gets the parent node.
 * @param node Pointer to the scene node.
 * @return Pointer to the parent, or NULL if this is a root node.
 */
wp_scenenode *wp_scenenode_get_parent( const wp_scenenode *node );

/**
 * @brief Sets the parent node without updating any child list.
 * @param node   Pointer to the scene node.
 * @param parent Pointer to the new parent, or NULL to clear.
 */
void wp_scenenode_set_parent( wp_scenenode *node, wp_scenenode *parent );

/**
 * @brief Returns the number of direct children.
 * @param node Pointer to the scene node.
 * @return Number of children.
 */
wp_s32 wp_scenenode_get_child_count( const wp_scenenode *node );

/**
 * @brief Returns the child at the given zero-based index.
 * @param node  Pointer to the scene node.
 * @param index Zero-based child index.
 * @return Pointer to the child, or NULL if the index is out of range.
 */
wp_scenenode *wp_scenenode_get_child( const wp_scenenode *node, wp_s32 index );

/**
 * @brief Adds a child node and sets its parent pointer.
 * @param node  Pointer to the parent scene node.
 * @param child Pointer to the child scene node to add.
 */
void wp_scenenode_add_child( wp_scenenode *node, wp_scenenode *child );

/**
 * @brief Removes a child node and clears its parent pointer.
 * @param node  Pointer to the parent scene node.
 * @param child Pointer to the child scene node to remove.
 *
 * @note This is a no-op if child is not a direct child of node.
 */
void wp_scenenode_remove_child( wp_scenenode *node, wp_scenenode *child );

/* -------------------------------------------------------------------------
 * Transform
 * ---------------------------------------------------------------------- */

/**
 * @brief Gets the local position relative to the parent node.
 * @param node Pointer to the scene node.
 * @return Local position. Returns a zero vector if node is NULL.
 */
wp_vec3f wp_scenenode_get_position( const wp_scenenode *node );

/**
 * @brief Sets the local position relative to the parent node.
 * @param node     Pointer to the scene node.
 * @param position New local position.
 */
void wp_scenenode_set_position( wp_scenenode *node, wp_vec3f position );

/**
 * @brief Gets the local orientation as a unit quaternion.
 * @param node Pointer to the scene node.
 * @return Local orientation. Returns identity (0,0,0,1) if node is NULL.
 */
wp_quatf wp_scenenode_get_orientation( const wp_scenenode *node );

/**
 * @brief Sets the local orientation.
 * @param node        Pointer to the scene node.
 * @param orientation New local orientation as a unit quaternion.
 */
void wp_scenenode_set_orientation( wp_scenenode *node, wp_quatf orientation );

/**
 * @brief Gets the local scale factors.
 * @param node Pointer to the scene node.
 * @return Local scale. Returns a zero vector if node is NULL.
 */
wp_vec3f wp_scenenode_get_scale( const wp_scenenode *node );

/**
 * @brief Sets the local scale factors.
 * @param node  Pointer to the scene node.
 * @param scale New local scale factors.
 */
void wp_scenenode_set_scale( wp_scenenode *node, wp_vec3f scale );

/** @brief Computes the node's local transform as translation * rotation * scale. */
void wp_scenenode_get_local_matrix( const wp_scenenode *node, wp_mat4f *matrix );

/** @brief Computes the complete parent-relative world transform. */
void wp_scenenode_get_world_matrix( const wp_scenenode *node, wp_mat4f *matrix );

/* -------------------------------------------------------------------------
 * Graphics object attachment
 * ---------------------------------------------------------------------- */

/**
 * @brief Attaches a graphics object to this node and marks it as attached.
 * @param node Pointer to the scene node.
 * @param obj  Pointer to the graphics object to attach.
 */
void wp_scenenode_attach_object( wp_scenenode *node, wp_graphics_object *obj );

/**
 * @brief Detaches a graphics object from this node.
 * @param node Pointer to the scene node.
 * @param obj  Pointer to the graphics object to detach.
 *
 * @note This is a no-op if obj is not attached to node.
 */
void wp_scenenode_detach_object( wp_scenenode *node, wp_graphics_object *obj );

/**
 * @brief Returns the number of graphics objects attached to this node.
 * @param node Pointer to the scene node.
 * @return Number of attached objects.
 */
wp_s32 wp_scenenode_get_object_count( const wp_scenenode *node );

/**
 * @brief Returns the attached graphics object at the given index.
 * @param node  Pointer to the scene node.
 * @param index Zero-based index.
 * @return Pointer to the object, or NULL if the index is out of range.
 */
wp_graphics_object *wp_scenenode_get_object( const wp_scenenode *node, wp_s32 index );

/* -------------------------------------------------------------------------
 * Creator scene
 * ---------------------------------------------------------------------- */

/**
 * @brief Gets the graphics scene that owns this node.
 * @param node Pointer to the scene node.
 * @return Pointer to the creator scene, or NULL if none.
 */
wp_graphics_scene *wp_scenenode_get_creator( const wp_scenenode *node );

/**
 * @brief Sets the graphics scene that owns this node.
 * @param node  Pointer to the scene node.
 * @param scene Pointer to the creator scene, or NULL to clear.
 */
void wp_scenenode_set_creator( wp_scenenode *node, wp_graphics_scene *scene );

/* -------------------------------------------------------------------------
 * Native access
 * ---------------------------------------------------------------------- */

/**
 * @brief Retrieves the underlying native implementation pointer.
 * @param node      Pointer to the scene node.
 * @param pp_object Output pointer that receives the native object pointer.
 */
void wp_scenenode_get_native( const wp_scenenode *node, void **pp_object );

/**
 * @brief Sets the underlying native implementation pointer.
 * @param node   Pointer to the scene node.
 * @param native Pointer to the native object.
 */
void wp_scenenode_set_native( wp_scenenode *node, void *native );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_SCENENODE_H */
