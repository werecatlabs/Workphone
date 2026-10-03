/**
 * @file workphone_graphics_light.c
 * @brief Implementation of the C89 scene light API.
 */

#include "workphone_graphics_light.h"
#include "workphone_graphics_scenenode.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct wp_light
{
    wp_light_type type;
    wp_colour_f diffuse_colour;
    wp_colour_f specular_colour;
    wp_vec3f position;
    wp_vec3f direction;
    wp_f32 attenuation_range;
    wp_f32 attenuation_constant;
    wp_f32 attenuation_linear;
    wp_f32 attenuation_quadratic;
    wp_f32 power_scale;
    wp_f32 spotlight_inner_angle;
    wp_f32 spotlight_outer_angle;
    wp_f32 spotlight_falloff;
    wp_u32 visibility_mask;
    wp_s32 visible;
    wp_s32 cast_shadows;
    wp_scenenode *node;
    wp_graphics_scene *creator;
    void *native;
};

static wp_colour_f light_zero_colour( void )
{
    wp_colour_f colour;
    memset( &colour, 0, sizeof( colour ) );
    return colour;
}

static wp_vec3f light_zero_vector( void )
{
    wp_vec3f vector;
    memset( &vector, 0, sizeof( vector ) );
    return vector;
}

static wp_f32 light_clamp( wp_f32 value, wp_f32 minimum, wp_f32 maximum )
{
    if( value < minimum )
    {
        return minimum;
    }
    if( value > maximum )
    {
        return maximum;
    }
    return value;
}

static wp_vec3f light_rotate_vector( wp_quatf orientation, wp_vec3f vector )
{
    wp_vec3f axis;
    wp_vec3f cross;
    wp_vec3f result;
    wp_f32 length_sq;
    wp_f32 inverse_length;
    wp_f32 axis_dot_vector;
    wp_f32 axis_dot_axis;

    length_sq = orientation.x * orientation.x + orientation.y * orientation.y +
                orientation.z * orientation.z + orientation.w * orientation.w;
    if( length_sq <= 0.0000001f )
    {
        return vector;
    }

    inverse_length = 1.0f / sqrtf( length_sq );
    orientation.x *= inverse_length;
    orientation.y *= inverse_length;
    orientation.z *= inverse_length;
    orientation.w *= inverse_length;

    axis.x = orientation.x;
    axis.y = orientation.y;
    axis.z = orientation.z;
    cross = wp_vec3f_cross( axis, vector );
    axis_dot_vector = wp_vec3f_dot( axis, vector );
    axis_dot_axis = wp_vec3f_dot( axis, axis );

    result = wp_vec3f_scale( axis, 2.0f * axis_dot_vector );
    result =
        wp_vec3f_add( result, wp_vec3f_scale( vector, orientation.w * orientation.w - axis_dot_axis ) );
    result = wp_vec3f_add( result, wp_vec3f_scale( cross, 2.0f * orientation.w ) );
    return result;
}

wp_light *wp_light_create( void )
{
    wp_light *light;

    light = (wp_light *)malloc( sizeof( wp_light ) );
    if( !light )
    {
        return NULL;
    }

    memset( light, 0, sizeof( wp_light ) );
    light->type = WORKPHONE_LIGHT_DIRECTIONAL;
    light->diffuse_colour.r = 1.0f;
    light->diffuse_colour.g = 1.0f;
    light->diffuse_colour.b = 1.0f;
    light->diffuse_colour.a = 1.0f;
    light->specular_colour = light->diffuse_colour;
    light->direction.y = -1.0f;
    light->attenuation_range = 1000.0f;
    light->attenuation_constant = 1.0f;
    light->attenuation_linear = 1.0f;
    light->attenuation_quadratic = 1.0f;
    light->power_scale = 1.0f;
    light->spotlight_inner_angle = WORKPHONE_PI_F / 6.0f;
    light->spotlight_outer_angle = WORKPHONE_PI_F / 4.0f;
    light->spotlight_falloff = 1.0f;
    light->visibility_mask = 0xFFFFFFFFu;
    light->visible = 1;
    light->cast_shadows = 1;
    return light;
}

