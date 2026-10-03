/**
 * @file workphone_graphics_mesh_ogre_reader.c
 * @brief Reader for the Ogre-compatible format used by MeshSerializerImpl.
 */

#include "workphone_graphics_mesh_ogre_reader.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_HEADER 0x1000u
#define CHUNK_MESH 0x3000u
#define CHUNK_SUBMESH 0x4000u
#define CHUNK_SUBMESH_OPERATION 0x4010u
#define CHUNK_GEOMETRY 0x5000u
#define CHUNK_VERTEX_DECLARATION 0x5100u
#define CHUNK_VERTEX_ELEMENT 0x5110u
#define CHUNK_VERTEX_BUFFER 0x5200u
#define CHUNK_VERTEX_BUFFER_DATA 0x5210u
#define CHUNK_BOUNDS 0x9000u

#define VES_POSITION 1u
#define VES_NORMAL 4u
#define VES_DIFFUSE 5u
#define VES_TEXTURE_COORDINATES 7u

#define VET_FLOAT1 0u
#define VET_FLOAT2 1u
#define VET_FLOAT3 2u
#define VET_FLOAT4 3u
#define VET_UBYTE4 9u
#define VET_DOUBLE1 12u
#define VET_DOUBLE2 13u
#define VET_DOUBLE3 14u
#define VET_DOUBLE4 15u
#define VET_BYTE4 28u
#define VET_BYTE4_NORM 29u
#define VET_UBYTE4_NORM 30u

#define ATTR_POSITION 1u
#define ATTR_NORMAL 2u
#define ATTR_UV 4u
#define ATTR_COLOR 8u

typedef struct mesh_cursor
{
    const uint8_t *p;
    const uint8_t *end;
    wp_s32 swap;
} mesh_cursor;

typedef struct mesh_chunk
{
    wp_u32 id;
    mesh_cursor payload;
} mesh_chunk;

typedef struct vertex_element
{
    wp_u32 source;
    wp_u32 type;
    wp_u32 semantic;
    wp_u32 offset;
    wp_u32 index;
} vertex_element;

typedef struct decoded_vertex
{
    float position[3];
    float normal[3];
    float uv[2];
    wp_u32 color;
} decoded_vertex;

typedef struct decoded_geometry
{
    decoded_vertex *vertices;
    wp_u32 count;
    wp_u32 attributes;
} decoded_geometry;

typedef struct decoded_submesh
{
    wp_u32 *indices;
    wp_u32 index_count;
    wp_u32 material_id;
    wp_u32 operation;
    wp_s32 shared;
    decoded_geometry geometry;
} decoded_submesh;

typedef struct decoded_mesh
{
    decoded_geometry shared_geometry;
    decoded_submesh *submeshes;
    wp_u32 submesh_count;
    wp_u32 submesh_capacity;
    wp_aabb3f bounds;
    wp_s32 has_bounds;
} decoded_mesh;

static uint16_t swap16( uint16_t value )
{
    return (uint16_t)( ( value << 8u ) | ( value >> 8u ) );
}

static wp_u32 swap32( wp_u32 value )
{
    return ( ( value & 0x000000ffu ) << 24u ) | ( ( value & 0x0000ff00u ) << 8u ) |
           ( ( value & 0x00ff0000u ) >> 8u ) | ( ( value & 0xff000000u ) >> 24u );
}

static wp_s32 read_u8( mesh_cursor *cursor, uint8_t *value )
{
    if( cursor->p == cursor->end )
    {
        return 0;
    }
    *value = *cursor->p++;
    return 1;
}

static wp_s32 read_u16( mesh_cursor *cursor, uint16_t *value )
{
    uint16_t raw;
    if( (size_t)( cursor->end - cursor->p ) < sizeof( raw ) )
    {
        return 0;
    }
    memcpy( &raw, cursor->p, sizeof( raw ) );
    cursor->p += sizeof( raw );
    *value = cursor->swap ? swap16( raw ) : raw;
    return 1;
}

static wp_s32 read_u32( mesh_cursor *cursor, wp_u32 *value )
{
    wp_u32 raw;
    if( (size_t)( cursor->end - cursor->p ) < sizeof( raw ) )
    {
        return 0;
    }
    memcpy( &raw, cursor->p, sizeof( raw ) );
    cursor->p += sizeof( raw );
    *value = cursor->swap ? swap32( raw ) : raw;
    return 1;
}

