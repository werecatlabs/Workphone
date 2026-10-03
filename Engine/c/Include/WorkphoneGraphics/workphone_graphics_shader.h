/**
 * @file wp_graphics_shader.h
 * @brief C API for GPU shader objects and shader programs.
 *
 * This API is data-driven. Shader programs own a parameter definition table
 * (wp_shader_param_def) that is registered by the client at setup time, plus a
 * set of named options (wp_shader_option) used to drive permutation selection
 * and runtime behaviour. The design integrates:
 *   - Esoterica engine:  parameter info / handle / annotation model
 *                        (MaterialShaderParameterInfo, MaterialShaderParameterHandle,
 *                        ReflectedShader::ParameterAnnotation, permutations).
 *   - Ogre3D:            auto-parameter semantics and constant definition table
 *                        (AutoConstantType, GpuConstantDefinition, GpuParamVariability).
 *
 * All public types and functions are C89 compatible.
 */

#ifndef WORKPHONE_GRAPHICS_SHADER_H
#define WORKPHONE_GRAPHICS_SHADER_H

#include "workphone_vector.h"
#include "workphone_matrix.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Opaque handles
 * ====================================================================== */

typedef struct wp_shader wp_shader;
typedef struct wp_shader_program wp_shader_program;
typedef struct wp_renderer wp_renderer;

/* =========================================================================
 * Shader stage / language
 * ====================================================================== */

typedef enum wp_shader_type
{
    WORKPHONE_SHADER_TYPE_VERTEX = 0,
    WORKPHONE_SHADER_TYPE_PIXEL = 1,
    WORKPHONE_SHADER_TYPE_GEOMETRY = 2,
    WORKPHONE_SHADER_TYPE_HULL = 3,
    WORKPHONE_SHADER_TYPE_DOMAIN = 4,
    WORKPHONE_SHADER_TYPE_COMPUTE = 5,
    WORKPHONE_SHADER_TYPE_RAY_GENERATION = 6,
    WORKPHONE_SHADER_TYPE_ANY_HIT = 7,
    WORKPHONE_SHADER_TYPE_CLOSEST_HIT = 8,
    WORKPHONE_SHADER_TYPE_MISS = 9,
    WORKPHONE_SHADER_TYPE_CALLABLE = 10,
    WORKPHONE_SHADER_TYPE_INTERSECTION = 11
} wp_shader_type;

#define WORKPHONE_SHADER_TYPE_COUNT 12

typedef enum wp_shader_language
{
    WORKPHONE_SHADER_LANGUAGE_HLSL = 0,
    WORKPHONE_SHADER_LANGUAGE_GLSL = 1,
    WORKPHONE_SHADER_LANGUAGE_SPIRV = 2,
    WORKPHONE_SHADER_LANGUAGE_METAL = 3,
    WORKPHONE_SHADER_LANGUAGE_DXIL = 4,
    WORKPHONE_SHADER_LANGUAGE_DXBC = 5,
    WORKPHONE_SHADER_LANGUAGE_WGSL = 6,
    WORKPHONE_SHADER_LANGUAGE_UNKNOWN = 7
} wp_shader_language;

typedef enum wp_shader_status
{
    WORKPHONE_SHADER_STATUS_EMPTY = 0,
    WORKPHONE_SHADER_STATUS_SOURCE_READY = 1,
    WORKPHONE_SHADER_STATUS_COMPILING = 2,
    WORKPHONE_SHADER_STATUS_COMPILED = 3,
    WORKPHONE_SHADER_STATUS_FAILED = 4
} wp_shader_status;

/* =========================================================================
 * Uniform value types
 * ====================================================================== */

typedef enum wp_uniform_type
{
    WORKPHONE_UNIFORM_TYPE_INT = 0,
    WORKPHONE_UNIFORM_TYPE_FLOAT = 1,
    WORKPHONE_UNIFORM_TYPE_VEC2 = 2,
    WORKPHONE_UNIFORM_TYPE_VEC3 = 3,
    WORKPHONE_UNIFORM_TYPE_VEC4 = 4,
    WORKPHONE_UNIFORM_TYPE_MAT4 = 5
} wp_uniform_type;

/* =========================================================================
 * Capacity limits (overridable)
 * ====================================================================== */

#ifndef WP_SHADER_MAX_NAME
#    define WP_SHADER_MAX_NAME 256
#endif

#ifndef WP_SHADER_MAX_ENTRY
#    define WP_SHADER_MAX_ENTRY 128
#endif

