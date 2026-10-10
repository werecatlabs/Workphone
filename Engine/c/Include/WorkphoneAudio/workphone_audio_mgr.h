/**
 * @file workphone_audio_mgr.h
 * @brief C API for the audio manager.
 *
 * The audio manager is the central entry point for the audio subsystem.
 * It owns sounds and listeners, controls master volume and mute state,
 * and drives the per-frame audio update.
 *
 * Legacy device dispatch is currently unsupported: load/platform_init return 0.
 * Use workphone_audio_core.h for offline processing and WPAudio for device output.
 * No platform playback certification is implied by platform detection below.
 */

#ifndef WORKPHONE_AUDIO_MGR_H
#define WORKPHONE_AUDIO_MGR_H

#include "workphone_config.h"
#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Platform detection
 * ---------------------------------------------------------------------- */

/**
 * @def WP_AUDIO_PLATFORM_WINDOWS
 * Defined when compiling for Windows.
 */
#if defined( _WIN32 ) || defined( _WIN64 )
#    define WP_AUDIO_PLATFORM_WINDOWS 1
#else
#    define WP_AUDIO_PLATFORM_WINDOWS 0
#endif

/**
 * @def WP_AUDIO_PLATFORM_MACOS
 * Defined when compiling for macOS.
 */
#if defined( __APPLE__ ) && defined( __MACH__ )
#    include <TargetConditionals.h>
#    if defined( TARGET_OS_MAC ) && !defined( TARGET_OS_IPHONE )
#        define WP_AUDIO_PLATFORM_MACOS 1
#    else
#        define WP_AUDIO_PLATFORM_MACOS 0
#    endif
#else
#    define WP_AUDIO_PLATFORM_MACOS 0
#endif

/**
 * @def WP_AUDIO_PLATFORM_IOS
 * Defined when compiling for iOS.
 */
#if defined( __APPLE__ ) && defined( __MACH__ )
#    include <TargetConditionals.h>
#    if defined( TARGET_OS_IPHONE ) && !defined( TARGET_OS_SIMULATOR )
#        define WP_AUDIO_PLATFORM_IOS 1
#    else
#        define WP_AUDIO_PLATFORM_IOS 0
#    endif
#elif defined( __ARM_NEON__ ) || defined( __ARM_ARCH_7A__ )
/* Fallback for platforms that don't define TargetConditionals */
#    define WP_AUDIO_PLATFORM_IOS 0
#else
#    define WP_AUDIO_PLATFORM_IOS 0
#endif

/**
 * @def WP_AUDIO_PLATFORM_ANDROID
 * Defined when compiling for Android.
 */
#if defined( __ANDROID__ ) || defined( ANDROID )
#    define WP_AUDIO_PLATFORM_ANDROID 1
#else
#    define WP_AUDIO_PLATFORM_ANDROID 0
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_audio_mgr wp_audio_mgr;
typedef struct wp_audio_sound wp_audio_sound;
typedef struct wp_audio_listener wp_audio_listener;

/* -------------------------------------------------------------------------
 * Platform backend interface
 * ---------------------------------------------------------------------- */

/**
 * @struct wp_audio_mgr_backend
 * @brief Platform-specific audio backend functions.
 *
 * Each platform implements these functions and the audio manager
 * dispatches to them at runtime.
 */
struct wp_audio_mgr_backend
{
    /**
     * @brief Initialise the platform audio subsystem.
     * @return 1 on success, 0 on failure.
     */
    wp_s32 ( *init )( void );

    /**
     * @brief Shutdown the platform audio subsystem.
     */
    void ( *shutdown )( void );

    /**
     * @brief Update the platform audio subsystem (per-frame).
     * @param mgr Pointer to the audio manager.
     */
    void ( *update )( void *mgr );

    /**
     * @brief Load a sound from file.
     * @param filepath Path to the audio file.
     * @param loop    Non-zero to loop the sound.
     * @return Platform-specific sound handle, or NULL on failure.
     */
    void *( *sound_load )( const wp_c8 *filepath, wp_s32 loop );

    /**
     * @brief Unload a sound.
     * @param handle Platform-specific sound handle.
     */
    void ( *sound_unload )( void *handle );

    /**
     * @brief Play a sound.
     * @param handle Platform-specific sound handle.
     */
    void ( *sound_play )( void *handle );

    /**
     * @brief Stop a sound.
     * @param handle Platform-specific sound handle.
     */
    void ( *sound_stop )( void *handle );

    /**
     * @brief Pause a sound.
     * @param handle Platform-specific sound handle.
     */
    void ( *sound_pause )( void *handle );

    /**
     * @brief Resume a paused sound.
     * @param handle Platform-specific sound handle.
     */
    void ( *sound_resume )( void *handle );

