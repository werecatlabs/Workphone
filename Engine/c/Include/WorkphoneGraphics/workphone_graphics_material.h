/**
 * @file wp_graphics_material.h
 * @brief C API for graphics materials.
 *
 * A material describes the visual surface properties of a rendered object.
 * It stores colour channels (ambient, diffuse, specular, emissive), PBR
 * parameters (metalness, roughness), texture slots, blend/depth/cull state,
 * and an optional reference to a shader program. A material technique
 * groups one or more material passes; each pass represents a single
 * rendering step with its own state and texture bindings.
 */

#ifndef WORKPHONE_GRAPHICS_MATERIAL_H
#define WORKPHONE_GRAPHICS_MATERIAL_H

#include <stdint.h>
#include "workphone_color.h"
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_graphics_material wp_graphics_material;
typedef struct wp_renderer wp_renderer;
typedef struct wp_shader_program wp_shader_program;

typedef enum wp_graphics_material_type
{
    WORKPHONE_MATERIAL_TYPE_STANDARD = 0,
    WORKPHONE_MATERIAL_TYPE_TERRAIN = 1,
    WORKPHONE_MATERIAL_TYPE_SKYBOX = 2,
    WORKPHONE_MATERIAL_TYPE_UI = 3,
    WORKPHONE_MATERIAL_TYPE_CUSTOM = 4
} wp_graphics_material_type;

enum
{
    WORKPHONE_MATERIAL_FLAG_TRANSPARENT = ( 1u << 0 ),
    WORKPHONE_MATERIAL_FLAG_CUTOUT = ( 1u << 1 ),
    WORKPHONE_MATERIAL_FLAG_LIGHTING = ( 1u << 2 ),
    WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE = ( 1u << 3 ),
    WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK = ( 1u << 4 ),
    WORKPHONE_MATERIAL_FLAG_EMISSION = ( 1u << 5 ),
    WORKPHONE_MATERIAL_FLAG_REFRACTION = ( 1u << 6 )
};

#define WORKPHONE_MATERIAL_FLAG_ALL ( ( 1u << 7 ) - 1u )

#ifndef WP_MATERIAL_MAX_TEXTURES
#    define WP_MATERIAL_MAX_TEXTURES 8
#endif

#ifndef WP_MATERIAL_MAX_NAME
#    define WP_MATERIAL_MAX_NAME 256
#endif

/** Conventional PBR texture slots. Backends may leave unsupported slots unbound. */
typedef enum wp_graphics_material_texture_slot
{
    WORKPHONE_MATERIAL_TEXTURE_BASE_COLOUR = 0,
    WORKPHONE_MATERIAL_TEXTURE_NORMAL = 1,
    WORKPHONE_MATERIAL_TEXTURE_METALLIC_ROUGHNESS = 2,
    WORKPHONE_MATERIAL_TEXTURE_OCCLUSION = 3,
    WORKPHONE_MATERIAL_TEXTURE_EMISSIVE = 4,
    WORKPHONE_MATERIAL_TEXTURE_HEIGHT = 5,
    WORKPHONE_MATERIAL_TEXTURE_CLEARCOAT = 6,
    WORKPHONE_MATERIAL_TEXTURE_TRANSMISSION = 7
} wp_graphics_material_texture_slot;

/** Complete, serialisable CPU-side material description. */
typedef struct wp_graphics_material_desc
{
    wp_u32 struct_size;
    wp_graphics_material_type material_type;
    wp_colour_f ambient;
    wp_colour_f diffuse;
    wp_colour_f specular;
    wp_colour_f emissive;
    wp_f32 metalness;
    wp_f32 roughness;
    wp_f32 normal_scale;
    wp_f32 occlusion_strength;
    wp_f32 emissive_intensity;
    wp_f32 alpha_cutoff;
    wp_f32 clearcoat;
    wp_f32 clearcoat_roughness;
    wp_f32 transmission;
    wp_f32 index_of_refraction;
    wp_u32 flags;
    wp_blend_mode blend_mode;
    wp_cull_mode cull_mode;
    wp_c8 texture_names[WP_MATERIAL_MAX_TEXTURES][WP_MATERIAL_MAX_NAME];
} wp_graphics_material_desc;

/**
 * Backend hook invoked automatically before a dirty material is rendered.
 * Return non-zero only after all backend resources and constants are current.
 */
typedef wp_s32 ( *wp_graphics_material_sync_func )( const wp_graphics_material *mat, void *native,
                                                    void *user_data );
typedef wp_s32 ( *wp_graphics_material_bind_func )( const wp_graphics_material *mat,
                                                    wp_renderer *renderer, void *native,
                                                    void *user_data );
typedef void ( *wp_graphics_material_release_func )( void *native, void *user_data );

WORKPHONE_API wp_graphics_material *wp_graphics_material_create( void );
WORKPHONE_API wp_graphics_material *wp_graphics_material_create_from_desc(
    const wp_graphics_material_desc *desc );
WORKPHONE_API wp_graphics_material *wp_graphics_material_clone( const wp_graphics_material *source );
WORKPHONE_API void wp_graphics_material_destroy( wp_graphics_material *mat );

WORKPHONE_API void wp_graphics_material_desc_init( wp_graphics_material_desc *desc );
WORKPHONE_API wp_s32 wp_graphics_material_get_desc( const wp_graphics_material *mat,
                                                    wp_graphics_material_desc *desc );
