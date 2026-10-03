#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;

namespace
{
    void checkFloatClose( float actual, float expected )
    {
        BOOST_CHECK( Math<float>::equals( actual, expected ) );
    }

    void checkVector2Close( const Vector2F &actual, const Vector2F &expected )
    {
        checkFloatClose( actual.X(), expected.X() );
        checkFloatClose( actual.Y(), expected.Y() );
    }

    void checkAABB2Close( const AABB2F &actual, const AABB2F &expected )
    {
        checkVector2Close( actual.getMin(), expected.getMin() );
        checkVector2Close( actual.getMax(), expected.getMax() );
    }
}  // namespace

BOOST_AUTO_TEST_CASE( aabb2_coordinate_constructor_sets_min_and_max )
{
    AABB2F box( -1.0f, 2.0f, 3.0f, 6.0f );

    checkVector2Close( box.getMin(), Vector2F( -1.0f, 2.0f ) );
    checkVector2Close( box.getMax(), Vector2F( 3.0f, 6.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_vector_constructor_sets_min_and_max )
{
    AABB2F box( Vector2F( -2.0f, -3.0f ), Vector2F( 4.0f, 5.0f ) );

    checkVector2Close( box.getMin(), Vector2F( -2.0f, -3.0f ) );
    checkVector2Close( box.getMax(), Vector2F( 4.0f, 5.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_size_constructor_uses_position_plus_size )
{
    AABB2F box( Vector2F( 2.0f, 3.0f ), Vector2F( 4.0f, 5.0f ), false );

    checkAABB2Close( box, AABB2F( 2.0f, 3.0f, 6.0f, 8.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_extents_constructor_uses_center_and_half_size )
{
    AABB2F box( Vector2F( 10.0f, 20.0f ), Vector2F( 2.0f, 3.0f ), true );

    checkAABB2Close( box, AABB2F( 8.0f, 17.0f, 12.0f, 23.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_copy_assignment_and_equality )
{
    AABB2F box( -1.0f, -2.0f, 3.0f, 4.0f );
    AABB2F copy( box );
    AABB2F assigned;

    assigned = box;

    BOOST_CHECK( copy == box );
    BOOST_CHECK( assigned == box );
    BOOST_CHECK( !( assigned != box ) );
    BOOST_CHECK( assigned != AABB2F( 0.0f, 0.0f, 1.0f, 1.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_translation_operators_move_min_and_max )
{
    AABB2F box( 1.0f, 2.0f, 4.0f, 6.0f );
    Vector2F offset( -2.0f, 3.0f );

    checkAABB2Close( box + offset, AABB2F( -1.0f, 5.0f, 2.0f, 9.0f ) );
    checkAABB2Close( box - offset, AABB2F( 3.0f, -1.0f, 6.0f, 3.0f ) );

    box += offset;
    checkAABB2Close( box, AABB2F( -1.0f, 5.0f, 2.0f, 9.0f ) );

    box -= offset;
    checkAABB2Close( box, AABB2F( 1.0f, 2.0f, 4.0f, 6.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_area_size_center_and_half_size )
{
    AABB2F box( -2.0f, 1.0f, 4.0f, 9.0f );

    checkFloatClose( box.getWidth(), 6.0f );
    checkFloatClose( box.getHeight(), 8.0f );
    checkFloatClose( box.getArea(), 48.0f );
    checkVector2Close( box.getSize(), Vector2F( 6.0f, 8.0f ) );
    checkVector2Close( box.getHalfSize(), Vector2F( 3.0f, 4.0f ) );
    checkVector2Close( box.getCenter(), Vector2F( 1.0f, 5.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_is_inside_includes_edges )
{
    AABB2F box( 0.0f, 0.0f, 10.0f, 20.0f );

    BOOST_CHECK( box.isInside( Vector2F( 5.0f, 10.0f ) ) );
    BOOST_CHECK( box.isInside( Vector2F( 0.0f, 0.0f ) ) );
    BOOST_CHECK( box.isInside( Vector2F( 10.0f, 20.0f ) ) );
    BOOST_CHECK( !box.isInside( Vector2F( -0.1f, 10.0f ) ) );
    BOOST_CHECK( !box.isInside( Vector2F( 5.0f, 20.1f ) ) );
}

BOOST_AUTO_TEST_CASE( aabb2_intersects_overlapping_boxes_but_not_touching_edges )
{
    AABB2F box( 0.0f, 0.0f, 10.0f, 10.0f );

    BOOST_CHECK( box.intersects( AABB2F( 5.0f, 5.0f, 12.0f, 12.0f ) ) );
    BOOST_CHECK( box.intersects( AABB2F( -1.0f, -1.0f, 1.0f, 1.0f ) ) );
    BOOST_CHECK( !box.intersects( AABB2F( 10.0f, 0.0f, 12.0f, 2.0f ) ) );
    BOOST_CHECK( !box.intersects( AABB2F( 11.0f, 0.0f, 12.0f, 2.0f ) ) );
}

BOOST_AUTO_TEST_CASE( aabb2_clip_against_intersection )
{
    AABB2F box( 0.0f, 0.0f, 10.0f, 10.0f );

    box.clipAgainst( AABB2F( 2.0f, 3.0f, 8.0f, 9.0f ) );

    checkAABB2Close( box, AABB2F( 2.0f, 3.0f, 8.0f, 9.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_clip_against_disjoint_box_collapses_to_boundary )
{
    AABB2F box( 0.0f, 0.0f, 10.0f, 10.0f );

    box.clipAgainst( AABB2F( 20.0f, 30.0f, 40.0f, 50.0f ) );

    checkAABB2Close( box, AABB2F( 10.0f, 10.0f, 10.0f, 10.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_constrain_to_moves_box_inside_bounds )
{
    AABB2F box( 8.0f, 9.0f, 12.0f, 13.0f );

    BOOST_CHECK( box.constrainTo( AABB2F( 0.0f, 0.0f, 10.0f, 10.0f ) ) );

    checkAABB2Close( box, AABB2F( 6.0f, 6.0f, 10.0f, 10.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_constrain_to_returns_false_when_box_is_too_large )
{
    AABB2F box( 0.0f, 0.0f, 12.0f, 12.0f );

    BOOST_CHECK( !box.constrainTo( AABB2F( 0.0f, 0.0f, 10.0f, 10.0f ) ) );
    checkAABB2Close( box, AABB2F( 0.0f, 0.0f, 12.0f, 12.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_repair_swaps_inverted_axes )
{
    AABB2F box( 5.0f, 7.0f, -1.0f, -2.0f );

    BOOST_CHECK( !box.isValid() );
    box.repair();

    BOOST_CHECK( box.isValid() );
    checkAABB2Close( box, AABB2F( -1.0f, -2.0f, 5.0f, 7.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_zero_area_box_is_invalid )
{
    BOOST_CHECK( !AABB2F( 0.0f, 0.0f, 0.0f, 10.0f ).isValid() );
    BOOST_CHECK( !AABB2F( 0.0f, 0.0f, 10.0f, 0.0f ).isValid() );
    BOOST_CHECK( AABB2F( 0.0f, 0.0f, 10.0f, 10.0f ).isValid() );
}

BOOST_AUTO_TEST_CASE( aabb2_add_internal_point_expands_box )
{
    AABB2F box( 0.0f, 0.0f, 2.0f, 3.0f );

    box.addInternalPoint( Vector2F( -1.0f, 5.0f ) );

    checkAABB2Close( box, AABB2F( -1.0f, 0.0f, 2.0f, 5.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_setters_update_bounds )
{
    AABB2F box( 0.0f, 0.0f, 1.0f, 1.0f );

    box.setMin( Vector2F( -3.0f, -4.0f ) );
    box.setMax( Vector2F( 5.0f, 6.0f ) );

    checkAABB2Close( box, AABB2F( -3.0f, -4.0f, 5.0f, 6.0f ) );
}

BOOST_AUTO_TEST_CASE( aabb2_less_than_compares_area )
{
    AABB2F smallerBox( 0.0f, 0.0f, 2.0f, 3.0f );
    AABB2F largerBox( 0.0f, 0.0f, 4.0f, 5.0f );

    BOOST_CHECK( ( smallerBox < largerBox ) );
    BOOST_CHECK( !( largerBox < smallerBox ) );
}
