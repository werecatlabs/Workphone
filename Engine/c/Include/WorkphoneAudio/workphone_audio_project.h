/**
 * @file workphone_audio_project.h
 * @brief C API for an audio project.
 *
 * An audio project is a named, self-contained collection of sounds that
 * can be loaded and unloaded as a group.  It holds an optional base path
 * that is prepended to each relative sound file path on load.
 * Platform-specific handles are stored via the opaque native pointer.
 */

#ifndef WORKPHONE_AUDIO_PROJECT_H
#define WORKPHONE_AUDIO_PROJECT_H

#include "workphone_config.h"
#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_audio_project wp_audio_project;
typedef struct wp_audio_sound wp_audio_sound;

/* -------------------------------------------------------------------------
 * Capacity defaults
 * ---------------------------------------------------------------------- */

#ifndef WP_AUDIO_PROJECT_INITIAL_SOUND_CAPACITY
#    define WP_AUDIO_PROJECT_INITIAL_SOUND_CAPACITY 16
#endif

#ifndef WP_AUDIO_PROJECT_NAME_MAX
#    define WP_AUDIO_PROJECT_NAME_MAX 128
#endif

#ifndef WP_AUDIO_PROJECT_PATH_MAX
#    define WP_AUDIO_PROJECT_PATH_MAX 512
#endif

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

/**
 * @brief Create a new audio project.
 * @return Pointer to the created project, or NULL on failure.
 */
wp_audio_project *wp_audio_project_create( void );

/**
 * @brief Destroy an audio project and all resources it owns.
 * @param project Pointer to the project. Ignored if NULL.
 */
void wp_audio_project_destroy( wp_audio_project *project );

/**
 * @brief Load / initialise the project back-end.
 * @param project Pointer to the project.
 * @return 1 on success, 0 on failure.
 */
wp_s32 wp_audio_project_load( wp_audio_project *project );

/**
 * @brief Unload the project and release backend resources for all sounds.
 * @param project Pointer to the project.
 */
void wp_audio_project_unload( wp_audio_project *project );

/* =========================================================================
 * Update
 * ====================================================================== */

/**
 * @brief Advance the project by one frame.
 * @param project Pointer to the project.
 */
void wp_audio_project_update( wp_audio_project *project );

/* =========================================================================
 * Name
 * ====================================================================== */

/**
 * @brief Get the project name.
 * @param project Pointer to the project.
 * @return Null-terminated name string, or NULL if project is NULL.
 */
const wp_c8 *wp_audio_project_get_name( const wp_audio_project *project );

/**
 * @brief Set the project name.
 * @param project Pointer to the project.
 * @param name    Null-terminated name string.
 */
void wp_audio_project_set_name( wp_audio_project *project, const wp_c8 *name );

/* =========================================================================
 * Path
 * ====================================================================== */

/**
 * @brief Get the base path for audio assets in this project.
 * @param project Pointer to the project.
 * @return Null-terminated path string, or NULL if project is NULL.
 */
const wp_c8 *wp_audio_project_get_path( const wp_audio_project *project );

/**
 * @brief Set the base path for audio assets in this project.
 * @param project Pointer to the project.
 * @param path    Null-terminated base path string.
 */
void wp_audio_project_set_path( wp_audio_project *project, const wp_c8 *path );

/* =========================================================================
 * Sound management
 * ====================================================================== */

/**
 * @brief Add a sound to the project loaded from the given file path.
 *
 * If the project has a base path set and @p filepath is relative, the base
 * path is prepended before loading.
 *
 * @param project  Pointer to the project.
 * @param filepath Null-terminated path to the audio file.
 * @param loop     Non-zero to loop the sound.
 * @return Index of the created sound (>= 0), or -1 on failure.
 */
wp_s32 wp_audio_project_add_sound( wp_audio_project *project, const wp_c8 *filepath, wp_s32 loop );

/**
 * @brief Remove a sound by index.
 * @param project Pointer to the project.
 * @param index   Zero-based sound index.
 */
void wp_audio_project_remove_sound( wp_audio_project *project, wp_s32 index );

/**
 * @brief Remove all sounds from the project.
 * @param project Pointer to the project.
 */
void wp_audio_project_remove_all_sounds( wp_audio_project *project );

/**
 * @brief Get a sound by index.
 * @param project Pointer to the project.
 * @param index   Zero-based sound index.
 * @return Pointer to the sound, or NULL if the index is out of range.
 */
wp_audio_sound *wp_audio_project_get_sound( const wp_audio_project *project, wp_s32 index );

/**
 * @brief Get the number of sounds in the project.
 * @param project Pointer to the project.
 * @return Number of sounds.
 */
wp_s32 wp_audio_project_get_sound_count( const wp_audio_project *project );

/* =========================================================================
 * Volume
 * ====================================================================== */

/**
 * @brief Get the project volume.
 * @param project Pointer to the project.
 * @return Volume level (0.0 to 1.0).
 */
wp_f32 wp_audio_project_get_volume( const wp_audio_project *project );

/**
 * @brief Set the project volume.
 * @param project Pointer to the project.
 * @param volume  Volume level (0.0 to 1.0).
 */
void wp_audio_project_set_volume( wp_audio_project *project, wp_f32 volume );

/* =========================================================================
 * Mute
 * ====================================================================== */

/**
 * @brief Check whether the project is muted.
 * @param project Pointer to the project.
 * @return Non-zero if muted, 0 otherwise.
 */
wp_s32 wp_audio_project_is_mute( const wp_audio_project *project );

/**
 * @brief Set the mute state of the project.
 * @param project Pointer to the project.
 * @param mute    Non-zero to mute, 0 to unmute.
 */
void wp_audio_project_set_mute( wp_audio_project *project, wp_s32 mute );

/* =========================================================================
 * Loaded state
 * ====================================================================== */

/**
 * @brief Check whether the project has been loaded.
 * @param project Pointer to the project.
 * @return Non-zero if loaded, 0 otherwise.
 */
wp_s32 wp_audio_project_is_loaded( const wp_audio_project *project );

/* =========================================================================
 * Native access
 * ====================================================================== */

/**
 * @brief Get the native / platform-specific object.
 * @param project   Pointer to the project.
 * @param pp_object Output pointer that receives the native object pointer.
 */
void wp_audio_project_get_native( const wp_audio_project *project, void **pp_object );

/**
 * @brief Set the native / platform-specific object.
 * @param project Pointer to the project.
 * @param native  Pointer to the native object.
 */
void wp_audio_project_set_native( wp_audio_project *project, void *native );

/* =========================================================================
 * User data
 * ====================================================================== */

/**
 * @brief Get the user data pointer.
 * @param project Pointer to the project.
 * @return User data pointer, or NULL if not set.
 */
void *wp_audio_project_get_user_data( const wp_audio_project *project );

/**
 * @brief Set the user data pointer.
 * @param project   Pointer to the project.
 * @param user_data Pointer to user data.
 */
void wp_audio_project_set_user_data( wp_audio_project *project, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_AUDIO_PROJECT_H */
