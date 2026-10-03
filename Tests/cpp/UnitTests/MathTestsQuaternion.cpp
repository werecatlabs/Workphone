#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( testQuaternionConstructor )
{
    QuaternionF q( 1.0f, 2.0f, 3.0f, 4.0f );
    BOOST_CHECK( q.w == 1.0f );
    BOOST_CHECK( q.x == 2.0f );
    BOOST_CHECK( q.y == 3.0f );
    BOOST_CHECK( q.z == 4.0f );
}

BOOST_AUTO_TEST_CASE( testQuaternionIdentity )
{
    QuaternionF q = QuaternionF::identity();
    BOOST_CHECK( q.w == 1.0f );
    BOOST_CHECK( q.x == 0.0f );
    BOOST_CHECK( q.y == 0.0f );
    BOOST_CHECK( q.z == 0.0f );
}

BOOST_AUTO_TEST_CASE( testQuaternionMultiplication )
{
    QuaternionF q1( 1.0f, 2.0f, 3.0f, 4.0f );
    QuaternionF q2( 5.0f, 6.0f, 7.0f, 8.0f );
    QuaternionF result = q1 * q2;
    BOOST_CHECK( result.w == -60.0f );
    BOOST_CHECK( result.x == 12.0f );
    BOOST_CHECK( result.y == 30.0f );
    BOOST_CHECK( result.z == 24.0f );
}

BOOST_AUTO_TEST_CASE( testQuaternionInverse )
{
    QuaternionF q( 1.0f, 2.0f, 3.0f, 4.0f );
    QuaternionF result = q.inverse();
    BOOST_TEST( result.w == 1.0f / 30.0f, boost::test_tools::tolerance( 1e-6f ) );
    BOOST_TEST( result.x == -2.0f / 30.0f, boost::test_tools::tolerance( 1e-6f ) );
    BOOST_TEST( result.y == -3.0f / 30.0f, boost::test_tools::tolerance( 1e-6f ) );
    BOOST_TEST( result.z == -4.0f / 30.0f, boost::test_tools::tolerance( 1e-6f ) );
}

BOOST_AUTO_TEST_CASE( testQuaternionDoubleInverse )
{
    QuaternionD q( 1.0, 2.0, 3.0f, 4.0f );
    QuaternionD result = q.inverse();
    BOOST_TEST( result.w == 1.0 / 30.0, boost::test_tools::tolerance( 1e-6f ) );
    BOOST_TEST( result.x == -2.0 / 30.0, boost::test_tools::tolerance( 1e-6f ) );
    BOOST_TEST( result.y == -3.0 / 30.0, boost::test_tools::tolerance( 1e-6f ) );
    BOOST_TEST( result.z == -4.0 / 30.0, boost::test_tools::tolerance( 1e-6f ) );
}
