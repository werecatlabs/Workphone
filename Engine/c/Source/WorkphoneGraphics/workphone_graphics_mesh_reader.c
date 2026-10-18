/**
 * @file workphone_graphics_mesh_reader.c
 * @brief Implementation of the binary mesh deserializer.
 */

#include "workphone_graphics_mesh_reader.h"
#include "workphone_graphics_mesh_ogre_reader.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>

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

/* Each multiplication is bounded by the remaining input before it occurs.
 * This must also be safe when size_t is only 32 bits. Header is 56 bytes. */
static wp_s32 wpm_record_size( const uint8_t *header, wp_u32 available, size_t *record_size )
{
    const uint8_t *p;
    uint32_t vertex_format, index_format, counts[3], strides[3], i;
    size_t remaining;
    if( available < WP_MESH_HEADER_SIZE ) return 0;
    p = header + 4;
    if( read_u32(&p) != WP_MESH_FILE_VERSION ) return 0;
    vertex_format = read_u32(&p);
    index_format = read_u32(&p);
    if( vertex_format > (uint32_t)WORKPHONE_VERTEX_FORMAT_PTC ||
        index_format > (uint32_t)WORKPHONE_INDEX_FORMAT_UINT32 ||
        read_u32(&p) > (uint32_t)WORKPHONE_PRIMITIVE_POINT_LIST ) return 0;
    for( i = 0; i < 3; ++i ) counts[i] = read_u32(&p);
    strides[0] = wp_vertex_format_stride((wp_vertex_format)vertex_format);
    strides[1] = index_format == WORKPHONE_INDEX_FORMAT_UINT16 ? 2u : 4u;
    strides[2] = WP_MESH_SUBMESH_SIZE;
    remaining = (size_t)available - WP_MESH_HEADER_SIZE;
    for( i = 0; i < 3; ++i )
    {
        if( !strides[i] || counts[i] > remaining / strides[i] ) return 0;
        remaining -= (size_t)counts[i] * strides[i];
    }
    *record_size = (size_t)available - remaining;
    return 1;
}

static wp_s32 wpm_finite( float value )
{
    return value >= -FLT_MAX && value <= FLT_MAX;
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
    size_t expected_size;
    wp_graphics_mesh *mesh;
    uint32_t i;
    uint32_t index_start;
    uint32_t index_count_sm;
    uint32_t material_id;
    const uint8_t *q;
    uint16_t index16;
    uint32_t index_value, element_count;
    float position;
    wp_u32 axis;

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
    if( !wpm_record_size((const uint8_t *)data, size, &expected_size) ) return NULL;

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
    if( !wpm_finite(aabb.min.x) || !wpm_finite(aabb.min.y) || !wpm_finite(aabb.min.z) ||
        !wpm_finite(aabb.max.x) || !wpm_finite(aabb.max.y) || !wpm_finite(aabb.max.z) ||
        aabb.min.x > aabb.max.x || aabb.min.y > aabb.max.y || aabb.min.z > aabb.max.z ) return NULL;

    /* validate total size before touching any blob data */
    vertex_stride = wp_vertex_format_stride( vertex_format );
    index_stride = ( index_format == WORKPHONE_INDEX_FORMAT_UINT16 ) ? (uint32_t)sizeof( uint16_t )
                                                                     : (uint32_t)sizeof( uint32_t );

    vertex_bytes = (size_t)vertex_stride * vertex_count;
    index_bytes = (size_t)index_stride * index_count;
    /* Validate the full publication before allocating or passing it to render
     * preparation. memcpy reads permit arbitrarily aligned caller buffers. */
    for( i = 0; i < vertex_count; ++i )
        for( axis = 0; axis < 3; ++axis )
        {
            memcpy(&position, p + (size_t)i * vertex_stride + axis * sizeof(float), sizeof(float));
            if( !wpm_finite(position) ) return NULL;
        }
    q = p + vertex_bytes;
    for( i = 0; i < index_count; ++i )
    {
        if( index_stride == 2 )
        {
            memcpy(&index16, q, sizeof(index16));
            index_value = index16;
        }
        else memcpy(&index_value, q, sizeof(index_value));
        if( index_value >= vertex_count ) return NULL;
        q += index_stride;
    }
    element_count = index_count ? index_count : vertex_count;
    for( i = 0; i < submesh_count; ++i )
    {
        index_start = read_u32(&q);
        index_count_sm = read_u32(&q);
        material_id = read_u32(&q);
        if( index_start > element_count || index_count_sm > element_count - index_start ) return NULL;
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
    uint8_t header[WP_MESH_HEADER_SIZE];
    long file_size;
    long start, end;
    size_t bytes_read, record_size;
    wp_graphics_mesh *mesh;

    if( !file )
    {
        return NULL;
    }

    start = ftell(file);
    if( start < 0 || fseek( file, 0L, SEEK_END ) != 0 )
    {
        return NULL;
    }

    end = ftell(file);
    if( end < start ) return NULL;
    file_size = end - start;
    if( file_size <= 0L || (unsigned long)file_size > (wp_u32)-1 ||
        (unsigned long)file_size > (size_t)-1 )
    {
        return NULL;
    }

    if( fseek( file, start, SEEK_SET ) != 0 )
    {
        return NULL;
    }
    record_size = (size_t)file_size;
    if( file_size >= (long)WP_MESH_HEADER_SIZE )
    {
        if( fread(header, 1, sizeof(header), file) != sizeof(header) ) return NULL;
        if( memcmp(header, WP_MESH_FILE_MAGIC, 4) == 0 &&
            !wpm_record_size(header, (wp_u32)file_size, &record_size) ) return NULL;
        if( fseek(file, start, SEEK_SET) != 0 ) return NULL;
    }

    buf = (uint8_t *)malloc( record_size );
    if( !buf )
    {
        return NULL;
    }

    bytes_read = fread( buf, 1u, record_size, file );
    if( bytes_read != record_size )
    {
        free( buf );
        return NULL;
    }

    mesh = wp_graphics_mesh_read_from_buffer( buf, (uint32_t)record_size );
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
