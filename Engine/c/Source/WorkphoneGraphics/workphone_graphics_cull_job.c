/**
 * @file workphone_graphics_cull_job.c
 * @brief Snapshot-based per-view cull job.
 */

#include "workphone_graphics_cull_job.h"
#include "workphone_graphics_camera.h"
#include "workphone_graphics_scenenode.h"
#include "workphone_graphics_object.h"
#include "workphone_graphics_mesh.h"
#include "workphone_graphics_transform_job.h"
#include "graphics_cull_bounds.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define WP_CULL_GROUP_SIZE 32
#define WP_CULL_GROUP_MINIMUM 64

typedef struct cull_node_revision
{
    const wp_scenenode *node;
    wp_u64 revision;
} cull_node_revision;

typedef struct cull_entry
{
    wp_graphics_object *object;
    wp_graphics_mesh *mesh;
    wp_u64 object_generation, mesh_generation;
    cull_node_revision *ancestors;
    wp_u32 ancestor_count;
    wp_u32 visibility_flags;
    wp_u32 queue_group;
    wp_u32 z_order;
    wp_s32 visible;
    wp_s32 has_render_func;
    wp_s32 casts_shadows;
    wp_s32 has_bounds;
    wp_aabb3f bounds;
    wp_graphics_obb local_obb;
    wp_s32 has_obb;
    wp_cull_box box;
    wp_mat4f world;
} cull_entry;

/* Reference changes occur only on the owner. Workers may read retained
 * immutable entries but may not derive/destroy views concurrently. */
typedef struct cull_snapshot
{
    cull_entry *entries;
    wp_s32 count;
    wp_u32 references;
    wp_cull_box *groups;
    wp_s32 *entry_groups;
    wp_s32 group_count;
} cull_snapshot;

struct wp_graphics_cull_result
{
    cull_snapshot *snapshot;
    wp_u32 transform_dispatches;
    cull_entry *entries;
    wp_s32 *indices;
    const wp_graphics_scene *scene;
    wp_u64 native_generation;
    wp_mat4f view, projection;
    wp_s32 has_camera;
    const wp_camera *camera;
    wp_u32 camera_mask, scene_mask;
    wp_s32 visible_count;
    wp_s32 candidate_count;
    wp_u32 scene_generation;
    wp_graphics_cull_pass pass;
    wp_transform_job_stats transform_stats;
    wp_graphics_cull_bounds_stats bounds_stats;
    wp_graphics_cull_sort_stats sort_stats;
    wp_s32 sphere_test, sat_test, group_test, spatial_test;
};

struct wp_graphics_cull_job
{
    wp_graphics_cull_status status;
    cull_snapshot *shared_snapshot; /* internal borrowed input for view derivation */
    cull_entry *entries;
    wp_cull_box *groups;
    wp_s32 *entry_groups;
    wp_s32 group_count;
    wp_s32 entry_count;
    wp_u32 visibility_mask;
    wp_s32 has_camera;
    const wp_camera *camera;
    wp_u32 camera_mask, scene_mask;
    wp_mat4f view_projection, view, projection;
    wp_cull_frustum frustum;
    wp_s32 sphere_test, sat_test, group_test, spatial_test;
    const wp_graphics_scene *scene;
    wp_u64 native_generation;
    wp_transform_job_system *transforms;
    wp_graphics_cull_pass pass;
    wp_transform_job_stats transform_stats;
    wp_u32 scene_generation;
    wp_graphics_cull_result *result;
};

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void cull_job_release_snapshot( wp_graphics_cull_job *job )
{
    wp_s32 i;
    if( job->entries )
    {
        for( i = 0; i < job->entry_count; ++i )
        {
            free( job->entries[i].ancestors );
        }
    }
    free( job->groups );
    free( job->entry_groups );
    job->entry_groups = NULL;
    job->group_count = 0;
    job->groups = NULL;
    free( job->entries );
    job->entries = NULL;
    job->entry_count = 0;
}

/* Spatial bins split the widest remaining cell dimension, without changing entry or
 * draw-list traversal order. Unknown bounds remain individual candidates. */
