#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace workphone;

namespace
{
    Polygon2F makeBox( const Vector2F &minPoint, const Vector2F &maxPoint )
    {
        Polygon2F polygon;
        polygon.setAsBox( minPoint, maxPoint );
        return polygon;
    }

    Polygon2F makeTriangle()
    {
        Polygon2F polygon;
        polygon.addPoint( Vector2F( 0.0f, 0.0f ) );
        polygon.addPoint( Vector2F( 4.0f, 0.0f ) );
        polygon.addPoint( Vector2F( 0.0f, 3.0f ) );
        return polygon;
    }

    void checkVector2Close( const Vector2F &actual, const Vector2F &expected )
    {
        BOOST_CHECK( Math<float>::equals( actual.X(), expected.X() ) );
        BOOST_CHECK( Math<float>::equals( actual.Y(), expected.Y() ) );
    }

    Vector2F lotExtents( const Polygon2F &lot )
    {
        BOOST_REQUIRE_GE( lot.getNumPoints(), 3u );
        auto minPoint = lot.getPoint( 0 );
        auto maxPoint = minPoint;
        for( const auto &p : lot.getPoints() )
        {
            minPoint.X() = std::min( minPoint.X(), p.X() );
            minPoint.Y() = std::min( minPoint.Y(), p.Y() );
            maxPoint.X() = std::max( maxPoint.X(), p.X() );
            maxPoint.Y() = std::max( maxPoint.Y(), p.Y() );
        }
        return maxPoint - minPoint;
    }
}  // namespace

BOOST_AUTO_TEST_CASE( polygon2_default_constructor_starts_empty )
{
    Polygon2F polygon;

    BOOST_CHECK_EQUAL( polygon.getNumPoints(), 0u );
    BOOST_CHECK( polygon.getPoints().empty() );
    BOOST_CHECK( !polygon.isPointInside( Vector2F( 0.0f, 0.0f ) ) );
}

BOOST_AUTO_TEST_CASE( polygon2_add_points_and_accessors )
{
    Polygon2F polygon;
    polygon.addPoint( Vector2F( 1.0f, 2.0f ) );
    polygon.addPoint( Vector2F( 3.0f, 4.0f ) );

    Array<Vector2F> morePoints;
    morePoints.push_back( Vector2F( 5.0f, 6.0f ) );
    morePoints.push_back( Vector2F( 7.0f, 8.0f ) );
    polygon.addPoints( morePoints );

    BOOST_CHECK_EQUAL( polygon.getNumPoints(), 4u );
    checkVector2Close( polygon.getPoint( 0 ), Vector2F( 1.0f, 2.0f ) );
    checkVector2Close( polygon.getPoint( 1 ), Vector2F( 3.0f, 4.0f ) );
    checkVector2Close( polygon.getPoint( 2 ), Vector2F( 5.0f, 6.0f ) );
    checkVector2Close( polygon.getPoint( 3 ), Vector2F( 7.0f, 8.0f ) );
}

BOOST_AUTO_TEST_CASE( polygon2_set_points_replaces_existing_points )
{
    Polygon2F polygon = makeBox( Vector2F( 0.0f, 0.0f ), Vector2F( 1.0f, 1.0f ) );

    Array<Vector2F> points;
    points.push_back( Vector2F( -1.0f, -2.0f ) );
    points.push_back( Vector2F( 2.0f, -2.0f ) );
    points.push_back( Vector2F( 0.0f, 2.0f ) );
    polygon.setPoints( points );

    BOOST_CHECK_EQUAL( polygon.getNumPoints(), 3u );
    checkVector2Close( polygon.getPoint( 0 ), points[0] );
    checkVector2Close( polygon.getPoint( 1 ), points[1] );
    checkVector2Close( polygon.getPoint( 2 ), points[2] );
}

BOOST_AUTO_TEST_CASE( polygon2_constructs_from_points_and_copies )
{
    Array<Vector2F> points;
    points.push_back( Vector2F( 0.0f, 0.0f ) );
    points.push_back( Vector2F( 2.0f, 0.0f ) );
    points.push_back( Vector2F( 2.0f, 2.0f ) );
    points.push_back( Vector2F( 0.0f, 2.0f ) );

    Polygon2F polygon( points );
    Polygon2F copy( polygon );

    BOOST_CHECK( copy == polygon );

    copy.removePoint( 3 );
    BOOST_CHECK( copy != polygon );
    BOOST_CHECK_EQUAL( polygon.getNumPoints(), 4u );
    BOOST_CHECK_EQUAL( copy.getNumPoints(), 3u );
}

