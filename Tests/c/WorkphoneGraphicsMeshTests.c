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

int main( int argc, char **argv )
{
    int i;
    if( !test_wpm1_round_trip() )
    {
        fprintf( stderr, "WPM1 compatibility round trip failed\n" );
        return 1;
    }
    if( argc < 2 )
    {
        fprintf( stderr, "expected at least one mesh path\n" );
        return 1;
    }
    for( i = 1; i < argc; ++i )
    {
        wp_graphics_mesh *mesh;
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
