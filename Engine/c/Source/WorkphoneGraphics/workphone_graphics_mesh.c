/**
 * @file wp_graphics_mesh.c
 * @brief Implementation of the C graphics mesh API.
 */

#include "workphone_graphics_mesh.h"
#include "workphone_graphics_material.h"
#include "workphone_graphics_renderer.h"
#include "workphone_graphics_mesh_memory.h"
#include <limits.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_graphics_mesh
{
    wp_mesh_memory_callbacks memory;
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
    wp_graphics_obb local_obb;
    wp_s32 has_obb, aabb_authored;
    wp_graphics_mesh_bounds_stats bounds_stats;
    void *native;
    wp_s32 is_dirty;
    wp_vertex_ptc *render_vertices;
    wp_vertex_pntc *lit_vertices;
    uint64_t content_generation;
} wp_graphics_mesh;

static const wp_mesh_memory_callbacks *wp_mesh_callbacks( const wp_graphics_mesh *mesh )
{
    return mesh->memory.reserve ? &mesh->memory : NULL;
}

void *wp_mesh_memory_allocate_working( wp_graphics_mesh *mesh, size_t bytes )
{
    return mesh ? wp_mesh_memory_allocate( wp_mesh_callbacks( mesh ), bytes, 1 ) : NULL;
}

static const wp_u32 WORKPHONE_SUBMESH_INITIAL_CAP = 4u;
/* Match the C90-compatible lifetime-ID atomics used by native physics. Meshes
 * may be created independently by workers; mutation of each mesh is serialized. */
#ifdef _MSC_VER
#    include <intrin.h>
static __declspec( align( 8 ) ) volatile __int64 wp_mesh_generation;

static uint64_t wp_mesh_next_generation( void )
{
#if defined(_M_IX86)
    /* MSVC x86 provides the 64-bit compare/exchange intrinsic, but not the
     * increment intrinsic. Keep the lifetime ID atomic on both architectures. */
    __int64 expected, desired, observed;
    expected = _InterlockedCompareExchange64( &wp_mesh_generation, 0, 0 );
    for( ;; )
    {
        desired = (__int64)((uint64_t)expected + 1u);
        observed = _InterlockedCompareExchange64( &wp_mesh_generation, desired, expected );
        if( observed == expected ) return (uint64_t)desired;
        expected = observed;
    }
#else
    return (uint64_t)_InterlockedIncrement64( &wp_mesh_generation );
#endif
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
    wp_mesh_memory_free( mesh->lit_vertices );
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
    return wp_graphics_mesh_create_budgeted( NULL );
}