#ifndef WP_SHADER_MAX_PROFILE
#    define WP_SHADER_MAX_PROFILE 64
#endif

#ifndef WP_SHADER_MAX_PARAMS
#    define WP_SHADER_MAX_PARAMS 64
#endif

#ifndef WP_SHADER_MAX_OPTIONS
#    define WP_SHADER_MAX_OPTIONS 32
#endif

#ifndef WP_SHADER_MAX_DEFINES
#    define WP_SHADER_MAX_DEFINES 64
#endif

#ifndef WP_SHADER_MAX_SPECIALIZATIONS
#    define WP_SHADER_MAX_SPECIALIZATIONS 32
#endif

typedef struct wp_shader_define
{
    wp_c8 name[WP_SHADER_MAX_NAME];
    wp_c8 value[WP_SHADER_MAX_NAME];
} wp_shader_define;

typedef struct wp_shader_specialization
{
    wp_c8 name[WP_SHADER_MAX_NAME];
    wp_u32 value;
} wp_shader_specialization;

/* =========================================================================
 * Data-driven: parameter variability (Ogre GpuParamVariability)
 * Powers of two so they can be combined as masks.
 * ====================================================================== */

typedef enum wp_shader_param_variability
{
    WORKPHONE_SHADER_PARAM_VARIABILITY_GLOBAL = 0x0001u,     /**< set once, never changes            */
    WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT = 0x0002u, /**< varies per renderable             */
    WORKPHONE_SHADER_PARAM_VARIABILITY_PER_LIGHT = 0x0004u,  /**< varies with light setup           */
    WORKPHONE_SHADER_PARAM_VARIABILITY_PER_PASS = 0x0008u,   /**< varies per render pass            */
    WORKPHONE_SHADER_PARAM_VARIABILITY_PER_FRAME = 0x0010u,  /**< varies once per frame             */
    WORKPHONE_SHADER_PARAM_VARIABILITY_USER = 0x0020u        /**< client-managed custom frequency   */
} wp_shader_param_variability;

/* =========================================================================
 * Data-driven: auto-parameter semantics (Ogre AutoConstantType, trimmed set)
 * When a parameter definition is bound to one of these the renderer is
 * expected to supply the value automatically; the client does not set it.
 * ====================================================================== */

typedef enum wp_shader_auto_param
{
    WORKPHONE_SHADER_AUTO_PARAM_NONE = 0,
    WORKPHONE_SHADER_AUTO_PARAM_WORLD_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_INVERSE_WORLD_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_TRANSPOSE_WORLD_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_INVERSE_TRANSPOSE_WORLD_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_VIEW_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_INVERSE_VIEW_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_TRANSPOSE_VIEW_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_INVERSE_TRANSPOSE_VIEW_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_PROJECTION_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_INVERSE_PROJECTION_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_TRANSPOSE_PROJECTION_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_INVERSE_TRANSPOSE_PROJECTION_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_VIEWPROJ_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_INVERSE_VIEWPROJ_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_WORLDVIEW_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_INVERSE_WORLDVIEW_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_TRANSPOSE_WORLDVIEW_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_INVERSE_TRANSPOSE_WORLDVIEW_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_WORLDVIEWPROJ_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_INVERSE_WORLDVIEWPROJ_MATRIX,
    WORKPHONE_SHADER_AUTO_PARAM_NORMAL_MATRIX, /**< inverse-transpose upper 3x3 of worldview */
    WORKPHONE_SHADER_AUTO_PARAM_WORLD_MATRIX_ARRAY,
    WORKPHONE_SHADER_AUTO_PARAM_WORLD_MATRIX_ARRAY_3X4,
    WORKPHONE_SHADER_AUTO_PARAM_CAMERA_POSITION,
    WORKPHONE_SHADER_AUTO_PARAM_CAMERA_POSITION_OBJECT_SPACE,
    WORKPHONE_SHADER_AUTO_PARAM_CAMERA_DIRECTION,
    WORKPHONE_SHADER_AUTO_PARAM_TIME,
    WORKPHONE_SHADER_AUTO_PARAM_TIME_0_X,
    WORKPHONE_SHADER_AUTO_PARAM_VIEWPORT_SIZE,
    WORKPHONE_SHADER_AUTO_PARAM_VIEWPORT_WIDTH,
    WORKPHONE_SHADER_AUTO_PARAM_VIEWPORT_HEIGHT,
    WORKPHONE_SHADER_AUTO_PARAM_NEAR_CLIP_DISTANCE,
    WORKPHONE_SHADER_AUTO_PARAM_FAR_CLIP_DISTANCE,
    WORKPHONE_SHADER_AUTO_PARAM_FOV,
    WORKPHONE_SHADER_AUTO_PARAM_ASPECT_RATIO,
    WORKPHONE_SHADER_AUTO_PARAM_LIGHT_COUNT,
    WORKPHONE_SHADER_AUTO_PARAM_LIGHT_POSITION,
    WORKPHONE_SHADER_AUTO_PARAM_LIGHT_POSITION_OBJECT_SPACE,
    WORKPHONE_SHADER_AUTO_PARAM_LIGHT_DIRECTION,
    WORKPHONE_SHADER_AUTO_PARAM_LIGHT_DIFFUSE_COLOR,
    WORKPHONE_SHADER_AUTO_PARAM_LIGHT_SPECULAR_COLOR,
    WORKPHONE_SHADER_AUTO_PARAM_LIGHT_ATTENUATION,
    WORKPHONE_SHADER_AUTO_PARAM_AMBIENT_LIGHT_COLOR,
    WORKPHONE_SHADER_AUTO_PARAM_FOG_COLOR,
    WORKPHONE_SHADER_AUTO_PARAM_FOG_PARAMS,
    WORKPHONE_SHADER_AUTO_PARAM_PASS_ITERATION_NUMBER,
    WORKPHONE_SHADER_AUTO_PARAM_RENDER_TARGET_FLIP, /**< -1 for GL / +1 for D3D */
    WORKPHONE_SHADER_AUTO_PARAM_COUNT
} wp_shader_auto_param;

