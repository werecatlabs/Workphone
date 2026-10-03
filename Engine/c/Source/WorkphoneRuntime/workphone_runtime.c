/**
 * @file workphone_runtime.c
 * @brief Runtime lifecycle using the software renderer.
 *
 * Mirrors the structure of WorkphoneEditor but targets a CPU-side
 * framebuffer via the software renderer from WorkphoneGraphics instead of
 * a hardware (D3D11) back-end.
 */

#define WORKPHONE_IMPLEMENTATION
#include "workphone_prerequisites.h"
#include "workphone_ui.h"
#include "workphone.h"
#include "workphone_runtime.h"
#include "workphone_graphics_renderer.h"
#include "workphone_graphics_renderer_software.h"
#include "workphone_graphics_system.h"
#include "workphone_graphics_renderer_dx11.h"
#include "workphone_graphics_renderer_dx12.h"
#include "workphone_graphics_scene.h"
#include "workphone_graphics_camera.h"
/* #undef WORKPHONE_D3D11_IMPLEMENTATION */
/* #include "workphone_d3d11.h" */
#include "workphone_platform_window_win32.h"
#include "workphone_threadpool.h"
#include "workphone_fsmmgr.h"
#include "workphone_script_parser.h"
#include "workphone_script_vm.h"
#include "workphone_script_object.h"
#include "workphone_script_method.h"
#include "workphone_script_state.h"
#include "workphone_script_value.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_INDEX_BUFFER 128 * 1024

typedef struct wp_runtime_ui_vertex
{
    wp_f32 position[2];
    wp_f32 uv[2];
    wp_byte color[4];
} wp_runtime_ui_vertex;

typedef struct wp_runtime_software_texture
{
    wp_u8 *pixels;
    wp_s32 width;
    wp_s32 height;
    wp_pixel_format format;
} wp_runtime_software_texture;

/* =========================================================================
 * Internal structure
 * ====================================================================== */
struct wp_runtime
{
    wp_renderer *renderer;
    wp_renderer_type renderer_type;
    wp_graphics_system *graphics_system;
    wp_graphics_scene *default_scene;
    wp_camera *default_camera;
    wp_platform_window_win32 *window;
    wp_threadpool *thread_pool;
    wp_fsm_manager fsm_mgr;

    wp_s32 width;
    wp_s32 height;
    wp_s32 running;

    wp_script_state *script_state;
    wp_script_object *app_object;
    wp_c8 *project_path;
    wp_s32 scripts_loaded;
    wp_s32 app_initialized;

    struct wp_context *ctx;
    struct wp_colorf bg;

    struct wp_font_atlas software_atlas;
    struct wp_draw_null_texture software_null_texture;
    struct wp_buffer software_ui_commands;
    wp_runtime_software_texture software_font_texture;
    void *software_ui_vertices;
    wp_vertex_ptc *software_draw_vertices;
    void *software_ui_indices;
    wp_s32 software_ui_initialized;
    wp_s32 fsm_initialized;
};

static wp_s32 wp_runtime_handle_window_event( void *userdata, HWND hwnd, UINT msg, WPARAM wparam,
                                               LPARAM lparam );

static wp_c8 *wp_runtime_strdup( const wp_c8 *source )
{
    size_t length;
    wp_c8 *copy;

    if( !source )
        return NULL;
    length = strlen( source ) + 1u;
    copy = (wp_c8 *)malloc( length );
    if( copy )
        memcpy( copy, source, length );
    return copy;
}

static wp_runtime *wp_runtime_from_script_object( wp_script_object *object, void *user_data )
{
    wp_runtime *runtime;

    runtime = (wp_runtime *)wp_script_object_get_native( object );
    if( !runtime )
        runtime = (wp_runtime *)user_data;
    return runtime;
}

static void *wp_runtime_script_create( wp_script_state *state, wp_script_object *object,
                                       void *user_data )
{
    (void)state;
    (void)object;
    return user_data;
}

static wp_s32 wp_runtime_script_stop( wp_script_state *state, wp_script_object *self,
                                      const wp_script_value *args, wp_s32 arg_count,
                                      wp_script_value *result, void *user_data )
{
    wp_runtime *runtime;
    (void)args;

    runtime = wp_runtime_from_script_object( self, user_data );
    if( !runtime || arg_count != 0 )
    {
        wp_script_set_error( state, "RuntimeApplication:stop expects no arguments" );
        return 0;
    }
    wp_runtime_stop( runtime );
    *result = wp_script_make_nil();
    return 1;
}

static wp_s32 wp_runtime_script_is_running( wp_script_state *state, wp_script_object *self,
                                            const wp_script_value *args, wp_s32 arg_count,
                                            wp_script_value *result, void *user_data )
{
    wp_runtime *runtime;
    (void)args;

    runtime = wp_runtime_from_script_object( self, user_data );
    if( !runtime || arg_count != 0 )
    {
        wp_script_set_error( state, "RuntimeApplication:isRunning expects no arguments" );
        return 0;
    }
    *result = wp_script_make_number( wp_runtime_is_running( runtime ) ? 1.0 : 0.0 );
    return 1;
}

static wp_s32 wp_runtime_script_get_width( wp_script_state *state, wp_script_object *self,
                                           const wp_script_value *args, wp_s32 arg_count,
                                           wp_script_value *result, void *user_data )
{
    wp_runtime *runtime;
    (void)args;

    runtime = wp_runtime_from_script_object( self, user_data );
    if( !runtime || arg_count != 0 )
    {
        wp_script_set_error( state, "RuntimeApplication:getWidth expects no arguments" );
        return 0;
    }
    *result = wp_script_make_number( (wp_f64)runtime->width );
    return 1;
}

static wp_s32 wp_runtime_script_get_height( wp_script_state *state, wp_script_object *self,
                                            const wp_script_value *args, wp_s32 arg_count,
                                            wp_script_value *result, void *user_data )
{
    wp_runtime *runtime;
    (void)args;

    runtime = wp_runtime_from_script_object( self, user_data );
    if( !runtime || arg_count != 0 )
    {
        wp_script_set_error( state, "RuntimeApplication:getHeight expects no arguments" );
        return 0;
    }
    *result = wp_script_make_number( (wp_f64)runtime->height );
    return 1;
}