wp_graphics_mesh *wp_graphics_mesh_create_budgeted( const wp_mesh_memory_callbacks *callbacks )
{
    wp_graphics_mesh *mesh = (wp_graphics_mesh *)wp_mesh_memory_allocate(
        callbacks, sizeof( wp_graphics_mesh ), 0 );
    if( !mesh )
    {
        return NULL;
    }

    memset( mesh, 0, sizeof( wp_graphics_mesh ) );
    if( callbacks ) mesh->memory = *callbacks;

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

    wp_mesh_memory_free( mesh->vertices );
    wp_mesh_memory_free( mesh->indices );
    wp_mesh_memory_free( mesh->submeshes );
    wp_mesh_memory_free( mesh->render_vertices );
    wp_mesh_memory_free( mesh->lit_vertices );
    wp_mesh_memory_free( mesh );
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

    if( (size_t)count > (size_t)-1 / stride )
    {
        return 0;
    }
    copy = wp_mesh_memory_allocate( wp_mesh_callbacks( mesh ), (size_t)stride * count, 0 );
    if( !copy )
    {
        return 0;
    }

    memcpy( copy, data, (size_t)stride * count );
    wp_mesh_memory_free( mesh->vertices );

    mesh->has_obb = 0;
    if( !mesh->aabb_authored )
    {
        memset( &mesh->local_aabb, 0, sizeof( mesh->local_aabb ) );
    }
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

    if( (size_t)count > (size_t)-1 / sizeof( uint16_t ) )
    {
        return 0;
    }
    copy = wp_mesh_memory_allocate( wp_mesh_callbacks( mesh ), sizeof( uint16_t ) * (size_t)count, 0 );
    if( !copy )
    {
        return 0;
    }

    memcpy( copy, data, sizeof( uint16_t ) * (size_t)count );
    wp_mesh_memory_free( mesh->indices );

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

    if( (size_t)count > (size_t)-1 / sizeof( wp_u32 ) )
    {
        return 0;
    }
    copy = wp_mesh_memory_allocate( wp_mesh_callbacks( mesh ), sizeof( wp_u32 ) * (size_t)count, 0 );
    if( !copy )
    {
        return 0;
    }

    memcpy( copy, data, sizeof( wp_u32 ) * (size_t)count );
    wp_mesh_memory_free( mesh->indices );

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

    if( !mesh || mesh->submesh_count >= (wp_u32)INT_MAX )
    {
        return -1;
    }

    if( mesh->submesh_count >= mesh->submesh_cap )
    {
        if( mesh->submesh_cap > (wp_u32)-1 / 2u ) return -1;
        new_cap = ( mesh->submesh_cap == 0u ) ? WORKPHONE_SUBMESH_INITIAL_CAP : mesh->submesh_cap * 2u;
        if( (size_t)new_cap > (size_t)-1 / sizeof( wp_submesh ) ) return -1;
        new_submeshes = (wp_submesh *)wp_mesh_memory_reallocate( wp_mesh_callbacks( mesh ),
            mesh->submeshes, (size_t)new_cap * sizeof( wp_submesh ), 0 );
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
    mesh->aabb_authored = 1;
    mesh->has_obb = 0;
    mesh->content_generation = wp_mesh_next_generation();
}

static wp_s32 wp_mesh_finite( double x )
{
    return x <= DBL_MAX && x >= -DBL_MAX;
}

static double wp_mesh_dot( const wp_vec3d *a, const wp_vec3d *b )
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

wp_s32 wp_graphics_mesh_get_local_obb( const wp_graphics_mesh *mesh, wp_graphics_obb *out )
{
    if( !mesh || !out || !mesh->has_obb )
    {
        return 0;
    }
    *out = mesh->local_obb;
    return 1;
}

wp_s32 wp_graphics_mesh_set_local_obb( wp_graphics_mesh *mesh, const wp_graphics_obb *box )
{
    wp_s32 i, j;
    double lengths[3], dot;
    if( !mesh || !box )
    {
        return 0;
    }
    for( i = 0; i < 12; ++i )
    {
        if( !wp_mesh_finite( ( (const double *)box )[i] ) )
        {
            return 0;
        }
    }
    for( i = 0; i < 3; ++i )
    {
        lengths[i] = wp_mesh_dot( &box->half_axes[i], &box->half_axes[i] );
        if( !wp_mesh_finite( lengths[i] ) )
        {
            return 0;
        }
        for( j = 0; j < i; ++j )
        {
            dot = wp_mesh_dot( &box->half_axes[i], &box->half_axes[j] );
            if( fabs( dot ) > 1.0e-8 * sqrt( lengths[i] ) * sqrt( lengths[j] ) )
            {
                return 0;
            }
        }
    }
    mesh->local_obb = *box;
    mesh->has_obb = 1;
    mesh->content_generation = wp_mesh_next_generation();
    return 1;
}

void wp_graphics_mesh_clear_local_obb( wp_graphics_mesh *mesh )
{
    if( mesh && mesh->has_obb )
    {
        mesh->has_obb = 0;
        mesh->content_generation = wp_mesh_next_generation();
    }
}

wp_s32 wp_graphics_mesh_compute_obb( wp_graphics_mesh *mesh )
{
    double mean[3] = { 0, 0, 0 }, covariance[3][3], basis[3][3], delta[3], low[3], high[3], middle[3],
           extent[3];
    double largest, trace, angle, cosine, sine, left, right, ap, aq, cross, value, scale = 0, pad, score,
                                                                                   axis_score;
    wp_graphics_obb box;
    const wp_vec3f *vertex;
    const wp_u8 *data;
    wp_u32 n, stride;
    wp_s32 i, j, k, p = 0, q = 1, sweep;
    if( !mesh )
    {
        return 0;
    }
    wp_graphics_mesh_clear_local_obb( mesh );
    if( !mesh->vertices || !mesh->vertex_count )
    {
        return 0;
    }
    n = mesh->vertex_count;
    stride = wp_vertex_format_stride( mesh->vertex_format );
    if( !stride )
    {
        return 0;
    }
    data = (const wp_u8 *)mesh->vertices;
    /* Incremental mean avoids a large unscaled sum on wide meshes. */
    for( k = 0; k < 3; ++k )
    {
        mean[k] = 0;
    }
    for( n = 0; n < mesh->vertex_count; ++n )
    {
        vertex = (const wp_vec3f *)( data + (size_t)n * stride );
        for( i = 0; i < 3; ++i )
        {
            value = ( (const wp_f32 *)vertex )[i];
            if( !( value <= FLT_MAX && value >= -FLT_MAX ) )
            {
                return 0;
            }
            mean[i] += ( value - mean[i] ) / (double)( n + 1u );
        }
    }
    ++mesh->bounds_stats.pca_solves;
    memset( covariance, 0, sizeof( covariance ) );
    memset( basis, 0, sizeof( basis ) );
    for( i = 0; i < 3; ++i )
    {
        basis[i][i] = 1;
    }
    for( n = 0; n < mesh->vertex_count; ++n )
    {
        vertex = (const wp_vec3f *)( data + (size_t)n * stride );
        for( i = 0; i < 3; ++i )
        {
            delta[i] = ( (const wp_f32 *)vertex )[i] - mean[i];
        }
        for( i = 0; i < 3; ++i )
        {
            for( j = 0; j < 3; ++j )
            {
                covariance[i][j] += delta[i] * delta[j] / (double)mesh->vertex_count;
            }
        }
    }
    for( sweep = 0; sweep < 32; ++sweep )
    {
        largest = 0;
        trace = 0;
        for( i = 0; i < 3; ++i )
        {
            trace += fabs( covariance[i][i] );
            for( j = i + 1; j < 3; ++j )
            {
                if( fabs( covariance[i][j] ) > largest )
                {
                    largest = fabs( covariance[i][j] );
                    p = i;
                    q = j;
                }
            }
        }
        if( largest <= 1.0e-14 * trace || largest == 0 )
        {
            break;
        }
        angle = 0.5 * atan2( 2 * covariance[p][q], covariance[q][q] - covariance[p][p] );
        cosine = cos( angle );
        sine = sin( angle );
        ap = covariance[p][p];
        aq = covariance[q][q];
        cross = covariance[p][q];
        for( k = 0; k < 3; ++k )
        {
            if( k != p && k != q )
            {
                left = covariance[k][p];
                right = covariance[k][q];
                covariance[k][p] = covariance[p][k] = cosine * left - sine * right;
                covariance[k][q] = covariance[q][k] = sine * left + cosine * right;
            }
            left = basis[k][p];
            right = basis[k][q];
            basis[k][p] = cosine * left - sine * right;
            basis[k][q] = sine * left + cosine * right;
        }
        covariance[p][p] = cosine * cosine * ap - 2 * sine * cosine * cross + sine * sine * aq;
        covariance[q][q] = sine * sine * ap + 2 * sine * cosine * cross + cosine * cosine * aq;
        covariance[p][q] = covariance[q][p] = 0;
    }
    for( i = 0; i < 3; ++i )
    {
        low[i] = DBL_MAX;
        high[i] = -DBL_MAX;
    }
    for( n = 0; n < mesh->vertex_count; ++n )
    {
        vertex = (const wp_vec3f *)( data + (size_t)n * stride );
        for( i = 0; i < 3; ++i )
        {
            value = 0;
            for( j = 0; j < 3; ++j )
            {
                value += basis[j][i] * ( ( (const wp_f32 *)vertex )[j] - mean[j] );
            }
            if( value < low[i] )
            {
                low[i] = value;
            }
            if( value > high[i] )
            {
                high[i] = value;
            }
        }
    }
    for( i = 0; i < 3; ++i )
    {
        middle[i] = ( low[i] + high[i] ) * 0.5;
        extent[i] = ( high[i] - low[i] ) * 0.5;
        if( extent[i] > scale )
        {
            scale = extent[i];
        }
    }
    score = extent[0] * extent[1] + extent[0] * extent[2] + extent[1] * extent[2];
    /* Prefer the local axes when PCA gives a larger surface-area proxy. */
    for( i = 0; i < 3; ++i )
    {
        delta[i] = ( ( (const wp_f32 *)&mesh->local_aabb.max )[i] -
                     (double)( (const wp_f32 *)&mesh->local_aabb.min )[i] ) *
                   0.5;
    }
    axis_score = delta[0] * delta[1] + delta[0] * delta[2] + delta[1] * delta[2];
    if( mesh->local_aabb.min.x <= mesh->local_aabb.max.x &&
        mesh->local_aabb.min.y <= mesh->local_aabb.max.y &&
        mesh->local_aabb.min.z <= mesh->local_aabb.max.z && axis_score > 0 && score > axis_score )
    {
        /* Reproject rather than trust possibly authored AABB extents. */
        memset( basis, 0, sizeof( basis ) );
        for( i = 0; i < 3; ++i )
        {
            basis[i][i] = 1;
            low[i] = DBL_MAX;
            high[i] = -DBL_MAX;
        }
        for( n = 0; n < mesh->vertex_count; ++n )
        {
            vertex = (const wp_vec3f *)( data + (size_t)n * stride );
            for( i = 0; i < 3; ++i )
            {
                value = ( (const wp_f32 *)vertex )[i] - mean[i];
                if( value < low[i] )
                {
                    low[i] = value;
                }
                if( value > high[i] )
                {
                    high[i] = value;
                }
            }
        }
        for( i = 0; i < 3; ++i )
        {
            middle[i] = ( low[i] + high[i] ) * 0.5;
            extent[i] = ( high[i] - low[i] ) * 0.5;
        }
    }
    memset( &box, 0, sizeof( box ) );
    pad = 1.0e-6 * scale +
          32 * DBL_EPSILON * ( fabs( mean[0] ) + fabs( mean[1] ) + fabs( mean[2] ) + scale ) + 1.0e-12;
    for( i = 0; i < 3; ++i )
    {
        value = mean[i];
        for( j = 0; j < 3; ++j )
        {
            value += basis[i][j] * middle[j];
        }
        ( (double *)&box.center )[i] = value;
        for( j = 0; j < 3; ++j )
        {
            ( (double *)&box.half_axes[j] )[i] = basis[i][j] * ( extent[j] + pad );
        }
    }
    return wp_graphics_mesh_set_local_obb( mesh, &box );
}

static wp_s32 wp_mesh_compute_aabb_only( wp_graphics_mesh *mesh )
{
    const wp_u8 *data;
    const wp_vec3f *pos;
    wp_u32 stride;
    wp_u32 i;
    wp_s32 axis;

    if( !mesh || !mesh->vertices || mesh->vertex_count == 0 )
    {
        return 0;
    }

    stride = wp_vertex_format_stride( mesh->vertex_format );
    if( stride == 0 )
    {
        return 0;
    }

    data = (const wp_u8 *)mesh->vertices;
    pos = (const wp_vec3f *)data;

    mesh->content_generation = wp_mesh_next_generation();
    mesh->aabb_authored = 0;
    mesh->has_obb = 0;
    for( i = 0; i < mesh->vertex_count; ++i )
    {
        pos = (const wp_vec3f *)( data + (size_t)i * stride );
        for( axis = 0; axis < 3; ++axis )
        {
            if( !( ( (const wp_f32 *)pos )[axis] <= FLT_MAX &&
                   ( (const wp_f32 *)pos )[axis] >= -FLT_MAX ) )
            {
                memset( &mesh->local_aabb, 0, sizeof( mesh->local_aabb ) );
                return 0;
            }
        }
    }
    pos = (const wp_vec3f *)data;
    mesh->local_aabb.min = *pos;
    mesh->local_aabb.max = *pos;

    for( i = 1; i < mesh->vertex_count; i++ )
    {
        pos = (const wp_vec3f *)( data + (size_t)i * stride );

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
    return 1;
}

void wp_graphics_mesh_compute_aabb( wp_graphics_mesh *mesh )
{
    if( wp_mesh_compute_aabb_only( mesh ) )
    {
        wp_graphics_mesh_compute_obb( mesh );
    }
}

static wp_s32 wp_mesh_refit_orientation( wp_graphics_mesh *mesh, const wp_graphics_obb *previous )
{
    double basis[3][3], origin[3], low[3], high[3], lengths[3], value, dot, extent, scale = 0, pad;
    wp_graphics_obb box;
    const wp_u8 *data = (const wp_u8 *)mesh->vertices;
    const wp_vec3f *position;
    wp_u32 n, stride = wp_vertex_format_stride( mesh->vertex_format );
    wp_s32 i, j, k;
    if( !previous )
    {
        return 0;
    }
    for( i = 0; i < 3; ++i )
    {
        lengths[i] = 0;
        for( j = 0; j < 3; ++j )
        {
            value = ( (const double *)&previous->half_axes[i] )[j];
            if( !wp_mesh_finite( value ) )
            {
                return 0;
            }
            basis[i][j] = value;
            lengths[i] += value * value;
        }
        if( !wp_mesh_finite( lengths[i] ) || lengths[i] <= DBL_MIN )
        {
            return 0;
        }
        lengths[i] = sqrt( lengths[i] );
        for( j = 0; j < 3; ++j )
        {
            basis[i][j] /= lengths[i];
        }
        for( k = 0; k < i; ++k )
        {
            dot = 0;
            for( j = 0; j < 3; ++j )
            {
                dot += basis[i][j] * basis[k][j];
            }
            if( fabs( dot ) > 1.0e-8 )
            {
                return 0;
            }
        }
    }
    /* Orthonormalize accepted near-orthogonal directions before projecting. */
    dot = 0;
    for( j = 0; j < 3; ++j )
    {
        dot += basis[0][j] * basis[1][j];
    }
    value = 0;
    for( j = 0; j < 3; ++j )
    {
        basis[1][j] -= dot * basis[0][j];
        value += basis[1][j] * basis[1][j];
    }
    value = sqrt( value );
    for( j = 0; j < 3; ++j )
    {
        basis[1][j] /= value;
    }
    for( j = 0; j < 3; ++j )
    {
        basis[2][j] = basis[0][( j + 1 ) % 3] * basis[1][( j + 2 ) % 3] -
                      basis[0][( j + 2 ) % 3] * basis[1][( j + 1 ) % 3];
    }
    for( j = 0; j < 3; ++j )
    {
        origin[j] = ( (const wp_f32 *)&mesh->local_aabb.min )[j] * 0.5 +
                    ( (const wp_f32 *)&mesh->local_aabb.max )[j] * 0.5;
        low[j] = DBL_MAX;
        high[j] = -DBL_MAX;
    }
    for( n = 0; n < mesh->vertex_count; ++n )
    {
        position = (const wp_vec3f *)( data + (size_t)n * stride );
        for( i = 0; i < 3; ++i )
        {
            value = 0;
            for( j = 0; j < 3; ++j )
            {
                value += basis[i][j] * ( ( (const wp_f32 *)position )[j] - origin[j] );
            }
            if( value < low[i] )
            {
                low[i] = value;
            }
            if( value > high[i] )
            {
                high[i] = value;
            }
        }
    }
    for( i = 0; i < 3; ++i )
    {
        extent = ( high[i] - low[i] ) * 0.5;
        if( extent > scale )
        {
            scale = extent;
        }
    }
    pad = 1.0e-6 * scale +
          32 * DBL_EPSILON * ( fabs( origin[0] ) + fabs( origin[1] ) + fabs( origin[2] ) + scale ) +
          1.0e-12;
    memset( &box, 0, sizeof( box ) );
    for( j = 0; j < 3; ++j )
    {
        value = origin[j];
        for( i = 0; i < 3; ++i )
        {
            value += basis[i][j] * ( low[i] + high[i] ) * 0.5;
            ( (double *)&box.half_axes[i] )[j] = basis[i][j] * ( ( high[i] - low[i] ) * 0.5 + pad );
        }
        ( (double *)&box.center )[j] = value;
    }
    if( !wp_graphics_mesh_set_local_obb( mesh, &box ) )
    {
        return 0;
    }
    ++mesh->bounds_stats.orientation_refits;
    return 1;
}

wp_s32 wp_graphics_mesh_refit_bounds( wp_graphics_mesh *mesh, const wp_graphics_obb *previous )
{
    if( !wp_mesh_compute_aabb_only( mesh ) )
    {
        return 0;
    }
    if( wp_mesh_refit_orientation( mesh, previous ) )
    {
        return 1;
    }
    return wp_graphics_mesh_compute_obb( mesh );
}

wp_s32 wp_graphics_mesh_refit_bounds_adaptive( wp_graphics_mesh *mesh, const wp_graphics_obb *previous,
                                               wp_f64 max_surface_ratio )
{
    wp_u64 refits;
    wp_s32 i;
    double half[3], box_half[3], scale = 0, pad, box_score, axis_score, ratio;
    wp_graphics_obb retained;
    if( !mesh || !wp_mesh_finite( max_surface_ratio ) || max_surface_ratio < 1 )
    {
        return 0;
    }
    refits = mesh->bounds_stats.orientation_refits;
    if( !wp_graphics_mesh_refit_bounds( mesh, previous ) )
    {
        return 0;
    }
    /* Missing/invalid prior directions already requested a full fit. */
    if( mesh->bounds_stats.orientation_refits == refits )
    {
        return 1;
    }
    for( i = 0; i < 3; ++i )
    {
        half[i] = ( ( (const wp_f32 *)&mesh->local_aabb.max )[i] -
                    (double)( (const wp_f32 *)&mesh->local_aabb.min )[i] ) *
                  0.5;
        if( half[i] > scale )
        {
            scale = half[i];
        }
        box_half[i] =
            sqrt( wp_mesh_dot( &mesh->local_obb.half_axes[i], &mesh->local_obb.half_axes[i] ) );
    }
    /* Match fitting's numerical margin so points/lines/planes cannot force a
     * refresh solely because one of the unpadded AABB dimensions is zero. */
    pad = 1.0e-6 * scale +
          32 * DBL_EPSILON *
              ( fabs( mesh->local_obb.center.x ) + fabs( mesh->local_obb.center.y ) +
                fabs( mesh->local_obb.center.z ) + scale ) +
          1.0e-12;
    for( i = 0; i < 3; ++i )
    {
        half[i] += pad;
    }
    box_score = box_half[0] * box_half[1] + box_half[0] * box_half[2] + box_half[1] * box_half[2];
    axis_score = half[0] * half[1] + half[0] * half[2] + half[1] * half[2];
    ratio = box_score / axis_score;
    if( ratio <= max_surface_ratio || ratio - max_surface_ratio <= 1.0e-12 * ratio )
    {
        return 1;
    }
    retained = mesh->local_obb;
    if( wp_graphics_mesh_compute_obb( mesh ) )
    {
        ++mesh->bounds_stats.adaptive_refreshes;
    }
    else
    {
        wp_graphics_mesh_set_local_obb( mesh, &retained );
    }
    return 1;
}

void wp_graphics_mesh_get_bounds_stats( const wp_graphics_mesh *mesh,
                                        wp_graphics_mesh_bounds_stats *out )
{
    if( !out )
    {
        return;
    }
    if( mesh )
    {
        *out = mesh->bounds_stats;
    }
    else
    {
        memset( out, 0, sizeof( *out ) );
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
    mesh->has_obb = 0;
    if( !mesh->aabb_authored )
    {
        memset( &mesh->local_aabb, 0, sizeof( mesh->local_aabb ) );
    }
    wp_mesh_invalidate_lit( mesh );
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

    vertices = (wp_vertex_ptc *)wp_mesh_memory_allocate( wp_mesh_callbacks( mesh ),
        (size_t)mesh->vertex_count * sizeof( wp_vertex_ptc ), 0 );
    if( !vertices )
    {
        return 0;
    }
    for( i = 0u; i < mesh->vertex_count; ++i )
    {
        vertices[i] = wp_graphics_mesh_read_ptc( mesh, i );
    }

    wp_mesh_memory_free( mesh->render_vertices );
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
    if( !mesh->indices )
    {
        return i;
    }
    return mesh->index_format == WORKPHONE_INDEX_FORMAT_UINT32 ? ( (const wp_u32 *)mesh->indices )[i]
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
    if( !mesh || !mesh->vertices || !mesh->vertex_count )
    {
        return NULL;
    }
    if( mesh->lit_vertices )
    {
        return mesh->lit_vertices;
    }
    if( (size_t)mesh->vertex_count > (size_t)-1 / sizeof( *vertices ) )
    {
        return NULL;
    }
    stride = wp_vertex_format_stride( mesh->vertex_format );
    if( !stride )
    {
        return NULL;
    }
    vertices = (wp_vertex_pntc *)wp_mesh_memory_allocate( wp_mesh_callbacks( mesh ),
        (size_t)mesh->vertex_count * sizeof( *vertices ), 0 );
    if( !vertices )
    {
        return NULL;
    }
    memset( vertices, 0, (size_t)mesh->vertex_count * sizeof( *vertices ) );
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
            vertices[i].normal = ( (const wp_graphics_mesh_vertex_pn *)source )->normal;
            break;
        case WORKPHONE_VERTEX_FORMAT_PT:
            vertices[i].uv = ( (const wp_graphics_mesh_vertex_pt *)source )->uv;
            break;
        case WORKPHONE_VERTEX_FORMAT_PNT:
            vertices[i].normal = ( (const wp_graphics_mesh_vertex_pnt *)source )->normal;
            vertices[i].uv = ( (const wp_graphics_mesh_vertex_pnt *)source )->uv;
            break;
        case WORKPHONE_VERTEX_FORMAT_PNTC:
            vertices[i].normal = ( (const wp_graphics_mesh_vertex_pntc *)source )->normal;
            vertices[i].uv = ( (const wp_graphics_mesh_vertex_pntc *)source )->uv;
            vertices[i].color = ( (const wp_graphics_mesh_vertex_pntc *)source )->color;
            break;
        case WORKPHONE_VERTEX_FORMAT_PC:
            vertices[i].color = ( (const wp_graphics_mesh_vertex_pc *)source )->color;
            break;
        case WORKPHONE_VERTEX_FORMAT_PTC:
            vertices[i].uv = ( (const wp_graphics_mesh_vertex_ptc *)source )->uv;
            vertices[i].color = ( (const wp_graphics_mesh_vertex_ptc *)source )->color;
            break;
        default:
            break;
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
                a = wp_mesh_lit_index( mesh, i );
                b = wp_mesh_lit_index( mesh, i + 1u );
                c = wp_mesh_lit_index( mesh, i + 2u );
                if( step == 1u && ( i & 1u ) )
                {
                    swap = a;
                    a = b;
                    b = swap;
                }
                if( a >= mesh->vertex_count || b >= mesh->vertex_count || c >= mesh->vertex_count )
                {
                    wp_mesh_memory_free( vertices );
                    return NULL;
                }
                ab.x = vertices[b].position.x - vertices[a].position.x;
                ab.y = vertices[b].position.y - vertices[a].position.y;
                ab.z = vertices[b].position.z - vertices[a].position.z;
                ac.x = vertices[c].position.x - vertices[a].position.x;
                ac.y = vertices[c].position.y - vertices[a].position.y;
                ac.z = vertices[c].position.z - vertices[a].position.z;
                normal.x = ab.y * ac.z - ab.z * ac.y;
                normal.y = ab.z * ac.x - ab.x * ac.z;
                normal.z = ab.x * ac.y - ab.y * ac.x;
                vertices[a].normal.x += normal.x;
                vertices[a].normal.y += normal.y;
                vertices[a].normal.z += normal.z;
                vertices[b].normal.x += normal.x;
                vertices[b].normal.y += normal.y;
                vertices[b].normal.z += normal.z;
                vertices[c].normal.x += normal.x;
                vertices[c].normal.y += normal.y;
                vertices[c].normal.z += normal.z;
            }
        }
        for( i = 0; i < mesh->vertex_count; ++i )
        {
            normal = vertices[i].normal;
            length = sqrt( (double)normal.x * normal.x + (double)normal.y * normal.y +
                           (double)normal.z * normal.z );
            if( length > 1.0e-6 )
            {
                vertices[i].normal.x = (float)( normal.x / length );
                vertices[i].normal.y = (float)( normal.y / length );
                vertices[i].normal.z = (float)( normal.z / length );
            }
            else
            {
                vertices[i].normal.x = 0.0f;
                vertices[i].normal.y = 1.0f;
                vertices[i].normal.z = 0.0f;
            }
        }
    }
    mesh->lit_vertices = vertices;
    return vertices;
}