    /**
     * @brief Set sound volume.
     * @param handle Platform-specific sound handle.
     * @param volume Volume level (0.0 to 1.0).
     */
    void ( *sound_set_volume )( void *handle, wp_f32 volume );

    /**
     * @brief Get sound volume.
     * @param handle Platform-specific sound handle.
     * @return Volume level (0.0 to 1.0).
     */
    wp_f32 ( *sound_get_volume )( void *handle );

    /**
     * @brief Set sound pitch.
     * @param handle Platform-specific sound handle.
     * @param pitch  Pitch multiplier (1.0 = normal).
     */
    void ( *sound_set_pitch )( void *handle, wp_f32 pitch );

    /**
     * @brief Get sound pitch.
     * @param handle Platform-specific sound handle.
     * @return Pitch multiplier.
     */
    wp_f32 ( *sound_get_pitch )( void *handle );

    /**
     * @brief Set sound position (for 3D audio).
     * @param handle Platform-specific sound handle.
     * @param x      X position.
     * @param y      Y position.
     * @param z      Z position.
     */
    void ( *sound_set_position )( void *handle, wp_f32 x, wp_f32 y, wp_f32 z );

    /**
     * @brief Check if a sound is playing.
     * @param handle Platform-specific sound handle.
     * @return Non-zero if playing, 0 otherwise.
     */
    wp_s32 ( *sound_is_playing )( void *handle );
};

typedef struct wp_audio_mgr_backend wp_audio_mgr_backend;

/**
 * @brief Get the platform-specific audio backend.
 * @return Pointer to the backend interface, or NULL if not available.
 */
const struct wp_audio_mgr_backend *wp_audio_mgr_get_backend( void );

/* -------------------------------------------------------------------------
 * Capacity defaults
 * ---------------------------------------------------------------------- */

#ifndef WP_AUDIO_MGR_INITIAL_SOUND_CAPACITY
#    define WP_AUDIO_MGR_INITIAL_SOUND_CAPACITY 16
#endif

#ifndef WP_AUDIO_MGR_INITIAL_LISTENER_CAPACITY
#    define WP_AUDIO_MGR_INITIAL_LISTENER_CAPACITY 4
#endif

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

/**
 * @brief Create a new audio manager.
 * @return Pointer to the created audio manager, or NULL on failure.
 */
wp_audio_mgr *wp_audio_mgr_create( void );

/**
 * @brief Destroy the audio manager and all resources it owns.
 * @param mgr Pointer to the audio manager. Ignored if NULL.
 */
void wp_audio_mgr_destroy( wp_audio_mgr *mgr );

/**
 * @brief Load / initialise the audio back-end.
 * @param mgr Pointer to the audio manager.
 * @return 1 on success, 0 on failure.
 */
wp_s32 wp_audio_mgr_load( wp_audio_mgr *mgr );

/**
 * @brief Unload the audio back-end and release platform resources.
 * @param mgr Pointer to the audio manager.
 */
void wp_audio_mgr_unload( wp_audio_mgr *mgr );

/* =========================================================================
 * Update
 * ====================================================================== */

/**
 * @brief Advance the audio manager by one frame.
 * @param mgr Pointer to the audio manager.
 */
void wp_audio_mgr_update( wp_audio_mgr *mgr );

/* =========================================================================
 * Sound management
 * ====================================================================== */

/**
 * @brief Add a sound loaded from the given file path.
 * @param mgr      Pointer to the audio manager.
 * @param filepath Null-terminated path to the audio file.
 * @param loop     Non-zero to loop the sound.
 * @return Index of the created sound (>= 0), or -1 on failure.
 */
wp_s32 wp_audio_mgr_add_sound( wp_audio_mgr *mgr, const wp_c8 *filepath, wp_s32 loop );

/**
 * @brief Remove a sound by index.
 * @param mgr   Pointer to the audio manager.
 * @param index  Zero-based sound index.
 */
void wp_audio_mgr_remove_sound( wp_audio_mgr *mgr, wp_s32 index );

/**
 * @brief Remove all sounds from the manager.
 * @param mgr Pointer to the audio manager.
 */
void wp_audio_mgr_remove_all_sounds( wp_audio_mgr *mgr );

/**
 * @brief Get a sound by index.
 * @param mgr   Pointer to the audio manager.
 * @param index  Zero-based sound index.
 * @return Pointer to the sound, or NULL if the index is out of range.
 */
wp_audio_sound *wp_audio_mgr_get_sound( const wp_audio_mgr *mgr, wp_s32 index );

/**
 * @brief Get the number of sounds currently managed.
 * @param mgr Pointer to the audio manager.
 * @return Number of active sounds.
 */
