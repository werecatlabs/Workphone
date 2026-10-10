#include "workphone_physics_narrowphase.h"
#include "workphone_physics_narrowphase_internal.h"
#include "workphone_physics_geometry.h"
#include "workphone_physics_cache_internal.h"
#include "workphone_physics_rigidbody.h"
#include "workphone_physics_collisionshape.h"
#include "workphone_physics_scene.h"
#include "workphone_physics_triangle_mesh.h"
#include "WorkphonePhysicsNarrowphaseReference.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define CHECK( x )                                                                                 \
    do                                                                                             \
    {                                                                                              \
        if ( !( x ) )                                                                              \
        {                                                                                          \
            fprintf( stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x );                              \
            return 0;                                                                              \
        }                                                                                          \
    } while ( 0 )
static wp_vec3f v( float x, float y, float z )
{
    return ( wp_vec3f ){ x, y, z };
}
static wp_vec3f sub( wp_vec3f a, wp_vec3f b )
{
    return v( a.x - b.x, a.y - b.y, a.z - b.z );
}
static float dot( wp_vec3f a, wp_vec3f b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static unsigned seed = 0x713a729b;
static float random_float( float lo, float hi )
{
    seed = 1664525u * seed + 1013904223u;
    return lo + ( hi - lo ) * (float)( seed >> 8 ) / 16777216.f;
}
static wp_vec3f random_vector( float lo, float hi )
{
    return v( random_float( lo, hi ), random_float( lo, hi ), random_float( lo, hi ) );
}
static wp_quatf random_rotation( void )
{
    wp_quatf q = { random_float( -1, 1 ), random_float( -1, 1 ), random_float( -1, 1 ),
                   random_float( -1, 1 ) };
    float n = sqrtf( q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z );
    return ( wp_quatf ){ q.w / n, q.x / n, q.y / n, q.z / n };
}
static int close_vector( wp_vec3f a, wp_vec3f b, float tolerance )
{
    wp_vec3f d = sub( a, b );
    return dot( d, d ) <= tolerance * tolerance;
}

static int test_batch( void )
{
    enum
    {
        n = 46,
        p = n * ( n - 1 ) / 2
    };
    wp_rigidbody *b[n];
    wp_collision_shape *s[n];
    wp_broadphase_pair pairs[p];
    wp_narrowphase *np = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    int k = 0;
    CHECK( np );
    for ( int i = 0; i < n; ++i )
    {
        b[i] = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
        s[i] = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE );
        CHECK( b[i] && s[i] && wp_rigidbody_add_shape( b[i], s[i] ) >= 0 );
    }
    for ( int i = 0; i < n; ++i )
        for ( int j = i + 1; j < n; ++j )
            pairs[k++] = ( wp_broadphase_pair ){ b[i], b[j] };
    CHECK( wp_narrowphase_process_pairs_checked( np, pairs, p ) );
    CHECK( wp_narrowphase_get_manifold_count( np ) == p );
    CHECK( wp_narrowphase_get_touching_pair_count( np ) == p );
    wp_narrowphase_process_pairs( np, NULL, 0 );
    CHECK( wp_narrowphase_get_manifold_count( np ) == 0 );
    CHECK( !wp_narrowphase_process_pairs_checked( np, NULL, 1 ) );
    CHECK( wp_narrowphase_get_manifold_count( np ) == 0 );
    for ( int i = 0; i < n; ++i )
    {
        wp_rigidbody_destroy( b[i] );
        wp_collision_shape_destroy( s[i] );
    }
    wp_narrowphase_destroy( np );
    return 1;
}

static int test_hash_cache( void )
{
    enum
    {
        n = 6000
    };
    /* Opaque identities; the public utility must never dereference keys. */
    unsigned char identities[n * 4];
    unsigned char active[n];
    memset( active, 1, sizeof( active ) );
    wp_collision_cache *c = wp_collision_cache_create_with_payload( 1, sizeof( unsigned ) );
    CHECK( c );
    for ( int i = 0; i < n; ++i )
    {
        wp_rigidbody *a = (wp_rigidbody *)&identities[i * 4],
                     *b = (wp_rigidbody *)&identities[i * 4 + 1];
        wp_collision_shape *sa = (wp_collision_shape *)&identities[i * 4 + 2],
                           *sb = (wp_collision_shape *)&identities[i * 4 + 3];
        int k = wp_collision_cache_add( c, a, sa, b, sb );
        CHECK( k >= 0 );
        *(unsigned *)wp_collision_cache_payload( c, k ) = (unsigned)i;
        CHECK( wp_collision_cache_add( c, b, sb, a, sa ) == k );
    }
    CHECK( wp_collision_cache_get_count( c ) == n );
    for ( int i = 0; i < n; i += 3 )
    {
        int k = wp_collision_cache_find(
            c, (wp_rigidbody *)&identities[i * 4 + 1], (wp_collision_shape *)&identities[i * 4 + 3],
            (wp_rigidbody *)&identities[i * 4], (wp_collision_shape *)&identities[i * 4 + 2] );
        CHECK( k >= 0 );
        wp_collision_cache_remove_at( c, k );
        active[i] = 0;
    }
    for ( int i = 0; i < n; ++i )
    {
        int k = wp_collision_cache_find(
            c, (wp_rigidbody *)&identities[i * 4], (wp_collision_shape *)&identities[i * 4 + 2],
            (wp_rigidbody *)&identities[i * 4 + 1], (wp_collision_shape *)&identities[i * 4 + 3] );
        CHECK( ( k >= 0 ) == active[i] );
        if ( k >= 0 )
            CHECK( *(unsigned *)wp_collision_cache_payload( c, k ) == (unsigned)i );
    }
    wp_collision_cache_step_ages( c );
    CHECK( wp_collision_cache_get_entries( c )[0].age == 1 );
    wp_collision_cache_clear( c );
    CHECK( wp_collision_cache_get_count( c ) == 0 );
    CHECK( wp_collision_cache_add( c, (wp_rigidbody *)identities,
                                   (wp_collision_shape *)( identities + 2 ),
                                   (wp_rigidbody *)( identities + 1 ),
                                   (wp_collision_shape *)( identities + 3 ) ) == 0 );
    CHECK( *(unsigned *)wp_collision_cache_payload( c, 0 ) == 0 );
    wp_collision_cache_destroy( c );
    CHECK( !wp_collision_cache_create( INT_MAX ) );
    return 1;
}

