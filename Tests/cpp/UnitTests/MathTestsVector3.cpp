#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( math_vector3_normalise )
{
    try
    {
        Vector3<real_Num> object( 10, 10, 10 );
        object.normalise();

        BOOST_CHECK( object.length() > static_cast<real_Num>( 0.975 ) );
        BOOST_CHECK( Math<real_Num>::equals( object.length(), static_cast<real_Num>( 1.0 ) ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( math_vector3_dot )
{
    try
    {
        Vector3<real_Num> vecDot1( 1, 0, 0 );
        vecDot1.normalise();

        Vector3<real_Num> vecDot2( 1, 0, 0 );
        vecDot2.normalise();

        BOOST_CHECK( vecDot1.dotProduct( vecDot2 ) > static_cast<real_Num>( 0.85 ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( math_vector3_cross )
{
    try
    {
        Vector3<real_Num> right = Vector3<real_Num>::right();
        right.normalise();

        Vector3<real_Num> forward = Vector3<real_Num>::forward();
        forward.normalise();

        Vector3<real_Num> res = right.crossProduct( forward );

        BOOST_CHECK( right.crossProduct( forward ).dotProduct( Vector3<real_Num>::up() ) >
                     static_cast<real_Num>( 0.85 ) );
        BOOST_CHECK( forward.crossProduct( right ).dotProduct( Vector3<real_Num>::down() ) >
                     static_cast<real_Num>( 0.85 ) );

        right = Vector3<real_Num>( 0, 0, 1 );
        forward = Vector3<real_Num>( 1, 0, 0 );
        res = right.crossProduct( forward );

        BOOST_CHECK( right.crossProduct( forward ).dotProduct( Vector3<real_Num>::up() ) >
                     static_cast<real_Num>( 0.85 ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( testVector3Constructor )
{
    Vector3F v( 1.0f, 2.0f, 3.0f );
    BOOST_TEST( v.x == 1.0f );
    BOOST_TEST( v.y == 2.0f );
    BOOST_TEST( v.z == 3.0f );
}

BOOST_AUTO_TEST_CASE( testVector3Addition )
{
    Vector3F v1( 1.0f, 2.0f, 3.0f );
    Vector3F v2( 4.0f, 5.0f, 6.0f );
    Vector3F result = v1 + v2;
    BOOST_TEST( result.x == 5.0f );
    BOOST_TEST( result.y == 7.0f );
    BOOST_TEST( result.z == 9.0f );
}

BOOST_AUTO_TEST_CASE( testVector3Subtraction )
{
    Vector3F v1( 1.0f, 2.0f, 3.0f );
    Vector3F v2( 4.0f, 5.0f, 6.0f );
    Vector3F result = v1 - v2;
    BOOST_TEST( result.x == -3.0f );
    BOOST_TEST( result.y == -3.0f );
    BOOST_TEST( result.z == -3.0f );
}

BOOST_AUTO_TEST_CASE( testVector3Multiplication )
{
    Vector3F v( 1.0f, 2.0f, 3.0f );
    Vector3F result = v * 2.0f;
    BOOST_TEST( result.x == 2.0f );
    BOOST_TEST( result.y == 4.0f );
    BOOST_TEST( result.z == 6.0f );
}

BOOST_AUTO_TEST_CASE( testVector3Division )
{
    Vector3F v( 2.0f, 4.0f, 6.0f );
    Vector3F result = v / 2.0f;
    BOOST_TEST( result.x == 1.0f );
    BOOST_TEST( result.y == 2.0f );
    BOOST_TEST( result.z == 3.0f );
}

BOOST_AUTO_TEST_CASE( testVector3Magnitude )
{
    Vector3F v( 3.0f, 4.0f, 0.0f );
    float result = v.length();
    BOOST_TEST( result == 5.0f );
}

BOOST_AUTO_TEST_CASE( testVector3Normalization )
{
    Vector3F v( 3.0f, 4.0f, 0.0f );
    Vector3F result = v.normaliseCopy();
    BOOST_TEST( result.x == 0.6f, boost::test_tools::tolerance( 1e-6f ) );
    BOOST_TEST( result.y == 0.8f, boost::test_tools::tolerance( 1e-6f ) );
    BOOST_TEST( result.z == 0.0f, boost::test_tools::tolerance( 1e-6f ) );
    // Normalised vector must have unit length
    BOOST_CHECK( Math<float>::equals( result.length(), 1.0f ) );

    // Zero-vector: normalising should not crash and length stays 0 or remains safe
    Vector3F zero( 0.0f, 0.0f, 0.0f );
    Vector3F zeroNorm = zero.normaliseCopy();
    BOOST_CHECK( !Math<float>::isNaN( zeroNorm.x ) );
}

BOOST_AUTO_TEST_CASE( vector3_accessor_X_Y_Z_const )
{
    const Vector3F v( 1.0f, 2.0f, 3.0f );
    BOOST_CHECK( Math<float>::equals( v.X(), 1.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Y(), 2.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Z(), 3.0f ) );
}

BOOST_AUTO_TEST_CASE( vector3_accessor_X_Y_Z_mutable )
{
    Vector3F v( 0.0f, 0.0f, 0.0f );
    v.X() = 10.0f;
    v.Y() = 20.0f;
    v.Z() = 30.0f;
    BOOST_CHECK( Math<float>::equals( v.X(), 10.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Y(), 20.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Z(), 30.0f ) );
    // Public members should be consistent with accessors
    BOOST_CHECK( Math<float>::equals( v.x, 10.0f ) );
    BOOST_CHECK( Math<float>::equals( v.y, 20.0f ) );
    BOOST_CHECK( Math<float>::equals( v.z, 30.0f ) );
}

BOOST_AUTO_TEST_CASE( vector3_subscript_operator )
{
    Vector3F v( 4.0f, 5.0f, 6.0f );
    BOOST_CHECK( Math<float>::equals( v[0], 4.0f ) );
    BOOST_CHECK( Math<float>::equals( v[1], 5.0f ) );
    BOOST_CHECK( Math<float>::equals( v[2], 6.0f ) );

    // Mutable subscript
    v[0] = 7.0f;
    BOOST_CHECK( Math<float>::equals( v.X(), 7.0f ) );
}

BOOST_AUTO_TEST_CASE( vector3_set_method )
{
    Vector3F v( 0.0f, 0.0f, 0.0f );
    v.set( 1.0f, 2.0f, 3.0f );
    BOOST_CHECK( Math<float>::equals( v.X(), 1.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Y(), 2.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Z(), 3.0f ) );

    Vector3F other( 9.0f, 8.0f, 7.0f );
    v.set( other );
    BOOST_CHECK( v == other );
}

BOOST_AUTO_TEST_CASE( vector3_length_squared )
{
    Vector3F v( 3.0f, 4.0f, 0.0f );
    // lengthSquared should equal length^2
    float lenSq = v.lengthSquared();
    float len = v.length();
    BOOST_CHECK( Math<float>::equals( lenSq, len * len ) );
    BOOST_CHECK( Math<float>::equals( lenSq, 25.0f ) );

    // Zero vector
    Vector3F zero( 0.0f, 0.0f, 0.0f );
    BOOST_CHECK( Math<float>::equals( zero.lengthSquared(), 0.0f ) );
}

BOOST_AUTO_TEST_CASE( vector3_get_distance_from )
{
    Vector3F a( 0.0f, 0.0f, 0.0f );
    Vector3F b( 3.0f, 4.0f, 0.0f );
    BOOST_CHECK( Math<float>::equals( a.getDistanceFrom( b ), 5.0f ) );
    // Distance is symmetric
    BOOST_CHECK( Math<float>::equals( b.getDistanceFrom( a ), 5.0f ) );
    // Distance from self is zero
    BOOST_CHECK( Math<float>::equals( a.getDistanceFrom( a ), 0.0f ) );
}

BOOST_AUTO_TEST_CASE( vector3_equals_with_tolerance )
{
    Vector3F a( 1.0f, 2.0f, 3.0f );
    Vector3F b( 1.0f + 1e-8f, 2.0f, 3.0f );
    // Should be equal within default tolerance
    BOOST_CHECK( a.equals( b ) );

    Vector3F c( 1.0f + 1.0f, 2.0f, 3.0f );
    // Should not be equal when difference is large
    BOOST_CHECK( !a.equals( c ) );
}

BOOST_AUTO_TEST_CASE( vector3_negation_operator )
{
    Vector3F v( 1.0f, -2.0f, 3.0f );
    Vector3F neg = -v;
    BOOST_CHECK( Math<float>::equals( neg.X(), -1.0f ) );
    BOOST_CHECK( Math<float>::equals( neg.Y(), 2.0f ) );
    BOOST_CHECK( Math<float>::equals( neg.Z(), -3.0f ) );
}

BOOST_AUTO_TEST_CASE( vector3_compound_assignment )
{
    Vector3F v( 1.0f, 2.0f, 3.0f );
    v += Vector3F( 1.0f, 1.0f, 1.0f );
    BOOST_CHECK( Math<float>::equals( v.X(), 2.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Y(), 3.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Z(), 4.0f ) );

    v -= Vector3F( 1.0f, 1.0f, 1.0f );
    BOOST_CHECK( Math<float>::equals( v.X(), 1.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Y(), 2.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Z(), 3.0f ) );

    v *= 2.0f;
    BOOST_CHECK( Math<float>::equals( v.X(), 2.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Y(), 4.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Z(), 6.0f ) );

    v /= 2.0f;
    BOOST_CHECK( Math<float>::equals( v.X(), 1.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Y(), 2.0f ) );
    BOOST_CHECK( Math<float>::equals( v.Z(), 3.0f ) );
}

BOOST_AUTO_TEST_CASE( vector3_comparison_operators )
{
    Vector3F a( 1.0f, 2.0f, 3.0f );
    Vector3F b( 1.0f, 2.0f, 3.0f );
    Vector3F c( 4.0f, 5.0f, 6.0f );
    BOOST_CHECK( a == b );
    BOOST_CHECK( a != c );
    BOOST_CHECK( a < c );
    BOOST_CHECK( c > a );
    BOOST_CHECK( a <= b );
    BOOST_CHECK( b >= a );
}

BOOST_AUTO_TEST_CASE( vector3_static_helpers )
{
    BOOST_CHECK( Vector3F::ZERO == Vector3F( 0.0f, 0.0f, 0.0f ) );
    BOOST_CHECK( Math<float>::equals( Vector3F::up().Y(), 1.0f ) );
    BOOST_CHECK( Math<float>::equals( Vector3F::down().Y(), -1.0f ) );
    BOOST_CHECK( Math<float>::equals( Vector3F::right().X(), 1.0f ) );
    BOOST_CHECK_CLOSE( Math<float>::Abs( Vector3F::forward().Z() ), 1.0f, 0.001f );
}

BOOST_AUTO_TEST_CASE( vector3_dot_product_edge_cases )
{
    // Perpendicular vectors: dot product == 0
    Vector3F x( 1.0f, 0.0f, 0.0f );
    Vector3F y( 0.0f, 1.0f, 0.0f );
    BOOST_CHECK( Math<float>::equals( x.dotProduct( y ), 0.0f ) );

    // Parallel (same direction): dot product == product of magnitudes
    Vector3F a( 2.0f, 0.0f, 0.0f );
    Vector3F b( 3.0f, 0.0f, 0.0f );
    BOOST_CHECK( Math<float>::equals( a.dotProduct( b ), 6.0f ) );

    // Anti-parallel: dot product is negative
    BOOST_CHECK( a.dotProduct( -b ) < 0.0f );
}

BOOST_AUTO_TEST_CASE( vector3_cross_product_identities )
{
    Vector3F x( 1.0f, 0.0f, 0.0f );
    Vector3F y( 0.0f, 1.0f, 0.0f );
    Vector3F z( 0.0f, 0.0f, 1.0f );

    // x cross y == z
    Vector3F xy = x.crossProduct( y );
    BOOST_CHECK( xy.equals( z ) );

    // y cross x == -z (anti-commutative)
    Vector3F yx = y.crossProduct( x );
    BOOST_CHECK( yx.equals( -z ) );

    // v cross v == zero
    Vector3F self = x.crossProduct( x );
    BOOST_CHECK( self.equals( Vector3F::ZERO ) );
}