static wp_s32 wp_runtime_script_set_background( wp_script_state *state, wp_script_object *self,
                                                const wp_script_value *args, wp_s32 arg_count,
                                                wp_script_value *result, void *user_data )
{
    wp_runtime *runtime;
    wp_s32 i;

    runtime = wp_runtime_from_script_object( self, user_data );
    if( !runtime || arg_count != 4 )
    {
        wp_script_set_error( state,
                             "RuntimeApplication:setBackgroundColor expects four numbers" );
        return 0;
    }
    for( i = 0; i < 4; ++i )
    {
        if( args[i].type != VAL_NUMBER )
        {
            wp_script_set_error( state,
                                 "RuntimeApplication:setBackgroundColor expects four numbers" );
            return 0;
        }
    }
    runtime->bg.r = (wp_f32)args[0].as.number;
    runtime->bg.g = (wp_f32)args[1].as.number;
    runtime->bg.b = (wp_f32)args[2].as.number;
    runtime->bg.a = (wp_f32)args[3].as.number;
    *result = wp_script_make_nil();
    return 1;
}

static wp_s32 wp_runtime_register_script_bindings( wp_runtime *runtime )
{
    wp_script_class *klass;

    if( !runtime || !runtime->script_state )
        return 0;
    klass = wp_script_bind_class( runtime->script_state, "RuntimeApplication", "Object",
                                  wp_runtime_script_create, NULL, runtime );
    if( !klass )
        return 0;
    return wp_script_bind_method( klass, "stop", wp_runtime_script_stop, runtime ) &&
           wp_script_bind_method( klass, "isRunning", wp_runtime_script_is_running, runtime ) &&
           wp_script_bind_method( klass, "getWidth", wp_runtime_script_get_width, runtime ) &&
           wp_script_bind_method( klass, "getHeight", wp_runtime_script_get_height, runtime ) &&
           wp_script_bind_method( klass, "setBackgroundColor",
                                  wp_runtime_script_set_background, runtime );
}

static wp_s32 wp_runtime_create_script_app( wp_runtime *runtime )
{
    wp_script_value app_value;
    wp_script_value result;

    if( !runtime || !runtime->script_state )
        return 0;
    if( runtime->app_object )
        return 1;

    app_value = wp_script_table_get( wp_script_get_globals_table( runtime->script_state ), "app" );
    if( app_value.type == VAL_OBJECT )
        runtime->app_object = app_value.as.object;
    else
    {
        app_value =
            wp_script_table_get( wp_script_get_globals_table( runtime->script_state ),
                                 "Application" );
        if( app_value.type != VAL_CLASS )
            return 1;
        runtime->app_object =
            wp_script_new_object( runtime->script_state, app_value.as.klass );
    }
    if( !runtime->app_object )
        return 0;

    wp_script_object_set_native( runtime->app_object, runtime, NULL, NULL );
    if( wp_script_has_method( runtime->app_object, "__init" ) &&
        !wp_script_call_method_args( runtime->app_object, "__init", NULL, 0, &result ) )
    {
        fprintf( stderr, "Script __init failed: %s\n",
                 wp_script_get_last_error( runtime->script_state ) );
        return 0;
    }
    runtime->app_initialized = 1;
    return 1;
}

static wp_s32 wp_runtime_init_software_ui( wp_runtime *runtime )
{
    const void *pixels;
    struct wp_font *font;
    wp_s32 atlas_width, atlas_height;
    size_t pixel_bytes;
    const size_t draw_vertex_capacity =
        ( MAX_VERTEX_BUFFER / sizeof( wp_runtime_ui_vertex ) ) * sizeof( wp_vertex_ptc );

    runtime->ctx = (struct wp_context *)malloc( sizeof( *runtime->ctx ) );
    runtime->software_ui_vertices = malloc( MAX_VERTEX_BUFFER );
    runtime->software_ui_indices = malloc( MAX_INDEX_BUFFER );
    runtime->software_draw_vertices = (wp_vertex_ptc *)malloc( draw_vertex_capacity );
    if( !runtime->ctx || !runtime->software_ui_vertices || !runtime->software_ui_indices ||
        !runtime->software_draw_vertices )
        return 0;

    memset( runtime->ctx, 0, sizeof( *runtime->ctx ) );
    if( !wp_init_default( runtime->ctx, NULL ) )
        return 0;

    runtime->software_ui_initialized = 1;
    runtime->ctx->clip.copy = wp_renderer_dx11_clipboard_copy;
    runtime->ctx->clip.paste = wp_renderer_dx11_clipboard_paste;
    runtime->ctx->clip.userdata = wp_handle_ptr( runtime );
    wp_buffer_init_default( &runtime->software_ui_commands );

    wp_font_atlas_init_default( &runtime->software_atlas );
    wp_font_atlas_begin( &runtime->software_atlas );
    font = wp_font_atlas_add_default( &runtime->software_atlas, 13.0f, NULL );
    if( !font )
        return 0;

    pixels = wp_font_atlas_bake( &runtime->software_atlas, &atlas_width, &atlas_height,
                                 WORKPHONE_FONT_ATLAS_RGBA32 );
    if( !pixels || atlas_width <= 0 || atlas_height <= 0 ||
        (size_t)atlas_width > SIZE_MAX / ( (size_t)atlas_height * 4u ) )
        return 0;

    pixel_bytes = (size_t)atlas_width * (size_t)atlas_height * 4u;
    runtime->software_font_texture.pixels = (wp_u8 *)malloc( pixel_bytes );
    if( !runtime->software_font_texture.pixels )
        return 0;

    memcpy( runtime->software_font_texture.pixels, pixels, pixel_bytes );
    runtime->software_font_texture.width = atlas_width;
    runtime->software_font_texture.height = atlas_height;
    runtime->software_font_texture.format = WORKPHONE_PIXEL_FORMAT_RGBA8;
    wp_font_atlas_end( &runtime->software_atlas, wp_handle_ptr( &runtime->software_font_texture ),
                       &runtime->software_null_texture );
    wp_style_load_all_cursors( runtime->ctx, runtime->software_atlas.cursors );
    wp_style_set_font( runtime->ctx, &font->handle );
    return 1;
}

