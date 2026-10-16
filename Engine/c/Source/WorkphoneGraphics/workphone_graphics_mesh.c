/**
 * @file wp_graphics_mesh.c
 * @brief Implementation of the C graphics mesh API.
 */

#include "workphone_graphics_mesh.h"
#include "workphone_graphics_material.h"
#include "workphone_graphics_renderer.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_graphics_mesh
{
    wp_vertex_format vertex_format;
    wp_primitive_type primitive_type;
    wp_index_format index_format;

    void *vertices;
    wp_u32 vertex_count;

    void *indices;
    wp_u32 index_count;

    wp_submesh *submeshes;
    wp_u32 submesh_count;
    wp_u32 submesh_cap;

    wp_aabb3f local_aabb;
    void *native;
    wp_s32 is_dirty;
    wp_vertex_ptc *render_vertices;
    wp_vertex_pntc *lit_vertices;
    uint64_t content_generation;
} wp_graphics_mesh;

static const wp_u32 WORKPHONE_SUBMESH_INITIAL_CAP = 4u;
/* Match the C90-compatible lifetime-ID atomics used by native physics. Meshes
 * may be created independently by workers; mutation of each mesh is serialized. */
#ifdef _MSC_VER
#    include <intrin.h>
static __declspec( align( 8 ) ) volatile __int64 wp_mesh_generation;
static uint64_t wp_mesh_next_generation( void )
{
    return (uint64_t)_InterlockedIncrement64( &wp_mesh_generation );
}
#else
static uint64_t wp_mesh_generation;
static uint64_t wp_mesh_next_generation( void )
{
    return __atomic_add_fetch( &wp_mesh_generation, 1, __ATOMIC_RELAXED );
}
#endif
static void wp_mesh_invalidate_lit( wp_graphics_mesh *mesh )
{
    free( mesh->lit_vertices );
    mesh->lit_vertices = NULL;
    mesh->content_generation = wp_mesh_next_generation();
}
uint64_t wp_graphics_mesh_get_content_generation( const wp_graphics_mesh *mesh )
{
    return mesh ? mesh->content_generation : 0;
}


/* =========================================================================
 * Vertex format helpers
 * ====================================================================== */

wp_u32 wp_vertex_format_stride( wp_vertex_format fmt )
{
    switch( fmt )
    {
    case WORKPHONE_VERTEX_FORMAT_P:
        return (wp_u32)sizeof( wp_graphics_mesh_vertex_p );
    case WORKPHONE_VERTEX_FORMAT_PN:
        return (wp_u32)sizeof( wp_graphics_mesh_vertex_pn );
    case WORKPHONE_VERTEX_FORMAT_PT:
        return (wp_u32)sizeof( wp_graphics_mesh_vertex_pt );
    case WORKPHONE_VERTEX_FORMAT_PNT:
        return (wp_u32)sizeof( wp_graphics_mesh_vertex_pnt );
    case WORKPHONE_VERTEX_FORMAT_PNTC:
        return (wp_u32)sizeof( wp_graphics_mesh_vertex_pntc );
    case WORKPHONE_VERTEX_FORMAT_PC:
        return (wp_u32)sizeof( wp_graphics_mesh_vertex_pc );
    case WORKPHONE_VERTEX_FORMAT_PTC:
        return (wp_u32)sizeof( wp_graphics_mesh_vertex_ptc );
    default:
        return 0u;
    }
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_graphics_mesh *wp_graphics_mesh_create( void )
{
    wp_graphics_mesh *mesh = (wp_graphics_mesh *)malloc( sizeof( wp_graphics_mesh ) );
    if( !mesh )
    {
        return NULL;
    }

    memset( mesh, 0, sizeof( wp_graphics_mesh ) );

    mesh->primitive_type = WORKPHONE_PRIMITIVE_TRIANGLE_LIST;
    mesh->vertex_format = WORKPHONE_VERTEX_FORMAT_P;
    mesh->index_format = WORKPHONE_INDEX_FORMAT_UINT16;
    wp_mesh_invalidate_lit( mesh );

    return mesh;
}

void wp_graphics_mesh_destroy( wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return;
    }

    free( mesh->vertices );
    free( mesh->indices );
    free( mesh->submeshes );
    free( mesh->render_vertices );
    free( mesh->lit_vertices );
    free( mesh );
}

