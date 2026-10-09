/* Native implementation details; not part of the installed C API. */
#ifndef WORKPHONE_PHYSICS_INTERNAL_H
#define WORKPHONE_PHYSICS_INTERNAL_H

struct wp_rigidbody;
struct wp_triangle_mesh;
/* Geometry edits invalidate both the world AABB and cached local OBB fit.
 * Pose setters advance only the world bounds revision. */
void wp_rigidbody_invalidate_bounds( struct wp_rigidbody *body );
void wp_triangle_mesh_set_refit_callback( struct wp_triangle_mesh *mesh, void ( *callback )( void * ),
                                          void *context );

#endif
