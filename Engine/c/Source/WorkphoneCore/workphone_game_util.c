/**
 * @file workphone_game_util.c
 * @brief Implementation of common game utility helpers.
 */

#include "workphone_game_util.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

#include <math.h>
#include <string.h>

/* =========================================================================
 * Actor — component lookup
 * ====================================================================== */

wp_game_component *wp_game_util_actor_find_component( const wp_game_actor *actor,
                                                      enum wp_component_type type )
{
    wp_u32 i;

    if( !actor )
        return NULL;

    for( i = 0; i < actor->num_components; ++i )
    {
        wp_game_component *comp = actor->components[i];
        if( comp && comp->type == type )
            return comp;
    }

    return NULL;
}

wp_u32 wp_game_util_actor_collect_components( const wp_game_actor *actor, enum wp_component_type type,
                                              wp_game_component **out_buf, wp_u32 buf_cap )
{
    wp_u32 i, count;

    if( !actor || !out_buf || buf_cap == 0 )
        return 0;

    count = 0;
    for( i = 0; i < actor->num_components && count < buf_cap; ++i )
    {
        wp_game_component *comp = actor->components[i];
        if( comp && comp->type == type )
            out_buf[count++] = comp;
    }

    return count;
}

/* =========================================================================
 * Actor — spatial helpers
 * ====================================================================== */

wp_f32 wp_game_util_actor_distance_sq( const wp_game_actor *a, const wp_game_actor *b )
{
    if( !a || !b )
        return 0.0f;

    return wp_vec3f_distance_sq( a->world_transform.position, b->world_transform.position );
}

wp_f32 wp_game_util_actor_distance( const wp_game_actor *a, const wp_game_actor *b )
{
    if( !a || !b )
        return 0.0f;

    return wp_vec3f_distance( a->world_transform.position, b->world_transform.position );
}

/* =========================================================================
 * Scene — actor search
 * ====================================================================== */

wp_game_actor *wp_game_util_scene_find_actor_by_tag( const wp_game_scene *scene, const wp_c8 *tag )
{
    wp_u32 i;

    if( !scene || !tag )
        return NULL;

    for( i = 0; i < scene->num_actors; ++i )
    {
        wp_game_actor *actor = scene->actors[i];
        if( actor && wp_game_actor_has_tag( actor, tag ) )
            return actor;
    }

    return NULL;
}

wp_u32 wp_game_util_scene_collect_actors_by_tag( const wp_game_scene *scene, const wp_c8 *tag,
                                                 wp_game_actor **out_buf, wp_u32 buf_cap )
{
    wp_u32 i, count;

    if( !scene || !tag || !out_buf || buf_cap == 0 )
        return 0;

    count = 0;
    for( i = 0; i < scene->num_actors && count < buf_cap; ++i )
    {
        wp_game_actor *actor = scene->actors[i];
        if( actor && wp_game_actor_has_tag( actor, tag ) )
            out_buf[count++] = actor;
    }

    return count;
}

wp_game_actor *wp_game_util_scene_find_actor_by_component( const wp_game_scene *scene,
                                                           enum wp_component_type type )
{
    wp_u32 i;

    if( !scene )
        return NULL;

    for( i = 0; i < scene->num_actors; ++i )
    {
        wp_game_actor *actor = scene->actors[i];
        if( actor && wp_game_util_actor_find_component( actor, type ) )
            return actor;
    }

    return NULL;
}

wp_u32 wp_game_util_scene_count_actors_by_tag( const wp_game_scene *scene, const wp_c8 *tag )
{
    wp_u32 i, count;

    if( !scene || !tag )
        return 0;

    count = 0;
    for( i = 0; i < scene->num_actors; ++i )
    {
        wp_game_actor *actor = scene->actors[i];
        if( actor && wp_game_actor_has_tag( actor, tag ) )
            ++count;
    }

    return count;
}

/* =========================================================================
 * Transform composition
 * ====================================================================== */

wp_transform3f wp_game_util_transform_mul( const wp_transform3f *parent, const wp_transform3f *local )
{
    wp_transform3f result;
    wp_vec3f scaled_pos;

    /* Scale: component-wise product. */
    result.scale.x = parent->scale.x * local->scale.x;
    result.scale.y = parent->scale.y * local->scale.y;
    result.scale.z = parent->scale.z * local->scale.z;

    /* Orientation: parent * local, then normalise. */
    result.orientation = wp_quatf_normalize( wp_quatf_mul( parent->orientation, local->orientation ) );

    /* Position: parent_pos + parent_orient * (parent_scale * local_pos). */
    scaled_pos.x = local->position.x * parent->scale.x;
    scaled_pos.y = local->position.y * parent->scale.y;
    scaled_pos.z = local->position.z * parent->scale.z;

    result.position =
        wp_vec3f_add( parent->position, wp_quatf_rotate_vec3( parent->orientation, scaled_pos ) );

    return result;
}

wp_vec3f wp_game_util_transform_point( const wp_transform3f *t, wp_vec3f local_point )
{
    wp_vec3f scaled;

    if( !t )
        return local_point;

    /* Scale, then rotate, then translate. */
    scaled.x = local_point.x * t->scale.x;
    scaled.y = local_point.y * t->scale.y;
    scaled.z = local_point.z * t->scale.z;

    return wp_vec3f_add( t->position, wp_quatf_rotate_vec3( t->orientation, scaled ) );
}

wp_vec3f wp_game_util_transform_direction( const wp_transform3f *t, wp_vec3f local_dir )
{
    if( !t )
        return local_dir;

    return wp_quatf_rotate_vec3( t->orientation, local_dir );
}

wp_vec3f wp_game_util_inverse_transform_point( const wp_transform3f *t, wp_vec3f world_point )
{
    wp_vec3f delta, unrotated;
    wp_quatf inv;

    if( !t )
        return world_point;

    inv = wp_quatf_conjugate( t->orientation );
    delta = wp_vec3f_sub( world_point, t->position );

    unrotated = wp_quatf_rotate_vec3( inv, delta );

    /* Undo scale (guard against zero scale). */
    unrotated.x = ( t->scale.x != 0.0f ) ? unrotated.x / t->scale.x : 0.0f;
    unrotated.y = ( t->scale.y != 0.0f ) ? unrotated.y / t->scale.y : 0.0f;
    unrotated.z = ( t->scale.z != 0.0f ) ? unrotated.z / t->scale.z : 0.0f;

    return unrotated;
}

wp_vec3f wp_game_util_inverse_transform_direction( const wp_transform3f *t, wp_vec3f world_dir )
{
    if( !t )
        return world_dir;

    return wp_quatf_rotate_vec3( wp_quatf_conjugate( t->orientation ), world_dir );
}
