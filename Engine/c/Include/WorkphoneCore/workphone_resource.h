/**
 * @file workphone_resource.h
 * @brief C API for engine resources -- the base type for all managed assets.
 */

#ifndef WORKPHONE_RESOURCE_H
#define WORKPHONE_RESOURCE_H

#include "workphone_prerequisites.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_resource wp_resource;

/* =========================================================================
 * Enumerations
 * ====================================================================== */

/**
 * @brief Category of a managed resource.
 */
typedef enum wp_resource_type
{
    WP_RESOURCE_TYPE_UNKNOWN = 0,
    WP_RESOURCE_TYPE_TEXTURE = 1,
    WP_RESOURCE_TYPE_MESH = 2,
    WP_RESOURCE_TYPE_AUDIO = 3,
    WP_RESOURCE_TYPE_MATERIAL = 4,
    WP_RESOURCE_TYPE_SHADER = 5,
    WP_RESOURCE_TYPE_SCRIPT = 6,
    WP_RESOURCE_TYPE_FONT = 7,
    WP_RESOURCE_TYPE_ANIMATION = 8,
    WP_RESOURCE_TYPE_SCENE = 9,
    WP_RESOURCE_TYPE_PREFAB = 10,
    WP_RESOURCE_TYPE_CUSTOM = 11,
    WP_RESOURCE_TYPE_COUNT = 12
} wp_resource_type;

/**
 * @brief Loading state of a resource.
 */
typedef enum wp_resource_state
{
    WP_RESOURCE_STATE_NONE = 0,
    WP_RESOURCE_STATE_LOADING = 1,
    WP_RESOURCE_STATE_LOADED = 2,
    WP_RESOURCE_STATE_FAILED = 3,
    WP_RESOURCE_STATE_UNLOADING = 4,
    WP_RESOURCE_STATE_COUNT = 5
} wp_resource_state;

/**
 * @brief Behaviour flags for a resource.
 */
enum
{
    WP_RESOURCE_FLAG_KEEP_ALIVE = ( 1u << 0 ), /**< Don't auto-unload when ref count hits 0. */
    WP_RESOURCE_FLAG_STREAMING = ( 1u << 1 ),  /**< Load data in chunks asynchronously.       */
    WP_RESOURCE_FLAG_BUILT_IN = ( 1u << 2 ),   /**< Engine built-in; not loaded from disk.    */
    WP_RESOURCE_FLAG_DIRTY = ( 1u << 3 ),      /**< Resource data has been modified.           */
    WP_RESOURCE_FLAG_ASYNC = ( 1u << 4 )       /**< Perform load/unload asynchronously.        */
};

/* =========================================================================
 * Constants
 * ====================================================================== */

#ifndef WP_RESOURCE_MAX_PATH
#    define WP_RESOURCE_MAX_PATH 512
#endif

#ifndef WP_RESOURCE_MAX_NAME
#    define WP_RESOURCE_MAX_NAME 128
#endif

/* =========================================================================
 * Callbacks
 * ====================================================================== */

/**
 * @brief Called when a resource should load its data into memory.
 *
 * The callback is responsible for setting the resource state to either
 * WP_RESOURCE_STATE_LOADED or WP_RESOURCE_STATE_FAILED on completion.
 *
 * @param resource  The resource being loaded.
 * @param user_data Opaque context supplied when the callback was registered.
 */
typedef void ( *wp_resource_load_fn )( wp_resource *resource, void *user_data );

/**
 * @brief Called when a resource should release its in-memory data.
 * @param resource  The resource being unloaded.
 * @param user_data Opaque context supplied when the callback was registered.
 */
typedef void ( *wp_resource_unload_fn )( wp_resource *resource, void *user_data );

/* =========================================================================
 * Structure
 * ====================================================================== */

/**
 * @brief Base type for all managed engine assets.
 *
 * A resource represents a single named, typed asset that may be loaded
 * from disk or supplied as built-in data.  Reference counting drives
 * automatic unloading via wp_resource_release(); set
 * WP_RESOURCE_FLAG_KEEP_ALIVE to suppress that behaviour.
 */
typedef struct wp_resource
{
    wp_s32 id;
    wp_resource_type type;
    wp_resource_state state;
    wp_u32 flags;
    wp_s32 ref_count;

    wp_c8 path[WP_RESOURCE_MAX_PATH];
    wp_c8 name[WP_RESOURCE_MAX_NAME];

    void *data;
    wp_u32 data_size;

    wp_resource_load_fn load_fn;
    void *load_fn_user_data;

    wp_resource_unload_fn unload_fn;
    void *unload_fn_user_data;

    void *user_data;
} wp_resource;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

/**
 * @brief Allocates and initialises a new resource with a unique id.
 * @return Pointer to the created resource, or NULL on allocation failure.
 */
