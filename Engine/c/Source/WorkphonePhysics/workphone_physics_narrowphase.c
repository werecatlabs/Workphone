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
 * Mesh contacts stream sphere/AABB candidates from a shared flat BVH,
 * then evaluate exact contacts against the caller-owned triangle data.
 * Dynamic mesh-vs-mesh response is intentionally excluded; production scenes
 * should use sphere, box, or capsule dynamic shapes against static meshes.
 */

#include "workphone_physics_narrowphase.h"
#include "workphone_physics.h"
#include "workphone_physics_collisionshape.h"
#include "workphone_physics_rigidbody.h"
#include "workphone_physics_triangle_mesh.h"
#include "workphone_physics_geometry.h"
#include "workphone_physics_narrowphase_internal.h"
#include <limits.h>
#include <float.h>
#include <time.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* =========================================================================
 * Internal constants
 * ====================================================================== */

#define WP_NP_DEFAULT_TOLERANCE 0.001f
#define WP_NP_DEFAULT_ITERATIONS 64

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_narrowphase
{
    wp_narrowphase_algorithm algorithm;
    wp_s32 max_iterations;
    wp_f32 contact_tolerance;

    wp_contact_manifold *manifolds;
    wp_s32 manifold_capacity;
    wp_s32 simd_enabled, acceleration_enabled, mesh_obb_enabled, timing_enabled;
    wp_narrowphase_stats stats;
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
    /* All callers use normalized prepared or relative poses. */
    qv.x = q.x;
    qv.y = q.y;
    qv.z = q.z;
    t = np_scale( np_cross( qv, v ), 2.0f );
    return np_add( v, np_add( np_scale( t, q.w ), np_cross( qv, t ) ) );
}