static wp_s32 cull_prepare_spatial_groups( wp_graphics_cull_job *job, size_t bytes )
{
    wp_s32 i, j, k, axis, bin, count, target, grid[3], cell[3], spanning;
    double low[3], high[3], spans[3], extents[3], ratio, extent, a, b, squared, padding;
    const wp_cull_box *box;
    wp_cull_box *group;
    size_t group_bytes, map_bytes;
    for( j = 0; j < 3; ++j )
    {
        low[j] = DBL_MAX;
        high[j] = -DBL_MAX;
    }
    for( i = 0; i < job->entry_count; ++i )
    {
        box = &job->entries[i].box;
        if( !box->valid )
        {
            continue;
        }
        for( j = 0; j < 3; ++j )
        {
            if( box->center[j] < low[j] )
            {
                low[j] = box->center[j];
            }
            if( box->center[j] > high[j] )
            {
                high[j] = box->center[j];
            }
        }
    }
    if( low[0] > high[0] )
    {
        return 0;
    }
    for( j = 0; j < 3; ++j )
    {
        spans[j] = high[j] - low[j];
        if( !wp_cull_finite( spans[j] ) )
        {
            return 0;
        }
        grid[j] = 1;
    }
    target = job->entry_count / WP_CULL_GROUP_SIZE + ( job->entry_count % WP_CULL_GROUP_SIZE != 0 );
    count = 1;
    /* Power-of-two grids fit the existing bin budget. Equal spans prefer X/Y/Z
     * deterministically; coincident centres need only one occupied cell. */
    while( count <= target / 2 )
    {
        axis = 0;
        for( j = 1; j < 3; ++j )
        {
            if( spans[j] / grid[j] > spans[axis] / grid[axis] )
            {
                axis = j;
            }
        }
        if( spans[axis] <= 0 )
        {
            break;
        }
        grid[axis] *= 2;
        count *= 2;
    }
    group_bytes = (size_t)count * sizeof( *group );
    map_bytes = (size_t)job->entry_count * sizeof( *job->entry_groups );
    /* Include a view's optional classification bytes as well as retained arrays. */
    if( group_bytes + map_bytes + (size_t)count > 64u * 1024u * 1024u - bytes )
    {
        return 0;
    }
    job->groups = (wp_cull_box *)calloc( (size_t)count, sizeof( *group ) );
    job->entry_groups = (wp_s32 *)malloc( map_bytes );
    if( !job->groups || !job->entry_groups )
    {
        free( job->groups );
        free( job->entry_groups );
        job->groups = NULL;
        job->entry_groups = NULL;
        return 0;
    }
    job->group_count = count;
    for( i = 0; i < job->entry_count; ++i )
    {
        box = &job->entries[i].box;
        job->entry_groups[i] = -1;
        if( !box->valid )
        {
            continue;
        }
        spanning = 0;
        for( j = 0; j < 3; ++j )
        {
            extents[j] = 0;
            for( k = 0; k < 3; ++k )
            {
                extents[j] += fabs( box->axes[k][j] );
            }
            if( grid[j] == 1 )
            {
                cell[j] = 0;
                continue;
            }
            ratio = spans[j] > 0 ? ( box->center[j] - low[j] ) / spans[j] : 0;
            cell[j] = (wp_s32)( ratio * grid[j] );
            if( cell[j] < 0 )
            {
                cell[j] = 0;
            }
            if( cell[j] >= grid[j] )
            {
                cell[j] = grid[j] - 1;
            }
            /* Grid limits describe centres, not geometry. Only interior planes
             * separate neighbours: outer cells may extend beyond the centre range.
             * A cross-cell bound remains individually tested instead of loosening
             * one bin enough to prevent rejection of its small neighbours. */
            extent = spans[j] / grid[j];
            a = low[j] + extent * cell[j];
            b = low[j] + extent * ( cell[j] + 1 );
            if( ( cell[j] > 0 && box->center[j] - extents[j] < a ) ||
                ( cell[j] + 1 < grid[j] && box->center[j] + extents[j] > b ) )
            {
                spanning = 1;
            }
        }
        if( spanning )
        {
            continue;
        }
        bin = ( cell[0] * grid[1] + cell[1] ) * grid[2] + cell[2];
        group = &job->groups[bin];
        /* During accumulation, centre/first axis hold lower/upper bounds. */
        for( j = 0; j < 3; ++j )
        {
            a = box->center[j] - extents[j];
            b = box->center[j] + extents[j];
            if( !group->valid || a < group->center[j] )
            {
                group->center[j] = a;
            }
            if( !group->valid || b > group->axes[0][j] )
            {
                group->axes[0][j] = b;
            }
        }
        group->valid = 1;
        job->entry_groups[i] = bin;
    }
    for( bin = 0; bin < count; ++bin )
    {
        group = &job->groups[bin];
        if( !group->valid )
        {
            continue;
        }
        for( j = 0; j < 3; ++j )
        {
            low[j] = group->center[j];
            high[j] = group->axes[0][j];
        }
        memset( group, 0, sizeof( *group ) );
        group->valid = 1;
        squared = 0;
        for( j = 0; j < 3; ++j )
        {
            padding = 1.0e-4 * ( 1 + fabs( low[j] ) + fabs( high[j] ) );
            group->center[j] = 0.5 * low[j] + 0.5 * high[j];
            extent = 0.5 * high[j] - 0.5 * low[j] + padding;
            group->axes[j][j] = extent;
            squared += extent * extent;
            if( !wp_cull_finite( group->center[j] ) || !wp_cull_finite( extent ) )
            {
                group->valid = 0;
            }
        }
        group->radius = sqrt( squared );
        if( !wp_cull_finite( group->radius ) )
        {
            group->valid = 0;
        }
    }
    return 1;
}

/* Immutable coarse bounds over capture-order blocks; no reordering of objects.
 * A single invalid member disables its entire block. Padding covers child test
 * tolerances as well as rounding of the enclosing half-axis representation. */