static wp_s32 read_float( mesh_cursor *cursor, float *value )
{
    wp_u32 bits;
    if( !read_u32( cursor, &bits ) )
    {
        return 0;
    }
    memcpy( value, &bits, sizeof( bits ) );
    return 1;
}

static wp_s32 read_chunk( mesh_cursor *cursor, mesh_chunk *chunk )
{
    uint16_t id;
    wp_u32 length;
    size_t payload_size;
    if( !read_u16( cursor, &id ) || !read_u32( cursor, &length ) || length < 6u )
    {
        return 0;
    }
    payload_size = (size_t)( length - 6u );
    if( payload_size > (size_t)( cursor->end - cursor->p ) )
    {
        return 0;
    }
    chunk->id = id;
    chunk->payload.p = cursor->p;
    chunk->payload.end = cursor->p + payload_size;
    chunk->payload.swap = cursor->swap;
    cursor->p += payload_size;
    return 1;
}

static wp_s32 peek_chunk_id( const mesh_cursor *cursor, wp_u32 *id )
{
    mesh_cursor copy;
    uint16_t value;
    copy = *cursor;
    if( !read_u16( &copy, &value ) )
    {
        return 0;
    }
    *id = value;
    return 1;
}

static wp_s32 skip_chunk_header( mesh_cursor *cursor, wp_u32 expected_id )
{
    uint16_t id;
    wp_u32 ignored_length;
    return read_u16( cursor, &id ) && id == expected_id && read_u32( cursor, &ignored_length ) &&
           ignored_length >= 6u;
}

static wp_s32 read_line( mesh_cursor *cursor, const uint8_t **text, size_t *length )
{
    const uint8_t *p;
    p = cursor->p;
    while( p < cursor->end && *p != (uint8_t)'\n' )
    {
        ++p;
    }
    if( p == cursor->end )
    {
        return 0;
    }
    *text = cursor->p;
    *length = (size_t)( p - cursor->p );
    cursor->p = p + 1;
    return 1;
}

static wp_s32 version_is( const uint8_t *version, size_t length, const char *expected )
{
    size_t expected_length;
    expected_length = strlen( expected );
    return length == expected_length && memcmp( version, expected, length ) == 0;
}

static wp_u32 material_hash( const uint8_t *name, size_t length )
{
    wp_u32 hash;
    size_t i;
    hash = 2166136261u;
    for( i = 0u; i < length; ++i )
    {
        hash ^= name[i];
        hash *= 16777619u;
    }
    return hash;
}

static void free_geometry( decoded_geometry *geometry )
{
    free( geometry->vertices );
    memset( geometry, 0, sizeof( *geometry ) );
}

static void free_decoded_mesh( decoded_mesh *mesh )
{
    wp_u32 i;
    free_geometry( &mesh->shared_geometry );
    for( i = 0u; i < mesh->submesh_count; ++i )
    {
        free( mesh->submeshes[i].indices );
        free_geometry( &mesh->submeshes[i].geometry );
    }
    free( mesh->submeshes );
    memset( mesh, 0, sizeof( *mesh ) );
}

static wp_s32 allocate_geometry( decoded_geometry *geometry, wp_u32 count )
{
    wp_u32 i;
    if( count == 0u || (size_t)count > (size_t)-1 / sizeof( decoded_vertex ) )
    {
        return 0;
    }
    geometry->vertices = (decoded_vertex *)calloc( count, sizeof( decoded_vertex ) );
    if( !geometry->vertices )
    {
        return 0;
    }
    geometry->count = count;
    for( i = 0u; i < count; ++i )
    {
        geometry->vertices[i].color = 0xffffffffu;
    }
    return 1;
}

static size_t vertex_type_size( wp_u32 type )
{
    switch( type )
    {
    case VET_FLOAT1:
        return 4u;
    case VET_FLOAT2:
        return 8u;
    case VET_FLOAT3:
        return 12u;
    case VET_FLOAT4:
        return 16u;
    case VET_UBYTE4:
    case VET_BYTE4:
    case VET_BYTE4_NORM:
    case VET_UBYTE4_NORM:
        return 4u;
    case VET_DOUBLE1:
        return 8u;
    case VET_DOUBLE2:
        return 16u;
    case VET_DOUBLE3:
        return 24u;
    case VET_DOUBLE4:
        return 32u;
    default:
        return 0u;
    }
}