/* =========================================================================
 * Data-driven: bound resource kind (Esoterica texture/buffer/sampler)
 * ====================================================================== */

typedef enum wp_shader_resource_kind
{
    WORKPHONE_SHADER_RESOURCE_NONE = 0,
    WORKPHONE_SHADER_RESOURCE_TEXTURE,
    WORKPHONE_SHADER_RESOURCE_BUFFER,
    WORKPHONE_SHADER_RESOURCE_SAMPLER
} wp_shader_resource_kind;

/* =========================================================================
 * Data-driven: parameter definition (Esoterica MaterialShaderParameterInfo +
 * Ogre GpuConstantDefinition). Describes one named slot in the program's
 * parameter table. The client registers definitions, then sets values by
 * index or by handle.
 * ====================================================================== */

typedef struct wp_shader_param_def
{
    wp_c8 name[WP_SHADER_MAX_NAME];
    wp_uniform_type type;                  /**< value type                          */
    wp_u32 element_size;                   /**< bytes per array element (packed)    */
    wp_u32 array_size;                     /**< number of elements (1 for scalars)  */
    wp_u32 byte_offset;                    /**< physical offset in the param buffer */
    wp_u32 byte_size;                      /**< total bytes reserved for this param */
    wp_u32 variability;                    /**< bitmask of wp_shader_param_variability */
    wp_shader_auto_param auto_param;       /**< auto-bind semantic, or NONE         */
    wp_shader_resource_kind resource_kind; /**< NONE / TEXTURE / BUFFER / SAMPLER   */
    wp_s32 is_dirty;                       /**< per-param dirty flag                */
} wp_shader_param_def;

/** Lightweight handle used to set a parameter without a name lookup
 *  (Esoterica MaterialShaderParameterHandle). */
typedef struct wp_shader_param_handle
{
    const wp_shader_program *owner;
    wp_u32 element_size;
    wp_u32 byte_offset;
    wp_u32 parameter_index;
    wp_u32 definition_revision;
} wp_shader_param_handle;

#define WP_SHADER_PARAM_HANDLE_INVALID 0u

/** Successful program natives must retain or copy every stage dependency they need. A non-null
 * failed candidate is released with the registered release callback when one is available. */
typedef wp_s32 ( *wp_shader_compile_func )( wp_shader *shader, void **candidate_native,
                                            void *user_data );
typedef void ( *wp_shader_release_func )( void *native, void *user_data );
typedef wp_s32 ( *wp_shader_program_link_func )( wp_shader_program *program, void **candidate_native,
                                                 void *user_data );
typedef wp_s32 ( *wp_shader_program_bind_func )( wp_shader_program *program, wp_renderer *renderer,
                                                 void *native, void *user_data );