static float compound_step( wp_contact_update_strategy strategy )
{
    wp_physics_scene *sc = wp_physics_scene_create();
    wp_rigidbody *a = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC ),
                 *b = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *sa = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE ),
                       *sb = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE ),
                       *plane = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_PLANE );
    wp_contact_options o = { strategy, 1000, 100, 1 };
    wp_physics_scene_set_gravity( sc, v( 0, 0, 0 ) );
    wp_physics_scene_set_contact_options( sc, &o );
    wp_collision_shape_set_sphere_radius( sa, .5f );
    wp_collision_shape_set_sphere_radius( sb, .5f );
    wp_collision_shape_set_local_position( sb, v( 0, 10, 0 ) );
    wp_rigidbody_add_shape( a, sa );
    wp_rigidbody_add_shape( a, sb );
    wp_rigidbody_add_shape( b, plane );
    wp_rigidbody_set_position( a, v( 0, .4f, 0 ) );
    wp_physics_scene_add_actor( sc, a );
    wp_physics_scene_add_actor( sc, b );
    wp_physics_scene_simulate( sc, 1.f / 60 );
    float y = wp_rigidbody_get_position( a ).y;
    wp_physics_scene_destroy( sc );
    wp_rigidbody_destroy( a );
    wp_rigidbody_destroy( b );
    wp_collision_shape_destroy( sa );
    wp_collision_shape_destroy( sb );
    wp_collision_shape_destroy( plane );
    return y;
}

