#include <stdio.h>
#include <stdlib.h>
#include <workphone_graphics_mesh.h>
#include <workphone_graphics_mesh_reader.h>
#include <workphone_graphics_mesh_writer.h>

static int test_wpm1_round_trip( void )
{
    const wp_graphics_mesh_vertex_p vertices[3] = {
        { { 0.0f, 0.0f, 0.0f } },
        { { 1.0f, 0.0f, 0.0f } },
        { { 0.0f, 1.0f, 0.0f } }
    };
    const uint16_t indices[3] = { 0u, 1u, 2u };
    wp_graphics_mesh *source;
    wp_graphics_mesh *loaded;
    void *data;
    wp_u32 size;
    source = wp_graphics_mesh_create();
    if( !source || !wp_graphics_mesh_set_vertices(
                       source, WORKPHONE_VERTEX_FORMAT_P, vertices, 3u ) ||
        !wp_graphics_mesh_set_indices_u16( source, indices, 3u ) ||
        wp_graphics_mesh_add_submesh( source, 0u, 3u, 42u ) < 0 ||
        !wp_graphics_mesh_write_to_buffer( source, &data, &size ) )
    {
        wp_graphics_mesh_destroy( source );
        return 0;
    }
    loaded = wp_graphics_mesh_read_from_buffer( data, size );
    free( data );
    wp_graphics_mesh_destroy( source );
    if( !loaded || wp_graphics_mesh_get_vertex_count( loaded ) != 3u ||
        wp_graphics_mesh_get_index_count( loaded ) != 3u ||
        wp_graphics_mesh_get_submesh_count( loaded ) != 1 )
    {
        wp_graphics_mesh_destroy( loaded );
        return 0;
    }
    wp_graphics_mesh_destroy( loaded );
    return 1;
}

static int test_native_lit_preparation( void )
{
    const wp_graphics_mesh_vertex_pt positions[] = {
        { {0,0,0}, {0,0} }, { {1,0,0}, {1,0} },
        { {0,1,0}, {0,1} }, { {1,1,0}, {1,1} }
    };
    const uint16_t strip[] = {0,1,2,3};
    const wp_u32 reversed[] = {0,2,1};
    const wp_u32 invalid[] = {0,1,99};
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    const wp_vertex_pntc *lit;
    uint64_t generation;
    int ok = 1;
    if( !mesh ) return 0;
    ok = wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_PT, positions, 4 ) &&
        wp_graphics_mesh_set_indices_u16( mesh, strip, 4 );
    wp_graphics_mesh_set_primitive_type( mesh, WORKPHONE_PRIMITIVE_TRIANGLE_STRIP );
    lit = wp_graphics_mesh_prepare_lit_vertices( mesh );
    ok = ok && lit && lit[0].normal.z > 0.999f && lit[3].normal.z > 0.999f &&
        lit[3].uv.x == 1.0f && lit[3].color == 0xffffffffu &&
        wp_graphics_mesh_prepare_lit_vertices( mesh ) == lit;
    ok = ok && wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_PT,
        wp_graphics_mesh_get_vertices( mesh ), 4 ) &&
        wp_graphics_mesh_set_indices_u16( mesh,
            (const uint16_t *)wp_graphics_mesh_get_indices( mesh ), 4 );
    generation = wp_graphics_mesh_get_content_generation( mesh );
    ok = ok && !wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_PT, NULL, 4 ) &&
        wp_graphics_mesh_get_content_generation( mesh ) == generation;
    wp_graphics_mesh_set_primitive_type( mesh, WORKPHONE_PRIMITIVE_TRIANGLE_LIST );
    ok = ok && wp_graphics_mesh_set_indices_u32( mesh, reversed, 3 ) &&
        wp_graphics_mesh_get_content_generation( mesh ) != generation;
    lit = wp_graphics_mesh_prepare_lit_vertices( mesh );
    ok = ok && lit && lit[0].normal.z < -0.999f;
    ok = ok && wp_graphics_mesh_set_indices_u32( mesh, invalid, 3 ) &&
        !wp_graphics_mesh_prepare_lit_vertices( mesh );
    /* Failed preparation must be recoverable by valid replacement data. */
    ok = ok && wp_graphics_mesh_set_indices_u32( mesh, reversed, 3 ) &&
        wp_graphics_mesh_prepare_lit_vertices( mesh );
    wp_graphics_mesh_destroy( mesh );
    return ok;
}

int main( int argc, char **argv )
{
    int i;
    if( !test_native_lit_preparation() )
    {
        fprintf( stderr, "Native lit mesh preparation failed\n" );
        return 1;
    }
    if( !test_wpm1_round_trip() )
    {
        fprintf( stderr, "WPM1 compatibility round trip failed\n" );
        return 1;
    }
    /* Generated round-trip coverage does not depend on optional external media. */
    for( i = 1; i < argc; ++i )
    {
        wp_graphics_mesh *mesh;
        FILE *fixture = fopen( argv[i], "rb" );
        if( !fixture )
        {
            fprintf( stderr, "UNAVAILABLE: external mesh fixture %s\n", argv[i] );
            return 77;
        }
        fclose( fixture );
        mesh = wp_graphics_mesh_read( argv[i] );
        if( !mesh )
        {
            fprintf( stderr, "failed to load %s\n", argv[i] );
            return 1;
        }
        if( wp_graphics_mesh_get_vertex_count( mesh ) == 0u ||
            wp_graphics_mesh_get_index_count( mesh ) == 0u ||
            wp_graphics_mesh_get_submesh_count( mesh ) == 0 )
        {
            fprintf( stderr, "mesh has incomplete render geometry: %s\n", argv[i] );
            wp_graphics_mesh_destroy( mesh );
            return 1;
        }
        wp_graphics_mesh_destroy( mesh );
    }
    return 0;
}
