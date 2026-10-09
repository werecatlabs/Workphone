#include <workphone_physics_broadphase.h>
/* Both public headers must agree on the broadphase enum. */
#include <workphone_physics_solver.h>
#include <workphone_physics_scene.h>
#include <workphone_physics_rigidbody.h>
#include <workphone_physics_collisionshape.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CHECK( x )                                                    \
    do                                                                \
    {                                                                 \
        if( !( x ) )                                                  \
        {                                                             \
            fprintf( stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x ); \
            exit( 1 );                                                \
        }                                                             \
    } while( 0 )
typedef struct test_box
{
    wp_vec3f min, max;
    wp_s32 handle, enabled, movable, present;
} test_box;
/* The broadphase treats body pointers as opaque identities. */
static int identities[8192];
static wp_rigidbody *identity( int i )
{
    return (wp_rigidbody *)&identities[i];
}
static int index_of( wp_rigidbody *body )
{
    return (int)( (int *)body - identities );
}
static unsigned random_state = 12673;
static float random_float( void )
{
    random_state = random_state * 1664525u + 1013904223u;
    return (float)( random_state >> 8 ) / (float)0xffffff;
}
static int box_overlap( const test_box *a, const test_box *b )
{
    return !( a->max.x < b->min.x || a->min.x > b->max.x || a->max.y < b->min.y || a->min.y > b->max.y ||
              a->max.z < b->min.z || a->min.z > b->max.z );
}
static void random_box( test_box *a )
{
    float x = random_float() * 20 - 10, y = random_float() * 20 - 10, z = random_float() * 20 - 10;
    a->min.x = x;
    a->min.y = y;
    a->min.z = z;
    a->max.x = x + random_float() * 4;
    a->max.y = y + random_float() * 4;
    a->max.z = z + random_float() * 4;
}
static void verify( wp_broadphase *bp, test_box *boxes, int count )
{
    int i, j, expected = 0, actual = 0;
    const wp_broadphase_pair *pairs;
    wp_rigidbody *results[1024];
    unsigned char seen[1024] = { 0 };
    test_box query;
    CHECK( count <= 1024 );
    CHECK( wp_broadphase_calculate_overlapping_pairs_checked( bp ) );
    actual = wp_broadphase_get_pair_count( bp );
    pairs = wp_broadphase_get_pair_cache( bp );
    for( i = 0; i < count; ++i )
        for( j = i + 1; j < count; ++j )
        {
            if( !boxes[i].present || !boxes[j].present || !boxes[i].enabled || !boxes[j].enabled ||
                !( boxes[i].movable || boxes[j].movable ) || !box_overlap( &boxes[i], &boxes[j] ) )
                continue;
            CHECK( expected < actual );
            CHECK( pairs[expected].body_a == identity( i ) && pairs[expected].body_b == identity( j ) );
            ++expected;
        }
    CHECK( actual == expected );
    random_box( &query );
    actual = wp_broadphase_query_aabb( bp, query.min, query.max, results, count );
    for( i = 0; i < actual; ++i )
    {
        j = index_of( results[i] );
        CHECK( j >= 0 && j < count && !seen[j] );
        seen[j] = 1;
    }
    for( i = 0; i < count; ++i )
        CHECK( seen[i] == ( boxes[i].present && boxes[i].enabled && box_overlap( &boxes[i], &query ) ) );
}
static void test_random_updates( void )
{
    enum
    {
        count = 700
    };
    test_box boxes[count];
    int i, frame;
    wp_broadphase *bp = wp_broadphase_create( WORKPHONE_BROADPHASE_DBVT );
    CHECK( bp );
    memset( boxes, 0, sizeof( boxes ) );
    for( i = 0; i < count; ++i )
    {
        random_box( &boxes[i] );
        boxes[i].enabled = boxes[i].movable = boxes[i].present = 1;
        boxes[i].handle = wp_broadphase_create_proxy( bp, identity( i ), boxes[i].min, boxes[i].max );
        CHECK( boxes[i].handle >= 0 );
        wp_broadphase_configure_proxy( bp, boxes[i].handle, 1, 1, (wp_u32)i );
    }
    CHECK( wp_broadphase_get_proxy_count( bp ) == count );
    for( frame = 0; frame < 50; ++frame )
    {
        for( i = 0; i < count; ++i )
        {
            if( ( i + frame ) % 11 == 0 )
            {
                if( boxes[i].present )
                {
                    wp_broadphase_destroy_proxy( bp, boxes[i].handle );
                    boxes[i].present = 0;
                }
                else
                {
                    boxes[i].handle =
                        wp_broadphase_create_proxy( bp, identity( i ), boxes[i].min, boxes[i].max );
                    CHECK( boxes[i].handle >= 0 );
                    boxes[i].present = 1;
                }
            }
            if( !boxes[i].present )
                continue;
            if( ( i + frame ) % 3 == 0 )
                random_box( &boxes[i] );
            else
            {
                boxes[i].min.x += .001f;
                boxes[i].max.x += .001f;
            }
            boxes[i].enabled = ( i + frame ) % 7 != 0;
            boxes[i].movable = ( i + frame ) % 4 != 0;
            wp_broadphase_move_proxy( bp, boxes[i].handle, boxes[i].min, boxes[i].max );
            wp_broadphase_configure_proxy( bp, boxes[i].handle, boxes[i].enabled, boxes[i].movable,
                                           (wp_u32)i );
        }
        verify( bp, boxes, count );
    }
    wp_broadphase_clear( bp );
    CHECK( wp_broadphase_get_proxy_count( bp ) == 0 );
    CHECK( wp_broadphase_calculate_overlapping_pairs_checked( bp ) );
    CHECK( wp_broadphase_get_pair_count( bp ) == 0 );
    for( i = 0; i < 3; ++i )
        CHECK(
            wp_broadphase_add_proxy( bp, identity( i ), (wp_vec3f){ 0, 0, 0 }, (wp_vec3f){ 1, 1, 1 } ) );
    CHECK( wp_broadphase_add_proxy( bp, identity( 0 ), (wp_vec3f){ 0, 0, 0 }, (wp_vec3f){ 1, 1, 1 } ) );
    CHECK( wp_broadphase_get_proxy_count( bp ) == 3 );
    wp_broadphase_remove_proxy( bp, identity( 1 ) );
    CHECK( wp_broadphase_calculate_overlapping_pairs_checked( bp ) );
    CHECK( wp_broadphase_get_pair_count( bp ) == 1 );
    wp_broadphase_destroy( bp );
}
static void test_dense_and_extreme_bounds( void )
{
    int i;
    wp_rigidbody *out[80];
    wp_broadphase *bp = wp_broadphase_create( WORKPHONE_BROADPHASE_DBVT );
    wp_vec3f zero = { 0, 0, 0 }, one = { 1, 1, 1 };
    CHECK( bp );
    for( i = 0; i < 80; ++i )
        CHECK( wp_broadphase_add_proxy( bp, identity( i ), zero, one ) );
    CHECK( wp_broadphase_calculate_overlapping_pairs_checked( bp ) );
    CHECK( wp_broadphase_get_pair_count( bp ) == 80 * 79 / 2 );
    CHECK( wp_broadphase_query_aabb( bp, one, one, out, 80 ) == 80 ); /* inclusive touching */
    wp_broadphase_clear( bp );
    CHECK( wp_broadphase_add_proxy( bp, identity( 0 ), (wp_vec3f){ -FLT_MAX, -FLT_MAX, -FLT_MAX },
                                    (wp_vec3f){ FLT_MAX, FLT_MAX, FLT_MAX } ) );
    for( i = 1; i < 80; ++i )
    {
        wp_vec3f min = { (float)i * 3, 0, 0 }, max = { (float)i * 3 + 1, 1, 1 };
        CHECK( wp_broadphase_add_proxy( bp, identity( i ), min, max ) );
    }
    CHECK( wp_broadphase_calculate_overlapping_pairs_checked( bp ) );
    CHECK( wp_broadphase_get_pair_count( bp ) == 79 );
    wp_broadphase_update_proxy( bp, identity( 2 ), (wp_vec3f){ 3.05f, 0, 0 },
                                (wp_vec3f){ 3.08f, 1, 1 } );
    CHECK( wp_broadphase_calculate_overlapping_pairs_checked( bp ) );
    CHECK( wp_broadphase_get_pair_count( bp ) == 80 );
    wp_broadphase_update_proxy( bp, identity( 2 ), (wp_vec3f){ 4.05f, 0, 0 },
                                (wp_vec3f){ 4.08f, 1, 1 } );
    CHECK( wp_broadphase_calculate_overlapping_pairs_checked( bp ) );
    CHECK( wp_broadphase_get_pair_count( bp ) == 79 );
    CHECK( !wp_broadphase_add_proxy( bp, identity( 90 ), one, zero ) );
    wp_broadphase_destroy( bp );
}
static void test_small_motion_and_order( void )
{
    wp_broadphase *bp = wp_broadphase_create( WORKPHONE_BROADPHASE_DBVT );
    wp_s32 a, b;
    const wp_broadphase_pair *pair;
    CHECK( bp );
    a = wp_broadphase_create_proxy( bp, identity( 0 ), (wp_vec3f){ 0, 0, 0 }, (wp_vec3f){ 1, 1, 1 } );
    b = wp_broadphase_create_proxy( bp, identity( 1 ), (wp_vec3f){ 1.05f, 0, 0 },
                                    (wp_vec3f){ 2.05f, 1, 1 } );
    CHECK( a >= 0 && b >= 0 );
    CHECK( wp_broadphase_calculate_overlapping_pairs_checked( bp ) );
    CHECK( wp_broadphase_get_pair_count( bp ) == 0 );
    wp_broadphase_configure_proxy( bp, a, 1, 1, 10 );
    wp_broadphase_configure_proxy( bp, b, 1, 1, 2 );
    /* This move stays inside the fat AABB, but creates an exact overlap. */
    wp_broadphase_move_proxy( bp, b, (wp_vec3f){ .99f, 0, 0 }, (wp_vec3f){ 1.99f, 1, 1 } );
    CHECK( wp_broadphase_calculate_overlapping_pairs_checked( bp ) );
    CHECK( wp_broadphase_get_pair_count( bp ) == 1 );
    pair = wp_broadphase_get_pair_cache( bp );
    CHECK( pair[0].body_a == identity( 1 ) && pair[0].body_b == identity( 0 ) );
    wp_broadphase_move_proxy( bp, b, (wp_vec3f){ 1.01f, 0, 0 }, (wp_vec3f){ 2.01f, 1, 1 } );
    CHECK( wp_broadphase_calculate_overlapping_pairs_checked( bp ) );
    CHECK( wp_broadphase_get_pair_count( bp ) == 0 );
    wp_broadphase_destroy( bp );
}
static void test_scene_selections( void )
{
    int method;
    for( method = WP_SPATIAL_PARTITION_NONE; method < WP_SPATIAL_PARTITION_COUNT; ++method )
    {
        wp_physics_scene *scene = wp_physics_scene_create();
        wp_rigidbody *a = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
        wp_rigidbody *b = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
        wp_collision_shape *sa = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
        wp_collision_shape *sb = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
        wp_vec3f p;
        CHECK( scene && a && b && sa && sb );
        wp_physics_scene_set_spatial_partitioning( scene, (wp_spatial_partitioning_method)method );
        wp_physics_scene_set_gravity( scene, (wp_vec3f){ 0, 0, 0 } );
        wp_collision_shape_set_box_half_extents( sa, (wp_vec3f){ .5f, .5f, .5f } );
        wp_collision_shape_set_box_half_extents( sb, (wp_vec3f){ .5f, .5f, .5f } );
        wp_rigidbody_add_shape( a, sa );
        wp_rigidbody_add_shape( b, sb );
        wp_rigidbody_set_position( b, (wp_vec3f){ 10, 0, 0 } );
        CHECK( wp_physics_scene_add_actor( scene, a ) );
        CHECK( wp_physics_scene_add_actor( scene, b ) );
        wp_physics_scene_simulate( scene, 1.0f / 60.0f ); /* establishes old static bounds */
        wp_rigidbody_put_to_sleep( a );
        wp_rigidbody_set_position( b, (wp_vec3f){ .5f, 0, 0 } );
        wp_physics_scene_simulate( scene, 1.0f / 60.0f );
        p = wp_rigidbody_get_position( a );
        CHECK( p.x < -.1f );
        wp_physics_scene_clear( scene );
        wp_rigidbody_set_position( a, (wp_vec3f){ 0, 0, 0 } );
        wp_rigidbody_set_position( b, (wp_vec3f){ .5f, 0, 0 } );
        CHECK( wp_physics_scene_add_actor( scene, b ) );
        CHECK( wp_physics_scene_add_actor( scene, a ) );
        wp_physics_scene_simulate( scene, 1.0f / 60.0f );
        CHECK( wp_rigidbody_get_position( a ).x < -.1f );
        wp_rigidbody_set_position( a, (wp_vec3f){ 0, 0, 0 } );
        wp_rigidbody_set_flag( a, WORKPHONE_RIGIDBODY_FLAG_ENABLED, 0 );
        wp_physics_scene_simulate( scene, 1.0f / 60.0f );
        CHECK( wp_rigidbody_get_position( a ).x == 0 );
        wp_rigidbody_set_flag( a, WORKPHONE_RIGIDBODY_FLAG_ENABLED, 1 );
        wp_collision_shape_set_enabled( sb, 0 );
        wp_physics_scene_simulate( scene, 1.0f / 60.0f );
        CHECK( wp_rigidbody_get_position( a ).x == 0 );
        wp_collision_shape_set_enabled( sb, 1 );
        wp_collision_shape_set_local_position( sb, (wp_vec3f){ 10, 0, 0 } );
        wp_physics_scene_simulate( scene, 1.0f / 60.0f );
        CHECK( wp_rigidbody_get_position( a ).x == 0 );
        wp_collision_shape_set_local_position( sb, (wp_vec3f){ 0, 0, 0 } );
        wp_rigidbody_set_collision_type( a, 1 );
        wp_rigidbody_set_collision_mask( a, 0 );
        wp_rigidbody_set_collision_type( b, 2 );
        wp_rigidbody_set_collision_mask( b, 0 );
        wp_physics_scene_simulate( scene, 1.0f / 60.0f );
        CHECK( wp_rigidbody_get_position( a ).x == 0 );
        /* Existing filtering accepts either body's mask, rather than requiring both. */
        wp_rigidbody_set_collision_mask( a, 2 );
        wp_collision_shape_set_trigger( sb, 1 );
        wp_physics_scene_simulate( scene, 1.0f / 60.0f );
        CHECK( wp_rigidbody_get_position( a ).x == 0 );
        wp_collision_shape_set_trigger( sb, 0 );
        wp_physics_scene_simulate( scene, 1.0f / 60.0f );
        CHECK( wp_rigidbody_get_position( a ).x < -.1f );
        wp_physics_scene_remove_actor( scene, b );
        wp_rigidbody_destroy( b );
        wp_collision_shape_destroy( sb );
        wp_physics_scene_simulate( scene, 1.0f / 60.0f );
        wp_physics_scene_destroy( scene );
        wp_rigidbody_destroy( a );
        wp_collision_shape_destroy( sa );
    }
}
static void count_pair( const wp_broadphase_pair *pair, void *context )
{
    (void)pair;
    ++*(int *)context;
}
static void benchmark( void )
{
    enum
    {
        count = 4096,
        rounds = 40
    };
    test_box *boxes = (test_box *)calloc( count, sizeof( *boxes ) );
    wp_broadphase *bp = wp_broadphase_create( WORKPHONE_BROADPHASE_DBVT );
    int i, j, r, hits = 0;
    volatile int brute_hits = 0;
    clock_t start, tree_end, moving_end, brute_end;
    CHECK( bp && boxes );
    for( i = 0; i < count; ++i )
    {
        boxes[i].min = (wp_vec3f){ (float)( i % 64 ) * 3, (float)( i / 64 ) * 3, 0 };
        boxes[i].max = boxes[i].min;
        boxes[i].max.x += 1;
        boxes[i].max.y += 1;
        boxes[i].max.z += 1;
        boxes[i].handle = wp_broadphase_create_proxy( bp, identity( i ), boxes[i].min, boxes[i].max );
        CHECK( boxes[i].handle >= 0 );
    }
    start = clock();
    for( r = 0; r < rounds; ++r )
        wp_broadphase_visit_pairs( bp, count_pair, &hits );
    tree_end = clock();
    for( r = 0; r < rounds; ++r )
    {
        for( i = 0; i < count; ++i )
        {
            boxes[i].min.x += .03f;
            boxes[i].max.x += .03f;
            wp_broadphase_move_proxy( bp, boxes[i].handle, boxes[i].min, boxes[i].max );
        }
        wp_broadphase_visit_pairs( bp, count_pair, &hits );
    }
    moving_end = clock();
    for( r = 0; r < rounds; ++r )
    {
        for( i = 0; i < count; ++i )
        {
            boxes[i].min.x += .03f;
            boxes[i].max.x += .03f;
        }
        for( i = 0; i < count; ++i )
            for( j = i + 1; j < count; ++j )
                if( box_overlap( &boxes[i], &boxes[j] ) )
                    ++brute_hits;
    }
    brute_end = clock();
    CHECK( hits == 0 && brute_hits == 0 );
    printf(
        "Synthetic sparse scene, %d bodies x %d rounds: stationary tree %.2f ms, moving tree %.2f ms, "
        "all-pairs %.2f ms\n",
        count, rounds, 1000.0 * ( tree_end - start ) / CLOCKS_PER_SEC,
        1000.0 * ( moving_end - tree_end ) / CLOCKS_PER_SEC,
        1000.0 * ( brute_end - moving_end ) / CLOCKS_PER_SEC );
    wp_broadphase_destroy( bp );
    free( boxes );
}
int main( int argc, char **argv )
{
    test_random_updates();
    test_dense_and_extreme_bounds();
    test_small_motion_and_order();
    test_scene_selections();
    puts(
        "Broadphase oracle, growth, dense pairs, touching, plane bounds, moves, removal, reuse and "
        "scene selections: passed." );
    if( argc > 1 && strcmp( argv[1], "--benchmark" ) == 0 )
        benchmark();
    return 0;
}
