/**
 * @file workphone_physics_contact.c
 * @brief Contact manifold helper implementation.
 */

#include "workphone_physics_contact.h"
#include <string.h>

void wp_contact_point_reset( wp_contact_point *point )
{
    if( point )
    {
        memset( point, 0, sizeof( wp_contact_point ) );
    }
}

void wp_contact_manifold_reset( wp_contact_manifold *manifold )
{
    if( manifold )
    {
        memset( manifold, 0, sizeof( wp_contact_manifold ) );
    }
}

wp_s32 wp_contact_manifold_add_point( wp_contact_manifold *manifold, wp_contact_point point )
{
    if( !manifold || manifold->contact_count >= WP_MANIFOLD_MAX_CONTACTS )
    {
        return 0;
    }
    manifold->contacts[manifold->contact_count++] = point;
    manifold->is_touching = manifold->contact_count > 0;
    return 1;
}

wp_contact_point *wp_contact_manifold_get_point( wp_contact_manifold *manifold, wp_s32 index )
{
    if( !manifold || index < 0 || index >= manifold->contact_count )
    {
        return NULL;
    }
    return &manifold->contacts[index];
}

const wp_contact_point *wp_contact_manifold_get_point_const( const wp_contact_manifold *manifold,
                                                             wp_s32 index )
{
    if( !manifold || index < 0 || index >= manifold->contact_count )
    {
        return NULL;
    }
    return &manifold->contacts[index];
}

wp_f32 wp_contact_manifold_get_max_penetration( const wp_contact_manifold *manifold )
{
    wp_s32 i;
    wp_f32 max_penetration = 0.0f;
    if( !manifold )
    {
        return 0.0f;
    }
    for( i = 0; i < manifold->contact_count; ++i )
    {
        if( i == 0 || manifold->contacts[i].penetration_depth > max_penetration )
        {
            max_penetration = manifold->contacts[i].penetration_depth;
        }
    }
    return max_penetration;
}
