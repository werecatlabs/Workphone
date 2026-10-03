/**
 * @file workphone_graphics_light.h
 * @brief C89 API for a scene light.
 */

#ifndef WORKPHONE_GRAPHICS_LIGHT_H
#define WORKPHONE_GRAPHICS_LIGHT_H

#include "workphone_color.h"
#include "workphone_graphics_node.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_light wp_light;
typedef struct wp_scenenode wp_scenenode;
typedef struct wp_graphics_scene wp_graphics_scene;

/** Light source models supported by WorkphoneGraphics. */
typedef enum wp_light_type
{
    WORKPHONE_LIGHT_DIRECTIONAL = 0,
    WORKPHONE_LIGHT_POINT = 1,
    WORKPHONE_LIGHT_SPOTLIGHT = 2,
    WORKPHONE_LIGHT_VPL = 3,
    WORKPHONE_LIGHT_AREA_APPROX = 4,
    WORKPHONE_LIGHT_AREA_LTC = 5,
    WORKPHONE_LIGHT_TYPE_COUNT = 6
} wp_light_type;

/** Creates a light with directional-light defaults. */
wp_light *wp_light_create( void );

/** Destroys a light. Attached nodes and native objects are not owned. */
void wp_light_destroy( wp_light *light );

void wp_light_set_type( wp_light *light, wp_light_type type );
wp_light_type wp_light_get_type( const wp_light *light );

void wp_light_set_diffuse_colour( wp_light *light, wp_colour_f colour );
wp_colour_f wp_light_get_diffuse_colour( const wp_light *light );

void wp_light_set_specular_colour( wp_light *light, wp_colour_f colour );
wp_colour_f wp_light_get_specular_colour( const wp_light *light );

/**
 * Sets the light position. If attached to a scene node, this updates that
 * node's local position.
 */
void wp_light_set_position( wp_light *light, wp_vec3f position );
wp_vec3f wp_light_get_position( const wp_light *light );

/**
 * Sets a normalized local-space direction. A zero vector is ignored.
 */
void wp_light_set_direction( wp_light *light, wp_vec3f direction );
wp_vec3f wp_light_get_direction( const wp_light *light );

/**
 * Returns the direction transformed by the orientation of the attached node
 * and its ancestors.
 */
wp_vec3f wp_light_get_derived_direction( const wp_light *light );

void wp_light_set_attenuation( wp_light *light, wp_f32 range, wp_f32 constant, wp_f32 linear,
                               wp_f32 quadratic );
wp_f32 wp_light_get_attenuation_range( const wp_light *light );
wp_f32 wp_light_get_attenuation_constant( const wp_light *light );
wp_f32 wp_light_get_attenuation_linear( const wp_light *light );
wp_f32 wp_light_get_attenuation_quadratic( const wp_light *light );

void wp_light_set_power_scale( wp_light *light, wp_f32 power_scale );
wp_f32 wp_light_get_power_scale( const wp_light *light );

/**
 * Configures a spotlight cone in radians. Values are clamped so that
 * 0 <= inner_angle <= outer_angle <= PI and falloff is non-negative.
 */
void wp_light_set_spotlight_range( wp_light *light, wp_f32 inner_angle, wp_f32 outer_angle,
                                   wp_f32 falloff );
wp_f32 wp_light_get_spotlight_inner_angle( const wp_light *light );
wp_f32 wp_light_get_spotlight_outer_angle( const wp_light *light );
wp_f32 wp_light_get_spotlight_falloff( const wp_light *light );

void wp_light_set_visible( wp_light *light, wp_s32 visible );
wp_s32 wp_light_is_visible( const wp_light *light );

void wp_light_set_cast_shadows( wp_light *light, wp_s32 cast_shadows );
wp_s32 wp_light_get_cast_shadows( const wp_light *light );

void wp_light_set_visibility_mask( wp_light *light, wp_u32 mask );
wp_u32 wp_light_get_visibility_mask( const wp_light *light );

void wp_light_attach_to_node( wp_light *light, wp_scenenode *node );
void wp_light_detach_from_node( wp_light *light, wp_scenenode *node );
wp_scenenode *wp_light_get_node( const wp_light *light );

void wp_light_set_creator( wp_light *light, wp_graphics_scene *creator );
wp_graphics_scene *wp_light_get_creator( const wp_light *light );

/**
 * Evaluates distance attenuation including power scale. Directional lights
 * return their power scale. Other lights return zero outside their range.
 */
wp_f32 wp_light_get_attenuation_factor( const wp_light *light, wp_f32 distance );

void wp_light_get_native( const wp_light *light, void **pp_object );
void wp_light_set_native( wp_light *light, void *native );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_LIGHT_H */
