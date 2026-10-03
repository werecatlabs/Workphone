/**
 * @file workphone_physics_material.c
 * @brief Implementation of the data-driven 3D physics material API.
 *
 * The wp_physics_material handle owns its coefficient set plus a combine mode
 * for friction and restitution.  The combine helpers here are the single
 * source of truth for how two colliding materials become one contact
 * coefficient, mirroring the policy-based combining used by the Esoterica
 * engine's material system.
 */

#include "workphone_physics_material.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

struct wp_physics_material
{
    wp_c8 name[WP_PHYSICS_MATERIAL_MAX_NAME];

    wp_f32 static_friction;
    wp_f32 dynamic_friction;
    wp_f32 rolling_friction;
    wp_f32 restitution;

    wp_friction_combine_mode friction_combine_mode;
    wp_restitution_combine_mode restitution_combine_mode;
};

/* =========================================================================
 * Local helpers
 * ====================================================================== */

static wp_f32 wp_mat_clamp_coeff( wp_f32 value )
{
    if( value < WP_PHYSICS_MATERIAL_MIN_COEFF )
    {
        return WP_PHYSICS_MATERIAL_MIN_COEFF;
    }
    if( value > WP_PHYSICS_MATERIAL_MAX_COEFF )
    {
        return WP_PHYSICS_MATERIAL_MAX_COEFF;
    }
    return value;
}