static wp_s32 decode_floats( const uint8_t *source, wp_u32 type, wp_s32 swap, float *dest,
                             wp_u32 wanted )
{
    wp_u32 count;
    wp_u32 i;
    wp_u32 bits;
    double value;
    uint8_t bytes[8];
    wp_u32 j;
    if( type >= VET_FLOAT1 && type <= VET_FLOAT4 )
    {
        count = type - VET_FLOAT1 + 1u;
        if( count < wanted )
        {
            return 0;
        }
        for( i = 0u; i < wanted; ++i )
        {
            memcpy( &bits, source + i * 4u, 4u );
            bits = swap ? swap32( bits ) : bits;
            memcpy( &dest[i], &bits, 4u );
        }
        return 1;
    }
    if( type >= VET_DOUBLE1 && type <= VET_DOUBLE4 )
    {
        count = type - VET_DOUBLE1 + 1u;
        if( count < wanted )
        {
            return 0;
        }
        for( i = 0u; i < wanted; ++i )
        {
            if( swap )
            {
                for( j = 0u; j < 8u; ++j )
                {
                    bytes[j] = source[i * 8u + 7u - j];
                }
                memcpy( &value, bytes, 8u );
            }
            else
            {
                memcpy( &value, source + i * 8u, 8u );
            }
            dest[i] = (float)value;
        }
        return 1;
    }
    return 0;
}

static wp_s32 parse_declaration( mesh_cursor cursor, vertex_element **out_elements, wp_u32 *out_count )
{
    vertex_element *elements;
    wp_u32 count;
    wp_u32 capacity;
    mesh_chunk chunk;
    uint16_t value;
    elements = NULL;
    count = 0u;
    capacity = 0u;
    while( cursor.p < cursor.end )
    {
        vertex_element *grown;
        wp_u32 new_capacity;
        if( !read_chunk( &cursor, &chunk ) || chunk.id != CHUNK_VERTEX_ELEMENT )
        {
            free( elements );
            return 0;
        }
        if( count == capacity )
        {
            new_capacity = capacity == 0u ? 8u : capacity * 2u;
            grown =
                (vertex_element *)realloc( elements, (size_t)new_capacity * sizeof( vertex_element ) );
            if( !grown )
            {
                free( elements );
                return 0;
            }
            elements = grown;
            capacity = new_capacity;
        }
        if( !read_u16( &chunk.payload, &value ) )
            goto declaration_error;
        elements[count].source = value;
        if( !read_u16( &chunk.payload, &value ) )
            goto declaration_error;
        elements[count].type = value;
        if( !read_u16( &chunk.payload, &value ) )
            goto declaration_error;
        elements[count].semantic = value;
        if( !read_u16( &chunk.payload, &value ) )
            goto declaration_error;
        elements[count].offset = value;
        if( !read_u16( &chunk.payload, &value ) )
            goto declaration_error;
        elements[count].index = value;
        ++count;
    }
    *out_elements = elements;
    *out_count = count;
    return 1;

declaration_error:
    free( elements );
    return 0;
}

