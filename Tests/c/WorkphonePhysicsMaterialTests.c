/**
 * @file WorkphonePhysicsMaterialTests.c
 * @brief Tests for the data-driven physics material system (Esoterica-style
 *        Material / MaterialRegistry / MaterialDatabase / CollisionSettings
 *        ported into the WorkphonePhysics C layer).
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "workphone_physics.h"
#include "workphone_physics_material.h"
#include "workphone_physics_material_registry.h"
#include "workphone_physics_collisionsettings.h"
#include "workphone_physics_collisionshape.h"
#include "workphone_physics_rigidbody.h"

#define CHECK( condition )                                                        \
    do                                                                            \
    {                                                                             \
        if( !( condition ) )                                                      \
        {                                                                         \
            fprintf( stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,    \
                     #condition );                                                \
            return 0;                                                             \
        }                                                                         \
    } while( 0 )

static int approx( wp_f32 a, wp_f32 b )
{
    return fabsf( a - b ) < 1.0e-5f;
}

/* ------------------------------------------------------------------------- */

static int test_material_defaults_and_clamp( void )
{
    wp_physics_material *m = wp_physics_material_create();
    CHECK( m );
    CHECK( approx( wp_physics_material_get_static_friction( m ), 0.5f ) );
    CHECK( approx( wp_physics_material_get_dynamic_friction( m ), 0.5f ) );
    CHECK( approx( wp_physics_material_get_rolling_friction( m ), 0.5f ) );
    CHECK( approx( wp_physics_material_get_restitution( m ), 0.0f ) );

    wp_physics_material_set_static_friction( m, 5.0f );
    CHECK( approx( wp_physics_material_get_static_friction( m ), 1.0f ) ); /* clamped to max */
    wp_physics_material_set_restitution( m, -1.0f );
    CHECK( approx( wp_physics_material_get_restitution( m ), 0.0f ) );     /* clamped to min */

    wp_physics_material_set_name( m, "Ice" );
    CHECK( strcmp( wp_physics_material_get_name( m ), "Ice" ) == 0 );

    wp_physics_material_destroy( m );
    return 1;
}

static int test_descriptor_roundtrip( void )
{
    wp_physics_material_desc desc;
    wp_physics_material *m;
    wp_physics_material_desc readback;

    wp_physics_material_desc_reset( &desc );
    CHECK( desc.name[0] == '\0' );
    CHECK( approx( desc.static_friction, 0.5f ) );
    CHECK( desc.friction_combine_mode == WORKPHONE_FRICTION_COMBINE_AVERAGE );

    wp_physics_material_desc_set_name( &desc, "Rubber" );
    desc.static_friction = 0.9f;
    desc.dynamic_friction = 0.8f;
    desc.rolling_friction = 0.1f;
    desc.restitution = 0.3f;
    desc.friction_combine_mode = WORKPHONE_FRICTION_COMBINE_MAX;
    desc.restitution_combine_mode = WORKPHONE_RESTITUTION_COMBINE_MULTIPLY;

    CHECK( wp_physics_material_desc_is_valid( &desc ) );
    CHECK( !wp_physics_material_desc_is_valid( NULL ) );

    m = wp_physics_material_create_from_desc( &desc );
    CHECK( m );
    wp_physics_material_get_desc( m, &readback );
    CHECK( strcmp( readback.name, "Rubber" ) == 0 );
    CHECK( approx( readback.static_friction, 0.9f ) );
    CHECK( approx( readback.restitution, 0.3f ) );
    CHECK( readback.friction_combine_mode == WORKPHONE_FRICTION_COMBINE_MAX );
    CHECK( readback.restitution_combine_mode == WORKPHONE_RESTITUTION_COMBINE_MULTIPLY );

    wp_physics_material_destroy( m );
    return 1;
}