/* =========================================================================
 * Vertex buffer
 * ====================================================================== */

wp_s32 wp_graphics_mesh_set_vertices( wp_graphics_mesh *mesh, wp_vertex_format fmt, const void *data,
                                      wp_u32 count )
{
    wp_u32 stride;
    void *copy;

    if( !mesh || !data || count == 0 )
    {
        return 0;
    }

    stride = wp_vertex_format_stride( fmt );
    if( stride == 0 )
    {
        return 0;
    }

    if( (size_t)count > (size_t)-1 / stride ) return 0;
    copy = malloc( (size_t)stride * count );
    if( !copy )
    {
        return 0;
    }

    memcpy( copy, data, (size_t)stride * count );
    free( mesh->vertices );

    mesh->vertices = copy;
    mesh->vertex_count = count;
    mesh->vertex_format = fmt;
    mesh->is_dirty = 1;
    wp_mesh_invalidate_lit( mesh );

    return 1;
}

const void *wp_graphics_mesh_get_vertices( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return NULL;
    }

    return mesh->vertices;
}

wp_u32 wp_graphics_mesh_get_vertex_count( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return 0;
    }

    return mesh->vertex_count;
}

wp_u32 wp_graphics_mesh_get_vertex_stride( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return 0;
    }

    return wp_vertex_format_stride( mesh->vertex_format );
}

wp_vertex_format wp_graphics_mesh_get_vertex_format( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return WORKPHONE_VERTEX_FORMAT_P;
    }

    return mesh->vertex_format;
}

/* =========================================================================
 * Index buffer
 * ====================================================================== */

wp_s32 wp_graphics_mesh_set_indices_u16( wp_graphics_mesh *mesh, const uint16_t *data, wp_u32 count )
{
    void *copy;

    if( !mesh || !data || count == 0 )
    {
        return 0;
    }

    if( (size_t)count > (size_t)-1 / sizeof( uint16_t ) ) return 0;
    copy = malloc( sizeof( uint16_t ) * (size_t)count );
    if( !copy )
    {
        return 0;
    }

    memcpy( copy, data, sizeof( uint16_t ) * (size_t)count );
    free( mesh->indices );

    mesh->indices = copy;
    mesh->index_count = count;
    mesh->index_format = WORKPHONE_INDEX_FORMAT_UINT16;
    mesh->is_dirty = 1;
    wp_mesh_invalidate_lit( mesh );

    return 1;
}

wp_s32 wp_graphics_mesh_set_indices_u32( wp_graphics_mesh *mesh, const wp_u32 *data, wp_u32 count )
{
    void *copy;

    if( !mesh || !data || count == 0 )
    {
        return 0;
    }

    if( (size_t)count > (size_t)-1 / sizeof( wp_u32 ) ) return 0;
    copy = malloc( sizeof( wp_u32 ) * (size_t)count );
    if( !copy )
    {
        return 0;
    }

    memcpy( copy, data, sizeof( wp_u32 ) * (size_t)count );
    free( mesh->indices );

    mesh->indices = copy;
    mesh->index_count = count;
    mesh->index_format = WORKPHONE_INDEX_FORMAT_UINT32;
    mesh->is_dirty = 1;
    wp_mesh_invalidate_lit( mesh );

    return 1;
}

const void *wp_graphics_mesh_get_indices( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return NULL;
    }

    return mesh->indices;
}

wp_u32 wp_graphics_mesh_get_index_count( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return 0;
    }

    return mesh->index_count;
}

wp_index_format wp_graphics_mesh_get_index_format( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return WORKPHONE_INDEX_FORMAT_UINT16;
    }

    return mesh->index_format;
}

/* =========================================================================
 * Primitive type
 * ====================================================================== */

void wp_graphics_mesh_set_primitive_type( wp_graphics_mesh *mesh, wp_primitive_type type )
{
    if( !mesh )
    {
        return;
    }

    mesh->primitive_type = type;
    mesh->is_dirty = 1;
    wp_mesh_invalidate_lit( mesh );
}

wp_primitive_type wp_graphics_mesh_get_primitive_type( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return WORKPHONE_PRIMITIVE_TRIANGLE_LIST;
    }

    return mesh->primitive_type;
}

