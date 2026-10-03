/**
 * @file workphone_physics_material.h
 * @brief C API for 3D physics surface materials (data-driven).
 *
 * A physics material describes the physical surface properties used by the
 * solver when two shapes collide: static and dynamic friction, rolling
 * friction (for spheres/capsules), restitution (bounciness), and the combine
 * modes that decide how the two colliding materials' coefficients are merged
 * into a single contact coefficient.
 *
 * This header replaces the minimal opaque material that used to live entirely
 * inside workphone_physics.c.  The descriptor-based design is data-driven and
 * mirrors the Esoterica engine's reflected Material / MaterialDatabase /
 * MaterialRegistry pattern so that material libraries can be authored in the
 * editor, serialised, registered by name, and resolved at runtime.
 *
 * @see workphone_physics_material_registry.h
 */

#ifndef WORKPHONE_PHYSICS_MATERIAL_H
#define WORKPHONE_PHYSICS_MATERIAL_H

#include <stdint.h>
#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Limits (ported from Esoterica's reflected Min/Max metadata)
 * ---------------------------------------------------------------------- */

/** Minimum allowed value for friction / restitution / rolling coefficients. */
#define WP_PHYSICS_MATERIAL_MIN_COEFF ( 0.0f )
/** Maximum allowed value for friction / restitution / rolling coefficients. */
#define WP_PHYSICS_MATERIAL_MAX_COEFF ( 1.0f )

#ifndef WP_PHYSICS_MATERIAL_MAX_NAME
#    define WP_PHYSICS_MATERIAL_MAX_NAME 64
#endif

/* -------------------------------------------------------------------------
 * Combine modes
 * ---------------------------------------------------------------------- */

/**
 * @brief How to combine two materials' friction coefficients into one.
 *
 * Mirrors the standard combine policies used by PhysX/Box3D style solvers.
 */
typedef enum wp_friction_combine_mode
{
    /** Use the average of the two coefficients: (a + b) * 0.5.   */
    WORKPHONE_FRICTION_COMBINE_AVERAGE = 0,
    /** Use the smaller of the two coefficients.                  */
    WORKPHONE_FRICTION_COMBINE_MIN = 1,
    /** Use the larger of the two coefficients.                   */
    WORKPHONE_FRICTION_COMBINE_MAX = 2,
    /** Use the product of the two coefficients: a * b.           */
    WORKPHONE_FRICTION_COMBINE_MULTIPLY = 3
} wp_friction_combine_mode;

/** @brief How to combine two materials' restitution coefficients into one. */
typedef enum wp_restitution_combine_mode
{
    /** Use the average of the two coefficients: (a + b) * 0.5.   */
    WORKPHONE_RESTITUTION_COMBINE_AVERAGE = 0,
    /** Use the smaller of the two coefficients.                  */
    WORKPHONE_RESTITUTION_COMBINE_MIN = 1,
    /** Use the larger of the two coefficients.                   */
    WORKPHONE_RESTITUTION_COMBINE_MAX = 2,
    /** Use the product of the two coefficients: a * b.           */
    WORKPHONE_RESTITUTION_COMBINE_MULTIPLY = 3
} wp_restitution_combine_mode;

/* -------------------------------------------------------------------------
 * Opaque material handle
 * ---------------------------------------------------------------------- */

typedef struct wp_physics_material wp_physics_material;

/* -------------------------------------------------------------------------
 * Data-driven descriptor (the authoring/serialisation unit)
 *
 * This is the plain-old-data view of a material.  It is what the material
 * database serialises and what the editor edits.  A material handle is
 * created from a descriptor and can be read back into one.
 * ---------------------------------------------------------------------- */

typedef struct wp_physics_material_desc
{
    /** Unique material name (case-sensitive lookup key). May be empty. */
    wp_c8 name[WP_PHYSICS_MATERIAL_MAX_NAME];

    /** Static friction coefficient - [0, 1]. */
    wp_f32 static_friction;
    /** Dynamic (kinetic) friction coefficient - [0, 1]. */
    wp_f32 dynamic_friction;
    /** Rolling friction for spheres/capsules - [0, 1]. */
    wp_f32 rolling_friction;
    /** Restitution (bounciness) - [0, 1]. */
    wp_f32 restitution;

    /** Combine mode used when this material's friction meets another's. */
    wp_friction_combine_mode friction_combine_mode;
    /** Combine mode used when this material's restitution meets another's. */
    wp_restitution_combine_mode restitution_combine_mode;
} wp_physics_material_desc;