static wp_s32 apply_vertex_buffer( decoded_geometry *geometry, const vertex_element *elements,
                                   wp_u32 element_count, wp_u32 source_index, wp_u32 stride,
                                   mesh_cursor bytes )
{
    wp_u32 element_index;
    wp_u32 vertex_index;
    const vertex_element *element;
    const uint8_t *source;
    size_t element_size;
    size_t required;
    if( stride == 0u || (size_t)geometry->count > (size_t)-1 / stride )
    {
        return 0;
    }
    required = (size_t)geometry->count * stride;
    if( required > (size_t)( bytes.end - bytes.p ) )
    {
        return 0;
    }
    for( element_index = 0u; element_index < element_count; ++element_index )
    {
        element = &elements[element_index];
        element_size = vertex_type_size( element->type );
        if( element->source != source_index || element_size == 0u || element->offset > stride ||
            element_size > stride - element->offset )
        {
            continue;
        }
        if( element->semantic == VES_POSITION )
        {
            for( vertex_index = 0u; vertex_index < geometry->count; ++vertex_index )
            {
                source = bytes.p + (size_t)vertex_index * stride + element->offset;
                if( !decode_floats( source, element->type, bytes.swap,
                                    geometry->vertices[vertex_index].position, 3u ) )
                    return 0;
            }
            geometry->attributes |= ATTR_POSITION;
        }
        else if( element->semantic == VES_NORMAL )
        {
            for( vertex_index = 0u; vertex_index < geometry->count; ++vertex_index )
            {
                source = bytes.p + (size_t)vertex_index * stride + element->offset;
                if( !decode_floats( source, element->type, bytes.swap,
                                    geometry->vertices[vertex_index].normal, 3u ) )
                    return 0;
            }
            geometry->attributes |= ATTR_NORMAL;
        }
        else if( element->semantic == VES_TEXTURE_COORDINATES && element->index == 0u )
        {
            for( vertex_index = 0u; vertex_index < geometry->count; ++vertex_index )
            {
                source = bytes.p + (size_t)vertex_index * stride + element->offset;
                if( !decode_floats( source, element->type, bytes.swap,
                                    geometry->vertices[vertex_index].uv, 2u ) )
                    return 0;
            }
            geometry->attributes |= ATTR_UV;
        }
        else if( element->semantic == VES_DIFFUSE && element_size == 4u )
        {
            for( vertex_index = 0u; vertex_index < geometry->count; ++vertex_index )
            {
                source = bytes.p + (size_t)vertex_index * stride + element->offset;
                memcpy( &geometry->vertices[vertex_index].color, source, 4u );
            }
            geometry->attributes |= ATTR_COLOR;
        }
    }
    return 1;
}

static wp_s32 parse_geometry( mesh_cursor *cursor, decoded_geometry *geometry )
{
    wp_u32 vertex_count;
    vertex_element *elements;
    wp_u32 element_count;
    mesh_chunk chunk;
    mesh_chunk data_chunk;
    uint16_t source_index;
    uint16_t stride;
    elements = NULL;
    element_count = 0u;
    wp_u32 next_id;
    if( !read_u32( cursor, &vertex_count ) || !allocate_geometry( geometry, vertex_count ) )
    {
        return 0;
    }
    while( cursor->p < cursor->end && peek_chunk_id( cursor, &next_id ) &&
           ( next_id == CHUNK_VERTEX_DECLARATION || next_id == CHUNK_VERTEX_BUFFER ) )
    {
        if( !read_chunk( cursor, &chunk ) )
            goto geometry_error;
        if( chunk.id == CHUNK_VERTEX_DECLARATION )
        {
            free( elements );
            elements = NULL;
            element_count = 0u;
            if( !parse_declaration( chunk.payload, &elements, &element_count ) )
                goto geometry_error;
        }
        else if( chunk.id == CHUNK_VERTEX_BUFFER )
        {
            if( !elements || !read_u16( &chunk.payload, &source_index ) ||
                !read_u16( &chunk.payload, &stride ) || !read_chunk( &chunk.payload, &data_chunk ) ||
                data_chunk.id != CHUNK_VERTEX_BUFFER_DATA ||
                !apply_vertex_buffer( geometry, elements, element_count, source_index, stride,
                                      data_chunk.payload ) )
                goto geometry_error;
        }
    }
    free( elements );
    return ( geometry->attributes & ATTR_POSITION ) != 0u;

geometry_error:
    free( elements );
    return 0;
}

static wp_s32 add_submesh( decoded_mesh *mesh, decoded_submesh **submesh )
{
    decoded_submesh *grown;
    wp_u32 new_capacity;
    if( mesh->submesh_count == mesh->submesh_capacity )
    {
        new_capacity = mesh->submesh_capacity == 0u ? 4u : mesh->submesh_capacity * 2u;
        grown = (decoded_submesh *)realloc( mesh->submeshes,
                                            (size_t)new_capacity * sizeof( decoded_submesh ) );
        if( !grown )
        {
            return 0;
        }
        mesh->submeshes = grown;
        mesh->submesh_capacity = new_capacity;
    }
    *submesh = &mesh->submeshes[mesh->submesh_count++];
    memset( *submesh, 0, sizeof( **submesh ) );
    ( *submesh )->operation = 4u;
    return 1;
}

