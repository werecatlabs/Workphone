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
}  // namespace

BOOST_AUTO_TEST_CASE( matrix2_identity_preserves_vector )
{
    Matrix2F identity = Matrix2F::identity();
    Vector2F vector( 3.0f, -4.0f );

    checkVector2Close( identity * vector, vector );
}

BOOST_AUTO_TEST_CASE( matrix2_angle_constructor_creates_rotation )
{
    Matrix2F rotation( MathF::pi() / 2.0f );
    Vector2F rotated = rotation * Vector2F::UNIT_X;

    BOOST_CHECK_SMALL( rotated.X(), 1.0e-4f );
    BOOST_CHECK_CLOSE( rotated.Y(), 1.0f, 0.001f );
}

BOOST_AUTO_TEST_CASE( matrix2_from_angle_creates_rotation )
{
    Matrix2F rotation;
    rotation.fromAngle( MathF::pi() );

    Vector2F rotated = rotation * Vector2F::UNIT_X;

    BOOST_CHECK_CLOSE( rotated.X(), -1.0f, 0.001f );
    BOOST_CHECK_SMALL( rotated.Y(), 1.0e-4f );
}

BOOST_AUTO_TEST_CASE( matrix2_to_angle_round_trips_rotation_angle )
{
    const float angle = MathF::DegToRad( 45.0f );
    Matrix2F rotation( angle );
    float extractedAngle = 0.0f;

    rotation.toAngle( extractedAngle );

    checkFloatClose( extractedAngle, angle );
}

BOOST_AUTO_TEST_CASE( matrix2_negative_angle_round_trips )
{
    const float angle = MathF::DegToRad( -30.0f );
    Matrix2F rotation( angle );
    float extractedAngle = 0.0f;

    rotation.toAngle( extractedAngle );

    checkFloatClose( extractedAngle, angle );
}

BOOST_AUTO_TEST_CASE( matrix2_copy_constructor_copies_rotation )
{
    Matrix2F rotation( MathF::DegToRad( 60.0f ) );
    Matrix2F copy( rotation );
    Vector2F vector( 2.0f, -1.0f );

    checkVector2Close( copy * vector, rotation * vector );
}

BOOST_AUTO_TEST_CASE( matrix2_assignment_copies_rotation )
{
    Matrix2F rotation( MathF::DegToRad( 120.0f ) );
    Matrix2F assigned = Matrix2F::identity();
    Vector2F vector( -3.0f, 5.0f );

    assigned = rotation;

    checkVector2Close( assigned * vector, rotation * vector );
}

BOOST_AUTO_TEST_CASE( matrix2_rotation_preserves_vector_length )
{
    Matrix2F rotation( MathF::DegToRad( 72.0f ) );
    Vector2F vector( 3.0f, 4.0f );
    Vector2F rotated = rotation * vector;

    checkFloatClose( rotated.length(), vector.length() );
}

BOOST_AUTO_TEST_CASE( matrix2_rotation_of_basis_vectors_is_orthogonal )
{
    Matrix2F rotation( MathF::DegToRad( 33.0f ) );
    Vector2F rotatedX = rotation * Vector2F::UNIT_X;
    Vector2F rotatedY = rotation * Vector2F::UNIT_Y;

    BOOST_CHECK_SMALL( rotatedX.dotProduct( rotatedY ), 1.0e-4f );
    checkFloatClose( rotatedX.length(), 1.0f );
    checkFloatClose( rotatedY.length(), 1.0f );
}

BOOST_AUTO_TEST_CASE( matrix2_double_identity_preserves_vector )
{
    Matrix2D identity = Matrix2D::identity();
    Vector2D vector( 1.25, -2.5 );
    Vector2D result = identity * vector;

    BOOST_CHECK( Math<double>::equals( result.X(), vector.X() ) );
    BOOST_CHECK( Math<double>::equals( result.Y(), vector.Y() ) );
}
