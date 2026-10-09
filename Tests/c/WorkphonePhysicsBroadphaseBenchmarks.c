/* Optional measurements; correctness belongs in WorkphonePhysicsBroadphaseTests. */
#include <workphone_physics_broadphase.h>
#include <workphone_physics_scene.h>
#include <workphone_physics_rigidbody.h>
#include <workphone_physics_collisionshape.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
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
#ifndef WP_BENCHMARK_BASELINE
static double oriented_scene_case( int enabled, int pattern, wp_scene_broadphase_stats *stats )
{
    enum { count = 128, rounds = 160 };
    wp_physics_scene *scene = wp_physics_scene_create();
    wp_rigidbody *bodies[count];
    wp_f32 vertices[] = {-10,-1,0, 10,-1,0, 10,1,0, -10,1,0};
    wp_u32 indices[] = {0,1,2, 0,2,3};
    wp_collision_mesh_data mesh = {vertices, 4, indices, 2};
    double start, end;
    CHECK(scene);
    wp_physics_scene_set_gravity(scene, (wp_vec3f){0,0,0});
    wp_physics_scene_set_broadphase_obb_enabled(scene, enabled);
    for(int i = 0; i < count; ++i)
    {
        float spacing = pattern == 1 ? 1.5f : 3;
        int children = pattern == 2 ? 2 : 1;
        bodies[i] = wp_rigidbody_create(i == 64 ? WORKPHONE_RIGIDBODY_DYNAMIC : WORKPHONE_RIGIDBODY_STATIC);
        CHECK(bodies[i]);
        for(int child = 0; child < children; ++child)
        {
            int is_mesh = pattern == 4 && i != 64;
            wp_collision_shape *shape = wp_collision_shape_create(is_mesh ? WORKPHONE_COLLISION_SHAPE_MESH : WORKPHONE_COLLISION_SHAPE_BOX);
            CHECK(shape);
            if(is_mesh) wp_collision_shape_set_mesh_data(shape, &mesh);
            else wp_collision_shape_set_box_half_extents(shape, (wp_vec3f){pattern == 2 ? 1.0f : 10.0f,1,1});
            if(pattern == 2) wp_collision_shape_set_local_position(shape, (wp_vec3f){child ? 9.0f : -9.0f,0,0});
            wp_collision_shape_set_trigger(shape, 1);
            CHECK(wp_rigidbody_add_shape(bodies[i], shape) >= 0);
        }
        if(pattern != 3)
        {
            wp_rigidbody_set_orientation(bodies[i], (wp_quatf){.92387953f,0,0,.38268343f});
            wp_rigidbody_set_position(bodies[i], (wp_vec3f){-(i-64)*spacing*.70710678f,(i-64)*spacing*.70710678f,0});
        }
        CHECK(wp_physics_scene_add_actor(scene, bodies[i]));
    }
    wp_physics_scene_simulate(scene, 1.0f/60);
    start = now();
    for(int r = 0; r < rounds; ++r) wp_physics_scene_simulate(scene, 1.0f/60);
    end = now();
    *stats = wp_physics_scene_get_broadphase_stats(scene);
    wp_physics_scene_destroy(scene);
    for(int i = 0; i < count; ++i)
    {
        while(wp_rigidbody_get_shape_count(bodies[i]))
            wp_collision_shape_destroy(wp_rigidbody_get_shape(bodies[i], 0));
        wp_rigidbody_destroy(bodies[i]);
    }
    return (end-start)*1000.0/rounds;
}
#endif
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
#ifndef WP_BENCHMARK_BASELINE
    puts("OBB full scene, 1 dynamic/127 static, trigger shapes keep fixtures fixed; five-run median ms/step.");
    for(int pattern = 0; pattern < 5; ++pattern)
    {
        const char *labels[] = {"rotated bars", "rotated contacts", "rotated compounds", "aligned dense", "rotated mesh"};
        double values[2][5];
        wp_scene_broadphase_stats stats[2];
        for(int repeat = 0; repeat < 5; ++repeat)
            for(int pass = 0; pass < 2; ++pass)
            {
                int enabled = (repeat+pass)%2;
                values[enabled][repeat] = oriented_scene_case(enabled, pattern, &stats[enabled]);
            }
        CHECK(stats[0].candidate_pairs == stats[1].candidate_pairs);
        printf("%-18s", labels[pattern]);
        for(int enabled = 0; enabled < 2; ++enabled)
        {
            qsort(values[enabled], 5, sizeof(double), compare_double);
            printf(" %s=%.4f", enabled ? "OBB" : "AABB", values[enabled][2]);
        }
        printf(" pairs=%llu rejected=%llu narrowphase=%llu->%llu\n",
               (unsigned long long)stats[1].candidate_pairs, (unsigned long long)stats[1].obb_rejections,
               (unsigned long long)stats[0].narrowphase_tests, (unsigned long long)stats[1].narrowphase_tests);
    }
#endif
    return 0;
}