/* =========================================================================
 * Data-driven: named options exposed to the client (Esoterica
 * ReflectedShader::ParameterAnnotation). Options are free-form name/value
 * string pairs the client may set/query to drive permutation selection or
 * runtime behaviour (e.g. "AlphaTest"="1", "TwoSided"="1").
 * ====================================================================== */

typedef struct wp_shader_option
{
    wp_c8 name[WP_SHADER_MAX_NAME];
    wp_c8 value[WP_SHADER_MAX_NAME];
} wp_shader_option;

/* =========================================================================
 * Permutation flags (Esoterica MaterialShader::Permutation +
 * MaterialShaderFlags). Combined into the program's permutation mask to
 * select a compiled variant.
 * ====================================================================== */

#define WP_SHADER_PERMUTATION_DEPTH_ONLY ( 1u << 0 )
#define WP_SHADER_PERMUTATION_ALPHA_TEST ( 1u << 1 )
#define WP_SHADER_PERMUTATION_ALPHA_BLEND ( 1u << 2 )
#define WP_SHADER_PERMUTATION_TWO_SIDED ( 1u << 3 )
#define WP_SHADER_PERMUTATION_WIREFRAME ( 1u << 4 )
#define WP_SHADER_PERMUTATION_SHADOW_CASTER ( 1u << 5 )
#define WP_SHADER_PERMUTATION_COUNT_BITS 6

/* =========================================================================
 * Shader object API (unchanged surface, fully implemented)
 * ====================================================================== */

wp_shader *wp_shader_create( wp_shader_type type, wp_shader_language language, const wp_c8 *source,
                             const wp_c8 *entry_point );
void wp_shader_destroy( wp_shader *shader );

wp_shader_type wp_shader_get_type( const wp_shader *shader );
void wp_shader_set_type( wp_shader *shader, wp_shader_type type );
wp_shader_language wp_shader_get_language( const wp_shader *shader );
void wp_shader_set_language( wp_shader *shader, wp_shader_language language );
const wp_c8 *wp_shader_get_source( const wp_shader *shader );
wp_s32 wp_shader_set_source( wp_shader *shader, const wp_c8 *source );
const void *wp_shader_get_binary( const wp_shader *shader, wp_u32 *byte_count );
wp_s32 wp_shader_set_binary( wp_shader *shader, const void *binary, wp_u32 byte_count );
const wp_c8 *wp_shader_get_entry_point( const wp_shader *shader );
void wp_shader_set_entry_point( wp_shader *shader, const wp_c8 *entry_point );
const wp_c8 *wp_shader_get_profile( const wp_shader *shader );
void wp_shader_set_profile( wp_shader *shader, const wp_c8 *profile );
wp_s32 wp_shader_get_define_count( const wp_shader *shader );
const wp_shader_define *wp_shader_get_define_at( const wp_shader *shader, wp_s32 index );
wp_s32 wp_shader_set_define( wp_shader *shader, const wp_c8 *name, const wp_c8 *value );
wp_s32 wp_shader_remove_define( wp_shader *shader, const wp_c8 *name );
void wp_shader_clear_defines( wp_shader *shader );
wp_s32 wp_shader_get_specialization_count( const wp_shader *shader );
const wp_shader_specialization *wp_shader_get_specialization_at( const wp_shader *shader, wp_s32 index );
wp_s32 wp_shader_set_specialization( wp_shader *shader, const wp_c8 *name, wp_u32 value );

wp_s32 wp_shader_is_dirty( const wp_shader *shader );
void wp_shader_mark_dirty( wp_shader *shader );
void wp_shader_clear_dirty( wp_shader *shader );
wp_u32 wp_shader_get_revision( const wp_shader *shader );
wp_shader_status wp_shader_get_status( const wp_shader *shader );
const wp_c8 *wp_shader_get_last_error( const wp_shader *shader );
void wp_shader_set_last_error( wp_shader *shader, const wp_c8 *message );
void wp_shader_set_compile_func( wp_shader *shader, wp_shader_compile_func compile_func,
                                 void *user_data );
void wp_shader_set_release_func( wp_shader *shader, wp_shader_release_func release_func,
                                 void *user_data );
wp_s32 wp_shader_compile( wp_shader *shader );

void *wp_shader_get_native( const wp_shader *shader );
void wp_shader_set_native( wp_shader *shader, void *native );

/* =========================================================================
 * Shader program API - lifecycle & stage attachment
 * ====================================================================== */

wp_shader_program *wp_shader_program_create( void );
void wp_shader_program_retain( wp_shader_program *program );
void wp_shader_program_destroy( wp_shader_program *program );