static void cull_prepare_groups( wp_graphics_cull_job *job, size_t bytes )
{
    wp_s32 first, end, i, j, k, valid, group_count;
    double low[3], high[3], extent, minimum, maximum, padding, squared;
    const wp_cull_box *box;
    wp_cull_box *group;
    if( job->spatial_test && job->entry_count >= WP_CULL_GROUP_MINIMUM &&
        cull_prepare_spatial_groups( job, bytes ) )
    {
        return;
    }
    if( !job->group_test || job->entry_count < WP_CULL_GROUP_MINIMUM )
    {
        return;
    }
    group_count = job->entry_count / WP_CULL_GROUP_SIZE + ( job->entry_count % WP_CULL_GROUP_SIZE != 0 );
    if( (size_t)group_count > ( 64u * 1024u * 1024u - bytes ) / sizeof( *group ) )
    {
        return;
    }
    job->groups = (wp_cull_box *)calloc( (size_t)group_count, sizeof( *group ) );
    if( !job->groups )
    {
        return; /* Optional optimization: retain the flat path. */
    }
    job->group_count = group_count;
    for( first = 0; first < job->entry_count; first = end )
    {
        end = job->entry_count - first < WP_CULL_GROUP_SIZE ? job->entry_count
                                                            : first + WP_CULL_GROUP_SIZE;
        group = &job->groups[first / WP_CULL_GROUP_SIZE];
        for( j = 0; j < 3; ++j )
        {
            low[j] = DBL_MAX;
            high[j] = -DBL_MAX;
        }
        valid = 1;
        for( i = first; i < end; ++i )
        {
            box = &job->entries[i].box;
            if( !box->valid )
            {
                valid = 0;
                break;
            }
            for( j = 0; j < 3; ++j )
            {
                extent = 0;
                for( k = 0; k < 3; ++k )
                {
                    extent += fabs( box->axes[k][j] );
                }
                minimum = box->center[j] - extent;
                maximum = box->center[j] + extent;
                if( !wp_cull_finite( minimum ) || !wp_cull_finite( maximum ) )
                {
                    valid = 0;
                }
                if( minimum < low[j] )
                {
                    low[j] = minimum;
                }
                if( maximum > high[j] )
                {
                    high[j] = maximum;
                }
            }
        }
        if( !valid )
        {
            continue;
        }
        squared = 0;
        for( j = 0; j < 3; ++j )
        {
            padding = 1.0e-4 * ( 1 + fabs( low[j] ) + fabs( high[j] ) );
            group->center[j] = 0.5 * low[j] + 0.5 * high[j];
            extent = 0.5 * high[j] - 0.5 * low[j] + padding;
            group->axes[j][j] = extent;
            squared += extent * extent;
            if( !wp_cull_finite( group->center[j] ) || !wp_cull_finite( extent ) )
            {
                valid = 0;
            }
        }
        group->radius = sqrt( squared );
        group->valid = valid && wp_cull_finite( group->radius );
    }
}

static void cull_job_release_result( wp_graphics_cull_job *job )
{
    wp_graphics_cull_result_destroy( job->result );
    job->result = NULL;
}

static wp_s32 cull_entry_before( const cull_entry *left, const cull_entry *right )
{
    if( left->queue_group != right->queue_group )
    {
        return left->queue_group < right->queue_group;
    }
    return left->z_order < right->z_order;
}

/* Stable bottom-up merge sort of entry indices. Returns the sorted buffer. */
static wp_s32 *cull_sort_indices( const cull_entry *entries, wp_s32 *source, wp_s32 *scratch,
                                  wp_s32 count, wp_graphics_cull_sort_stats *stats )
{
    wp_s32 *destination;
    wp_s32 *swap;
    wp_s32 width;
    wp_s32 left, middle, right;
    wp_s32 li, ri, out;

    destination = scratch;
    for( width = 1; width < count; )
    {
        ++stats->merge_passes;
        for( left = 0; left < count; left = right )
        {
            middle = width > count - left ? count : left + width;
            right = width > count - middle ? count : middle + width;
            li = left;
            ri = middle;
            out = left;
            while( li < middle && ri < right )
            {
                if( cull_entry_before( &entries[source[ri]], &entries[source[li]] ) )
                {
                    destination[out++] = source[ri++];
                }
                else
                {
                    destination[out++] = source[li++];
                }
            }
            while( li < middle )
            {
                destination[out++] = source[li++];
            }
            while( ri < right )
            {
                destination[out++] = source[ri++];
            }
        }
        swap = source;
        source = destination;
        destination = swap;
        if( width >= count - width )
        {
            break;
        }
        width *= 2;
    }
    return source;
}

static wp_s32 cull_capture_entry( cull_entry *entry, wp_graphics_object *obj, size_t *bytes )
{
    wp_graphics_mesh *mesh;
    wp_aabb3f bounds;
    const wp_scenenode *node;
    wp_u32 count, i;

    memset( entry, 0, sizeof( *entry ) );
    entry->object = obj;
    if( !obj )
    {
        return 1;
    }

    entry->object_generation = wp_graphics_object_get_generation( obj );
    count = 0;
    node = wp_graphics_object_get_owner( obj );
    while( node )
    {
        if( count == 257u )
        {
            return 0;
        }
        ++count;
        node = wp_scenenode_get_parent( node );
    }
    if( count > ( ( 64u * 1024u * 1024u ) - *bytes ) / sizeof( cull_node_revision ) )
    {
        return 0;
    }
    if( count )
    {
        entry->ancestors = (cull_node_revision *)malloc( (size_t)count * sizeof( cull_node_revision ) );
        if( !entry->ancestors )
        {
            return 0;
        }
        entry->ancestor_count = count;
        *bytes += (size_t)count * sizeof( cull_node_revision );
        node = wp_graphics_object_get_owner( obj );
        for( i = 0; i < count; ++i )
        {
            entry->ancestors[i].node = node;
            entry->ancestors[i].revision = wp_scenenode_get_generation( node );
            node = wp_scenenode_get_parent( node );
        }
    }
    entry->visible = wp_graphics_object_is_visible( obj );
    entry->visibility_flags = wp_graphics_object_get_visibility_flags( obj );
    entry->queue_group = wp_graphics_object_get_render_queue_group( obj );
    entry->z_order = wp_graphics_object_get_z_order( obj );
    entry->has_render_func = wp_graphics_object_get_render_func( obj ) != NULL;
    entry->casts_shadows = wp_graphics_object_get_cast_shadows( obj );

    /* Current mesh bounds; unknown, inverted or degenerate bounds stay visible. */
    mesh = wp_graphics_object_get_mesh( obj );
    entry->mesh = mesh;
    entry->mesh_generation = mesh ? wp_graphics_mesh_get_content_generation( mesh ) : 0;
    if( mesh )
    {
        entry->has_obb = wp_graphics_mesh_get_local_obb( mesh, &entry->local_obb );
        bounds = wp_graphics_mesh_get_local_aabb( mesh );
        if( bounds.min.x >= -FLT_MAX && bounds.max.x <= FLT_MAX && bounds.min.y >= -FLT_MAX &&
            bounds.max.y <= FLT_MAX && bounds.min.z >= -FLT_MAX && bounds.max.z <= FLT_MAX &&
            bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y &&
            bounds.min.z <= bounds.max.z &&
            !( bounds.min.x == bounds.max.x && bounds.min.y == bounds.max.y &&
               bounds.min.z == bounds.max.z ) )
        {
            entry->bounds = bounds;
            entry->has_bounds = 1;
        }
    }
    return 1;
}

