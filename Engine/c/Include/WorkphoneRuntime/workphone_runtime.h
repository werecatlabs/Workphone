#ifndef workphone_runtime_h__
#define workphone_runtime_h__

#include "workphone_types.h"
#include "workphone_graphics_renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_runtime wp_runtime;
typedef struct wp_graphics_system wp_graphics_system;
typedef struct wp_platform_window_win32 wp_platform_window_win32;
typedef struct wp_threadpool wp_threadpool;
typedef struct wp_script_state wp_script_state;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

/**
 * @brief Creates a runtime instance with the specified renderer back-end.
 *
 * Allocates a graphics system, the chosen renderer back-end, a Win32
 * window, and a thread pool.
 *
 * @param width  Framebuffer width in pixels.  Must be > 0.
 * @param height Framebuffer height in pixels.  Must be > 0.
 * @param type   Renderer back-end to use (software or DX11).
 * @return Pointer to the created runtime, or NULL on failure.
 */
wp_runtime *wp_runtime_create( wp_s32 width, wp_s32 height, wp_renderer_type type );

/**
 * @brief Destroys a runtime and frees all associated resources.
 * @param runtime Pointer to the runtime.  Ignored if NULL.
 */
void wp_runtime_destroy( wp_runtime *runtime );

/* =========================================================================
 * Main loop
 * ====================================================================== */

/**
 * @brief Enters the runtime main loop, calling update then render each tick.
 *
 * Returns only after wp_runtime_stop() has been called.
 *
 * @param runtime Pointer to the runtime.
 */
void wp_runtime_run( wp_runtime *runtime );

/**
 * @brief Signals the main loop to exit on the next iteration.
 * @param runtime Pointer to the runtime.
 */
void wp_runtime_stop( wp_runtime *runtime );

/**
 * @brief Queries whether the main loop is active.
 * @param runtime Pointer to the runtime.
 * @return Non-zero while the loop is running.
 */
wp_s32 wp_runtime_is_running( const wp_runtime *runtime );

/* =========================================================================
 * Per-frame
 * ====================================================================== */

/**
 * @brief Advances all runtime subsystems by one logical tick.
 * @param runtime Pointer to the runtime.
 * @param dt      Delta-time in seconds since the last update.
 */
void wp_runtime_update( wp_runtime *runtime, wp_f64 dt );

/**
 * @brief Renders one frame: begin, clear, scene render, end.
 * @param runtime Pointer to the runtime.
 */
void wp_runtime_render( wp_runtime *runtime );

/* =========================================================================
 * Accessors
 * ====================================================================== */

/**
 * @brief Returns the abstract renderer (software back-end).
 * @param runtime Pointer to the runtime.
 * @return Pointer to the renderer, or NULL.
 */
wp_renderer *wp_runtime_get_renderer( const wp_runtime *runtime );

/**
 * @brief Returns the graphics system that owns all scenes.
 * @param runtime Pointer to the runtime.
 * @return Pointer to the graphics system, or NULL.
 */
wp_graphics_system *wp_runtime_get_graphics_system( const wp_runtime *runtime );

/**
 * @brief Returns the worker thread pool.
 * @param runtime Pointer to the runtime.
 * @return Pointer to the thread pool, or NULL.
 */
wp_threadpool *wp_runtime_get_thread_pool( const wp_runtime *runtime );

/**
 * @brief Returns the framebuffer width the runtime was created with.
 * @param runtime Pointer to the runtime.
 * @return Width in pixels, or 0.
 */
wp_s32 wp_runtime_get_width( const wp_runtime *runtime );

/**
 * @brief Returns the framebuffer height the runtime was created with.
 * @param runtime Pointer to the runtime.
 * @return Height in pixels, or 0.
 */
wp_s32 wp_runtime_get_height( const wp_runtime *runtime );

/**
 * @brief Returns the render window created by this runtime.
 * @param runtime Pointer to the runtime.
 * @return Pointer to the render window, or NULL.
 */
wp_platform_window_win32 *wp_runtime_get_window( const wp_runtime *runtime );

/**
 * @brief Returns the scripting state owned by this runtime.
 */
wp_script_state *wp_runtime_get_script_state( const wp_runtime *runtime );

/**
 * @brief Sets the project file or folder used for script discovery.
 *
 * A folder is searched recursively for .meow and .lua files. Either file
 * extension is treated as Lua 5.x source and executed by Lua/Luabind.
 */
wp_s32 wp_runtime_set_project_path( wp_runtime *runtime, const wp_c8 *path );

/**
 * @brief Returns the configured project path, or an empty string.
 */
const wp_c8 *wp_runtime_get_project_path( const wp_runtime *runtime );

/* =========================================================================
 * Script loading
 * ====================================================================== */

/**
 * @brief Loads and executes a .meow or .lua script via WorkphoneScript.
 *
 * Reads the file into memory, compiles it with Lua, and runs the retained chunk.
 *
 * @param runtime Pointer to the runtime.
 * @param path    File path to the .meow or .lua script.
 * @return Non-zero on success, 0 on failure.
 */
wp_s32 wp_runtime_load_script( wp_runtime *runtime, const wp_c8 *path );

/**
 * @brief Recursively loads all .meow and .lua scripts under a path.
 * @return Number of scripts loaded, or -1 when discovery/compilation fails.
 */
wp_s32 wp_runtime_load_scripts( wp_runtime *runtime, const wp_c8 *path );

/**
 * @brief Reloads the configured project scripts into a fresh VM.
 *
 * The existing VM remains active if discovery, compilation, or application
 * initialisation fails.
 */
wp_s32 wp_runtime_reload_scripts( wp_runtime *runtime );

#ifdef __cplusplus
}
#endif

#endif /* workphone_runtime_h__ */
