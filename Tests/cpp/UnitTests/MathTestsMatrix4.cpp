#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;

namespace
{
    Matrix4F makeSampleMatrix()
    {
        return Matrix4F( 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f,
                         13.0f, 14.0f, 15.0f, 16.0f );
    }

    Matrix4F makeTranslationMatrix( const Vector3F &translation )
    {
        Matrix4F matrix = Matrix4F::identity();
        matrix[0][3] = translation.X();
        matrix[1][3] = translation.Y();
        matrix[2][3] = translation.Z();
        return matrix;
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

    void checkVector4Close( const Vector4F &actual, const Vector4F &expected )
    {
        checkFloatClose( actual.X(), expected.X() );
        checkFloatClose( actual.Y(), expected.Y() );
        checkFloatClose( actual.Z(), expected.Z() );
        checkFloatClose( actual.W(), expected.W() );
    }

    void checkMatrixClose( const Matrix4F &actual, const Matrix4F &expected )
    {
        for( u32 row = 0; row < 4; ++row )
        {
            for( u32 col = 0; col < 4; ++col )
            {
                BOOST_CHECK( Math<float>::equals( actual[row][col], expected[row][col] ) );
            }
        }
    }
}  // namespace

BOOST_AUTO_TEST_CASE( matrix4_parameter_constructor_stores_row_major_values )
{
    Matrix4F matrix = makeSampleMatrix();

    checkFloatClose( matrix[0][0], 1.0f );
    checkFloatClose( matrix[0][3], 4.0f );
    checkFloatClose( matrix[1][0], 5.0f );
    checkFloatClose( matrix[2][2], 11.0f );
    checkFloatClose( matrix[3][3], 16.0f );
}

BOOST_AUTO_TEST_CASE( matrix4_copy_and_pointer_constructor_copy_values )
{
    Matrix4F matrix = makeSampleMatrix();
    Matrix4F copied( matrix );
    Matrix4F fromPointer( matrix.ptr() );

    checkMatrixClose( copied, matrix );
    checkMatrixClose( fromPointer, matrix );
}

BOOST_AUTO_TEST_CASE( matrix4_ptr_and_row_access_are_mutable )
{
    Matrix4F matrix = Matrix4F::zero();

    matrix.ptr()[0] = 1.0f;
    matrix.ptr()[15] = 16.0f;
    matrix[2][1] = 9.0f;

    checkFloatClose( matrix[0][0], 1.0f );
    checkFloatClose( matrix[3][3], 16.0f );
    checkFloatClose( matrix.ptr()[9], 9.0f );
}

BOOST_AUTO_TEST_CASE( matrix4_static_zero_zeroaffine_and_identity )
{
    Matrix4F zero = Matrix4F::zero();
    Matrix4F zeroAffine = Matrix4F::zeroaffine();
    Matrix4F identity = Matrix4F::identity();

    for( u32 row = 0; row < 4; ++row )
    {
        for( u32 col = 0; col < 4; ++col )
        {
            checkFloatClose( zero[row][col], 0.0f );
            checkFloatClose( identity[row][col], row == col ? 1.0f : 0.0f );
        }
    }

    BOOST_CHECK( !zero.isAffine() );
    BOOST_CHECK( zeroAffine.isAffine() );
    BOOST_CHECK( identity.isAffine() );
}

BOOST_AUTO_TEST_CASE( matrix4_get_row_returns_requested_row )
{
    Matrix4F matrix = makeSampleMatrix();

    checkVector4Close( matrix.getRow( 2 ), Vector4F( 9.0f, 10.0f, 11.0f, 12.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix4_transform_affine_transforms_vector3 )
{
    Matrix4F matrix = Matrix4F::identity();
    matrix[0][0] = 2.0f;
    matrix[1][1] = 3.0f;
    matrix[2][2] = 4.0f;
    matrix[0][3] = 10.0f;
    matrix[1][3] = 20.0f;
    matrix[2][3] = 30.0f;

    Vector3F result = matrix.transformAffine( Vector3F( 1.0f, 2.0f, 3.0f ) );

    checkVector3Close( result, Vector3F( 12.0f, 26.0f, 42.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix4_transform_affine_transforms_vector4 )
{
    Matrix4F matrix = Matrix4F::identity();
    matrix[0][0] = 2.0f;
    matrix[1][1] = 3.0f;
    matrix[2][2] = 4.0f;
    matrix[0][3] = 10.0f;
    matrix[1][3] = 20.0f;
    matrix[2][3] = 30.0f;

    Vector4F result = matrix.transformAffine( Vector4F( 1.0f, 2.0f, 3.0f, 2.0f ) );

    checkVector4Close( result, Vector4F( 22.0f, 46.0f, 72.0f, 2.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix4_vector3_operator_multiplies_identity )
{
    Matrix4F identity = Matrix4F::identity();

    Vector3F result = identity * Vector3F( 1.0f, 2.0f, 3.0f );

    checkVector3Close( result, Vector3F( 1.0f, 2.0f, 3.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix4_vector4_times_matrix_multiplies_as_row_vector )
{
    Matrix4F matrix = makeSampleMatrix();

    Vector4F result = Vector4F( 1.0f, 2.0f, 3.0f, 4.0f ) * matrix;

    checkVector4Close( result, Vector4F( 90.0f, 100.0f, 110.0f, 120.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix4_transpose_swaps_rows_and_columns )
{
    Matrix4F matrix = makeSampleMatrix();
    Matrix4F transposed = matrix.transpose();

    checkFloatClose( transposed[0][1], 5.0f );
    checkFloatClose( transposed[1][0], 2.0f );
    checkFloatClose( transposed[2][3], 15.0f );
    checkFloatClose( transposed[3][2], 12.0f );
}

BOOST_AUTO_TEST_CASE( matrix4_concatenate_matches_operator_multiply )
{
    Matrix4F left = makeTranslationMatrix( Vector3F( 1.0f, 2.0f, 3.0f ) );
    Matrix4F right = Matrix4F::identity();
    right[0][0] = 2.0f;
    right[1][1] = 3.0f;
    right[2][2] = 4.0f;

    Matrix4F concatenated = left.concatenate( right );
    Matrix4F multiplied = left * right;

    checkMatrixClose( concatenated, multiplied );
    checkVector3Close( concatenated.transformAffine( Vector3F( 1.0f, 1.0f, 1.0f ) ),
                       Vector3F( 3.0f, 5.0f, 7.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix4_concatenate_affine_combines_transforms )
{
    Matrix4F first = makeTranslationMatrix( Vector3F( 1.0f, 2.0f, 3.0f ) );
    Matrix4F second = makeTranslationMatrix( Vector3F( 10.0f, 20.0f, 30.0f ) );

    Matrix4F combined = first.concatenateAffine( second );

    BOOST_CHECK( combined.isAffine() );
    checkVector3Close( combined.transformAffine( Vector3F::ZERO ), Vector3F( 11.0f, 22.0f, 33.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix4_inverse_inverts_affine_scale_and_translation )
{
    Matrix4F matrix = Matrix4F::identity();
    matrix[0][0] = 2.0f;
    matrix[1][1] = 4.0f;
    matrix[2][2] = 5.0f;
    matrix[0][3] = 10.0f;
    matrix[1][3] = 20.0f;
    matrix[2][3] = 30.0f;

    Matrix4F inverse = matrix.inverse();
    Matrix4F product = matrix * inverse;

    checkMatrixClose( product, Matrix4F::identity() );
}

BOOST_AUTO_TEST_CASE( matrix4_make_transform_builds_affine_transform )
{
    Matrix4F matrix;
    matrix.makeTransform( Vector3F( 10.0f, 20.0f, 30.0f ), Vector3F( 2.0f, 3.0f, 4.0f ),
                          QuaternionF::identity() );

    BOOST_CHECK( matrix.isAffine() );
    checkVector3Close( matrix.transformAffine( Vector3F( 1.0f, 2.0f, 3.0f ) ),
                       Vector3F( 12.0f, 26.0f, 42.0f ) );
}

BOOST_AUTO_TEST_CASE( matrix4_extract_rotation_matrix_copies_upper_3x3 )
{
    Matrix4F matrix = makeSampleMatrix();
    Matrix3F rotation = Matrix3F::identity();

    matrix.getRotationMat( rotation );

    checkFloatClose( rotation[0][0], 1.0f );
    checkFloatClose( rotation[0][1], 2.0f );
    checkFloatClose( rotation[0][2], 3.0f );
    checkFloatClose( rotation[1][0], 5.0f );
    checkFloatClose( rotation[1][1], 6.0f );
    checkFloatClose( rotation[1][2], 7.0f );
    checkFloatClose( rotation[2][0], 9.0f );
    checkFloatClose( rotation[2][1], 10.0f );
    checkFloatClose( rotation[2][2], 11.0f );
}

BOOST_AUTO_TEST_CASE( matrix4_make_perspective_sets_projection_shape )
{
    Matrix4F matrix;
    matrix.makePerspective( MathF::DegToRad( 90.0f ), 1.0f, 1.0f, 100.0f );

    BOOST_CHECK( !matrix.isAffine() );
    BOOST_CHECK_CLOSE( matrix[0][0], 1.0f, 0.001f );
    BOOST_CHECK_CLOSE( matrix[1][1], 1.0f, 0.001f );
    BOOST_CHECK( matrix[2][2] < 0.0f );
    BOOST_CHECK( matrix[2][3] < 0.0f );
    checkFloatClose( matrix[3][2], -1.0f );
    checkFloatClose( matrix[3][3], 0.0f );
}
