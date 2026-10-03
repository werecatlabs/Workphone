/**
 * @file wp_graphics_animation.h
 * @brief C API for skeletal/node animation (keyframes, tracks, interpolation).
 *
 * This module provides the structures and functions needed to drive
 * per-node animation in the renderer.  The design follows the
 * Ogre3D Animation / AnimationTrack / KeyFrame hierarchy but is
 * expressed in plain C89.
 *
 * An #wp_animation owns two kinds of track collection:
 *   - Node tracks    (wp_node_animation_track)  - animate a wp_node transform
 *   - Numeric tracks (wp_numeric_animation_track) - animate a single wp_f32 value
 *
 * Each track stores an ordered list of keyframes.  At evaluation time
 * the animation is sampled at a given time position and the two
 * surrounding keyframes are linearly (or spherically for rotation)
 * interpolated.
 */

#ifndef WORKPHONE_GRAPHICS_ANIMATION_H
#define WORKPHONE_GRAPHICS_ANIMATION_H

#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

/* wp_node is forward-declared here to avoid the wp_quatf redefinition that
 * occurs when workphone_graphics_node.h (which defines its own wp_quatf) is
 * included after workphone_quat.h.  The implementation file includes
 * workphone_graphics_node.h directly after this header. */
typedef struct wp_node wp_node;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Capacity limits
 * ---------------------------------------------------------------------- */

#define WP_ANIMATION_MAX_NODE_TRACKS 64     ///< Maximum node animation tracks per animation.
#define WP_ANIMATION_MAX_NUMERIC_TRACKS 32  ///< Maximum numeric animation tracks per animation.
#define WP_ANIMATION_MAX_KEYFRAMES 256      ///< Maximum keyframes per track.
#define WP_ANIMATION_NAME_MAX 64            ///< Maximum animation name length including null terminator.

/* -------------------------------------------------------------------------
 * Enumerations
 * ---------------------------------------------------------------------- */

/**
 * @brief Keyframe interpolation mode for translation and scale channels.
 */
typedef enum
{
    WP_ANIM_INTERP_LINEAR = 0,  ///< Linear interpolation between keyframes.
    WP_ANIM_INTERP_SPLINE       ///< Catmull-Rom spline interpolation (reserved; falls back to linear).
} wp_anim_interp_mode;

/**
 * @brief Interpolation mode used for the rotation channel.
 */
typedef enum
{
    WP_ANIM_ROT_INTERP_LINEAR = 0,  ///< Normalised linear interpolation (nlerp).
    WP_ANIM_ROT_INTERP_SPHERICAL    ///< Spherical linear interpolation (slerp).
} wp_anim_rot_interp_mode;

/* -------------------------------------------------------------------------
 * Keyframe types
 * ---------------------------------------------------------------------- */

/**
 * @brief A single keyframe for a node (transform) animation track.
 *
 * Stores a sample of the full local transform of a node at a given time.
 */
typedef struct
{
    wp_f32 time;           ///< Time position of this keyframe in seconds.
    wp_vec3f translation;  ///< Local translation at this keyframe.
    wp_quatf rotation;     ///< Local rotation (unit quaternion) at this keyframe.
    wp_vec3f scale;        ///< Local scale at this keyframe.
} wp_transform_keyframe;

/**
 * @brief A single keyframe for a numeric animation track.
 *
 * Stores a scalar value at a given time.
 */
typedef struct
{
    wp_f32 time;   ///< Time position of this keyframe in seconds.
    wp_f32 value;  ///< Scalar value at this keyframe.
} wp_numeric_keyframe;

/* -------------------------------------------------------------------------
 * Track types
 * ---------------------------------------------------------------------- */

/**
 * @brief An animation track that drives the transform of a single wp_node.
 *
 * Keyframes are kept in ascending time order.  At evaluation time the two
 * surrounding keyframes are found and the transform is interpolated.
 */
typedef struct
{
    unsigned short handle;  ///< Unique track handle within its parent animation.
    wp_node *target_node;   ///< Node whose transform is driven by this track.
    wp_transform_keyframe keyframes[WP_ANIMATION_MAX_KEYFRAMES];  ///< Ordered keyframe array.
    unsigned int keyframe_count;     ///< Number of keyframes currently stored.
    int use_shortest_rotation_path;  ///< Non-zero to flip quaternion sign for shortest arc.
} wp_node_animation_track;

/**
 * @brief An animation track that drives a single wp_f32 value.
 *
 * Keyframes are kept in ascending time order.
 */
typedef struct
{
    unsigned short handle;  ///< Unique track handle within its parent animation.
    wp_f32 *target_value;   ///< Pointer to the scalar value driven by this track.
    wp_numeric_keyframe keyframes[WP_ANIMATION_MAX_KEYFRAMES];  ///< Ordered keyframe array.
    unsigned int keyframe_count;  ///< Number of keyframes currently stored.
} wp_numeric_animation_track;

/* -------------------------------------------------------------------------
 * Animation container
 * ---------------------------------------------------------------------- */