static wp_s32 parse_submesh( mesh_cursor *cursor, decoded_mesh *mesh )
{
    decoded_submesh *submesh;
    const uint8_t *material;
    size_t material_length;
    uint8_t shared;
    uint8_t indices32;
    wp_u32 i;
    uint16_t index16;
    uint16_t operation;
    mesh_chunk chunk;
    wp_u32 next_id;
    if( !add_submesh( mesh, &submesh ) || !read_line( cursor, &material, &material_length ) ||
        !read_u8( cursor, &shared ) || !read_u32( cursor, &submesh->index_count ) ||
        !read_u8( cursor, &indices32 ) )
    {
        return 0;
    }
    submesh->shared = shared != 0u;
    submesh->material_id = material_hash( material, material_length );
    if( submesh->index_count )
    {
        if( (size_t)submesh->index_count > (size_t)-1 / sizeof( wp_u32 ) )
            return 0;
        submesh->indices = (wp_u32 *)malloc( (size_t)submesh->index_count * sizeof( wp_u32 ) );
        if( !submesh->indices )
            return 0;
        for( i = 0u; i < submesh->index_count; ++i )
        {
            if( indices32 )
            {
                if( !read_u32( cursor, &submesh->indices[i] ) )
                    return 0;
            }
            else
            {
                if( !read_u16( cursor, &index16 ) )
                    return 0;
                submesh->indices[i] = index16;
            }
        }
    }
    if( !submesh->shared )
    {
        if( !skip_chunk_header( cursor, CHUNK_GEOMETRY ) ||
            !parse_geometry( cursor, &submesh->geometry ) )
            return 0;
    }
    while( cursor->p < cursor->end && peek_chunk_id( cursor, &next_id ) &&
           ( next_id == CHUNK_SUBMESH_OPERATION || next_id == 0x4100u || next_id == 0x4200u ) )
    {
        if( !read_chunk( cursor, &chunk ) )
            return 0;
        if( chunk.id == CHUNK_SUBMESH_OPERATION )
        {
            if( !read_u16( &chunk.payload, &operation ) )
                return 0;
            submesh->operation = operation == 0u ? 4u : operation;
        }
    }
    return 1;
}

static wp_s32 parse_mesh( mesh_cursor cursor, decoded_mesh *mesh )
{
    uint8_t animated;
    float radius;
    wp_u32 next_id;
    if( !read_u8( &cursor, &animated ) )
    {
        return 0;
    }
    while( cursor.p < cursor.end )
    {
        if( !peek_chunk_id( &cursor, &next_id ) )
            return 0;
        if( next_id == CHUNK_GEOMETRY )
        {
            if( !skip_chunk_header( &cursor, CHUNK_GEOMETRY ) )
                return 0;
            free_geometry( &mesh->shared_geometry );
            if( !parse_geometry( &cursor, &mesh->shared_geometry ) )
                return 0;
        }
        else if( next_id == CHUNK_SUBMESH )
        {
            if( !skip_chunk_header( &cursor, CHUNK_SUBMESH ) || !parse_submesh( &cursor, mesh ) )
                return 0;
        }
        else if( next_id == CHUNK_BOUNDS )
        {
            if( !skip_chunk_header( &cursor, CHUNK_BOUNDS ) ||
                !read_float( &cursor, &mesh->bounds.min.x ) ||
                !read_float( &cursor, &mesh->bounds.min.y ) ||
                !read_float( &cursor, &mesh->bounds.min.z ) ||
                !read_float( &cursor, &mesh->bounds.max.x ) ||
                !read_float( &cursor, &mesh->bounds.max.y ) ||
                !read_float( &cursor, &mesh->bounds.max.z ) || !read_float( &cursor, &radius ) )
                return 0;
            mesh->has_bounds = 1;
        }
        else
        {
            /* Rendering geometry is always written before skeleton, LOD,
             * animation and edge metadata. Some Ogre exporters (and the
             * Workphone writer's historical size calculators) emit incorrect
             * lengths for those chunks, so do not use them to seek. */
            break;
        }
    }
    return 1;
}

static wp_vertex_format choose_vertex_format( wp_u32 attributes )
{
    if( attributes & ATTR_NORMAL )
    {
        if( attributes & ATTR_COLOR )
            return WORKPHONE_VERTEX_FORMAT_PNTC;
        return ( attributes & ATTR_UV ) ? WORKPHONE_VERTEX_FORMAT_PNT : WORKPHONE_VERTEX_FORMAT_PN;
    }
    if( attributes & ATTR_COLOR )
    {
        return ( attributes & ATTR_UV ) ? WORKPHONE_VERTEX_FORMAT_PTC : WORKPHONE_VERTEX_FORMAT_PC;
    }
    return ( attributes & ATTR_UV ) ? WORKPHONE_VERTEX_FORMAT_PT : WORKPHONE_VERTEX_FORMAT_P;
}