static wp_vec3f np_shape_position( const wp_rigidbody *body, const wp_collision_shape *shape )
{
    return wp_shape_prepare( body, shape, NULL )->center;
}
static wp_quatf np_shape_orientation( const wp_rigidbody *body, const wp_collision_shape *shape )
{
    return wp_shape_prepare( body, shape, NULL )->orientation;
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

    /* Degenerate triangles reduce to their three segments, avoiding 0/0 in
     * barycentric regions. Sphere/capsule contacts may legitimately touch them. */
    if ( np_len_sq( np_cross( ab, ac ) ) <= 1.e-20f )
    {
        wp_vec3f starts[3] = { a, b, c }, ends[3] = { b, c, a }, best = a;
        wp_f32 best_distance = FLT_MAX;
        for ( wp_s32 i = 0; i < 3; ++i )
        {
            wp_vec3f edge = np_sub( ends[i], starts[i] );
            wp_f32 length = np_len_sq( edge );
            wp_f32 t = length > 1.e-20f
                           ? np_clampf( np_dot( np_sub( point, starts[i] ), edge ) / length, 0, 1 )
                           : 0;
            wp_vec3f candidate = np_add( starts[i], np_scale( edge, t ) );
            wp_f32 distance = np_len_sq( np_sub( candidate, point ) );
            if ( distance < best_distance )
            {
                best = candidate;
                best_distance = distance;
            }
        }
        return best;
    }

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

    tolerance *= np_sqrtf( length_sq );
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

wp_s32 wp_triangle_box_overlaps( wp_vec3f a, wp_vec3f b, wp_vec3f c, wp_vec3f half_extents,
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

    /* The three box-face axes are direct coordinate bounds. */
#define TRI_FACE( component )                                                                      \
    if ( fminf( a.component, fminf( b.component, c.component ) ) >                                 \
             half_extents.component + tolerance ||                                                 \
         fmaxf( a.component, fmaxf( b.component, c.component ) ) <                                 \
             -half_extents.component - tolerance )                                                 \
        return 0;
    TRI_FACE( x )
    TRI_FACE( y ) TRI_FACE( z )
#undef TRI_FACE
        if ( !np_triangle_box_axis_overlaps( triangle_normal, a, b, c, half_extents, tolerance ) )
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
    m->is_touching = 1;
}

void wp_manifold_refresh_materials( wp_contact_manifold *m )
{
    wp_contact_point *cp;
    if ( !m || !m->contact_count )
        return;
    cp = &m->contacts[0];
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

    for ( wp_s32 i = 1; i < m->contact_count; ++i )
    {
        m->contacts[i].combined_friction = cp->combined_friction;
        m->contacts[i].combined_restitution = cp->combined_restitution;
    }
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
        /* Merge in mesh space's world anchor regardless of dispatch direction. */
        wp_vec3f point_delta =
            wp_collision_shape_get_type( m->shape_a ) == WORKPHONE_COLLISION_SHAPE_MESH
                ? np_sub( existing->position_world_on_a, on_a )
                : np_sub( existing->position_world_on_b, on_b );
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

/* Relative-matrix SAT. Face axes need no normalization. Cross-axis tests
 * scale tolerance by length; only the selected minimum axis is normalized. */
static wp_s32 test_box_box( const wp_prepared_shape *a, const wp_prepared_shape *b,
                            wp_f32 tolerance, wp_contact_manifold *out )
{
    wp_f32 r[3][3], ar[3][3], t[3], ha[3] = { a->half.x, a->half.y, a->half.z };
    wp_f32 hb[3] = { b->half.x, b->half.y, b->half.z }, minimum = FLT_MAX;
    wp_vec3f delta = np_sub( b->center, a->center ), normal = np_zero();
    for ( wp_s32 i = 0; i < 3; ++i )
    {
        t[i] = np_dot( delta, a->axes[i] );
        for ( wp_s32 j = 0; j < 3; ++j )
        {
            r[i][j] = np_dot( a->axes[i], b->axes[j] );
            ar[i][j] = fabsf( r[i][j] );
        }
    }
    for ( wp_s32 i = 0; i < 3; ++i )
    {
        wp_f32 distance_b = np_dot( delta, b->axes[i] );
        wp_f32 overlap_a =
            ha[i] + ar[i][0] * hb[0] + ar[i][1] * hb[1] + ar[i][2] * hb[2] - fabsf( t[i] );
        wp_f32 overlap_b =
            hb[i] + ar[0][i] * ha[0] + ar[1][i] * ha[1] + ar[2][i] * ha[2] - fabsf( distance_b );
        if ( overlap_a < -tolerance || overlap_b < -tolerance )
            return 0;
        if ( overlap_a < minimum )
        {
            minimum = overlap_a;
            normal = np_scale( a->axes[i], t[i] >= 0 ? 1.f : -1.f );
        }
        if ( overlap_b < minimum )
        {
            minimum = overlap_b;
            normal = np_scale( b->axes[i], distance_b >= 0 ? 1.f : -1.f );
        }
    }
    for ( wp_s32 i = 0; i < 3; ++i )
        for ( wp_s32 j = 0; j < 3; ++j )
        {
            wp_s32 i1 = ( i + 1 ) % 3, i2 = ( i + 2 ) % 3, j1 = ( j + 1 ) % 3, j2 = ( j + 2 ) % 3;
            wp_vec3f axis = np_cross( a->axes[i], b->axes[j] );
            wp_f32 length_sq = np_len_sq( axis ), length, distance, overlap;
            if ( length_sq <= 1.e-12f )
                continue;
            length = sqrtf( length_sq );
            distance = t[i2] * r[i1][j] - t[i1] * r[i2][j];
            overlap = ha[i1] * ar[i2][j] + ha[i2] * ar[i1][j] + hb[j1] * ar[i][j2] +
                      hb[j2] * ar[i][j1] - fabsf( distance );
            if ( overlap < -tolerance * length )
                return 0;
            overlap /= length;
            if ( overlap < minimum )
            {
                minimum = overlap;
                normal =
                    np_scale( axis, np_dot( delta, axis ) >= 0 ? 1.f / length : -1.f / length );
            }
        }
    {
        wp_f32 ra = fabsf( np_dot( normal, a->axes[0] ) ) * ha[0] +
                    fabsf( np_dot( normal, a->axes[1] ) ) * ha[1] +
                    fabsf( np_dot( normal, a->axes[2] ) ) * ha[2];
        wp_f32 rb = fabsf( np_dot( normal, b->axes[0] ) ) * hb[0] +
                    fabsf( np_dot( normal, b->axes[1] ) ) * hb[1] +
                    fabsf( np_dot( normal, b->axes[2] ) ) * hb[2];
        manifold_add_contact( out, np_add( a->center, np_scale( normal, ra ) ),
                              np_sub( b->center, np_scale( normal, rb ) ), normal,
                              fmaxf( minimum, 0 ) );
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

/* Squared segment/AABB distance is quadratic between the at most six face
 * crossing parameters. Minimize each interval analytically, including endpoints. */
wp_f32 wp_segment_box_closest( wp_vec3f start, wp_vec3f end, wp_vec3f half, wp_vec3f *on_segment,
                               wp_vec3f *on_box )
{
    double points[8] = { 0, 1 }, p[3] = { start.x, start.y, start.z };
    double d[3] = { (double)end.x - start.x, (double)end.y - start.y, (double)end.z - start.z };
    double h[3] = { half.x, half.y, half.z }, best = DBL_MAX, best_t = 0;
    wp_s32 count = 2;
    for ( wp_s32 axis = 0; axis < 3; ++axis )
        if ( d[axis] != 0 )
            for ( wp_s32 sign = -1; sign <= 1; sign += 2 )
            {
                double t = ( sign * h[axis] - p[axis] ) / d[axis];
                if ( t > 0 && t < 1 )
                    points[count++] = t;
            }
    for ( wp_s32 i = 1; i < count; ++i )
    {
        double value = points[i];
        wp_s32 j = i;
        while ( j && points[j - 1] > value )
        {
            points[j] = points[j - 1];
            --j;
        }
        points[j] = value;
    }
    for ( wp_s32 i = 0; i < count - 1; ++i )
    {
        double lo = points[i], hi = points[i + 1], mid = ( lo + hi ) * .5, aa = 0, bb = 0, t,
               dist = 0;
        for ( wp_s32 axis = 0; axis < 3; ++axis )
        {
            double x = p[axis] + d[axis] * mid, offset;
            if ( x < -h[axis] )
                offset = p[axis] + h[axis];
            else if ( x > h[axis] )
                offset = p[axis] - h[axis];
            else
                continue;
            aa += d[axis] * d[axis];
            bb += d[axis] * offset;
        }
        t = aa > 0 ? fmax( lo, fmin( hi, -bb / aa ) ) : mid;
        for ( wp_s32 axis = 0; axis < 3; ++axis )
        {
            double x = p[axis] + d[axis] * t, e = fmax( -h[axis], fmin( h[axis], x ) ) - x;
            dist += e * e;
        }
        if ( dist < best )
        {
            best = dist;
            best_t = t;
        }
    }
    *on_segment = ( wp_vec3f ){ (wp_f32)( p[0] + d[0] * best_t ), (wp_f32)( p[1] + d[1] * best_t ),
                                (wp_f32)( p[2] + d[2] * best_t ) };
    return np_point_aabb_distance_sq( *on_segment, half, on_box );
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

    distance_sq = wp_segment_box_closest( local_start, local_end, box_half_extents,
                                          &on_segment_local, &on_box_local );
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
        /* Minimum box-face translation that ejects the entire capsule,
         * not just an arbitrary point of an embedded segment. */
        wp_f32 starts[3] = { local_start.x, local_start.y, local_start.z };
        wp_f32 ends[3] = { local_end.x, local_end.y, local_end.z };
        wp_f32 halves[3] = { box_half_extents.x, box_half_extents.y, box_half_extents.z };
        wp_s32 best_axis = 0, best_sign = 1, use_end = 0;
        depth = FLT_MAX;
        for ( wp_s32 axis = 0; axis < 3; ++axis )
            for ( wp_s32 sign = -1; sign <= 1; sign += 2 )
            {
                wp_f32 trailing = sign > 0 ? fminf( starts[axis], ends[axis] )
                                           : fmaxf( starts[axis], ends[axis] );
                wp_f32 push = halves[axis] - sign * trailing + capsule_radius;
                if ( push < depth )
                {
                    depth = push;
                    best_axis = axis;
                    best_sign = sign;
                    use_end = sign > 0 ? ends[axis] < starts[axis] : ends[axis] > starts[axis];
                }
            }
        on_segment_local = use_end ? local_end : local_start;
        np_point_aabb_distance_sq( on_segment_local, box_half_extents, &on_box_local );
        normal_local = np_zero();
        if ( best_axis == 0 )
        {
            normal_local.x = (wp_f32)-best_sign;
            on_box_local.x = best_sign * box_half_extents.x;
        }
        if ( best_axis == 1 )
        {
            normal_local.y = (wp_f32)-best_sign;
            on_box_local.y = best_sign * box_half_extents.y;
        }
        if ( best_axis == 2 )
        {
            normal_local.z = (wp_f32)-best_sign;
            on_box_local.z = best_sign * box_half_extents.z;
        }
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

typedef struct np_mesh_context
{
    wp_narrowphase *np;
    wp_contact_manifold *out;
    const wp_prepared_shape *primitive, *mesh;
    wp_collision_shape_type type;
    wp_s32 primitive_is_a;
    wp_f32 tolerance;
    wp_vec3f center, start, end, offset;
    wp_quatf relative;
} np_mesh_context;

static void mesh_contact_triangle( wp_u32 triangle, wp_vec3f a, wp_vec3f b, wp_vec3f c,
                                   void *context )
{
    np_mesh_context *ctx = (np_mesh_context *)context;
    wp_vec3f on_primitive, on_mesh, normal, normal_world, primitive_world, mesh_world;
    wp_f32 depth;
    (void)triangle;
    ++ctx->np->stats.tested_triangles;
    if ( ctx->type == WORKPHONE_COLLISION_SHAPE_BOX )
    {
        wp_vec3f half = ctx->primitive->half;
        wp_f32 length, signed_distance, support;
        a = np_add( ctx->offset, np_quat_rotate( ctx->relative, a ) );
        b = np_add( ctx->offset, np_quat_rotate( ctx->relative, b ) );
        c = np_add( ctx->offset, np_quat_rotate( ctx->relative, c ) );
        if ( !wp_triangle_box_overlaps( a, b, c, half, ctx->tolerance ) )
            return;
        normal = np_cross( np_sub( b, a ), np_sub( c, a ) );
        length = np_len( normal );
        if ( length <= 1.e-8f )
            return;
        normal = np_scale( normal, 1.f / length );
        signed_distance = -np_dot( normal, a );
        support =
            fabsf( normal.x ) * half.x + fabsf( normal.y ) * half.y + fabsf( normal.z ) * half.z;
        depth = support - fabsf( signed_distance );
        if ( signed_distance >= 0 )
            normal = np_scale( normal, -1 );
        on_mesh = np_closest_point_triangle( np_zero(), a, b, c );
        on_primitive = ( wp_vec3f ){ np_clampf( on_mesh.x, -half.x, half.x ),
                                     np_clampf( on_mesh.y, -half.y, half.y ),
                                     np_clampf( on_mesh.z, -half.z, half.z ) };
        normal_world = np_quat_rotate( ctx->primitive->orientation, normal );
        primitive_world = np_add( ctx->primitive->center,
                                  np_quat_rotate( ctx->primitive->orientation, on_primitive ) );
        mesh_world = np_add( ctx->primitive->center,
                             np_quat_rotate( ctx->primitive->orientation, on_mesh ) );
    }
    else
    {
        wp_vec3f on_segment, delta;
        wp_f32 distance_sq, distance, radius = ctx->primitive->radius;
        if ( ctx->type == WORKPHONE_COLLISION_SHAPE_SPHERE )
        {
            on_segment = ctx->center;
            on_mesh = np_closest_point_triangle( on_segment, a, b, c );
        }
        else
            np_closest_segment_triangle( ctx->start, ctx->end, a, b, c, &on_segment, &on_mesh );
        delta = np_sub( on_mesh, on_segment );
        distance_sq = np_len_sq( delta );
        if ( distance_sq > ( radius + ctx->tolerance ) * ( radius + ctx->tolerance ) )
            return;
        distance = sqrtf( distance_sq );
        depth = radius - distance;
        if ( distance > 1.e-7f )
            normal = np_scale( delta, 1.f / distance );
        else
        {
            normal = np_normalize( np_cross( np_sub( b, a ), np_sub( c, a ) ) );
            if ( np_dot( normal, np_sub( on_segment, a ) ) >= 0 )
                normal = np_scale( normal, -1 );
        }
        on_primitive = np_add( on_segment, np_scale( normal, radius ) );
        normal_world = np_quat_rotate( ctx->mesh->orientation, normal );
        primitive_world =
            np_add( ctx->mesh->center, np_quat_rotate( ctx->mesh->orientation, on_primitive ) );
        mesh_world = np_add( ctx->mesh->center, np_quat_rotate( ctx->mesh->orientation, on_mesh ) );
    }
    ++ctx->np->stats.contacts_generated;
    if ( ctx->primitive_is_a )
        manifold_add_mesh_contact( ctx->out, primitive_world, mesh_world, normal_world, depth );
    else
        manifold_add_mesh_contact( ctx->out, mesh_world, primitive_world,
                                   np_scale( normal_world, -1 ), depth );
}
static wp_s32 test_primitive_mesh( wp_narrowphase *np, const wp_prepared_shape *primitive,
                                   wp_collision_shape_type type, const wp_rigidbody *mesh_body,
                                   const wp_collision_shape *mesh_shape, wp_f32 tolerance,
                                   wp_s32 primitive_is_a, wp_contact_manifold *out )
{
    const wp_collision_mesh_data *data = wp_collision_shape_get_mesh_data( mesh_shape );
    const wp_triangle_mesh *mesh = wp_collision_shape_get_triangle_mesh( mesh_shape );
    const wp_prepared_shape *prepared_mesh = wp_shape_prepare( mesh_body, mesh_shape, NULL );
    np_mesh_context ctx = { 0 };
    wp_triangle_mesh_query query = { 0 };
    wp_triangle_mesh_query_stats stats = { 0 };
    wp_vec3f extents;
    wp_f32 pad = tolerance + primitive->roundoff + prepared_mesh->roundoff;
    if ( !data || !data->vertices || !data->indices )
        return 0;
    ctx.np = np;
    ctx.out = out;
    ctx.primitive = primitive;
    ctx.mesh = prepared_mesh;
    ctx.type = type;
    ctx.tolerance = tolerance;
    ctx.primitive_is_a = primitive_is_a;
    ctx.center = np_quat_rotate( prepared_mesh->inverse,
                                 np_sub( primitive->center, prepared_mesh->center ) );
    ctx.start = np_quat_rotate( prepared_mesh->inverse,
                                np_sub( primitive->segment_start, prepared_mesh->center ) );
    ctx.end = np_quat_rotate( prepared_mesh->inverse,
                              np_sub( primitive->segment_end, prepared_mesh->center ) );
    ctx.relative =
        np_quat_normalize( np_quat_multiply( primitive->inverse, prepared_mesh->orientation ) );
    ctx.offset =
        np_quat_rotate( primitive->inverse, np_sub( prepared_mesh->center, primitive->center ) );
    query.center = ctx.center;
    query.simd_enabled = np->simd_enabled;
    if ( type == WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
        query.sphere = 1;
        query.radius = primitive->radius + pad;
    }
    else if ( type == WORKPHONE_COLLISION_SHAPE_CAPSULE )
    {
        wp_f32 r = primitive->radius + pad;
        query.minimum =
            ( wp_vec3f ){ fminf( ctx.start.x, ctx.end.x ) - r, fminf( ctx.start.y, ctx.end.y ) - r,
                          fminf( ctx.start.z, ctx.end.z ) - r };
        query.maximum =
            ( wp_vec3f ){ fmaxf( ctx.start.x, ctx.end.x ) + r, fmaxf( ctx.start.y, ctx.end.y ) + r,
                          fmaxf( ctx.start.z, ctx.end.z ) + r };
    }
    else
    {
        wp_vec3f h = primitive->half;
        for ( wp_s32 i = 0; i < 3; ++i )
            query.axes[i] = np_quat_rotate( prepared_mesh->inverse, primitive->axes[i] );
        /* Inflate in the box basis, then enclose it in mesh-local coordinates. */
        h.x += pad;
        h.y += pad;
        h.z += pad;
        query.half = h;
        extents = ( wp_vec3f ){ fabsf( query.axes[0].x ) * h.x + fabsf( query.axes[1].x ) * h.y +
                                    fabsf( query.axes[2].x ) * h.z,
                                fabsf( query.axes[0].y ) * h.x + fabsf( query.axes[1].y ) * h.y +
                                    fabsf( query.axes[2].y ) * h.z,
                                fabsf( query.axes[0].z ) * h.x + fabsf( query.axes[1].z ) * h.y +
                                    fabsf( query.axes[2].z ) * h.z };
        query.minimum = np_sub( ctx.center, extents );
        query.maximum = np_add( ctx.center, extents );
        query.oriented = np->mesh_obb_enabled &&
                         (double)extents.x * extents.y * extents.z > (double)h.x * h.y * h.z * 1.5;
    }
    if ( mesh && np->acceleration_enabled )
        wp_triangle_mesh_visit( mesh, &query, mesh_contact_triangle, &ctx, &stats );
    else
    {
        ++stats.full_scan_fallbacks;
        for ( wp_u32 i = 0; i < data->triangle_count; ++i )
        {
            wp_u32 ia = data->indices[i * 3], ib = data->indices[i * 3 + 1],
                   ic = data->indices[i * 3 + 2];
            if ( ia >= data->vertex_count || ib >= data->vertex_count || ic >= data->vertex_count )
                continue;
            ++stats.candidates;
            mesh_contact_triangle( i,
                                   ( wp_vec3f ){ data->vertices[ia * 3], data->vertices[ia * 3 + 1],
                                                 data->vertices[ia * 3 + 2] },
                                   ( wp_vec3f ){ data->vertices[ib * 3], data->vertices[ib * 3 + 1],
                                                 data->vertices[ib * 3 + 2] },
                                   ( wp_vec3f ){ data->vertices[ic * 3], data->vertices[ic * 3 + 1],
                                                 data->vertices[ic * 3 + 2] },
                                   &ctx );
        }
    }
    np->stats.mesh_nodes += stats.nodes;
    np->stats.mesh_candidates += stats.candidates;
    np->stats.full_scan_fallbacks += stats.full_scan_fallbacks;
    np->stats.simd_batches += stats.simd_batches;
    np->stats.oriented_tests += stats.oriented_tests;
    np->stats.oriented_rejections += stats.oriented_rejections;
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
        /* Explicit tests may use one unattached shape at two body poses. */
        if( shape_a == shape_b )
        {
            const wp_prepared_shape prepared_a = *wp_shape_prepare(body_a,shape_a,NULL);
            return test_box_box(&prepared_a,wp_shape_prepare(body_b,shape_b,NULL),tol,out);
        }
        return test_box_box( wp_shape_prepare( body_a, shape_a, NULL ),
                             wp_shape_prepare( body_b, shape_b, NULL ), tol, out );
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

    if ( tb == WORKPHONE_COLLISION_SHAPE_MESH && ta != WORKPHONE_COLLISION_SHAPE_PLANE &&
         ta != WORKPHONE_COLLISION_SHAPE_MESH )
        return test_primitive_mesh( np, wp_shape_prepare( body_a, shape_a, NULL ), ta, body_b,
                                    shape_b, tol, 1, out );
    if ( ta == WORKPHONE_COLLISION_SHAPE_MESH && tb != WORKPHONE_COLLISION_SHAPE_PLANE &&
         tb != WORKPHONE_COLLISION_SHAPE_MESH )
        return test_primitive_mesh( np, wp_shape_prepare( body_b, shape_b, NULL ), tb, body_a,
                                    shape_a, tol, 0, out );

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

    np->acceleration_enabled = 1;
    np->mesh_obb_enabled = 1;
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

    free( np->manifolds );
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

wp_s32 wp_narrowphase_process_pairs_checked( wp_narrowphase *np, const wp_broadphase_pair *pairs,
                                             wp_s32 pair_count )
{
    if ( !np )
        return 0;
    np->manifold_count = np->touching_pair_count = 0;
    if ( pair_count == 0 )
        return 1;
    if ( !pairs || pair_count < 0 )
        return 0;
    for ( wp_s32 i = 0; i < pair_count; ++i )
    {
        wp_rigidbody *a = pairs[i].body_a, *b = pairs[i].body_b;
        if ( !a || !b )
            continue;
        for ( wp_s32 sa = 0; sa < wp_rigidbody_get_shape_count( a ); ++sa )
            for ( wp_s32 sb = 0; sb < wp_rigidbody_get_shape_count( b ); ++sb )
            {
                wp_contact_manifold manifold;
                if ( !wp_narrowphase_test_pair( np, a, wp_rigidbody_get_shape( a, sa ), b,
                                                wp_rigidbody_get_shape( b, sb ), &manifold ) )
                    continue;
                if ( np->manifold_count == np->manifold_capacity )
                {
                    wp_s32 cap;
                    wp_contact_manifold *grown;
                    if ( np->manifold_capacity > INT_MAX / 2 )
                        goto failed;
                    cap = np->manifold_capacity ? np->manifold_capacity * 2 : 64;
                    if ( (size_t)cap > SIZE_MAX / sizeof( *grown ) )
                        goto failed;
                    grown = (wp_contact_manifold *)realloc( np->manifolds,
                                                            (size_t)cap * sizeof( *grown ) );
                    if ( !grown )
                        goto failed;
                    np->manifolds = grown;
                    np->manifold_capacity = cap;
                }
                np->manifolds[np->manifold_count++] = manifold;
            }
    }
    np->touching_pair_count = np->manifold_count;
    return 1;
failed:
    np->manifold_count = np->touching_pair_count = 0;
    return 0;
}
void wp_narrowphase_process_pairs( wp_narrowphase *np, const wp_broadphase_pair *pairs,
                                   wp_s32 count )
{
    (void)wp_narrowphase_process_pairs_checked( np, pairs, count );
}

/* =========================================================================
 * Single-pair test
 * ====================================================================== */

static uint64_t np_nanoseconds( void )
{
#ifdef _WIN32
    LARGE_INTEGER t, f;
    QueryPerformanceCounter( &t );
    QueryPerformanceFrequency( &f );
    return (uint64_t)( (double)t.QuadPart * 1.e9 / (double)f.QuadPart );
#else
    struct timespec t;
    timespec_get( &t, TIME_UTC );
    return (uint64_t)t.tv_sec * UINT64_C( 1000000000 ) + (uint64_t)t.tv_nsec;
#endif
}
wp_narrowphase_stats wp_narrowphase_get_stats( const wp_narrowphase *np )
{
    wp_narrowphase_stats empty = { 0 };
    return np ? np->stats : empty;
}
void wp_narrowphase_reset_stats( wp_narrowphase *np )
{
    if ( np )
        memset( &np->stats, 0, sizeof( np->stats ) );
}
void wp_narrowphase_set_timing_enabled( wp_narrowphase *np, wp_s32 enabled )
{
    if ( np )
        np->timing_enabled = enabled != 0;
}
wp_s32 wp_narrowphase_set_simd_enabled( wp_narrowphase *np, wp_s32 enabled )
{
    if ( !np )
        return 0;
    np->simd_enabled = enabled && wp_triangle_mesh_simd_available();
    return np->simd_enabled;
}
void wp_narrowphase_set_mesh_acceleration_enabled( wp_narrowphase *np, wp_s32 enabled )
{
    if ( np )
        np->acceleration_enabled = enabled != 0;
}
void wp_narrowphase_set_mesh_obb_enabled( wp_narrowphase *np, wp_s32 enabled )
{
    if ( np )
        np->mesh_obb_enabled = enabled != 0;
}

wp_s32 wp_narrowphase_test_pair( wp_narrowphase *np, wp_rigidbody *body_a, wp_collision_shape *shape_a,
                                 wp_rigidbody *body_b, wp_collision_shape *shape_b,
                                 wp_contact_manifold *out_manifold )
{
    if( !np || !body_a || !shape_a || !body_b || !shape_b || !out_manifold )
    {
        return 0;
    }

    wp_s32 rebuilt_a, rebuilt_b, hit;
    wp_collision_shape_type ta = wp_collision_shape_get_type( shape_a ),
                            tb = wp_collision_shape_get_type( shape_b );
    uint64_t start = 0;
    if ( np->timing_enabled )
        start = np_nanoseconds();
    wp_shape_prepare( body_a, shape_a, &rebuilt_a );
    wp_shape_prepare( body_b, shape_b, &rebuilt_b );
    np->stats.prepared_rebuilds += rebuilt_a + rebuilt_b;
    np->stats.prepared_reuses += 2 - rebuilt_a - rebuilt_b;
    if ( (unsigned)ta < 5 && (unsigned)tb < 5 )
        ++np->stats.pair_calls[ta][tb];
    hit = dispatch_pair( np, body_a, shape_a, body_b, shape_b, out_manifold );
    if ( hit )
    {
        wp_manifold_refresh_materials( out_manifold );
        np->stats.contacts_retained += out_manifold->contact_count;
        if ( ta != WORKPHONE_COLLISION_SHAPE_MESH && tb != WORKPHONE_COLLISION_SHAPE_MESH )
            np->stats.contacts_generated += out_manifold->contact_count;
    }
    if ( np->timing_enabled && (unsigned)ta < 5 && (unsigned)tb < 5 )
        np->stats.pair_nanoseconds[ta][tb] += np_nanoseconds() - start;
    return hit;
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