/* =========================================================================
 * Submeshes
 * ====================================================================== */

wp_s32 wp_graphics_mesh_add_submesh( wp_graphics_mesh *mesh, wp_u32 index_start, wp_u32 index_count,
                                     wp_u32 material_id )
{
    wp_submesh *new_submeshes;
    wp_u32 new_cap;
    wp_s32 idx;

    if( !mesh )
    {
        return -1;
    }

    if( mesh->submesh_count >= mesh->submesh_cap )
    {
        new_cap = ( mesh->submesh_cap == 0u ) ? WORKPHONE_SUBMESH_INITIAL_CAP : mesh->submesh_cap * 2u;
        new_submeshes = (wp_submesh *)realloc( mesh->submeshes, (wp_u32)new_cap * sizeof( wp_submesh ) );
        if( !new_submeshes )
        {
            return -1;
        }

        mesh->submeshes = new_submeshes;
        mesh->submesh_cap = new_cap;
    }

    idx = (wp_s32)mesh->submesh_count;
    mesh->submeshes[idx].index_start = index_start;
    mesh->submeshes[idx].index_count = index_count;
    mesh->submeshes[idx].material_id = material_id;
    mesh->submesh_count++;

    return idx;
}

const wp_submesh *wp_graphics_mesh_get_submesh( const wp_graphics_mesh *mesh, wp_s32 index )
{
    if( !mesh || index < 0 || (wp_u32)index >= mesh->submesh_count )
    {
        return NULL;
    }

    return &mesh->submeshes[index];
}

wp_s32 wp_graphics_mesh_get_submesh_count( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return 0;
    }

    return (wp_s32)mesh->submesh_count;
}

void wp_graphics_mesh_remove_submesh( wp_graphics_mesh *mesh, wp_s32 index )
{
    wp_u32 remaining;

    if( !mesh || index < 0 || (wp_u32)index >= mesh->submesh_count )
    {
        return;
    }

    remaining = mesh->submesh_count - (wp_u32)index - 1u;
    if( remaining > 0u )
    {
        memmove( &mesh->submeshes[index], &mesh->submeshes[index + 1],
                 (wp_u32)remaining * sizeof( wp_submesh ) );
    }

    mesh->submesh_count--;
}

void wp_graphics_mesh_clear_submeshes( wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return;
    }

    mesh->submesh_count = 0;
}

/* =========================================================================
 * AABB
 * ====================================================================== */

wp_aabb3f wp_graphics_mesh_get_local_aabb( const wp_graphics_mesh *mesh )
{
    wp_aabb3f empty;
    memset( &empty, 0, sizeof( wp_aabb3f ) );

    if( !mesh )
    {
        return empty;
    }

    return mesh->local_aabb;
}

void wp_graphics_mesh_set_local_aabb( wp_graphics_mesh *mesh, wp_aabb3f aabb )
{
    if( !mesh )
    {
        return;
    }

    mesh->local_aabb = aabb;
    mesh->content_generation = wp_mesh_next_generation();
}

void wp_graphics_mesh_compute_aabb( wp_graphics_mesh *mesh )
{
    const wp_u8 *data;
    const wp_vec3f *pos;
    wp_u32 stride;
    wp_u32 i;

    if( !mesh || !mesh->vertices || mesh->vertex_count == 0 )
    {
        return;
    }

    stride = wp_vertex_format_stride( mesh->vertex_format );
    if( stride == 0 )
    {
        return;
    }

    data = (const wp_u8 *)mesh->vertices;
    pos = (const wp_vec3f *)data;

    mesh->content_generation = wp_mesh_next_generation();
    mesh->local_aabb.min = *pos;
    mesh->local_aabb.max = *pos;

    for( i = 1; i < mesh->vertex_count; i++ )
    {
        pos = (const wp_vec3f *)( data + (wp_u32)i * (wp_u32)stride );

        if( pos->x < mesh->local_aabb.min.x )
        {
            mesh->local_aabb.min.x = pos->x;
        }
        if( pos->y < mesh->local_aabb.min.y )
        {
            mesh->local_aabb.min.y = pos->y;
        }
        if( pos->z < mesh->local_aabb.min.z )
        {
            mesh->local_aabb.min.z = pos->z;
        }

        if( pos->x > mesh->local_aabb.max.x )
        {
            mesh->local_aabb.max.x = pos->x;
        }
        if( pos->y > mesh->local_aabb.max.y )
        {
            mesh->local_aabb.max.y = pos->y;
        }
        if( pos->z > mesh->local_aabb.max.z )
        {
            mesh->local_aabb.max.z = pos->z;
        }
    }
}

