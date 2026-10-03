#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;

namespace
{
    Matrix3F makeSampleMatrix()
    {
        return Matrix3F( 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f );
    }

    void checkFloatClose( float actual, float expected )
    {
        BOOST_CHECK( Math<float>::equals( actual, expected ) );
    }

    void checkVector3Close( const Vector3F &actual, const Vector3F &expected )
    {
        checkFloatClose( actual.X(), expected.X() );
        checkFloatClose( actual.Y(), expected.Y() );
        checkFloatClose( actual.Z(), expected.Z() );
    }

    void checkMatrixClose( const Matrix3F &actual, const Matrix3F &expected )
    {
        for( u32 row = 0; row < 3; ++row )
        {
            for( u32 col = 0; col < 3; ++col )
            {
                BOOST_CHECK( Math<float>::equals( actual[row][col], expected[row][col] ) );
            }
        }
    }
}  // namespace

BOOST_AUTO_TEST_CASE( matrix3_parameter_constructor_stores_row_major_values )
{
    Matrix3F matrix = makeSampleMatrix();

    checkFloatClose( matrix[0][0], 1.0f );
    checkFloatClose( matrix[0][2], 3.0f );
    checkFloatClose( matrix[1][0], 4.0f );
    checkFloatClose( matrix[2][1], 8.0f );
    checkFloatClose( matrix[2][2], 9.0f );
}

BOOST_AUTO_TEST_CASE( matrix3_copy_and_assignment_copy_values )
{
    Matrix3F matrix = makeSampleMatrix();
    Matrix3F copy( matrix );
    Matrix3F assigned = Matrix3F::identity();

    assigned = matrix;

    BOOST_CHECK( copy == matrix );
    BOOST_CHECK( assigned == matrix );
}

BOOST_AUTO_TEST_CASE( matrix3_row_access_is_mutable )
{
    Matrix3F matrix = Matrix3F::identity();

    matrix[0][1] = 2.0f;
    matrix[2][0] = 3.0f;

    checkFloatClose( matrix[0][1], 2.0f );
    checkFloatClose( matrix[2][0], 3.0f );
}

BOOST_AUTO_TEST_CASE( matrix3_identity_has_expected_values )
{
    Matrix3F identity = Matrix3F::identity();

    for( u32 row = 0; row < 3; ++row )
    {
        for( u32 col = 0; col < 3; ++col )
        {
            checkFloatClose( identity[row][col], row == col ? 1.0f : 0.0f );
        }
    }
}

BOOST_AUTO_TEST_CASE( matrix3_equality_and_inequality )
{
    Matrix3F a = makeSampleMatrix();
    Matrix3F b = makeSampleMatrix();
    Matrix3F c = Matrix3F::identity();

    BOOST_CHECK( a == b );
    BOOST_CHECK( !( a != b ) );
    BOOST_CHECK( a != c );
}

BOOST_AUTO_TEST_CASE( matrix3_addition_and_subtraction )
{
    Matrix3F a = makeSampleMatrix();
    Matrix3F b( 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f );

    Matrix3F sum = a + b;
    Matrix3F diff = a - b;

    checkMatrixClose( sum, Matrix3F( 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f ) );
    checkMatrixClose( diff, Matrix3F( -8.0f, -6.0f, -4.0f, -2.0f, 0.0f, 2.0f, 4.0f, 6.0f, 8.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix3_matrix_multiplication )
{
    Matrix3F a( 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f );
    Matrix3F b( 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f );

    Matrix3F result = a * b;

    checkMatrixClose( result,
                      Matrix3F( 30.0f, 24.0f, 18.0f, 84.0f, 69.0f, 54.0f, 138.0f, 114.0f, 90.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix3_scalar_multiplication_and_negation )
{
    Matrix3F matrix = makeSampleMatrix();

    checkMatrixClose( matrix * 2.0f,
                      Matrix3F( 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 18.0f ) );
    checkMatrixClose( -matrix,
                      Matrix3F( -1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f, -7.0f, -8.0f, -9.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix3_matrix_vector_multiplication )
{
    Matrix3F matrix = makeSampleMatrix();
    Vector3F vector( 1.0f, 2.0f, 3.0f );

    checkVector3Close( matrix * vector, Vector3F( 14.0f, 32.0f, 50.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix3_transpose_swaps_rows_and_columns )
{
    Matrix3F matrix = makeSampleMatrix();
    Matrix3F transposed = matrix.transpose();

    checkMatrixClose( transposed, Matrix3F( 1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix3_from_and_to_euler_xyz_round_trip )
{
    const float yaw = MathF::DegToRad( 10.0f );
    const float pitch = MathF::DegToRad( 20.0f );
    const float roll = MathF::DegToRad( 30.0f );

    Matrix3F matrix;
    matrix.fromEulerAnglesXYZ( yaw, pitch, roll );

    float outYaw = 0.0f;
    float outPitch = 0.0f;
    float outRoll = 0.0f;
    BOOST_CHECK( matrix.toEulerAnglesXYZ( outYaw, outPitch, outRoll ) );
    checkFloatClose( outYaw, yaw );
    checkFloatClose( outPitch, pitch );
    checkFloatClose( outRoll, roll );
}

BOOST_AUTO_TEST_CASE( matrix3_identity_euler_conversions_return_zero_angles )
{
    Matrix3F identity = Matrix3F::identity();
    float yaw = 1.0f;
    float pitch = 1.0f;
    float roll = 1.0f;

    BOOST_CHECK( identity.toEulerAnglesXYZ( yaw, pitch, roll ) );
    checkFloatClose( yaw, 0.0f );
    checkFloatClose( pitch, 0.0f );
    checkFloatClose( roll, 0.0f );

    BOOST_CHECK( identity.toEulerAnglesZYX( yaw, pitch, roll ) );
    checkFloatClose( yaw, 0.0f );
    checkFloatClose( pitch, 0.0f );
    checkFloatClose( roll, 0.0f );
}

BOOST_AUTO_TEST_CASE( matrix3_quaternion_rotation_matrix_interop )
{
    QuaternionF quaternion = QuaternionF::angleAxis( MathF::DegToRad( 90.0f ), Vector3F::UNIT_Z );
    Matrix3F matrix = Matrix3F::identity();

    quaternion.toRotationMatrix( matrix );
    Vector3F rotated = matrix * Vector3F::UNIT_X;

    BOOST_CHECK_SMALL( rotated.X(), 1.0e-4f );
    BOOST_CHECK_CLOSE( rotated.Y(), 1.0f, 0.001f );
    BOOST_CHECK_SMALL( rotated.Z(), 1.0e-4f );
}

BOOST_AUTO_TEST_CASE( matrix3_qdu_decomposition_extracts_diagonal_scale )
{
    Matrix3F matrix( 2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 4.0f );
    Matrix3F q = Matrix3F::identity();
    Vector3F d;
    Vector3F u;

    matrix.QDUDecomposition( q, d, u );

    checkMatrixClose( q, Matrix3F::identity() );
    checkVector3Close( d, Vector3F( 2.0f, 3.0f, 4.0f ) );
    checkVector3Close( u, Vector3F::ZERO );
}
