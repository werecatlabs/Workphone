/**
 * @file workphone_game_util.h
 * @brief C API for common game utility helpers.
 *
 * Provides stateless helpers that work across actors, scenes, components
 * and transforms without belonging to any one of those sub-systems.
 */

#ifndef WORKPHONE_GAME_UTIL_H
#define WORKPHONE_GAME_UTIL_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_quat.h"
#include "workphone_game_actor.h"
#include "workphone_game_scene.h"
#include "workphone_game_component.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Actor — component lookup
 * ====================================================================== */

/**
 * @brief Returns the first component of the given type on an actor, or NULL.
 */
wp_game_component *wp_game_util_actor_find_component( const wp_game_actor *actor,
                                                      enum wp_component_type type );

/**
 * @brief Collects all components of the given type into @p out_buf.
 *
 * @param actor    Source actor.
 * @param type     Component type to search for.
 * @param out_buf  Caller-supplied array of pointers to fill.
 * @param buf_cap  Capacity of @p out_buf.
 * @return         Number of components written (may be less than the total
 *                 if @p buf_cap was exceeded).
 */
wp_u32 wp_game_util_actor_collect_components( const wp_game_actor *actor, enum wp_component_type type,
                                              wp_game_component **out_buf, wp_u32 buf_cap );

/* =========================================================================
 * Actor — spatial helpers
 * ====================================================================== */

/**
 * @brief World-space squared distance between two actors.
 */
wp_f32 wp_game_util_actor_distance_sq( const wp_game_actor *a, const wp_game_actor *b );

/**
 * @brief World-space distance between two actors.
 */
wp_f32 wp_game_util_actor_distance( const wp_game_actor *a, const wp_game_actor *b );

/* =========================================================================
 * Scene — actor search
 * ====================================================================== */

/**
 * @brief Returns the first actor in the scene that carries the given tag,
 *        or NULL if none is found.
 */
wp_game_actor *wp_game_util_scene_find_actor_by_tag( const wp_game_scene *scene, const wp_c8 *tag );

/**
 * @brief Collects all actors in the scene that carry the given tag into
 *        @p out_buf.
 *
 * @return Number of actors written.
 */
wp_u32 wp_game_util_scene_collect_actors_by_tag( const wp_game_scene *scene, const wp_c8 *tag,
                                                 wp_game_actor **out_buf, wp_u32 buf_cap );

/**
 * @brief Returns the first actor in the scene that has a component of the
 *        given type, or NULL.
 */
wp_game_actor *wp_game_util_scene_find_actor_by_component( const wp_game_scene *scene,
                                                           enum wp_component_type type );

/**
 * @brief Returns the count of actors in the scene that carry the given tag.
 */
wp_u32 wp_game_util_scene_count_actors_by_tag( const wp_game_scene *scene, const wp_c8 *tag );

/* =========================================================================
 * Transform composition
 * ====================================================================== */

/**
 * @brief Composes a parent and local transform into a single world transform.
 *
 * Equivalent to: world = parent * local
 *   - scale       : component-wise product
 *   - orientation : parent_orient * local_orient (normalised)
 *   - position    : parent_pos + parent_orient * (parent_scale * local_pos)
 */
wp_transform3f wp_game_util_transform_mul( const wp_transform3f *parent, const wp_transform3f *local );

/**
 * @brief Transforms a powp_s32 from local space into world space.
 */
wp_vec3f wp_game_util_transform_point( const wp_transform3f *t, wp_vec3f local_point );

/**
 * @brief Transforms a direction vector from local space into world space.
 *
 * Translation and scale are ignored; only the orientation is applied.
 */
wp_vec3f wp_game_util_transform_direction( const wp_transform3f *t, wp_vec3f local_dir );

/**
 * @brief Transforms a powp_s32 from world space back into local space.
 */
wp_vec3f wp_game_util_inverse_transform_point( const wp_transform3f *t, wp_vec3f world_point );

/**
 * @brief Transforms a direction from world space back into local space.
 */
wp_vec3f wp_game_util_inverse_transform_direction( const wp_transform3f *t, wp_vec3f world_dir );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_UTIL_H */