BOOST_AUTO_TEST_CASE( polygon2_equality_ignores_point_order )
{
    Polygon2F polygonA;
    polygonA.addPoint( Vector2F( 0.0f, 0.0f ) );
    polygonA.addPoint( Vector2F( 1.0f, 0.0f ) );
    polygonA.addPoint( Vector2F( 1.0f, 1.0f ) );
    polygonA.addPoint( Vector2F( 0.0f, 1.0f ) );

    Polygon2F polygonB;
    polygonB.addPoint( Vector2F( 1.0f, 1.0f ) );
    polygonB.addPoint( Vector2F( 0.0f, 1.0f ) );
    polygonB.addPoint( Vector2F( 0.0f, 0.0f ) );
    polygonB.addPoint( Vector2F( 1.0f, 0.0f ) );

    BOOST_CHECK( polygonA == polygonB );

    polygonB.removePoint( 0 );
    polygonB.addPoint( Vector2F( 2.0f, 2.0f ) );
    BOOST_CHECK( polygonA != polygonB );
}

BOOST_AUTO_TEST_CASE( polygon2_remove_point_erases_requested_index )
{
    Polygon2F polygon = makeTriangle();

    BOOST_CHECK( polygon.removePoint( 1 ) );
    BOOST_CHECK_EQUAL( polygon.getNumPoints(), 2u );
    checkVector2Close( polygon.getPoint( 0 ), Vector2F( 0.0f, 0.0f ) );
    checkVector2Close( polygon.getPoint( 1 ), Vector2F( 0.0f, 3.0f ) );
}

BOOST_AUTO_TEST_CASE( polygon2_set_as_box_creates_expected_vertices )
{
    Polygon2F box = makeBox( Vector2F( -1.0f, -2.0f ), Vector2F( 3.0f, 4.0f ) );

    BOOST_CHECK_EQUAL( box.getNumPoints(), 4u );
    checkVector2Close( box.getPoint( 0 ), Vector2F( -1.0f, -2.0f ) );
    checkVector2Close( box.getPoint( 1 ), Vector2F( 3.0f, -2.0f ) );
    checkVector2Close( box.getPoint( 2 ), Vector2F( 3.0f, 4.0f ) );
    checkVector2Close( box.getPoint( 3 ), Vector2F( -1.0f, 4.0f ) );
}

BOOST_AUTO_TEST_CASE( polygon2_center_is_average_of_points )
{
    Polygon2F box = makeBox( Vector2F( -2.0f, -4.0f ), Vector2F( 6.0f, 8.0f ) );

    checkVector2Close( box.getCenter(), Vector2F( 2.0f, 2.0f ) );
}

BOOST_AUTO_TEST_CASE( polygon2_area_reports_signed_area )
{
    Polygon2F box = makeBox( Vector2F( 0.0f, 0.0f ), Vector2F( 4.0f, 3.0f ) );

    BOOST_CHECK( Math<float>::equals( box.getArea(), 12.0f ) );

    Array<Vector2F> clockwisePoints;
    clockwisePoints.push_back( Vector2F( 0.0f, 0.0f ) );
    clockwisePoints.push_back( Vector2F( 0.0f, 3.0f ) );
    clockwisePoints.push_back( Vector2F( 4.0f, 3.0f ) );
    clockwisePoints.push_back( Vector2F( 4.0f, 0.0f ) );

    Polygon2F clockwiseBox( clockwisePoints );
    BOOST_CHECK( Math<float>::equals( clockwiseBox.getArea(), -12.0f ) );
}

BOOST_AUTO_TEST_CASE( polygon2_area_subdivision_appends_and_preserves_winding )
{
    const auto previous = makeTriangle();
    for( bool clockwise : { false, true } )
    {
        auto box = makeBox( Vector2F( -20, -16 ), Vector2F( -4, -8 ) );
        if( clockwise )
        {
            box.changePointOrdering();
        }
        const auto points = box.getPoints();
        BOOST_CHECK_EQUAL( box.getArea(), clockwise ? -128.0f : 128.0f );
        Array<Polygon2F> lots{ previous };
        // Covering 128 square units within these limits requires three lots.
        box.subDivide( 40.0f, 45.0f, lots );
        BOOST_REQUIRE_EQUAL( lots.size(), 4u );
        BOOST_CHECK( lots.front().getPoints() == previous.getPoints() );
        BOOST_CHECK( box.getPoints() == points );
        float totalArea = 0;
        for( size_t i = 1; i < lots.size(); ++i )
        {
            const auto area = lots[i].getArea();
            BOOST_CHECK( clockwise ? area < 0 : area > 0 );
            BOOST_CHECK_GE( std::abs( area ), 40.0f );
            BOOST_CHECK_LE( std::abs( area ), 45.0f );
            totalArea += std::abs( area );
        }
        BOOST_CHECK_CLOSE( totalArea, 128.0f, 0.001f );
    }
}

