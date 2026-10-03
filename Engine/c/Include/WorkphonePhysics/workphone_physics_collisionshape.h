/**
 * @file wp_collisionshape.h
 * @brief C API for a 3D physics collision shape.
 *
 * A collision shape defines the geometry used by a rigid body for collision
 * detection.  Supported primitive types are box, sphere, capsule, and plane.
 * A mesh type is also provided for convex/trimesh geometry supplied by the
 * caller.
 *
 * Shapes can be enabled or disabled independently of their parent body and may
 * optionally act as trigger volumes (overlap detection without physical
 * response).  Collision filtering is controlled by two 32-bit bitmasks: a
 * type (category) mask that identifies this shape, and a collision mask that
 * describes which categories this shape should interact with.
 */

#ifndef WORKPHONE_COLLISIONSHAPE_H
#define WORKPHONE_COLLISIONSHAPE_H

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_quat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_collision_shape wp_collision_shape;
typedef struct wp_rigidbody wp_rigidbody;
typedef struct wp_physics_material wp_physics_material;
typedef struct wp_triangle_mesh wp_triangle_mesh;

/* -------------------------------------------------------------------------
 * Shape type
 * ---------------------------------------------------------------------- */

/**
 * @brief Primitive geometry type of a collision shape.
 */
typedef enum wp_collision_shape_type
{
    WORKPHONE_COLLISION_SHAPE_BOX = 0,     /**< Axis-aligned box defined by half-extents.              */
    WORKPHONE_COLLISION_SHAPE_SPHERE = 1,  /**< Sphere defined by a radius.                            */
    WORKPHONE_COLLISION_SHAPE_CAPSULE = 2, /**< Capsule (cylinder capped with hemispheres) defined by
                                         a radius and a half-height along the Y axis.           */
    WORKPHONE_COLLISION_SHAPE_PLANE = 3,   /**< Infinite plane defined by a normal and offset.         */
    WORKPHONE_COLLISION_SHAPE_MESH = 4     /**< Triangle or convex mesh supplied by the caller.        */
} wp_collision_shape_type;

/** @brief Shape participates in collision detection. */
#define WORKPHONE_COLLISION_SHAPE_FLAG_ENABLED ( 1u << 0 )

/** @brief Shape acts as a trigger volume (events only, no physical response). */
#define WORKPHONE_COLLISION_SHAPE_FLAG_TRIGGER ( 1u << 1 )

/**
 * @brief Four-word collision filter data identical in layout to PhysX FilterData.
 *
 * word0 and word1 are typically used for category/mask bits.
 * word2 and word3 are available for application-defined use.
 */
typedef struct
{
    wp_u32 word0; /**< Collision type / category bitmask for this shape.    */
    wp_u32 word1; /**< Collision mask: categories this shape interacts with. */
    wp_u32 word2; /**< Application-defined filter word 2.                   */
    wp_u32 word3; /**< Application-defined filter word 3.                   */
} wp_filter_data;

/**
 * @brief Describes a triangle mesh to be used as a collision shape.
 *
 * The caller owns the pointed-to arrays; they must remain valid for the
 * lifetime of the shape or until new mesh data is supplied.
 */
typedef struct
{
    const wp_f32 *vertices; /**< Flat XYZ positions (3 wp_f32s per vertex). */
    wp_u32 vertex_count;    /**< Number of vertices.                        */
    const wp_u32 *indices;  /**< Flat triangle indices (3 per triangle).    */
    wp_u32 triangle_count;  /**< Number of triangles.                       */
} wp_collision_mesh_data;

/**
 * @brief Creates a new collision shape of the specified type.
 *
 * The shape is initialised with default dimensions (half-extents 0.5,
 * radius 0.5, half-height 0.5), an identity local pose, all-pass filter data,
 * and the enabled flag set.
 *
 * @param type  Primitive geometry type for the shape.
 * @return Pointer to the created collision shape, or NULL on allocation failure.
 */
wp_collision_shape *wp_collision_shape_create( wp_collision_shape_type type );

/**
 * @brief Destroys a collision shape and releases its resources.
 * @param shape Pointer to the collision shape to destroy. Ignored if NULL.
 */