static int test_scene_cache( void )
{
    float reference = compound_step( WP_CONTACT_STRATEGY_ALWAYS );
    CHECK( fabsf( reference - .4792f ) < 1.e-5f );
    CHECK( fabsf( reference - compound_step( WP_CONTACT_STRATEGY_FIXED ) ) < 1.e-6f );
    CHECK( fabsf( reference - compound_step( WP_CONTACT_STRATEGY_DISTANCE ) ) < 1.e-6f );
    wp_physics_scene *sc = wp_physics_scene_create();
    wp_rigidbody *unused = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC ),
                 *a = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC ),
                 *b = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *sa = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX ),
                       *sb = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    CHECK( sc && a && b && unused && sa && sb );
    wp_contact_options o = { WP_CONTACT_STRATEGY_FIXED, 1000, 100, 1 };
    wp_physics_scene_set_gravity( sc, v( 0, 0, 0 ) );
    wp_physics_scene_set_contact_options( sc, &o );
    wp_rigidbody_set_position( a, v( 10, 0, 0 ) );
    wp_rigidbody_set_position( b, v( 10.8f, 0, 0 ) );
    wp_collision_shape_set_trigger( sa, 1 );
    wp_rigidbody_add_shape( a, sa );
    wp_rigidbody_add_shape( b, sb );
    CHECK( wp_physics_scene_add_actor( sc, unused ) && wp_physics_scene_add_actor( sc, a ) &&
           wp_physics_scene_add_actor( sc, b ) );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_misses == 1 );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_hits == 1 );
    /* Removal swaps actor order: cached anchors and normal must reverse. */
    wp_physics_scene_remove_actor( sc, unused );
    wp_collision_shape_set_trigger( sa, 0 );
    wp_rigidbody_set_restitution( b, 1 );
    wp_rigidbody_set_linear_velocity( a, v( 1, 0, 0 ) );
    wp_physics_scene_simulate( sc, 1.e-9f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_hits == 1 );
    CHECK( wp_rigidbody_get_linear_velocity( a ).x < -.9f );
    wp_collision_shape_set_trigger( sa, 1 );
    wp_rigidbody_set_linear_velocity( a, v( 0, 0, 0 ) );
    wp_rigidbody_set_orientation( b, ( wp_quatf ){ .92387953f, 0, .38268343f, 0 } );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_misses == 1 );
    wp_collision_shape_set_local_position( sb, v( .1f, 0, 0 ) );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_misses == 1 );
    wp_collision_shape_set_box_half_extents( sb, v( .6f, .6f, .6f ) );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_misses == 1 );
    wp_collision_shape_set_enabled( sb, 0 );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_retired == 1 );
    wp_collision_shape_set_enabled( sb, 1 );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_misses == 1 );
    wp_rigidbody_set_position( b, v( 100, 0, 0 ) );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_retired == 1 );
    wp_rigidbody_set_position( b, v( 10.8f, 0, 0 ) );
    wp_physics_scene_simulate( sc, 1.e-6f );
    wp_rigidbody_set_mass( a, 0 );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_retired == 1 );
    wp_rigidbody_set_mass( a, 1 );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_misses == 1 );
    wp_rigidbody_remove_shape( b, 0 );
    wp_collision_shape_destroy( sb );
    sb = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    CHECK( sb );
    wp_rigidbody_add_shape( b, sb );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_misses == 1 );
    o.strategy = WP_CONTACT_STRATEGY_ALWAYS;
    wp_physics_scene_set_contact_options( sc, &o );
    wp_physics_scene_simulate( sc, 1.e-6f );
    wp_scene_broadphase_stats stats = wp_physics_scene_get_broadphase_stats( sc );
    CHECK( stats.cache_hits == 0 && stats.cache_misses == 0 && stats.cache_probes == 0 );
    CHECK( stats.static_actors == 1 && stats.dynamic_actors == 1 );
    o.strategy = WP_CONTACT_STRATEGY_FIXED;
    wp_physics_scene_set_contact_options( sc, &o );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_misses == 1 );
    wp_physics_scene_destroy( sc );
    wp_rigidbody_destroy( a );
    wp_rigidbody_destroy( b );
    wp_rigidbody_destroy( unused );
    wp_collision_shape_destroy( sa );
    wp_collision_shape_destroy( sb );
    return 1;
}

static int test_scene_cache_growth_and_static_transitions( void )
{
    enum
    {
        n = 46
    };
    wp_physics_scene *sc = wp_physics_scene_create();
    wp_rigidbody *b[n];
    wp_collision_shape *s[n];
    CHECK( sc );
    wp_physics_scene_set_gravity( sc, v( 0, 0, 0 ) );
    wp_contact_options o = { WP_CONTACT_STRATEGY_FIXED, 1000, 100, 1 };
    wp_physics_scene_set_contact_options( sc, &o );
    for ( int i = 0; i < n; ++i )
    {
        b[i] = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
        s[i] = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE );
        CHECK( b[i] && s[i] );
        wp_collision_shape_set_trigger( s[i], 1 );
        wp_rigidbody_add_shape( b[i], s[i] );
        CHECK( wp_physics_scene_add_actor( sc, b[i] ) );
    }
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_misses == 1035 );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).cache_hits == 1035 );
    wp_rigidbody_add_force( b[0], v( 100, 0, 0 ), WORKPHONE_FORCE_MODE_FORCE );
    CHECK(wp_rigidbody_get_accumulated_force(b[0]).x==100);
    for ( int i = 0; i < n; ++i )
        wp_rigidbody_set_type( b[i], WORKPHONE_RIGIDBODY_STATIC );
    wp_physics_scene_simulate( sc, 1.e-6f );
    wp_scene_broadphase_stats st = wp_physics_scene_get_broadphase_stats( sc );
    CHECK( st.static_actors == n && st.candidate_pairs == 0 && st.cache_retired == 1035 );
    CHECK( close_vector( wp_rigidbody_get_accumulated_force( b[0] ), v( 0, 0, 0 ), 1.e-6f ) );
    wp_rigidbody_set_type( b[0], WORKPHONE_RIGIDBODY_KINEMATIC );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).kinematic_actors == 1 );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).candidate_pairs == 0 );
    wp_rigidbody_set_type( b[0], WORKPHONE_RIGIDBODY_DYNAMIC );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).candidate_pairs == n - 1 );
    wp_physics_scene_destroy( sc );
    for ( int i = 0; i < n; ++i )
    {
        wp_rigidbody_destroy( b[i] );
        wp_collision_shape_destroy( s[i] );
    }
    return 1;
}