/* =========================================================================
 * Job lifecycle
 * ====================================================================== */

wp_graphics_cull_job *wp_graphics_cull_job_create( void )
{
    wp_graphics_cull_job *job;

    job = (wp_graphics_cull_job *)malloc( sizeof( *job ) );
    if( !job )
    {
        return NULL;
    }
    memset( job, 0, sizeof( *job ) );
    job->status = WP_GRAPHICS_CULL_STATUS_IDLE;
    job->sphere_test = 1;
    job->sat_test = 1;
    job->transforms = wp_transform_job_system_create( NULL, 64 );
    if( !job->transforms )
    {
        free( job );
        return NULL;
    }
    return job;
}

void wp_graphics_cull_job_destroy( wp_graphics_cull_job *job )
{
    if( !job )
    {
        return;
    }
    cull_job_release_snapshot( job );
    cull_job_release_result( job );
    wp_transform_job_system_destroy( job->transforms );
    free( job );
}

wp_s32 wp_graphics_cull_job_set_threadpool( wp_graphics_cull_job *job, wp_threadpool *pool )
{
    wp_transform_job_system *next;
    if( !job || job->status == WP_GRAPHICS_CULL_STATUS_READY )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_STATE;
    }
    next = wp_transform_job_system_create( pool, 64 );
    if( !next )
    {
        return WP_GRAPHICS_CULL_ERROR_OUT_OF_MEMORY;
    }
    wp_transform_job_system_destroy( job->transforms );
    job->transforms = next;
    return WP_GRAPHICS_CULL_OK;
}

static void cull_capture_view( wp_graphics_cull_job *job, const wp_graphics_scene *scene,
                               const wp_camera *camera, wp_graphics_cull_pass pass,
                               wp_u32 scene_generation )
{
    wp_camera *view_camera;
    wp_mat4f view_matrix, projection_matrix;
    wp_u32 mask;
    view_camera = (wp_camera *)camera;
    if( !view_camera )
    {
        view_camera = scene->active_camera ? scene->active_camera : scene->default_camera;
    }

    mask = scene->visibility_mask;
    job->has_camera = view_camera != NULL;
    job->camera = view_camera;
    job->camera_mask = view_camera ? wp_camera_get_visibility_mask( view_camera ) : 0;
    if( view_camera )
    {
        mask &= wp_camera_get_visibility_mask( view_camera );
        wp_camera_get_view_matrix( view_camera, &view_matrix );
        wp_camera_get_projection_matrix( view_camera, &projection_matrix );
        wp_mat4f_mul( &job->view_projection, &projection_matrix, &view_matrix );
        wp_cull_prepare_frustum( &job->frustum, &job->view_projection );
        if( job->sat_test && pass == WP_GRAPHICS_CULL_PASS_MAIN )
        {
            wp_cull_prepare_sat_frustum( &job->frustum );
        }
    }
    if( view_camera )
    {
        job->view = view_matrix;
        job->projection = projection_matrix;
    }
    job->scene = scene;
    job->native_generation = wp_graphics_scene_get_generation( scene );
    job->scene_mask = scene->visibility_mask;
    job->visibility_mask = mask;
    job->pass = pass;
    job->scene_generation = scene_generation;
}

