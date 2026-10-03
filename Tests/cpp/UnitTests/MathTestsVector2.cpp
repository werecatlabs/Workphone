#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;

namespace
{
    void checkVector2Close( const Vector2F &actual, const Vector2F &expected )
    {
        BOOST_CHECK( Math<float>::equals( actual.X(), expected.X() ) );
        BOOST_CHECK( Math<float>::equals( actual.Y(), expected.Y() ) );
    }
}  // namespace

BOOST_AUTO_TEST_CASE( vector2_default_constructor_sets_zero )
{
    Vector2F v;

    BOOST_CHECK( Math<float>::equals( v.X(), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Y(), 0.0f ) );
    BOOST_CHECK( v == Vector2F::ZERO );
}

BOOST_AUTO_TEST_CASE( vector2_component_constructor_and_copy )
{
    Vector2F v( 1.5f, -2.5f );
    Vector2F copy( v );

    BOOST_CHECK( Math<float>::equals( v.x, 1.5f ) );
    BOOST_CHECK( Math<float>::equals( v.y, -2.5f ) );
    BOOST_CHECK( copy == v );
}

BOOST_AUTO_TEST_CASE( vector2_assignment_and_set_methods )
{
    Vector2F v;
    v.set( 3.0f, 4.0f );
    checkVector2Close( v, Vector2F( 3.0f, 4.0f ) );

    Vector2F other( -5.0f, 6.0f );
    v.set( other );
    checkVector2Close( v, other );

    Vector2F assigned;
    assigned = v;
    BOOST_CHECK( assigned == v );
}

BOOST_AUTO_TEST_CASE( vector2_accessors_and_subscript_operator )
{
    Vector2F v( 1.0f, 2.0f );

    BOOST_CHECK( Math<float>::equals( v[0], 1.0f ) );
    BOOST_CHECK( Math<float>::equals( v[1], 2.0f ) );

    v[0] = 10.0f;
    v[1] = 20.0f;
    BOOST_CHECK( Math<float>::equals( v.X(), 10.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Y(), 20.0f ) );

    v.X() = -1.0f;
    v.Y() = -2.0f;
    BOOST_CHECK( Math<float>::equals( v.x, -1.0f ) );
    BOOST_CHECK( Math<float>::equals( v.y, -2.0f ) );
}

BOOST_AUTO_TEST_CASE( vector2_pointer_access_exposes_contiguous_components )
{
    Vector2F v( 7.0f, 8.0f );

    float *data = v.ptr();
    BOOST_CHECK( Math<float>::equals( data[0], 7.0f ) );
    BOOST_CHECK( Math<float>::equals( data[1], 8.0f ) );

    data[0] = 9.0f;
    data[1] = 10.0f;
    checkVector2Close( v, Vector2F( 9.0f, 10.0f ) );

    const Vector2F constVector( 1.0f, 2.0f );
    const float *constData = constVector.ptr();
    BOOST_CHECK( Math<float>::equals( constData[0], 1.0f ) );
    BOOST_CHECK( Math<float>::equals( constData[1], 2.0f ) );
}

BOOST_AUTO_TEST_CASE( vector2_arithmetic_operators )
{
    Vector2F a( 6.0f, 8.0f );
    Vector2F b( 2.0f, 4.0f );

    checkVector2Close( a + b, Vector2F( 8.0f, 12.0f ) );
    checkVector2Close( a - b, Vector2F( 4.0f, 4.0f ) );
    checkVector2Close( a * b, Vector2F( 12.0f, 32.0f ) );
    checkVector2Close( a / b, Vector2F( 3.0f, 2.0f ) );
    checkVector2Close( a * 2.0f, Vector2F( 12.0f, 16.0f ) );
    checkVector2Close( 2.0f * a, Vector2F( 12.0f, 16.0f ) );
    checkVector2Close( a / 2.0f, Vector2F( 3.0f, 4.0f ) );
    checkVector2Close( -a, Vector2F( -6.0f, -8.0f ) );
}

