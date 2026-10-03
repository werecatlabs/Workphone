/**
 * @file workphone_physics_narrowphase.c
 * @brief Implementation of the C physics narrowphase API.
 *
 * Supported primitive pairs
 * -------------------------
 *  sphere  vs sphere
 *  sphere  vs plane
 *  sphere  vs box
 *  box     vs box      (SAT, 15 axes)
 *  box     vs plane
 *  capsule vs plane
 *  capsule vs sphere
 *  capsule vs box
 *  capsule vs capsule
 *  sphere  vs triangle mesh
 *  box     vs triangle mesh
 *  capsule vs triangle mesh
 *
 * Mesh contacts use an AABB-tree sphere query to select nearby triangles,
 * then evaluate exact contacts against the caller-owned triangle data.
 * Dynamic mesh-vs-mesh response is intentionally excluded; production scenes
 * should use sphere, box, or capsule dynamic shapes against static meshes.
 */

#include "workphone_physics_narrowphase.h"
#include "workphone_physics.h"
#include "workphone_physics_collisionshape.h"
#include "workphone_physics_rigidbody.h"
#include "workphone_physics_triangle_mesh.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* =========================================================================
 * Internal constants
 * ====================================================================== */

#ifndef WP_NARROWPHASE_MAX_MANIFOLDS
#    define WP_NARROWPHASE_MAX_MANIFOLDS 1024
#endif

#define WP_NP_DEFAULT_TOLERANCE 0.001f
#define WP_NP_DEFAULT_ITERATIONS 64
#define WP_NP_MESH_QUERY_CAPACITY 1024u

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_narrowphase
{
    wp_narrowphase_algorithm algorithm;
    wp_s32 max_iterations;
    wp_f32 contact_tolerance;

    wp_contact_manifold manifolds[WP_NARROWPHASE_MAX_MANIFOLDS];
    wp_s32 manifold_count;
    wp_s32 touching_pair_count;

    void *native;
    void *user_data;
} wp_narrowphase;

/* =========================================================================
 * Internal math helpers
 * ====================================================================== */

static wp_f32 np_sqrtf( wp_f32 x )
{
    return sqrtf( x );
}