wp_s32 wp_graphics_cull_job_configure( wp_graphics_cull_job *job, const wp_graphics_scene *scene,
                                       const wp_camera *camera, wp_graphics_cull_pass pass,
                                       wp_u32 scene_generation )
{
    wp_s32 i;
    size_t bytes;
    const wp_scenenode **nodes;
    const wp_mat4f *worlds;

    if( !job || !scene || scene->object_count < 0 || ( scene->object_count && !scene->objects ) )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_ARGUMENT;
    }
    if( pass != WP_GRAPHICS_CULL_PASS_MAIN && pass != WP_GRAPHICS_CULL_PASS_SHADOW )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_ARGUMENT;
    }
    if( job->status == WP_GRAPHICS_CULL_STATUS_READY )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_STATE;
    }

    cull_job_release_snapshot( job );
    cull_job_release_result( job );
    job->status = WP_GRAPHICS_CULL_STATUS_IDLE;
    memset( &job->transform_stats, 0, sizeof( job->transform_stats ) );

    if( scene->object_count > 0 )
    {
        if( (size_t)scene->object_count > ( 64u * 1024u * 1024u ) / sizeof( cull_entry ) )
        {
            return WP_GRAPHICS_CULL_ERROR_OUT_OF_MEMORY;
        }
        job->entries = (cull_entry *)calloc( (size_t)scene->object_count, sizeof( cull_entry ) );
        if( !job->entries )
        {
            return WP_GRAPHICS_CULL_ERROR_OUT_OF_MEMORY;
        }
        job->entry_count = scene->object_count;
        bytes = (size_t)scene->object_count * sizeof( cull_entry );
        for( i = 0; i < scene->object_count; ++i )
        {
            if( !cull_capture_entry( &job->entries[i], scene->objects[i], &bytes ) )
            {
                cull_job_release_snapshot( job );
                return WP_GRAPHICS_CULL_ERROR_OUT_OF_MEMORY;
            }
        }
        nodes = (const wp_scenenode **)malloc( (size_t)job->entry_count * sizeof( *nodes ) );
        if( !nodes )
        {
            cull_job_release_snapshot( job );
            return WP_GRAPHICS_CULL_ERROR_OUT_OF_MEMORY;
        }
        for( i = 0; i < job->entry_count; ++i )
        {
            nodes[i] = wp_graphics_object_get_owner( job->entries[i].object );
        }
        i = wp_transform_job_system_dispatch_snapshot( job->transforms, nodes, (wp_u32)job->entry_count,
                                                       wp_graphics_scene_get_generation( scene ) );
        free( nodes );
        if( !i )
        {
            cull_job_release_snapshot( job );
            return WP_GRAPHICS_CULL_ERROR_OUT_OF_MEMORY;
        }
        wp_transform_job_system_wait( job->transforms );
        wp_transform_job_system_get_stats( job->transforms, &job->transform_stats );
        worlds = wp_transform_job_system_get_result( job->transforms, NULL, NULL );
        for( i = 0; i < job->entry_count; ++i )
        {
            job->entries[i].world = worlds[i];
            wp_cull_prepare_box(
                &job->entries[i].box, job->entries[i].has_bounds ? &job->entries[i].bounds : NULL,
                job->entries[i].has_obb ? &job->entries[i].local_obb : NULL, &worlds[i] );
        }
    }

    if( scene->object_count > 0 )
    {
        cull_prepare_groups( job, bytes );
    }
    cull_capture_view( job, scene, camera, pass, scene_generation );
    job->status = WP_GRAPHICS_CULL_STATUS_READY;
    return WP_GRAPHICS_CULL_OK;
}