void wp_collision_shape_destroy( wp_collision_shape *shape );

/**
 * @brief Returns the primitive type of the shape.
 * @param shape Pointer to the collision shape.
 * @return Primitive type enumeration value.
 */
wp_collision_shape_type wp_collision_shape_get_type( const wp_collision_shape *shape );

/**
 * @brief Sets the half-extents of a box shape.
 *
 * Only meaningful when the shape type is WORKPHONE_COLLISION_SHAPE_BOX.
 *
 * @param shape        Pointer to the collision shape.
 * @param half_extents Half-extents along the X, Y, and Z axes.
 */
void wp_collision_shape_set_box_half_extents( wp_collision_shape *shape, wp_vec3f half_extents );

/**
 * @brief Gets the half-extents of a box shape.
 * @param shape Pointer to the collision shape.
 * @return Half-extents, or a zero vector if shape is NULL.
 */
wp_vec3f wp_collision_shape_get_box_half_extents( const wp_collision_shape *shape );

/**
 * @brief Sets the radius of a sphere shape.
 *
 * Only meaningful when the shape type is WORKPHONE_COLLISION_SHAPE_SPHERE.
 *
 * @param shape  Pointer to the collision shape.
 * @param radius Sphere radius (must be positive).
 */
void wp_collision_shape_set_sphere_radius( wp_collision_shape *shape, wp_f32 radius );

/**
 * @brief Gets the radius of a sphere shape.
 * @param shape Pointer to the collision shape.
 * @return Radius value, or 0 if shape is NULL.
 */
wp_f32 wp_collision_shape_get_sphere_radius( const wp_collision_shape *shape );

/**
 * @brief Sets the dimensions of a capsule shape.
 *
 * Only meaningful when the shape type is WORKPHONE_COLLISION_SHAPE_CAPSULE.
 * The capsule axis is aligned with the local Y axis.
 *
 * @param shape       Pointer to the collision shape.
 * @param radius      Radius of the cylindrical shaft and hemispherical caps.
 * @param half_height Half the length of the cylindrical shaft (not including caps).
 */
void wp_collision_shape_set_capsule( wp_collision_shape *shape, wp_f32 radius, wp_f32 half_height );

/**
 * @brief Gets the radius of a capsule shape.
 * @param shape Pointer to the collision shape.
 * @return Capsule radius, or 0 if shape is NULL.
 */
wp_f32 wp_collision_shape_get_capsule_radius( const wp_collision_shape *shape );

/**
 * @brief Gets the half-height of the cylindrical part of a capsule shape.
 * @param shape Pointer to the collision shape.
 * @return Half-height value, or 0 if shape is NULL.
 */
wp_f32 wp_collision_shape_get_capsule_half_height( const wp_collision_shape *shape );

/* -------------------------------------------------------------------------
 * Plane
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the plane equation for a plane shape.
 *
 * Only meaningful when the shape type is WORKPHONE_COLLISION_SHAPE_PLANE.
 * The plane is defined as dot(normal, x) + offset = 0.
 *
 * @param shape  Pointer to the collision shape.
 * @param normal Unit normal of the plane.
 * @param offset Signed distance from the world origin to the plane along the normal.
 */
void wp_collision_shape_set_plane( wp_collision_shape *shape, wp_vec3f normal, wp_f32 offset );

/**
 * @brief Gets the normal of a plane shape.
 * @param shape Pointer to the collision shape.
 * @return Plane normal, or a zero vector if shape is NULL.
 */
wp_vec3f wp_collision_shape_get_plane_normal( const wp_collision_shape *shape );

/**
 * @brief Gets the offset of a plane shape.
 * @param shape Pointer to the collision shape.
 * @return Plane offset, or 0 if shape is NULL.
 */
wp_f32 wp_collision_shape_get_plane_offset( const wp_collision_shape *shape );

/* -------------------------------------------------------------------------
 * Mesh data
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the mesh data for a mesh-type collision shape.
 *
 * Only meaningful when the shape type is WORKPHONE_COLLISION_SHAPE_MESH.
 * The shape stores a shallow reference to the data; the caller must keep
 * the arrays alive until the shape is destroyed or new data is set.
 *
 * @param shape Pointer to the collision shape.
 * @param data  Pointer to the mesh data descriptor.
 */