BOOST_AUTO_TEST_CASE( vector2_compound_assignment_operators )
{
    Vector2F v( 6.0f, 8.0f );

    v += Vector2F( 1.0f, 2.0f );
    checkVector2Close( v, Vector2F( 7.0f, 10.0f ) );

    v -= Vector2F( 2.0f, 3.0f );
    checkVector2Close( v, Vector2F( 5.0f, 7.0f ) );

    v *= Vector2F( 2.0f, 3.0f );
    checkVector2Close( v, Vector2F( 10.0f, 21.0f ) );

    v /= Vector2F( 5.0f, 7.0f );
    checkVector2Close( v, Vector2F( 2.0f, 3.0f ) );

    v *= 4.0f;
    checkVector2Close( v, Vector2F( 8.0f, 12.0f ) );

    v /= 4.0f;
    checkVector2Close( v, Vector2F( 2.0f, 3.0f ) );
}

BOOST_AUTO_TEST_CASE( vector2_equality_and_equals_use_tolerance )
{
    Vector2F a( 1.0f, 2.0f );
    Vector2F b( 1.0f, 2.0f );
    Vector2F c( 3.0f, 4.0f );

    BOOST_CHECK( a == b );
    BOOST_CHECK( !( a != b ) );
    BOOST_CHECK( a != c );
    BOOST_CHECK( a.equals( b ) );
    BOOST_CHECK( !a.equals( c ) );
}

BOOST_AUTO_TEST_CASE( vector2_length_and_distance )
{
    Vector2F v( 3.0f, 4.0f );

    BOOST_CHECK( Math<float>::equals( v.length(), 5.0f ) );
    BOOST_CHECK( Math<float>::equals( v.lengthSquared(), 25.0f ) );

    Vector2F origin( 0.0f, 0.0f );
    BOOST_CHECK( Math<float>::equals( v.getDistanceFrom( origin ), 5.0f ) );
    BOOST_CHECK( Math<float>::equals( v.getDistanceFromSQ( origin ), 25.0f ) );
}

