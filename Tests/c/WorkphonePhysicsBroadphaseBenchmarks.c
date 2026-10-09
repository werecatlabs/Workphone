/* Optional measurements; correctness belongs in WorkphonePhysicsBroadphaseTests. */
#include <workphone_physics_broadphase.h>
#include <workphone_physics_scene.h>
#include <workphone_physics_rigidbody.h>
#include <workphone_physics_collisionshape.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#endif
#define CHECK( x )                                                    \
    do                                                                \
    {                                                                 \
        if( !( x ) )                                                  \
        {                                                             \
            fprintf( stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x ); \
            exit( 1 );                                                \
        }                                                             \
    } while( 0 )
static double now( void )
{
#ifdef _WIN32
    LARGE_INTEGER t, f;
    QueryPerformanceCounter( &t );
    QueryPerformanceFrequency( &f );
    return (double)t.QuadPart / (double)f.QuadPart;
#else
    struct timespec t;
    timespec_get( &t, TIME_UTC );
    return t.tv_sec + t.tv_nsec * 1e-9;
#endif
}
static void count_pair( const wp_broadphase_pair *pair, void *context )
{
    (void)pair;
    ++*(uint64_t *)context;
}
static int identities[8192];
static double tree_case( int count, int rounds, int pattern, int simd, uint64_t *pairs )
{
    wp_broadphase *bp = wp_broadphase_create( WORKPHONE_BROADPHASE_DBVT );
    wp_s32 *handles = (wp_s32 *)malloc( count * sizeof( *handles ) );
    wp_vec3f *mins = (wp_vec3f *)malloc( count * sizeof( *mins ) ),
             *maxs = (wp_vec3f *)malloc( count * sizeof( *maxs ) );
    int i, r;
    double start, end;
    CHECK( bp && handles && mins && maxs );
#ifndef WP_BENCHMARK_BASELINE
    wp_broadphase_set_simd_enabled( bp, simd );
#else
    (void)simd;
#endif
    for( i = 0; i < count; ++i )
    {
        mins[i] = (wp_vec3f){ (float)( i % 64 ) * 3, (float)( i / 64 ) * 3, 0 };
        maxs[i] = mins[i];
        maxs[i].x += 1;
        maxs[i].y += 1;
        maxs[i].z += 1;
        if( pattern == 1 )
        {
            mins[i] = (wp_vec3f){ 0, 0, 0 };
            maxs[i] = (wp_vec3f){ 1, 1, 1 };
        }
        if( pattern == 2 && i % 13 == 0 )
        {
            maxs[i].x += 40;
            maxs[i].y += 10;
        }
        handles[i] = wp_broadphase_create_proxy( bp, (wp_rigidbody *)&identities[i], mins[i], maxs[i] );
        CHECK( handles[i] >= 0 );
        if( pattern == 3 )
            wp_broadphase_configure_proxy( bp, handles[i], 1, i % 100 == 0, (wp_u32)i );
    }
    *pairs = 0;
    wp_broadphase_visit_pairs( bp, count_pair, pairs );
    *pairs = 0;
    start = now();
    for( r = 0; r < rounds; ++r )
    {
        if( pattern == 4 || pattern == 5 )
            for( i = 0; i < count; ++i )
            {
                float offset = pattern == 4 ? .03f : ( ( i + r ) % 2 == 0 ? 75.0f : -75.0f );
                mins[i].x += offset;
                maxs[i].x += offset;
                wp_broadphase_move_proxy( bp, handles[i], mins[i], maxs[i] );
            }
        wp_broadphase_visit_pairs( bp, count_pair, pairs );
    }
    end = now();
    wp_broadphase_destroy( bp );
    free( handles );
    free( mins );
    free( maxs );
    return ( end - start ) * 1000.0 / rounds;
}
static double scene_case( int count, int dynamic_stride )
{
    wp_physics_scene *scene = wp_physics_scene_create();
    wp_rigidbody **bodies = (wp_rigidbody **)malloc( count * sizeof( *bodies ) );
    wp_collision_shape **shapes = (wp_collision_shape **)malloc( count * sizeof( *shapes ) );
    int i, r;
    double start, end;
    CHECK( scene && bodies && shapes );
    wp_physics_scene_set_gravity( scene, (wp_vec3f){ 0, 0, 0 } );
    for( i = 0; i < count; ++i )
    {
        int moving = dynamic_stride && i % dynamic_stride == 0;
        bodies[i] =
            wp_rigidbody_create( moving ? WORKPHONE_RIGIDBODY_DYNAMIC : WORKPHONE_RIGIDBODY_STATIC );
        shapes[i] = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
        CHECK( bodies[i] && shapes[i] );
        wp_rigidbody_add_shape( bodies[i], shapes[i] );
        wp_rigidbody_set_position( bodies[i],
                                   (wp_vec3f){ (float)( i % 64 ) * 3, (float)( i / 64 ) * 3, 0 } );
        if( moving )
            wp_rigidbody_set_linear_velocity( bodies[i], (wp_vec3f){ .1f, 0, 0 } );
        CHECK( wp_physics_scene_add_actor( scene, bodies[i] ) );
    }
    wp_physics_scene_simulate( scene, 1.0f / 60.0f );
    start = now();
    for( r = 0; r < 80; ++r )
        wp_physics_scene_simulate( scene, 1.0f / 60.0f );
    end = now();
    wp_physics_scene_destroy( scene );
    for( i = 0; i < count; ++i )
    {
        wp_rigidbody_destroy( bodies[i] );
        wp_collision_shape_destroy( shapes[i] );
    }
    free( bodies );
    free( shapes );
    return ( end - start ) * 1000.0 / 80;
}
static int compare_double( const void *a, const void *b )
{
    double x = *(const double *)a, y = *(const double *)b;
    return ( x > y ) - ( x < y );
}
int main( void )
{
    const char *names[] = { "small",        "sparse",     "dense",    "mixed-sizes",
                            "static-heavy", "all-moving", "teleports" };
    const int counts[] = { 32, 4096, 512, 2048, 4096, 4096, 2048 };
    const int rounds[] = { 4000, 40, 10, 40, 40, 40, 40 };
    const int patterns[] = { 0, 0, 1, 2, 3, 4, 5 };
    int c, mode, repeat;
    puts( "Median milliseconds/pass, five fresh scenes; tree setup excluded. Pair counts must agree." );
    for( c = 0; c < 7; ++c )
    {
        uint64_t reference = 0;
        double values[2][5];
        int pass, modes = 2;
#ifdef WP_BENCHMARK_BASELINE
        modes = 1;
#endif
        printf( "%-13s n=%4d", names[c], counts[c] );
        for( repeat = 0; repeat < 5; ++repeat )
        {
            for( pass = 0; pass < modes; ++pass )
            {
                uint64_t pairs = 0;
                mode = modes == 1 ? 0 : ( repeat + pass ) % 2;
                values[mode][repeat] = tree_case( counts[c], rounds[c], patterns[c], mode, &pairs );
                if( repeat == 0 && pass == 0 )
                    reference = pairs;
                else
                    CHECK( pairs == reference );
            }
        }
        for( mode = 0; mode < modes; ++mode )
        {
            qsort( values[mode], 5, sizeof( values[mode][0] ), compare_double );
            printf( " %s=%.4f", mode ? "SSE2" : "scalar", values[mode][2] );
        }
        printf( " pairs/pass=%llu\n", (unsigned long long)( reference / rounds[c] ) );
    }
    for( mode = 0; mode < 2; ++mode )
    {
        double values[5];
        for( repeat = 0; repeat < 5; ++repeat )
            values[repeat] = scene_case( 4096, mode ? 100 : 0 );
        qsort( values, 5, sizeof( *values ), compare_double );
        printf( "Full scene %-12s n=4096 %.4f ms/step\n", mode ? "1% moving" : "all static", values[2] );
    }
    return 0;
}