void wp_light_destroy( wp_light *light )
{
    free( light );
}

void wp_light_set_type( wp_light *light, wp_light_type type )
{
    if( light && type >= WORKPHONE_LIGHT_DIRECTIONAL && type < WORKPHONE_LIGHT_TYPE_COUNT )
    {
        light->type = type;
    }
}

wp_light_type wp_light_get_type( const wp_light *light )
{
    return light ? light->type : WORKPHONE_LIGHT_DIRECTIONAL;
}

void wp_light_set_diffuse_colour( wp_light *light, wp_colour_f colour )
{
    if( light )
    {
        light->diffuse_colour = colour;
    }
}

wp_colour_f wp_light_get_diffuse_colour( const wp_light *light )
{
    return light ? light->diffuse_colour : light_zero_colour();
}

void wp_light_set_specular_colour( wp_light *light, wp_colour_f colour )
{
    if( light )
    {
        light->specular_colour = colour;
    }
}

wp_colour_f wp_light_get_specular_colour( const wp_light *light )
{
    return light ? light->specular_colour : light_zero_colour();
}

void wp_light_set_position( wp_light *light, wp_vec3f position )
{
    if( !light )
    {
        return;
    }

    if( light->node )
    {
        wp_scenenode_set_position( light->node, position );
    }
    else
    {
        light->position = position;
    }
}

wp_vec3f wp_light_get_position( const wp_light *light )
{
    if( !light )
    {
        return light_zero_vector();
    }
    if( light->node )
    {
        return wp_scenenode_get_position( light->node );
    }
    return light->position;
}

void wp_light_set_direction( wp_light *light, wp_vec3f direction )
{
    if( light && !wp_vec3f_is_zero_length( direction ) )
    {
        light->direction = wp_vec3f_normalize( direction );
    }
}

wp_vec3f wp_light_get_direction( const wp_light *light )
{
    if( !light )
    {
        return light_zero_vector();
    }
    return light->direction;
}

wp_vec3f wp_light_get_derived_direction( const wp_light *light )
{
    wp_vec3f direction;
    wp_scenenode *node;

    if( !light )
    {
        return light_zero_vector();
    }

    direction = light->direction;
    node = light->node;
    while( node )
    {
        direction = light_rotate_vector( wp_scenenode_get_orientation( node ), direction );
        node = wp_scenenode_get_parent( node );
    }

    if( wp_vec3f_is_zero_length( direction ) )
    {
        return light_zero_vector();
    }
    return wp_vec3f_normalize( direction );
}

void wp_light_set_attenuation( wp_light *light, wp_f32 range, wp_f32 constant, wp_f32 linear,
                               wp_f32 quadratic )
{
    if( !light )
    {
        return;
    }

    light->attenuation_range = range < 0.0f ? 0.0f : range;
    light->attenuation_constant = constant < 0.0f ? 0.0f : constant;
    light->attenuation_linear = linear < 0.0f ? 0.0f : linear;
    light->attenuation_quadratic = quadratic < 0.0f ? 0.0f : quadratic;
}

wp_f32 wp_light_get_attenuation_range( const wp_light *light )
{
    return light ? light->attenuation_range : 0.0f;
}

wp_f32 wp_light_get_attenuation_constant( const wp_light *light )
{
    return light ? light->attenuation_constant : 0.0f;
}

wp_f32 wp_light_get_attenuation_linear( const wp_light *light )
{
    return light ? light->attenuation_linear : 0.0f;
}

wp_f32 wp_light_get_attenuation_quadratic( const wp_light *light )
{
    return light ? light->attenuation_quadratic : 0.0f;
}