static int test_combine_modes( void )
{
    /* Raw value combine helpers. */
    CHECK( approx( wp_physics_material_combine_friction_value( 0.2f, 0.8f,
                 WORKPHONE_FRICTION_COMBINE_AVERAGE ), 0.5f ) );
    CHECK( approx( wp_physics_material_combine_friction_value( 0.2f, 0.8f,
                 WORKPHONE_FRICTION_COMBINE_MIN ), 0.2f ) );
    CHECK( approx( wp_physics_material_combine_friction_value( 0.2f, 0.8f,
                 WORKPHONE_FRICTION_COMBINE_MAX ), 0.8f ) );
    CHECK( approx( wp_physics_material_combine_friction_value( 0.5f, 0.4f,
                 WORKPHONE_FRICTION_COMBINE_MULTIPLY ), 0.2f ) );

    /* Material-level combine using A's mode. */
    {
        wp_physics_material_desc a, b;
        wp_physics_material *ma, *mb;
        wp_physics_material_desc_reset( &a );
        wp_physics_material_desc_reset( &b );
        a.dynamic_friction = 0.2f;
        a.friction_combine_mode = WORKPHONE_FRICTION_COMBINE_MAX;
        a.restitution = 0.2f;
        a.restitution_combine_mode = WORKPHONE_RESTITUTION_COMBINE_AVERAGE;
        b.dynamic_friction = 0.8f;
        b.restitution = 0.4f;
        ma = wp_physics_material_create_from_desc( &a );
        mb = wp_physics_material_create_from_desc( &b );
        CHECK( approx( wp_physics_material_combine_friction( ma, mb ), 0.8f ) ); /* max */
        CHECK( approx( wp_physics_material_combine_restitution( ma, mb ), 0.3f ) ); /* average */
        wp_physics_material_destroy( ma );
        wp_physics_material_destroy( mb );
    }
    return 1;
}

static int test_registry( void )
{
    wp_physics_material_registry *reg = wp_physics_material_registry_create();
    wp_physics_material_desc desc;
    wp_physics_material *m;
    wp_physics_material *def;
    wp_u32 count;
    wp_c8 name[WP_PHYSICS_MATERIAL_MAX_NAME];

    CHECK( reg );
    CHECK( wp_physics_material_registry_get_count( reg ) == 1 ); /* default only */
    def = wp_physics_material_registry_get_default( reg );
    CHECK( def );
    CHECK( strcmp( wp_physics_material_get_name( def ), WP_PHYSICS_MATERIAL_DEFAULT_NAME ) == 0 );

    /* Unknown name resolves to default. */
    CHECK( wp_physics_material_registry_get_material( reg, "NoSuch" ) == def );
    CHECK( wp_physics_material_registry_find_material( reg, "NoSuch" ) == NULL );
    CHECK( !wp_physics_material_registry_has_material( reg, "Ice" ) );

    /* Register a material. */
    wp_physics_material_desc_reset( &desc );
    wp_physics_material_desc_set_name( &desc, "Ice" );
    desc.dynamic_friction = 0.05f;
    desc.restitution = 0.6f;
    m = wp_physics_material_registry_register( reg, &desc );
    CHECK( m );
    CHECK( wp_physics_material_registry_has_material( reg, "Ice" ) );
    CHECK( wp_physics_material_registry_get_material( reg, "Ice" ) == m );
    CHECK( approx( wp_physics_material_get_restitution( m ), 0.6f ) );

    /* Duplicate name rejected. */
    CHECK( wp_physics_material_registry_register( reg, &desc ) == NULL );

    /* Empty name rejected. */
    {
        wp_physics_material_desc empty;
        wp_physics_material_desc_reset( &empty );
        CHECK( wp_physics_material_registry_register( reg, &empty ) == NULL );
    }

    /* Enumeration. */
    count = wp_physics_material_registry_get_count( reg );
    CHECK( count == 2 ); /* default + Ice */
    CHECK( wp_physics_material_registry_get_by_index( reg, 0, name ) == def );
    CHECK( strcmp( name, WP_PHYSICS_MATERIAL_DEFAULT_NAME ) == 0 );
    CHECK( wp_physics_material_registry_get_by_index( reg, 1, name ) == m );
    CHECK( strcmp( name, "Ice" ) == 0 );

    /* Unregister. */
    CHECK( wp_physics_material_registry_unregister( reg, "Ice" ) == 1 );
    CHECK( !wp_physics_material_registry_has_material( reg, "Ice" ) );
    CHECK( wp_physics_material_registry_get_count( reg ) == 1 );

    /* Default cannot be removed. */
    CHECK( wp_physics_material_registry_unregister( reg, WP_PHYSICS_MATERIAL_DEFAULT_NAME ) == 0 );

    wp_physics_material_registry_destroy( reg );
    return 1;
}