/**
 * @brief Top-level animation object that owns a set of tracks.
 *
 * Mirrors Ogre3D's Animation class.  Tracks are stored in flat arrays with
 * a maximum capacity defined by WP_ANIMATION_MAX_NODE_TRACKS and
 * WP_ANIMATION_MAX_NUMERIC_TRACKS.
 */
typedef struct
{
    char name[WP_ANIMATION_NAME_MAX];         ///< Null-terminated animation name.
    wp_f32 length;                            ///< Total duration of the animation in seconds.
    wp_anim_interp_mode interp_mode;          ///< Interpolation mode for translation/scale.
    wp_anim_rot_interp_mode rot_interp_mode;  ///< Interpolation mode for rotation.
    wp_node_animation_track node_tracks[WP_ANIMATION_MAX_NODE_TRACKS];  ///< Node track storage.
    unsigned int node_track_count;  ///< Number of active node tracks.
    wp_numeric_animation_track
        numeric_tracks[WP_ANIMATION_MAX_NUMERIC_TRACKS];  ///< Numeric track storage.
    unsigned int numeric_track_count;                     ///< Number of active numeric tracks.
} wp_animation;

/* -------------------------------------------------------------------------
 * Animation lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Allocates and initialises a new animation.
 *
 * @param name   Null-terminated name string (at most WP_ANIMATION_NAME_MAX-1 chars).
 * @param length Total duration in seconds.
 * @return Pointer to the new animation, or NULL on allocation failure.
 */
wp_animation *wp_animation_create( const char *name, wp_f32 length );

/**
 * @brief Destroys an animation and releases its memory.
 * @param anim Animation to destroy. Ignored if NULL.
 */
void wp_animation_destroy( wp_animation *anim );

/* -------------------------------------------------------------------------
 * Animation properties
 * ---------------------------------------------------------------------- */

/**
 * @brief Returns the total length of the animation in seconds.
 * @param anim Pointer to the animation.
 * @return Length in seconds, or 0 if anim is NULL.
 */
wp_f32 wp_animation_get_length( const wp_animation *anim );

/**
 * @brief Sets the total length of the animation.
 * @param anim   Pointer to the animation.
 * @param length New length in seconds.
 */
void wp_animation_set_length( wp_animation *anim, wp_f32 length );

/**
 * @brief Returns the null-terminated name of the animation.
 * @param anim Pointer to the animation.
 * @return Pointer to the name string, or NULL if anim is NULL.
 */
const char *wp_animation_get_name( const wp_animation *anim );

/**
 * @brief Sets the interpolation mode for translation and scale channels.
 * @param anim Pointer to the animation.
 * @param mode Desired interpolation mode.
 */
void wp_animation_set_interp_mode( wp_animation *anim, wp_anim_interp_mode mode );

/**
 * @brief Sets the interpolation mode for the rotation channel.
 * @param anim Pointer to the animation.
 * @param mode Desired rotation interpolation mode.
 */
void wp_animation_set_rot_interp_mode( wp_animation *anim, wp_anim_rot_interp_mode mode );

/* -------------------------------------------------------------------------
 * Node track management
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a new node animation track within an animation.
 *
 * The handle must be unique within the animation.  The track is
 * initialised with no keyframes.
 *
 * @param anim        Owning animation.
 * @param handle      Unique handle for this track.
 * @param target_node Node driven by the track, or NULL to leave unbound.
 * @return Pointer to the new track, or NULL on failure (animation full or anim is NULL).
 */
wp_node_animation_track *wp_animation_create_node_track( wp_animation *anim, unsigned short handle,
                                                         wp_node *target_node );

/**
 * @brief Retrieves a node track by its handle.
 * @param anim   Owning animation.
 * @param handle Track handle to look up.
 * @return Pointer to the track, or NULL if not found.
 */
wp_node_animation_track *wp_animation_get_node_track( const wp_animation *anim, unsigned short handle );

/**
 * @brief Removes and clears a node track by handle.
 * @param anim   Owning animation.
 * @param handle Handle of the track to remove.
 */
void wp_animation_destroy_node_track( wp_animation *anim, unsigned short handle );

/* -------------------------------------------------------------------------
 * Node track keyframe management
 * ---------------------------------------------------------------------- */

/**
 * @brief Adds a new transform keyframe to a node track.
 *
 * The keyframe is inserted in ascending time order.  The new keyframe is
 * initialised with a zero translation, identity rotation, and unit scale.
 *
 * @param track Owning node track.
 * @param time  Time position of the new keyframe in seconds.
 * @return Pointer to the new keyframe, or NULL if the track is full or track is NULL.
 */
wp_transform_keyframe *wp_node_track_create_keyframe( wp_node_animation_track *track, wp_f32 time );

/**
 * @brief Retrieves a transform keyframe by index.
 * @param track Owning node track.
 * @param index Zero-based keyframe index.
 * @return Pointer to the keyframe, or NULL if out of range.
 */
wp_transform_keyframe *wp_node_track_get_keyframe( const wp_node_animation_track *track,
                                                   unsigned int index );