BOOST_AUTO_TEST_CASE( vector2_dot_product_and_perpendicular_helpers )
{
    Vector2F xAxis( 1.0f, 0.0f );
    Vector2F yAxis( 0.0f, 1.0f );
    Vector2F v( 3.0f, 4.0f );

    BOOST_CHECK( Math<float>::equals( xAxis.dotProduct( yAxis ), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( v.dotProduct( Vector2F( 2.0f, 5.0f ) ), 26.0f ) );
    checkVector2Close( v.perp(), Vector2F( 4.0f, -3.0f ) );
    BOOST_CHECK( Math<float>::equals( v.unitPerp().length(), 1.0f ) );
    BOOST_CHECK( Math<float>::equals( xAxis.dotPerp( yAxis ), 1.0f ) );
}

BOOST_AUTO_TEST_CASE( vector2_normalise_and_normalise_copy )
{
    Vector2F v( 3.0f, 4.0f );
    Vector2F normalised = v.normaliseCopy();

    checkVector2Close( normalised, Vector2F( 0.6f, 0.8f ) );
    checkVector2Close( v, Vector2F( 3.0f, 4.0f ) );

    v.normalise();
    BOOST_CHECK( Math<float>::equals( v.length(), 1.0f ) );

    Vector2F zero;
    zero.normalise();
    checkVector2Close( zero, Vector2F::ZERO );
    checkVector2Close( zero.normaliseCopy(), Vector2F::ZERO );
}

BOOST_AUTO_TEST_CASE( vector2_normalise_length_returns_original_length )
{
    Vector2F v( 3.0f, 4.0f );

    float originalLength = v.normaliseLength();

    BOOST_CHECK( Math<float>::equals( originalLength, 5.0f ) );
    BOOST_CHECK( Math<float>::equals( v.length(), 1.0f ) );

    Vector2F zero;
    BOOST_CHECK( Math<float>::equals( zero.normaliseLength(), 0.0f ) );
    checkVector2Close( zero, Vector2F::ZERO );
}

BOOST_AUTO_TEST_CASE( vector2_set_length_preserves_direction )
{
    Vector2F v( 3.0f, 4.0f );

    v.setLength( 10.0f );

    BOOST_CHECK( Math<float>::equals( v.length(), 10.0f ) );
    checkVector2Close( v, Vector2F( 6.0f, 8.0f ) );
}

BOOST_AUTO_TEST_CASE( vector2_rotate_by_rotates_around_center )
{
    Vector2F v( 1.0f, 0.0f );
    v.rotateBy( 90.0f, Vector2F::ZERO );

    BOOST_CHECK_SMALL( v.X(), 1.0e-4f );
    BOOST_CHECK_CLOSE( v.Y(), 1.0f, 0.001f );

    Vector2F aroundCenter( 2.0f, 1.0f );
    aroundCenter.rotateBy( 90.0f, Vector2F( 1.0f, 1.0f ) );
    BOOST_CHECK_CLOSE( aroundCenter.X(), 1.0f, 0.001f );
    BOOST_CHECK_CLOSE( aroundCenter.Y(), 2.0f, 0.001f );
}

BOOST_AUTO_TEST_CASE( vector2_angle_helpers )
{
    BOOST_CHECK( Math<float>::equals( Vector2F( 1.0f, 0.0f ).getAngleTrig(), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( Vector2F( 0.0f, 1.0f ).getAngleTrig(), 90.0f ) );
    BOOST_CHECK( Math<float>::equals( Vector2F( -1.0f, 0.0f ).getAngleTrig(), 180.0f ) );
    BOOST_CHECK( Math<float>::equals( Vector2F( 0.0f, -1.0f ).getAngleTrig(), 270.0f ) );

    BOOST_CHECK( Math<float>::equals( Vector2F( 1.0f, 0.0f ).getAngle(), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( Vector2F( 0.0f, -1.0f ).getAngle(), 90.0f ) );
    BOOST_CHECK( Math<float>::equals( Vector2F( -1.0f, 0.0f ).getAngle(), 180.0f ) );
    BOOST_CHECK( Math<float>::equals( Vector2F( 0.0f, 1.0f ).getAngle(), 270.0f ) );

    BOOST_CHECK(
        Math<float>::equals( Vector2F( 1.0f, 0.0f ).getAngleWith( Vector2F( 0.0f, 1.0f ) ), 90.0f ) );
    BOOST_CHECK(
        Math<float>::equals( Vector2F( 1.0f, 0.0f ).getAngleWith( Vector2F( 1.0f, 0.0f ) ), 0.0f ) );
}

BOOST_AUTO_TEST_CASE( vector2_between_points )
{
    Vector2F begin( 0.0f, 0.0f );
    Vector2F end( 10.0f, 0.0f );

    BOOST_CHECK( Vector2F( 5.0f, 0.0f ).isBetweenPoints( begin, end ) );
    BOOST_CHECK( !Vector2F( -1.0f, 0.0f ).isBetweenPoints( begin, end ) );
    BOOST_CHECK( !Vector2F( 11.0f, 0.0f ).isBetweenPoints( begin, end ) );
}

BOOST_AUTO_TEST_CASE( vector2_interpolation_helpers )
{
    Vector2F a( 0.0f, 0.0f );
    Vector2F b( 10.0f, 20.0f );

    checkVector2Close( a.getInterpolated( b, 0.0f ), b );
    checkVector2Close( a.getInterpolated( b, 1.0f ), a );
    checkVector2Close( a.getInterpolated( b, 0.5f ), Vector2F( 5.0f, 10.0f ) );

    Vector2F target;
    target.interpolate( a, b, 0.25f );
    checkVector2Close( target, Vector2F( 7.5f, 15.0f ) );

    Vector2F quadratic =
        Vector2F( 0.0f, 0.0f )
            .getInterpolated_quadratic( Vector2F( 10.0f, 0.0f ), Vector2F( 10.0f, 10.0f ), 0.5f );
    checkVector2Close( quadratic, Vector2F( 7.5f, 2.5f ) );
}

BOOST_AUTO_TEST_CASE( vector2_static_helpers_and_validity )
{
    BOOST_CHECK( Vector2F::ZERO == Vector2F( 0.0f, 0.0f ) );
    BOOST_CHECK( Vector2F::UNIT_X == Vector2F( 1.0f, 0.0f ) );
    BOOST_CHECK( Vector2F::UNIT_Y == Vector2F( 0.0f, 1.0f ) );
    BOOST_CHECK( Vector2F::UNIT == Vector2F( 1.0f, 1.0f ) );
    BOOST_CHECK( Vector2F::zero() == Vector2F::ZERO );
    BOOST_CHECK( Vector2F::unit() == Vector2F::UNIT );
    BOOST_CHECK( Vector2F( 1.0f, 2.0f ).isValid() );
    BOOST_CHECK( Vector2F( 1.0f, 2.0f ).isFinite() );
}