static int test_database_load_and_register( void )
{
    static const char *text =
        "# A small material library\n"
        "material Ice\n"
        "  static_friction 0.10\n"
        "  dynamic_friction 0.05\n"
        "  rolling_friction 0.02\n"
        "  restitution 0.60\n"
        "  friction_combine min\n"
        "  restitution_combine max\n"
        "\n"
        "material Rubber\n"
        "  static_friction 0.95\n"
        "  dynamic_friction 0.80\n"
        "  restitution 0.20\n"
        "  friction_combine multiply\n"
        "  restitution_combine average\n";
    wp_physics_material_database db;
    wp_physics_material_registry *reg;
    wp_s32 parsed;
    wp_u32 added;
    wp_physics_material *ice;

    wp_physics_material_database_init( &db );
    parsed = wp_physics_material_database_load_from_text( text, &db );
    CHECK( parsed == 2 );
    CHECK( db.count == 2 );
    CHECK( strcmp( db.materials[0].name, "Ice" ) == 0 );
    CHECK( approx( db.materials[0].dynamic_friction, 0.05f ) );
    CHECK( db.materials[0].friction_combine_mode == WORKPHONE_FRICTION_COMBINE_MIN );
    CHECK( db.materials[0].restitution_combine_mode == WORKPHONE_RESTITUTION_COMBINE_MAX );
    CHECK( approx( db.materials[1].restitution, 0.20f ) );
    CHECK( db.materials[1].friction_combine_mode == WORKPHONE_FRICTION_COMBINE_MULTIPLY );

    reg = wp_physics_material_registry_create();
    added = wp_physics_material_registry_register_database( reg, &db );
    CHECK( added == 2 );
    ice = wp_physics_material_registry_find_material( reg, "Ice" );
    CHECK( ice );
    CHECK( approx( wp_physics_material_get_dynamic_friction( ice ), 0.05f ) );
    CHECK( approx( wp_physics_material_get_restitution( ice ), 0.60f ) );

    wp_physics_material_database_free( &db );
    wp_physics_material_registry_destroy( reg );
    return 1;
}

static int test_system_material_registry( void )
{
    wp_physics_system *sys = wp_physics_system_create();
    wp_physics_material_desc desc;
    wp_physics_material *m;

    CHECK( sys );
    CHECK( wp_physics_system_get_material_count( sys ) == 1 ); /* default */
    CHECK( wp_physics_system_get_default_material( sys ) != NULL );

    wp_physics_material_desc_reset( &desc );
    wp_physics_material_desc_set_name( &desc, "Metal" );
    desc.restitution = 0.05f;
    CHECK( wp_physics_system_register_material( sys, &desc ) );
    CHECK( wp_physics_system_get_material_count( sys ) == 2 );

    m = wp_physics_system_get_material_by_name( sys, "Metal" );
    CHECK( m );
    CHECK( approx( wp_physics_material_get_restitution( m ), 0.05f ) );

    /* Unknown resolves to default. */
    CHECK( wp_physics_system_get_material_by_name( sys, "?" ) ==
           wp_physics_system_get_default_material( sys ) );

    wp_physics_system_unregister_material( sys, "Metal" );
    CHECK( wp_physics_system_get_material_count( sys ) == 1 );
    wp_physics_system_destroy( sys );
    return 1;
}

