#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

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
