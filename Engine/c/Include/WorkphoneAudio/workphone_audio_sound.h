/**
 * @file workphone_audio_sound.h
 * @brief C API for an individual audio sound instance.
 *
 * A sound represents a single loadable audio resource.  It tracks its file
 * path, playback state, volume and loop mode.  Platform-specific handles
 * are stored via the opaque native pointer.
 */

#ifndef WORKPHONE_AUDIO_SOUND_H
#define WORKPHONE_AUDIO_SOUND_H

#include "workphone_config.h"
#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Sound state
 * ---------------------------------------------------------------------- */

/**
 * @brief Playback state of a sound.
 */
typedef enum wp_audio_sound_state
{
    WP_AUDIO_SOUND_STATE_STOPPED = 0,
    WP_AUDIO_SOUND_STATE_PLAYING = 1,
    WP_AUDIO_SOUND_STATE_PAUSED = 2
} wp_audio_sound_state;

/* -------------------------------------------------------------------------
 * Sound structure
 * ---------------------------------------------------------------------- */

/**
 * @brief An individual audio sound instance.
 */
typedef struct wp_audio_sound
{
    wp_c8 *filepath;
    wp_s32 loop;
    wp_f32 volume;
    wp_audio_sound_state state;
    wp_s32 loaded;
    void *native;
    void *user_data;
} wp_audio_sound;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

/**
 * @brief Create a new sound instance.
 * @return Pointer to the created sound, or NULL on failure.
 */
wp_audio_sound *wp_audio_sound_create( void );

/**
 * @brief Destroy a sound instance and release all resources.
 * @param sound Pointer to the sound. Ignored if NULL.
 */
void wp_audio_sound_destroy( wp_audio_sound *sound );

/* =========================================================================
 * Loading
 * ====================================================================== */

/**
 * @brief Load audio data from a file path.
 * @param sound    Pointer to the sound.
 * @param filepath Null-terminated path to the audio file.
 * @param loop     Non-zero to loop the sound.
 * @return 1 on success, 0 on failure.
 */
wp_s32 wp_audio_sound_load( wp_audio_sound *sound, const wp_c8 *filepath, wp_s32 loop );

/**
 * @brief Unload audio data and release backend resources.
 * @param sound Pointer to the sound.
 */
void wp_audio_sound_unload( wp_audio_sound *sound );

/**
 * @brief Check whether the sound has been loaded.
 * @param sound Pointer to the sound.
 * @return Non-zero if loaded, 0 otherwise.
 */
wp_s32 wp_audio_sound_is_loaded( const wp_audio_sound *sound );

/* =========================================================================
 * Playback
 * ====================================================================== */

/**
 * @brief Start playback.
 * @param sound Pointer to the sound.
 */
void wp_audio_sound_play( wp_audio_sound *sound );

/**
 * @brief Stop playback.
 * @param sound Pointer to the sound.
 */
void wp_audio_sound_stop( wp_audio_sound *sound );

/**
 * @brief Check whether the sound is currently playing.
 * @param sound Pointer to the sound.
 * @return Non-zero if playing, 0 otherwise.
 */
wp_s32 wp_audio_sound_is_playing( const wp_audio_sound *sound );

/**
 * @brief Get the current playback state.
 * @param sound Pointer to the sound.
 * @return The playback state.
 */
wp_audio_sound_state wp_audio_sound_get_state( const wp_audio_sound *sound );

/* =========================================================================
 * Volume
 * ====================================================================== */

/**
 * @brief Get the volume.
 * @param sound Pointer to the sound.
 * @return Volume level (0.0 to 1.0).
 */
wp_f32 wp_audio_sound_get_volume( const wp_audio_sound *sound );

/**
 * @brief Set the volume.
 * @param sound  Pointer to the sound.
 * @param volume Volume level (0.0 to 1.0).
 */
void wp_audio_sound_set_volume( wp_audio_sound *sound, wp_f32 volume );

/* =========================================================================
 * Loop
 * ====================================================================== */

/**
 * @brief Get the loop state.
 * @param sound Pointer to the sound.
 * @return Non-zero if looping, 0 otherwise.
 */
wp_s32 wp_audio_sound_get_loop( const wp_audio_sound *sound );

/**
 * @brief Set the loop state.
 * @param sound Pointer to the sound.
 * @param loop  Non-zero to loop, 0 to play once.
 */
void wp_audio_sound_set_loop( wp_audio_sound *sound, wp_s32 loop );

/* =========================================================================
 * File path
 * ====================================================================== */

/**
 * @brief Get the file path associated with this sound.
 * @param sound Pointer to the sound.
 * @return Null-terminated file path, or NULL if not set.
 */
const wp_c8 *wp_audio_sound_get_filepath( const wp_audio_sound *sound );

/* =========================================================================
 * Native access
 * ====================================================================== */

/**
 * @brief Get the native / platform-specific object.
 * @param sound     Pointer to the sound.
 * @param pp_object Output pointer that receives the native object pointer.
 */
void wp_audio_sound_get_native( const wp_audio_sound *sound, void **pp_object );

/**
 * @brief Set the native / platform-specific object.
 * @param sound  Pointer to the sound.
 * @param native Pointer to the native object.
 */
void wp_audio_sound_set_native( wp_audio_sound *sound, void *native );

/* =========================================================================
 * User data
 * ====================================================================== */

/**
 * @brief Get the user data pointer.
 * @param sound Pointer to the sound.
 * @return User data pointer, or NULL if not set.
 */
void *wp_audio_sound_get_user_data( const wp_audio_sound *sound );

/**
 * @brief Set the user data pointer.
 * @param sound     Pointer to the sound.
 * @param user_data Pointer to user data.
 */
void wp_audio_sound_set_user_data( wp_audio_sound *sound, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_AUDIO_SOUND_H */