BOOST_AUTO_TEST_CASE( polygon2_dimension_subdivision_respects_full_lot_extents )
{
    auto block = makeBox( Vector2F( -20, -16 ), Vector2F( -4, -8 ) );
    Array<Polygon2F> lots;
    block.subDivide( Vector2F( 2, 2 ), Vector2F( 8, 8 ), lots );
    BOOST_REQUIRE_GE( lots.size(), 2u );
    float totalArea = 0;
    for( const auto &lot : lots )
    {
        const auto size = lotExtents( lot );
        BOOST_CHECK( size.X() >= 2 && size.X() <= 8 && size.Y() >= 2 && size.Y() <= 8 );
        totalArea += lot.getArea();
    }
    BOOST_CHECK_CLOSE( totalArea, 128.0f, 0.001f );
}

BOOST_AUTO_TEST_CASE( polygon2_dimension_subdivision_terminates_at_an_acute_tip )
{
    Polygon2F block( { { 0, 0 }, { 64, 0 }, { 0, 1 } } );
    Array<Polygon2F> lots;
    block.subDivide( Vector2F( 0, 0 ), Vector2F( 128, 128 ), lots );
    BOOST_REQUIRE( !lots.empty() );
    float covered = 0;
    for( const auto &lot : lots )
    {
        const auto size = lotExtents( lot );
        BOOST_CHECK_LE( std::max( size.X(), size.Y() ), 1.5f * std::min( size.X(), size.Y() ) );
        BOOST_CHECK_GT( lot.getArea(), 0.0f );
        covered += lot.getArea();
    }
    BOOST_CHECK_LE( covered, block.getArea() + 0.001f );
}

BOOST_AUTO_TEST_CASE( polygon2_subdivision_tiles_a_concave_block )
{
    Polygon2F block( { { 0, 0 }, { 16, 0 }, { 16, 4 }, { 4, 4 }, { 4, 16 }, { 0, 16 } } );
    Array<Polygon2F> lots;
    block.subDivide( 0.01f, 32.0f, lots );
    BOOST_REQUIRE_GT( lots.size(), 1u );
    float totalArea = 0;
    for( const auto &lot : lots )
    {
        BOOST_CHECK_GT( lot.getArea(), 0.0f );
        BOOST_CHECK_LE( lot.getArea(), 32.0f );
        totalArea += lot.getArea();
    }
    BOOST_CHECK_CLOSE( totalArea, 112.0f, 0.001f );
    // Sample both arms and the empty notch, away from integer subdivision lines.
    for( int x = 0; x < 16; ++x )
    {
        for( int y = 0; y < 16; ++y )
        {
            const Vector2F sample( x + 0.31f, y + 0.67f );
            const auto owners = std::count_if( lots.begin(), lots.end(),
                [&]( const auto &lot ) { return lot.isPointInside( sample ); } );
            BOOST_CHECK_EQUAL( owners, x < 4 || y < 4 ? 1 : 0 );
        }
    }
}

BOOST_AUTO_TEST_CASE( polygon2_integer_square_preserves_lattice_area )
{
    Polygon2<s32> square;
    square.setAsBox( { 0, 0 }, { 8, 8 } );
    BOOST_CHECK_EQUAL( square.getArea(), 64 );
    Array<Polygon2<s32>> lots;
    square.subDivide( 4, 16, lots );
    BOOST_REQUIRE_GT( lots.size(), 1u );
    s32 totalArea = 0;
    for( const auto &lot : lots )
    {
        BOOST_CHECK_GE( lot.getArea(), 4 );
        BOOST_CHECK_LE( lot.getArea(), 16 );
        totalArea += lot.getArea();
    }
    BOOST_CHECK_EQUAL( totalArea, 64 );
}