wp_s32 wp_audio_mgr_get_sound_count( const wp_audio_mgr *mgr );

/* =========================================================================
 * Listener management
 * ====================================================================== */

/**
 * @brief Add a named 3D listener at the given position.
 * @param mgr  Pointer to the audio manager.
 * @param name Null-terminated unique name.
 * @param x    Initial X position.
 * @param y    Initial Y position.
 * @param z    Initial Z position.
 * @return Index of the created listener (>= 0), or -1 on failure.
 */
wp_s32 wp_audio_mgr_add_listener( wp_audio_mgr *mgr, const wp_c8 *name, wp_f32 x, wp_f32 y, wp_f32 z );

/**
 * @brief Find a listener by name.
 * @param mgr  Pointer to the audio manager.
 * @param name Null-terminated name to search for.
 * @return Pointer to the listener, or NULL if not found.
 */
wp_audio_listener *wp_audio_mgr_find_listener( const wp_audio_mgr *mgr, const wp_c8 *name );

/**
 * @brief Remove a listener by index.
 * @param mgr   Pointer to the audio manager.
 * @param index  Zero-based listener index.
 */
void wp_audio_mgr_remove_listener( wp_audio_mgr *mgr, wp_s32 index );

/**
 * @brief Remove all listeners from the manager.
 * @param mgr Pointer to the audio manager.
 */
void wp_audio_mgr_remove_all_listeners( wp_audio_mgr *mgr );

/**
 * @brief Get a listener by index.
 * @param mgr   Pointer to the audio manager.
 * @param index  Zero-based listener index.
 * @return Pointer to the listener, or NULL if the index is out of range.
 */
wp_audio_listener *wp_audio_mgr_get_listener( const wp_audio_mgr *mgr, wp_s32 index );

/**
 * @brief Get the number of listeners currently managed.
 * @param mgr Pointer to the audio manager.
 * @return Number of active listeners.
 */
wp_s32 wp_audio_mgr_get_listener_count( const wp_audio_mgr *mgr );

/* =========================================================================
 * Volume
 * ====================================================================== */

/**
 * @brief Get the master volume.
 * @param mgr Pointer to the audio manager.
 * @return Volume level (0.0 to 1.0).
 */
wp_f32 wp_audio_mgr_get_volume( const wp_audio_mgr *mgr );

/**
 * @brief Set the master volume.
 * @param mgr    Pointer to the audio manager.
 * @param volume Volume level (0.0 to 1.0).
 */
void wp_audio_mgr_set_volume( wp_audio_mgr *mgr, wp_f32 volume );

/* =========================================================================
 * Mute
 * ====================================================================== */

/**
 * @brief Check whether audio is muted.
 * @param mgr Pointer to the audio manager.
 * @return Non-zero if muted, 0 otherwise.
 */
wp_s32 wp_audio_mgr_is_mute( const wp_audio_mgr *mgr );

/**
 * @brief Set the mute state.
 * @param mgr  Pointer to the audio manager.
 * @param mute Non-zero to mute, 0 to unmute.
 */
void wp_audio_mgr_set_mute( wp_audio_mgr *mgr, wp_s32 mute );

/* =========================================================================
 * Native access
 * ====================================================================== */

/**
 * @brief Retrieve the native / platform-specific audio object.
 * @param mgr       Pointer to the audio manager.
 * @param pp_object Output pointer that receives the native object pointer.
 */
void wp_audio_mgr_get_native( const wp_audio_mgr *mgr, void **pp_object );

/**
 * @brief Set the native / platform-specific audio object.
 * @param mgr    Pointer to the audio manager.
 * @param native Pointer to the native object.
 */
void wp_audio_mgr_set_native( wp_audio_mgr *mgr, void *native );

/* =========================================================================
 * User data
 * ====================================================================== */

/**
 * @brief Get the user data pointer.
 * @param mgr Pointer to the audio manager.
 * @return User data pointer, or NULL if not set.
 */
void *wp_audio_mgr_get_user_data( const wp_audio_mgr *mgr );

/**
 * @brief Set the user data pointer.
 * @param mgr       Pointer to the audio manager.
 * @param user_data Pointer to user data.
 */
void wp_audio_mgr_set_user_data( wp_audio_mgr *mgr, void *user_data );

/* =========================================================================
 * Platform subsystem lifecycle
 * ====================================================================== */

/**
 * @brief Initialise the platform-specific audio subsystem.
 * @return 1 on success, 0 on failure.
 */
wp_s32 wp_audio_mgr_platform_init( void );

/**
 * @brief Shutdown the platform-specific audio subsystem.
 */
void wp_audio_mgr_platform_shutdown( void );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_AUDIO_MGR_H */
