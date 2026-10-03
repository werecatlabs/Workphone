#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( testSimpleSplineConstructor )
{
    LinearSpline3F spline;
    BOOST_CHECK( spline.getNumPoints() == 0 );
}

BOOST_AUTO_TEST_CASE( testSimpleSplineAddPoint )
{
    LinearSpline3F spline;

    Array<Vector3F> points;
    points.push_back( Vector3F( 0.0f, 0.0f, 0.0f ) );
    points.push_back( Vector3F( 1.0f, 1.0f, 1.0f ) );
    spline.setPoints( points );

    BOOST_CHECK( spline.getNumPoints() == 2 );
}

BOOST_AUTO_TEST_CASE( testSimpleSplineGetPoint )
{
    LinearSpline3F spline;

    Array<Vector3F> points;
    points.push_back( Vector3F( 0.0f, 0.0f, 0.0f ) );
    points.push_back( Vector3F( 1.0f, 1.0f, 1.0f ) );
    spline.setPoints( points );

    auto point = spline.getPoint( 0 );
    BOOST_CHECK( point.x == 0.0f );
    BOOST_CHECK( point.y == 0.0f );
    BOOST_CHECK( point.z == 0.0f );
    point = spline.getPoint( 1 );
    BOOST_CHECK( point.x == 1.0f );
    BOOST_CHECK( point.y == 1.0f );
    BOOST_CHECK( point.z == 1.0f );
}

BOOST_AUTO_TEST_CASE( testSimpleSplineInterpolation )
{
    LinearSpline3F spline;

    Array<Vector3F> points;
    points.push_back( Vector3F( 0.0f, 0.0f, 0.0f ) );
    points.push_back( Vector3F( 1.0f, 1.0f, 1.0f ) );
    spline.setPoints( points );

    spline.recalcTangents();
    Vector3F point = spline.interpolate( 0.5f );
    BOOST_CHECK( point.x == 0.5f );
    BOOST_CHECK( point.y == 0.5f );
    BOOST_CHECK( point.z == 0.5f );
}

BOOST_AUTO_TEST_CASE( testRotationalSplineConstructor )
{
    RotationalSpline3F spline;
    BOOST_CHECK( spline.getNumPoints() == 0 );
}

BOOST_AUTO_TEST_CASE( testRotationalSplineAddPoint )
{
    RotationalSpline3F spline;
    spline.addPoint( QuaternionF::identity() );
    spline.addPoint( QuaternionF::angleAxis( MathF::DegToRad( 45 ), Vector3F::UNIT_Y ) );
    BOOST_CHECK( spline.getNumPoints() == 2 );
}

BOOST_AUTO_TEST_CASE( testRotationalSplineGetPoint )
{
    RotationalSpline3F spline;
    spline.addPoint( QuaternionF::identity() );
    spline.addPoint( QuaternionF::angleAxis( MathF::DegToRad( 45 ), Vector3F::UNIT_Y ) );
    QuaternionF point = spline.getPoint( 0 );
    BOOST_CHECK( point == QuaternionF::identity() );
    point = spline.getPoint( 1 );
    BOOST_CHECK( point == QuaternionF::angleAxis( MathF::DegToRad( 45 ), Vector3F::UNIT_Y ) );
}

BOOST_AUTO_TEST_CASE( testRotationalSplineInterpolation )
{
    RotationalSpline3F spline;
    spline.addPoint( QuaternionF::identity() );
    spline.addPoint( QuaternionF::angleAxis( MathF::DegToRad( 45 ), Vector3F::UNIT_Y ) );
    spline.recalcTangents();
    QuaternionF point = spline.interpolate( 0.5f );
    QuaternionF expected = QuaternionF::angleAxis( MathF::DegToRad( 22.5f ), Vector3F::UNIT_Y );
    // Use per-component tolerance instead of exact equality
    BOOST_TEST( point.w == expected.w, boost::test_tools::tolerance( 1e-4f ) );
    BOOST_TEST( point.x == expected.x, boost::test_tools::tolerance( 1e-4f ) );
    BOOST_TEST( point.y == expected.y, boost::test_tools::tolerance( 1e-4f ) );
    BOOST_TEST( point.z == expected.z, boost::test_tools::tolerance( 1e-4f ) );
}