wp_resource *wp_resource_create( void );

/**
 * @brief Unloads and frees a resource.
 *
 * If the resource is currently loaded, wp_resource_unload() is called first.
 *
 * @param resource Pointer to the resource.  Ignored if NULL.
 */
void wp_resource_destroy( wp_resource *resource );

/* =========================================================================
 * Reference counting
 * ====================================================================== */

/**
 * @brief Increments the reference count.
 * @param resource Pointer to the resource.
 */
void wp_resource_add_ref( wp_resource *resource );

/**
 * @brief Decrements the reference count.
 *
 * When the count reaches zero and WP_RESOURCE_FLAG_KEEP_ALIVE is not set,
 * wp_resource_unload() is called automatically.
 *
 * @param resource Pointer to the resource.
 */
void wp_resource_release( wp_resource *resource );

/**
 * @brief Returns the current reference count.
 * @param resource Pointer to the resource.
 * @return Reference count, or 0 if resource is NULL.
 */
wp_s32 wp_resource_get_ref_count( const wp_resource *resource );

/* =========================================================================
 * Load / Unload
 * ====================================================================== */

/**
 * @brief Invokes the load callback to bring resource data into memory.
 *
 * Sets the state to WP_RESOURCE_STATE_LOADING before calling the callback.
 * If no load callback has been registered the state advances directly to
 * WP_RESOURCE_STATE_LOADED.  Has no effect if already loaded.
 *
 * @param resource Pointer to the resource.
 */
void wp_resource_load( wp_resource *resource );

/**
 * @brief Invokes the unload callback to release in-memory resource data.
 *
 * Sets the state to WP_RESOURCE_STATE_UNLOADING, calls the callback if one
 * is registered, then clears the data pointer and resets the state to
 * WP_RESOURCE_STATE_NONE.  Has no effect if not loaded.
 *
 * @param resource Pointer to the resource.
 */
void wp_resource_unload( wp_resource *resource );

/* =========================================================================
 * Callbacks
 * ====================================================================== */

/**
 * @brief Registers the load callback.
 * @param resource  Pointer to the resource.
 * @param fn        Load callback, or NULL to clear.
 * @param user_data Opaque context forwarded to the callback.
 */
void wp_resource_set_load_fn( wp_resource *resource, wp_resource_load_fn fn, void *user_data );

/**
 * @brief Registers the unload callback.
 * @param resource  Pointer to the resource.
 * @param fn        Unload callback, or NULL to clear.
 * @param user_data Opaque context forwarded to the callback.
 */
void wp_resource_set_unload_fn( wp_resource *resource, wp_resource_unload_fn fn, void *user_data );

/* =========================================================================
 * Accessors
 * ====================================================================== */

wp_s32 wp_resource_get_id( const wp_resource *resource );
void wp_resource_set_id( wp_resource *resource, wp_s32 id );

wp_resource_type wp_resource_get_type( const wp_resource *resource );
void wp_resource_set_type( wp_resource *resource, wp_resource_type type );

wp_resource_state wp_resource_get_state( const wp_resource *resource );
void wp_resource_set_state( wp_resource *resource, wp_resource_state state );

wp_u32 wp_resource_get_flags( const wp_resource *resource );
void wp_resource_set_flags( wp_resource *resource, wp_u32 flags );
wp_s32 wp_resource_get_flag( const wp_resource *resource, wp_u32 flag );
void wp_resource_set_flag( wp_resource *resource, wp_u32 flag, wp_s32 value );

const wp_c8 *wp_resource_get_path( const wp_resource *resource );
void wp_resource_set_path( wp_resource *resource, const wp_c8 *path );

const wp_c8 *wp_resource_get_name( const wp_resource *resource );
void wp_resource_set_name( wp_resource *resource, const wp_c8 *name );

void *wp_resource_get_data( const wp_resource *resource );
void wp_resource_set_data( wp_resource *resource, void *data, wp_u32 size );
wp_u32 wp_resource_get_data_size( const wp_resource *resource );

void *wp_resource_get_user_data( const wp_resource *resource );
void wp_resource_set_user_data( wp_resource *resource, void *user_data );

/* =========================================================================
 * Queries
 * ====================================================================== */

/**
 * @brief Returns non-zero if the resource is fully loaded.
 * @param resource Pointer to the resource.
 */
wp_s32 wp_resource_is_loaded( const wp_resource *resource );

/**
 * @brief Returns non-zero if the resource is currently loading.
 * @param resource Pointer to the resource.
 */
wp_s32 wp_resource_is_loading( const wp_resource *resource );

/**
 * @brief Returns non-zero if the resource failed to load.
 * @param resource Pointer to the resource.
 */
wp_s32 wp_resource_is_failed( const wp_resource *resource );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_RESOURCE_H */
