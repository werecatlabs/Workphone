/**
 * @file workphone_physics_collisionsettings.h
 * @brief C API for data-driven collision settings (category + mask).
 *
 * A collision settings block answers two questions for a physics shape:
 *   - "What am I?"        (a single ObjectCategory bit)
 *   - "What do I hit?"     (a 32-bit mask of categories it collides with)
 *
 * This mirrors the Esoterica engine's CollisionSettings struct, keeping
 * gameplay/query categories in a single bit space so that the same mask can be
 * reused for collision filtering and spatial queries (navigation, visibility,
 * camera, IK).  The lioncat runtime already uses 32-bit collision_type /
 * collision_mask bitmasks on shapes and bodies, so this header stays in the
 * same 32-bit space and maps cleanly onto those existing bitmasks.
 *
 * @see workphone_physics_collisionshape.h
 */

#ifndef WORKPHONE_PHYSICS_COLLISIONSETTINGS_H
#define WORKPHONE_PHYSICS_COLLISIONSETTINGS_H

#include <stdint.h>
#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Object categories (colliders).  Bits 0..15 are reserved for object
 * categories; bits 16..31 are reserved for query-only channels.
 * Ported from Esoterica ObjectCategory.
 * ---------------------------------------------------------------------- */

typedef enum wp_object_category
{
    WORKPHONE_OBJECT_CATEGORY_ENVIRONMENT = 0,
    WORKPHONE_OBJECT_CATEGORY_CHARACTER = 1,
    WORKPHONE_OBJECT_CATEGORY_PROP = 2,
    WORKPHONE_OBJECT_CATEGORY_PROJECTILE = 3,
    WORKPHONE_OBJECT_CATEGORY_DESTRUCTIBLE = 4,
    WORKPHONE_OBJECT_CATEGORY_COUNT = 5
} wp_object_category;

/** @brief Query-only channels (never assigned to a collider). */
typedef enum wp_query_category
{
    WORKPHONE_QUERY_CATEGORY_NAVIGATION = 16,
    WORKPHONE_QUERY_CATEGORY_VISIBILITY = 17,
    WORKPHONE_QUERY_CATEGORY_CAMERA = 18,
    WORKPHONE_QUERY_CATEGORY_IK = 19
} wp_query_category;

/* -------------------------------------------------------------------------
 * Collision settings
 * ---------------------------------------------------------------------- */

typedef struct wp_collision_settings
{
    /** What is this shape? A single ObjectCategory value. */
    wp_object_category category;
    /** 32-bit mask of categories (object + query) this shape collides with. */
    wp_u32 collision_mask;
} wp_collision_settings;

/** @brief Returns settings initialised to Environment colliding with everything. */
wp_collision_settings wp_collision_settings_make_default( void );

/** @brief Sets the bit for an object category in a mask. */
wp_u32 wp_collision_settings_set_category_bit( wp_u32 mask, wp_object_category category );

/** @brief Clears the bit for an object category in a mask. */
wp_u32 wp_collision_settings_clear_category_bit( wp_u32 mask, wp_object_category category );

/** @brief Returns non-zero if a mask has the given object category bit set. */
wp_s32 wp_collision_settings_has_category( wp_u32 mask, wp_object_category category );

/** @brief Sets the bit for a query category in a mask. */
wp_u32 wp_collision_settings_set_query_bit( wp_u32 mask, wp_query_category category );

/** @brief Clears the bit for a query category in a mask. */
wp_u32 wp_collision_settings_clear_query_bit( wp_u32 mask, wp_query_category category );

/** @brief Returns non-zero if a mask has the given query category bit set. */
wp_s32 wp_collision_settings_has_query( wp_u32 mask, wp_query_category category );

/**
 * @brief Two shapes collide if (a.category bit) is set in b.mask AND
 *        (b.category bit) is set in a.mask.
 */
wp_s32 wp_collision_settings_should_collide( const wp_collision_settings *a,
                                             const wp_collision_settings *b );

/** @brief Converts collision settings to the legacy 32-bit collision_type word. */
wp_u32 wp_collision_settings_to_collision_type( const wp_collision_settings *settings );

/** @brief Converts collision settings to the legacy 32-bit collision_mask word. */
wp_u32 wp_collision_settings_to_collision_mask( const wp_collision_settings *settings );

/** @brief Builds collision settings from legacy 32-bit type/mask words. */
wp_collision_settings wp_collision_settings_from_legacy( wp_u32 collision_type, wp_u32 collision_mask );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_PHYSICS_COLLISIONSETTINGS_H */