void wp_collision_shape_set_mesh_data( wp_collision_shape *shape, const wp_collision_mesh_data *data );

/**
 * @brief Gets the mesh data associated with a mesh-type collision shape.
 * @param shape Pointer to the collision shape.
 * @return Pointer to the stored mesh data descriptor, or NULL if shape is NULL
 *         or the shape is not of mesh type.
 */
const wp_collision_mesh_data *wp_collision_shape_get_mesh_data( const wp_collision_shape *shape );

/**
 * @brief Gets the internal accelerated triangle mesh used for collision queries.
 *
 * The returned object is owned by the collision shape and is rebuilt whenever
 * wp_collision_shape_set_mesh_data is called.
 *
 * @param shape Pointer to a mesh-type collision shape.
 * @return Read-only accelerated mesh, or NULL when no valid mesh is assigned.
 */
const wp_triangle_mesh *wp_collision_shape_get_triangle_mesh( const wp_collision_shape *shape );

/* -------------------------------------------------------------------------
 * Local pose
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the local position of the shape relative to its parent body.
 * @param shape    Pointer to the collision shape.
 * @param position Local position offset.
 */
void wp_collision_shape_set_local_position( wp_collision_shape *shape, wp_vec3f position );

/**
 * @brief Gets the local position of the shape relative to its parent body.
 * @param shape Pointer to the collision shape.
 * @return Local position, or a zero vector if shape is NULL.
 */
wp_vec3f wp_collision_shape_get_local_position( const wp_collision_shape *shape );

/**
 * @brief Sets the local orientation of the shape as a unit quaternion.
 * @param shape       Pointer to the collision shape.
 * @param orientation Local orientation quaternion.
 */
void wp_collision_shape_set_local_orientation( wp_collision_shape *shape, wp_quatf orientation );

/**
 * @brief Gets the local orientation of the shape.
 * @param shape Pointer to the collision shape.
 * @return Local orientation quaternion, or identity (0,0,0,1) if shape is NULL.
 */
wp_quatf wp_collision_shape_get_local_orientation( const wp_collision_shape *shape );

/* -------------------------------------------------------------------------
 * Enable / trigger
 * ---------------------------------------------------------------------- */

/**
 * @brief Enables or disables the collision shape.
 *
 * A disabled shape is excluded from collision detection while remaining
 * attached to its parent body.
 *
 * @param shape   Pointer to the collision shape.
 * @param enabled Non-zero to enable; zero to disable.
 */
void wp_collision_shape_set_enabled( wp_collision_shape *shape, wp_s32 enabled );

/**
 * @brief Queries whether the collision shape is enabled.
 * @param shape Pointer to the collision shape.
 * @return Non-zero if enabled; zero otherwise.
 */
wp_s32 wp_collision_shape_is_enabled( const wp_collision_shape *shape );

/**
 * @brief Sets the trigger mode for the shape.
 *
 * Trigger shapes fire enter/exit events on overlap but do not generate
 * contact forces.
 *
 * @param shape   Pointer to the collision shape.
 * @param trigger Non-zero to enable trigger mode; zero to disable.
 */
void wp_collision_shape_set_trigger( wp_collision_shape *shape, wp_s32 trigger );

/**
 * @brief Queries whether the shape is in trigger mode.
 * @param shape Pointer to the collision shape.
 * @return Non-zero if the shape is a trigger; zero otherwise.
 */
wp_s32 wp_collision_shape_is_trigger( const wp_collision_shape *shape );

/* -------------------------------------------------------------------------
 * Collision filtering
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the collision type (category) bitmask for this shape.
 * @param shape Pointer to the collision shape.
 * @param mask  Category bitmask identifying what this shape is.
 */
void wp_collision_shape_set_collision_type( wp_collision_shape *shape, wp_u32 mask );

/**
 * @brief Gets the collision type (category) bitmask.
 * @param shape Pointer to the collision shape.
 * @return Current category bitmask.
 */
