#include <workphone_physics_narrowphase.h>
#include <workphone_physics_scene.h>
#include <workphone_physics_rigidbody.h>
#include <workphone_physics_collisionshape.h>
#include <workphone_physics_triangle_mesh.h>
#include <time.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define CHECK( x )                                                                                 \
    do                                                                                             \
    {                                                                                              \
        if ( !( x ) )                                                                              \
        {                                                                                          \
            fprintf( stderr, "check failed at %d: %s\n", __LINE__, #x );                           \
            exit( 1 );                                                                             \
        }                                                                                          \
    } while ( 0 )
static volatile unsigned checksum;
static wp_vec3f v( float x, float y, float z )
{
    return ( wp_vec3f ){ x, y, z };
}
static double seconds( void )
{
#ifdef _WIN32
    LARGE_INTEGER t, f;
    QueryPerformanceCounter( &t );
    QueryPerformanceFrequency( &f );
    return (double)t.QuadPart / f.QuadPart;
#else
    struct timespec t;
    timespec_get( &t, TIME_UTC );
    return t.tv_sec + t.tv_nsec * 1.e-9;
#endif
}
static int cmp( const void *a, const void *b )
{
    double x = *(const double *)a, y = *(const double *)b;
    return ( x > y ) - ( x < y );
}
static double time_pair( const char *name, wp_narrowphase *np, wp_rigidbody *a,
                         wp_collision_shape *sa, wp_rigidbody *b, wp_collision_shape *sb,
                         int rounds )
{
    double samples[5];
    int r, i;
    unsigned hits = 0;
    wp_contact_manifold m;
    for ( i = 0; i < 100; i++ )
        wp_narrowphase_test_pair( np, a, sa, b, sb, &m );
    for ( r = 0; r < 5; r++ )
    {
        double start = seconds();
        hits = 0;
        for ( i = 0; i < rounds; i++ )
        {
            hits += wp_narrowphase_test_pair( np, a, sa, b, sb, &m );
            checksum += (unsigned)m.contact_count;
        }
        samples[r] = ( seconds() - start ) * 1e6 / rounds;
    }
    qsort( samples, 5, sizeof( double ), cmp );
    printf( "%-27s %.4f us/pair hits=%u/%d", name, samples[2], hits, rounds );
    wp_narrowphase_reset_stats( np );
    wp_narrowphase_test_pair( np, a, sa, b, sb, &m );
    wp_narrowphase_stats st = wp_narrowphase_get_stats( np );
    printf( " nodes=%llu candidates=%llu tested=%llu fallback=%llu SSE=%llu OBB-reject=%llu\n",
            (unsigned long long)st.mesh_nodes, (unsigned long long)st.mesh_candidates,
            (unsigned long long)st.tested_triangles, (unsigned long long)st.full_scan_fallbacks,
            (unsigned long long)st.simd_batches, (unsigned long long)st.oriented_rejections );
    return samples[2];
}
static void primitive_times( void )
{
    wp_narrowphase *np = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    wp_rigidbody *a = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC ),
                 *b = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *sphere = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE ),
                       *box = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX ),
                       *capsule = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_CAPSULE );
    wp_collision_shape *other = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX ),
                       *sphere_b = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE );
    CHECK( np && a && b && sphere && box && capsule && other && sphere_b );
    wp_collision_shape_set_capsule( capsule, .5f, 1 );
    wp_rigidbody_set_position( b, v( .8f, 0, 0 ) );
    time_pair( "sphere/sphere hit", np, a, sphere, b, sphere_b, 300000 );
    time_pair( "sphere/box hit", np, a, sphere, b, other, 300000 );
    wp_rigidbody_set_orientation( b, ( wp_quatf ){ .92387953f, 0, .38268343f, 0 } );
    time_pair( "box/box rotated hit", np, a, box, b, other, 300000 );
    time_pair( "capsule/box rotated hit", np, a, capsule, b, other, 300000 );
    wp_rigidbody_set_position( b, v( 2, 0, 0 ) );
    time_pair( "capsule/box miss", np, a, capsule, b, other, 300000 );
    wp_rigidbody_destroy( a );
    wp_rigidbody_destroy( b );
    wp_collision_shape_destroy( sphere );
    wp_collision_shape_destroy( box );
    wp_collision_shape_destroy( capsule );
    wp_collision_shape_destroy( other );
    wp_collision_shape_destroy( sphere_b );
    wp_narrowphase_destroy( np );
}
static unsigned mesh_aabb_candidates( const wp_collision_mesh_data *data, float hx )
{
    unsigned t, count = 0, k;
    const float minv[3] = { -hx, -.251f, -.501f }, maxv[3] = { hx, .751f, .501f };
    for ( t = 0; t < data->triangle_count; t++ )
    {
        int overlap = 1;
        for ( k = 0; k < 3; k++ )
        {
            float lo = 1e30f, hi = -1e30f;
            unsigned j;
            for ( j = 0; j < 3; j++ )
            {
                float f = data->vertices[data->indices[t * 3 + j] * 3 + k];
                if ( f < lo )
                    lo = f;
                if ( f > hi )
                    hi = f;
            }
            if ( lo > maxv[k] || hi < minv[k] )
                overlap = 0;
        }
        count += overlap;
    }
    return count;
}
static void mesh_times( void )
{
    const unsigned n = 128, row = n + 1, nt = n * n * 2, nv = row * row;
    unsigned x, z, t = 0;
    float *verts = (float *)malloc( nv * 3 * sizeof( float ) );
    unsigned *inds = (unsigned *)malloc( nt * 3 * sizeof( unsigned ) );
    wp_narrowphase *np = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    wp_rigidbody *a = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC ),
                 *b = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *box = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX ),
                       *ms = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_MESH );
    wp_collision_mesh_data data;
    const wp_triangle_mesh *mesh;
    float below = 0, above = 0, r;
    unsigned cb = 0, ca = 0;
    CHECK( verts && inds && np && a && b && box && ms );
    for ( z = 0; z < row; z++ )
        for ( x = 0; x < row; x++ )
        {
            unsigned i = ( z * row + x ) * 3;
            verts[i] = (float)x - 64;
            verts[i + 1] = 0;
            verts[i + 2] = (float)z - 64;
        }
    for ( z = 0; z < n; z++ )
        for ( x = 0; x < n; x++ )
        {
            unsigned k = z * row + x;
            inds[t++] = k;
            inds[t++] = k + row;
            inds[t++] = k + 1;
            inds[t++] = k + 1;
            inds[t++] = k + row;
            inds[t++] = k + row + 1;
        }
    data = ( wp_collision_mesh_data ){ verts, nv, inds, nt };
    {
        double start = seconds();
        wp_collision_shape_set_mesh_data( ms, &data );
        printf( "Mesh cooking %u triangles: %.3f ms\n", nt, ( seconds() - start ) * 1000 );
    }
    mesh = wp_collision_shape_get_triangle_mesh( ms );
    CHECK( mesh );
    wp_rigidbody_set_position( a, v( 0, .25f, 0 ) );
    wp_collision_shape_set_box_half_extents( box, v( .5f, .5f, .5f ) );
    time_pair( "box/mesh small", np, a, box, b, ms, 3000 );
    for ( r = 1; r < 25; r += .125f )
    {
        unsigned count = wp_triangle_mesh_query_sphere( mesh, v( 0, .25f, 0 ), r, NULL, 0 );
        if ( count <= 1024 )
        {
            below = r;
            cb = count;
        }
        else
        {
            above = r;
            ca = count;
            break;
        }
    }
    CHECK( below > 0 && above > 0 );
    printf( "Mesh query threshold: r=%.3f -> %u candidates; r=%.3f -> %u candidates\n", below, cb,
            above, ca );
    printf(
        "Tight box AABB candidates: below=%u above=%u\n",
        mesh_aabb_candidates( &data, sqrtf( ( below - .001f ) * ( below - .001f ) - .5f ) + .001f ),
        mesh_aabb_candidates( &data,
                              sqrtf( ( above - .001f ) * ( above - .001f ) - .5f ) + .001f ) );
    wp_collision_shape_set_box_half_extents(
        box, v( sqrtf( ( below - .001f ) * ( below - .001f ) - .5f ), .5f, .5f ) );
    time_pair( "box/mesh below capacity", np, a, box, b, ms, 100 );
    wp_collision_shape_set_box_half_extents(
        box, v( sqrtf( ( above - .001f ) * ( above - .001f ) - .5f ), .5f, .5f ) );
    time_pair( "box/mesh above capacity", np, a, box, b, ms, 100 );
    wp_rigidbody_set_orientation( a, ( wp_quatf ){ .92387953f, 0, .38268343f, 0 } );
    for ( int mode = 0; mode < 3; ++mode )
    {
        wp_narrowphase_set_simd_enabled( np, mode == 1 );
        wp_narrowphase_set_mesh_obb_enabled( np, mode == 2 );
        time_pair( mode == 0   ? "rotated long mesh scalar"
                   : mode == 1 ? "rotated long mesh SSE2"
                               : "rotated long mesh OBB",
                   np, a, box, b, ms, 300 );
    }
    wp_rigidbody_set_orientation( a, ( wp_quatf ){ 1, 0, 0, 0 } );
    wp_collision_shape_set_box_half_extents( box, v( 30, .5f, 30 ) );
    for ( int mode = 0; mode < 2; ++mode )
    {
        wp_narrowphase_set_simd_enabled( np, mode );
        wp_narrowphase_set_mesh_obb_enabled( np, 0 );
        time_pair( mode ? "dense mesh SSE2" : "dense mesh scalar", np, a, box, b, ms, 30 );
    }
    wp_rigidbody_destroy( a );
    wp_rigidbody_destroy( b );
    wp_collision_shape_destroy( box );
    wp_collision_shape_destroy( ms );
    wp_narrowphase_destroy( np );
    free( verts );
    free( inds );
}

