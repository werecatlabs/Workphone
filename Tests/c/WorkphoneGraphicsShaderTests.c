#include "workphone_graphics_renderer.h"
#include "workphone_graphics_shader.h"
#include "workphone_graphics_material.h"
#include <stdio.h>
#include <string.h>

static int g_compile_success;
static int g_link_success;
static int g_compile_calls;
static int g_link_calls;
static int g_bind_calls;
static int g_release_calls;
static int g_reset_calls;
static int g_shader_native;
static int g_program_native;

static int check( int condition, const char *message )
{
    if( condition ) return 1;
    fprintf( stderr, "shader test failed: %s\n", message );
    return 0;
}

static wp_s32 compile_shader( wp_shader *shader, void **candidate, void *user_data )
{
    (void)shader;
    (void)user_data;
    ++g_compile_calls;
    if( !g_compile_success ) return 0;
    *candidate = &g_shader_native;
    return 1;
}

static wp_s32 link_program( wp_shader_program *program, void **candidate, void *user_data )
{
    (void)program;
    (void)user_data;
    ++g_link_calls;
    if( !g_link_success ) return 0;
    *candidate = &g_program_native;
    return 1;
}

static wp_s32 bind_program( wp_shader_program *program, wp_renderer *renderer, void *native,
                            void *user_data )
{
    (void)program;
    (void)renderer;
    (void)user_data;
    if( native != &g_program_native ) return 0;
    ++g_bind_calls;
    return 1;
}

static void release_native( void *native, void *user_data )
{
    (void)native;
    (void)user_data;
    ++g_release_calls;
}

static wp_s32 reset_program( wp_renderer *renderer, void *user_data )
{
    (void)renderer;
    (void)user_data;
    ++g_reset_calls;
    return 1;
}