static wp_f32 np_dot( wp_vec3f a, wp_vec3f b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static wp_vec3f np_sub( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

static wp_vec3f np_add( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

static wp_vec3f np_scale( wp_vec3f v, wp_f32 s )
{
    wp_vec3f r;
    r.x = v.x * s;
    r.y = v.y * s;
    r.z = v.z * s;
    return r;
}

static wp_f32 np_len_sq( wp_vec3f v )
{
    return np_dot( v, v );
}

static wp_f32 np_len( wp_vec3f v )
{
    return np_sqrtf( np_len_sq( v ) );
}

static wp_vec3f np_normalize( wp_vec3f v )
{
    wp_f32 len = np_len( v );
    if( len < 1e-8f )
    {
        wp_vec3f up;
        memset( &up, 0, sizeof( up ) );
        up.y = 1.0f;
        return up;
    }
    return np_scale( v, 1.0f / len );
}

static wp_vec3f np_cross( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

static wp_f32 np_clampf( wp_f32 v, wp_f32 lo, wp_f32 hi )
{
    return v < lo ? lo : ( v > hi ? hi : v );
}

static wp_f32 np_fabsf( wp_f32 v )
{
    return v < 0.0f ? -v : v;
}

static wp_vec3f np_zero( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

static wp_quatf np_quat_normalize( wp_quatf q )
{
    wp_f32 length = sqrtf( q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w );
    if( length <= 1.0e-8f )
    {
        memset( &q, 0, sizeof( q ) );
        q.w = 1.0f;
        return q;
    }
    q.x /= length;
    q.y /= length;
    q.z /= length;
    q.w /= length;
    return q;
}

static wp_quatf np_quat_multiply( wp_quatf a, wp_quatf b )
{
    wp_quatf q;
    q.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    q.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    q.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    q.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return q;
}

static wp_quatf np_quat_conjugate( wp_quatf q )
{
    q = np_quat_normalize( q );
    q.x = -q.x;
    q.y = -q.y;
    q.z = -q.z;
    return q;
}

static wp_vec3f np_quat_rotate( wp_quatf q, wp_vec3f v )
{
    wp_vec3f qv;
    wp_vec3f t;
    q = np_quat_normalize( q );
    qv.x = q.x;
    qv.y = q.y;
    qv.z = q.z;
    t = np_scale( np_cross( qv, v ), 2.0f );
    return np_add( v, np_add( np_scale( t, q.w ), np_cross( qv, t ) ) );
}

static wp_vec3f np_shape_position( const wp_rigidbody *body, const wp_collision_shape *shape )
{
    return np_add( wp_rigidbody_get_position( body ),
                   np_quat_rotate( wp_rigidbody_get_orientation( body ),
                                   wp_collision_shape_get_local_position( shape ) ) );
}

static wp_quatf np_shape_orientation( const wp_rigidbody *body, const wp_collision_shape *shape )
{
    return np_quat_normalize( np_quat_multiply( wp_rigidbody_get_orientation( body ),
                                                wp_collision_shape_get_local_orientation( shape ) ) );
}

static wp_vec3f np_plane_normal( const wp_rigidbody *body, const wp_collision_shape *shape )
{
    /*
     * Engine plane colliders store their normal in world-space convention.
     * Their actor may be rotated solely to orient the rendered plane mesh
     * (the stock plane asset is rotated -90 degrees around X). PhysX keeps
     * that collider world-up, so do not apply the body's visual rotation.
     * The shape-local orientation remains available for explicitly rotated
     * plane geometry.
     */
    (void)body;
    return np_normalize( np_quat_rotate( wp_collision_shape_get_local_orientation( shape ),
                                         wp_collision_shape_get_plane_normal( shape ) ) );
}

static wp_f32 np_plane_offset( const wp_rigidbody *body, const wp_collision_shape *shape )
{
    wp_vec3f normal = np_plane_normal( body, shape );
    /*
     * The public plane is n.x + d = 0. The primitive tests use the
     * equivalent n.x = offset representation in world space.
     */
    return np_dot( normal, np_shape_position( body, shape ) ) -
           wp_collision_shape_get_plane_offset( shape );
}

static wp_s32 np_mesh_triangle_world( const wp_rigidbody *body, const wp_collision_shape *shape,
                                      wp_u32 triangle_index, wp_vec3f *a, wp_vec3f *b, wp_vec3f *c )
{
    const wp_collision_mesh_data *mesh = wp_collision_shape_get_mesh_data( shape );
    wp_u32 ia;
    wp_u32 ib;
    wp_u32 ic;
    wp_vec3f position;
    wp_quatf orientation;

    if( !mesh || !mesh->vertices || !mesh->indices || triangle_index >= mesh->triangle_count || !a ||
        !b || !c )
    {
        return 0;
    }

    ia = mesh->indices[triangle_index * 3];
    ib = mesh->indices[triangle_index * 3 + 1];
    ic = mesh->indices[triangle_index * 3 + 2];
    if( ia >= mesh->vertex_count || ib >= mesh->vertex_count || ic >= mesh->vertex_count )
    {
        return 0;
    }

    a->x = mesh->vertices[ia * 3];
    a->y = mesh->vertices[ia * 3 + 1];
    a->z = mesh->vertices[ia * 3 + 2];
    b->x = mesh->vertices[ib * 3];
    b->y = mesh->vertices[ib * 3 + 1];
    b->z = mesh->vertices[ib * 3 + 2];
    c->x = mesh->vertices[ic * 3];
    c->y = mesh->vertices[ic * 3 + 1];
    c->z = mesh->vertices[ic * 3 + 2];

    position = np_shape_position( body, shape );
    orientation = np_shape_orientation( body, shape );
    *a = np_add( np_quat_rotate( orientation, *a ), position );
    *b = np_add( np_quat_rotate( orientation, *b ), position );
    *c = np_add( np_quat_rotate( orientation, *c ), position );
    return 1;
}

static wp_u32 np_mesh_sphere_candidates( const wp_rigidbody *mesh_body,
                                         const wp_collision_shape *mesh_shape, wp_vec3f world_center,
                                         wp_f32 radius, wp_u32 fallback_triangle_count,
                                         wp_u32 *candidates, wp_u32 capacity, wp_s32 *using_candidates )
{
    const wp_triangle_mesh *triangle_mesh = wp_collision_shape_get_triangle_mesh( mesh_shape );
    wp_vec3f local_center;
    wp_u32 candidate_count;

    *using_candidates = 0;
    if( !triangle_mesh )
    {
        return fallback_triangle_count;
    }

    local_center = np_quat_rotate( np_quat_conjugate( np_shape_orientation( mesh_body, mesh_shape ) ),
                                   np_sub( world_center, np_shape_position( mesh_body, mesh_shape ) ) );
    candidate_count =
        wp_triangle_mesh_query_sphere( triangle_mesh, local_center, radius, candidates, capacity );

    /*
     * A count greater than capacity means the supplied buffer is incomplete.
     * Preserve exact collision behavior by scanning the original mesh rather
     * than silently dropping candidates.
     */
    if( candidate_count == UINT32_MAX || candidate_count > capacity )
    {
        return fallback_triangle_count;
    }

    *using_candidates = 1;
    return candidate_count;
}

/* Closest point on a triangle, from Real-Time Collision Detection. */
static wp_vec3f np_closest_point_triangle( wp_vec3f point, wp_vec3f a, wp_vec3f b, wp_vec3f c )
{
    wp_vec3f ab = np_sub( b, a );
    wp_vec3f ac = np_sub( c, a );
    wp_vec3f ap = np_sub( point, a );
    wp_f32 d1 = np_dot( ab, ap );
    wp_f32 d2 = np_dot( ac, ap );
    wp_vec3f bp;
    wp_f32 d3;
    wp_f32 d4;
    wp_f32 vc;
    wp_vec3f cp;
    wp_f32 d5;
    wp_f32 d6;
    wp_f32 vb;
    wp_f32 va;

    if( d1 <= 0.0f && d2 <= 0.0f )
    {
        return a;
    }

    bp = np_sub( point, b );
    d3 = np_dot( ab, bp );
    d4 = np_dot( ac, bp );
    if( d3 >= 0.0f && d4 <= d3 )
    {
        return b;
    }

    vc = d1 * d4 - d3 * d2;
    if( vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f )
    {
        return np_add( a, np_scale( ab, d1 / ( d1 - d3 ) ) );
    }

    cp = np_sub( point, c );
    d5 = np_dot( ab, cp );
    d6 = np_dot( ac, cp );
    if( d6 >= 0.0f && d5 <= d6 )
    {
        return c;
    }

    vb = d5 * d2 - d1 * d6;
    if( vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f )
    {
        return np_add( a, np_scale( ac, d2 / ( d2 - d6 ) ) );
    }

    va = d3 * d6 - d5 * d4;
    if( va <= 0.0f && ( d4 - d3 ) >= 0.0f && ( d5 - d6 ) >= 0.0f )
    {
        wp_vec3f bc = np_sub( c, b );
        return np_add( b, np_scale( bc, ( d4 - d3 ) / ( ( d4 - d3 ) + ( d5 - d6 ) ) ) );
    }

    {
        wp_f32 denominator = 1.0f / ( va + vb + vc );
        wp_f32 v = vb * denominator;
        wp_f32 w = vc * denominator;
        return np_add( a, np_add( np_scale( ab, v ), np_scale( ac, w ) ) );
    }
}

static void np_closest_segment_segment( wp_vec3f p1, wp_vec3f q1, wp_vec3f p2, wp_vec3f q2,
                                        wp_vec3f *point1, wp_vec3f *point2 )
{
    wp_vec3f d1 = np_sub( q1, p1 );
    wp_vec3f d2 = np_sub( q2, p2 );
    wp_vec3f r = np_sub( p1, p2 );
    wp_f32 a = np_dot( d1, d1 );
    wp_f32 e = np_dot( d2, d2 );
    wp_f32 f = np_dot( d2, r );
    wp_f32 s;
    wp_f32 t;

    if( a <= 1.0e-12f && e <= 1.0e-12f )
    {
        *point1 = p1;
        *point2 = p2;
        return;
    }
    if( a <= 1.0e-12f )
    {
        s = 0.0f;
        t = np_clampf( f / e, 0.0f, 1.0f );
    }
    else
    {
        wp_f32 c = np_dot( d1, r );
        if( e <= 1.0e-12f )
        {
            t = 0.0f;
            s = np_clampf( -c / a, 0.0f, 1.0f );
        }
        else
        {
            wp_f32 b = np_dot( d1, d2 );
            wp_f32 denominator = a * e - b * b;
            s = denominator != 0.0f ? np_clampf( ( b * f - c * e ) / denominator, 0.0f, 1.0f ) : 0.0f;
            t = ( b * s + f ) / e;
            if( t < 0.0f )
            {
                t = 0.0f;
                s = np_clampf( -c / a, 0.0f, 1.0f );
            }
            else if( t > 1.0f )
            {
                t = 1.0f;
                s = np_clampf( ( b - c ) / a, 0.0f, 1.0f );
            }
        }
    }

    *point1 = np_add( p1, np_scale( d1, s ) );
    *point2 = np_add( p2, np_scale( d2, t ) );
}

static wp_s32 np_segment_intersects_triangle( wp_vec3f start, wp_vec3f end, wp_vec3f a, wp_vec3f b,
                                              wp_vec3f c, wp_vec3f *intersection )
{
    wp_vec3f direction = np_sub( end, start );
    wp_vec3f edge1 = np_sub( b, a );
    wp_vec3f edge2 = np_sub( c, a );
    wp_vec3f p = np_cross( direction, edge2 );
    wp_f32 determinant = np_dot( edge1, p );
    wp_f32 inverse_determinant;
    wp_vec3f t;
    wp_f32 u;
    wp_vec3f q;
    wp_f32 v;
    wp_f32 distance;

    if( np_fabsf( determinant ) <= 1.0e-10f )
    {
        return 0;
    }
    inverse_determinant = 1.0f / determinant;
    t = np_sub( start, a );
    u = np_dot( t, p ) * inverse_determinant;
    if( u < 0.0f || u > 1.0f )
    {
        return 0;
    }
    q = np_cross( t, edge1 );
    v = np_dot( direction, q ) * inverse_determinant;
    if( v < 0.0f || u + v > 1.0f )
    {
        return 0;
    }
    distance = np_dot( edge2, q ) * inverse_determinant;
    if( distance < 0.0f || distance > 1.0f )
    {
        return 0;
    }
    if( intersection )
    {
        *intersection = np_add( start, np_scale( direction, distance ) );
    }
    return 1;
}

static void np_closest_segment_triangle( wp_vec3f start, wp_vec3f end, wp_vec3f a, wp_vec3f b,
                                         wp_vec3f c, wp_vec3f *on_segment, wp_vec3f *on_triangle )
{
    wp_vec3f intersection;
    wp_vec3f best_segment;
    wp_vec3f best_triangle;
    wp_f32 best_distance_sq;
    wp_vec3f candidate_segment;
    wp_vec3f candidate_triangle;
    wp_f32 candidate_distance_sq;
    const wp_vec3f edge_starts[3] = { a, b, c };
    const wp_vec3f edge_ends[3] = { b, c, a };
    wp_s32 edge;

    if( np_segment_intersects_triangle( start, end, a, b, c, &intersection ) )
    {
        *on_segment = intersection;
        *on_triangle = intersection;
        return;
    }

    best_segment = start;
    best_triangle = np_closest_point_triangle( start, a, b, c );
    best_distance_sq = np_len_sq( np_sub( best_triangle, best_segment ) );

    candidate_segment = end;
    candidate_triangle = np_closest_point_triangle( end, a, b, c );
    candidate_distance_sq = np_len_sq( np_sub( candidate_triangle, candidate_segment ) );
    if( candidate_distance_sq < best_distance_sq )
    {
        best_segment = candidate_segment;
        best_triangle = candidate_triangle;
        best_distance_sq = candidate_distance_sq;
    }

    for( edge = 0; edge < 3; ++edge )
    {
        np_closest_segment_segment( start, end, edge_starts[edge], edge_ends[edge], &candidate_segment,
                                    &candidate_triangle );
        candidate_distance_sq = np_len_sq( np_sub( candidate_triangle, candidate_segment ) );
        if( candidate_distance_sq < best_distance_sq )
        {
            best_segment = candidate_segment;
            best_triangle = candidate_triangle;
            best_distance_sq = candidate_distance_sq;
        }
    }

    *on_segment = best_segment;
    *on_triangle = best_triangle;
}

static wp_s32 np_triangle_box_axis_overlaps( wp_vec3f axis, wp_vec3f a, wp_vec3f b, wp_vec3f c,
                                             wp_vec3f half_extents, wp_f32 tolerance )
{
    wp_f32 length_sq = np_len_sq( axis );
    wp_f32 pa;
    wp_f32 pb;
    wp_f32 pc;
    wp_f32 minimum;
    wp_f32 maximum;
    wp_f32 radius;

    if( length_sq <= 1.0e-12f )
    {
        return 1;
    }

    axis = np_scale( axis, 1.0f / np_sqrtf( length_sq ) );
    pa = np_dot( a, axis );
    pb = np_dot( b, axis );
    pc = np_dot( c, axis );
    minimum = pa < pb ? pa : pb;
    minimum = minimum < pc ? minimum : pc;
    maximum = pa > pb ? pa : pb;
    maximum = maximum > pc ? maximum : pc;
    radius = np_fabsf( axis.x ) * half_extents.x + np_fabsf( axis.y ) * half_extents.y +
             np_fabsf( axis.z ) * half_extents.z;
    return !( minimum > radius + tolerance || maximum < -radius - tolerance );
}

static wp_s32 np_triangle_box_overlaps( wp_vec3f a, wp_vec3f b, wp_vec3f c, wp_vec3f half_extents,
                                        wp_f32 tolerance )
{
    const wp_vec3f box_axes[3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    wp_vec3f edges[3];
    wp_vec3f triangle_normal;
    wp_s32 edge;
    wp_s32 axis;

    edges[0] = np_sub( b, a );
    edges[1] = np_sub( c, b );
    edges[2] = np_sub( a, c );
    triangle_normal = np_cross( edges[0], np_sub( c, a ) );

    for( axis = 0; axis < 3; ++axis )
    {
        if( !np_triangle_box_axis_overlaps( box_axes[axis], a, b, c, half_extents, tolerance ) )
        {
            return 0;
        }
    }
    if( !np_triangle_box_axis_overlaps( triangle_normal, a, b, c, half_extents, tolerance ) )
    {
        return 0;
    }
    for( edge = 0; edge < 3; ++edge )
    {
        for( axis = 0; axis < 3; ++axis )
        {
            if( !np_triangle_box_axis_overlaps( np_cross( edges[edge], box_axes[axis] ), a, b, c,
                                                half_extents, tolerance ) )
            {
                return 0;
            }
        }
    }
    return 1;
}

/* =========================================================================
 * Manifold helpers
 * ====================================================================== */

static void manifold_clear( wp_contact_manifold *m )
{
    memset( m, 0, sizeof( wp_contact_manifold ) );
}

static void manifold_add_contact( wp_contact_manifold *m, wp_vec3f on_a, wp_vec3f on_b, wp_vec3f normal,
                                  wp_f32 depth )
{
    wp_contact_point *cp;

    if( m->contact_count >= WP_MANIFOLD_MAX_CONTACTS )
    {
        return;
    }

    cp = &m->contacts[m->contact_count++];
    cp->position_world_on_a = on_a;
    cp->position_world_on_b = on_b;
    cp->normal_world_on_b = normal;
    cp->penetration_depth = depth;
    cp->combined_friction =
        sqrtf( wp_rigidbody_get_friction( m->body_a ) * wp_rigidbody_get_friction( m->body_b ) );
    cp->combined_restitution =
        wp_rigidbody_get_restitution( m->body_a ) > wp_rigidbody_get_restitution( m->body_b )
            ? wp_rigidbody_get_restitution( m->body_a )
            : wp_rigidbody_get_restitution( m->body_b );

    {
        const wp_physics_material *material_a = wp_collision_shape_get_material( m->shape_a );
        const wp_physics_material *material_b = wp_collision_shape_get_material( m->shape_b );
        if( material_a || material_b )
        {
            const wp_f32 friction_a = material_a ? wp_physics_material_get_dynamic_friction( material_a )
                                                 : wp_rigidbody_get_friction( m->body_a );
            const wp_f32 friction_b = material_b ? wp_physics_material_get_dynamic_friction( material_b )
                                                 : wp_rigidbody_get_friction( m->body_b );
            const wp_f32 restitution_a = material_a ? wp_physics_material_get_restitution( material_a )
                                                    : wp_rigidbody_get_restitution( m->body_a );
            const wp_f32 restitution_b = material_b ? wp_physics_material_get_restitution( material_b )
                                                    : wp_rigidbody_get_restitution( m->body_b );
            cp->combined_friction = sqrtf( ( friction_a > 0.0f ? friction_a : 0.0f ) *
                                           ( friction_b > 0.0f ? friction_b : 0.0f ) );
            cp->combined_restitution = restitution_a > restitution_b ? restitution_a : restitution_b;
        }
    }

    m->is_touching = 1;
}

/*
 * Triangle meshes commonly contribute several triangles to one physical
 * contact (road surface, kerb, barrier). Preserve distinct contacts instead
 * of allowing one mesh triangle to replace the whole manifold. Near-identical
 * contacts from the two triangles of a quad are merged, and a full manifold
 * retains the deepest candidates.
 */
static void manifold_add_mesh_contact( wp_contact_manifold *m, wp_vec3f on_a, wp_vec3f on_b,
                                       wp_vec3f normal, wp_f32 depth )
{
    wp_s32 i;
    wp_s32 shallowest = -1;
    wp_f32 shallowest_depth = depth;

    for( i = 0; i < m->contact_count; ++i )
    {
        wp_contact_point *existing = &m->contacts[i];
        wp_vec3f point_delta = np_sub( existing->position_world_on_b, on_b );
        if( np_len_sq( point_delta ) <= 1.0e-4f &&
            np_dot( existing->normal_world_on_b, normal ) >= 0.98f )
        {
            if( depth > existing->penetration_depth )
            {
                const wp_s32 old_count = m->contact_count;
                m->contact_count = i;
                manifold_add_contact( m, on_a, on_b, normal, depth );
                m->contacts[i] = m->contacts[m->contact_count - 1];
                m->contact_count = old_count;
            }
            return;
        }

        if( shallowest < 0 || existing->penetration_depth < shallowest_depth )
        {
            shallowest = i;
            shallowest_depth = existing->penetration_depth;
        }
    }

    if( m->contact_count < WP_MANIFOLD_MAX_CONTACTS )
    {
        manifold_add_contact( m, on_a, on_b, normal, depth );
    }
    else if( shallowest >= 0 && depth > shallowest_depth )
    {
        const wp_s32 old_count = m->contact_count;
        m->contact_count = shallowest;
        manifold_add_contact( m, on_a, on_b, normal, depth );
        m->contacts[shallowest] = m->contacts[m->contact_count - 1];
        m->contact_count = old_count;
    }
}

/* =========================================================================
 * Primitive collision tests
 * ====================================================================== */

/* ----- sphere vs sphere ------------------------------------------------- */

static wp_s32 test_sphere_sphere( wp_vec3f pa, wp_f32 ra, wp_vec3f pb, wp_f32 rb, wp_f32 tolerance,
                                  wp_contact_manifold *out )
{
    wp_vec3f delta = np_sub( pb, pa );
    wp_f32 dist_sq = np_len_sq( delta );
    wp_f32 radsum = ra + rb;

    if( dist_sq > ( radsum + tolerance ) * ( radsum + tolerance ) )
    {
        return 0;
    }

    {
        wp_f32 dist = np_sqrtf( dist_sq );
        wp_vec3f normal = ( dist > 1e-8f ) ? np_scale( delta, 1.0f / dist ) : np_normalize( delta );
        wp_f32 depth = radsum - dist;
        wp_vec3f on_a = np_add( pa, np_scale( normal, ra ) );
        wp_vec3f on_b = np_add( pb, np_scale( normal, -rb ) );

        manifold_add_contact( out, on_a, on_b, normal, depth );
    }
    return 1;
}

/* ----- sphere vs plane -------------------------------------------------- */

static wp_s32 test_sphere_plane( wp_vec3f center, wp_f32 radius, wp_vec3f plane_normal,
                                 wp_f32 plane_offset, wp_f32 tolerance, wp_s32 sphere_is_a,
                                 wp_contact_manifold *out )
{
    wp_f32 dist = np_dot( plane_normal, center ) - plane_offset;
    wp_f32 depth = radius - dist;

    if( depth < -tolerance )
    {
        return 0;
    }

    {
        wp_vec3f on_plane = np_sub( center, np_scale( plane_normal, dist ) );
        wp_vec3f on_sphere = np_sub( center, np_scale( plane_normal, radius ) );
        if( sphere_is_a )
        {
            manifold_add_contact( out, on_sphere, on_plane, np_scale( plane_normal, -1.0f ), depth );
        }
        else
        {
            manifold_add_contact( out, on_plane, on_sphere, plane_normal, depth );
        }
    }
    return 1;
}

/* ----- oriented box vs plane -------------------------------------------- */

static wp_s32 test_box_plane( wp_vec3f center, wp_quatf orientation, wp_vec3f half_extents,
                              wp_vec3f plane_normal, wp_f32 plane_offset, wp_f32 tolerance,
                              wp_s32 box_is_a, wp_contact_manifold *out )
{
    wp_vec3f axis_x = { 1.0f, 0.0f, 0.0f };
    wp_vec3f axis_y = { 0.0f, 1.0f, 0.0f };
    wp_vec3f axis_z = { 0.0f, 0.0f, 1.0f };
    wp_f32 r;

    axis_x = np_quat_rotate( orientation, axis_x );
    axis_y = np_quat_rotate( orientation, axis_y );
    axis_z = np_quat_rotate( orientation, axis_z );
    r = np_fabsf( np_dot( axis_x, plane_normal ) ) * half_extents.x +
        np_fabsf( np_dot( axis_y, plane_normal ) ) * half_extents.y +
        np_fabsf( np_dot( axis_z, plane_normal ) ) * half_extents.z;

    wp_f32 dist = np_dot( plane_normal, center ) - plane_offset;
    wp_f32 depth = r - dist;

    if( depth < -tolerance )
    {
        return 0;
    }

    {
        wp_vec3f on_plane = np_sub( center, np_scale( plane_normal, dist ) );
        wp_vec3f on_box = np_sub( center, np_scale( plane_normal, r ) );
        if( box_is_a )
        {
            manifold_add_contact( out, on_box, on_plane, np_scale( plane_normal, -1.0f ), depth );
        }
        else
        {
            manifold_add_contact( out, on_plane, on_box, plane_normal, depth );
        }
    }
    return 1;
}

/* ----- sphere vs box (oriented box) ------------------------------------- */

static wp_s32 test_sphere_box( wp_vec3f sphere_center, wp_f32 radius, wp_vec3f box_center,
                               wp_quatf box_orientation, wp_vec3f half_extents, wp_f32 tolerance,
                               wp_s32 sphere_is_a, wp_contact_manifold *out )
{
    wp_quatf inverse_orientation = np_quat_conjugate( box_orientation );
    wp_vec3f local = np_quat_rotate( inverse_orientation, np_sub( sphere_center, box_center ) );
    wp_vec3f clamped;
    wp_vec3f closest;
    wp_f32 distance_sq;
    wp_f32 depth;
    wp_vec3f normal;
    wp_vec3f on_sphere;

    clamped.x = np_clampf( local.x, -half_extents.x, half_extents.x );
    clamped.y = np_clampf( local.y, -half_extents.y, half_extents.y );
    clamped.z = np_clampf( local.z, -half_extents.z, half_extents.z );
    closest = np_add( box_center, np_quat_rotate( box_orientation, clamped ) );
    distance_sq = np_len_sq( np_sub( closest, sphere_center ) );

    if( distance_sq > ( radius + tolerance ) * ( radius + tolerance ) )
    {
        return 0;
    }

    {
        wp_f32 distance = np_sqrtf( distance_sq );
        if( distance > 1.0e-8f )
        {
            normal = np_scale( np_sub( closest, sphere_center ), 1.0f / distance );
            depth = radius - distance;
        }
        else
        {
            wp_f32 face_distance = half_extents.x - np_fabsf( local.x );
            wp_vec3f local_normal = np_zero();
            local_normal.x = local.x >= 0.0f ? 1.0f : -1.0f;
            if( half_extents.y - np_fabsf( local.y ) < face_distance )
            {
                face_distance = half_extents.y - np_fabsf( local.y );
                local_normal = np_zero();
                local_normal.y = local.y >= 0.0f ? 1.0f : -1.0f;
            }
            if( half_extents.z - np_fabsf( local.z ) < face_distance )
            {
                face_distance = half_extents.z - np_fabsf( local.z );
                local_normal = np_zero();
                local_normal.z = local.z >= 0.0f ? 1.0f : -1.0f;
            }
            /*
             * The center is inside the box. The solver normal points into
             * the box so subtracting it ejects the sphere through the
             * nearest face.
             */
            normal = np_quat_rotate( box_orientation, np_scale( local_normal, -1.0f ) );
            depth = radius + face_distance;
            clamped = np_add( local, np_scale( local_normal, face_distance ) );
            closest = np_add( box_center, np_quat_rotate( box_orientation, clamped ) );
        }
    }

    on_sphere = np_add( sphere_center, np_scale( normal, radius ) );
    if( sphere_is_a )
    {
        manifold_add_contact( out, on_sphere, closest, normal, depth );
    }
    else
    {
        manifold_add_contact( out, closest, on_sphere, np_scale( normal, -1.0f ), depth );
    }
    return 1;
}

/* ----- box vs box (oriented SAT, 15 separating axes) -------------------- */

static wp_s32 np_obb_axis_overlap( wp_vec3f axis, wp_vec3f center_delta, const wp_vec3f axes_a[3],
                                   wp_vec3f half_a, const wp_vec3f axes_b[3], wp_vec3f half_b,
                                   wp_f32 tolerance, wp_f32 *minimum_overlap, wp_vec3f *minimum_axis )
{
    wp_f32 length_sq = np_len_sq( axis );
    wp_f32 radius_a;
    wp_f32 radius_b;
    wp_f32 center_distance;
    wp_f32 overlap;

    if( length_sq <= 1.0e-12f )
    {
        return 1;
    }

    axis = np_scale( axis, 1.0f / np_sqrtf( length_sq ) );
    radius_a = np_fabsf( np_dot( axis, axes_a[0] ) ) * half_a.x +
               np_fabsf( np_dot( axis, axes_a[1] ) ) * half_a.y +
               np_fabsf( np_dot( axis, axes_a[2] ) ) * half_a.z;
    radius_b = np_fabsf( np_dot( axis, axes_b[0] ) ) * half_b.x +
               np_fabsf( np_dot( axis, axes_b[1] ) ) * half_b.y +
               np_fabsf( np_dot( axis, axes_b[2] ) ) * half_b.z;
    center_distance = np_dot( center_delta, axis );
    overlap = radius_a + radius_b - np_fabsf( center_distance );
    if( overlap < -tolerance )
    {
        return 0;
    }
    if( overlap < *minimum_overlap )
    {
        *minimum_overlap = overlap;
        *minimum_axis = center_distance >= 0.0f ? axis : np_scale( axis, -1.0f );
    }
    return 1;
}

static wp_s32 test_box_box( wp_vec3f center_a, wp_quatf orientation_a, wp_vec3f half_a,
                            wp_vec3f center_b, wp_quatf orientation_b, wp_vec3f half_b, wp_f32 tolerance,
                            wp_contact_manifold *out )
{
    const wp_vec3f local_axes[3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    wp_vec3f axes_a[3];
    wp_vec3f axes_b[3];
    wp_vec3f center_delta = np_sub( center_b, center_a );
    wp_f32 minimum_overlap = 3.402823466e+38F;
    wp_vec3f normal = np_zero();
    wp_s32 i;
    wp_s32 j;

    for( i = 0; i < 3; ++i )
    {
        axes_a[i] = np_quat_rotate( orientation_a, local_axes[i] );
        axes_b[i] = np_quat_rotate( orientation_b, local_axes[i] );
    }
    for( i = 0; i < 3; ++i )
    {
        if( !np_obb_axis_overlap( axes_a[i], center_delta, axes_a, half_a, axes_b, half_b, tolerance,
                                  &minimum_overlap, &normal ) ||
            !np_obb_axis_overlap( axes_b[i], center_delta, axes_a, half_a, axes_b, half_b, tolerance,
                                  &minimum_overlap, &normal ) )
        {
            return 0;
        }
    }
    for( i = 0; i < 3; ++i )
    {
        for( j = 0; j < 3; ++j )
        {
            if( !np_obb_axis_overlap( np_cross( axes_a[i], axes_b[j] ), center_delta, axes_a, half_a,
                                      axes_b, half_b, tolerance, &minimum_overlap, &normal ) )
            {
                return 0;
            }
        }
    }

    {
        wp_f32 radius_a = np_fabsf( np_dot( normal, axes_a[0] ) ) * half_a.x +
                          np_fabsf( np_dot( normal, axes_a[1] ) ) * half_a.y +
                          np_fabsf( np_dot( normal, axes_a[2] ) ) * half_a.z;
        wp_f32 radius_b = np_fabsf( np_dot( normal, axes_b[0] ) ) * half_b.x +
                          np_fabsf( np_dot( normal, axes_b[1] ) ) * half_b.y +
                          np_fabsf( np_dot( normal, axes_b[2] ) ) * half_b.z;
        wp_vec3f on_a = np_add( center_a, np_scale( normal, radius_a ) );
        wp_vec3f on_b = np_sub( center_b, np_scale( normal, radius_b ) );
        manifold_add_contact( out, on_a, on_b, normal, minimum_overlap > 0.0f ? minimum_overlap : 0.0f );
    }
    return 1;
}

/* ----- capsule vs plane ------------------------------------------------- */

static void np_capsule_endpoints( wp_vec3f center, wp_quatf orientation, wp_f32 half_height,
                                  wp_vec3f *start, wp_vec3f *end )
{
    wp_vec3f axis = { 0.0f, half_height, 0.0f };
    axis = np_quat_rotate( orientation, axis );
    *start = np_sub( center, axis );
    *end = np_add( center, axis );
}

static wp_s32 test_capsule_plane( wp_vec3f center, wp_quatf orientation, wp_f32 radius,
                                  wp_f32 half_height, wp_vec3f plane_normal, wp_f32 plane_offset,
                                  wp_f32 tolerance, wp_s32 capsule_is_a, wp_contact_manifold *out )
{
    /*
     * Test both hemisphere centres against the plane, treating each as a
     * sphere.  This gives up to two contact points.
     */
    wp_vec3f top, bot;
    wp_s32 hit = 0;

    np_capsule_endpoints( center, orientation, half_height, &bot, &top );

    hit |= test_sphere_plane( top, radius, plane_normal, plane_offset, tolerance, capsule_is_a, out );
    hit |= test_sphere_plane( bot, radius, plane_normal, plane_offset, tolerance, capsule_is_a, out );
    return hit;
}

/* ----- capsule vs sphere ------------------------------------------------ */

static wp_s32 test_capsule_sphere( wp_vec3f cap_center, wp_quatf cap_orientation, wp_f32 cap_radius,
                                   wp_f32 half_height, wp_vec3f sphere_center, wp_f32 sphere_radius,
                                   wp_f32 tolerance, wp_s32 capsule_is_a, wp_contact_manifold *out )
{
    /* Closest point on capsule segment to sphere center */
    wp_vec3f seg_start, seg_end, seg_dir, to_sphere;
    wp_f32 t, seg_len_sq;
    wp_vec3f closest;

    np_capsule_endpoints( cap_center, cap_orientation, half_height, &seg_start, &seg_end );

    seg_dir = np_sub( seg_end, seg_start );
    seg_len_sq = np_len_sq( seg_dir );
    to_sphere = np_sub( sphere_center, seg_start );

    t = ( seg_len_sq > 1e-8f ) ? np_clampf( np_dot( to_sphere, seg_dir ) / seg_len_sq, 0.0f, 1.0f )
                               : 0.0f;
    closest = np_add( seg_start, np_scale( seg_dir, t ) );

    {
        wp_vec3f delta = np_sub( sphere_center, closest );
        wp_f32 distance_sq = np_len_sq( delta );
        wp_f32 radius_sum = cap_radius + sphere_radius;
        wp_f32 distance;
        wp_vec3f normal;
        wp_vec3f on_capsule;
        wp_vec3f on_sphere;

        if( distance_sq > ( radius_sum + tolerance ) * ( radius_sum + tolerance ) )
        {
            return 0;
        }
        distance = np_sqrtf( distance_sq );
        normal = distance > 1.0e-8f ? np_scale( delta, 1.0f / distance )
                                    : np_normalize( np_sub( sphere_center, cap_center ) );
        on_capsule = np_add( closest, np_scale( normal, cap_radius ) );
        on_sphere = np_sub( sphere_center, np_scale( normal, sphere_radius ) );
        if( capsule_is_a )
        {
            manifold_add_contact( out, on_capsule, on_sphere, normal, radius_sum - distance );
        }
        else
        {
            manifold_add_contact( out, on_sphere, on_capsule, np_scale( normal, -1.0f ),
                                  radius_sum - distance );
        }
        return 1;
    }
}

/* ----- capsule vs capsule / box ---------------------------------------- */

static wp_s32 test_capsule_capsule( wp_vec3f center_a, wp_quatf orientation_a, wp_f32 radius_a,
                                    wp_f32 half_height_a, wp_vec3f center_b, wp_quatf orientation_b,
                                    wp_f32 radius_b, wp_f32 half_height_b, wp_f32 tolerance,
                                    wp_contact_manifold *out )
{
    wp_vec3f start_a;
    wp_vec3f end_a;
    wp_vec3f start_b;
    wp_vec3f end_b;
    wp_vec3f closest_a;
    wp_vec3f closest_b;
    np_capsule_endpoints( center_a, orientation_a, half_height_a, &start_a, &end_a );
    np_capsule_endpoints( center_b, orientation_b, half_height_b, &start_b, &end_b );
    np_closest_segment_segment( start_a, end_a, start_b, end_b, &closest_a, &closest_b );
    return test_sphere_sphere( closest_a, radius_a, closest_b, radius_b, tolerance, out );
}

static wp_f32 np_point_aabb_distance_sq( wp_vec3f point, wp_vec3f half_extents, wp_vec3f *closest )
{
    closest->x = np_clampf( point.x, -half_extents.x, half_extents.x );
    closest->y = np_clampf( point.y, -half_extents.y, half_extents.y );
    closest->z = np_clampf( point.z, -half_extents.z, half_extents.z );
    return np_len_sq( np_sub( point, *closest ) );
}

static wp_s32 test_capsule_box( wp_vec3f capsule_center, wp_quatf capsule_orientation,
                                wp_f32 capsule_radius, wp_f32 capsule_half_height, wp_vec3f box_center,
                                wp_quatf box_orientation, wp_vec3f box_half_extents, wp_f32 tolerance,
                                wp_s32 capsule_is_a, wp_contact_manifold *out )
{
    wp_vec3f segment_start;
    wp_vec3f segment_end;
    wp_quatf inverse_box_orientation = np_quat_conjugate( box_orientation );
    wp_vec3f local_start;
    wp_vec3f local_end;
    wp_vec3f segment_direction;
    wp_f32 low = 0.0f;
    wp_f32 high = 1.0f;
    wp_s32 iteration;
    wp_f32 t;
    wp_vec3f on_segment_local;
    wp_vec3f on_box_local;
    wp_f32 distance_sq;
    wp_f32 distance;
    wp_f32 depth;
    wp_vec3f normal_local;
    wp_vec3f normal_world;
    wp_vec3f on_capsule_world;
    wp_vec3f on_box_world;

    np_capsule_endpoints( capsule_center, capsule_orientation, capsule_half_height, &segment_start,
                          &segment_end );
    local_start = np_quat_rotate( inverse_box_orientation, np_sub( segment_start, box_center ) );
    local_end = np_quat_rotate( inverse_box_orientation, np_sub( segment_end, box_center ) );
    segment_direction = np_sub( local_end, local_start );

    /* Distance from a segment to an AABB is convex over the segment. */
    for( iteration = 0; iteration < 32; ++iteration )
    {
        wp_f32 first = low + ( high - low ) / 3.0f;
        wp_f32 second = high - ( high - low ) / 3.0f;
        wp_vec3f first_point = np_add( local_start, np_scale( segment_direction, first ) );
        wp_vec3f second_point = np_add( local_start, np_scale( segment_direction, second ) );
        wp_vec3f unused;
        wp_f32 first_distance = np_point_aabb_distance_sq( first_point, box_half_extents, &unused );
        wp_f32 second_distance = np_point_aabb_distance_sq( second_point, box_half_extents, &unused );
        if( first_distance < second_distance )
        {
            high = second;
        }
        else
        {
            low = first;
        }
    }

    t = ( low + high ) * 0.5f;
    on_segment_local = np_add( local_start, np_scale( segment_direction, t ) );
    distance_sq = np_point_aabb_distance_sq( on_segment_local, box_half_extents, &on_box_local );
    if( distance_sq > ( capsule_radius + tolerance ) * ( capsule_radius + tolerance ) )
    {
        return 0;
    }

    distance = np_sqrtf( distance_sq );
    if( distance > 1.0e-7f )
    {
        normal_local = np_scale( np_sub( on_box_local, on_segment_local ), 1.0f / distance );
        depth = capsule_radius - distance;
    }
    else
    {
        wp_f32 face_distance = box_half_extents.x - np_fabsf( on_segment_local.x );
        wp_vec3f face_direction = np_zero();
        face_direction.x = on_segment_local.x >= 0.0f ? 1.0f : -1.0f;
        if( box_half_extents.y - np_fabsf( on_segment_local.y ) < face_distance )
        {
            face_distance = box_half_extents.y - np_fabsf( on_segment_local.y );
            face_direction = np_zero();
            face_direction.y = on_segment_local.y >= 0.0f ? 1.0f : -1.0f;
        }
        if( box_half_extents.z - np_fabsf( on_segment_local.z ) < face_distance )
        {
            face_distance = box_half_extents.z - np_fabsf( on_segment_local.z );
            face_direction = np_zero();
            face_direction.z = on_segment_local.z >= 0.0f ? 1.0f : -1.0f;
        }
        depth = capsule_radius + face_distance;
        on_box_local = np_add( on_segment_local, np_scale( face_direction, face_distance ) );
        normal_local = np_scale( face_direction, -1.0f );
    }

    normal_world = np_quat_rotate( box_orientation, normal_local );
    on_capsule_world = np_add( np_add( np_quat_rotate( box_orientation, on_segment_local ), box_center ),
                               np_scale( normal_world, capsule_radius ) );
    on_box_world = np_add( np_quat_rotate( box_orientation, on_box_local ), box_center );
    if( capsule_is_a )
    {
        manifold_add_contact( out, on_capsule_world, on_box_world, normal_world, depth );
    }
    else
    {
        manifold_add_contact( out, on_box_world, on_capsule_world, np_scale( normal_world, -1.0f ),
                              depth );
    }
    return 1;
}

/* ----- primitive vs triangle mesh -------------------------------------- */

static wp_s32 test_sphere_mesh( wp_vec3f center, wp_f32 radius, const wp_rigidbody *mesh_body,
                                const wp_collision_shape *mesh_shape, wp_f32 tolerance,
                                wp_s32 primitive_is_a, wp_contact_manifold *out )
{
    const wp_collision_mesh_data *mesh = wp_collision_shape_get_mesh_data( mesh_shape );
    wp_u32 candidates[WP_NP_MESH_QUERY_CAPACITY];
    wp_u32 candidate_count;
    wp_u32 query_index;
    wp_s32 using_candidates;

    if( !mesh || !mesh->vertices || !mesh->indices )
    {
        return 0;
    }

    candidate_count = np_mesh_sphere_candidates( mesh_body, mesh_shape, center, radius + tolerance,
                                                 mesh->triangle_count, candidates,
                                                 WP_NP_MESH_QUERY_CAPACITY, &using_candidates );

    for( query_index = 0u; query_index < candidate_count; ++query_index )
    {
        const wp_u32 triangle = using_candidates ? candidates[query_index] : query_index;
        wp_vec3f a;
        wp_vec3f b;
        wp_vec3f c;
        wp_vec3f closest;
        wp_vec3f to_mesh;
        wp_f32 distance_sq;
        wp_f32 distance;
        wp_f32 depth;
        wp_vec3f normal;
        wp_vec3f on_sphere;

        if( !np_mesh_triangle_world( mesh_body, mesh_shape, triangle, &a, &b, &c ) )
        {
            continue;
        }

        closest = np_closest_point_triangle( center, a, b, c );
        to_mesh = np_sub( closest, center );
        distance_sq = np_len_sq( to_mesh );
        if( distance_sq > ( radius + tolerance ) * ( radius + tolerance ) )
        {
            continue;
        }

        distance = np_sqrtf( distance_sq );
        depth = radius - distance;
        if( distance > 1.0e-7f )
        {
            normal = np_scale( to_mesh, 1.0f / distance );
        }
        else
        {
            normal = np_normalize( np_cross( np_sub( b, a ), np_sub( c, a ) ) );
            if( np_dot( normal, np_sub( center, a ) ) >= 0.0f )
            {
                normal = np_scale( normal, -1.0f );
            }
        }

        on_sphere = np_add( center, np_scale( normal, radius ) );
        if( primitive_is_a )
        {
            manifold_add_mesh_contact( out, on_sphere, closest, normal, depth );
        }
        else
        {
            manifold_add_mesh_contact( out, closest, on_sphere, np_scale( normal, -1.0f ), depth );
        }
    }
    return out->contact_count > 0;
}

static wp_s32 test_box_mesh( wp_vec3f box_center, wp_quatf box_orientation, wp_vec3f half_extents,
                             const wp_rigidbody *mesh_body, const wp_collision_shape *mesh_shape,
                             wp_f32 tolerance, wp_s32 primitive_is_a, wp_contact_manifold *out )
{
    const wp_collision_mesh_data *mesh = wp_collision_shape_get_mesh_data( mesh_shape );
    wp_quatf inverse_box_orientation = np_quat_conjugate( box_orientation );
    wp_u32 candidates[WP_NP_MESH_QUERY_CAPACITY];
    wp_u32 candidate_count;
    wp_u32 query_index;
    wp_s32 using_candidates;
    wp_f32 query_radius;

    if( !mesh || !mesh->vertices || !mesh->indices )
    {
        return 0;
    }

    query_radius = np_sqrtf( half_extents.x * half_extents.x + half_extents.y * half_extents.y +
                             half_extents.z * half_extents.z ) +
                   tolerance;
    candidate_count =
        np_mesh_sphere_candidates( mesh_body, mesh_shape, box_center, query_radius, mesh->triangle_count,
                                   candidates, WP_NP_MESH_QUERY_CAPACITY, &using_candidates );

    for( query_index = 0u; query_index < candidate_count; ++query_index )
    {
        const wp_u32 triangle = using_candidates ? candidates[query_index] : query_index;
        wp_vec3f world_a;
        wp_vec3f world_b;
        wp_vec3f world_c;
        wp_vec3f a;
        wp_vec3f b;
        wp_vec3f c;
        wp_vec3f normal;
        wp_f32 normal_length;
        wp_f32 signed_distance;
        wp_f32 support_radius;
        wp_f32 depth;
        wp_vec3f closest;
        wp_vec3f on_box;
        wp_vec3f normal_world;
        wp_vec3f on_box_world;
        wp_vec3f on_mesh_world;

        if( !np_mesh_triangle_world( mesh_body, mesh_shape, triangle, &world_a, &world_b, &world_c ) )
        {
            continue;
        }

        a = np_quat_rotate( inverse_box_orientation, np_sub( world_a, box_center ) );
        b = np_quat_rotate( inverse_box_orientation, np_sub( world_b, box_center ) );
        c = np_quat_rotate( inverse_box_orientation, np_sub( world_c, box_center ) );
        if( !np_triangle_box_overlaps( a, b, c, half_extents, tolerance ) )
        {
            continue;
        }

        normal = np_cross( np_sub( b, a ), np_sub( c, a ) );
        normal_length = np_len( normal );
        if( normal_length <= 1.0e-8f )
        {
            continue;
        }
        normal = np_scale( normal, 1.0f / normal_length );
        signed_distance = -np_dot( normal, a );
        support_radius = np_fabsf( normal.x ) * half_extents.x + np_fabsf( normal.y ) * half_extents.y +
                         np_fabsf( normal.z ) * half_extents.z;
        depth = support_radius - np_fabsf( signed_distance );

        /* Point the response normal from the box toward the mesh surface. */
        if( signed_distance >= 0.0f )
        {
            normal = np_scale( normal, -1.0f );
        }
        closest = np_closest_point_triangle( np_zero(), a, b, c );
        on_box.x = np_clampf( closest.x, -half_extents.x, half_extents.x );
        on_box.y = np_clampf( closest.y, -half_extents.y, half_extents.y );
        on_box.z = np_clampf( closest.z, -half_extents.z, half_extents.z );

        normal_world = np_quat_rotate( box_orientation, normal );
        on_box_world = np_add( np_quat_rotate( box_orientation, on_box ), box_center );
        on_mesh_world = np_add( np_quat_rotate( box_orientation, closest ), box_center );
        if( primitive_is_a )
        {
            manifold_add_mesh_contact( out, on_box_world, on_mesh_world, normal_world, depth );
        }
        else
        {
            manifold_add_mesh_contact( out, on_mesh_world, on_box_world, np_scale( normal_world, -1.0f ),
                                       depth );
        }
    }
    return out->contact_count > 0;
}

static wp_s32 test_capsule_mesh( wp_vec3f capsule_center, wp_quatf capsule_orientation,
                                 wp_f32 capsule_radius, wp_f32 capsule_half_height,
                                 const wp_rigidbody *mesh_body, const wp_collision_shape *mesh_shape,
                                 wp_f32 tolerance, wp_s32 primitive_is_a, wp_contact_manifold *out )
{
    const wp_collision_mesh_data *mesh = wp_collision_shape_get_mesh_data( mesh_shape );
    wp_vec3f segment_start;
    wp_vec3f segment_end;
    wp_u32 candidates[WP_NP_MESH_QUERY_CAPACITY];
    wp_u32 candidate_count;
    wp_u32 query_index;
    wp_s32 using_candidates;

    if( !mesh || !mesh->vertices || !mesh->indices )
    {
        return 0;
    }

    np_capsule_endpoints( capsule_center, capsule_orientation, capsule_half_height, &segment_start,
                          &segment_end );
    candidate_count = np_mesh_sphere_candidates(
        mesh_body, mesh_shape, capsule_center, capsule_half_height + capsule_radius + tolerance,
        mesh->triangle_count, candidates, WP_NP_MESH_QUERY_CAPACITY, &using_candidates );

    for( query_index = 0u; query_index < candidate_count; ++query_index )
    {
        const wp_u32 triangle = using_candidates ? candidates[query_index] : query_index;
        wp_vec3f a;
        wp_vec3f b;
        wp_vec3f c;
        wp_vec3f on_segment;
        wp_vec3f on_mesh;
        wp_vec3f to_mesh;
        wp_f32 distance_sq;
        wp_f32 distance;
        wp_f32 depth;
        wp_vec3f normal;
        wp_vec3f on_capsule;

        if( !np_mesh_triangle_world( mesh_body, mesh_shape, triangle, &a, &b, &c ) )
        {
            continue;
        }
        np_closest_segment_triangle( segment_start, segment_end, a, b, c, &on_segment, &on_mesh );
        to_mesh = np_sub( on_mesh, on_segment );
        distance_sq = np_len_sq( to_mesh );
        if( distance_sq > ( capsule_radius + tolerance ) * ( capsule_radius + tolerance ) )
        {
            continue;
        }

        distance = np_sqrtf( distance_sq );
        depth = capsule_radius - distance;
        if( distance > 1.0e-7f )
        {
            normal = np_scale( to_mesh, 1.0f / distance );
        }
        else
        {
            normal = np_normalize( np_cross( np_sub( b, a ), np_sub( c, a ) ) );
            if( np_dot( normal, np_sub( on_segment, a ) ) >= 0.0f )
            {
                normal = np_scale( normal, -1.0f );
            }
        }

        on_capsule = np_add( on_segment, np_scale( normal, capsule_radius ) );
        if( primitive_is_a )
        {
            manifold_add_mesh_contact( out, on_capsule, on_mesh, normal, depth );
        }
        else
        {
            manifold_add_mesh_contact( out, on_mesh, on_capsule, np_scale( normal, -1.0f ), depth );
        }
    }
    return out->contact_count > 0;
}

/* =========================================================================
 * Shape-pair dispatch
 * ====================================================================== */

static wp_s32 dispatch_pair( wp_narrowphase *np, wp_rigidbody *body_a, wp_collision_shape *shape_a,
                             wp_rigidbody *body_b, wp_collision_shape *shape_b,
                             wp_contact_manifold *out )
{
    wp_collision_shape_type ta, tb;
    wp_vec3f pa, pb;
    wp_f32 tol;

    manifold_clear( out );
    out->body_a = body_a;
    out->body_b = body_b;
    out->shape_a = shape_a;
    out->shape_b = shape_b;

    if( !shape_a || !shape_b )
    {
        return 0;
    }

    ta = wp_collision_shape_get_type( shape_a );
    tb = wp_collision_shape_get_type( shape_b );
    pa = np_shape_position( body_a, shape_a );
    pb = np_shape_position( body_b, shape_b );
    tol = np->contact_tolerance;

    /* Sphere vs Sphere */
    if( ta == WORKPHONE_COLLISION_SHAPE_SPHERE && tb == WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
        return test_sphere_sphere( pa, wp_collision_shape_get_sphere_radius( shape_a ), pb,
                                   wp_collision_shape_get_sphere_radius( shape_b ), tol, out );
    }

    /* Sphere vs Plane */
    if( ta == WORKPHONE_COLLISION_SHAPE_SPHERE && tb == WORKPHONE_COLLISION_SHAPE_PLANE )
    {
        return test_sphere_plane( pa, wp_collision_shape_get_sphere_radius( shape_a ),
                                  np_plane_normal( body_b, shape_b ), np_plane_offset( body_b, shape_b ),
                                  tol, 1, out );
    }

    /* Plane vs Sphere (swap) */
    if( ta == WORKPHONE_COLLISION_SHAPE_PLANE && tb == WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
        return test_sphere_plane( pb, wp_collision_shape_get_sphere_radius( shape_b ),
                                  np_plane_normal( body_a, shape_a ), np_plane_offset( body_a, shape_a ),
                                  tol, 0, out );
    }

    /* Sphere vs Box */
    if( ta == WORKPHONE_COLLISION_SHAPE_SPHERE && tb == WORKPHONE_COLLISION_SHAPE_BOX )
    {
        return test_sphere_box( pa, wp_collision_shape_get_sphere_radius( shape_a ), pb,
                                np_shape_orientation( body_b, shape_b ),
                                wp_collision_shape_get_box_half_extents( shape_b ), tol, 1, out );
    }

    /* Box vs Sphere (swap) */
    if( ta == WORKPHONE_COLLISION_SHAPE_BOX && tb == WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
        return test_sphere_box( pb, wp_collision_shape_get_sphere_radius( shape_b ), pa,
                                np_shape_orientation( body_a, shape_a ),
                                wp_collision_shape_get_box_half_extents( shape_a ), tol, 0, out );
    }

    /* Box vs Box */
    if( ta == WORKPHONE_COLLISION_SHAPE_BOX && tb == WORKPHONE_COLLISION_SHAPE_BOX )
    {
        return test_box_box( pa, np_shape_orientation( body_a, shape_a ),
                             wp_collision_shape_get_box_half_extents( shape_a ), pb,
                             np_shape_orientation( body_b, shape_b ),
                             wp_collision_shape_get_box_half_extents( shape_b ), tol, out );
    }

    /* Box vs Plane */
    if( ta == WORKPHONE_COLLISION_SHAPE_BOX && tb == WORKPHONE_COLLISION_SHAPE_PLANE )
    {
        return test_box_plane( pa, np_shape_orientation( body_a, shape_a ),
                               wp_collision_shape_get_box_half_extents( shape_a ),
                               np_plane_normal( body_b, shape_b ), np_plane_offset( body_b, shape_b ),
                               tol, 1, out );
    }

    /* Plane vs Box (swap) */
    if( ta == WORKPHONE_COLLISION_SHAPE_PLANE && tb == WORKPHONE_COLLISION_SHAPE_BOX )
    {
        return test_box_plane( pb, np_shape_orientation( body_b, shape_b ),
                               wp_collision_shape_get_box_half_extents( shape_b ),
                               np_plane_normal( body_a, shape_a ), np_plane_offset( body_a, shape_a ),
                               tol, 0, out );
    }

    /* Capsule vs Plane */
    if( ta == WORKPHONE_COLLISION_SHAPE_CAPSULE && tb == WORKPHONE_COLLISION_SHAPE_PLANE )
    {
        return test_capsule_plane( pa, np_shape_orientation( body_a, shape_a ),
                                   wp_collision_shape_get_capsule_radius( shape_a ),
                                   wp_collision_shape_get_capsule_half_height( shape_a ),
                                   np_plane_normal( body_b, shape_b ),
                                   np_plane_offset( body_b, shape_b ), tol, 1, out );
    }

    /* Plane vs Capsule (swap) */
    if( ta == WORKPHONE_COLLISION_SHAPE_PLANE && tb == WORKPHONE_COLLISION_SHAPE_CAPSULE )
    {
        return test_capsule_plane( pb, np_shape_orientation( body_b, shape_b ),
                                   wp_collision_shape_get_capsule_radius( shape_b ),
                                   wp_collision_shape_get_capsule_half_height( shape_b ),
                                   np_plane_normal( body_a, shape_a ),
                                   np_plane_offset( body_a, shape_a ), tol, 0, out );
    }

    /* Capsule vs Sphere */
    if( ta == WORKPHONE_COLLISION_SHAPE_CAPSULE && tb == WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
        return test_capsule_sphere( pa, np_shape_orientation( body_a, shape_a ),
                                    wp_collision_shape_get_capsule_radius( shape_a ),
                                    wp_collision_shape_get_capsule_half_height( shape_a ), pb,
                                    wp_collision_shape_get_sphere_radius( shape_b ), tol, 1, out );
    }

    /* Sphere vs Capsule (swap) */
    if( ta == WORKPHONE_COLLISION_SHAPE_SPHERE && tb == WORKPHONE_COLLISION_SHAPE_CAPSULE )
    {
        return test_capsule_sphere( pb, np_shape_orientation( body_b, shape_b ),
                                    wp_collision_shape_get_capsule_radius( shape_b ),
                                    wp_collision_shape_get_capsule_half_height( shape_b ), pa,
                                    wp_collision_shape_get_sphere_radius( shape_a ), tol, 0, out );
    }

    /* Capsule vs Capsule */
    if( ta == WORKPHONE_COLLISION_SHAPE_CAPSULE && tb == WORKPHONE_COLLISION_SHAPE_CAPSULE )
    {
        return test_capsule_capsule( pa, np_shape_orientation( body_a, shape_a ),
                                     wp_collision_shape_get_capsule_radius( shape_a ),
                                     wp_collision_shape_get_capsule_half_height( shape_a ), pb,
                                     np_shape_orientation( body_b, shape_b ),
                                     wp_collision_shape_get_capsule_radius( shape_b ),
                                     wp_collision_shape_get_capsule_half_height( shape_b ), tol, out );
    }

    /* Capsule vs Box */
    if( ta == WORKPHONE_COLLISION_SHAPE_CAPSULE && tb == WORKPHONE_COLLISION_SHAPE_BOX )
    {
        return test_capsule_box( pa, np_shape_orientation( body_a, shape_a ),
                                 wp_collision_shape_get_capsule_radius( shape_a ),
                                 wp_collision_shape_get_capsule_half_height( shape_a ), pb,
                                 np_shape_orientation( body_b, shape_b ),
                                 wp_collision_shape_get_box_half_extents( shape_b ), tol, 1, out );
    }

    /* Box vs Capsule */
    if( ta == WORKPHONE_COLLISION_SHAPE_BOX && tb == WORKPHONE_COLLISION_SHAPE_CAPSULE )
    {
        return test_capsule_box( pb, np_shape_orientation( body_b, shape_b ),
                                 wp_collision_shape_get_capsule_radius( shape_b ),
                                 wp_collision_shape_get_capsule_half_height( shape_b ), pa,
                                 np_shape_orientation( body_a, shape_a ),
                                 wp_collision_shape_get_box_half_extents( shape_a ), tol, 0, out );
    }

    /* Sphere vs Mesh */
    if( ta == WORKPHONE_COLLISION_SHAPE_SPHERE && tb == WORKPHONE_COLLISION_SHAPE_MESH )
    {
        return test_sphere_mesh( pa, wp_collision_shape_get_sphere_radius( shape_a ), body_b, shape_b,
                                 tol, 1, out );
    }

    /* Mesh vs Sphere */
    if( ta == WORKPHONE_COLLISION_SHAPE_MESH && tb == WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
        return test_sphere_mesh( pb, wp_collision_shape_get_sphere_radius( shape_b ), body_a, shape_a,
                                 tol, 0, out );
    }

    /* Box vs Mesh */
    if( ta == WORKPHONE_COLLISION_SHAPE_BOX && tb == WORKPHONE_COLLISION_SHAPE_MESH )
    {
        return test_box_mesh( pa, np_shape_orientation( body_a, shape_a ),
                              wp_collision_shape_get_box_half_extents( shape_a ), body_b, shape_b, tol,
                              1, out );
    }

    /* Mesh vs Box */
    if( ta == WORKPHONE_COLLISION_SHAPE_MESH && tb == WORKPHONE_COLLISION_SHAPE_BOX )
    {
        return test_box_mesh( pb, np_shape_orientation( body_b, shape_b ),
                              wp_collision_shape_get_box_half_extents( shape_b ), body_a, shape_a, tol,
                              0, out );
    }

    /* Capsule vs Mesh */
    if( ta == WORKPHONE_COLLISION_SHAPE_CAPSULE && tb == WORKPHONE_COLLISION_SHAPE_MESH )
    {
        return test_capsule_mesh( pa, np_shape_orientation( body_a, shape_a ),
                                  wp_collision_shape_get_capsule_radius( shape_a ),
                                  wp_collision_shape_get_capsule_half_height( shape_a ), body_b, shape_b,
                                  tol, 1, out );
    }

    /* Mesh vs Capsule */
    if( ta == WORKPHONE_COLLISION_SHAPE_MESH && tb == WORKPHONE_COLLISION_SHAPE_CAPSULE )
    {
        return test_capsule_mesh( pb, np_shape_orientation( body_b, shape_b ),
                                  wp_collision_shape_get_capsule_radius( shape_b ),
                                  wp_collision_shape_get_capsule_half_height( shape_b ), body_a, shape_a,
                                  tol, 0, out );
    }

    /* Dynamic mesh-vs-mesh response is intentionally unsupported. */
    (void)np;
    return 0;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_narrowphase *wp_narrowphase_create( wp_narrowphase_algorithm algorithm )
{
    wp_narrowphase *np = (wp_narrowphase *)malloc( sizeof( wp_narrowphase ) );
    if( !np )
    {
        return NULL;
    }

    memset( np, 0, sizeof( wp_narrowphase ) );

    np->algorithm = algorithm;
    np->max_iterations = WP_NP_DEFAULT_ITERATIONS;
    np->contact_tolerance = WP_NP_DEFAULT_TOLERANCE;

    return np;
}

void wp_narrowphase_destroy( wp_narrowphase *np )
{
    if( !np )
    {
        return;
    }

    free( np );
}

/* =========================================================================
 * Configuration
 * ====================================================================== */

wp_narrowphase_algorithm wp_narrowphase_get_algorithm( const wp_narrowphase *np )
{
    if( !np )
    {
        return WORKPHONE_NARROWPHASE_GJK_EPA;
    }
    return np->algorithm;
}

wp_s32 wp_narrowphase_get_max_iterations( const wp_narrowphase *np )
{
    if( !np )
    {
        return WP_NP_DEFAULT_ITERATIONS;
    }
    return np->max_iterations;
}

void wp_narrowphase_set_max_iterations( wp_narrowphase *np, wp_s32 iterations )
{
    if( !np )
    {
        return;
    }
    np->max_iterations = iterations;
}

wp_f32 wp_narrowphase_get_contact_tolerance( const wp_narrowphase *np )
{
    if( !np )
    {
        return WP_NP_DEFAULT_TOLERANCE;
    }
    return np->contact_tolerance;
}

void wp_narrowphase_set_contact_tolerance( wp_narrowphase *np, wp_f32 tolerance )
{
    if( !np )
    {
        return;
    }
    np->contact_tolerance = tolerance;
}

/* =========================================================================
 * Dispatch
 * ====================================================================== */

void wp_narrowphase_process_pairs( wp_narrowphase *np, const wp_broadphase_pair *pairs,
                                   wp_s32 pair_count )
{
    wp_s32 i, s, ss;
    wp_s32 shape_count_a, shape_count_b;

    if( !np || !pairs || pair_count <= 0 )
    {
        return;
    }

    np->manifold_count = 0;
    np->touching_pair_count = 0;

    for( i = 0; i < pair_count; ++i )
    {
        wp_rigidbody *body_a = pairs[i].body_a;
        wp_rigidbody *body_b = pairs[i].body_b;

        if( !body_a || !body_b )
        {
            continue;
        }

        shape_count_a = wp_rigidbody_get_shape_count( body_a );
        shape_count_b = wp_rigidbody_get_shape_count( body_b );

        for( s = 0; s < shape_count_a; ++s )
        {
            wp_collision_shape *sa = wp_rigidbody_get_shape( body_a, s );

            for( ss = 0; ss < shape_count_b; ++ss )
            {
                wp_collision_shape *sb = wp_rigidbody_get_shape( body_b, ss );

                if( np->manifold_count >= WP_NARROWPHASE_MAX_MANIFOLDS )
                {
                    break;
                }

                if( dispatch_pair( np, body_a, sa, body_b, sb, &np->manifolds[np->manifold_count] ) )
                {
                    ++np->manifold_count;
                    ++np->touching_pair_count;
                }
            }
        }
    }
}

/* =========================================================================
 * Single-pair test
 * ====================================================================== */

wp_s32 wp_narrowphase_test_pair( wp_narrowphase *np, wp_rigidbody *body_a, wp_collision_shape *shape_a,
                                 wp_rigidbody *body_b, wp_collision_shape *shape_b,
                                 wp_contact_manifold *out_manifold )
{
    if( !np || !body_a || !shape_a || !body_b || !shape_b || !out_manifold )
    {
        return 0;
    }

    return dispatch_pair( np, body_a, shape_a, body_b, shape_b, out_manifold );
}

/* =========================================================================
 * Manifold cache
 * ====================================================================== */

const wp_contact_manifold *wp_narrowphase_get_manifold_cache( const wp_narrowphase *np )
{
    if( !np || np->manifold_count == 0 )
    {
        return NULL;
    }
    return np->manifolds;
}

wp_s32 wp_narrowphase_get_manifold_count( const wp_narrowphase *np )
{
    if( !np )
    {
        return 0;
    }
    return np->manifold_count;
}

/* =========================================================================
 * Statistics
 * ====================================================================== */

wp_s32 wp_narrowphase_get_touching_pair_count( const wp_narrowphase *np )
{
    if( !np )
    {
        return 0;
    }
    return np->touching_pair_count;
}

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_narrowphase_get_native( const wp_narrowphase *np )
{
    if( !np )
    {
        return NULL;
    }
    return np->native;
}

void wp_narrowphase_set_native( wp_narrowphase *np, void *native )
{
    if( !np )
    {
        return;
    }
    np->native = native;
}

void *wp_narrowphase_get_user_data( const wp_narrowphase *np )
{
    if( !np )
    {
        return NULL;
    }
    return np->user_data;
}

void wp_narrowphase_set_user_data( wp_narrowphase *np, void *user_data )
{
    if( !np )
    {
        return;
    }
    np->user_data = user_data;
}
