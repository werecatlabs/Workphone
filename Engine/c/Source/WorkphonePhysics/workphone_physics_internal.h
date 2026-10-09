/* Native implementation details; not part of the installed C API. */
#ifndef WORKPHONE_PHYSICS_INTERNAL_H
#define WORKPHONE_PHYSICS_INTERNAL_H

struct wp_rigidbody;
struct wp_triangle_mesh;
void wp_rigidbody_invalidate_bounds( struct wp_rigidbody *body );
void wp_triangle_mesh_set_refit_callback( struct wp_triangle_mesh *mesh, void ( *callback )( void * ),
                                          void *context );

#endif