wp_s32 wp_graphics_cull_job_execute( wp_graphics_cull_job *job )
{
    wp_graphics_cull_result *result;
    cull_snapshot *snapshot;
    wp_s32 *indices;
    wp_s32 *scratch;
    wp_s32 *sorted;
    wp_s32 count;
    wp_s32 i, stage, outside, ordered, group_end, group_index, group_inside = 0;
    signed char *group_states;
    size_t alloc_count;
    const cull_entry *entry;

    if( !job )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_ARGUMENT;
    }
    if( job->status != WP_GRAPHICS_CULL_STATUS_READY )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_STATE;
    }

    alloc_count = job->entry_count > 0 ? (size_t)job->entry_count : 1u;
    snapshot =
        job->shared_snapshot ? job->shared_snapshot : (cull_snapshot *)malloc( sizeof( *snapshot ) );
    result = (wp_graphics_cull_result *)malloc( sizeof( *result ) );
    indices = (wp_s32 *)malloc( alloc_count * sizeof( wp_s32 ) );
    scratch = NULL;
    if( !snapshot || !result || !indices )
    {
        if( !job->shared_snapshot )
        {
            free( snapshot );
        }
        free( result );
        free( indices );
        free( scratch );
        job->status = WP_GRAPHICS_CULL_STATUS_FAILED;
        return WP_GRAPHICS_CULL_ERROR_OUT_OF_MEMORY;
    }
    memset( result, 0, sizeof( *result ) );

    /* Per-view cache never mutates shared bounds or affects stable traversal.
     * Missing optional storage simply executes the flat geometry path. */
    group_states = job->entry_groups && job->has_camera && job->pass == WP_GRAPHICS_CULL_PASS_MAIN
                       ? (signed char *)calloc( (size_t)job->group_count, 1 )
                       : NULL;
    count = 0;
    for( i = 0; i < job->entry_count; ++i )
    {
        if( job->entry_groups )
        {
            group_inside = 0;
            group_index = job->entry_groups[i];
            if( group_states && group_index >= 0 )
            {
                if( !group_states[group_index] )
                {
                    ++result->bounds_stats.group_tests;
                    outside = wp_cull_box_outside( &job->groups[group_index], &job->frustum, 0, &stage );
                    group_states[group_index] = (signed char)( outside ? -1 : stage ? stage : 6 );
                    if( outside )
                    {
                        ++result->bounds_stats.group_rejections;
                    }
                    else if( stage == 5 )
                    {
                        ++result->bounds_stats.group_inside;
                    }
                    else if( !stage )
                    {
                        ++result->bounds_stats.group_fallbacks;
                    }
                }
                if( group_states[group_index] == -1 )
                {
                    ++result->bounds_stats.group_skipped_candidates;
                    continue;
                }
                if( group_states[group_index] == 5 )
                {
                    group_inside = 1;
                    ++result->bounds_stats.group_inside_candidates;
                }
            }
        }
        else if( job->groups && job->group_test && job->has_camera &&
                 job->pass == WP_GRAPHICS_CULL_PASS_MAIN && i % WP_CULL_GROUP_SIZE == 0 )
        {
            group_inside = 0;
            group_end =
                job->entry_count - i < WP_CULL_GROUP_SIZE ? job->entry_count : i + WP_CULL_GROUP_SIZE;
            ++result->bounds_stats.group_tests;
            if( wp_cull_box_outside( &job->groups[i / WP_CULL_GROUP_SIZE], &job->frustum, 0, &stage ) )
            {
                ++result->bounds_stats.group_rejections;
                result->bounds_stats.group_skipped_candidates += (wp_u32)( group_end - i );
                i = group_end - 1;
                continue;
            }
            if( stage == 5 )
            {
                /* A padded enclosing bound is inside every plane. Only geometry
                 * tests are redundant: member visibility/masks/callbacks still apply. */
                group_inside = 1;
                ++result->bounds_stats.group_inside;
                result->bounds_stats.group_inside_candidates += (wp_u32)( group_end - i );
            }
            if( !stage )
            {
                ++result->bounds_stats.group_fallbacks;
            }
        }
        entry = &job->entries[i];
        if( !entry->object || !entry->visible )
        {
            continue;
        }
        if( !( entry->visibility_flags & job->visibility_mask ) )
        {
            continue;
        }
        if( job->pass == WP_GRAPHICS_CULL_PASS_SHADOW )
        {
            if( !entry->casts_shadows )
            {
                continue;
            }
        }
        else if( job->has_camera && !group_inside )
        {
            outside = wp_cull_box_outside( &entry->box, &job->frustum, job->sphere_test, &stage );
            if( !stage )
            {
                ++result->bounds_stats.conservative_candidates;
            }
            else
            {
                if( job->sphere_test )
                {
                    ++result->bounds_stats.sphere_tests;
                }
                if( stage == 1 )
                {
                    ++result->bounds_stats.sphere_rejections;
                }
                if( stage == 2 )
                {
                    ++result->bounds_stats.sphere_inside;
                }
                if( stage == 3 || stage == 4 || stage == 5 )
                {
                    ++result->bounds_stats.box_tests;
                    if( entry->box.fitted )
                    {
                        ++result->bounds_stats.fitted_obb_tests;
                    }
                    else
                    {
                        ++result->bounds_stats.aabb_box_tests;
                    }
                    if( stage == 3 )
                    {
                        ++result->bounds_stats.box_rejections;
                    }
                }
            }
            if( !outside && stage == 4 && job->sat_test )
            {
                if( job->frustum.sat_valid )
                {
                    wp_u32 tested;
                    ++result->bounds_stats.sat_tests;
                    outside = wp_cull_sat_outside( &entry->box, &job->frustum, &tested );
                    result->bounds_stats.sat_axis_tests += tested;
                    if( outside )
                    {
                        ++result->bounds_stats.sat_rejections;
                    }
                }
                else
                {
                    ++result->bounds_stats.sat_fallbacks;
                }
            }
            if( outside )
            {
                continue;
            }
        }
        if( !entry->has_render_func )
        {
            continue;
        }
        indices[count++] = i;
    }

    free( group_states );

    /* Equal keys retain capture order. Check only visible survivors: excluded
     * candidates must not force allocation or sorting of an already ordered view. */
    ordered = 1;
    for( i = 1; i < count; ++i )
    {
        ++result->sort_stats.order_checks;
        if( cull_entry_before( &job->entries[indices[i]], &job->entries[indices[i - 1]] ) )
        {
            ordered = 0;
            break;
        }
    }
    sorted = indices;
    if( !ordered )
    {
        scratch = (wp_s32 *)malloc( (size_t)count * sizeof( *scratch ) );
        if( !scratch )
        {
            if( !job->shared_snapshot )
            {
                free( snapshot );
            }
            free( result );
            free( indices );
            job->status = WP_GRAPHICS_CULL_STATUS_FAILED;
            return WP_GRAPHICS_CULL_ERROR_OUT_OF_MEMORY;
        }
        result->sort_stats.scratch_allocations = 1;
        result->sort_stats.scratch_bytes = (size_t)count * sizeof( *scratch );
        sorted = cull_sort_indices( job->entries, indices, scratch, count, &result->sort_stats );
    }

    if( job->shared_snapshot )
    {
        ++snapshot->references;
    }
    else
    {
        snapshot->groups = job->groups;
        snapshot->entry_groups = job->entry_groups;
        snapshot->group_count = job->group_count;
        snapshot->entries = job->entries;
        snapshot->count = job->entry_count;
        snapshot->references = 1;
    }
    result->snapshot = snapshot;
    result->transform_dispatches = job->shared_snapshot ? 0u : ( job->entry_count > 0 ? 1u : 0u );
    result->entries = job->entries;
    job->entries = NULL;
    job->groups = NULL;
    job->entry_groups = NULL;
    job->group_count = 0;
    result->indices = sorted;
    result->scene = job->scene;
    result->native_generation = job->native_generation;
    result->camera = job->camera;
    result->camera_mask = job->camera_mask;
    result->scene_mask = job->scene_mask;
    result->view = job->view;
    result->projection = job->projection;
    result->has_camera = job->has_camera;
    result->visible_count = count;
    result->candidate_count = job->entry_count;
    result->scene_generation = job->scene_generation;
    result->pass = job->pass;
    result->transform_stats = job->transform_stats;
    result->sphere_test = job->sphere_test;
    result->sat_test = job->sat_test;
    result->group_test = job->group_test;
    result->spatial_test = job->spatial_test;

    free( sorted == indices ? scratch : indices );
    cull_job_release_snapshot( job );
    cull_job_release_result( job );
    job->result = result;
    job->status = WP_GRAPHICS_CULL_STATUS_COMPLETE;
    return WP_GRAPHICS_CULL_OK;
}

void wp_graphics_cull_job_cancel( wp_graphics_cull_job *job )
{
    if( !job || job->status != WP_GRAPHICS_CULL_STATUS_READY )
    {
        return;
    }
    cull_job_release_snapshot( job );
    job->status = WP_GRAPHICS_CULL_STATUS_CANCELLED;
}