BOOST_AUTO_TEST_CASE( polygon2_subdivision_stays_inside_float_block_at_large_world_coordinates )
{
    constexpr float offset = 10000000.0f;
    Polygon2F block( { { offset, -offset }, { offset + 24, -offset }, { offset + 8, -offset + 10 } } );
    Array<Polygon2F> lots;
    block.subDivide( 0.0f, 100.0f, lots );
    BOOST_REQUIRE_GT( lots.size(), 1u );
    float covered = 0;
    for( const auto &lot : lots )
    {
        for( const auto &p : lot.getPoints() )
        {
            const float x = p.X() - offset, y = p.Y() + offset;
            BOOST_CHECK( y >= 0 && 10 * x >= 8 * y && 10 * x + 16 * y <= 240 );
        }
        BOOST_CHECK_GT( lot.getArea(), 0.0f );
        BOOST_CHECK_LE( lot.getArea(), 100.0f );
        covered += lot.getArea();
    }
    BOOST_CHECK_EQUAL( covered, 120.0f );
}

BOOST_AUTO_TEST_CASE( polygon2_subdivision_preserves_existing_lots_on_invalid_or_empty_area )
{
    auto block = makeBox( Vector2F( 0, 0 ), Vector2F( 8, 8 ) );
    const auto previous = makeTriangle();
    Array<Polygon2F> lots{ previous };
    BOOST_CHECK_THROW( block.subDivide( 16.0f, 4.0f, lots ), std::invalid_argument );
    BOOST_CHECK_THROW( block.subDivide( Vector2F( 2, 9 ), Vector2F( 8, 8 ), lots ), std::invalid_argument );
    BOOST_CHECK_THROW( block.subDivide( 0.0f, 1e-37f, lots ), std::length_error );
    Polygon2F line( { { 0, 0 }, { 2, 0 }, { 4, 0 } } );
    line.subDivide( 0.0f, 1.0f, lots );
    BOOST_REQUIRE_EQUAL( lots.size(), 1u );
    BOOST_CHECK( lots.front().getPoints() == previous.getPoints() );
}

BOOST_AUTO_TEST_CASE( polygon2_scale_polygon_scales_around_center )
{
    Polygon2F box = makeBox( Vector2F( 0.0f, 0.0f ), Vector2F( 2.0f, 2.0f ) );

    Polygon2F scaled = box.getScalePolygon( 2.0f );

    BOOST_CHECK_EQUAL( scaled.getNumPoints(), 4u );
    checkVector2Close( scaled.getCenter(), Vector2F( 1.0f, 1.0f ) );
    checkVector2Close( scaled.getPoint( 0 ), Vector2F( -1.0f, -1.0f ) );
    checkVector2Close( scaled.getPoint( 1 ), Vector2F( 3.0f, -1.0f ) );
    checkVector2Close( scaled.getPoint( 2 ), Vector2F( 3.0f, 3.0f ) );
    checkVector2Close( scaled.getPoint( 3 ), Vector2F( -1.0f, 3.0f ) );
}

BOOST_AUTO_TEST_CASE( polygon2_point_inside_detects_inside_and_outside_points )
{
    Polygon2F box = makeBox( Vector2F( 0.0f, 0.0f ), Vector2F( 4.0f, 4.0f ) );

    BOOST_CHECK( box.isPointInside( Vector2F( 2.0f, 2.0f ) ) );
    BOOST_CHECK( !box.isPointInside( Vector2F( -1.0f, 2.0f ) ) );
    BOOST_CHECK( !box.isPointInside( Vector2F( 2.0f, 5.0f ) ) );
}

BOOST_AUTO_TEST_CASE( polygon2_is_inside_requires_all_points_inside )
{
    Polygon2F outer = makeBox( Vector2F( 0.0f, 0.0f ), Vector2F( 10.0f, 10.0f ) );
    Polygon2F inner = makeBox( Vector2F( 2.0f, 2.0f ), Vector2F( 4.0f, 4.0f ) );
    Polygon2F overlapping = makeBox( Vector2F( 8.0f, 8.0f ), Vector2F( 12.0f, 12.0f ) );

    BOOST_CHECK( outer.isInside( inner ) );
    BOOST_CHECK( !outer.isInside( overlapping ) );
}

BOOST_AUTO_TEST_CASE( polygon2_intersects_detects_overlap_and_separation )
{
    Polygon2F boxA = makeBox( Vector2F( 0.0f, 0.0f ), Vector2F( 2.0f, 2.0f ) );
    Polygon2F overlapping = makeBox( Vector2F( 1.0f, 1.0f ), Vector2F( 3.0f, 3.0f ) );
    Polygon2F separate = makeBox( Vector2F( 3.0f, 3.0f ), Vector2F( 5.0f, 5.0f ) );

    BOOST_CHECK( boxA.intersects( overlapping ) );
    BOOST_CHECK( overlapping.intersects( boxA ) );
    BOOST_CHECK( !boxA.intersects( separate ) );
    BOOST_CHECK( !separate.intersects( boxA ) );
}
