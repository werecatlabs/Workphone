/**
 * @file workphone_graphics_mesh_reader.c
 * @brief Implementation of the binary mesh deserializer.
 */

#include "workphone_graphics_mesh_reader.h"
#include "workphone_graphics_mesh_ogre_reader.h"
#include <stdlib.h>
#include <string.h>

static const uint8_t WP_MESH_FILE_MAGIC[4] = { 'W', 'P', 'M', '1' };
static const uint32_t WP_MESH_FILE_VERSION = 1u;

#define WP_MESH_HEADER_SIZE 56u
#define WP_MESH_SUBMESH_SIZE 12u /* 3 x uint32 */

static uint32_t read_u32( const uint8_t **p )
{
    uint32_t v;
    memcpy( &v, *p, sizeof( uint32_t ) );
    *p += sizeof( uint32_t );
    return v;
}

static float read_f32( const uint8_t **p )
{
    float v;
    memcpy( &v, *p, sizeof( float ) );
    *p += sizeof( float );
    return v;
}

wp_graphics_mesh *wp_graphics_mesh_read_from_buffer( const void *data, wp_u32 size )
{
    const uint8_t *p;
    uint32_t version;
    uint32_t vertex_format_u32;
    uint32_t index_format_u32;
    uint32_t primitive_type_u32;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t submesh_count;
    wp_aabb3f aabb;
    wp_vertex_format vertex_format;
    wp_index_format index_format;
    uint32_t vertex_stride;
    uint32_t index_stride;
    size_t vertex_bytes;
    size_t index_bytes;
    size_t submesh_bytes;
    size_t expected_size;
    wp_graphics_mesh *mesh;
    uint32_t i;
    uint32_t index_start;
    uint32_t index_count_sm;
    uint32_t material_id;

    if( !data )
    {
        return NULL;
    }

    /* Workphone's C++ serializer uses the Ogre chunked .mesh format. */
    if( size < 4u || memcmp( data, WP_MESH_FILE_MAGIC, 4u ) != 0 )
    {
        return wp_graphics_mesh_read_ogre_from_buffer( data, size );
    }

    if( size < WP_MESH_HEADER_SIZE )
    {
        return NULL;
    }

    p = (const uint8_t *)data;

    /* magic */
    if( memcmp( p, WP_MESH_FILE_MAGIC, 4u ) != 0 )
    {
        return NULL;
    }
    p += 4u;

    /* version */
    version = read_u32( &p );
    if( version != WP_MESH_FILE_VERSION )
    {
        return NULL;
    }

    /* formats */
    vertex_format_u32 = read_u32( &p );
    index_format_u32 = read_u32( &p );
    primitive_type_u32 = read_u32( &p );

    if( vertex_format_u32 > (uint32_t)WORKPHONE_VERTEX_FORMAT_PTC )
    {
        return NULL;
    }
    if( index_format_u32 > (uint32_t)WORKPHONE_INDEX_FORMAT_UINT32 )
    {
        return NULL;
    }
    if( primitive_type_u32 > (uint32_t)WORKPHONE_PRIMITIVE_POINT_LIST )
    {
        return NULL;
    }

    vertex_format = (wp_vertex_format)vertex_format_u32;
    index_format = (wp_index_format)index_format_u32;

    /* counts */
    vertex_count = read_u32( &p );
    index_count = read_u32( &p );
    submesh_count = read_u32( &p );

    /* AABB */
    aabb.min.x = read_f32( &p );
    aabb.min.y = read_f32( &p );
    aabb.min.z = read_f32( &p );
    aabb.max.x = read_f32( &p );
    aabb.max.y = read_f32( &p );
    aabb.max.z = read_f32( &p );

    /* validate total size before touching any blob data */
    vertex_stride = wp_vertex_format_stride( vertex_format );
    index_stride = ( index_format == WORKPHONE_INDEX_FORMAT_UINT16 ) ? (uint32_t)sizeof( uint16_t )
                                                                     : (uint32_t)sizeof( uint32_t );

    vertex_bytes = (size_t)vertex_stride * vertex_count;
    index_bytes = (size_t)index_stride * index_count;
    submesh_bytes = (size_t)submesh_count * WP_MESH_SUBMESH_SIZE;
    expected_size = WP_MESH_HEADER_SIZE + vertex_bytes + index_bytes + submesh_bytes;

    if( size < expected_size )
    {
        return NULL;
    }

    /* build the mesh */
    mesh = wp_graphics_mesh_create();
    if( !mesh )
    {
        return NULL;
    }

    wp_graphics_mesh_set_primitive_type( mesh, (wp_primitive_type)primitive_type_u32 );
    wp_graphics_mesh_set_local_aabb( mesh, aabb );

    /* vertices */
    if( vertex_count > 0u )
    {
        if( !wp_graphics_mesh_set_vertices( mesh, vertex_format, p, vertex_count ) )
        {
            wp_graphics_mesh_destroy( mesh );
            return NULL;
        }
        p += vertex_bytes;
    }

    /* indices */
    if( index_count > 0u )
    {
        wp_s32 ok;

        if( index_format == WORKPHONE_INDEX_FORMAT_UINT16 )
        {
            ok = wp_graphics_mesh_set_indices_u16( mesh, (const uint16_t *)p, index_count );
        }
        else
        {
            ok = wp_graphics_mesh_set_indices_u32( mesh, (const uint32_t *)p, index_count );
        }

        if( !ok )
        {
            wp_graphics_mesh_destroy( mesh );
            return NULL;
        }
        p += index_bytes;
    }

    /* submeshes */
    for( i = 0u; i < submesh_count; i++ )
    {
        index_start = read_u32( &p );
        index_count_sm = read_u32( &p );
        material_id = read_u32( &p );

        if( wp_graphics_mesh_add_submesh( mesh, index_start, index_count_sm, material_id ) < 0 )
        {
            wp_graphics_mesh_destroy( mesh );
            return NULL;
        }
    }

    return mesh;
}

wp_graphics_mesh *wp_graphics_mesh_read_from_file( FILE *file )
{
    uint8_t *buf;
    long file_size;
    long bytes_read;
    wp_graphics_mesh *mesh;

    if( !file )
    {
        return NULL;
    }

    if( fseek( file, 0L, SEEK_END ) != 0 )
    {
        return NULL;
    }

    file_size = ftell( file );
    if( file_size <= 0L )
    {
        return NULL;
    }

    if( fseek( file, 0L, SEEK_SET ) != 0 )
    {
        return NULL;
    }

    buf = (uint8_t *)malloc( (size_t)file_size );
    if( !buf )
    {
        return NULL;
    }

    bytes_read = (long)fread( buf, 1u, (size_t)file_size, file );
    if( bytes_read != file_size )
    {
        free( buf );
        return NULL;
    }

    mesh = wp_graphics_mesh_read_from_buffer( buf, (uint32_t)file_size );
    free( buf );
    return mesh;
}

wp_graphics_mesh *wp_graphics_mesh_read( const char *path )
{
    FILE *f;
    wp_graphics_mesh *mesh;

    if( !path )
    {
        return NULL;
    }

    f = fopen( path, "rb" );
    if( !f )
    {
        return NULL;
    }

    mesh = wp_graphics_mesh_read_from_file( f );
    fclose( f );
    return mesh;
}
