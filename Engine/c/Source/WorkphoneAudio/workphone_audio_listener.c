/**
 * @file workphone_audio_listener.h
 * @brief C API for an audio listener.
 *
 * A listener represents a powp_s32 in 3D space that receives audio.  It
 * tracks position, velocity and orientation (forward / up vectors).
 * Platform-specific handles are stored via the opaque native pointer.
 */

#ifndef WORKPHONE_AUDIO_LISTENER_H
#define WORKPHONE_AUDIO_LISTENER_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------------- */

#ifndef WP_AUDIO_LISTENER_NAME_MAX
#    define WP_AUDIO_LISTENER_NAME_MAX 128
#endif

/* -------------------------------------------------------------------------
 * Listener structure
 * ---------------------------------------------------------------------- */

/**
 * @brief An audio listener instance.
 */
typedef struct wp_audio_listener
{
    wp_c8 name[WP_AUDIO_LISTENER_NAME_MAX];
    wp_vec3f position;
    wp_vec3f velocity;
    wp_vec3f forward;
    wp_vec3f up;
    void *native;
    void *user_data;
} wp_audio_listener;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

/**
 * @brief Create a new listener instance.
 * @return Pointer to the created listener, or NULL on failure.
 */
wp_audio_listener *wp_audio_listener_create( void );

/**
 * @brief Destroy a listener instance and release all resources.
 * @param listener Pointer to the listener. Ignored if NULL.
 */
void wp_audio_listener_destroy( wp_audio_listener *listener );

/* =========================================================================
 * Name
 * ====================================================================== */

/**
 * @brief Get the listener name.
 * @param listener Pointer to the listener.
 * @return Null-terminated name string, or NULL if listener is NULL.
 */
const wp_c8 *wp_audio_listener_get_name( const wp_audio_listener *listener );

/**
 * @brief Set the listener name.
 * @param listener Pointer to the listener.
 * @param name     Null-terminated name string.
 */
void wp_audio_listener_set_name( wp_audio_listener *listener, const wp_c8 *name );

/* =========================================================================
 * Position
 * ====================================================================== */

/**
 * @brief Get the listener position.
 * @param listener Pointer to the listener.
 * @return Position vector.
 */
wp_vec3f wp_audio_listener_get_position( const wp_audio_listener *listener );

/**
 * @brief Set the listener position.
 * @param listener Pointer to the listener.
 * @param position New position vector.
 */
void wp_audio_listener_set_position( wp_audio_listener *listener, wp_vec3f position );

/* =========================================================================
 * Velocity
 * ====================================================================== */

/**
 * @brief Get the listener velocity.
 * @param listener Pointer to the listener.
 * @return Velocity vector.
 */
wp_vec3f wp_audio_listener_get_velocity( const wp_audio_listener *listener );

/**
 * @brief Set the listener velocity.
 * @param listener Pointer to the listener.
 * @param velocity New velocity vector.
 */
void wp_audio_listener_set_velocity( wp_audio_listener *listener, wp_vec3f velocity );

/* =========================================================================
 * Orientation
 * ====================================================================== */

/**
 * @brief Get the forward direction vector.
 * @param listener Pointer to the listener.
 * @return Normalised forward vector.
 */
wp_vec3f wp_audio_listener_get_forward( const wp_audio_listener *listener );

/**
 * @brief Set the forward direction vector (will be normalised).
 * @param listener Pointer to the listener.
 * @param forward  New forward vector.
 */
void wp_audio_listener_set_forward( wp_audio_listener *listener, wp_vec3f forward );

/**
 * @brief Get the up direction vector.
 * @param listener Pointer to the listener.
 * @return Normalised up vector.
 */
wp_vec3f wp_audio_listener_get_up( const wp_audio_listener *listener );

/**
 * @brief Set the up direction vector (will be normalised).
 * @param listener Pointer to the listener.
 * @param up       New up vector.
 */
void wp_audio_listener_set_up( wp_audio_listener *listener, wp_vec3f up );

/**
 * @brief Set both forward and up orientation vectors (will be normalised).
 * @param listener Pointer to the listener.
 * @param forward  New forward vector.
 * @param up       New up vector.
 */
void wp_audio_listener_set_orientation( wp_audio_listener *listener, wp_vec3f forward, wp_vec3f up );

/**
 * @brief Get both forward and up orientation vectors.
 * @param listener Pointer to the listener.
 * @param forward  Output pointer for forward vector (may be NULL).
 * @param up       Output pointer for up vector (may be NULL).
 */
void wp_audio_listener_get_orientation( const wp_audio_listener *listener, wp_vec3f *forward,
                                        wp_vec3f *up );

/* =========================================================================
 * Native access
 * ====================================================================== */

/**
 * @brief Get the native / platform-specific object.
 * @param listener  Pointer to the listener.
 * @param pp_object Output pointer that receives the native object pointer.
 */
void wp_audio_listener_get_native( const wp_audio_listener *listener, void **pp_object );

/**
 * @brief Set the native / platform-specific object.
 * @param listener Pointer to the listener.
 * @param native   Pointer to the native object.
 */
void wp_audio_listener_set_native( wp_audio_listener *listener, void *native );

/* =========================================================================
 * User data
 * ====================================================================== */

/**
 * @brief Get the user data pointer.
 * @param listener Pointer to the listener.
 * @return User data pointer, or NULL if not set.
 */
void *wp_audio_listener_get_user_data( const wp_audio_listener *listener );

/**
 * @brief Set the user data pointer.
 * @param listener  Pointer to the listener.
 * @param user_data Pointer to user data.
 */
void wp_audio_listener_set_user_data( wp_audio_listener *listener, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_AUDIO_LISTENER_H */