static int test_compound_culling( void )
{
    wp_physics_scene *sc = wp_physics_scene_create();
    wp_rigidbody *a = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC ),
                 *b = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *left = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE ),
                       *right = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE ),
                       *middle = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE );
    CHECK( sc && a && b && left && right && middle );
    wp_physics_scene_set_gravity( sc, v( 0, 0, 0 ) );
    wp_physics_scene_set_narrowphase_timing_enabled( sc, 1 );
    wp_collision_shape_set_local_position( left, v( -10, 0, 0 ) );
    wp_collision_shape_set_local_position( right, v( 10, 0, 0 ) );
    wp_collision_shape_set_trigger( middle, 1 );
    wp_rigidbody_add_shape( a, left );
    wp_rigidbody_add_shape( a, right );
    wp_rigidbody_add_shape( b, middle );
    CHECK( wp_physics_scene_add_actor( sc, a ) && wp_physics_scene_add_actor( sc, b ) );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).child_pair_rejections == 2 );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).narrowphase_tests == 0 );
    wp_collision_shape_set_local_position( right, v( 0, 0, 0 ) );
    wp_physics_scene_simulate( sc, 1.e-6f );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).child_pair_rejections == 1 );
    CHECK( wp_physics_scene_get_broadphase_stats( sc ).narrowphase_tests == 1 );
    CHECK(
        wp_physics_scene_get_narrowphase_stats( sc )
            .pair_nanoseconds[WORKPHONE_COLLISION_SHAPE_SPHERE][WORKPHONE_COLLISION_SHAPE_SPHERE] >
        0 );
    wp_physics_scene_destroy( sc );
    wp_rigidbody_destroy( a );
    wp_rigidbody_destroy( b );
    wp_collision_shape_destroy( left );
    wp_collision_shape_destroy( right );
    wp_collision_shape_destroy( middle );
    return 1;
}