static void scene_times( const char *name, int count, int moving, int sleeping )
{
    wp_physics_scene *sc = wp_physics_scene_create();
    CHECK( sc );
    wp_rigidbody **b = (wp_rigidbody **)calloc( count, sizeof( *b ) );
    wp_collision_shape **s = (wp_collision_shape **)calloc( count, sizeof( *s ) );
    CHECK( b && s );
    wp_physics_scene_set_gravity( sc, v( 0, 0, 0 ) );
    for ( int i = 0; i < count; ++i )
    {
        b[i] = wp_rigidbody_create( i < moving ? WORKPHONE_RIGIDBODY_DYNAMIC
                                               : WORKPHONE_RIGIDBODY_STATIC );
        s[i] = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
        CHECK( b[i] && s[i] );
        wp_rigidbody_add_shape( b[i], s[i] );
        wp_rigidbody_set_position( b[i], v( (float)( i % 64 ) * 3, 0, (float)( i / 64 ) * 3 ) );
        if ( sleeping && i < moving )
            wp_rigidbody_put_to_sleep( b[i] );
        CHECK( wp_physics_scene_add_actor( sc, b[i] ) );
    }
    wp_physics_scene_simulate( sc, 1.f / 60 );
    double samples[5];
    for ( int r = 0; r < 5; ++r )
    {
        double start = seconds();
        for ( int i = 0; i < 200; ++i )
            wp_physics_scene_simulate( sc, 1.f / 60 );
        samples[r] = ( seconds() - start ) * 1.e6 / 200;
    }
    qsort( samples, 5, sizeof( double ), cmp );
    wp_scene_broadphase_stats st = wp_physics_scene_get_broadphase_stats( sc );
    printf( "%-27s %.4f us/step bounds-reused=%llu tests=%llu\n", name, samples[2],
            (unsigned long long)st.bounds_reuses, (unsigned long long)st.narrowphase_tests );
    wp_physics_scene_destroy( sc );
    for ( int i = 0; i < count; ++i )
    {
        wp_rigidbody_destroy( b[i] );
        wp_collision_shape_destroy( s[i] );
    }
    free( b );
    free( s );
}
int main( void )
{
    puts( "Narrowphase benchmarks: median of five samples, setup excluded; timing assertions are "
          "not tests." );
    primitive_times();
    mesh_times();
    scene_times( "all-static 4096", 4096, 0, 0 );
    scene_times( "one dynamic + 4095 static", 4096, 1, 0 );
    scene_times( "256 sleeping + 3840 static", 4096, 256, 1 );
    printf( "checksum=%u\n", checksum );
    return 0;
}