static void wp_mat_copy_name( wp_c8 *dst, const wp_c8 *src )
{
    wp_u32 i;
    if( !dst )
    {
        return;
    }
    if( !src )
    {
        dst[0] = '\0';
        return;
    }
    for( i = 0; i < ( wp_u32 )( WP_PHYSICS_MATERIAL_MAX_NAME - 1 ) && src[i] != '\0'; ++i )
    {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/* =========================================================================
 * Descriptor helpers
 * ====================================================================== */

void wp_physics_material_desc_reset( wp_physics_material_desc *desc )
{
    if( !desc )
    {
        return;
    }
    memset( desc, 0, sizeof( *desc ) );
    desc->static_friction = 0.5f;
    desc->dynamic_friction = 0.5f;
    desc->rolling_friction = 0.5f;
    desc->restitution = 0.5f;
    desc->friction_combine_mode = WORKPHONE_FRICTION_COMBINE_AVERAGE;
    desc->restitution_combine_mode = WORKPHONE_RESTITUTION_COMBINE_AVERAGE;
}

void wp_physics_material_desc_set_name( wp_physics_material_desc *desc, const wp_c8 *name )
{
    if( !desc )
    {
        return;
    }
    wp_mat_copy_name( desc->name, name );
}

wp_s32 wp_physics_material_desc_is_valid( const wp_physics_material_desc *desc )
{
    if( !desc )
    {
        return 0;
    }
    if( desc->name[0] == '\0' )
    {
        return 0;
    }
    if( desc->static_friction < WP_PHYSICS_MATERIAL_MIN_COEFF ||
        desc->static_friction > WP_PHYSICS_MATERIAL_MAX_COEFF ||
        desc->dynamic_friction < WP_PHYSICS_MATERIAL_MIN_COEFF ||
        desc->dynamic_friction > WP_PHYSICS_MATERIAL_MAX_COEFF ||
        desc->rolling_friction < WP_PHYSICS_MATERIAL_MIN_COEFF ||
        desc->rolling_friction > WP_PHYSICS_MATERIAL_MAX_COEFF ||
        desc->restitution < WP_PHYSICS_MATERIAL_MIN_COEFF ||
        desc->restitution > WP_PHYSICS_MATERIAL_MAX_COEFF )
    {
        return 0;
    }
    return 1;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_physics_material *wp_physics_material_create( void )
{
    wp_physics_material *mat = (wp_physics_material *)malloc( sizeof( wp_physics_material ) );
    if( !mat )
    {
        return NULL;
    }
    memset( mat, 0, sizeof( *mat ) );
    mat->static_friction = 0.5f;
    mat->dynamic_friction = 0.5f;
    mat->rolling_friction = 0.5f;
    mat->restitution = 0.0f;
    mat->friction_combine_mode = WORKPHONE_FRICTION_COMBINE_AVERAGE;
    mat->restitution_combine_mode = WORKPHONE_RESTITUTION_COMBINE_AVERAGE;
    return mat;
}

wp_physics_material *wp_physics_material_create_from_desc( const wp_physics_material_desc *desc )
{
    wp_physics_material *mat = wp_physics_material_create();
    if( !mat )
    {
        return NULL;
    }
    if( desc )
    {
        wp_physics_material_set_desc( mat, desc );
    }
    return mat;
}

void wp_physics_material_destroy( wp_physics_material *mat )
{
    free( mat );
}

/* =========================================================================
 * Whole-descriptor access
 * ====================================================================== */

void wp_physics_material_get_desc( const wp_physics_material *mat, wp_physics_material_desc *out_desc )
{
    if( !out_desc )
    {
        return;
    }
    if( !mat )
    {
        wp_physics_material_desc_reset( out_desc );
        return;
    }
    wp_mat_copy_name( out_desc->name, mat->name );
    out_desc->static_friction = mat->static_friction;
    out_desc->dynamic_friction = mat->dynamic_friction;
    out_desc->rolling_friction = mat->rolling_friction;
    out_desc->restitution = mat->restitution;
    out_desc->friction_combine_mode = mat->friction_combine_mode;
    out_desc->restitution_combine_mode = mat->restitution_combine_mode;
}

void wp_physics_material_set_desc( wp_physics_material *mat, const wp_physics_material_desc *desc )
{
    if( !mat || !desc )
    {
        return;
    }
    wp_mat_copy_name( mat->name, desc->name );
    mat->static_friction = wp_mat_clamp_coeff( desc->static_friction );
    mat->dynamic_friction = wp_mat_clamp_coeff( desc->dynamic_friction );
    mat->rolling_friction = wp_mat_clamp_coeff( desc->rolling_friction );
    mat->restitution = wp_mat_clamp_coeff( desc->restitution );
    mat->friction_combine_mode = desc->friction_combine_mode;
    mat->restitution_combine_mode = desc->restitution_combine_mode;
}

/* =========================================================================
 * Individual property accessors
 * ====================================================================== */

const wp_c8 *wp_physics_material_get_name( const wp_physics_material *mat )
{
    return mat ? mat->name : "";
}

void wp_physics_material_set_name( wp_physics_material *mat, const wp_c8 *name )
{
    if( !mat )
    {
        return;
    }
    wp_mat_copy_name( mat->name, name );
}

wp_f32 wp_physics_material_get_static_friction( const wp_physics_material *mat )
{
    return mat ? mat->static_friction : 0.0f;
}

void wp_physics_material_set_static_friction( wp_physics_material *mat, wp_f32 friction )
{
    if( !mat )
    {
        return;
    }
    mat->static_friction = wp_mat_clamp_coeff( friction );
}

wp_f32 wp_physics_material_get_dynamic_friction( const wp_physics_material *mat )
{
    return mat ? mat->dynamic_friction : 0.0f;
}

void wp_physics_material_set_dynamic_friction( wp_physics_material *mat, wp_f32 friction )
{
    if( !mat )
    {
        return;
    }
    mat->dynamic_friction = wp_mat_clamp_coeff( friction );
}

wp_f32 wp_physics_material_get_rolling_friction( const wp_physics_material *mat )
{
    return mat ? mat->rolling_friction : 0.0f;
}

void wp_physics_material_set_rolling_friction( wp_physics_material *mat, wp_f32 friction )
{
    if( !mat )
    {
        return;
    }
    mat->rolling_friction = wp_mat_clamp_coeff( friction );
}

wp_f32 wp_physics_material_get_restitution( const wp_physics_material *mat )
{
    return mat ? mat->restitution : 0.0f;
}

void wp_physics_material_set_restitution( wp_physics_material *mat, wp_f32 restitution )
{
    if( !mat )
    {
        return;
    }
    mat->restitution = wp_mat_clamp_coeff( restitution );
}

wp_friction_combine_mode wp_physics_material_get_friction_combine_mode( const wp_physics_material *mat )
{
    return mat ? mat->friction_combine_mode : WORKPHONE_FRICTION_COMBINE_AVERAGE;
}

void wp_physics_material_set_friction_combine_mode( wp_physics_material *mat, wp_friction_combine_mode mode )
{
    if( !mat )
    {
        return;
    }
    mat->friction_combine_mode = mode;
}

wp_restitution_combine_mode wp_physics_material_get_restitution_combine_mode( const wp_physics_material *mat )
{
    return mat ? mat->restitution_combine_mode : WORKPHONE_RESTITUTION_COMBINE_AVERAGE;
}

void wp_physics_material_set_restitution_combine_mode( wp_physics_material *mat, wp_restitution_combine_mode mode )
{
    if( !mat )
    {
        return;
    }
    mat->restitution_combine_mode = mode;
}

/* =========================================================================
 * Combine helpers
 * ====================================================================== */

wp_f32 wp_physics_material_combine_friction_value( wp_f32 a, wp_f32 b, wp_friction_combine_mode mode )
{
    switch( mode )
    {
        case WORKPHONE_FRICTION_COMBINE_MIN:
            return a < b ? a : b;
        case WORKPHONE_FRICTION_COMBINE_MAX:
            return a > b ? a : b;
        case WORKPHONE_FRICTION_COMBINE_MULTIPLY:
            return a * b;
        case WORKPHONE_FRICTION_COMBINE_AVERAGE:
        default:
            return ( a + b ) * 0.5f;
    }
}

wp_f32 wp_physics_material_combine_restitution_value( wp_f32 a, wp_f32 b, wp_restitution_combine_mode mode )
{
    switch( mode )
    {
        case WORKPHONE_RESTITUTION_COMBINE_MIN:
            return a < b ? a : b;
        case WORKPHONE_RESTITUTION_COMBINE_MAX:
            return a > b ? a : b;
        case WORKPHONE_RESTITUTION_COMBINE_MULTIPLY:
            return a * b;
        case WORKPHONE_RESTITUTION_COMBINE_AVERAGE:
        default:
            return ( a + b ) * 0.5f;
    }
}

wp_f32 wp_physics_material_combine_friction( const wp_physics_material *a, const wp_physics_material *b )
{
    wp_f32 va;
    wp_f32 vb;
    if( !a && !b )
    {
        return 0.0f;
    }
    if( !a )
    {
        return wp_mat_clamp_coeff( b->dynamic_friction );
    }
    if( !b )
    {
        return wp_mat_clamp_coeff( a->dynamic_friction );
    }
    va = a->dynamic_friction;
    vb = b->dynamic_friction;
    return wp_mat_clamp_coeff(
        wp_physics_material_combine_friction_value( va, vb, a->friction_combine_mode ) );
}

wp_f32 wp_physics_material_combine_restitution( const wp_physics_material *a, const wp_physics_material *b )
{
    wp_f32 va;
    wp_f32 vb;
    if( !a && !b )
    {
        return 0.0f;
    }
    if( !a )
    {
        return wp_mat_clamp_coeff( b->restitution );
    }
    if( !b )
    {
        return wp_mat_clamp_coeff( a->restitution );
    }
    va = a->restitution;
    vb = b->restitution;
    return wp_mat_clamp_coeff(
        wp_physics_material_combine_restitution_value( va, vb, a->restitution_combine_mode ) );
}