void wp_graphics_cull_job_reset( wp_graphics_cull_job *job )
{
    if( !job )
    {
        return;
    }
    cull_job_release_snapshot( job );
    cull_job_release_result( job );
    job->status = WP_GRAPHICS_CULL_STATUS_IDLE;
}

wp_graphics_cull_status wp_graphics_cull_job_get_status( const wp_graphics_cull_job *job )
{
    return job ? job->status : WP_GRAPHICS_CULL_STATUS_IDLE;
}

wp_graphics_cull_result *wp_graphics_cull_job_take_result( wp_graphics_cull_job *job )
{
    wp_graphics_cull_result *result;

    if( !job || job->status != WP_GRAPHICS_CULL_STATUS_COMPLETE )
    {
        return NULL;
    }
    result = job->result;
    job->result = NULL;
    job->status = WP_GRAPHICS_CULL_STATUS_IDLE;
    return result;
}

/* =========================================================================
 * Result
 * ====================================================================== */

void wp_graphics_cull_result_destroy( wp_graphics_cull_result *result )
{
    wp_s32 i;
    if( !result )
    {
        return;
    }
    if( --result->snapshot->references == 0 )
    {
        for( i = 0; i < result->snapshot->count; ++i )
        {
            free( result->snapshot->entries[i].ancestors );
        }
        free( result->snapshot->entry_groups );
        free( result->snapshot->groups );
        free( result->snapshot->entries );
        free( result->snapshot );
    }
    free( result->indices );
    free( result );
}

wp_s32 wp_graphics_cull_result_get_candidate_count( const wp_graphics_cull_result *result )
{
    return result ? result->candidate_count : 0;
}

wp_s32 wp_graphics_cull_result_get_visible_count( const wp_graphics_cull_result *result )
{
    return result ? result->visible_count : 0;
}

wp_graphics_object *wp_graphics_cull_result_get_object( const wp_graphics_cull_result *result,
                                                        wp_s32 index )
{
    if( !result || index < 0 || index >= result->visible_count )
    {
        return NULL;
    }
    return result->entries[result->indices[index]].object;
}

wp_u32 wp_graphics_cull_result_get_scene_generation( const wp_graphics_cull_result *result )
{
    return result ? result->scene_generation : 0;
}

wp_graphics_cull_pass wp_graphics_cull_result_get_pass( const wp_graphics_cull_result *result )
{
    return result ? result->pass : WP_GRAPHICS_CULL_PASS_MAIN;
}

wp_s32 wp_graphics_cull_result_get_world_matrix( const wp_graphics_cull_result *r, wp_s32 i,
                                                 wp_mat4f *out )
{
    if( !r || !out || i < 0 || i >= r->visible_count )
    {
        return 0;
    }
    *out = r->entries[r->indices[i]].world;
    return 1;
}

wp_s32 wp_graphics_cull_result_get_camera_matrices( const wp_graphics_cull_result *r, wp_mat4f *v,
                                                    wp_mat4f *p )
{
    if( !r || !r->has_camera || !v || !p )
    {
        return 0;
    }
    *v = r->view;
    *p = r->projection;
    return 1;
}

static wp_s32 cull_entry_valid( const cull_entry *e )
{
    const wp_scenenode *node;
    wp_u32 i;
    if( !e->object )
    {
        return 1;
    }
    if( wp_graphics_object_get_generation( e->object ) != e->object_generation ||
        ( e->mesh && wp_graphics_mesh_get_content_generation( e->mesh ) != e->mesh_generation ) )
    {
        return 0;
    }
    node = wp_graphics_object_get_owner( e->object );
    /* Compare the full path, not a max/hash of revisions: mutations can arrive
     * through different DLLs that each link the native static library. */
    for( i = 0; i < e->ancestor_count; ++i )
    {
        if( node != e->ancestors[i].node ||
            wp_scenenode_get_generation( node ) != e->ancestors[i].revision )
        {
            return 0;
        }
        node = wp_scenenode_get_parent( node );
    }
    return node == NULL;
}

static wp_s32 cull_membership_valid( const wp_graphics_cull_result *r, const wp_graphics_scene *scene,
                                     wp_u32 generation )
{
    return r && scene && r->scene == scene &&
           r->native_generation == wp_graphics_scene_get_generation( scene ) &&
           r->scene_generation == generation && r->candidate_count == scene->object_count &&
           r->scene_mask == scene->visibility_mask;
}

wp_s32 wp_graphics_cull_result_validate_object( const wp_graphics_cull_result *r,
                                                const wp_graphics_scene *scene, wp_s32 index )
{
    wp_s32 candidate;
    if( !cull_membership_valid( r, scene, r ? r->scene_generation : 0 ) || index < 0 ||
        index >= r->visible_count )
    {
        return WP_GRAPHICS_CULL_ERROR_STALE;
    }
    candidate = r->indices[index];
    if( scene->objects[candidate] != r->entries[candidate].object ||
        !cull_entry_valid( &r->entries[candidate] ) )
    {
        return WP_GRAPHICS_CULL_ERROR_STALE;
    }
    return WP_GRAPHICS_CULL_OK;
}