/* =========================================================================
 * Dirty flag
 * ====================================================================== */

wp_s32 wp_graphics_mesh_is_dirty( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return 0;
    }

    return mesh->is_dirty;
}

void wp_graphics_mesh_mark_dirty( wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return;
    }

    mesh->is_dirty = 1;
}

void wp_graphics_mesh_clear_dirty( wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return;
    }

    mesh->is_dirty = 0;
}

/* =========================================================================
 * Native GPU handle
 * ====================================================================== */

void *wp_graphics_mesh_get_native( const wp_graphics_mesh *mesh )
{
    if( !mesh )
    {
        return NULL;
    }

    return mesh->native;
}

void wp_graphics_mesh_set_native( wp_graphics_mesh *mesh, void *native )
{
    if( !mesh )
    {
        return;
    }

    mesh->native = native;
}

#if !defined( WP_GRAPHICS_MESH_SERIALIZATION_ONLY )

static wp_vertex_ptc wp_graphics_mesh_read_ptc( const wp_graphics_mesh *mesh, wp_u32 index )
{
    wp_vertex_ptc result;
    const wp_u8 *source;

    memset( &result, 0, sizeof( result ) );
    result.color = 0xFFFFFFFFu;
    source = (const wp_u8 *)mesh->vertices + index * wp_vertex_format_stride( mesh->vertex_format );

    switch( mesh->vertex_format )
    {
    case WORKPHONE_VERTEX_FORMAT_P:
        result.position = ( (const wp_graphics_mesh_vertex_p *)source )->position;
        break;
    case WORKPHONE_VERTEX_FORMAT_PN:
        result.position = ( (const wp_graphics_mesh_vertex_pn *)source )->position;
        break;
    case WORKPHONE_VERTEX_FORMAT_PT:
        result.position = ( (const wp_graphics_mesh_vertex_pt *)source )->position;
        result.uv = ( (const wp_graphics_mesh_vertex_pt *)source )->uv;
        break;
    case WORKPHONE_VERTEX_FORMAT_PNT:
        result.position = ( (const wp_graphics_mesh_vertex_pnt *)source )->position;
        result.uv = ( (const wp_graphics_mesh_vertex_pnt *)source )->uv;
        break;
    case WORKPHONE_VERTEX_FORMAT_PNTC:
        result.position = ( (const wp_graphics_mesh_vertex_pntc *)source )->position;
        result.uv = ( (const wp_graphics_mesh_vertex_pntc *)source )->uv;
        result.color = ( (const wp_graphics_mesh_vertex_pntc *)source )->color;
        break;
    case WORKPHONE_VERTEX_FORMAT_PC:
        result.position = ( (const wp_graphics_mesh_vertex_pc *)source )->position;
        result.color = ( (const wp_graphics_mesh_vertex_pc *)source )->color;
        break;
    case WORKPHONE_VERTEX_FORMAT_PTC:
        result.position = ( (const wp_graphics_mesh_vertex_ptc *)source )->position;
        result.uv = ( (const wp_graphics_mesh_vertex_ptc *)source )->uv;
        result.color = ( (const wp_graphics_mesh_vertex_ptc *)source )->color;
        break;
    default:
        break;
    }

    return result;
}

static wp_s32 wp_graphics_mesh_rebuild_render_vertices( wp_graphics_mesh *mesh )
{
    wp_vertex_ptc *vertices;
    wp_u32 i;

    if( !mesh || !mesh->vertices || mesh->vertex_count == 0u )
    {
        return 0;
    }
    if( (size_t)mesh->vertex_count > ( ~(size_t)0 ) / sizeof( wp_vertex_ptc ) )
    {
        return 0;
    }

    vertices = (wp_vertex_ptc *)malloc( (size_t)mesh->vertex_count * sizeof( wp_vertex_ptc ) );
    if( !vertices )
    {
        return 0;
    }
    for( i = 0u; i < mesh->vertex_count; ++i )
    {
        vertices[i] = wp_graphics_mesh_read_ptc( mesh, i );
    }

    free( mesh->render_vertices );
    mesh->render_vertices = vertices;
    mesh->is_dirty = 0;
    return 1;
}