static void wp_runtime_render_software_ui( wp_runtime *runtime )
{
    wp_renderer_software *renderer;
    struct wp_buffer vertex_buffer, index_buffer;
    struct wp_convert_config config;
    const struct wp_draw_command *cmd;
    const wp_runtime_ui_vertex *source_vertices;
    const wp_draw_index *indices;
    wp_mat4f old_world, old_view, old_projection, identity, projection;
    wp_blend_mode old_blend;
    wp_fill_mode old_fill;
    wp_cull_mode old_cull;
    wp_s32 old_depth_test, old_depth_write;
    wp_s32 vertex_count, i;
    wp_u32 index_offset = 0;
    const struct wp_draw_vertex_layout_element vertex_layout[] = {
        { WORKPHONE_VERTEX_POSITION, WORKPHONE_FORMAT_FLOAT,
          WORKPHONE_OFFSETOF( wp_runtime_ui_vertex, position ) },
        { WORKPHONE_VERTEX_TEXCOORD, WORKPHONE_FORMAT_FLOAT,
          WORKPHONE_OFFSETOF( wp_runtime_ui_vertex, uv ) },
        { WORKPHONE_VERTEX_COLOR, WORKPHONE_FORMAT_R8G8B8A8,
          WORKPHONE_OFFSETOF( wp_runtime_ui_vertex, color ) },
        { WORKPHONE_VERTEX_LAYOUT_END }
    };

    if( !runtime || !runtime->software_ui_initialized || !runtime->ctx )
        return;

    renderer = wp_renderer_get_software( runtime->renderer );
    if( !renderer )
        return;

    memset( &config, 0, sizeof( config ) );
    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof( wp_runtime_ui_vertex );
    config.global_alpha = 1.0f;
    config.shape_AA = WORKPHONE_ANTI_ALIASING_ON;
    config.line_AA = WORKPHONE_ANTI_ALIASING_ON;
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.tex_null = runtime->software_null_texture;

    wp_buffer_init_fixed( &vertex_buffer, runtime->software_ui_vertices, MAX_VERTEX_BUFFER );
    wp_buffer_init_fixed( &index_buffer, runtime->software_ui_indices, MAX_INDEX_BUFFER );
    if( wp_convert( runtime->ctx, &runtime->software_ui_commands, &vertex_buffer, &index_buffer,
                    &config ) != WORKPHONE_CONVERT_SUCCESS )
        goto cleanup;

    vertex_count = (wp_s32)( vertex_buffer.allocated / sizeof( wp_runtime_ui_vertex ) );
    source_vertices = (const wp_runtime_ui_vertex *)runtime->software_ui_vertices;
    indices = (const wp_draw_index *)runtime->software_ui_indices;
    for( i = 0; i < vertex_count; i++ )
    {
        runtime->software_draw_vertices[i].position.x = source_vertices[i].position[0];
        runtime->software_draw_vertices[i].position.y = source_vertices[i].position[1];
        runtime->software_draw_vertices[i].position.z = 0.0f;
        runtime->software_draw_vertices[i].uv.x = source_vertices[i].uv[0];
        runtime->software_draw_vertices[i].uv.y = source_vertices[i].uv[1];
        runtime->software_draw_vertices[i].color =
            ( (wp_u32)source_vertices[i].color[0] << 24 ) |
            ( (wp_u32)source_vertices[i].color[1] << 16 ) |
            ( (wp_u32)source_vertices[i].color[2] << 8 ) |
            (wp_u32)source_vertices[i].color[3];
    }

    wp_renderer_software_get_world_matrix( renderer, &old_world );
    wp_renderer_software_get_view_matrix( renderer, &old_view );
    wp_renderer_software_get_projection_matrix( renderer, &old_projection );
    old_blend = wp_renderer_software_get_blend_mode( renderer );
    old_fill = wp_renderer_software_get_fill_mode( renderer );
    old_cull = wp_renderer_software_get_cull_mode( renderer );
    old_depth_test = wp_renderer_software_get_depth_test_enabled( renderer );
    old_depth_write = wp_renderer_software_get_depth_write_enabled( renderer );

    wp_mat4f_identity( &identity );
    wp_mat4f_ortho( &projection, 0.0f, (wp_f32)runtime->width, (wp_f32)runtime->height, 0.0f, -1.0f,
                    1.0f );
    wp_renderer_software_set_world_matrix( renderer, &identity );
    wp_renderer_software_set_view_matrix( renderer, &identity );
    wp_renderer_software_set_projection_matrix( renderer, &projection );
    wp_renderer_software_set_blend_mode( renderer, WORKPHONE_BLEND_MODE_ALPHA );
    wp_renderer_software_set_fill_mode( renderer, WORKPHONE_FILL_MODE_SOLID );
    wp_renderer_software_set_cull_mode( renderer, WORKPHONE_CULL_MODE_NONE );
    wp_renderer_software_set_depth_test_enabled( renderer, 0 );
    wp_renderer_software_set_depth_write_enabled( renderer, 0 );
    wp_renderer_software_set_scissor_enabled( renderer, 1 );

    wp_draw_foreach( cmd, runtime->ctx, &runtime->software_ui_commands )
    {
        wp_viewport_i clip;
        const wp_runtime_software_texture *texture;
        if( !cmd->elem_count )
            continue;

        clip.x = (wp_s32)cmd->clip_rect.x;
        clip.y = (wp_s32)cmd->clip_rect.y;
        clip.width = (wp_s32)cmd->clip_rect.w;
        clip.height = (wp_s32)cmd->clip_rect.h;
        wp_renderer_software_set_scissor_rect( renderer, clip );

        texture = (const wp_runtime_software_texture *)cmd->texture.ptr;
        if( texture == &runtime->software_font_texture )
            wp_renderer_software_set_texture( renderer, texture->pixels, texture->width, texture->height,
                                              texture->format );
        else
            wp_renderer_software_set_texture( renderer, NULL, 0, 0, WORKPHONE_PIXEL_FORMAT_RGBA8 );

        wp_renderer_software_draw_indexed_triangles_ptc(
            renderer, runtime->software_draw_vertices, vertex_count,
            (const uint16_t *)( indices + index_offset ), (wp_s32)cmd->elem_count );
        index_offset += cmd->elem_count;
    }

    wp_renderer_software_set_texture( renderer, NULL, 0, 0, WORKPHONE_PIXEL_FORMAT_RGBA8 );
    wp_renderer_software_set_scissor_enabled( renderer, 0 );
    wp_renderer_software_set_world_matrix( renderer, &old_world );
    wp_renderer_software_set_view_matrix( renderer, &old_view );
    wp_renderer_software_set_projection_matrix( renderer, &old_projection );
    wp_renderer_software_set_blend_mode( renderer, old_blend );
    wp_renderer_software_set_fill_mode( renderer, old_fill );
    wp_renderer_software_set_cull_mode( renderer, old_cull );
    wp_renderer_software_set_depth_test_enabled( renderer, old_depth_test );
    wp_renderer_software_set_depth_write_enabled( renderer, old_depth_write );
    wp_renderer_software_render( renderer, WORKPHONE_ANTI_ALIASING_ON );

cleanup:
    wp_clear( runtime->ctx );
    wp_buffer_clear( &runtime->software_ui_commands );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_runtime *wp_runtime_create( wp_s32 width, wp_s32 height, wp_renderer_type type )
{
    wp_runtime *runtime;

    if( width <= 0 || height <= 0 )
        return NULL;
    if( type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        fprintf( stderr, "DX12 renderer is not implemented\n" );
        return NULL;
    }
    if( type != WORKPHONE_RENDERER_TYPE_SOFTWARE && type != WORKPHONE_RENDERER_TYPE_DX11 )
        return NULL;

    runtime = (wp_runtime *)calloc( 1, sizeof( wp_runtime ) );
    if( !runtime )
        return NULL;

    runtime->width = width;
    runtime->height = height;
    runtime->renderer_type = type;
    runtime->bg.r = 0.10f;
    runtime->bg.g = 0.18f;
    runtime->bg.b = 0.24f;
    runtime->bg.a = 1.0f;

    runtime->window =
        wp_platform_window_win32_create( "WorkphoneRuntime", (wp_u32)width, (wp_u32)height );
    if( !runtime->window )
        goto fail;

    if( type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        void *hwnd = wp_platform_window_win32_get_hwnd( runtime->window );
        wp_renderer_dx11 *dx11_renderer;
        struct wp_font_atlas *atlas = NULL;

        runtime->renderer = wp_renderer_create_dx11( hwnd, width, height );
        if( !runtime->renderer )
            goto fail;
        dx11_renderer = wp_renderer_get_dx11( runtime->renderer );
        runtime->ctx =
            wp_renderer_dx11_init( dx11_renderer, width, height, MAX_VERTEX_BUFFER, MAX_INDEX_BUFFER );
        if( !runtime->ctx )
            goto fail;

        wp_renderer_dx11_font_stash_begin( dx11_renderer, &atlas );
        if( !atlas )
            goto fail;
        wp_renderer_dx11_font_stash_end( dx11_renderer );
        if( !atlas->default_font || !atlas->default_font->handle.width ||
            !atlas->default_font->handle.texture.ptr )
            goto fail;
        wp_style_load_all_cursors( runtime->ctx, atlas->cursors );
        if( atlas->default_font )
            wp_style_set_font( runtime->ctx, &atlas->default_font->handle );
    }
    else
    {
        runtime->renderer =
            wp_renderer_create_software( width, height, WORKPHONE_PIXEL_FORMAT_BGRA8 );
        if( !runtime->renderer || !wp_runtime_init_software_ui( runtime ) )
            goto fail;
    }
    runtime->renderer_type = wp_renderer_get_type( runtime->renderer );

    runtime->graphics_system = wp_graphics_system_create();
    if( !runtime->graphics_system )
        goto fail;
    wp_graphics_system_set_renderer( runtime->graphics_system, runtime->renderer );

    runtime->default_scene = wp_graphics_system_create_scene( runtime->graphics_system );
    if( !runtime->default_scene )
        goto fail;
    runtime->default_camera = wp_camera_create();
    if( !runtime->default_camera )
        goto fail;
    wp_camera_set_aspect_ratio( runtime->default_camera, (wp_f32)width / (wp_f32)height );
    wp_graphics_scene_set_default_camera( runtime->default_scene, runtime->default_camera );
    wp_graphics_scene_set_active_camera( runtime->default_scene, runtime->default_camera );

    runtime->thread_pool = wp_threadpool_create( 0 );
    if( !runtime->thread_pool )
        goto fail;

    wp_fsm_mgr_init( &runtime->fsm_mgr );
    runtime->fsm_initialized = 1;
    runtime->script_state = wp_script_create_state();
    if( !runtime->script_state || !wp_runtime_register_script_bindings( runtime ) )
        goto fail;

    wp_platform_window_win32_set_event_callback( runtime->window, wp_runtime_handle_window_event,
                                                  runtime );
    wp_platform_window_win32_show( runtime->window );
    return runtime;

fail:
    wp_runtime_destroy( runtime );
    return NULL;
}

static wp_s32 wp_runtime_handle_window_event( void *userdata, HWND hwnd, UINT msg, WPARAM wparam,
                                               LPARAM lparam )
{
    wp_runtime *runtime = (wp_runtime *)userdata;
    struct wp_context *ctx;

    if( !runtime || !( ctx = runtime->ctx ) )
        return 0;
    if( msg == WM_KEYDOWN && wparam == VK_ESCAPE )
    {
        wp_runtime_stop( runtime );
        return 1;
    }
    if( msg == WM_KEYDOWN && wparam == VK_F5 && !( lparam & ( 1L << 30 ) ) )
    {
        if( !wp_runtime_reload_scripts( runtime ) )
            fprintf( stderr, "Script reload failed; keeping the previous VM\n" );
        return 1;
    }
    if( runtime->renderer_type == WORKPHONE_RENDERER_TYPE_DX11 )
        return wp_renderer_dx11_handle_event( wp_renderer_get_dx11( runtime->renderer ), hwnd, msg,
                                               wparam, lparam );

    switch( msg )
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        wp_s32 down = !( ( lparam >> 31 ) & 1 );
        wp_s32 ctrl = ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
        switch( wparam )
        {
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            wp_input_key( ctx, WORKPHONE_KEY_SHIFT, down );
            return 1;
        case VK_DELETE:
            wp_input_key( ctx, WORKPHONE_KEY_DEL, down );
            return 1;
        case VK_RETURN:
            wp_input_key( ctx, WORKPHONE_KEY_ENTER, down );
            return 1;
        case VK_TAB:
            wp_input_key( ctx, WORKPHONE_KEY_TAB, down );
            return 1;
        case VK_UP:
            wp_input_key( ctx, WORKPHONE_KEY_UP, down );
            return 1;
        case VK_DOWN:
            wp_input_key( ctx, WORKPHONE_KEY_DOWN, down );
            return 1;
        case VK_LEFT:
            wp_input_key( ctx, ctrl ? WORKPHONE_KEY_TEXT_WORD_LEFT : WORKPHONE_KEY_LEFT, down );
            return 1;
        case VK_RIGHT:
            wp_input_key( ctx, ctrl ? WORKPHONE_KEY_TEXT_WORD_RIGHT : WORKPHONE_KEY_RIGHT, down );
            return 1;
        case VK_BACK:
            wp_input_key( ctx, WORKPHONE_KEY_BACKSPACE, down );
            return 1;
        case VK_HOME:
            wp_input_key( ctx, WORKPHONE_KEY_TEXT_START, down );
            wp_input_key( ctx, WORKPHONE_KEY_SCROLL_START, down );
            return 1;
        case VK_END:
            wp_input_key( ctx, WORKPHONE_KEY_TEXT_END, down );
            wp_input_key( ctx, WORKPHONE_KEY_SCROLL_END, down );
            return 1;
        case VK_NEXT:
            wp_input_key( ctx, WORKPHONE_KEY_SCROLL_DOWN, down );
            return 1;
        case VK_PRIOR:
            wp_input_key( ctx, WORKPHONE_KEY_SCROLL_UP, down );
            return 1;
        case 'A':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_TEXT_SELECT_ALL, down );
            return ctrl;
        case 'C':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_COPY, down );
            return ctrl;
        case 'V':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_PASTE, down );
            return ctrl;
        case 'X':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_CUT, down );
            return ctrl;
        case 'Z':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_TEXT_UNDO, down );
            return ctrl;
        case 'R':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_TEXT_REDO, down );
            return ctrl;
        default:
            return 0;
        }
    }
    case WM_CHAR:
        if( wparam >= 32 )
        {
            wp_input_unicode( ctx, (wp_rune)wparam );
            return 1;
        }
        return 0;
    case WM_LBUTTONDOWN:
        wp_input_button( ctx, WORKPHONE_BUTTON_LEFT, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 1 );
        SetCapture( hwnd );
        return 1;
    case WM_LBUTTONUP:
        wp_input_button( ctx, WORKPHONE_BUTTON_LEFT, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 0 );
        wp_input_button( ctx, WORKPHONE_BUTTON_DOUBLE, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 0 );
        ReleaseCapture();
        return 1;
    case WM_RBUTTONDOWN:
        wp_input_button( ctx, WORKPHONE_BUTTON_RIGHT, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 1 );
        SetCapture( hwnd );
        return 1;
    case WM_RBUTTONUP:
        wp_input_button( ctx, WORKPHONE_BUTTON_RIGHT, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 0 );
        ReleaseCapture();
        return 1;
    case WM_MBUTTONDOWN:
        wp_input_button( ctx, WORKPHONE_BUTTON_MIDDLE, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 1 );
        SetCapture( hwnd );
        return 1;
    case WM_MBUTTONUP:
        wp_input_button( ctx, WORKPHONE_BUTTON_MIDDLE, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 0 );
        ReleaseCapture();
        return 1;
    case WM_MOUSEWHEEL:
        wp_input_scroll( ctx, wp_make_vec2f( 0.0f, (wp_f32)(short)HIWORD( wparam ) / WHEEL_DELTA ) );
        return 1;
    case WM_MOUSEMOVE:
        wp_input_motion( ctx, (short)LOWORD( lparam ), (short)HIWORD( lparam ) );
        return 1;
    case WM_LBUTTONDBLCLK:
        wp_input_button( ctx, WORKPHONE_BUTTON_DOUBLE, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 1 );
        return 1;
    default:
        return 0;
    }
}

