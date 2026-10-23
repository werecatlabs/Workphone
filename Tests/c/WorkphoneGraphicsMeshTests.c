#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <workphone_graphics_mesh.h>
#include <workphone_graphics_mesh_reader.h>
#include <workphone_graphics_mesh_writer.h>
#include "WorkphoneGraphicsMeshBudgetContracts.h"

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

static void corrupt_u32( unsigned char *data, size_t offset, uint32_t value )
{
    memcpy( data + offset, &value, sizeof( value ) );
}

static int expect_bad_wpm( const void *data, wp_u32 size, const char *label )
{
    wp_graphics_mesh *mesh = wp_graphics_mesh_read_from_buffer( data, size );
    if( mesh )
    {
        fprintf( stderr, "Accepted corrupt WPM1: %s\n", label );
        wp_graphics_mesh_destroy( mesh );
        return 0;
    }
    return 1;
}

static int test_wpm_corruption_and_streams( void )
{
    const wp_graphics_mesh_vertex_p vertices[3] = {
        { {0,0,0} }, { {1,0,0} }, { {0,1,0} }
    };
    const uint16_t indices16[3] = {0,1,2};
    const uint32_t indices32[3] = {0,1,2};
    const size_t count_offsets[3] = {20,24,28};
    wp_graphics_mesh *source, *loaded;
    unsigned char *data, *copy, *unaligned;
    void *buffer;
    wp_u32 size, n;
    size_t index_bytes, submesh_offset;
    uint16_t bad16;
    FILE *stream;
    int wide, i, ok = 1;
    for( wide = 0; wide < 2; ++wide )
    {
        source = wp_graphics_mesh_create();
        if( !source ) return 0;
        ok = wp_graphics_mesh_set_vertices( source, WORKPHONE_VERTEX_FORMAT_P, vertices, 3 ) &&
            (wide ? wp_graphics_mesh_set_indices_u32( source, indices32, 3 ) :
                    wp_graphics_mesh_set_indices_u16( source, indices16, 3 )) &&
            wp_graphics_mesh_add_submesh( source, 0, 3, 42 ) >= 0 &&
            wp_graphics_mesh_write_to_buffer( source, &buffer, &size );
        wp_graphics_mesh_destroy( source );
        if( !ok ) return 0;
        data = (unsigned char *)buffer;
        copy = (unsigned char *)malloc( size );
        unaligned = (unsigned char *)malloc( (size_t)size + 1 );
        if( !copy || !unaligned ) { free(copy); free(unaligned); free(data); return 0; }
        index_bytes = wide ? 12u : 6u;
        submesh_offset = 56u + 36u + index_bytes;
        for( n = 0; n < size; ++n ) ok &= expect_bad_wpm( data, n, "truncation" );
        for( i = 0; i < 3; ++i )
        {
            memcpy( copy, data, size );
            corrupt_u32( copy, count_offsets[i], 0x40000000u );
            ok &= expect_bad_wpm( copy, size, "overflowing count" );
        }
        memcpy( copy, data, size );
        if( wide ) corrupt_u32( copy, 92u, 3u );
        else { bad16 = 3; memcpy( copy + 92u, &bad16, sizeof(bad16) ); }
        ok &= expect_bad_wpm( copy, size, "index outside vertex buffer" );
        memcpy( copy, data, size );
        corrupt_u32( copy, submesh_offset, 2u );
        ok &= expect_bad_wpm( copy, size, "submesh beyond index buffer" );
        corrupt_u32( copy, submesh_offset, 0xffffffffu );
        ok &= expect_bad_wpm( copy, size, "wrapped submesh span" );
        memcpy( copy, data, size );
        corrupt_u32( copy, 56u, 0x7fc00000u );
        ok &= expect_bad_wpm( copy, size, "NaN position" );
        memcpy( copy, data, size );
        corrupt_u32( copy, 32u, 0x7f800000u );
        ok &= expect_bad_wpm( copy, size, "infinite bounds" );
        memcpy( copy, data, size );
        corrupt_u32( copy, 32u, 0x3f800000u );
        ok &= expect_bad_wpm( copy, size, "inverted bounds" );
        memcpy( unaligned + 1, data, size );
        loaded = wp_graphics_mesh_read_from_buffer( unaligned + 1, size );
        ok &= loaded != NULL;
        wp_graphics_mesh_destroy( loaded );
        /* An embedded mesh must start at the caller's offset and stop before
         * the next record. Repeated reads exercise the same stream contract. */
        stream = tmpfile();
        if( !stream ) ok = 0;
        else
        {
            if( fwrite("prefix!", 1, 7, stream) != 7 ||
                fwrite(data, 1, size, stream) != size ||
                fwrite(data, 1, size, stream) != size ||
                fseek(stream, 7L, SEEK_SET) != 0 ) ok = 0;
            for( i = 0; i < 2; ++i )
            {
                loaded = wp_graphics_mesh_read_from_file(stream);
                if( !loaded || ftell(stream) != 7L + (long)size * (i + 1) )
                { fprintf(stderr, "WPM1 stream offset/record boundary failed\n"); ok = 0; }
                wp_graphics_mesh_destroy(loaded);
            }
            fclose(stream);
        }
        free(unaligned); free(copy); free(data);
    }
    return ok;
}