static int test_scalar_kernels( void )
{
    wp_narrowphase *np = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    wp_rigidbody *a = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC ),
                 *b = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *sa = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX ),
                       *sb = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    CHECK( np && a && b && sa && sb );
    CHECK( wp_rigidbody_add_shape( a, sa ) >= 0 && wp_rigidbody_add_shape( b, sb ) >= 0 );
    for ( int i = 0; i < 12000; ++i )
    {
        wp_vec3f ca = random_vector( -2, 2 ), cb = random_vector( -2, 2 ),
                 ha = random_vector( .01f, 2 ), hb = random_vector( .01f, 2 );
        wp_quatf qa = random_rotation(), qb = random_rotation();
        if ( i % 10 == 0 )
            qb = qa; /* Parallel and near-parallel bases. */
        wp_contact_manifold fast, old, reverse;
        memset( &old, 0, sizeof( old ) );
        wp_rigidbody_set_position( a, ca );
        wp_rigidbody_set_position( b, cb );
        wp_rigidbody_set_orientation( a, qa );
        wp_rigidbody_set_orientation( b, qb );
        wp_collision_shape_set_box_half_extents( sa, ha );
        wp_collision_shape_set_box_half_extents( sb, hb );
        int hit = wp_narrowphase_test_pair( np, a, sa, b, sb, &fast );
        CHECK( hit == reference_box_box( ca, qa, ha, cb, qb, hb, .001f, &old ) );
        if ( hit )
        {
            CHECK( fabsf( fast.contacts[0].penetration_depth - old.contacts[0].penetration_depth ) <
                   8.e-4f );
            if(i%10!=0)
                CHECK(dot(fast.contacts[0].normal_world_on_b,old.contacts[0].normal_world_on_b)>.99f);
            CHECK( dot( fast.contacts[0].normal_world_on_b, sub( cb, ca ) ) >= -1.e-4f );
            CHECK( wp_narrowphase_test_pair( np, b, sb, a, sa, &reverse ) );
            CHECK( fabsf( reverse.contacts[0].penetration_depth -
                          fast.contacts[0].penetration_depth ) < 8.e-4f );
        }
        wp_vec3f x = random_vector( -3, 3 ), y = random_vector( -3, 3 ), z = random_vector( -3, 3 );
        if ( i % 20 == 0 )
            z = y;
        CHECK( wp_triangle_box_overlaps( x, y, z, ha, .001f ) ==
               reference_triangle_box( x, y, z, ha, .001f ) );
        wp_vec3f on_segment, on_box;
        float exact = wp_segment_box_closest( x, y, ha, &on_segment, &on_box ),
              prior = reference_segment_box( x, y, ha );
        if ( !isfinite( exact ) || fabsf( exact - prior ) >= 2.e-4f )
            fprintf(
                stderr,
                "segment case %d exact=%g prior=%g a=(%g,%g,%g) b=(%g,%g,%g) half=(%g,%g,%g)\n", i,
                exact, prior, x.x, x.y, x.z, y.x, y.y, y.z, ha.x, ha.y, ha.z );
        CHECK( isfinite( exact ) && fabsf( exact - prior ) < 2.e-4f );
        /* Independent dense sampling must never beat the analytic minimum. */
        for ( int k = 0; k <= 64; ++k )
        {
            float t = (float)k / 64;
            wp_vec3f p =
                v( x.x + ( y.x - x.x ) * t, x.y + ( y.y - x.y ) * t, x.z + ( y.z - x.z ) * t );
            float dx = fmaxf( fabsf( p.x ) - ha.x, 0 ), dy = fmaxf( fabsf( p.y ) - ha.y, 0 ),
                  dz = fmaxf( fabsf( p.z ) - ha.z, 0 );
            CHECK( exact <= dx * dx + dy * dy + dz * dz + 2.e-5f );
        }
    }
    wp_vec3f p, q;
    wp_rigidbody_set_position( a, v( 0, 0, 0 ) );
    wp_rigidbody_set_position( b, v( 4, 0, 0 ) );
    wp_rigidbody_set_orientation( a, ( wp_quatf ){ 1, 0, 0, 0 } );
    wp_rigidbody_set_orientation( b, ( wp_quatf ){ 1, 0, 0, 0 } );
    wp_collision_shape_set_box_half_extents( sa, v( .5f, .5f, .5f ) );
    wp_contact_manifold shared;
    CHECK( !wp_narrowphase_test_pair( np, a, sa, b, sa, &shared ) );
    wp_rigidbody_set_position( b, v( .8f, 0, 0 ) );
    CHECK( wp_narrowphase_test_pair( np, a, sa, b, sa, &shared ) );
    CHECK( fabsf( shared.contacts[0].penetration_depth - .2f ) < 1.e-6f );
    wp_rigidbody_set_position( b, v( 1.0009f, 0, 0 ) );
    CHECK( wp_narrowphase_test_pair( np, a, sa, b, sa, &shared ) );
    CHECK( shared.contacts[0].penetration_depth == 0 );
    wp_rigidbody_set_position( b, v( 1.0011f, 0, 0 ) );
    CHECK( !wp_narrowphase_test_pair( np, a, sa, b, sa, &shared ) );
    CHECK( fabsf( wp_segment_box_closest( v( 2, -2, 0 ), v( 2, 2, 0 ), v( 1, 1, 1 ), &p, &q ) -
                  1 ) < 1.e-6f );
    CHECK( wp_segment_box_closest( v( 1, 0, 0 ), v( 1, 0, 0 ), v( 1, 1, 1 ), &p, &q ) == 0 );
    wp_collision_shape *cap = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_CAPSULE );
    CHECK( cap );
    wp_collision_shape_set_capsule( cap, .25f, 2 );
    wp_rigidbody_set_position( a, v( 0, 0, 0 ) );
    wp_rigidbody_set_position( b, v( 0, 0, 0 ) );
    wp_rigidbody_set_orientation( a, ( wp_quatf ){ 1, 0, 0, 0 } );
    wp_rigidbody_set_orientation( b, ( wp_quatf ){ 1, 0, 0, 0 } );
    wp_collision_shape_set_box_half_extents( sb, v( 1, 1, 1 ) );
    wp_contact_manifold m;
    CHECK( wp_narrowphase_test_pair( np, a, cap, b, sb, &m ) );
    CHECK( fabsf( m.contacts[0].penetration_depth - 1.25f ) < 1.e-6f );
    CHECK( fabsf( m.contacts[0].normal_world_on_b.y ) < 1.e-6f );
    wp_narrowphase_reset_stats( np );
    wp_narrowphase_set_timing_enabled( np, 1 );
    for ( int i = 0; i < 100; ++i )
        wp_narrowphase_test_pair( np, a, cap, b, sb, &m );
    wp_narrowphase_stats stats = wp_narrowphase_get_stats( np );
    CHECK( stats.pair_calls[WORKPHONE_COLLISION_SHAPE_CAPSULE][WORKPHONE_COLLISION_SHAPE_BOX] ==
           100 );
    CHECK(
        stats.pair_nanoseconds[WORKPHONE_COLLISION_SHAPE_CAPSULE][WORKPHONE_COLLISION_SHAPE_BOX] >
        0 );
    wp_narrowphase_destroy( np );
    wp_rigidbody_destroy( a );
    wp_rigidbody_destroy( b );
    wp_collision_shape_destroy( sa );
    wp_collision_shape_destroy( sb );
    wp_collision_shape_destroy( cap );
    return 1;
}