/* -------------------------------------------------------------------------
 * Descriptor helpers
 * ---------------------------------------------------------------------- */

/** @brief Resets a descriptor to sane defaults (name empty, 0.5 friction). */
void wp_physics_material_desc_reset( wp_physics_material_desc *desc );

/** @brief Sets the material name (truncates to WP_PHYSICS_MATERIAL_MAX_NAME). */
void wp_physics_material_desc_set_name( wp_physics_material_desc *desc, const wp_c8 *name );

/** @brief Returns non-zero if the descriptor has a valid, clamped value set. */
wp_s32 wp_physics_material_desc_is_valid( const wp_physics_material_desc *desc );

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/** @brief Creates a material initialised to default coefficients. */
wp_physics_material *wp_physics_material_create( void );

/** @brief Creates a material from a data-driven descriptor. */
wp_physics_material *wp_physics_material_create_from_desc( const wp_physics_material_desc *desc );

/** @brief Destroys a material created by wp_physics_material_create*. */
void wp_physics_material_destroy( wp_physics_material *mat );

/* -------------------------------------------------------------------------
 * Whole-descriptor access (data-driven bulk get/set)
 * ---------------------------------------------------------------------- */

void wp_physics_material_get_desc( const wp_physics_material *mat, wp_physics_material_desc *out_desc );
void wp_physics_material_set_desc( wp_physics_material *mat, const wp_physics_material_desc *desc );

/* -------------------------------------------------------------------------
 * Individual property accessors
 * ---------------------------------------------------------------------- */

const wp_c8 *wp_physics_material_get_name( const wp_physics_material *mat );
void wp_physics_material_set_name( wp_physics_material *mat, const wp_c8 *name );

wp_f32 wp_physics_material_get_static_friction( const wp_physics_material *mat );
void wp_physics_material_set_static_friction( wp_physics_material *mat, wp_f32 friction );

wp_f32 wp_physics_material_get_dynamic_friction( const wp_physics_material *mat );
void wp_physics_material_set_dynamic_friction( wp_physics_material *mat, wp_f32 friction );

wp_f32 wp_physics_material_get_rolling_friction( const wp_physics_material *mat );
void wp_physics_material_set_rolling_friction( wp_physics_material *mat, wp_f32 friction );

wp_f32 wp_physics_material_get_restitution( const wp_physics_material *mat );
void wp_physics_material_set_restitution( wp_physics_material *mat, wp_f32 restitution );

wp_friction_combine_mode wp_physics_material_get_friction_combine_mode( const wp_physics_material *mat );
void wp_physics_material_set_friction_combine_mode( wp_physics_material *mat, wp_friction_combine_mode mode );

wp_restitution_combine_mode wp_physics_material_get_restitution_combine_mode( const wp_physics_material *mat );
void wp_physics_material_set_restitution_combine_mode( wp_physics_material *mat, wp_restitution_combine_mode mode );

/* -------------------------------------------------------------------------
 * Combine helpers
 *
 * Resolve a single contact coefficient from two materials according to the
 * first material's combine mode.  These are exposed so the solver, contact
 * callbacks, and editor previews can share the exact same combine logic.
 * ---------------------------------------------------------------------- */

wp_f32 wp_physics_material_combine_friction( const wp_physics_material *a, const wp_physics_material *b );
wp_f32 wp_physics_material_combine_restitution( const wp_physics_material *a, const wp_physics_material *b );

/** @brief Combines two raw friction coefficients with the given mode. */
wp_f32 wp_physics_material_combine_friction_value( wp_f32 a, wp_f32 b, wp_friction_combine_mode mode );

/** @brief Combines two raw restitution coefficients with the given mode. */
wp_f32 wp_physics_material_combine_restitution_value( wp_f32 a, wp_f32 b, wp_restitution_combine_mode mode );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_PHYSICS_MATERIAL_H */