void wp_runtime_destroy( wp_runtime *runtime )
{
    wp_script_value result;

    if( !runtime )
        return;

    if( runtime->window )
        wp_platform_window_win32_set_event_callback( runtime->window, NULL, NULL );

    if( runtime->app_object && runtime->app_initialized &&
        wp_script_has_method( runtime->app_object, "__finalize" ) )
    {
        if( !wp_script_call_method_args( runtime->app_object, "__finalize", NULL, 0, &result ) )
            fprintf( stderr, "Script __finalize failed: %s\n",
                     wp_script_get_last_error( runtime->script_state ) );
    }
    runtime->app_object = NULL;
    runtime->app_initialized = 0;

    if( runtime->fsm_initialized )
        wp_fsm_mgr_destroy( &runtime->fsm_mgr );

    if( runtime->script_state )
    {
        wp_script_destroy_state( runtime->script_state );
        runtime->script_state = NULL;
    }

    if( runtime->thread_pool )
    {
        wp_threadpool_wait( runtime->thread_pool );
        wp_threadpool_destroy( runtime->thread_pool );
    }

    if( runtime->default_camera )
        wp_camera_destroy( runtime->default_camera );

    if( runtime->graphics_system )
        wp_graphics_system_destroy( runtime->graphics_system );

    if( runtime->software_ui_initialized )
    {
        wp_font_atlas_clear( &runtime->software_atlas );
        wp_buffer_free( &runtime->software_ui_commands );
        wp_free( runtime->ctx );
    }
    if( runtime->renderer_type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
        free( runtime->ctx );
    free( runtime->software_font_texture.pixels );
    free( runtime->software_ui_vertices );
    free( runtime->software_draw_vertices );
    free( runtime->software_ui_indices );

    if( runtime->renderer )
        wp_renderer_destroy( runtime->renderer );

    if( runtime->window )
        wp_platform_window_win32_destroy( runtime->window );

    free( runtime->project_path );
    free( runtime );
}

/* =========================================================================
 * Main loop
 * ====================================================================== */

void wp_runtime_run( wp_runtime *runtime )
{
    LARGE_INTEGER frequency, previous, current;

    if( !runtime )
        return;

    if( runtime->project_path && !runtime->scripts_loaded &&
        wp_runtime_load_scripts( runtime, runtime->project_path ) < 0 )
        return;
    runtime->running = 1;
    if( !wp_runtime_create_script_app( runtime ) )
    {
        runtime->running = 0;
        return;
    }
    QueryPerformanceFrequency( &frequency );
    QueryPerformanceCounter( &previous );

    while( runtime->running && !wp_platform_window_win32_is_closed( runtime->window ) )
    {
        wp_f64 dt;
        wp_u32 window_width, window_height;

        wp_input_begin( runtime->ctx );
        wp_platform_window_win32_pump_messages( runtime->window );
        wp_input_end( runtime->ctx );

        if( wp_platform_window_win32_is_closed( runtime->window ) )
            break;

        window_width = wp_platform_window_win32_get_width( runtime->window );
        window_height = wp_platform_window_win32_get_height( runtime->window );
        if( window_width > 0 && window_height > 0 &&
            ( (wp_s32)window_width != runtime->width || (wp_s32)window_height != runtime->height ) )
        {
            if( wp_renderer_resize( runtime->renderer, (wp_s32)window_width, (wp_s32)window_height ) )
            {
                runtime->width = (wp_s32)window_width;
                runtime->height = (wp_s32)window_height;
                if( runtime->default_camera && window_height > 0 )
                {
                    wp_camera_set_aspect_ratio( runtime->default_camera,
                                                (wp_f32)window_width /
                                                    (wp_f32)window_height );
                }
            }
        }

        QueryPerformanceCounter( &current );
        dt = frequency.QuadPart > 0
                 ? (wp_f64)( current.QuadPart - previous.QuadPart ) / (wp_f64)frequency.QuadPart
                 : 1.0 / 60.0;
        previous = current;
        if( dt < 0.0 || dt > 0.25 )
            dt = 1.0 / 60.0;
        runtime->ctx->delta_time_seconds = (wp_f32)dt;
        wp_runtime_update( runtime, dt );
        wp_runtime_render( runtime );
    }

    runtime->running = 0;
}

void wp_runtime_stop( wp_runtime *runtime )
{
    if( runtime )
        runtime->running = 0;
}

wp_s32 wp_runtime_is_running( const wp_runtime *runtime )
{
    return runtime ? runtime->running : 0;
}

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

#ifdef _WIN32
static void wp_runtime_software_blit( wp_runtime *runtime )
{
    HWND hwnd;
    HDC hdc;
    const void *pixels;
    BITMAPINFO bmi;

    pixels = wp_renderer_get_framebuffer( runtime->renderer );
    if( !pixels )
        return;

    hwnd = (HWND)wp_platform_window_win32_get_hwnd( runtime->window );
    if( !hwnd )
        return;

    memset( &bmi, 0, sizeof( bmi ) );
    bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    bmi.bmiHeader.biWidth = runtime->width;
    bmi.bmiHeader.biHeight = -runtime->height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hdc = GetDC( hwnd );
    if( !hdc )
        return;
    StretchDIBits( hdc, 0, 0, runtime->width, runtime->height, 0, 0, runtime->width, runtime->height,
                   pixels, &bmi, DIB_RGB_COLORS, SRCCOPY );
    ReleaseDC( hwnd, hdc );
}
#endif

/* =========================================================================
 * Per-frame
 * ====================================================================== */

void wp_runtime_update( wp_runtime *runtime, wp_f64 dt )
{
    wp_script_value argument;
    wp_script_value result;

    if( !runtime )
        return;

    wp_fsm_mgr_update( &runtime->fsm_mgr, dt );
    if( runtime->app_object && wp_script_has_method( runtime->app_object, "update" ) )
    {
        argument = wp_script_make_number( dt );
        if( !wp_script_call_method_args( runtime->app_object, "update", &argument, 1, &result ) )
        {
            fprintf( stderr, "Script update failed: %s\n",
                     wp_script_get_last_error( runtime->script_state ) );
            wp_runtime_stop( runtime );
        }
    }
    wp_graphics_system_update( runtime->graphics_system );
}

void wp_runtime_render( wp_runtime *runtime )
{
    if( !runtime )
        return;

    wp_renderer_begin_frame( runtime->renderer );
    wp_renderer_set_clear_color( runtime->renderer, runtime->bg.r, runtime->bg.g, runtime->bg.b,
                                 runtime->bg.a );
    wp_renderer_clear( runtime->renderer, WORKPHONE_CLEAR_FLAG_ALL );
    wp_graphics_system_render( runtime->graphics_system );

    if( runtime->renderer_type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_runtime_render_software_ui( runtime );
    }
    else if( runtime->renderer_type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11 *dx11_renderer = wp_renderer_get_dx11( runtime->renderer );
        wp_renderer_dx11_render( dx11_renderer, WORKPHONE_ANTI_ALIASING_ON );
    }

    wp_renderer_end_frame( runtime->renderer );

#ifdef _WIN32
    if( runtime->renderer_type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
        wp_runtime_software_blit( runtime );
#endif
}

/* =========================================================================
 * Accessors
 * ====================================================================== */

wp_renderer *wp_runtime_get_renderer( const wp_runtime *runtime )
{
    return runtime ? runtime->renderer : NULL;
}

wp_graphics_system *wp_runtime_get_graphics_system( const wp_runtime *runtime )
{
    return runtime ? runtime->graphics_system : NULL;
}

wp_threadpool *wp_runtime_get_thread_pool( const wp_runtime *runtime )
{
    return runtime ? runtime->thread_pool : NULL;
}

wp_s32 wp_runtime_get_width( const wp_runtime *runtime )
{
    return runtime ? runtime->width : 0;
}

wp_s32 wp_runtime_get_height( const wp_runtime *runtime )
{
    return runtime ? runtime->height : 0;
}

wp_platform_window_win32 *wp_runtime_get_window( const wp_runtime *runtime )
{
    return runtime ? runtime->window : NULL;
}

wp_script_state *wp_runtime_get_script_state( const wp_runtime *runtime )
{
    return runtime ? runtime->script_state : NULL;
}

wp_s32 wp_runtime_set_project_path( wp_runtime *runtime, const wp_c8 *path )
{
    wp_c8 *copy;

    if( !runtime || !path )
        return 0;
    copy = wp_runtime_strdup( path );
    if( !copy )
        return 0;
    free( runtime->project_path );
    runtime->project_path = copy;
    runtime->scripts_loaded = 0;
    return 1;
}

const wp_c8 *wp_runtime_get_project_path( const wp_runtime *runtime )
{
    return runtime && runtime->project_path ? runtime->project_path : "";
}

typedef struct wp_runtime_path_list
{
    wp_c8 **paths;
    wp_s32 count;
    wp_s32 capacity;
} wp_runtime_path_list;

static void wp_runtime_path_list_clear( wp_runtime_path_list *list )
{
    wp_s32 i;

    if( !list )
        return;
    for( i = 0; i < list->count; ++i )
        free( list->paths[i] );
    free( list->paths );
    memset( list, 0, sizeof( *list ) );
}

static wp_s32 wp_runtime_path_list_add( wp_runtime_path_list *list, const wp_c8 *path )
{
    wp_c8 **new_paths;
    wp_s32 new_capacity;
    wp_c8 *copy;

    if( !list || !path )
        return 0;
    if( list->count >= list->capacity )
    {
        new_capacity = list->capacity ? list->capacity * 2 : 16;
        new_paths = (wp_c8 **)realloc( list->paths,
                                      (size_t)new_capacity * sizeof( wp_c8 * ) );
        if( !new_paths )
            return 0;
        list->paths = new_paths;
        list->capacity = new_capacity;
    }
    copy = wp_runtime_strdup( path );
    if( !copy )
        return 0;
    list->paths[list->count++] = copy;
    return 1;
}

static wp_s32 wp_runtime_is_script_path( const wp_c8 *path )
{
    const wp_c8 *extension;

    extension = path ? strrchr( path, '.' ) : NULL;
    return extension &&
           ( _stricmp( extension, ".meow" ) == 0 ||
             _stricmp( extension, ".lua" ) == 0 );
}

static wp_s32 wp_runtime_collect_scripts( const wp_c8 *path, wp_runtime_path_list *list )
{
    DWORD attributes;
    wp_c8 search_path[MAX_PATH];
    wp_c8 child_path[MAX_PATH];
    size_t path_length;
    HANDLE find_handle;
    WIN32_FIND_DATAA find_data;
    wp_s32 ok;

    if( !path || !list )
        return 0;
    attributes = GetFileAttributesA( path );
    if( attributes == INVALID_FILE_ATTRIBUTES )
        return 0;
    if( !( attributes & FILE_ATTRIBUTE_DIRECTORY ) )
    {
        wp_c8 parent_path[MAX_PATH];
        wp_c8 *separator;

        if( wp_runtime_is_script_path( path ) )
            return wp_runtime_path_list_add( list, path );
        path_length = strlen( path );
        if( path_length >= sizeof( parent_path ) )
            return 0;
        memcpy( parent_path, path, path_length + 1u );
        separator = strrchr( parent_path, '\\' );
        if( !separator )
            separator = strrchr( parent_path, '/' );
        if( separator )
        {
            if( separator == parent_path )
                separator[1] = '\0';
            else
                *separator = '\0';
        }
        else
        {
            strcpy( parent_path, "." );
        }
        return wp_runtime_collect_scripts( parent_path, list );
    }

    path_length = strlen( path );
    if( path_length + 3u >= sizeof( search_path ) )
        return 0;
    memcpy( search_path, path, path_length );
    if( path_length > 0 && path[path_length - 1] != '\\' && path[path_length - 1] != '/' )
        search_path[path_length++] = '\\';
    search_path[path_length++] = '*';
    search_path[path_length] = '\0';

    find_handle = FindFirstFileA( search_path, &find_data );
    if( find_handle == INVALID_HANDLE_VALUE )
        return 0;
    ok = 1;
    do
    {
        size_t name_length;

        if( strcmp( find_data.cFileName, "." ) == 0 ||
            strcmp( find_data.cFileName, ".." ) == 0 )
            continue;
        name_length = strlen( find_data.cFileName );
        path_length = strlen( path );
        if( path_length + name_length + 2u >= sizeof( child_path ) )
        {
            ok = 0;
            break;
        }
        memcpy( child_path, path, path_length );
        if( path_length > 0 && path[path_length - 1] != '\\' &&
            path[path_length - 1] != '/' )
            child_path[path_length++] = '\\';
        memcpy( &child_path[path_length], find_data.cFileName, name_length + 1u );

        if( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            if( !( find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) &&
                !wp_runtime_collect_scripts( child_path, list ) )
            {
                ok = 0;
                break;
            }
        }
        else if( wp_runtime_is_script_path( child_path ) &&
                 !wp_runtime_path_list_add( list, child_path ) )
        {
            ok = 0;
            break;
        }
    } while( FindNextFileA( find_handle, &find_data ) );
    FindClose( find_handle );
    return ok;
}

static int wp_runtime_compare_script_paths( const void *left, const void *right )
{
    const wp_c8 *left_path = *(const wp_c8 *const *)left;
    const wp_c8 *right_path = *(const wp_c8 *const *)right;
    return _stricmp( left_path, right_path );
}

wp_s32 wp_runtime_load_script( wp_runtime *runtime, const wp_c8 *path )
{
    FILE *file;
    long size;
    wp_c8 *buffer;
    size_t bytes_read;
    wp_s32 offset;
    wp_script_value result;

    if( !runtime || !path )
        return 0;

    file = fopen( path, "rb" );
    if( !file )
    {
        printf( "Failed to open script: %s\n", path );
        return 0;
    }

    if( fseek( file, 0, SEEK_END ) != 0 )
    {
        fclose( file );
        return 0;
    }
    size = ftell( file );
    if( size < 0 || fseek( file, 0, SEEK_SET ) != 0 )
    {
        fclose( file );
        return 0;
    }

    buffer = (wp_c8 *)malloc( (size_t)size + 1 );
    if( !buffer )
    {
        fclose( file );
        return 0;
    }

    bytes_read = fread( buffer, 1, (size_t)size, file );
    fclose( file );

    if( bytes_read != (size_t)size )
    {
        free( buffer );
        return 0;
    }

    buffer[bytes_read] = '\0';

    if( !wp_script_compile_ex( runtime->script_state, buffer ) )
    {
        fprintf( stderr, "Failed to compile script '%s': %s\n", path,
                 wp_script_get_last_error( runtime->script_state ) );
        free( buffer );
        return 0;
    }
    offset = wp_script_get_compile_offset( runtime->script_state );
    if( !wp_script_run_from_ex( runtime->script_state, offset, &result ) )
    {
        fprintf( stderr, "Failed to execute script '%s': %s\n", path,
                 wp_script_get_last_error( runtime->script_state ) );
        free( buffer );
        return 0;
    }

    free( buffer );
    runtime->scripts_loaded++;
    return 1;
}

wp_s32 wp_runtime_load_scripts( wp_runtime *runtime, const wp_c8 *path )
{
    wp_runtime_path_list list;
    wp_s32 i;
    wp_s32 loaded;

    if( !runtime || !path )
        return -1;

    memset( &list, 0, sizeof( list ) );

    if( !wp_runtime_set_project_path( runtime, path ) ||
        !wp_runtime_collect_scripts( runtime->project_path, &list ) )
    {
        fprintf( stderr, "Failed to discover scripts at: %s\n",
                 runtime->project_path ? runtime->project_path : "" );
        wp_runtime_path_list_clear( &list );
        return -1;
    }
    if( list.count > 1 )
    {
        qsort( list.paths, (size_t)list.count, sizeof( wp_c8 * ),
               wp_runtime_compare_script_paths );
    }
    loaded = 0;
    for( i = 0; i < list.count; ++i )
    {
        if( !wp_runtime_load_script( runtime, list.paths[i] ) )
        {
            wp_runtime_path_list_clear( &list );
            return -1;
        }
        loaded++;
    }
    wp_runtime_path_list_clear( &list );
    return loaded;
}

wp_s32 wp_runtime_reload_scripts( wp_runtime *runtime )
{
    wp_script_state *old_state;
    wp_script_object *old_app;
    wp_s32 old_scripts_loaded;
    wp_s32 old_app_initialized;
    wp_script_state *new_state;
    wp_script_value result;
    wp_s32 loaded;

    if( !runtime || !runtime->project_path || !runtime->project_path[0] )
        return 0;
    new_state = wp_script_create_state();
    if( !new_state )
        return 0;

    old_state = runtime->script_state;
    old_app = runtime->app_object;
    old_scripts_loaded = runtime->scripts_loaded;
    old_app_initialized = runtime->app_initialized;
    runtime->script_state = new_state;
    runtime->app_object = NULL;
    runtime->scripts_loaded = 0;
    runtime->app_initialized = 0;

    if( !wp_runtime_register_script_bindings( runtime ) )
        loaded = -1;
    else
        loaded = wp_runtime_load_scripts( runtime, runtime->project_path );
    if( loaded < 0 || !wp_runtime_create_script_app( runtime ) )
    {
        wp_script_destroy_state( new_state );
        runtime->script_state = old_state;
        runtime->app_object = old_app;
        runtime->scripts_loaded = old_scripts_loaded;
        runtime->app_initialized = old_app_initialized;
        return 0;
    }

    if( old_app && old_app_initialized && wp_script_has_method( old_app, "__finalize" ) )
    {
        if( !wp_script_call_method_args( old_app, "__finalize", NULL, 0, &result ) )
            fprintf( stderr, "Previous script __finalize failed: %s\n",
                     wp_script_get_last_error( old_state ) );
    }
    wp_script_destroy_state( old_state );
    return 1;
}

int main( int argc, char **argv )
{
    wp_runtime *runtime;
    wp_renderer_type type;
    const wp_c8 *project_path;
    wp_s32 i;
    wp_s32 loaded;

#ifdef WP_MEDIA_PATH
    project_path = WP_MEDIA_PATH "/Scripts/MS";
#else
    project_path = "Media/Scripts/MS";
#endif

    type = WORKPHONE_RENDERER_TYPE_DX11;

    for( i = 1; i < argc; ++i )
    {
        if( strcmp( argv[i], "--software" ) == 0 )
            type = WORKPHONE_RENDERER_TYPE_SOFTWARE;
        else if( strcmp( argv[i], "--dx11" ) == 0 )
            type = WORKPHONE_RENDERER_TYPE_DX11;
        else if( strcmp( argv[i], "--help" ) == 0 || strcmp( argv[i], "-h" ) == 0 )
        {
            printf( "Usage: WorkphoneRuntime [project-path] [--software|--dx11]\n" );
            return 0;
        }
        else
            project_path = (const wp_c8 *)argv[i];
    }

    runtime = wp_runtime_create( WINDOW_WIDTH, WINDOW_HEIGHT, type );
    if( !runtime )
        return 1;

    loaded = wp_runtime_load_scripts( runtime, project_path );

    if( loaded < 0 )
    {
        wp_runtime_destroy( runtime );
        return 1;
    }

    printf( "Loaded %d script(s) from %s\n", loaded, project_path );

    wp_runtime_run( runtime );
    wp_runtime_destroy( runtime );

    return 0;
}
