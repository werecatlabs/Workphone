/**
 * @file wp_node.h
 * @brief C API for a basic scene graph node (position, orientation, scale, parent).
 *
 * A node is the fundamental transform unit in the scene graph hierarchy.
 * Each node holds a local position, orientation (quaternion), and scale, and
 * may have a parent node from which its world transform is derived.
 */

#ifndef WORKPHONE_NODE_H
#define WORKPHONE_NODE_H

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_quat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_node wp_node;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a new node with an identity transform.
 *
 * The node is initialised with zero position, unit scale, and identity
 * orientation (0, 0, 0, 1). It has no parent.
 *
 * @return Pointer to the created node, or NULL on allocation failure.
 */
wp_node *wp_node_create( void );

/**
 * @brief Destroys a node and releases its resources.
 * @param node Pointer to the node to destroy. Ignored if NULL.
 */
void wp_node_destroy( wp_node *node );

/* -------------------------------------------------------------------------
 * Parent
 * ---------------------------------------------------------------------- */

/**
 * @brief Gets the parent node of this node.
 * @param node Pointer to the node.
 * @return Pointer to the parent node, or NULL if this node has no parent.
 */
wp_node *wp_node_get_parent( const wp_node *node );

/**
 * @brief Sets the parent node.
 * @param node   Pointer to the node whose parent to update.
 * @param parent Pointer to the new parent node, or NULL to detach from any parent.
 */
void wp_node_set_parent( wp_node *node, wp_node *parent );

/* -------------------------------------------------------------------------
 * Position
 * ---------------------------------------------------------------------- */

/**
 * @brief Gets the local position of the node relative to its parent.
 * @param node Pointer to the node.
 * @return Local position as a 3D wp_f32 vector. Returns a zero vector if node is NULL.
 */
wp_vec3f wp_node_get_position( const wp_node *node );

/**
 * @brief Sets the local position of the node relative to its parent.
 * @param node     Pointer to the node.
 * @param position New local position.
 */
void wp_node_set_position( wp_node *node, wp_vec3f position );

/* -------------------------------------------------------------------------
 * Orientation
 * ---------------------------------------------------------------------- */

/**
 * @brief Gets the local orientation of the node as a unit quaternion.
 * @param node Pointer to the node.
 * @return Local orientation quaternion. Returns identity (0,0,0,1) if node is NULL.
 */
wp_quatf wp_node_get_orientation( const wp_node *node );

/**
 * @brief Sets the local orientation of the node.
 * @param node        Pointer to the node.
 * @param orientation New local orientation as a unit quaternion.
 */
void wp_node_set_orientation( wp_node *node, wp_quatf orientation );

/* -------------------------------------------------------------------------
 * Scale
 * ---------------------------------------------------------------------- */

/**
 * @brief Gets the local scale factors of the node.
 * @param node Pointer to the node.
 * @return Local scale as a 3D wp_f32 vector. Returns a zero vector if node is NULL.
 */
wp_vec3f wp_node_get_scale( const wp_node *node );

/**
 * @brief Sets the local scale factors of the node.
 * @param node  Pointer to the node.
 * @param scale New local scale factors.
 */
void wp_node_set_scale( wp_node *node, wp_vec3f scale );

/* -------------------------------------------------------------------------
 * Native object access
 * ---------------------------------------------------------------------- */

/**
 * @brief Retrieves the underlying native implementation pointer.
 * @param node      Pointer to the node.
 * @param pp_object Output pointer that receives the native object pointer.
 *                  Set to NULL if node is NULL or no native object is set.
 *
 * @note The returned pointer is opaque; callers must know the concrete type.
 */
void wp_node_get_native( const wp_node *node, void **pp_object );

/**
 * @brief Sets the underlying native implementation pointer.
 * @param node   Pointer to the node.
 * @param native Pointer to the native object to associate with this node.
 */
void wp_node_set_native( wp_node *node, void *native );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_NODE_H */