wp_s32 wp_graphics_cull_result_validate( const wp_graphics_cull_result *r,
                                         const wp_graphics_scene *scene, wp_u32 generation )
{
    wp_s32 i;
    wp_mat4f view, projection;
    if( !r || !scene )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_ARGUMENT;
    }
    if( !cull_membership_valid( r, scene, generation ) )
    {
        return WP_GRAPHICS_CULL_ERROR_STALE;
    }
    if( r->camera )
    {
        if( wp_camera_get_visibility_mask( r->camera ) != r->camera_mask )
        {
            return WP_GRAPHICS_CULL_ERROR_STALE;
        }
        wp_camera_get_view_matrix( r->camera, &view );
        wp_camera_get_projection_matrix( r->camera, &projection );
        if( memcmp( &view, &r->view, sizeof( view ) ) ||
            memcmp( &projection, &r->projection, sizeof( projection ) ) )
        {
            return WP_GRAPHICS_CULL_ERROR_STALE;
        }
    }
    for( i = 0; i < r->candidate_count; ++i )
    {
        if( scene->objects[i] != r->entries[i].object || !cull_entry_valid( &r->entries[i] ) )
        {
            return WP_GRAPHICS_CULL_ERROR_STALE;
        }
    }
    return WP_GRAPHICS_CULL_OK;
}

void wp_graphics_cull_result_get_transform_stats( const wp_graphics_cull_result *result,
                                                  wp_transform_job_stats *out )
{
    if( !out )
    {
        return;
    }
    if( result )
    {
        *out = result->transform_stats;
    }
    else
    {
        memset( out, 0, sizeof( *out ) );
    }
}

wp_u32 wp_graphics_cull_result_get_transform_dispatch_count( const wp_graphics_cull_result *result )
{
    return result ? result->transform_dispatches : 0;
}

wp_graphics_cull_result *wp_graphics_cull_result_create_view( const wp_graphics_cull_result *source,
                                                              const wp_camera *camera,
                                                              wp_graphics_cull_pass pass )
{
    wp_graphics_cull_job view;
    wp_s32 i;
    if( !source || !source->snapshot || source->snapshot->references == (wp_u32)-1 ||
        ( pass != WP_GRAPHICS_CULL_PASS_MAIN && pass != WP_GRAPHICS_CULL_PASS_SHADOW ) )
    {
        return NULL;
    }
    /* The new view captures its own camera; only geometry/membership from the
     * publication must still be current. Never dereference retired candidates. */
    if( !cull_membership_valid( source, source->scene, source->scene_generation ) )
    {
        return NULL;
    }
    for( i = 0; i < source->candidate_count; ++i )
    {
        if( source->scene->objects[i] != source->entries[i].object ||
            !cull_entry_valid( &source->entries[i] ) )
        {
            return NULL;
        }
    }
    memset( &view, 0, sizeof( view ) );
    view.entries = source->entries;
    view.groups = source->snapshot->groups;
    view.entry_groups = source->snapshot->entry_groups;
    view.group_count = source->snapshot->group_count;
    view.entry_count = source->candidate_count;
    view.shared_snapshot = source->snapshot;
    view.transform_stats = source->transform_stats;
    view.sphere_test = source->sphere_test;
    view.sat_test = source->sat_test;
    view.group_test = source->group_test;
    view.spatial_test = source->spatial_test;
    cull_capture_view( &view, source->scene, camera, pass, source->scene_generation );
    view.status = WP_GRAPHICS_CULL_STATUS_READY;
    /* Execute uses only captured entries, allocates independent indices and
     * retains the publication on success. Failure leaves source ownership intact. */
    if( wp_graphics_cull_job_execute( &view ) != WP_GRAPHICS_CULL_OK )
    {
        return NULL;
    }
    return wp_graphics_cull_job_take_result( &view );
}

wp_s32 wp_graphics_cull_job_set_sat_test( wp_graphics_cull_job *job, wp_s32 enabled )
{
    if( !job )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_ARGUMENT;
    }
    if( job->status == WP_GRAPHICS_CULL_STATUS_READY )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_STATE;
    }
    job->sat_test = enabled != 0;
    return WP_GRAPHICS_CULL_OK;
}

wp_s32 wp_graphics_cull_job_set_sphere_test( wp_graphics_cull_job *job, wp_s32 enabled )
{
    if( !job )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_ARGUMENT;
    }
    if( job->status == WP_GRAPHICS_CULL_STATUS_READY )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_STATE;
    }
    job->sphere_test = enabled != 0;
    return WP_GRAPHICS_CULL_OK;
}

void wp_graphics_cull_result_get_bounds_stats( const wp_graphics_cull_result *result,
                                               wp_graphics_cull_bounds_stats *out )
{
    if( !out )
    {
        return;
    }
    if( result )
    {
        *out = result->bounds_stats;
    }
    else
    {
        memset( out, 0, sizeof( *out ) );
    }
}

void wp_graphics_cull_result_get_sort_stats( const wp_graphics_cull_result *result,
                                             wp_graphics_cull_sort_stats *out )
{
    if( out )
    {
        if( result )
        {
            *out = result->sort_stats;
        }
        else
        {
            memset( out, 0, sizeof( *out ) );
        }
    }
}

wp_s32 wp_graphics_cull_job_set_group_test( wp_graphics_cull_job *job, wp_s32 enabled )
{
    if( !job )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_ARGUMENT;
    }
    if( job->status == WP_GRAPHICS_CULL_STATUS_READY )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_STATE;
    }
    job->group_test = enabled != 0;
    return WP_GRAPHICS_CULL_OK;
}

wp_s32 wp_graphics_cull_job_set_spatial_test( wp_graphics_cull_job *job, wp_s32 enabled )
{
    if( !job )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_ARGUMENT;
    }
    if( job->status == WP_GRAPHICS_CULL_STATUS_READY )
    {
        return WP_GRAPHICS_CULL_ERROR_INVALID_STATE;
    }
    job->spatial_test = enabled != 0;
    return WP_GRAPHICS_CULL_OK;
}
