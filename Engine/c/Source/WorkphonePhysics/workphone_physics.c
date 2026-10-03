/**
 * @file wp_physics.c
 * @brief Implementation of the C physics system (manager) API.
 */

#include "workphone_physics.h"
#include "workphone_physics_scene.h"
#include "workphone_physics_collision.h"
#include "workphone_physics_material.h"
#include "workphone_physics_material_registry.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* =========================================================================
 * Internal structures
 * ====================================================================== */

#ifndef WP_PHYSICS_MAX_SCENES
#    define WP_PHYSICS_MAX_SCENES 8
#endif


typedef struct wp_physics_system
{
    wp_vec3f gravity;
    wp_s32 debug_draw;

    wp_physics_scene *scenes[WP_PHYSICS_MAX_SCENES];
    wp_s32 scene_count;

    void *native;
    void *user_data;
    wp_physics_material_registry *material_registry;
} wp_physics_system;

static wp_vec3f vec3f_zero( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

/* =========================================================================
 * System lifecycle
 * ====================================================================== */

wp_physics_system *wp_physics_system_create( void )
{
    wp_physics_system *sys = (wp_physics_system *)malloc( sizeof( wp_physics_system ) );
    if( !sys )
    {
        return NULL;
    }

    memset( sys, 0, sizeof( wp_physics_system ) );

    /* Default gravity: Earth-like, Y-down */
    sys->gravity.y = -9.81f;

    /* Data-driven material registry (owns the named material handles). */
    sys->material_registry = wp_physics_material_registry_create();

    return sys;
}

void wp_physics_system_destroy( wp_physics_system *sys )
{
    wp_s32 i;

    if( !sys )
    {
        return;
    }

    for( i = 0; i < sys->scene_count; ++i )
    {
        wp_physics_scene_destroy( sys->scenes[i] );
    }

    wp_physics_material_registry_destroy( sys->material_registry );
    sys->material_registry = NULL;

    free( sys );
}

/* =========================================================================
 * Scene management
 * ====================================================================== */

wp_physics_scene *wp_physics_system_add_scene( wp_physics_system *sys )
{
    wp_physics_scene *scene;

    if( !sys || sys->scene_count >= WP_PHYSICS_MAX_SCENES )
    {
        return NULL;
    }

    scene = wp_physics_scene_create();
    if( !scene )
    {
        return NULL;
    }

    wp_physics_scene_set_gravity( scene, sys->gravity );
    sys->scenes[sys->scene_count++] = scene;

    return scene;
}

void wp_physics_system_remove_scene( wp_physics_system *sys, wp_physics_scene *scene )
{
    wp_s32 i;

    if( !sys || !scene )
    {
        return;
    }

    for( i = 0; i < sys->scene_count; ++i )
    {
        if( sys->scenes[i] == scene )
        {
            wp_physics_scene_destroy( scene );
            sys->scenes[i] = sys->scenes[sys->scene_count - 1];
            sys->scenes[sys->scene_count - 1] = NULL;
            --sys->scene_count;
            return;
        }
    }
}

wp_physics_scene *wp_physics_system_get_scene( const wp_physics_system *sys, wp_s32 index )
{
    if( !sys || index < 0 || index >= sys->scene_count )
    {
        return NULL;
    }

    return sys->scenes[index];
}

wp_s32 wp_physics_system_get_scene_count( const wp_physics_system *sys )
{
    if( !sys )
    {
        return 0;
    }
    return sys->scene_count;
}

/* =========================================================================
 * Gravity
 * ====================================================================== */

wp_vec3f wp_physics_system_get_gravity( const wp_physics_system *sys )
{
    if( !sys )
    {
        return vec3f_zero();
    }
    return sys->gravity;
}

void wp_physics_system_set_gravity( wp_physics_system *sys, wp_vec3f gravity )
{
    if( !sys )
    {
        return;
    }
    sys->gravity = gravity;
    {
        wp_s32 i;
        for( i = 0; i < sys->scene_count; ++i )
        {
            wp_physics_scene_set_gravity( sys->scenes[i], gravity );
        }
    }
}

/* =========================================================================
 * Simulation step
 * ====================================================================== */

void wp_physics_system_step( wp_physics_system *sys, wp_f32 dt )
{
    if( !sys )
    {
        return;
    }

    {
        wp_s32 i;
        for( i = 0; i < sys->scene_count; ++i )
        {
            wp_physics_scene_simulate( sys->scenes[i], dt );
            wp_physics_scene_fetch_results( sys->scenes[i], 1 );
        }
    }
}

/* =========================================================================
 * Debug draw
 * ====================================================================== */

wp_s32 wp_physics_system_get_debug_draw( const wp_physics_system *sys )
{
    if( !sys )
    {
        return 0;
    }
    return sys->debug_draw;
}

void wp_physics_system_set_debug_draw( wp_physics_system *sys, wp_s32 enabled )
{
    if( !sys )
    {
        return;
    }
    sys->debug_draw = enabled;
}

/* =========================================================================
 * Raycasting
 * ====================================================================== */

wp_s32 wp_physics_system_raycast( const wp_physics_system *sys, wp_vec3f origin, wp_vec3f direction,
                                  wp_f32 max_distance, wp_u32 collision_mask, wp_raycast_hit *out_hit )
{
    if( !sys || !out_hit || max_distance < 0.0f )
    {
        return 0;
    }

    {
        wp_s32 hit_any = 0;
        wp_s32 i;
        wp_vec3f end;
        wp_vec3f best_pos;
        wp_vec3f best_normal;
        wp_rigidbody *best_body = NULL;
        wp_collision_shape *best_shape = NULL;
        wp_f32 best_distance = max_distance;
        wp_f32 direction_length =
            sqrtf( direction.x * direction.x + direction.y * direction.y + direction.z * direction.z );

        wp_raycast_hit_set_body( out_hit, NULL );
        wp_raycast_hit_set_shape( out_hit, NULL );
        wp_raycast_hit_set_distance( out_hit, 0.0f );
        if( direction_length <= 1.0e-8f || max_distance == 0.0f )
        {
            return 0;
        }

        direction.x /= direction_length;
        direction.y /= direction_length;
        direction.z /= direction_length;

        end.x = origin.x + direction.x * max_distance;
        end.y = origin.y + direction.y * max_distance;
        end.z = origin.z + direction.z * max_distance;

        for( i = 0; i < sys->scene_count; ++i )
        {
            wp_vec3f hit_pos;
            wp_vec3f hit_normal;
            wp_rigidbody *hit_body = NULL;
            wp_collision_shape *hit_shape = NULL;
            if( wp_physics_scene_intersects_ex( sys->scenes[i], origin, end, &hit_pos, &hit_normal,
                                                &hit_body, &hit_shape, 0, collision_mask ) )
            {
                wp_vec3f delta;
                wp_f32 dist_sq;
                delta.x = hit_pos.x - origin.x;
                delta.y = hit_pos.y - origin.y;
                delta.z = hit_pos.z - origin.z;
                dist_sq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                if( !hit_any || dist_sq < best_distance * best_distance )
                {
                    hit_any = 1;
                    best_pos = hit_pos;
                    best_normal = hit_normal;
                    best_body = hit_body;
                    best_shape = hit_shape;
                    best_distance = sqrtf( dist_sq );
                }
            }
        }

        if( hit_any )
        {
            wp_raycast_hit_set_point( out_hit, best_pos );
            wp_raycast_hit_set_normal( out_hit, best_normal );
            wp_raycast_hit_set_distance( out_hit, best_distance );
            wp_raycast_hit_set_collision_mask( out_hit, collision_mask );
            wp_raycast_hit_set_body( out_hit, best_body );
            wp_raycast_hit_set_shape( out_hit, best_shape );
            wp_raycast_hit_set_flags( out_hit, WORKPHONE_RAYCAST_FLAG_ALL );
        }

        return hit_any;
    }
}

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_physics_system_get_native( const wp_physics_system *sys )
{
    if( !sys )
    {
        return NULL;
    }
    return sys->native;
}

void wp_physics_system_set_native( wp_physics_system *sys, void *native )
{
    if( !sys )
    {
        return;
    }
    sys->native = native;
}

void *wp_physics_system_get_user_data( const wp_physics_system *sys )
{
    if( !sys )
    {
        return NULL;
    }
    return sys->user_data;
}

void wp_physics_system_set_user_data( wp_physics_system *sys, void *user_data )
{
    if( !sys )
    {
        return;
    }
    sys->user_data = user_data;
}

/* =========================================================================
 * Data-driven material registry accessors
 * ====================================================================== */

wp_s32 wp_physics_system_register_material( wp_physics_system *sys, const wp_physics_material_desc *desc )
{
    if( !sys || !sys->material_registry || !desc )
    {
        return 0;
    }
    return wp_physics_material_registry_register( sys->material_registry, desc ) != NULL;
}

wp_u32 wp_physics_system_register_material_database( wp_physics_system *sys, const wp_physics_material_database *db )
{
    if( !sys || !sys->material_registry || !db )
    {
        return 0;
    }
    return wp_physics_material_registry_register_database( sys->material_registry, db );
}

wp_s32 wp_physics_system_unregister_material( wp_physics_system *sys, const wp_c8 *name )
{
    if( !sys || !sys->material_registry )
    {
        return 0;
    }
    return wp_physics_material_registry_unregister( sys->material_registry, name );
}

void wp_physics_system_clear_materials( wp_physics_system *sys )
{
    if( !sys || !sys->material_registry )
    {
        return;
    }
    wp_physics_material_registry_clear( sys->material_registry );
}

wp_physics_material *wp_physics_system_get_material_by_name( wp_physics_system *sys, const wp_c8 *name )
{
    if( !sys || !sys->material_registry )
    {
        return NULL;
    }
    return wp_physics_material_registry_get_material( sys->material_registry, name );
}

wp_physics_material *wp_physics_system_get_default_material( wp_physics_system *sys )
{
    if( !sys || !sys->material_registry )
    {
        return NULL;
    }
    return wp_physics_material_registry_get_default( sys->material_registry );
}

wp_physics_material_registry *wp_physics_system_get_material_registry( wp_physics_system *sys )
{
    return sys ? sys->material_registry : NULL;
}

wp_u32 wp_physics_system_get_material_count( wp_physics_system *sys )
{
    if( !sys || !sys->material_registry )
    {
        return 0;
    }
    return wp_physics_material_registry_get_count( sys->material_registry );
}

wp_physics_material *wp_physics_system_get_material_by_index( wp_physics_system *sys, wp_u32 index, wp_c8 *out_name )
{
    if( !sys || !sys->material_registry )
    {
        return NULL;
    }
    return wp_physics_material_registry_get_by_index( sys->material_registry, index, out_name );
}