static int test_collision_settings( void )
{
    wp_collision_settings a = wp_collision_settings_make_default();
    wp_collision_settings b;
    wp_u32 mask;

    CHECK( a.category == WORKPHONE_OBJECT_CATEGORY_ENVIRONMENT );
    CHECK( a.collision_mask == 0xFFFFFFFFu );

    mask = 0;
    mask = wp_collision_settings_set_category_bit( mask, WORKPHONE_OBJECT_CATEGORY_CHARACTER );
    mask = wp_collision_settings_set_query_bit( mask, WORKPHONE_QUERY_CATEGORY_NAVIGATION );
    CHECK( wp_collision_settings_has_category( mask, WORKPHONE_OBJECT_CATEGORY_CHARACTER ) );
    CHECK( wp_collision_settings_has_query( mask, WORKPHONE_QUERY_CATEGORY_NAVIGATION ) );
    CHECK( !wp_collision_settings_has_category( mask, WORKPHONE_OBJECT_CATEGORY_PROP ) );

    b.category = WORKPHONE_OBJECT_CATEGORY_CHARACTER;
    b.collision_mask = wp_collision_settings_set_category_bit( 0, WORKPHONE_OBJECT_CATEGORY_ENVIRONMENT );
    /* a is Environment|all; b is Character colliding with Environment -> collide. */
    CHECK( wp_collision_settings_should_collide( &a, &b ) );

    /* Legacy round-trip. */
    {
        wp_collision_settings s = wp_collision_settings_from_legacy( 0, 0xFFFFFFFFu );
        wp_u32 type = wp_collision_settings_to_collision_type( &s );
        wp_u32 msk = wp_collision_settings_to_collision_mask( &s );
        CHECK( type == 0xFFFFFFFFu );
        CHECK( msk == 0xFFFFFFFFu );
    }
    return 1;
}

static int test_shape_and_body_material_name( void )
{
    wp_physics_material_registry *reg = wp_physics_material_registry_create();
    wp_physics_material_desc desc;
    wp_collision_shape *shape;
    wp_rigidbody *body;
    wp_physics_material *resolved;

    wp_physics_material_desc_reset( &desc );
    wp_physics_material_desc_set_name( &desc, "Ice" );
    desc.dynamic_friction = 0.05f;
    wp_physics_material_registry_register( reg, &desc );

    shape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    wp_collision_shape_set_material_name( shape, "Ice" );
    CHECK( strcmp( wp_collision_shape_get_material_name( shape ), "Ice" ) == 0 );

    /* Resolves via the shape's own name. */
    resolved = wp_collision_shape_resolve_material( shape, reg );
    CHECK( resolved );
    CHECK( approx( wp_physics_material_get_dynamic_friction( resolved ), 0.05f ) );

    /* Empty shape name falls back to the body's name, then the default. */
    wp_collision_shape_set_material_name( shape, "" );
    body = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
    wp_rigidbody_set_material_name( body, "Ice" );
    wp_collision_shape_set_body( shape, body );
    resolved = wp_collision_shape_resolve_material( shape, reg );
    CHECK( resolved );
    CHECK( approx( wp_physics_material_get_dynamic_friction( resolved ), 0.05f ) );

    /* No name anywhere -> default. */
    wp_rigidbody_set_material_name( body, "" );
    resolved = wp_collision_shape_resolve_material( shape, reg );
    CHECK( resolved == wp_physics_material_registry_get_default( reg ) );

    wp_collision_shape_destroy( shape );
    wp_rigidbody_destroy( body );
    wp_physics_material_registry_destroy( reg );
    return 1;
}

/* ------------------------------------------------------------------------- */

typedef int ( *test_fn )( void );

static const test_fn tests[] = {
    test_material_defaults_and_clamp,
    test_descriptor_roundtrip,
    test_combine_modes,
    test_registry,
    test_database_load_and_register,
    test_system_material_registry,
    test_collision_settings,
    test_shape_and_body_material_name
};

int main( void )
{
    wp_u32 i;
    int failures = 0;
    for( i = 0; i < sizeof( tests ) / sizeof( tests[0] ); ++i )
    {
        if( !tests[i]() )
        {
            ++failures;
        }
    }
    if( failures == 0 )
    {
        printf( "All %u material-system tests passed.\n",
                (unsigned)( sizeof( tests ) / sizeof( tests[0] ) ) );
        return 0;
    }
    fprintf( stderr, "%d test(s) failed.\n", failures );
    return 1;
}
