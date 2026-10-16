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
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

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
    wp_mat4f world;
} cull_entry;

struct wp_graphics_cull_result
{
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
};

struct wp_graphics_cull_job
{
    wp_graphics_cull_status status;
    cull_entry *entries;
    wp_s32 entry_count;
    wp_u32 visibility_mask;
    wp_s32 has_camera;
    const wp_camera *camera;
    wp_u32 camera_mask, scene_mask;
    wp_mat4f view_projection, view, projection;
    const wp_graphics_scene *scene;
    wp_u64 native_generation;
    wp_transform_job_system *transforms;
    wp_graphics_cull_pass pass;
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
    free( job->entries );
    job->entries = NULL;
    job->entry_count = 0;
}

static void cull_job_release_result( wp_graphics_cull_job *job )
{
    wp_graphics_cull_result_destroy( job->result );
    job->result = NULL;
}

static wp_s32 cull_entry_outside_frustum( const cull_entry *entry, const wp_mat4f *view_projection )
{
    wp_mat4f clip;
    wp_s32 axis;
    wp_s32 sign;
    wp_f32 a, b, c, d;
    wp_f32 x, y, z;
    wp_f32 distance;
    wp_f32 tolerance;

    if( !entry->has_bounds )
    {
        return 0;
    }

    wp_mat4f_mul( &clip, view_projection, &entry->world );
    /* Invalid transforms/planes must never make a partially valid frustum reject. */
    for( axis = 0; axis < 16; ++axis )
    {
        if( !( ( (const wp_f32 *)&clip )[axis] <= FLT_MAX &&
               ( (const wp_f32 *)&clip )[axis] >= -FLT_MAX ) )
        {
            return 0;
        }
    }
    for( axis = 0; axis < 3; ++axis )
    {
        for( sign = -1; sign <= 1; sign += 2 )
        {
            a = clip.m[3][0] + sign * clip.m[axis][0];
            b = clip.m[3][1] + sign * clip.m[axis][1];
            c = clip.m[3][2] + sign * clip.m[axis][2];
            if( fabs( a ) + fabs( b ) + fabs( c ) < 1.0e-20 )
            {
                return 0;
            }
        }
    }
    for( axis = 0; axis < 3; ++axis )
    {
        for( sign = -1; sign <= 1; sign += 2 )
        {
            a = clip.m[3][0] + sign * clip.m[axis][0];
            b = clip.m[3][1] + sign * clip.m[axis][1];
            c = clip.m[3][2] + sign * clip.m[axis][2];
            d = clip.m[3][3] + sign * clip.m[axis][3];
            if( fabs( a ) + fabs( b ) + fabs( c ) < 1.0e-20 )
            {
                return 0;
            }
            x = a >= 0 ? entry->bounds.max.x : entry->bounds.min.x;
            y = b >= 0 ? entry->bounds.max.y : entry->bounds.min.y;
            z = c >= 0 ? entry->bounds.max.z : entry->bounds.min.z;
            distance = a * x + b * y + c * z + d;
            tolerance =
                (wp_f32)( 1.0e-5 * ( fabs( a * x ) + fabs( b * y ) + fabs( c * z ) + fabs( d ) + 1.0 ) );
            if( distance < -tolerance )
            {
                return 1;
            }
        }
    }
    return 0;
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
                                  wp_s32 count )
{
    wp_s32 *destination;
    wp_s32 *swap;
    wp_s32 width;
    wp_s32 left, middle, right;
    wp_s32 li, ri, out;

    destination = scratch;
    for( width = 1; width < count; )
    {
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

wp_s32 wp_graphics_cull_job_configure( wp_graphics_cull_job *job, const wp_graphics_scene *scene,
                                       const wp_camera *camera, wp_graphics_cull_pass pass,
                                       wp_u32 scene_generation )
{
    wp_camera *view_camera;
    wp_mat4f view_matrix;
    wp_mat4f projection_matrix;
    wp_u32 mask;
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
        worlds = wp_transform_job_system_get_result( job->transforms, NULL, NULL );
        for( i = 0; i < job->entry_count; ++i )
        {
            job->entries[i].world = worlds[i];
        }
    }

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
    job->status = WP_GRAPHICS_CULL_STATUS_READY;
    return WP_GRAPHICS_CULL_OK;
}

wp_s32 wp_graphics_cull_job_execute( wp_graphics_cull_job *job )
{
    wp_graphics_cull_result *result;
    wp_s32 *indices;
    wp_s32 *scratch;
    wp_s32 *sorted;
    wp_s32 count;
    wp_s32 i;
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
    result = (wp_graphics_cull_result *)malloc( sizeof( *result ) );
    indices = (wp_s32 *)malloc( alloc_count * sizeof( wp_s32 ) );
    scratch = (wp_s32 *)malloc( alloc_count * sizeof( wp_s32 ) );
    if( !result || !indices || !scratch )
    {
        free( result );
        free( indices );
        free( scratch );
        job->status = WP_GRAPHICS_CULL_STATUS_FAILED;
        return WP_GRAPHICS_CULL_ERROR_OUT_OF_MEMORY;
    }
    memset( result, 0, sizeof( *result ) );

    count = 0;
    for( i = 0; i < job->entry_count; ++i )
    {
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
        else if( job->has_camera && cull_entry_outside_frustum( entry, &job->view_projection ) )
        {
            continue;
        }
        if( !entry->has_render_func )
        {
            continue;
        }
        indices[count++] = i;
    }

    sorted = cull_sort_indices( job->entries, indices, scratch, count );

    result->entries = job->entries;
    job->entries = NULL;
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
    for( i = 0; i < result->candidate_count; ++i )
    {
        free( result->entries[i].ancestors );
    }
    free( result->entries );
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
