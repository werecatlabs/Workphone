#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( testSphereConstructor )
{
    Sphere3F sphere( Vector3F::ZERO, 1.0f );
    BOOST_CHECK( sphere.getCenter() == Vector3F::ZERO );
    BOOST_CHECK( sphere.getRadius() == 1.0f );
}

BOOST_AUTO_TEST_CASE( testSphereIntersection )
{
    Sphere3F sphere1( Vector3F::ZERO, 1.0f );
    Sphere3F sphere2( Vector3F( 2.0f, 0.0f, 0.0f ), 1.0f );
    BOOST_CHECK( sphere1.intersects( sphere2 ) );
    sphere2.setCenter( Vector3F( 3.0f, 0.0f, 0.0f ) );
    BOOST_CHECK( !sphere1.intersects( sphere2 ) );
}

BOOST_AUTO_TEST_CASE( sphere_touching_exactly )
{
    // Two unit spheres whose centres are exactly 2 units apart (touching)
    Sphere3F s1( Vector3F( 0.0f, 0.0f, 0.0f ), 1.0f );
    Sphere3F s2( Vector3F( 2.0f, 0.0f, 0.0f ), 1.0f );
    // Touching counts as intersecting in most implementations
    BOOST_CHECK( s1.intersects( s2 ) );
}

BOOST_AUTO_TEST_CASE( sphere_contained_inside_larger )
{
    // Small sphere entirely inside larger sphere
    Sphere3F large( Vector3F( 0.0f, 0.0f, 0.0f ), 10.0f );
    Sphere3F inner( Vector3F( 0.0f, 0.0f, 0.0f ), 1.0f );
    BOOST_CHECK( large.intersects( inner ) );
}

BOOST_AUTO_TEST_CASE( sphere_accessor_set_radius )
{
    Sphere3F sphere( Vector3F::ZERO, 1.0f );
    BOOST_CHECK( Math<float>::equals( sphere.getRadius(), 1.0f ) );
    sphere.setRadius( 5.0f );
    BOOST_CHECK( Math<float>::equals( sphere.getRadius(), 5.0f ) );
}

BOOST_AUTO_TEST_CASE( sphere_accessor_set_center )
{
    Sphere3F sphere( Vector3F::ZERO, 1.0f );
    BOOST_CHECK( sphere.getCenter() == Vector3F::ZERO );
    sphere.setCenter( Vector3F( 3.0f, 4.0f, 5.0f ) );
    BOOST_CHECK( sphere.getCenter() == Vector3F( 3.0f, 4.0f, 5.0f ) );
}