wp_u32 wp_collision_shape_get_collision_type( const wp_collision_shape *shape );

/**
 * @brief Sets the collision mask controlling which categories this shape
 *        interacts with.
 * @param shape Pointer to the collision shape.
 * @param mask  Bitmask of categories that can collide with this shape.
 */
void wp_collision_shape_set_collision_mask( wp_collision_shape *shape, wp_u32 mask );

/**
 * @brief Gets the collision mask.
 * @param shape Pointer to the collision shape.
 * @return Current collision mask bitmask.
 */
wp_u32 wp_collision_shape_get_collision_mask( const wp_collision_shape *shape );

/**
 * @brief Replaces the full four-word filter data for this shape.
 * @param shape Pointer to the collision shape.
 * @param data  New filter data to apply.
 */
void wp_collision_shape_set_filter_data( wp_collision_shape *shape, wp_filter_data data );

/**
 * @brief Gets the full four-word filter data for this shape.
 * @param shape Pointer to the collision shape.
 * @return Current filter data, or all-zero data if shape is NULL.
 */
wp_filter_data wp_collision_shape_get_filter_data( const wp_collision_shape *shape );

/* -------------------------------------------------------------------------
 * Body attachment
 * ---------------------------------------------------------------------- */

/**
 * @brief Attaches the shape to a rigid body.
 * @param shape Pointer to the collision shape.
 * @param body  Pointer to the owning rigid body, or NULL to detach.
 */
void wp_collision_shape_set_body( wp_collision_shape *shape, wp_rigidbody *body );

/**
 * @brief Gets the rigid body this shape is attached to.
 * @param shape Pointer to the collision shape.
 * @return Pointer to the owning rigid body, or NULL if not attached.
 */
wp_rigidbody *wp_collision_shape_get_body( const wp_collision_shape *shape );

/**
 * @brief Queries whether the shape is currently attached to a body.
 * @param shape Pointer to the collision shape.
 * @return Non-zero if the shape is attached; zero otherwise.
 */
wp_s32 wp_collision_shape_is_attached( const wp_collision_shape *shape );

/* -------------------------------------------------------------------------
 * Material
 *
 * ---------------------------------------------------------------------- */

void wp_collision_shape_set_material( wp_collision_shape *shape, wp_physics_material *material );
wp_physics_material *wp_collision_shape_get_material( const wp_collision_shape *shape );

/* Data-driven material reference (resolved through a registry by name). */
void wp_collision_shape_set_material_name( wp_collision_shape *shape, const wp_c8 *name );
const wp_c8 *wp_collision_shape_get_material_name( const wp_collision_shape *shape );

struct wp_physics_material_registry;
/** @brief Resolves the shape's material: its own name via the registry, else
 *         the owning body's material name, else the registry default. */
wp_physics_material *wp_collision_shape_resolve_material( const wp_collision_shape *shape,
                                                   struct wp_physics_material_registry *registry );

/* -------------------------------------------------------------------------
 * Native / user data
 *
 * ---------------------------------------------------------------------- */

/**
 * @brief Stores an opaque native-backend pointer on the shape.
 * @param shape  Pointer to the collision shape.
 * @param native Opaque pointer to the backend object (e.g. PxShape *).
 */
void wp_collision_shape_set_native( wp_collision_shape *shape, void *native );

/**
 * @brief Retrieves the opaque native-backend pointer.
 * @param shape Pointer to the collision shape.
 * @return Stored native pointer, or NULL if none has been set.
 */
void *wp_collision_shape_get_native( const wp_collision_shape *shape );

/**
 * @brief Stores an opaque user data pointer on the shape.
 * @param shape     Pointer to the collision shape.
 * @param user_data Opaque pointer for caller use.
 */
void wp_collision_shape_set_user_data( wp_collision_shape *shape, void *user_data );

/**
 * @brief Retrieves the opaque user data pointer.
 * @param shape Pointer to the collision shape.
 * @return Stored user data pointer, or NULL if none has been set.
 */
void *wp_collision_shape_get_user_data( const wp_collision_shape *shape );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_COLLISIONSHAPE_H */