/**
 * @brief Finds the two keyframes that bracket a given time and returns the blend factor.
 *
 * On return, *out_k0 and *out_k1 are the indices of the keyframes on either
 * side of @p time, and *out_t is the normalised blend factor in [0, 1].
 * If the track has fewer than two keyframes the behaviour is implementation-
 * defined (both indices will be set to 0 and *out_t to 0).
 *
 * @param track  Source track (read-only).
 * @param time   Sample time in seconds.
 * @param out_k0 Receives the index of the keyframe at or before @p time.
 * @param out_k1 Receives the index of the keyframe at or after @p time.
 * @param out_t  Receives the blend factor between the two keyframes.
 */
void wp_node_track_get_keyframes_at_time( const wp_node_animation_track *track, wp_f32 time,
                                          unsigned int *out_k0, unsigned int *out_k1, wp_f32 *out_t );

/**
 * @brief Samples the interpolated transform of a node track at a given time.
 *
 * The returned keyframe represents the blended transform.  The interpolation
 * modes are taken directly from the supplied arguments so that the track can
 * be driven by the parent animation's global settings.
 *
 * @param track           Source track (read-only).
 * @param time            Sample time in seconds.
 * @param interp_mode     Interpolation mode for translation and scale.
 * @param rot_interp_mode Interpolation mode for rotation.
 * @return Interpolated transform keyframe.
 */
wp_transform_keyframe wp_node_track_interpolate( const wp_node_animation_track *track, wp_f32 time,
                                                 wp_anim_interp_mode interp_mode,
                                                 wp_anim_rot_interp_mode rot_interp_mode );

/* -------------------------------------------------------------------------
 * Numeric track management
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a new numeric animation track within an animation.
 * @param anim         Owning animation.
 * @param handle       Unique handle for this track.
 * @param target_value Pointer to the wp_f32 value driven by this track, or NULL.
 * @return Pointer to the new track, or NULL on failure.
 */
wp_numeric_animation_track *wp_animation_create_numeric_track( wp_animation *anim, unsigned short handle,
                                                               wp_f32 *target_value );

/**
 * @brief Retrieves a numeric track by its handle.
 * @param anim   Owning animation.
 * @param handle Track handle to look up.
 * @return Pointer to the track, or NULL if not found.
 */
wp_numeric_animation_track *wp_animation_get_numeric_track( const wp_animation *anim,
                                                            unsigned short handle );

/**
 * @brief Removes and clears a numeric track by handle.
 * @param anim   Owning animation.
 * @param handle Handle of the track to remove.
 */
void wp_animation_destroy_numeric_track( wp_animation *anim, unsigned short handle );

/* -------------------------------------------------------------------------
 * Numeric track keyframe management
 * ---------------------------------------------------------------------- */

/**
 * @brief Adds a new numeric keyframe to a numeric track.
 *
 * The keyframe is inserted in ascending time order and initialised to 0.
 *
 * @param track Owning numeric track.
 * @param time  Time position in seconds.
 * @return Pointer to the new keyframe, or NULL if the track is full or NULL.
 */
wp_numeric_keyframe *wp_numeric_track_create_keyframe( wp_numeric_animation_track *track, wp_f32 time );

/**
 * @brief Retrieves a numeric keyframe by index.
 * @param track Owning numeric track.
 * @param index Zero-based keyframe index.
 * @return Pointer to the keyframe, or NULL if out of range.
 */
wp_numeric_keyframe *wp_numeric_track_get_keyframe( const wp_numeric_animation_track *track,
                                                    unsigned int index );

/* -------------------------------------------------------------------------
 * Apply / evaluate
 * ---------------------------------------------------------------------- */

/**
 * @brief Applies all tracks of an animation at a given time position.
 *
 * Each node track writes the interpolated transform into its target node.
 * Each numeric track writes the interpolated value into its target pointer.
 * If the target pointer/node of a track is NULL the track is skipped.
 *
 * @param anim     Animation to apply.
 * @param time_pos Sample time in seconds (clamped to [0, length]).
 * @param weight   Blend weight in [0, 1] (1 = full override, 0 = no effect).
 */
void wp_animation_apply( const wp_animation *anim, wp_f32 time_pos, wp_f32 weight );

/**
 * @brief Applies a single node track at a given time position.
 *
 * Writes the interpolated transform to track->target_node using
 * wp_node_set_position, wp_node_set_orientation, and wp_node_set_scale.
 *
 * @param track                   Track to apply.
 * @param time_pos                Sample time in seconds.
 * @param weight                  Blend weight in [0, 1].
 * @param interp_mode             Translation/scale interpolation mode.
 * @param rot_interp_mode         Rotation interpolation mode.
 */
void wp_node_track_apply( const wp_node_animation_track *track, wp_f32 time_pos, wp_f32 weight,
                          wp_anim_interp_mode interp_mode, wp_anim_rot_interp_mode rot_interp_mode );

/**
 * @brief Applies a single numeric track at a given time position.
 *
 * Writes the interpolated scalar into *track->target_value.
 *
 * @param track    Track to apply.
 * @param time_pos Sample time in seconds.
 * @param weight   Blend weight in [0, 1].
 */
void wp_numeric_track_apply( const wp_numeric_animation_track *track, wp_f32 time_pos, wp_f32 weight );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_ANIMATION_H */