int main( void )
{
    wp_shader *vertex;
    wp_shader_program *program;
    wp_shader_program *other_program;
    wp_shader_program *invalid_program;
    wp_shader *unbacked;
    wp_shader *invalid_pixel;
    wp_graphics_material *material;
    wp_graphics_material *default_material;
    wp_renderer *renderer;
    wp_shader_param_handle stale_handle;
    wp_shader_param_handle valid_handle;
    const wp_shader_param_def *def;
    const wp_byte *buffer;
    const void *binary_data;
    wp_byte binary_input[4];
    wp_u32 binary_size;
    wp_u32 revision;
    wp_u32 pipeline_revision;
    wp_u32 parameter_revision;
    wp_u32 mask;
    wp_s32 index;
    wp_s32 i;
    wp_f32 value;
    int ok;

    ok = 1;
    unbacked = wp_shader_create( WORKPHONE_SHADER_TYPE_VERTEX, WORKPHONE_SHADER_LANGUAGE_HLSL,
                                 "void main(){}", "main" );
    ok &= check( unbacked != NULL && wp_shader_compile( unbacked ) == 0 &&
                     wp_shader_get_status( unbacked ) == WORKPHONE_SHADER_STATUS_FAILED &&
                     wp_shader_get_last_error( unbacked )[0] != '\0',
                 "a missing renderer compiler must be an explicit diagnostic failure" );
    binary_input[0] = 3u;
    binary_input[1] = 2u;
    binary_input[2] = 1u;
    binary_input[3] = 0u;
    wp_shader_set_language( unbacked, WORKPHONE_SHADER_LANGUAGE_SPIRV );
    ok &= check( wp_shader_set_binary( unbacked, binary_input, 4u ) != 0 &&
                     wp_shader_get_source( unbacked )[0] == '\0',
                 "precompiled binary input must replace source atomically" );
    binary_data = wp_shader_get_binary( unbacked, &binary_size );
    ok &= check( binary_size == 4u && binary_data != NULL &&
                     memcmp( binary_data, binary_input, 4u ) == 0,
                 "binary shader bytes must round-trip without text assumptions" );
    wp_shader_set_native( unbacked, &g_shader_native );
    ok &= check( wp_shader_get_status( unbacked ) == WORKPHONE_SHADER_STATUS_COMPILED &&
                     !wp_shader_is_dirty( unbacked ),
                 "injecting a native shader must commit its lifecycle state" );
    wp_shader_set_native( unbacked, NULL );
    ok &= check( wp_shader_is_dirty( unbacked ) &&
                     wp_shader_get_status( unbacked ) == WORKPHONE_SHADER_STATUS_SOURCE_READY,
                 "clearing a native shader must schedule recompilation" );
    wp_shader_destroy( unbacked );
    invalid_program = wp_shader_program_create();
    invalid_pixel = wp_shader_create( WORKPHONE_SHADER_TYPE_PIXEL, WORKPHONE_SHADER_LANGUAGE_HLSL,
                                      "void main(){}", "main" );
    wp_shader_program_attach( invalid_program, invalid_pixel );
    wp_shader_program_set_native( invalid_program, &g_program_native );
    wp_shader_program_set_native( invalid_program, NULL );
    ok &= check( wp_shader_program_is_dirty( invalid_program ) &&
                     wp_shader_program_get_status( invalid_program ) ==
                         WORKPHONE_SHADER_STATUS_SOURCE_READY,
                 "clearing a native program must schedule relinking" );
    ok &= check( wp_shader_program_update( invalid_program ) == 0 &&
                     wp_shader_program_get_status( invalid_program ) ==
                         WORKPHONE_SHADER_STATUS_FAILED,
                 "raster topology must reject a fragment-only program" );
    wp_shader_destroy( invalid_pixel );
    wp_shader_program_destroy( invalid_program );
    ok &= check( wp_shader_create( (wp_shader_type)99, WORKPHONE_SHADER_LANGUAGE_HLSL,
                                   "void main(){}", "main" ) == NULL,
                 "invalid stages must be rejected" );
    vertex = wp_shader_create( WORKPHONE_SHADER_TYPE_VERTEX, WORKPHONE_SHADER_LANGUAGE_HLSL,
                               "void main(){}", "main" );
    ok &= check( vertex != NULL, "valid shader creation" );
    if( !vertex ) return 1;

    revision = wp_shader_get_revision( vertex );
    wp_shader_set_entry_point( vertex, "main" );
    ok &= check( wp_shader_get_revision( vertex ) == revision,
                 "unchanged shader metadata must not invalidate compilation" );
    wp_shader_set_compile_func( vertex, compile_shader, NULL );
    wp_shader_set_release_func( vertex, release_native, NULL );
    ok &= check( wp_shader_set_define( vertex, "QUALITY", "ULTRA" ) != 0 &&
                     wp_shader_get_define_count( vertex ) == 1 &&
                     wp_shader_set_specialization( vertex, "LIGHT_COUNT", 8u ) != 0 &&
                     wp_shader_get_specialization_count( vertex ) == 1,
                 "compile-affecting defines and specialization constants must live in C89" );
    g_compile_success = 1;
    ok &= check( wp_shader_compile( vertex ) != 0 &&
                     wp_shader_get_status( vertex ) == WORKPHONE_SHADER_STATUS_COMPILED &&
                     wp_shader_get_native( vertex ) == &g_shader_native,
                 "successful compilation must commit native state" );
    wp_shader_set_source( vertex, "void main(){ int changed; }" );
    g_compile_success = 0;
    ok &= check( wp_shader_compile( vertex ) == 0 &&
                     wp_shader_get_status( vertex ) == WORKPHONE_SHADER_STATUS_FAILED &&
                     wp_shader_get_native( vertex ) == &g_shader_native && g_release_calls == 0,
                 "failed hot reload must preserve last-known-good native state" );
    g_compile_success = 1;
    wp_shader_mark_dirty( vertex );

    program = wp_shader_program_create();
    renderer = wp_renderer_create_software( 8, 8, WORKPHONE_PIXEL_FORMAT_BGRA8 );
    material = wp_graphics_material_create();
    default_material = wp_graphics_material_create();
    ok &= check( program != NULL && renderer != NULL && material != NULL &&
                     default_material != NULL,
                 "program, material and renderer creation" );
    if( !program || !renderer || !material || !default_material ) return 1;
    wp_renderer_set_program_reset_func( renderer, reset_program, NULL );
    wp_shader_program_attach( program, vertex );
    wp_shader_set_type( vertex, WORKPHONE_SHADER_TYPE_PIXEL );
    ok &= check( wp_shader_program_get_shader( program, WORKPHONE_SHADER_TYPE_VERTEX ) == NULL &&
                     wp_shader_program_get_shader( program, WORKPHONE_SHADER_TYPE_PIXEL ) == vertex,
                 "stage lookup must follow a safely retained shader after stage mutation" );
    wp_shader_set_type( vertex, WORKPHONE_SHADER_TYPE_VERTEX );
    ok &= check( wp_shader_program_get_shader( program, WORKPHONE_SHADER_TYPE_VERTEX ) == vertex,
                 "restoring a stage must preserve valid program topology" );
    wp_shader_destroy( vertex );
    index = wp_shader_program_register_uniform( program, "scalar", WORKPHONE_UNIFORM_TYPE_FLOAT );
    stale_handle = wp_shader_program_find_param( program, "scalar" );
    ok &= check( index == 0, "first parameter index" );
    index = wp_shader_program_register_uniform( program, "direction", WORKPHONE_UNIFORM_TYPE_VEC3 );
    ok &= check( index == 1, "second parameter index" );
    index = wp_shader_program_register_uniform( program, "matrix", WORKPHONE_UNIFORM_TYPE_MAT4 );
    ok &= check( index == 2 && wp_shader_program_get_param_buffer_size( program ) == 96u,
                 "constant data must be tightly packed independently of allocation capacity" );
    def = wp_shader_program_get_param_def( program, 1 );
    ok &= check( def && def->byte_offset == 16u, "second parameter offset must be 16 bytes" );
    def = wp_shader_program_get_param_def( program, 2 );
    ok &= check( def && def->byte_offset == 32u, "matrix parameter offset must be 32 bytes" );

    buffer = (const wp_byte *)wp_shader_program_get_param_buffer( program );
    value = 3.5f;
    wp_shader_program_set_float_h( program, stale_handle, value );
    ok &= check( buffer && buffer[0] == 0u,
                 "stale handles must not write after the parameter layout changes" );
    valid_handle = wp_shader_program_find_param( program, "scalar" );
    wp_shader_program_set_float_h( program, valid_handle, value );
    ok &= check( memcmp( buffer, &value, sizeof( value ) ) == 0,
                  "current typed handles must update canonical parameter data" );
    other_program = wp_shader_program_create();
    wp_shader_program_register_uniform( other_program, "scalar", WORKPHONE_UNIFORM_TYPE_FLOAT );
    wp_shader_program_set_float_h( other_program, valid_handle, 9.0f );
    value = 0.0f;
    ok &= check( memcmp( wp_shader_program_get_param_buffer( other_program ), &value,
                         sizeof( value ) ) == 0,
                 "parameter handles must never cross program ownership boundaries" );
    wp_shader_program_destroy( other_program );
    value = 3.5f;
    wp_shader_program_set_mat4( program, 0, NULL );
    ok &= check( memcmp( buffer, &value, sizeof( value ) ) == 0,
                 "wrong typed setters must not corrupt adjacent parameters" );

    for( i = 3; i < 40; ++i )
    {
        wp_c8 name[32];
        sprintf( name, "parameter_%d", (int)i );
        wp_shader_program_register_uniform( program, name, WORKPHONE_UNIFORM_TYPE_FLOAT );
    }
    mask = wp_shader_program_get_param_dirty_mask_word(
        program, WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT, 1u );
    ok &= check( mask == 0xffu, "second dirty-mask word must represent parameters 32 through 39" );

    wp_shader_program_set_link_func( program, link_program, NULL );
    wp_shader_program_set_bind_func( program, bind_program, NULL );
    wp_shader_program_set_release_func( program, release_native, NULL );
    wp_graphics_material_set_shader_program( material, program );
    g_link_success = 1;
    ok &= check( wp_graphics_material_apply( material, renderer ) != 0 &&
                     g_link_calls == 1 && g_bind_calls == 1,
                 "material rendering must automatically compile, link and bind its program" );
    value = 8.0f;
    pipeline_revision = wp_shader_program_get_pipeline_revision( program );
    parameter_revision = wp_shader_program_get_parameter_revision( program );
    wp_shader_program_set_float( program, 0, value );
    ok &= check( wp_shader_program_get_pipeline_revision( program ) == pipeline_revision &&
                     wp_shader_program_get_parameter_revision( program ) != parameter_revision,
                 "constant writes must advance uploads without invalidating the pipeline" );
    ok &= check( wp_graphics_material_apply( material, renderer ) != 0 && g_link_calls == 1 &&
                      g_bind_calls == 2,
                  "uniform changes must upload without relinking the program" );
    revision = wp_shader_program_get_revision( program );
    wp_shader_program_set_float( program, 0, value );
    ok &= check( wp_shader_program_get_revision( program ) == revision,
                 "unchanged constants must not schedule redundant uploads" );
    g_link_success = 0;
    wp_shader_program_set_permutation( program, WP_SHADER_PERMUTATION_ALPHA_TEST, 1 );
    ok &= check( wp_graphics_material_apply( material, renderer ) != 0 && g_link_calls == 2 &&
                     g_bind_calls == 3,
                 "a failed hot link must keep rendering with the last-known-good program" );
    ok &= check( wp_shader_program_get_status( program ) == WORKPHONE_SHADER_STATUS_FAILED &&
                     wp_shader_program_get_last_error( program )[0] != '\0',
                 "stale-but-usable programs must retain actionable link diagnostics" );
    value = 9.0f;
    wp_shader_program_set_float( program, 0, value );
    ok &= check( wp_graphics_material_apply( material, renderer ) != 0 && g_link_calls == 2 &&
                     g_bind_calls == 4,
                 "the same failed revision must be latched instead of retried every draw" );
    ok &= check( wp_graphics_material_apply( default_material, renderer ) != 0 &&
                     g_reset_calls == 1 && !wp_renderer_has_program_bound( renderer ),
                 "an unprogrammed material must explicitly restore the default pipeline" );

    wp_shader_program_destroy( program );
    wp_graphics_material_destroy( material );
    wp_graphics_material_destroy( default_material );
    wp_renderer_destroy( renderer );
    ok &= check( g_release_calls == 2, "owned shader and program native handles must be released" );
    if( !ok ) return 1;
    puts( "Workphone graphics shader contract tests passed." );
    return 0;
}