void wp_shader_program_attach( wp_shader_program *program, wp_shader *shader );
void wp_shader_program_detach( wp_shader_program *program, wp_shader_type type );
wp_shader *wp_shader_program_get_shader( const wp_shader_program *program, wp_shader_type type );

/* =========================================================================
 * Shader program API - legacy uniform registry
 * (backwards compatible with the original declaration; internally backed by
 *  the data-driven parameter definition table)
 * ====================================================================== */

wp_s32 wp_shader_program_add_uniform( wp_shader_program *program, const wp_c8 *name,
                                      wp_uniform_type type );
wp_s32 wp_shader_program_get_uniform_count( const wp_shader_program *program );
wp_s32 wp_shader_program_find_uniform( const wp_shader_program *program, const wp_c8 *name );
const wp_c8 *wp_shader_program_get_uniform_name( const wp_shader_program *program, wp_s32 index );
wp_uniform_type wp_shader_program_get_uniform_type( const wp_shader_program *program, wp_s32 index );

void wp_shader_program_set_uniform_int( wp_shader_program *program, wp_s32 index, wp_s32 value );
void wp_shader_program_set_uniform_float( wp_shader_program *program, wp_s32 index, wp_f32 value );
void wp_shader_program_set_uniform_vec2( wp_shader_program *program, wp_s32 index, wp_vec2f value );
void wp_shader_program_set_uniform_vec3( wp_shader_program *program, wp_s32 index, wp_vec3f value );
void wp_shader_program_set_uniform_vec4( wp_shader_program *program, wp_s32 index, wp_vec4f value );
void wp_shader_program_set_uniform_mat4( wp_shader_program *program, wp_s32 index,
                                         const wp_mat4f *value );

/* =========================================================================
 * Shader program API - data-driven parameter definitions
 * ====================================================================== */

/** Register a parameter definition. Returns the parameter index on success,
 *  -1 if the table is full or the name is already registered. */
wp_s32 wp_shader_program_register_param( wp_shader_program *program, const wp_c8 *name,
                                         wp_uniform_type type, wp_u32 array_size, wp_u32 variability,
                                         wp_shader_auto_param auto_param,
                                         wp_shader_resource_kind resource_kind );

/** Register a plain uniform (convenience wrapper, default variability). */
wp_s32 wp_shader_program_register_uniform( wp_shader_program *program, const wp_c8 *name,
                                           wp_uniform_type type );

wp_s32 wp_shader_program_get_param_count( const wp_shader_program *program );
const wp_shader_param_def *wp_shader_program_get_param_def( const wp_shader_program *program,
                                                            wp_s32 index );

/** Resolve a stable handle for a named parameter (no per-call string lookup). */
wp_shader_param_handle wp_shader_program_find_param( const wp_shader_program *program,
                                                     const wp_c8 *name );
wp_s32 wp_shader_param_handle_is_valid( wp_shader_param_handle handle );

/* =========================================================================
 * Shader program API - set values by index or handle
 * ====================================================================== */

void wp_shader_program_set_int( wp_shader_program *program, wp_s32 index, wp_s32 value );
void wp_shader_program_set_float( wp_shader_program *program, wp_s32 index, wp_f32 value );
void wp_shader_program_set_vec2( wp_shader_program *program, wp_s32 index, wp_vec2f value );
void wp_shader_program_set_vec3( wp_shader_program *program, wp_s32 index, wp_vec3f value );
void wp_shader_program_set_vec4( wp_shader_program *program, wp_s32 index, wp_vec4f value );
void wp_shader_program_set_mat4( wp_shader_program *program, wp_s32 index, const wp_mat4f *value );

void wp_shader_program_set_int_h( wp_shader_program *program, wp_shader_param_handle handle,
                                  wp_s32 value );
void wp_shader_program_set_float_h( wp_shader_program *program, wp_shader_param_handle handle,
                                    wp_f32 value );
void wp_shader_program_set_vec2_h( wp_shader_program *program, wp_shader_param_handle handle,
                                   wp_vec2f value );
void wp_shader_program_set_vec3_h( wp_shader_program *program, wp_shader_param_handle handle,
                                   wp_vec3f value );
void wp_shader_program_set_vec4_h( wp_shader_program *program, wp_shader_param_handle handle,
                                   wp_vec4f value );
void wp_shader_program_set_mat4_h( wp_shader_program *program, wp_shader_param_handle handle,
                                   const wp_mat4f *value );