typedef struct visit_result
{
    unsigned count;
    unsigned char *seen;
    unsigned limit;
} visit_result;
static void collect_triangle( wp_u32 t, wp_vec3f a, wp_vec3f b, wp_vec3f c, void *context )
{
    visit_result *r = (visit_result *)context;
    (void)a;
    (void)b;
    (void)c;
    if ( t >= r->limit || r->seen[t] )
    {
        fputs( "Invalid or duplicated triangle identity\n", stderr );
        abort();
    }
    r->seen[t] = 1;
    ++r->count;
}
static int independent_bounds_test( const float *verts, const unsigned *ids, unsigned t,
                                    const wp_triangle_mesh_query *q )
{
    float lo[3] = { 1.e30f, 1.e30f, 1.e30f }, hi[3] = { -1.e30f, -1.e30f, -1.e30f };
    for ( int j = 0; j < 3; ++j )
        for ( int k = 0; k < 3; ++k )
        {
            float x = verts[ids[t * 3 + j] * 3 + k];
            lo[k] = fminf( lo[k], x );
            hi[k] = fmaxf( hi[k], x );
        }
    if ( q->sphere )
    {
        float center[3] = { q->center.x, q->center.y, q->center.z }, d = 0;
        for ( int k = 0; k < 3; ++k )
        {
            float e = fmaxf( lo[k] - center[k], fmaxf( center[k] - hi[k], 0 ) );
            d += e * e;
        }
        return d <= q->radius * q->radius;
    }
    float qlo[3] = { q->minimum.x, q->minimum.y, q->minimum.z },
          qhi[3] = { q->maximum.x, q->maximum.y, q->maximum.z };
    for ( int k = 0; k < 3; ++k )
        if ( lo[k] > qhi[k] || hi[k] < qlo[k] )
            return 0;
    return 1;
}
typedef struct nested_result
{
    const wp_triangle_mesh *mesh;
    wp_triangle_mesh_query inner;
    visit_result outer, nested;
    int entered;
} nested_result;
static void nested_triangle( wp_u32 t, wp_vec3f a, wp_vec3f b, wp_vec3f c, void *context )
{
    nested_result *r = (nested_result *)context;
    collect_triangle( t, a, b, c, &r->outer );
    if ( !r->entered )
    {
        r->entered = 1;
        wp_triangle_mesh_visit( r->mesh, &r->inner, collect_triangle, &r->nested, NULL );
    }
}
static float maximum_depth( const wp_contact_manifold *m )
{
    float d = 0;
    for ( int i = 0; i < m->contact_count; ++i )
        d = fmaxf( d, m->contacts[i].penetration_depth );
    return d;
}
static int test_mesh( void )
{
    enum
    {
        n = 40,
        row = n + 1,
        nv = row * row,
        nt = n * n * 2
    };
    float *verts = (float *)malloc( nv * 3 * sizeof( float ) );
    unsigned *ids = (unsigned *)malloc( nt * 3 * sizeof( unsigned ) );
    unsigned char *seen = (unsigned char *)calloc( nt, 1 ),
                  *simd = (unsigned char *)calloc( nt, 1 );
    CHECK( verts && ids && seen && simd );
    unsigned t = 0;
    for ( unsigned z = 0; z < row; ++z )
        for ( unsigned x = 0; x < row; ++x )
        {
            unsigned k = ( z * row + x ) * 3;
            verts[k] = (float)x - 20;
            verts[k + 1] = 0;
            verts[k + 2] = (float)z - 20;
        }
    for ( unsigned z = 0; z < n; ++z )
        for ( unsigned x = 0; x < n; ++x )
        {
            unsigned k = z * row + x;
            ids[t++] = k;
            ids[t++] = k + row;
            ids[t++] = k + 1;
            ids[t++] = k + 1;
            ids[t++] = k + row;
            ids[t++] = k + row + 1;
        }
    wp_collision_shape *ms = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_MESH );
    wp_collision_mesh_data data = { verts, nv, ids, nt };
    CHECK( ms );
    wp_collision_shape_set_mesh_data( ms, &data );
    wp_triangle_mesh *mesh = (wp_triangle_mesh *)wp_collision_shape_get_triangle_mesh( ms );
    CHECK( mesh );
    for ( int i = 0; i < 120; ++i )
    {
        wp_triangle_mesh_query q = { 0 };
        q.center = random_vector( -22, 22 );
        q.center.y = 0;
        q.radius = random_float( .01f, 30 );
        q.sphere = i % 2;
        wp_vec3f h = random_vector( .01f, 22 );
        q.minimum = sub( q.center, h );
        q.maximum = v( q.center.x + h.x, q.center.y + h.y, q.center.z + h.z );
        memset( seen, 0, nt );
        memset( simd, 0, nt );
        visit_result scalar = { 0, seen, nt }, vector = { 0, simd, nt };
        wp_triangle_mesh_query_stats st = { 0 };
        wp_triangle_mesh_visit( mesh, &q, collect_triangle, &scalar, &st );
        q.simd_enabled = 1;
        wp_triangle_mesh_visit( mesh, &q, collect_triangle, &vector, NULL );
        CHECK( scalar.count == vector.count && !memcmp( seen, simd, nt ) );
        for ( unsigned k = 0; k < nt; ++k )
            CHECK( seen[k] == independent_bounds_test( verts, ids, k, &q ) );
        CHECK( st.full_scan_fallbacks == 0 && st.candidates == scalar.count );
    }
    unsigned sentinel[2] = { 0, 0x1234abcd };
    CHECK( wp_triangle_mesh_query_sphere( mesh, v( 0, 0, 0 ), 30, sentinel, 1 ) > 1024 );
    CHECK( sentinel[1] == 0x1234abcd );
    memset( seen, 0, nt );
    memset( simd, 0, nt );
    nested_result nested = { 0 };
    nested.mesh = mesh;
    nested.inner.sphere = 1;
    nested.inner.center = v( 0, 0, 0 );
    nested.inner.radius = 1;
    nested.outer = ( visit_result ){ 0, seen, nt };
    nested.nested = ( visit_result ){ 0, simd, nt };
    wp_triangle_mesh_query whole = { 0 };
    whole.sphere = 1;
    whole.radius = 30;
    wp_triangle_mesh_visit( mesh, &whole, nested_triangle, &nested, NULL );
    CHECK( nested.outer.count == nt && nested.nested.count > 0 && nested.nested.count < nt );

    wp_narrowphase *np = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    wp_rigidbody *a = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC ),
                 *b = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    CHECK( np && a && b );
    wp_rigidbody_add_shape( b, ms );
    wp_collision_shape_set_local_position( ms, v( .3f, .2f, -.1f ) );
    wp_collision_shape_set_local_orientation( ms, ( wp_quatf ){ .92387953f, 0, .38268343f, 0 } );
    for ( int kind = 0; kind < 3; ++kind )
    {
        wp_collision_shape_type types[3] = { WORKPHONE_COLLISION_SHAPE_SPHERE,
                                             WORKPHONE_COLLISION_SHAPE_CAPSULE,
                                             WORKPHONE_COLLISION_SHAPE_BOX };
        wp_collision_shape *shape = wp_collision_shape_create( types[kind] );
        CHECK( shape );
        wp_rigidbody_add_shape( a, shape );
        wp_collision_shape_set_local_position( shape, v( .2f, .1f, -.2f ) );
        wp_collision_shape_set_local_orientation( shape,
                                                  ( wp_quatf ){ .92387953f, .38268343f, 0, 0 } );
        if ( kind == 1 )
            wp_collision_shape_set_capsule( shape, .5f, 2 );
        if ( kind == 2 )
            wp_collision_shape_set_box_half_extents( shape, v( 2, .5f, .5f ) );
        for ( int i = 0; i < 100; ++i )
        {
            wp_vec3f pos = random_vector( -3, 3 );
            pos.y = random_float( -.1f, 1.2f );
            wp_rigidbody_set_position( a, pos );
            wp_rigidbody_set_position( b, random_vector( -.2f, .2f ) );
            wp_quatf q = { cosf( (float)i * .01f ), 0, sinf( (float)i * .01f ), 0 };
            wp_rigidbody_set_orientation( b, q );
            wp_rigidbody_set_orientation( a, random_rotation() );
            wp_contact_manifold fast, full, reverse, vector, obb;
            wp_narrowphase_set_mesh_acceleration_enabled( np, 1 );
            wp_narrowphase_set_simd_enabled( np, 0 );
            wp_narrowphase_set_mesh_obb_enabled( np, 0 );
            int hit = wp_narrowphase_test_pair( np, a, shape, b, ms, &fast );
            wp_narrowphase_set_simd_enabled( np, 1 );
            CHECK( hit == wp_narrowphase_test_pair( np, a, shape, b, ms, &vector ) );
            CHECK( fast.contact_count == vector.contact_count );
            for ( int c = 0; c < fast.contact_count; ++c )
            {
                CHECK( close_vector( fast.contacts[c].position_world_on_b,
                                     vector.contacts[c].position_world_on_b, 1.e-6f ) );
                CHECK( fabsf( fast.contacts[c].penetration_depth -
                              vector.contacts[c].penetration_depth ) < 1.e-6f );
            }
            wp_narrowphase_set_mesh_obb_enabled( np, 1 );
            CHECK( hit == wp_narrowphase_test_pair( np, a, shape, b, ms, &obb ) );
            CHECK( fabsf( maximum_depth( &fast ) - maximum_depth( &obb ) ) < 1.e-4f );
            wp_narrowphase_set_mesh_acceleration_enabled( np, 0 );
            CHECK( hit == wp_narrowphase_test_pair( np, a, shape, b, ms, &full ) );
            CHECK( fabsf( maximum_depth( &fast ) - maximum_depth( &full ) ) < 1.e-4f );
            CHECK( hit == wp_narrowphase_test_pair( np, b, ms, a, shape, &reverse ) );
            if ( hit )
                CHECK( close_vector( full.contacts[0].normal_world_on_b,
                                     v( -reverse.contacts[0].normal_world_on_b.x,
                                        -reverse.contacts[0].normal_world_on_b.y,
                                        -reverse.contacts[0].normal_world_on_b.z ),
                                     1.e-5f ) );
        }
        wp_rigidbody_remove_shape( a, 0 );
        wp_collision_shape_destroy( shape );
    }
    wp_collision_shape_set_local_position( ms, v( 0, 0, 0 ) );
    wp_collision_shape_set_local_orientation( ms, ( wp_quatf ){ 1, 0, 0, 0 } );
    wp_rigidbody_set_position( a, v( 0, .25f, 0 ) );
    wp_rigidbody_set_position( b, v( 0, 0, 0 ) );
    wp_rigidbody_set_orientation( a, ( wp_quatf ){ 1, 0, 0, 0 } );
    wp_rigidbody_set_orientation( b, ( wp_quatf ){ 1, 0, 0, 0 } );
    wp_collision_shape *box = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    CHECK( box );
    wp_collision_shape_set_box_half_extents( box, v( 19, .5f, 19 ) );
    wp_rigidbody_add_shape( a, box );
    wp_narrowphase_set_mesh_acceleration_enabled( np, 1 );
    wp_narrowphase_reset_stats( np );
    wp_contact_manifold m;
    CHECK( wp_narrowphase_test_pair( np, a, box, b, ms, &m ) );
    wp_narrowphase_stats ns = wp_narrowphase_get_stats( np );
    CHECK( ns.mesh_candidates > 1024 && ns.tested_triangles > 1024 && ns.full_scan_fallbacks == 0 );
    wp_s32 rebuilt;
    const wp_prepared_shape *prepared = wp_shape_prepare( b, ms, &rebuilt );
    float old_max = prepared->maximum.y;
    for ( int i = 0; i < nv; ++i )
        verts[i * 3 + 1] = 2;
    wp_triangle_mesh_refit_aabb( mesh );
    prepared = wp_shape_prepare( b, ms, &rebuilt );
    CHECK( rebuilt && prepared->maximum.y > old_max + 1 );
    wp_narrowphase_reset_stats( np );
    CHECK( !wp_narrowphase_test_pair( np, a, box, b, ms, &m ) );
    ids[0] = nv + 1;
    wp_triangle_mesh_refit_aabb( mesh );
    memset( seen, 0, nt );
    visit_result valid = { 0, seen, nt };
    wp_triangle_mesh_visit( mesh, &whole, collect_triangle, &valid, NULL );
    CHECK( valid.count == nt - 1 && !seen[0] );
    ids[0] = 0;
    ids[1] = 0;
    ids[2] = 0;
    wp_triangle_mesh_refit_aabb( mesh );
    memset( seen, 0, nt );
    valid.count = 0;
    wp_triangle_mesh_visit( mesh, &whole, collect_triangle, &valid, NULL );
    CHECK( valid.count == nt && seen[0] );
    float distance;
    wp_vec3f normal;
    int tri;
    CHECK( wp_triangle_mesh_raycast( mesh, v( 5, 5, 5 ), v( 0, -1, 0 ), 10, &distance, &normal,
                                     &tri ) );
    CHECK( fabsf( distance - 3 ) < 1.e-5f && normal.y > .9f && tri >= 0 && tri < nt );
    wp_narrowphase_destroy( np );
    wp_rigidbody_destroy( a );
    wp_rigidbody_destroy( b );
    wp_collision_shape_destroy( box );
    wp_collision_shape_destroy( ms );
    free( verts );
    free( ids );
    free( seen );
    free( simd );
    wp_triangle_mesh_destroy( NULL );
    return 1;
}
static int test_degenerate_mesh( void )
{
    float vertices[9] = { 0 };
    unsigned indices[3] = { 0, 1, 7 };
    wp_collision_mesh_data data = { vertices, 3, indices, 1 };
    wp_collision_shape *mesh_shape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_MESH );
    wp_collision_shape *sphere = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE );
    wp_narrowphase *np = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    wp_rigidbody *a = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC ),
                 *b = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    CHECK( np && a && b && sphere && mesh_shape );
    wp_collision_shape_set_mesh_data( mesh_shape, &data );
    wp_triangle_mesh *mesh = (wp_triangle_mesh *)wp_collision_shape_get_triangle_mesh( mesh_shape );
    CHECK( mesh );
    wp_triangle_mesh_query query = { 0 };
    query.sphere = 1;
    query.radius = 1;
    unsigned char seen[1] = { 0 };
    visit_result result = { 0, seen, 1 };
    wp_triangle_mesh_query_stats stats = { 0 };
    wp_triangle_mesh_visit( mesh, &query, collect_triangle, &result, &stats );
    CHECK( result.count == 0 && stats.full_scan_fallbacks == 1 );
    indices[2] = 2;
    wp_triangle_mesh_refit_aabb( mesh );
    wp_rigidbody_set_position( a, v( 0, .25f, 0 ) );
    wp_contact_manifold m;
    CHECK( wp_narrowphase_test_pair( np, a, sphere, b, mesh_shape, &m ) );
    CHECK( isfinite( m.contacts[0].penetration_depth ) );
    CHECK( fabsf( m.contacts[0].penetration_depth - .25f ) < 1.e-6f );
    CHECK( close_vector( m.contacts[0].normal_world_on_b, v( 0, -1, 0 ), 1.e-6f ) );
    wp_narrowphase_destroy( np );
    wp_rigidbody_destroy( a );
    wp_rigidbody_destroy( b );
    wp_collision_shape_destroy( sphere );
    wp_collision_shape_destroy( mesh_shape );
    return 1;
}

int main( void )
{
    int passed = 1;
    passed &= test_batch();
    passed &= test_hash_cache();
    passed &= test_scene_cache();
    passed &= test_scene_cache_growth_and_static_transitions();
    passed &= test_compound_culling();
    passed &= test_scalar_kernels();
    passed &= test_mesh();
    passed &= test_degenerate_mesh();
    if ( passed )
        puts( "All narrowphase optimization regressions passed." );
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