static void write_vertex( uint8_t *dest, wp_vertex_format format, const decoded_vertex *source )
{
    memcpy( dest, source->position, 12u );
    switch( format )
    {
    case WORKPHONE_VERTEX_FORMAT_PN:
        memcpy( dest + 12u, source->normal, 12u );
        break;
    case WORKPHONE_VERTEX_FORMAT_PT:
        memcpy( dest + 12u, source->uv, 8u );
        break;
    case WORKPHONE_VERTEX_FORMAT_PNT:
        memcpy( dest + 12u, source->normal, 12u );
        memcpy( dest + 24u, source->uv, 8u );
        break;
    case WORKPHONE_VERTEX_FORMAT_PNTC:
        memcpy( dest + 12u, source->normal, 12u );
        memcpy( dest + 24u, source->uv, 8u );
        memcpy( dest + 32u, &source->color, 4u );
        break;
    case WORKPHONE_VERTEX_FORMAT_PC:
        memcpy( dest + 12u, &source->color, 4u );
        break;
    case WORKPHONE_VERTEX_FORMAT_PTC:
        memcpy( dest + 12u, source->uv, 8u );
        memcpy( dest + 20u, &source->color, 4u );
        break;
    default:
        break;
    }
}

static wp_primitive_type convert_operation( wp_u32 operation )
{
    switch( operation )
    {
    case 1u:
        return WORKPHONE_PRIMITIVE_POINT_LIST;
    case 2u:
        return WORKPHONE_PRIMITIVE_LINE_LIST;
    case 3u:
        return WORKPHONE_PRIMITIVE_LINE_STRIP;
    case 5u:
        return WORKPHONE_PRIMITIVE_TRIANGLE_STRIP;
    default:
        return WORKPHONE_PRIMITIVE_TRIANGLE_LIST;
    }
}

