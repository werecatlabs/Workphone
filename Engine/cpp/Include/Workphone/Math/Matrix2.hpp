#ifndef __WP_MATRIX_2_H_INCLUDED__
#define __WP_MATRIX_2_H_INCLUDED__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    /**
     * @brief Simple 2x2 matrix class for 2D linear transforms and rotations.
     *
     * Matrix elements are stored in row-major order:
     * @code
     * [ M[0]  M[1] ]
     * [ M[2]  M[3] ]
     * @endcode
     *
     * This class provides:
     * - construction and copy
     * - multiplication by a 2D vector (M * v)
     * - setting the matrix from a rotation angle
     * - extracting a rotation angle from a rotation matrix
     * - obtaining an identity matrix
     *
     * Note: The template parameter T should typically be a floating-point type
     * (for example, `float` or `double`) because trigonometric functions are used.
     *
     * @tparam T Numeric type of matrix elements (e.g. `float`, `double`).
     */
    template <class T>
    class WPCore_API Matrix2
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Does not initialize the matrix elements (kept minimal). Use
         * `Matrix2<T>::identity()` or `fromAngle()` to initialize to a known value.
         */
        Matrix2();

        /**
         * @brief Copy constructor.
         *
         * Performs a memberwise copy of the matrix elements.
         *
         * @param other Matrix to copy from.
         */
        Matrix2( const Matrix2 &other );

        /**
         * @brief Construct a rotation matrix from an angle (radians).
         *
         * The constructor taking an angle sets this matrix to the 2D rotation
         * corresponding to the provided angle (counter-clockwise, right-handed).
         *
         * @param angle Rotation angle in radians.
         */
        Matrix2( T angle );

        /**
         * @brief Matrix-times-vector multiplication (M * v).
         *
         * Returns the result of multiplying this matrix by the provided
         * column vector `rkV`. The computation performed is:
         * x' = M[0]*v.x + M[1]*v.y
         * y' = M[2]*v.x + M[3]*v.y
         *
         * @param rkV Right-hand side vector to multiply.
         * @return Resulting transformed vector.
         */
        Vector2<T> operator*( const Vector2<T> &rkV ) const;

        /**
         * @brief Set this matrix to a rotation by the specified angle (radians).
         *
         * Produces the standard 2D rotation matrix:
         * [ cos(theta)  -sin(theta) ]
         * [ sin(theta)   cos(theta) ]
         *
         * After calling this method this matrix represents a rotation about
         * the Z axis by `fAngle` radians (counter-clockwise).
         *
         * @param fAngle Angle to rotate by, in radians.
         */
        void fromAngle( T fAngle );

        /**
         * @brief Extract the rotation angle from this matrix.
         *
         * Interprets this matrix as a pure rotation matrix and computes the
         * corresponding angle in radians. The result is in the range [-pi, pi]
         * (depending on the implementation of ATan2).
         *
         * This method assumes the matrix is a valid rotation (orthonormal with
         * determinant +1). Behavior is undefined for arbitrary matrices.
         *
         * @param rfAngle Output parameter that will contain the rotation angle in radians.
         */
        void toAngle( T &rfAngle ) const;

        /**
         * @brief Return an identity matrix.
         *
         * Produces the 2x2 identity matrix:
         * [ 1  0 ]
         * [ 0  1 ]
         *
         * @return Identity Matrix2<T>.
         */
        static Matrix2 identity();

    private:
        //! Matrix data, stored in row-major order: [ M[0] M[1] ; M[2] M[3] ].
        T M[4];
    };

    using Matrix2F = Matrix2<f32>;
    using Matrix2D = Matrix2<f64>;

}  // namespace workphone

#endif