WORKPHONE_API wp_s32 wp_graphics_material_set_desc( wp_graphics_material *mat,
                                                    const wp_graphics_material_desc *desc );
WORKPHONE_API wp_s32 wp_graphics_material_copy( wp_graphics_material *destination,
                                                const wp_graphics_material *source );
WORKPHONE_API void wp_graphics_material_reset( wp_graphics_material *mat );

WORKPHONE_API wp_graphics_material_type wp_graphics_material_get_type( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_type( wp_graphics_material *mat,
                                                  wp_graphics_material_type type );

WORKPHONE_API wp_colour_f wp_graphics_material_get_ambient( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_ambient( wp_graphics_material *mat, wp_colour_f colour );

WORKPHONE_API wp_colour_f wp_graphics_material_get_diffuse( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_diffuse( wp_graphics_material *mat, wp_colour_f colour );

WORKPHONE_API wp_colour_f wp_graphics_material_get_specular( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_specular( wp_graphics_material *mat, wp_colour_f colour );

WORKPHONE_API wp_colour_f wp_graphics_material_get_emissive( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_emissive( wp_graphics_material *mat, wp_colour_f colour );

WORKPHONE_API wp_f32 wp_graphics_material_get_metalness( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_metalness( wp_graphics_material *mat, wp_f32 metalness );

WORKPHONE_API wp_f32 wp_graphics_material_get_roughness( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_roughness( wp_graphics_material *mat, wp_f32 roughness );

WORKPHONE_API wp_f32 wp_graphics_material_get_normal_scale( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_normal_scale( wp_graphics_material *mat, wp_f32 value );
WORKPHONE_API wp_f32 wp_graphics_material_get_occlusion_strength( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_occlusion_strength( wp_graphics_material *mat,
                                                                wp_f32 value );
WORKPHONE_API wp_f32 wp_graphics_material_get_emissive_intensity( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_emissive_intensity( wp_graphics_material *mat,
                                                                wp_f32 value );
WORKPHONE_API wp_f32 wp_graphics_material_get_alpha_cutoff( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_alpha_cutoff( wp_graphics_material *mat, wp_f32 value );
WORKPHONE_API wp_f32 wp_graphics_material_get_clearcoat( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_clearcoat( wp_graphics_material *mat, wp_f32 value );
WORKPHONE_API wp_f32 wp_graphics_material_get_clearcoat_roughness( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_clearcoat_roughness( wp_graphics_material *mat,
                                                                 wp_f32 value );
WORKPHONE_API wp_f32 wp_graphics_material_get_transmission( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_transmission( wp_graphics_material *mat, wp_f32 value );
WORKPHONE_API wp_f32 wp_graphics_material_get_index_of_refraction( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_index_of_refraction( wp_graphics_material *mat,
                                                                 wp_f32 value );

WORKPHONE_API wp_u32 wp_graphics_material_get_flags( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_flags( wp_graphics_material *mat, wp_u32 flags );
WORKPHONE_API void wp_graphics_material_set_flag( wp_graphics_material *mat, wp_u32 flag,
                                                  wp_s32 enabled );
WORKPHONE_API wp_s32 wp_graphics_material_has_flag( const wp_graphics_material *mat, wp_u32 flag );

WORKPHONE_API wp_blend_mode wp_graphics_material_get_blend_mode( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_blend_mode( wp_graphics_material *mat, wp_blend_mode mode );

WORKPHONE_API wp_cull_mode wp_graphics_material_get_cull_mode( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_cull_mode( wp_graphics_material *mat, wp_cull_mode mode );

WORKPHONE_API const wp_c8 *wp_graphics_material_get_texture_name( const wp_graphics_material *mat,
                                                                  wp_s32 layer );
WORKPHONE_API void wp_graphics_material_set_texture_name( wp_graphics_material *mat, wp_s32 layer,
                                                          const wp_c8 *name );

WORKPHONE_API wp_s32 wp_graphics_material_is_dirty( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_mark_dirty( wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_clear_dirty( wp_graphics_material *mat );
WORKPHONE_API wp_u32 wp_graphics_material_get_revision( const wp_graphics_material *mat );
WORKPHONE_API wp_shader_program *wp_graphics_material_get_shader_program(
    const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_shader_program( wp_graphics_material *mat,
                                                            wp_shader_program *program );

WORKPHONE_API void wp_graphics_material_set_sync_func( wp_graphics_material *mat,
                                                       wp_graphics_material_sync_func sync_func,
                                                       void *user_data );
WORKPHONE_API void wp_graphics_material_set_bind_func( wp_graphics_material *mat,
                                                       wp_graphics_material_bind_func bind_func,
                                                       void *user_data );
WORKPHONE_API void wp_graphics_material_set_release_func( wp_graphics_material *mat,
                                                          wp_graphics_material_release_func release_func,
                                                          void *user_data );
WORKPHONE_API wp_s32 wp_graphics_material_update( wp_graphics_material *mat );
WORKPHONE_API wp_s32 wp_graphics_material_apply( wp_graphics_material *mat, wp_renderer *renderer );

WORKPHONE_API void *wp_graphics_material_get_native( const wp_graphics_material *mat );
WORKPHONE_API void wp_graphics_material_set_native( wp_graphics_material *mat, void *native );

#ifdef __cplusplus
}
#endif

#endif
