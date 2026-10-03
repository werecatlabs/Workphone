/**
 * @file wp_graphics_shader.c
 * @brief Implementation of the C graphics shader API.
 *
 * Data-driven shader parameter system integrating:
 *   - Esoterica engine parameter info / handle / annotation / permutation model
 *   - Ogre3D auto-parameter semantic and constant-definition model
 *
 * The implementation is C89 conforming: declarations precede statements in
 * every block, C89 block comments only, no designated initialisers.
 */

#include "workphone_graphics_shader.h"
#include "workphone_graphics_renderer.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

#define WP_SHADER_CBUFFER_ALIGNMENT 16u
#define WP_SHADER_TEXT( text ) ( (const wp_c8 *)(const void *)( text ) )

static wp_u32 wp_uniform_type_size_bytes( wp_uniform_type type )
{
    switch( type )
    {
    case WORKPHONE_UNIFORM_TYPE_INT:
        return 4u;
    case WORKPHONE_UNIFORM_TYPE_FLOAT:
        return 4u;
    case WORKPHONE_UNIFORM_TYPE_VEC2:
        return 8u;
    case WORKPHONE_UNIFORM_TYPE_VEC3:
        return 12u;
    case WORKPHONE_UNIFORM_TYPE_VEC4:
        return 16u;
    case WORKPHONE_UNIFORM_TYPE_MAT4:
        return 64u;
    default:
        return 0u;
    }
}

static wp_u32 wp_align_up( wp_u32 value, wp_u32 alignment )
{
    if( alignment == 0u )
    {
        return value;
    }
    return ( value + alignment - 1u ) & ~( alignment - 1u );
}