void wp_light_set_power_scale( wp_light *light, wp_f32 power_scale )
{
    if( light )
    {
        light->power_scale = power_scale < 0.0f ? 0.0f : power_scale;
    }
}

wp_f32 wp_light_get_power_scale( const wp_light *light )
{
    return light ? light->power_scale : 0.0f;
}

void wp_light_set_spotlight_range( wp_light *light, wp_f32 inner_angle, wp_f32 outer_angle,
                                   wp_f32 falloff )
{
    if( !light )
    {
        return;
    }

    outer_angle = light_clamp( outer_angle, 0.0f, WORKPHONE_PI_F );
    inner_angle = light_clamp( inner_angle, 0.0f, outer_angle );
    light->spotlight_inner_angle = inner_angle;
    light->spotlight_outer_angle = outer_angle;
    light->spotlight_falloff = falloff < 0.0f ? 0.0f : falloff;
}

wp_f32 wp_light_get_spotlight_inner_angle( const wp_light *light )
{
    return light ? light->spotlight_inner_angle : 0.0f;
}

wp_f32 wp_light_get_spotlight_outer_angle( const wp_light *light )
{
    return light ? light->spotlight_outer_angle : 0.0f;
}

wp_f32 wp_light_get_spotlight_falloff( const wp_light *light )
{
    return light ? light->spotlight_falloff : 0.0f;
}

void wp_light_set_visible( wp_light *light, wp_s32 visible )
{
    if( light )
    {
        light->visible = visible != 0;
    }
}

wp_s32 wp_light_is_visible( const wp_light *light )
{
    return light ? light->visible : 0;
}

void wp_light_set_cast_shadows( wp_light *light, wp_s32 cast_shadows )
{
    if( light )
    {
        light->cast_shadows = cast_shadows != 0;
    }
}

wp_s32 wp_light_get_cast_shadows( const wp_light *light )
{
    return light ? light->cast_shadows : 0;
}

void wp_light_set_visibility_mask( wp_light *light, wp_u32 mask )
{
    if( light )
    {
        light->visibility_mask = mask;
    }
}

wp_u32 wp_light_get_visibility_mask( const wp_light *light )
{
    return light ? light->visibility_mask : 0u;
}

void wp_light_attach_to_node( wp_light *light, wp_scenenode *node )
{
    if( light )
    {
        light->node = node;
    }
}

void wp_light_detach_from_node( wp_light *light, wp_scenenode *node )
{
    if( light && light->node == node )
    {
        light->node = NULL;
    }
}

wp_scenenode *wp_light_get_node( const wp_light *light )
{
    return light ? light->node : NULL;
}

void wp_light_set_creator( wp_light *light, wp_graphics_scene *creator )
{
    if( light )
    {
        light->creator = creator;
    }
}

wp_graphics_scene *wp_light_get_creator( const wp_light *light )
{
    return light ? light->creator : NULL;
}

wp_f32 wp_light_get_attenuation_factor( const wp_light *light, wp_f32 distance )
{
    wp_f32 denominator;

    if( !light || !light->visible )
    {
        return 0.0f;
    }
    if( light->type == WORKPHONE_LIGHT_DIRECTIONAL )
    {
        return light->power_scale;
    }

    distance = distance < 0.0f ? -distance : distance;
    if( distance > light->attenuation_range )
    {
        return 0.0f;
    }

    denominator = light->attenuation_constant + light->attenuation_linear * distance +
                  light->attenuation_quadratic * distance * distance;
    if( denominator <= 0.0000001f )
    {
        return light->power_scale;
    }
    return light->power_scale / denominator;
}

void wp_light_get_native( const wp_light *light, void **pp_object )
{
    if( pp_object )
    {
        *pp_object = light ? light->native : NULL;
    }
}

void wp_light_set_native( wp_light *light, void *native )
{
    if( light )
    {
        light->native = native;
    }
}