static int test_wpm_nonindexed_spans( void )
{
    const wp_graphics_mesh_vertex_p vertices[3] = {
        { {0,0,0} }, { {1,0,0} }, { {0,1,0} }
    };
    wp_graphics_mesh *source, *loaded;
    void *buffer;
    wp_u32 size;
    int ok, i;
    source = wp_graphics_mesh_create();
    if( !source ) return 0;
    ok = wp_graphics_mesh_set_vertices(source, WORKPHONE_VERTEX_FORMAT_P, vertices, 3) &&
         wp_graphics_mesh_add_submesh(source, 0, 3, 42) >= 0 &&
         wp_graphics_mesh_write_to_buffer(source, &buffer, &size);
    wp_graphics_mesh_destroy(source);
    if( !ok ) return 0;
    loaded = wp_graphics_mesh_read_from_buffer(buffer, size);
    ok = loaded != NULL;
    wp_graphics_mesh_destroy(loaded);
    corrupt_u32((unsigned char *)buffer, 92u, 2u);
    ok &= expect_bad_wpm(buffer, size, "nonindexed span beyond vertices");
    /* An empty span at the end is valid for both indexed/nonindexed meshes. */
    corrupt_u32((unsigned char *)buffer, 92u, 3u);
    corrupt_u32((unsigned char *)buffer, 96u, 0u);
    loaded = wp_graphics_mesh_read_from_buffer(buffer, size);
    ok &= loaded != NULL;
    wp_graphics_mesh_destroy(loaded);
    free(buffer);
    /* Exercise real section growth and round-trip publication without huge
     * allocations or a test-only mesh representation. */
    source = wp_graphics_mesh_create();
    if( !source ) return 0;
    ok &= wp_graphics_mesh_set_vertices(source, WORKPHONE_VERTEX_FORMAT_P, vertices, 3);
    for( i = 0; i < 1000; ++i )
        ok &= wp_graphics_mesh_add_submesh(source, 0, 3, (wp_u32)i) == i;
    if( !wp_graphics_mesh_write_to_buffer(source, &buffer, &size) ) ok = 0;
    else
    {
        loaded = wp_graphics_mesh_read_from_buffer(buffer, size);
        ok &= loaded && wp_graphics_mesh_get_submesh_count(loaded) == 1000 &&
              wp_graphics_mesh_get_submesh(loaded, 999)->material_id == 999;
        wp_graphics_mesh_destroy(loaded);
        free(buffer);
    }
    wp_graphics_mesh_destroy(source);
    return ok;
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
    if( !test_mesh_budget_lifetimes() || !test_mesh_budget_decoding() )
    {
        fprintf( stderr, "Native mesh CPU allocation budget contracts failed\n" );
        return 1;
    }
    if( !test_wpm_corruption_and_streams() || !test_wpm_nonindexed_spans() ) return 1;
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