static void wp_copy_cstr( wp_c8 *dst, wp_u32 dst_size, const wp_c8 *src )
{
    wp_u32 i;
    if( dst_size == 0u )
    {
        return;
    }
    if( src == NULL )
    {
        dst[0] = '\0';
        return;
    }
    i = 0u;
    while( i + 1u < dst_size && src[i] != '\0' )
    {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static wp_size wp_text_length( const wp_c8 *text )
{
    wp_size length;
    length = 0u;
    if( text == NULL )
        return 0u;
    while( text[length] != '\0' )
        ++length;
    return length;
}

static wp_s32 wp_text_equal( const wp_c8 *a, const wp_c8 *b )
{
    if( a == NULL || b == NULL )
        return a == b;
    while( *a != '\0' && *a == *b )
    {
        ++a;
        ++b;
    }
    return *a == *b;
}

/* =========================================================================
 * Internal shader object
 * ====================================================================== */

typedef struct wp_shader
{
    wp_u32 reference_count;
    wp_shader_type type;
    wp_shader_language language;
    wp_c8 *source;   /* heap-allocated source text    */
    wp_byte *binary; /* optional precompiled bytecode */
    wp_u32 binary_size;
    wp_c8 entry_point[WP_SHADER_MAX_ENTRY];
    wp_c8 profile[WP_SHADER_MAX_PROFILE];
    wp_shader_define defines[WP_SHADER_MAX_DEFINES];
    wp_u32 define_count;
    wp_shader_specialization specializations[WP_SHADER_MAX_SPECIALIZATIONS];
    wp_u32 specialization_count;
    wp_s32 is_dirty;
    wp_u32 revision;
    wp_u32 attempted_revision;
    wp_shader_status status;
    wp_c8 last_error[WP_SHADER_MAX_NAME];
    void *native;
    wp_shader_compile_func compile_func;
    void *compile_user_data;
    wp_shader_release_func release_func;
    void *release_user_data;
} wp_shader;

/* =========================================================================
 * Internal shader program
 * ====================================================================== */

typedef struct wp_shader_program
{
    wp_u32 reference_count;
    wp_shader *shaders[WORKPHONE_SHADER_TYPE_COUNT];
    wp_u32 shader_revisions[WORKPHONE_SHADER_TYPE_COUNT];

    /* Data-driven parameter definition table (Esoterica ParameterInfo +
       Ogre GpuConstantDefinition). */
    wp_shader_param_def param_defs[WP_SHADER_MAX_PARAMS];
    wp_u32 param_count;

    /* Flat constant-buffer-style backing store. Grown on demand. */
    wp_byte *param_buffer;
    wp_u32 param_buffer_size;
    wp_u32 param_buffer_capacity;

    /* Native resource bindings (texture / buffer / sampler), parallel to
       param_defs and indexed by the same index. */
    void *resource_values[WP_SHADER_MAX_PARAMS];

    /* Named options exposed to the client (Esoterica annotations). */
    wp_shader_option options[WP_SHADER_MAX_OPTIONS];
    wp_u32 option_count;

    /* Permutation selection mask (Esoterica MaterialShader::Permutation). */
    wp_u32 permutation_mask;

    wp_s32 is_dirty;
    wp_s32 link_dirty;
    wp_s32 link_failed;
    wp_u32 revision;
    wp_u32 pipeline_revision;
    wp_u32 parameter_revision;
    wp_u32 linked_generation;
    wp_shader_status status;
    wp_c8 last_error[WP_SHADER_MAX_NAME];
    wp_u32 link_attempt_revision;
    wp_u32 definition_revision;
    void *native;
    wp_shader_program_link_func link_func;
    void *link_user_data;
    wp_shader_program_bind_func bind_func;
    void *bind_user_data;
    wp_shader_release_func release_func;
    void *release_user_data;
} wp_shader_program;

static void wp_shader_touch( wp_shader *shader )
{
    ++shader->revision;
    if( shader->revision == 0u )
        shader->revision = 1u;
    shader->is_dirty = 1;
    shader->status = ( shader->source && shader->source[0] ) || shader->binary_size > 0u
                         ? WORKPHONE_SHADER_STATUS_SOURCE_READY
                         : WORKPHONE_SHADER_STATUS_EMPTY;
    shader->last_error[0] = '\0';
}

static wp_s32 wp_shader_type_valid( wp_shader_type type )
{
    return type >= WORKPHONE_SHADER_TYPE_VERTEX && type < WORKPHONE_SHADER_TYPE_COUNT;
}

static wp_s32 wp_shader_language_valid( wp_shader_language language )
{
    return language >= WORKPHONE_SHADER_LANGUAGE_HLSL && language <= WORKPHONE_SHADER_LANGUAGE_UNKNOWN;
}

static wp_s32 wp_shader_find_define( const wp_shader *shader, const wp_c8 *name )
{
    wp_u32 i;
    if( shader == NULL || name == NULL )
        return -1;
    for( i = 0u; i < shader->define_count; ++i )
        if( wp_text_equal( shader->defines[i].name, name ) )
            return (wp_s32)i;
    return -1;
}

static wp_s32 wp_shader_find_specialization( const wp_shader *shader, const wp_c8 *name )
{
    wp_u32 i;
    if( shader == NULL || name == NULL )
        return -1;
    for( i = 0u; i < shader->specialization_count; ++i )
        if( wp_text_equal( shader->specializations[i].name, name ) )
            return (wp_s32)i;
    return -1;
}

static void wp_shader_retain( wp_shader *shader )
{
    if( shader != NULL && shader->reference_count < (wp_u32)UINT_MAX )
        ++shader->reference_count;
}

static void wp_shader_program_touch( wp_shader_program *program )
{
    ++program->revision;
    if( program->revision == 0u )
        program->revision = 1u;
    ++program->pipeline_revision;
    if( program->pipeline_revision == 0u )
        program->pipeline_revision = 1u;
    program->is_dirty = 1;
    program->link_dirty = 1;
    program->status = WORKPHONE_SHADER_STATUS_SOURCE_READY;
    program->link_failed = 0;
    program->last_error[0] = '\0';
}

/* ---- internal parameter buffer management -------------------------------- */

static wp_s32 wp_shader_program_ensure_buffer( wp_shader_program *program, wp_u32 needed )
{
    wp_byte *new_buffer;
    wp_u32 new_size;

    if( program->param_buffer_capacity >= needed )
    {
        return 1;
    }

    new_size = program->param_buffer_capacity == 0u ? 256u : program->param_buffer_capacity;
    while( new_size < needed )
    {
        if( new_size > (wp_u32)UINT_MAX / 2u )
        {
            new_size = needed;
            break;
        }
        new_size *= 2u;
    }

    new_buffer = (wp_byte *)realloc( program->param_buffer, new_size );
    if( new_buffer == NULL )
    {
        return 0;
    }

    /* Zero the newly grown region so unset parameters are deterministic. */
    memset( new_buffer + program->param_buffer_capacity, 0, new_size - program->param_buffer_capacity );

    program->param_buffer = new_buffer;
    program->param_buffer_capacity = new_size;
    return 1;
}

static wp_s32 wp_shader_program_find_index( const wp_shader_program *program, const wp_c8 *name )
{
    wp_u32 i;
    if( program == NULL || name == NULL )
    {
        return -1;
    }
    for( i = 0u; i < program->param_count; ++i )
    {
        if( wp_text_equal( program->param_defs[i].name, name ) )
        {
            return (wp_s32)i;
        }
    }
    return -1;
}

static wp_s32 wp_shader_program_register_param_internal( wp_shader_program *program, const wp_c8 *name,
                                                         wp_uniform_type type, wp_u32 array_size,
                                                         wp_u32 variability,
                                                         wp_shader_auto_param auto_param,
                                                         wp_shader_resource_kind resource_kind )
{
    wp_shader_param_def *def;
    wp_u32 element_bytes;
    wp_u32 slot_bytes;
    wp_u32 offset;

    if( program == NULL || name == NULL || name[0] == '\0' ||
        wp_text_length( name ) >= WP_SHADER_MAX_NAME )
    {
        return -1;
    }
    if( program->param_count >= WP_SHADER_MAX_PARAMS )
    {
        return -1;
    }
    if( wp_shader_program_find_index( program, name ) >= 0 )
    {
        return -1;
    }
    if( array_size == 0u )
    {
        array_size = 1u;
    }

    element_bytes = wp_uniform_type_size_bytes( type );
    if( element_bytes == 0u )
    {
        return -1;
    }

    /* Ogre-style float4 (16-byte) packing: each array element occupies a
       whole 16-byte slot, scalars are packed to their natural size but the
       slot is 16-byte aligned. */
    if( array_size > (wp_u32)UINT_MAX / wp_align_up( element_bytes, WP_SHADER_CBUFFER_ALIGNMENT ) )
    {
        return -1;
    }
    slot_bytes = wp_align_up( element_bytes, WP_SHADER_CBUFFER_ALIGNMENT ) * array_size;
    if( program->param_buffer_size > (wp_u32)UINT_MAX - ( WP_SHADER_CBUFFER_ALIGNMENT - 1u ) )
    {
        return -1;
    }
    offset = wp_align_up( program->param_buffer_size, WP_SHADER_CBUFFER_ALIGNMENT );
    if( offset > (wp_u32)UINT_MAX - slot_bytes )
    {
        return -1;
    }

    if( !wp_shader_program_ensure_buffer( program, offset + slot_bytes ) )
    {
        return -1;
    }

    def = &program->param_defs[program->param_count];
    memset( def, 0, sizeof( *def ) );
    wp_copy_cstr( def->name, WP_SHADER_MAX_NAME, name );
    def->type = type;
    def->element_size = wp_align_up( element_bytes, WP_SHADER_CBUFFER_ALIGNMENT );
    def->array_size = array_size;
    def->byte_offset = offset;
    def->byte_size = slot_bytes;
    def->variability = variability;
    def->auto_param = auto_param;
    def->resource_kind = resource_kind;
    def->is_dirty = 1;

    program->resource_values[program->param_count] = NULL;
    program->param_count++;
    program->param_buffer_size = offset + slot_bytes;
    ++program->parameter_revision;
    if( program->parameter_revision == 0u )
        program->parameter_revision = 1u;
    ++program->definition_revision;
    if( program->definition_revision == 0u )
        program->definition_revision = 1u;
    wp_shader_program_touch( program );
    return (wp_s32)( program->param_count - 1u );
}

static wp_s32 wp_shader_program_write_bytes( wp_shader_program *program, wp_u32 offset, const void *data,
                                             wp_u32 byte_count )
{
    if( program == NULL || data == NULL || byte_count == 0u )
    {
        return 0;
    }
    if( offset > program->param_buffer_size || byte_count > program->param_buffer_size - offset )
    {
        return 0;
    }
    if( memcmp( program->param_buffer + offset, data, byte_count ) == 0 )
        return 0;
    memcpy( program->param_buffer + offset, data, byte_count );
    return 1;
}

static wp_s32 wp_shader_program_valid_typed_index( const wp_shader_program *program, wp_s32 index,
                                                   wp_uniform_type type )
{
    return program != NULL && index >= 0 && (wp_u32)index < program->param_count &&
           program->param_defs[index].resource_kind == WORKPHONE_SHADER_RESOURCE_NONE &&
           program->param_defs[index].type == type;
}

static wp_s32 wp_shader_program_handle_index( const wp_shader_program *program,
                                              wp_shader_param_handle handle, wp_uniform_type type )
{
    const wp_shader_param_def *def;
    if( program == NULL || handle.element_size == 0u || handle.owner != program ||
        handle.definition_revision != program->definition_revision ||
        handle.parameter_index >= program->param_count )
        return -1;
    def = &program->param_defs[handle.parameter_index];
    if( def->type != type || def->resource_kind != WORKPHONE_SHADER_RESOURCE_NONE ||
        def->byte_offset != handle.byte_offset || def->element_size != handle.element_size )
        return -1;
    return (wp_s32)handle.parameter_index;
}

static void wp_shader_program_dirty_param( wp_shader_program *program, wp_s32 index )
{
    program->param_defs[index].is_dirty = 1;
    ++program->revision;
    if( program->revision == 0u )
        program->revision = 1u;
    ++program->parameter_revision;
    if( program->parameter_revision == 0u )
        program->parameter_revision = 1u;
    program->is_dirty = 1;
}

static wp_s32 wp_shader_program_topology_valid( const wp_shader_program *program )
{
    wp_s32 i;
    wp_s32 has_raster;
    wp_s32 has_ray;
    wp_s32 has_any;
    has_raster = 0;
    has_ray = 0;
    has_any = 0;
    for( i = 0; i < WORKPHONE_SHADER_TYPE_COUNT; ++i )
    {
        if( program->shaders[i] == NULL )
            continue;
        has_any = 1;
        if( i <= WORKPHONE_SHADER_TYPE_DOMAIN )
            has_raster = 1;
        if( i >= WORKPHONE_SHADER_TYPE_RAY_GENERATION )
            has_ray = 1;
    }
    if( !has_any )
        return 0;
    if( has_raster && program->shaders[WORKPHONE_SHADER_TYPE_VERTEX] == NULL )
        return 0;
    if( program->shaders[WORKPHONE_SHADER_TYPE_COMPUTE] != NULL && ( has_raster || has_ray ) )
        return 0;
    if( has_ray && has_raster )
        return 0;
    if( has_ray && program->shaders[WORKPHONE_SHADER_TYPE_RAY_GENERATION] == NULL )
        return 0;
    if( ( program->shaders[WORKPHONE_SHADER_TYPE_HULL] == NULL ) !=
        ( program->shaders[WORKPHONE_SHADER_TYPE_DOMAIN] == NULL ) )
        return 0;
    return 1;
}

/* =========================================================================
 * Shader object lifecycle
 * ====================================================================== */

wp_shader *wp_shader_create( wp_shader_type type, wp_shader_language language, const wp_c8 *source,
                             const wp_c8 *entry_point )
{
    wp_shader *shader;
    wp_size len;
    if( !wp_shader_type_valid( type ) || !wp_shader_language_valid( language ) )
    {
        return NULL;
    }
    shader = (wp_shader *)malloc( sizeof( wp_shader ) );
    if( shader == NULL )
    {
        return NULL;
    }
    memset( shader, 0, sizeof( wp_shader ) );

    shader->type = type;
    shader->language = language;
    shader->reference_count = 1u;
    shader->is_dirty = 1;
    shader->revision = 1u;

    if( source != NULL )
    {
        len = wp_text_length( source );
        if( len == (wp_size)-1 )
        {
            free( shader );
            return NULL;
        }
        shader->source = (wp_c8 *)malloc( (wp_size)len + 1u );
        if( shader->source == NULL )
        {
            free( shader );
            return NULL;
        }
        memcpy( shader->source, source, len + 1u );
    }

    wp_copy_cstr( shader->entry_point, WP_SHADER_MAX_ENTRY,
                  entry_point ? entry_point : WP_SHADER_TEXT( "main" ) );
    shader->status = shader->source && shader->source[0] ? WORKPHONE_SHADER_STATUS_SOURCE_READY
                                                         : WORKPHONE_SHADER_STATUS_EMPTY;

    return shader;
}

void wp_shader_destroy( wp_shader *shader )
{
    if( shader == NULL )
    {
        return;
    }
    if( shader->reference_count > 1u )
    {
        --shader->reference_count;
        return;
    }
    shader->reference_count = 0u;
    if( shader->source != NULL )
    {
        free( shader->source );
        shader->source = NULL;
    }
    if( shader->binary != NULL )
    {
        free( shader->binary );
        shader->binary = NULL;
    }
    if( shader->native != NULL && shader->release_func != NULL )
    {
        shader->release_func( shader->native, shader->release_user_data );
    }
    free( shader );
}

/* =========================================================================
 * Shader object accessors
 * ====================================================================== */

wp_shader_type wp_shader_get_type( const wp_shader *shader )
{
    if( shader == NULL )
    {
        return WORKPHONE_SHADER_TYPE_VERTEX;
    }
    return shader->type;
}

void wp_shader_set_type( wp_shader *shader, wp_shader_type type )
{
    if( shader != NULL && wp_shader_type_valid( type ) && shader->type != type )
    {
        shader->type = type;
        wp_shader_touch( shader );
    }
}

wp_shader_language wp_shader_get_language( const wp_shader *shader )
{
    if( shader == NULL )
    {
        return WORKPHONE_SHADER_LANGUAGE_HLSL;
    }
    return shader->language;
}

void wp_shader_set_language( wp_shader *shader, wp_shader_language language )
{
    if( shader != NULL && wp_shader_language_valid( language ) && shader->language != language )
    {
        shader->language = language;
        wp_shader_touch( shader );
    }
}

const wp_c8 *wp_shader_get_source( const wp_shader *shader )
{
    if( shader == NULL )
    {
        return WP_SHADER_TEXT( "" );
    }
    return shader->source ? shader->source : WP_SHADER_TEXT( "" );
}

wp_s32 wp_shader_set_source( wp_shader *shader, const wp_c8 *source )
{
    wp_c8 *new_source;
    wp_size len;

    if( shader == NULL )
    {
        return 0;
    }
    if( source == NULL )
    {
        if( shader->source == NULL )
            return 1;
        free( shader->source );
        shader->source = NULL;
        wp_shader_touch( shader );
        return 1;
    }

    if( shader->source != NULL && wp_text_equal( shader->source, source ) )
    {
        return 1;
    }
    len = wp_text_length( source );
    if( len == (wp_size)-1 )
        return 0;
    new_source = (wp_c8 *)malloc( (wp_size)len + 1u );
    if( new_source == NULL )
    {
        return 0;
    }
    memcpy( new_source, source, len + 1u );

    if( shader->source != NULL )
    {
        free( shader->source );
    }
    shader->source = new_source;
    if( shader->binary != NULL )
    {
        free( shader->binary );
        shader->binary = NULL;
        shader->binary_size = 0u;
    }
    wp_shader_touch( shader );
    return 1;
}

const void *wp_shader_get_binary( const wp_shader *shader, wp_u32 *byte_count )
{
    if( byte_count != NULL )
        *byte_count = shader ? shader->binary_size : 0u;
    return shader ? shader->binary : NULL;
}

wp_s32 wp_shader_set_binary( wp_shader *shader, const void *binary, wp_u32 byte_count )
{
    wp_byte *new_binary;
    if( shader == NULL )
        return 0;
    if( ( binary == NULL && byte_count != 0u ) || ( binary != NULL && byte_count == 0u ) )
        return 0;
    if( binary == NULL )
    {
        if( shader->binary == NULL )
            return 1;
        free( shader->binary );
        shader->binary = NULL;
        shader->binary_size = 0u;
        wp_shader_touch( shader );
        return 1;
    }
    if( shader->language == WORKPHONE_SHADER_LANGUAGE_UNKNOWN )
        return 0;
    if( shader->binary_size == byte_count && shader->binary != NULL &&
        memcmp( shader->binary, binary, byte_count ) == 0 )
        return 1;
    new_binary = (wp_byte *)malloc( byte_count );
    if( new_binary == NULL )
        return 0;
    memcpy( new_binary, binary, byte_count );
    if( shader->binary != NULL )
        free( shader->binary );
    shader->binary = new_binary;
    shader->binary_size = byte_count;
    if( shader->source != NULL )
    {
        free( shader->source );
        shader->source = NULL;
    }
    wp_shader_touch( shader );
    shader->status = WORKPHONE_SHADER_STATUS_COMPILED;
    return 1;
}

const wp_c8 *wp_shader_get_entry_point( const wp_shader *shader )
{
    if( shader == NULL )
    {
        return WP_SHADER_TEXT( "" );
    }
    return shader->entry_point;
}

void wp_shader_set_entry_point( wp_shader *shader, const wp_c8 *entry_point )
{
    wp_c8 clean[WP_SHADER_MAX_ENTRY];
    if( shader == NULL )
    {
        return;
    }
    if( entry_point == NULL )
        entry_point = WP_SHADER_TEXT( "main" );
    wp_copy_cstr( clean, WP_SHADER_MAX_ENTRY, entry_point );
    if( !wp_text_equal( shader->entry_point, clean ) )
    {
        wp_copy_cstr( shader->entry_point, WP_SHADER_MAX_ENTRY, clean );
        wp_shader_touch( shader );
    }
}

const wp_c8 *wp_shader_get_profile( const wp_shader *shader )
{
    if( shader == NULL )
    {
        return WP_SHADER_TEXT( "" );
    }
    return shader->profile;
}

void wp_shader_set_profile( wp_shader *shader, const wp_c8 *profile )
{
    wp_c8 clean[WP_SHADER_MAX_PROFILE];
    if( shader == NULL )
    {
        return;
    }
    if( profile == NULL )
        profile = WP_SHADER_TEXT( "" );
    wp_copy_cstr( clean, WP_SHADER_MAX_PROFILE, profile );
    if( !wp_text_equal( shader->profile, clean ) )
    {
        wp_copy_cstr( shader->profile, WP_SHADER_MAX_PROFILE, clean );
        wp_shader_touch( shader );
    }
}

wp_s32 wp_shader_get_define_count( const wp_shader *shader )
{
    return shader ? (wp_s32)shader->define_count : 0;
}

const wp_shader_define *wp_shader_get_define_at( const wp_shader *shader, wp_s32 index )
{
    if( shader == NULL || index < 0 || (wp_u32)index >= shader->define_count )
        return NULL;
    return &shader->defines[index];
}

wp_s32 wp_shader_set_define( wp_shader *shader, const wp_c8 *name, const wp_c8 *value )
{
    wp_s32 index;
    if( shader == NULL || name == NULL || name[0] == '\0' ||
        wp_text_length( name ) >= WP_SHADER_MAX_NAME ||
        ( value != NULL && wp_text_length( value ) >= WP_SHADER_MAX_NAME ) )
        return 0;
    index = wp_shader_find_define( shader, name );
    if( index >= 0 )
    {
        if( wp_text_equal( shader->defines[index].value, value ? value : WP_SHADER_TEXT( "" ) ) )
            return 1;
        wp_copy_cstr( shader->defines[index].value, WP_SHADER_MAX_NAME,
                      value ? value : WP_SHADER_TEXT( "" ) );
        wp_shader_touch( shader );
        return 1;
    }
    if( shader->define_count >= WP_SHADER_MAX_DEFINES )
        return 0;
    wp_copy_cstr( shader->defines[shader->define_count].name, WP_SHADER_MAX_NAME, name );
    wp_copy_cstr( shader->defines[shader->define_count].value, WP_SHADER_MAX_NAME,
                  value ? value : WP_SHADER_TEXT( "" ) );
    ++shader->define_count;
    wp_shader_touch( shader );
    return 1;
}

wp_s32 wp_shader_remove_define( wp_shader *shader, const wp_c8 *name )
{
    wp_s32 index;
    wp_u32 i;
    index = wp_shader_find_define( shader, name );
    if( index < 0 )
        return 0;
    for( i = (wp_u32)index + 1u; i < shader->define_count; ++i )
        shader->defines[i - 1u] = shader->defines[i];
    --shader->define_count;
    memset( &shader->defines[shader->define_count], 0, sizeof( shader->defines[shader->define_count] ) );
    wp_shader_touch( shader );
    return 1;
}

void wp_shader_clear_defines( wp_shader *shader )
{
    if( shader == NULL || shader->define_count == 0u )
        return;
    memset( shader->defines, 0, sizeof( shader->defines ) );
    shader->define_count = 0u;
    wp_shader_touch( shader );
}

wp_s32 wp_shader_get_specialization_count( const wp_shader *shader )
{
    return shader ? (wp_s32)shader->specialization_count : 0;
}

const wp_shader_specialization *wp_shader_get_specialization_at( const wp_shader *shader, wp_s32 index )
{
    if( shader == NULL || index < 0 || (wp_u32)index >= shader->specialization_count )
        return NULL;
    return &shader->specializations[index];
}

wp_s32 wp_shader_set_specialization( wp_shader *shader, const wp_c8 *name, wp_u32 value )
{
    wp_s32 index;
    if( shader == NULL || name == NULL || name[0] == '\0' ||
        wp_text_length( name ) >= WP_SHADER_MAX_NAME )
        return 0;
    index = wp_shader_find_specialization( shader, name );
    if( index >= 0 )
    {
        if( shader->specializations[index].value == value )
            return 1;
        shader->specializations[index].value = value;
        wp_shader_touch( shader );
        return 1;
    }
    if( shader->specialization_count >= WP_SHADER_MAX_SPECIALIZATIONS )
        return 0;
    wp_copy_cstr( shader->specializations[shader->specialization_count].name, WP_SHADER_MAX_NAME, name );
    shader->specializations[shader->specialization_count].value = value;
    ++shader->specialization_count;
    wp_shader_touch( shader );
    return 1;
}

wp_s32 wp_shader_is_dirty( const wp_shader *shader )
{
    if( shader == NULL )
    {
        return 0;
    }
    return shader->is_dirty;
}

void wp_shader_mark_dirty( wp_shader *shader )
{
    if( shader == NULL )
    {
        return;
    }
    wp_shader_touch( shader );
}

void wp_shader_clear_dirty( wp_shader *shader )
{
    if( shader == NULL )
    {
        return;
    }
    if( shader->status == WORKPHONE_SHADER_STATUS_COMPILED && shader->native != NULL )
        shader->is_dirty = 0;
}

wp_u32 wp_shader_get_revision( const wp_shader *shader )
{
    return shader ? shader->revision : 0u;
}

wp_shader_status wp_shader_get_status( const wp_shader *shader )
{
    return shader ? shader->status : WORKPHONE_SHADER_STATUS_EMPTY;
}

const wp_c8 *wp_shader_get_last_error( const wp_shader *shader )
{
    return shader ? shader->last_error : WP_SHADER_TEXT( "" );
}

void wp_shader_set_last_error( wp_shader *shader, const wp_c8 *message )
{
    if( shader == NULL )
        return;
    wp_copy_cstr( shader->last_error, WP_SHADER_MAX_NAME, message ? message : WP_SHADER_TEXT( "" ) );
}

void wp_shader_set_compile_func( wp_shader *shader, wp_shader_compile_func func, void *user_data )
{
    if( shader == NULL )
        return;
    if( shader->compile_func != func || shader->compile_user_data != user_data )
    {
        shader->compile_func = func;
        shader->compile_user_data = func ? user_data : NULL;
        wp_shader_touch( shader );
    }
}

void wp_shader_set_release_func( wp_shader *shader, wp_shader_release_func func, void *user_data )
{
    if( shader == NULL )
        return;
    if( shader->native != NULL &&
        ( shader->release_func != func || shader->release_user_data != user_data ) )
        return;
    shader->release_func = func;
    shader->release_user_data = func ? user_data : NULL;
}

wp_s32 wp_shader_compile( wp_shader *shader )
{
    wp_u32 revision;
    void *candidate;
    if( shader == NULL )
        return 0;
    revision = shader->revision;
    if( shader->status == WORKPHONE_SHADER_STATUS_FAILED && shader->attempted_revision == revision )
        return 0;
    shader->attempted_revision = revision;
    shader->last_error[0] = '\0';
    if( ( shader->source == NULL || shader->source[0] == '\0' ) && shader->binary_size == 0u )
    {
        wp_copy_cstr( shader->last_error, WP_SHADER_MAX_NAME,
                      WP_SHADER_TEXT( "Shader has no source or binary." ) );
        shader->status = WORKPHONE_SHADER_STATUS_FAILED;
        return 0;
    }
    if( shader->language == WORKPHONE_SHADER_LANGUAGE_UNKNOWN )
    {
        wp_copy_cstr( shader->last_error, WP_SHADER_MAX_NAME,
                      WP_SHADER_TEXT( "Shader language is unknown." ) );
        shader->status = WORKPHONE_SHADER_STATUS_FAILED;
        return 0;
    }
    if( shader->entry_point[0] == '\0' )
    {
        wp_copy_cstr( shader->last_error, WP_SHADER_MAX_NAME,
                      WP_SHADER_TEXT( "Shader entry point is empty." ) );
        shader->status = WORKPHONE_SHADER_STATUS_FAILED;
        return 0;
    }
    if( shader->compile_func == NULL )
    {
        wp_copy_cstr( shader->last_error, WP_SHADER_MAX_NAME,
                      WP_SHADER_TEXT( "No renderer shader compiler is installed." ) );
        shader->status = WORKPHONE_SHADER_STATUS_FAILED;
        return 0;
    }
    shader->status = WORKPHONE_SHADER_STATUS_COMPILING;
    candidate = NULL;
    if( !shader->compile_func( shader, &candidate, shader->compile_user_data ) )
    {
        if( candidate != NULL && candidate != shader->native && shader->release_func )
            shader->release_func( candidate, shader->release_user_data );
        if( shader->last_error[0] == '\0' )
            wp_copy_cstr( shader->last_error, WP_SHADER_MAX_NAME,
                          WP_SHADER_TEXT( "Shader compilation callback failed." ) );
        shader->status = WORKPHONE_SHADER_STATUS_FAILED;
        return 0;
    }
    if( candidate == NULL )
    {
        wp_copy_cstr( shader->last_error, WP_SHADER_MAX_NAME,
                      WP_SHADER_TEXT( "Shader compiler returned no native object." ) );
        shader->status = WORKPHONE_SHADER_STATUS_FAILED;
        return 0;
    }
    if( shader->revision != revision )
    {
        if( candidate != shader->native && shader->release_func )
            shader->release_func( candidate, shader->release_user_data );
        wp_copy_cstr( shader->last_error, WP_SHADER_MAX_NAME,
                      WP_SHADER_TEXT( "Shader changed while compilation was in progress." ) );
        shader->status = WORKPHONE_SHADER_STATUS_SOURCE_READY;
        return 0;
    }
    if( shader->native != candidate )
    {
        if( shader->native && shader->release_func )
            shader->release_func( shader->native, shader->release_user_data );
        shader->native = candidate;
    }
    shader->status = WORKPHONE_SHADER_STATUS_COMPILED;
    shader->is_dirty = 0;
    return 1;
}

void *wp_shader_get_native( const wp_shader *shader )
{
    if( shader == NULL )
    {
        return NULL;
    }
    return shader->native;
}

void wp_shader_set_native( wp_shader *shader, void *native )
{
    if( shader == NULL )
    {
        return;
    }
    if( shader->native != native )
    {
        if( shader->native && shader->release_func )
            shader->release_func( shader->native, shader->release_user_data );
        shader->native = native;
        if( native != NULL )
        {
            shader->status = WORKPHONE_SHADER_STATUS_COMPILED;
            shader->is_dirty = 0;
            shader->last_error[0] = '\0';
            shader->attempted_revision = shader->revision;
        }
        else
        {
            wp_shader_touch( shader );
        }
    }
}

/* =========================================================================
 * Shader program lifecycle
 * ====================================================================== */

wp_shader_program *wp_shader_program_create( void )
{
    wp_shader_program *program;
    program = (wp_shader_program *)malloc( sizeof( wp_shader_program ) );
    if( program == NULL )
    {
        return NULL;
    }
    memset( program, 0, sizeof( wp_shader_program ) );
    program->reference_count = 1u;
    program->is_dirty = 1;
    program->link_dirty = 1;
    program->revision = 1u;
    program->pipeline_revision = 1u;
    program->parameter_revision = 1u;
    program->definition_revision = 1u;
    program->status = WORKPHONE_SHADER_STATUS_EMPTY;
    return program;
}

void wp_shader_program_retain( wp_shader_program *program )
{
    if( program != NULL && program->reference_count < (wp_u32)UINT_MAX )
        ++program->reference_count;
}

void wp_shader_program_destroy( wp_shader_program *program )
{
    wp_s32 i;
    if( program == NULL )
    {
        return;
    }
    if( program->reference_count > 1u )
    {
        --program->reference_count;
        return;
    }
    program->reference_count = 0u;
    if( program->param_buffer != NULL )
    {
        free( program->param_buffer );
        program->param_buffer = NULL;
    }
    if( program->native != NULL && program->release_func != NULL )
    {
        program->release_func( program->native, program->release_user_data );
    }
    for( i = 0; i < WORKPHONE_SHADER_TYPE_COUNT; ++i )
    {
        if( program->shaders[i] != NULL )
            wp_shader_destroy( program->shaders[i] );
    }
    free( program );
}

/* =========================================================================
 * Stage attachment
 * ====================================================================== */

void wp_shader_program_attach( wp_shader_program *program, wp_shader *shader )
{
    wp_s32 old_slot;
    wp_shader *replaced;
    if( program == NULL || shader == NULL )
    {
        return;
    }
    if( (int)shader->type < 0 || shader->type >= WORKPHONE_SHADER_TYPE_COUNT )
    {
        return;
    }
    old_slot = -1;
    for( old_slot = 0; old_slot < WORKPHONE_SHADER_TYPE_COUNT; ++old_slot )
    {
        if( program->shaders[old_slot] == shader )
            break;
    }
    if( old_slot == shader->type )
        return;
    replaced = program->shaders[shader->type];
    if( old_slot >= WORKPHONE_SHADER_TYPE_COUNT )
        wp_shader_retain( shader );
    else
    {
        program->shaders[old_slot] = NULL;
        program->shader_revisions[old_slot] = 0u;
    }
    program->shaders[shader->type] = shader;
    program->shader_revisions[shader->type] = shader->revision;
    if( replaced != NULL )
        wp_shader_destroy( replaced );
    wp_shader_program_touch( program );
}

void wp_shader_program_detach( wp_shader_program *program, wp_shader_type type )
{
    if( program == NULL )
    {
        return;
    }
    if( (int)type < 0 || type >= WORKPHONE_SHADER_TYPE_COUNT )
    {
        return;
    }
    if( program->shaders[type] != NULL )
    {
        wp_shader *shader;
        shader = program->shaders[type];
        program->shaders[type] = NULL;
        program->shader_revisions[type] = 0u;
        wp_shader_destroy( shader );
        wp_shader_program_touch( program );
    }
}

wp_shader *wp_shader_program_get_shader( const wp_shader_program *program, wp_shader_type type )
{
    if( program == NULL )
    {
        return NULL;
    }
    if( (int)type < 0 || type >= WORKPHONE_SHADER_TYPE_COUNT )
    {
        return NULL;
    }
    {
        wp_s32 i;
        for( i = 0; i < WORKPHONE_SHADER_TYPE_COUNT; ++i )
        {
            if( program->shaders[i] != NULL && program->shaders[i]->type == type )
                return program->shaders[i];
        }
    }
    return NULL;
}

/* =========================================================================
 * Legacy uniform registry (backed by the parameter definition table)
 * ====================================================================== */

wp_s32 wp_shader_program_add_uniform( wp_shader_program *program, const wp_c8 *name,
                                      wp_uniform_type type )
{
    return wp_shader_program_register_param_internal(
        program, name, type, 1u, WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,
        WORKPHONE_SHADER_AUTO_PARAM_NONE, WORKPHONE_SHADER_RESOURCE_NONE );
}

wp_s32 wp_shader_program_get_uniform_count( const wp_shader_program *program )
{
    if( program == NULL )
    {
        return 0;
    }
    return (wp_s32)program->param_count;
}

wp_s32 wp_shader_program_find_uniform( const wp_shader_program *program, const wp_c8 *name )
{
    return wp_shader_program_find_index( program, name );
}

const wp_c8 *wp_shader_program_get_uniform_name( const wp_shader_program *program, wp_s32 index )
{
    if( program == NULL || index < 0 || (wp_u32)index >= program->param_count )
    {
        return WP_SHADER_TEXT( "" );
    }
    return program->param_defs[index].name;
}

wp_uniform_type wp_shader_program_get_uniform_type( const wp_shader_program *program, wp_s32 index )
{
    if( program == NULL || index < 0 || (wp_u32)index >= program->param_count )
    {
        return WORKPHONE_UNIFORM_TYPE_FLOAT;
    }
    return program->param_defs[index].type;
}

/* =========================================================================
 * Data-driven parameter registration & query
 * ====================================================================== */

wp_s32 wp_shader_program_register_param( wp_shader_program *program, const wp_c8 *name,
                                         wp_uniform_type type, wp_u32 array_size, wp_u32 variability,
                                         wp_shader_auto_param auto_param,
                                         wp_shader_resource_kind resource_kind )
{
    return wp_shader_program_register_param_internal( program, name, type, array_size, variability,
                                                      auto_param, resource_kind );
}

wp_s32 wp_shader_program_register_uniform( wp_shader_program *program, const wp_c8 *name,
                                           wp_uniform_type type )
{
    return wp_shader_program_register_param_internal(
        program, name, type, 1u, WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,
        WORKPHONE_SHADER_AUTO_PARAM_NONE, WORKPHONE_SHADER_RESOURCE_NONE );
}

wp_s32 wp_shader_program_get_param_count( const wp_shader_program *program )
{
    if( program == NULL )
    {
        return 0;
    }
    return (wp_s32)program->param_count;
}

const wp_shader_param_def *wp_shader_program_get_param_def( const wp_shader_program *program,
                                                            wp_s32 index )
{
    if( program == NULL || index < 0 || (wp_u32)index >= program->param_count )
    {
        return NULL;
    }
    return &program->param_defs[index];
}

wp_shader_param_handle wp_shader_program_find_param( const wp_shader_program *program,
                                                     const wp_c8 *name )
{
    wp_shader_param_handle handle;
    wp_s32 index;

    handle.owner = NULL;
    handle.element_size = 0u;
    handle.byte_offset = 0u;
    handle.parameter_index = 0u;
    handle.definition_revision = 0u;

    index = wp_shader_program_find_index( program, name );
    if( index < 0 )
    {
        return handle;
    }
    handle.owner = program;
    handle.element_size = program->param_defs[index].element_size;
    handle.byte_offset = program->param_defs[index].byte_offset;
    handle.parameter_index = (wp_u32)index;
    handle.definition_revision = program->definition_revision;
    return handle;
}

wp_s32 wp_shader_param_handle_is_valid( wp_shader_param_handle handle )
{
    return handle.owner != NULL && handle.element_size != 0u;
}

/* =========================================================================
 * Value setters - by index (legacy + new)
 * ====================================================================== */

void wp_shader_program_set_uniform_int( wp_shader_program *program, wp_s32 index, wp_s32 value )
{
    wp_shader_program_set_int( program, index, value );
}

void wp_shader_program_set_uniform_float( wp_shader_program *program, wp_s32 index, wp_f32 value )
{
    wp_shader_program_set_float( program, index, value );
}

void wp_shader_program_set_uniform_vec2( wp_shader_program *program, wp_s32 index, wp_vec2f value )
{
    wp_shader_program_set_vec2( program, index, value );
}

void wp_shader_program_set_uniform_vec3( wp_shader_program *program, wp_s32 index, wp_vec3f value )
{
    wp_shader_program_set_vec3( program, index, value );
}

void wp_shader_program_set_uniform_vec4( wp_shader_program *program, wp_s32 index, wp_vec4f value )
{
    wp_shader_program_set_vec4( program, index, value );
}

void wp_shader_program_set_uniform_mat4( wp_shader_program *program, wp_s32 index,
                                         const wp_mat4f *value )
{
    wp_shader_program_set_mat4( program, index, value );
}

void wp_shader_program_set_int( wp_shader_program *program, wp_s32 index, wp_s32 value )
{
    if( !wp_shader_program_valid_typed_index( program, index, WORKPHONE_UNIFORM_TYPE_INT ) )
    {
        return;
    }
    if( wp_shader_program_write_bytes( program, program->param_defs[index].byte_offset, &value,
                                       sizeof( value ) ) )
        wp_shader_program_dirty_param( program, index );
}

void wp_shader_program_set_float( wp_shader_program *program, wp_s32 index, wp_f32 value )
{
    if( !wp_shader_program_valid_typed_index( program, index, WORKPHONE_UNIFORM_TYPE_FLOAT ) )
    {
        return;
    }
    if( wp_shader_program_write_bytes( program, program->param_defs[index].byte_offset, &value,
                                       sizeof( value ) ) )
        wp_shader_program_dirty_param( program, index );
}

void wp_shader_program_set_vec2( wp_shader_program *program, wp_s32 index, wp_vec2f value )
{
    if( !wp_shader_program_valid_typed_index( program, index, WORKPHONE_UNIFORM_TYPE_VEC2 ) )
    {
        return;
    }
    if( wp_shader_program_write_bytes( program, program->param_defs[index].byte_offset, &value,
                                       sizeof( value ) ) )
        wp_shader_program_dirty_param( program, index );
}

void wp_shader_program_set_vec3( wp_shader_program *program, wp_s32 index, wp_vec3f value )
{
    if( !wp_shader_program_valid_typed_index( program, index, WORKPHONE_UNIFORM_TYPE_VEC3 ) )
    {
        return;
    }
    if( wp_shader_program_write_bytes( program, program->param_defs[index].byte_offset, &value,
                                       sizeof( value ) ) )
        wp_shader_program_dirty_param( program, index );
}

void wp_shader_program_set_vec4( wp_shader_program *program, wp_s32 index, wp_vec4f value )
{
    if( !wp_shader_program_valid_typed_index( program, index, WORKPHONE_UNIFORM_TYPE_VEC4 ) )
    {
        return;
    }
    if( wp_shader_program_write_bytes( program, program->param_defs[index].byte_offset, &value,
                                       sizeof( value ) ) )
        wp_shader_program_dirty_param( program, index );
}

void wp_shader_program_set_mat4( wp_shader_program *program, wp_s32 index, const wp_mat4f *value )
{
    if( !wp_shader_program_valid_typed_index( program, index, WORKPHONE_UNIFORM_TYPE_MAT4 ) ||
        value == NULL )
    {
        return;
    }
    if( wp_shader_program_write_bytes( program, program->param_defs[index].byte_offset, value,
                                       sizeof( wp_mat4f ) ) )
        wp_shader_program_dirty_param( program, index );
}

void wp_shader_program_set_raw( wp_shader_program *program, wp_s32 index, const void *data,
                                wp_u32 byte_count )
{
    if( program == NULL || index < 0 || (wp_u32)index >= program->param_count || data == NULL )
    {
        return;
    }
    if( program->param_defs[index].resource_kind != WORKPHONE_SHADER_RESOURCE_NONE )
        return;
    if( byte_count > program->param_defs[index].byte_size )
    {
        byte_count = program->param_defs[index].byte_size;
    }
    if( wp_shader_program_write_bytes( program, program->param_defs[index].byte_offset, data,
                                       byte_count ) )
        wp_shader_program_dirty_param( program, index );
}

/* =========================================================================
 * Value setters - by handle
 * ====================================================================== */

void wp_shader_program_set_int_h( wp_shader_program *program, wp_shader_param_handle handle,
                                  wp_s32 value )
{
    wp_s32 index;
    index = wp_shader_program_handle_index( program, handle, WORKPHONE_UNIFORM_TYPE_INT );
    if( index < 0 )
        return;
    wp_shader_program_set_int( program, index, value );
}

void wp_shader_program_set_float_h( wp_shader_program *program, wp_shader_param_handle handle,
                                    wp_f32 value )
{
    wp_s32 index;
    index = wp_shader_program_handle_index( program, handle, WORKPHONE_UNIFORM_TYPE_FLOAT );
    if( index < 0 )
        return;
    wp_shader_program_set_float( program, index, value );
}

void wp_shader_program_set_vec2_h( wp_shader_program *program, wp_shader_param_handle handle,
                                   wp_vec2f value )
{
    wp_s32 index;
    index = wp_shader_program_handle_index( program, handle, WORKPHONE_UNIFORM_TYPE_VEC2 );
    if( index < 0 )
        return;
    wp_shader_program_set_vec2( program, index, value );
}

void wp_shader_program_set_vec3_h( wp_shader_program *program, wp_shader_param_handle handle,
                                   wp_vec3f value )
{
    wp_s32 index;
    index = wp_shader_program_handle_index( program, handle, WORKPHONE_UNIFORM_TYPE_VEC3 );
    if( index < 0 )
        return;
    wp_shader_program_set_vec3( program, index, value );
}

void wp_shader_program_set_vec4_h( wp_shader_program *program, wp_shader_param_handle handle,
                                   wp_vec4f value )
{
    wp_s32 index;
    index = wp_shader_program_handle_index( program, handle, WORKPHONE_UNIFORM_TYPE_VEC4 );
    if( index < 0 )
        return;
    wp_shader_program_set_vec4( program, index, value );
}

void wp_shader_program_set_mat4_h( wp_shader_program *program, wp_shader_param_handle handle,
                                   const wp_mat4f *value )
{
    wp_s32 index;
    index = wp_shader_program_handle_index( program, handle, WORKPHONE_UNIFORM_TYPE_MAT4 );
    if( index < 0 || value == NULL )
        return;
    wp_shader_program_set_mat4( program, index, value );
}

/* =========================================================================
 * Bound resources (texture / buffer / sampler)
 * ====================================================================== */

wp_s32 wp_shader_program_set_resource( wp_shader_program *program, wp_s32 index, void *native )
{
    if( program == NULL || index < 0 || (wp_u32)index >= program->param_count )
    {
        return 0;
    }
    if( program->param_defs[index].resource_kind == WORKPHONE_SHADER_RESOURCE_NONE )
    {
        return 0;
    }
    if( program->resource_values[index] == native )
        return 1;
    program->resource_values[index] = native;
    wp_shader_program_dirty_param( program, index );
    return 1;
}

void *wp_shader_program_get_resource( const wp_shader_program *program, wp_s32 index )
{
    if( program == NULL || index < 0 || (wp_u32)index >= program->param_count )
    {
        return NULL;
    }
    return program->resource_values[index];
}

wp_s32 wp_shader_program_set_resource_named( wp_shader_program *program, const wp_c8 *name,
                                             void *native )
{
    wp_s32 index = wp_shader_program_find_index( program, name );
    if( index < 0 )
    {
        return 0;
    }
    return wp_shader_program_set_resource( program, index, native );
}

/* =========================================================================
 * Parameter buffer access (renderer upload)
 * ====================================================================== */

const void *wp_shader_program_get_param_buffer( const wp_shader_program *program )
{
    if( program == NULL )
    {
        return NULL;
    }
    return program->param_buffer;
}

wp_u32 wp_shader_program_get_param_buffer_size( const wp_shader_program *program )
{
    if( program == NULL )
    {
        return 0u;
    }
    return program->param_buffer_size;
}

wp_u32 wp_shader_program_get_param_dirty_mask( const wp_shader_program *program,
                                               wp_u32 variability_mask )
{
    return wp_shader_program_get_param_dirty_mask_word( program, variability_mask, 0u );
}

wp_u32 wp_shader_program_get_param_dirty_mask_word( const wp_shader_program *program,
                                                    wp_u32 variability_mask, wp_u32 word_index )
{
    wp_u32 mask;
    wp_u32 i;
    wp_u32 begin;
    wp_u32 end;

    mask = 0u;
    if( program == NULL )
    {
        return mask;
    }
    if( word_index >= ( WP_SHADER_MAX_PARAMS + 31u ) / 32u )
        return 0u;
    begin = word_index * 32u;
    end = begin + 32u;
    if( end > program->param_count )
        end = program->param_count;
    for( i = begin; i < end; ++i )
    {
        if( program->param_defs[i].is_dirty &&
            ( program->param_defs[i].variability & variability_mask ) != 0u )
        {
            mask |= ( 1u << ( i - begin ) );
        }
    }
    return mask;
}

void wp_shader_program_clear_param_dirty( wp_shader_program *program, wp_s32 index )
{
    wp_u32 i;
    if( program == NULL || index < 0 || (wp_u32)index >= program->param_count )
    {
        return;
    }
    program->param_defs[index].is_dirty = 0;
    program->is_dirty = program->link_dirty;
    for( i = 0u; i < program->param_count; ++i )
    {
        if( program->param_defs[i].is_dirty )
        {
            program->is_dirty = 1;
            break;
        }
    }
}

/* =========================================================================
 * Named options (client-exposed, data-driven)
 * ====================================================================== */

static wp_s32 wp_shader_program_find_option( const wp_shader_program *program, const wp_c8 *name )
{
    wp_s32 i;
    if( program == NULL || name == NULL )
    {
        return -1;
    }
    for( i = 0; i < (wp_s32)program->option_count; ++i )
    {
        if( wp_text_equal( program->options[i].name, name ) )
        {
            return i;
        }
    }
    return -1;
}

wp_s32 wp_shader_program_set_option( wp_shader_program *program, const wp_c8 *name, const wp_c8 *value )
{
    wp_s32 index;
    if( program == NULL || name == NULL || name[0] == '\0' ||
        wp_text_length( name ) >= WP_SHADER_MAX_NAME ||
        ( value != NULL && wp_text_length( value ) >= WP_SHADER_MAX_NAME ) )
    {
        return 0;
    }

    index = wp_shader_program_find_option( program, name );
    if( index >= 0 )
    {
        if( wp_text_equal( program->options[index].value, value ? value : WP_SHADER_TEXT( "" ) ) )
            return 1;
        wp_copy_cstr( program->options[index].value, WP_SHADER_MAX_NAME,
                      value ? value : WP_SHADER_TEXT( "" ) );
        wp_shader_program_touch( program );
        return 1;
    }

    if( program->option_count >= WP_SHADER_MAX_OPTIONS )
    {
        return 0;
    }

    wp_copy_cstr( program->options[program->option_count].name, WP_SHADER_MAX_NAME, name );
    wp_copy_cstr( program->options[program->option_count].value, WP_SHADER_MAX_NAME,
                  value ? value : WP_SHADER_TEXT( "" ) );
    program->option_count++;
    wp_shader_program_touch( program );
    return 1;
}

const wp_c8 *wp_shader_program_get_option( const wp_shader_program *program, const wp_c8 *name )
{
    wp_s32 index = wp_shader_program_find_option( program, name );
    if( index < 0 )
    {
        return WP_SHADER_TEXT( "" );
    }
    return program->options[index].value;
}

wp_s32 wp_shader_program_get_option_count( const wp_shader_program *program )
{
    if( program == NULL )
    {
        return 0;
    }
    return (wp_s32)program->option_count;
}

const wp_shader_option *wp_shader_program_get_option_at( const wp_shader_program *program, wp_s32 index )
{
    if( program == NULL || index < 0 || (wp_u32)index >= program->option_count )
    {
        return NULL;
    }
    return &program->options[index];
}

wp_s32 wp_shader_program_get_option_int( const wp_shader_program *program, const wp_c8 *name,
                                         wp_s32 default_value )
{
    const wp_c8 *value = wp_shader_program_get_option( program, name );
    if( value[0] == '\0' )
    {
        return default_value;
    }
    return (wp_s32)strtol( (const char *)(const void *)value, NULL, 10 );
}

wp_f32 wp_shader_program_get_option_float( const wp_shader_program *program, const wp_c8 *name,
                                           wp_f32 default_value )
{
    const wp_c8 *value = wp_shader_program_get_option( program, name );
    if( value[0] == '\0' )
    {
        return default_value;
    }
    return (wp_f32)strtod( (const char *)(const void *)value, NULL );
}

/* =========================================================================
 * Permutation selection
 * ====================================================================== */

wp_u32 wp_shader_program_get_permutation_mask( const wp_shader_program *program )
{
    if( program == NULL )
    {
        return 0u;
    }
    return program->permutation_mask;
}

void wp_shader_program_set_permutation_mask( wp_shader_program *program, wp_u32 mask )
{
    if( program == NULL )
    {
        return;
    }
    mask &= ( 1u << WP_SHADER_PERMUTATION_COUNT_BITS ) - 1u;
    if( program->permutation_mask != mask )
    {
        program->permutation_mask = mask;
        wp_shader_program_touch( program );
    }
}

void wp_shader_program_set_permutation( wp_shader_program *program, wp_u32 flag, wp_s32 enabled )
{
    wp_u32 mask;
    if( program == NULL )
    {
        return;
    }
    mask = enabled ? program->permutation_mask | flag : program->permutation_mask & ~flag;
    wp_shader_program_set_permutation_mask( program, mask );
}

wp_s32 wp_shader_program_has_permutation( const wp_shader_program *program, wp_u32 flag )
{
    if( program == NULL )
    {
        return 0;
    }
    return ( program->permutation_mask & flag ) != 0u;
}

/* =========================================================================
 * Program dirty tracking & native handle
 * ====================================================================== */

wp_s32 wp_shader_program_is_dirty( const wp_shader_program *program )
{
    if( program == NULL )
    {
        return 0;
    }
    return program->is_dirty;
}

void wp_shader_program_mark_dirty( wp_shader_program *program )
{
    if( program == NULL )
    {
        return;
    }
    wp_shader_program_touch( program );
}

void wp_shader_program_clear_dirty( wp_shader_program *program )
{
    wp_u32 i;
    if( program == NULL )
    {
        return;
    }
    for( i = 0u; i < program->param_count; ++i )
    {
        program->param_defs[i].is_dirty = 0;
    }
    program->is_dirty = program->link_dirty;
}

wp_u32 wp_shader_program_get_revision( const wp_shader_program *program )
{
    return program ? program->revision : 0u;
}

wp_u32 wp_shader_program_get_pipeline_revision( const wp_shader_program *program )
{
    return program ? program->pipeline_revision : 0u;
}

wp_u32 wp_shader_program_get_parameter_revision( const wp_shader_program *program )
{
    return program ? program->parameter_revision : 0u;
}

wp_u32 wp_shader_program_get_layout_revision( const wp_shader_program *program )
{
    return program ? program->definition_revision : 0u;
}

wp_u32 wp_shader_program_get_linked_generation( const wp_shader_program *program )
{
    return program ? program->linked_generation : 0u;
}

wp_shader_status wp_shader_program_get_status( const wp_shader_program *program )
{
    return program ? program->status : WORKPHONE_SHADER_STATUS_EMPTY;
}

const wp_c8 *wp_shader_program_get_last_error( const wp_shader_program *program )
{
    return program ? program->last_error : WP_SHADER_TEXT( "" );
}

void wp_shader_program_set_last_error( wp_shader_program *program, const wp_c8 *message )
{
    if( program != NULL )
        wp_copy_cstr( program->last_error, WP_SHADER_MAX_NAME,
                      message ? message : WP_SHADER_TEXT( "" ) );
}

void wp_shader_program_set_link_func( wp_shader_program *program, wp_shader_program_link_func func,
                                      void *user_data )
{
    if( program == NULL )
        return;
    if( program->link_func != func || program->link_user_data != user_data )
    {
        program->link_func = func;
        program->link_user_data = func ? user_data : NULL;
        wp_shader_program_touch( program );
    }
}

void wp_shader_program_set_bind_func( wp_shader_program *program, wp_shader_program_bind_func func,
                                      void *user_data )
{
    if( program == NULL )
        return;
    program->bind_func = func;
    program->bind_user_data = func ? user_data : NULL;
}

void wp_shader_program_set_release_func( wp_shader_program *program, wp_shader_release_func func,
                                         void *user_data )
{
    if( program == NULL )
        return;
    if( program->native != NULL &&
        ( program->release_func != func || program->release_user_data != user_data ) )
        return;
    program->release_func = func;
    program->release_user_data = func ? user_data : NULL;
}

wp_s32 wp_shader_program_update( wp_shader_program *program )
{
    wp_u32 revision;
    wp_s32 i;
    wp_s32 linked;
    wp_shader *attached;
    void *candidate;
    if( program == NULL )
        return 0;
    i = 0;
    while( i < WORKPHONE_SHADER_TYPE_COUNT )
    {
        attached = program->shaders[i];
        if( attached != NULL && attached->type != (wp_shader_type)i )
        {
            wp_shader_program_attach( program, attached );
            i = 0;
        }
        else
        {
            ++i;
        }
    }
    if( !wp_shader_program_topology_valid( program ) )
    {
        wp_shader_program_set_last_error( program,
                                          WP_SHADER_TEXT( "Shader stage topology is invalid." ) );
        program->status = WORKPHONE_SHADER_STATUS_FAILED;
        return program->native != NULL;
    }
    for( i = 0; i < WORKPHONE_SHADER_TYPE_COUNT; ++i )
    {
        if( program->shaders[i] &&
            program->shader_revisions[i] != wp_shader_get_revision( program->shaders[i] ) )
        {
            program->shader_revisions[i] = wp_shader_get_revision( program->shaders[i] );
            wp_shader_program_touch( program );
        }
        if( program->shaders[i] && wp_shader_is_dirty( program->shaders[i] ) &&
            !wp_shader_compile( program->shaders[i] ) )
        {
            wp_shader_program_set_last_error( program, wp_shader_get_last_error( program->shaders[i] ) );
            program->status = WORKPHONE_SHADER_STATUS_FAILED;
            return program->native != NULL;
        }
    }
    if( !program->link_dirty )
        return 1;
    if( !program->link_func )
    {
        wp_shader_program_set_last_error( program,
                                          WP_SHADER_TEXT( "No renderer program linker is installed." ) );
        program->status = WORKPHONE_SHADER_STATUS_FAILED;
        return program->native != NULL;
    }
    revision = program->pipeline_revision;
    if( program->link_failed && program->link_attempt_revision == revision )
        return program->native != NULL;
    program->link_attempt_revision = revision;
    candidate = NULL;
    program->status = WORKPHONE_SHADER_STATUS_COMPILING;
    program->last_error[0] = '\0';
    linked = program->link_func( program, &candidate, program->link_user_data );
    if( !linked || candidate == NULL )
    {
        if( candidate != NULL && candidate != program->native && program->release_func )
            program->release_func( candidate, program->release_user_data );
        if( program->last_error[0] == '\0' )
            wp_shader_program_set_last_error(
                program, linked ? WP_SHADER_TEXT( "Program linker returned no native object." )
                                : WP_SHADER_TEXT( "Program link callback failed." ) );
        program->link_failed = 1;
        program->status = WORKPHONE_SHADER_STATUS_FAILED;
        return program->native != NULL;
    }
    if( program->pipeline_revision != revision )
    {
        if( candidate != program->native && program->release_func )
            program->release_func( candidate, program->release_user_data );
        wp_shader_program_set_last_error(
            program, WP_SHADER_TEXT( "Shader program changed while linking was in progress." ) );
        program->status = WORKPHONE_SHADER_STATUS_SOURCE_READY;
        return program->native != NULL;
    }
    if( program->native != candidate )
    {
        if( program->native && program->release_func )
            program->release_func( program->native, program->release_user_data );
        program->native = candidate;
    }
    program->link_dirty = 0;
    program->link_failed = 0;
    program->status = WORKPHONE_SHADER_STATUS_COMPILED;
    program->last_error[0] = '\0';
    ++program->linked_generation;
    if( program->linked_generation == 0u )
        program->linked_generation = 1u;
    return 1;
}

wp_s32 wp_shader_program_bind( wp_shader_program *program, wp_renderer *renderer )
{
    wp_u32 revision;
    wp_u32 i;
    wp_s32 result;
    if( program == NULL || renderer == NULL || !wp_shader_program_update( program ) ||
        program->bind_func == NULL || program->native == NULL )
        return 0;
    revision = program->revision;
    result = program->bind_func( program, renderer, program->native, program->bind_user_data ) != 0;
    if( result )
        wp_renderer_mark_program_bound( renderer );
    if( result && program->revision == revision )
    {
        for( i = 0u; i < program->param_count; ++i )
            program->param_defs[i].is_dirty = 0;
        program->is_dirty = program->link_dirty;
    }
    return result;
}

void *wp_shader_program_get_native( const wp_shader_program *program )
{
    if( program == NULL )
    {
        return NULL;
    }
    return program->native;
}

void wp_shader_program_set_native( wp_shader_program *program, void *native )
{
    if( program == NULL )
    {
        return;
    }
    if( program->native != native )
    {
        if( program->native && program->release_func )
            program->release_func( program->native, program->release_user_data );
        program->native = native;
        if( native != NULL )
        {
            program->link_dirty = 0;
            program->link_failed = 0;
            program->link_attempt_revision = program->pipeline_revision;
            program->status = WORKPHONE_SHADER_STATUS_COMPILED;
            program->last_error[0] = '\0';
            ++program->linked_generation;
            if( program->linked_generation == 0u )
                program->linked_generation = 1u;
        }
        else
        {
            wp_shader_program_touch( program );
        }
    }
}