void wp_graphics_mesh_render( wp_graphics_mesh *mesh, wp_renderer *renderer,
                              const wp_graphics_material *material )
{
    if( !mesh || !renderer || !mesh->vertices || mesh->vertex_count == 0u )
    {
        return;
    }
    if( mesh->vertex_count > (wp_u32)INT_MAX || mesh->index_count > (wp_u32)INT_MAX )
    {
        return;
    }
    if( mesh->primitive_type != WORKPHONE_PRIMITIVE_TRIANGLE_LIST )
    {
        return;
    }
    if( ( mesh->is_dirty || !mesh->render_vertices ) &&
        !wp_graphics_mesh_rebuild_render_vertices( mesh ) )
    {
        return;
    }

    if( material )
    {
        if( !wp_graphics_material_apply( (wp_graphics_material *)material, renderer ) )
        {
            return;
        }
    }
    else
    {
        wp_renderer_set_blend_mode( renderer, WORKPHONE_BLEND_MODE_NONE );
        wp_renderer_set_cull_mode( renderer, WORKPHONE_CULL_MODE_BACK );
        wp_renderer_set_depth_test_enabled( renderer, 1 );
        wp_renderer_set_depth_write_enabled( renderer, 1 );
    }
    if( mesh->indices && mesh->index_count >= 3u )
    {
        if( mesh->index_format == WORKPHONE_INDEX_FORMAT_UINT32 )
        {
            wp_renderer_draw_indexed_triangles_ptc_u32(
                renderer, mesh->render_vertices, (wp_s32)mesh->vertex_count,
                (const wp_u32 *)mesh->indices, (wp_s32)mesh->index_count );
        }
        else
        {
            wp_renderer_draw_indexed_triangles_ptc(
                renderer, mesh->render_vertices, (wp_s32)mesh->vertex_count,
                (const uint16_t *)mesh->indices, (wp_s32)mesh->index_count );
        }
    }
    else
    {
        wp_renderer_draw_triangles_ptc( renderer, mesh->render_vertices, (wp_s32)mesh->vertex_count );
    }
}

#endif