/** Copy bytes already packed for the slot's reflected 16-byte array stride. */
void wp_shader_program_set_raw( wp_shader_program *program, wp_s32 index, const void *data,
                                wp_u32 byte_count );

/* =========================================================================
 * Shader program API - bound resources (Esoterica texture/buffer/sampler)
 * The native pointer is renderer-specific (e.g. a texture view handle).
 * ====================================================================== */

wp_s32 wp_shader_program_set_resource( wp_shader_program *program, wp_s32 index, void *native );
void *wp_shader_program_get_resource( const wp_shader_program *program, wp_s32 index );
wp_s32 wp_shader_program_set_resource_named( wp_shader_program *program, const wp_c8 *name,
                                             void *native );

/* =========================================================================
 * Shader program API - parameter buffer access (renderer upload)
 * ====================================================================== */

const void *wp_shader_program_get_param_buffer( const wp_shader_program *program );
wp_u32 wp_shader_program_get_param_buffer_size( const wp_shader_program *program );
wp_u32 wp_shader_program_get_param_dirty_mask( const wp_shader_program *program,
                                               wp_u32 variability_mask );
wp_u32 wp_shader_program_get_param_dirty_mask_word( const wp_shader_program *program,
                                                    wp_u32 variability_mask, wp_u32 word_index );
void wp_shader_program_clear_param_dirty( wp_shader_program *program, wp_s32 index );

/* =========================================================================
 * Shader program API - named options (client-exposed, data-driven)
 * ====================================================================== */

wp_s32 wp_shader_program_set_option( wp_shader_program *program, const wp_c8 *name, const wp_c8 *value );
const wp_c8 *wp_shader_program_get_option( const wp_shader_program *program, const wp_c8 *name );
wp_s32 wp_shader_program_get_option_count( const wp_shader_program *program );
const wp_shader_option *wp_shader_program_get_option_at( const wp_shader_program *program,
                                                         wp_s32 index );
wp_s32 wp_shader_program_get_option_int( const wp_shader_program *program, const wp_c8 *name,
                                         wp_s32 default_value );
wp_f32 wp_shader_program_get_option_float( const wp_shader_program *program, const wp_c8 *name,
                                           wp_f32 default_value );

/* =========================================================================
 * Shader program API - permutation selection
 * ====================================================================== */

wp_u32 wp_shader_program_get_permutation_mask( const wp_shader_program *program );
void wp_shader_program_set_permutation_mask( wp_shader_program *program, wp_u32 mask );
void wp_shader_program_set_permutation( wp_shader_program *program, wp_u32 flag, wp_s32 enabled );
wp_s32 wp_shader_program_has_permutation( const wp_shader_program *program, wp_u32 flag );

/* =========================================================================
 * Shader program API - dirty tracking & native handle
 * ====================================================================== */

wp_s32 wp_shader_program_is_dirty( const wp_shader_program *program );
void wp_shader_program_mark_dirty( wp_shader_program *program );
void wp_shader_program_clear_dirty( wp_shader_program *program );
wp_u32 wp_shader_program_get_revision( const wp_shader_program *program );
wp_u32 wp_shader_program_get_pipeline_revision( const wp_shader_program *program );
wp_u32 wp_shader_program_get_parameter_revision( const wp_shader_program *program );
wp_u32 wp_shader_program_get_layout_revision( const wp_shader_program *program );
wp_u32 wp_shader_program_get_linked_generation( const wp_shader_program *program );
wp_shader_status wp_shader_program_get_status( const wp_shader_program *program );
const wp_c8 *wp_shader_program_get_last_error( const wp_shader_program *program );
void wp_shader_program_set_last_error( wp_shader_program *program, const wp_c8 *message );
void wp_shader_program_set_link_func( wp_shader_program *program, wp_shader_program_link_func link_func,
                                      void *user_data );
void wp_shader_program_set_bind_func( wp_shader_program *program, wp_shader_program_bind_func bind_func,
                                      void *user_data );
void wp_shader_program_set_release_func( wp_shader_program *program, wp_shader_release_func release_func,
                                         void *user_data );
wp_s32 wp_shader_program_update( wp_shader_program *program );
wp_s32 wp_shader_program_bind( wp_shader_program *program, wp_renderer *renderer );

void *wp_shader_program_get_native( const wp_shader_program *program );
void wp_shader_program_set_native( wp_shader_program *program, void *native );

#ifdef __cplusplus
}
#endif

#endif
