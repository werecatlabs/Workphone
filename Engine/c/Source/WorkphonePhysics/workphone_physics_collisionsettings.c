/**
 * @file workphone_physics_collisionsettings.c
 * @brief Implementation of data-driven collision settings (category + mask).
 */

#include "workphone_physics_collisionsettings.h"

wp_collision_settings wp_collision_settings_make_default( void )
{
    wp_collision_settings s;
    s.category = WORKPHONE_OBJECT_CATEGORY_ENVIRONMENT;
    s.collision_mask = 0xFFFFFFFFu;
    return s;
}

wp_u32 wp_collision_settings_set_category_bit( wp_u32 mask, wp_object_category category )
{
    return mask | ( (wp_u32)1 << (wp_u32)category );
}

wp_u32 wp_collision_settings_clear_category_bit( wp_u32 mask, wp_object_category category )
{
    return mask & ~( (wp_u32)1 << (wp_u32)category );
}

wp_s32 wp_collision_settings_has_category( wp_u32 mask, wp_object_category category )
{
    return ( mask & ( (wp_u32)1 << (wp_u32)category ) ) != 0;
}

wp_u32 wp_collision_settings_set_query_bit( wp_u32 mask, wp_query_category category )
{
    return mask | ( (wp_u32)1 << (wp_u32)category );
}

wp_u32 wp_collision_settings_clear_query_bit( wp_u32 mask, wp_query_category category )
{
    return mask & ~( (wp_u32)1 << (wp_u32)category );
}

wp_s32 wp_collision_settings_has_query( wp_u32 mask, wp_query_category category )
{
    return ( mask & ( (wp_u32)1 << (wp_u32)category ) ) != 0;
}

wp_s32 wp_collision_settings_should_collide( const wp_collision_settings *a,
                                             const wp_collision_settings *b )
{
    wp_u32 a_bit;
    wp_u32 b_bit;
    if( !a || !b )
    {
        return 0;
    }
    a_bit = (wp_u32)1 << (wp_u32)a->category;
    b_bit = (wp_u32)1 << (wp_u32)b->category;
    return ( ( b->collision_mask & a_bit ) != 0 ) && ( ( a->collision_mask & b_bit ) != 0 );
}

wp_u32 wp_collision_settings_to_collision_type( const wp_collision_settings *settings )
{
    if( !settings )
    {
        return 0;
    }
    return settings->collision_mask;
}

wp_u32 wp_collision_settings_to_collision_mask( const wp_collision_settings *settings )
{
    return wp_collision_settings_to_collision_type( settings );
}

wp_collision_settings wp_collision_settings_from_legacy( wp_u32 collision_type, wp_u32 collision_mask )
{
    wp_collision_settings s;
    s.category = WORKPHONE_OBJECT_CATEGORY_ENVIRONMENT;
    s.collision_mask = collision_mask;
    (void)collision_type;
    return s;
}