/* Native format conversion and generated normals shared by all hardware views. */
static wp_u32 wp_mesh_lit_index( const wp_graphics_mesh *mesh, wp_u32 i )
{
    if( !mesh->indices ) return i;
    return mesh->index_format == WORKPHONE_INDEX_FORMAT_UINT32
        ? ( (const wp_u32 *)mesh->indices )[i]
        : ( (const uint16_t *)mesh->indices )[i];
}
const wp_vertex_pntc *wp_graphics_mesh_prepare_lit_vertices( wp_graphics_mesh *mesh )
{
    wp_vertex_pntc *vertices;
    wp_u32 i, stride, elements, step, a, b, c, swap;
    wp_s32 has_normals;
    const unsigned char *source;
    wp_vec3f ab, ac, normal;
    double length;
    if( !mesh || !mesh->vertices || !mesh->vertex_count ) return NULL;
    if( mesh->lit_vertices ) return mesh->lit_vertices;
    if( (size_t)mesh->vertex_count > (size_t)-1 / sizeof( *vertices ) ) return NULL;
    stride = wp_vertex_format_stride( mesh->vertex_format );
    if( !stride ) return NULL;
    vertices = (wp_vertex_pntc *)calloc( mesh->vertex_count, sizeof( *vertices ) );
    if( !vertices ) return NULL;
    has_normals = mesh->vertex_format == WORKPHONE_VERTEX_FORMAT_PN ||
        mesh->vertex_format == WORKPHONE_VERTEX_FORMAT_PNT ||
        mesh->vertex_format == WORKPHONE_VERTEX_FORMAT_PNTC;
    for( i = 0; i < mesh->vertex_count; ++i )
    {
        source = (const unsigned char *)mesh->vertices + (size_t)i * stride;
        memcpy( &vertices[i].position, source, sizeof( wp_vec3f ) );
        vertices[i].color = 0xffffffffu;
        switch( mesh->vertex_format )
        {
        case WORKPHONE_VERTEX_FORMAT_PN:
            vertices[i].normal = ( (const wp_graphics_mesh_vertex_pn *)source )->normal; break;
        case WORKPHONE_VERTEX_FORMAT_PT:
            vertices[i].uv = ( (const wp_graphics_mesh_vertex_pt *)source )->uv; break;
        case WORKPHONE_VERTEX_FORMAT_PNT:
            vertices[i].normal = ( (const wp_graphics_mesh_vertex_pnt *)source )->normal;
            vertices[i].uv = ( (const wp_graphics_mesh_vertex_pnt *)source )->uv; break;
        case WORKPHONE_VERTEX_FORMAT_PNTC:
            vertices[i].normal = ( (const wp_graphics_mesh_vertex_pntc *)source )->normal;
            vertices[i].uv = ( (const wp_graphics_mesh_vertex_pntc *)source )->uv;
            vertices[i].color = ( (const wp_graphics_mesh_vertex_pntc *)source )->color; break;
        case WORKPHONE_VERTEX_FORMAT_PC:
            vertices[i].color = ( (const wp_graphics_mesh_vertex_pc *)source )->color; break;
        case WORKPHONE_VERTEX_FORMAT_PTC:
            vertices[i].uv = ( (const wp_graphics_mesh_vertex_ptc *)source )->uv;
            vertices[i].color = ( (const wp_graphics_mesh_vertex_ptc *)source )->color; break;
        default: break;
        }
    }
    if( !has_normals )
    {
        elements = mesh->indices ? mesh->index_count : mesh->vertex_count;
        step = mesh->primitive_type == WORKPHONE_PRIMITIVE_TRIANGLE_STRIP ? 1u : 3u;
        if( mesh->primitive_type == WORKPHONE_PRIMITIVE_TRIANGLE_STRIP ||
            mesh->primitive_type == WORKPHONE_PRIMITIVE_TRIANGLE_LIST )
        {
            for( i = 0; elements >= 3u && i <= elements - 3u; i += step )
            {
                a = wp_mesh_lit_index( mesh, i ); b = wp_mesh_lit_index( mesh, i + 1u );
                c = wp_mesh_lit_index( mesh, i + 2u );
                if( step == 1u && ( i & 1u ) ) { swap = a; a = b; b = swap; }
                if( a >= mesh->vertex_count || b >= mesh->vertex_count || c >= mesh->vertex_count )
                { free( vertices ); return NULL; }
                ab.x = vertices[b].position.x - vertices[a].position.x;
                ab.y = vertices[b].position.y - vertices[a].position.y;
                ab.z = vertices[b].position.z - vertices[a].position.z;
                ac.x = vertices[c].position.x - vertices[a].position.x;
                ac.y = vertices[c].position.y - vertices[a].position.y;
                ac.z = vertices[c].position.z - vertices[a].position.z;
                normal.x = ab.y * ac.z - ab.z * ac.y;
                normal.y = ab.z * ac.x - ab.x * ac.z;
                normal.z = ab.x * ac.y - ab.y * ac.x;
                vertices[a].normal.x += normal.x; vertices[a].normal.y += normal.y; vertices[a].normal.z += normal.z;
                vertices[b].normal.x += normal.x; vertices[b].normal.y += normal.y; vertices[b].normal.z += normal.z;
                vertices[c].normal.x += normal.x; vertices[c].normal.y += normal.y; vertices[c].normal.z += normal.z;
            }
        }
        for( i = 0; i < mesh->vertex_count; ++i )
        {
            normal = vertices[i].normal;
            length = sqrt( (double)normal.x * normal.x + (double)normal.y * normal.y + (double)normal.z * normal.z );
            if( length > 1.0e-6 )
            {
                vertices[i].normal.x = (float)( normal.x / length );
                vertices[i].normal.y = (float)( normal.y / length );
                vertices[i].normal.z = (float)( normal.z / length );
            }
            else { vertices[i].normal.x = 0.0f; vertices[i].normal.y = 1.0f; vertices[i].normal.z = 0.0f; }
        }
    }
    mesh->lit_vertices = vertices;
    return vertices;
}