static wp_graphics_mesh *build_mesh( const decoded_mesh *source )
{
    wp_u32 total_vertices;
    wp_u32 total_indices;
    wp_u32 attributes;
    wp_u32 i;
    wp_u32 j;
    wp_u32 vertex_offset;
    wp_u32 index_offset;
    wp_u32 base_vertex;
    wp_u32 vertex_limit;
    wp_u32 max_index;
    wp_vertex_format format;
    wp_u32 stride;
    uint8_t *vertices;
    wp_u32 *indices;
    uint16_t *indices16;
    wp_graphics_mesh *mesh;

    total_vertices = source->shared_geometry.count;
    total_indices = 0u;
    attributes = source->shared_geometry.attributes;
    for( i = 0u; i < source->submesh_count; ++i )
    {
        if( !source->submeshes[i].shared )
        {
            if( UINT32_MAX - total_vertices < source->submeshes[i].geometry.count )
                return NULL;
            total_vertices += source->submeshes[i].geometry.count;
            attributes |= source->submeshes[i].geometry.attributes;
        }
        if( UINT32_MAX - total_indices < source->submeshes[i].index_count )
            return NULL;
        total_indices += source->submeshes[i].index_count;
    }
    if( total_vertices == 0u )
        return NULL;
    format = choose_vertex_format( attributes );
    stride = wp_vertex_format_stride( format );
    if( (size_t)total_vertices > (size_t)-1 / stride ||
        ( total_indices && (size_t)total_indices > (size_t)-1 / sizeof( wp_u32 ) ) )
        return NULL;
    vertices = (uint8_t *)malloc( (size_t)total_vertices * stride );
    indices = total_indices ? (wp_u32 *)malloc( (size_t)total_indices * sizeof( wp_u32 ) ) : NULL;
    if( !vertices || ( total_indices && !indices ) )
    {
        free( vertices );
        free( indices );
        return NULL;
    }

    vertex_offset = 0u;
    for( i = 0u; i < source->shared_geometry.count; ++i )
    {
        write_vertex( vertices + (size_t)vertex_offset++ * stride, format,
                      &source->shared_geometry.vertices[i] );
    }
    index_offset = 0u;
    max_index = 0u;
    for( i = 0u; i < source->submesh_count; ++i )
    {
        if( source->submeshes[i].shared )
        {
            base_vertex = 0u;
            vertex_limit = source->shared_geometry.count;
            if( vertex_limit == 0u )
                goto build_error;
        }
        else
        {
            base_vertex = vertex_offset;
            vertex_limit = source->submeshes[i].geometry.count;
            for( j = 0u; j < vertex_limit; ++j )
            {
                write_vertex( vertices + (size_t)vertex_offset++ * stride, format,
                              &source->submeshes[i].geometry.vertices[j] );
            }
        }
        for( j = 0u; j < source->submeshes[i].index_count; ++j )
        {
            if( source->submeshes[i].indices[j] >= vertex_limit )
                goto build_error;
            indices[index_offset + j] = base_vertex + source->submeshes[i].indices[j];
            if( indices[index_offset + j] > max_index )
                max_index = indices[index_offset + j];
        }
        index_offset += source->submeshes[i].index_count;
    }

    mesh = wp_graphics_mesh_create();
    if( !mesh || !wp_graphics_mesh_set_vertices( mesh, format, vertices, total_vertices ) )
    {
        wp_graphics_mesh_destroy( mesh );
        goto build_error;
    }
    free( vertices );
    vertices = NULL;
    if( total_indices && max_index <= 65535u )
    {
        indices16 = (uint16_t *)malloc( (size_t)total_indices * sizeof( uint16_t ) );
        if( !indices16 )
            goto mesh_error;
        for( i = 0u; i < total_indices; ++i )
            indices16[i] = (uint16_t)indices[i];
        if( !wp_graphics_mesh_set_indices_u16( mesh, indices16, total_indices ) )
        {
            free( indices16 );
            goto mesh_error;
        }
        free( indices16 );
    }
    else if( total_indices && !wp_graphics_mesh_set_indices_u32( mesh, indices, total_indices ) )
    {
        goto mesh_error;
    }
    free( indices );
    indices = NULL;
    index_offset = 0u;
    for( i = 0u; i < source->submesh_count; ++i )
    {
        if( wp_graphics_mesh_add_submesh( mesh, index_offset, source->submeshes[i].index_count,
                                          source->submeshes[i].material_id ) < 0 )
            goto mesh_error;
        index_offset += source->submeshes[i].index_count;
    }
    if( source->submesh_count )
    {
        wp_graphics_mesh_set_primitive_type( mesh, convert_operation( source->submeshes[0].operation ) );
    }
    if( source->has_bounds )
        wp_graphics_mesh_set_local_aabb( mesh, source->bounds );
    else
        wp_graphics_mesh_compute_aabb( mesh );
    return mesh;

mesh_error:
    wp_graphics_mesh_destroy( mesh );
build_error:
    free( vertices );
    free( indices );
    return NULL;
}

wp_graphics_mesh *wp_graphics_mesh_read_ogre_from_buffer( const void *data, wp_u32 size )
{
    mesh_cursor cursor;
    uint16_t raw_header;
    uint16_t header;
    const uint8_t *version;
    size_t version_length;
    decoded_mesh decoded;
    wp_graphics_mesh *mesh;
    if( !data || size < 3u )
        return NULL;
    memcpy( &raw_header, data, sizeof( raw_header ) );
    if( raw_header == CHUNK_HEADER )
        cursor.swap = 0;
    else if( raw_header == 0x0010u )
        cursor.swap = 1;
    else
        return NULL;
    cursor.p = (const uint8_t *)data;
    cursor.end = cursor.p + size;
    if( !read_u16( &cursor, &header ) || header != CHUNK_HEADER ||
        !read_line( &cursor, &version, &version_length ) )
        return NULL;
    if( !version_is( version, version_length, "[MeshSerializer_v1.8]" ) &&
        !version_is( version, version_length, "[MeshSerializer_v1.100]" ) &&
        !version_is( version, version_length, "[MeshSerializer_v1.41]" ) &&
        !version_is( version, version_length, "[MeshSerializer_v1.40]" ) &&
        !version_is( version, version_length, "[MeshSerializer_v1.30]" ) )
        return NULL;

    memset( &decoded, 0, sizeof( decoded ) );
    if( !skip_chunk_header( &cursor, CHUNK_MESH ) || !parse_mesh( cursor, &decoded ) )
    {
        free_decoded_mesh( &decoded );
        return NULL;
    }
    mesh = build_mesh( &decoded );
    free_decoded_mesh( &decoded );
    return mesh;
}
