#ifndef __WP_MATRIX_3_H_INCLUDED__
#define __WP_MATRIX_3_H_INCLUDED__

#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /**
     * @brief A 3x3 matrix class template for linear transformations.
     *
     * This class represents a 3x3 matrix commonly used for 3D rotations, scaling,
     * and other linear transformations. The matrix is stored in row-major order.
     * Matrix elements can be accessed via the m[row][column] notation or as a
     * flat array using m_[index].
     *
     * @tparam T The data type of the matrix elements (typically float or double).
     *
     * @note The matrix uses row-major storage where m[i][j] represents the element
     *       at row i and column j.
     *
     * @see Vector3
     */
    template <class T>
    class WPCore_API Matrix3
    {
    public:
        /**
         * @brief Default constructor. Initializes the matrix to uninitialized values.
         *
         * @warning The matrix elements are not initialized. Use identity() for an
         *          identity matrix or explicitly set values.
         */
        Matrix3();

        /**
         * @brief Copy constructor.
         *
         * Creates a new matrix by copying all elements from another matrix.
         *
         * @param other The matrix to copy from.
         */
        Matrix3( const Matrix3 &other );

        /**
         * @brief Constructs a matrix with the given entries.
         *
         * Creates a 3x3 matrix with explicitly specified values for each element.
         * Elements are specified in row-major order.
         *
         * @param fEntry00 The element at row 0, column 0.
         * @param fEntry01 The element at row 0, column 1.
         * @param fEntry02 The element at row 0, column 2.
         * @param fEntry10 The element at row 1, column 0.
         * @param fEntry11 The element at row 1, column 1.
         * @param fEntry12 The element at row 1, column 2.
         * @param fEntry20 The element at row 2, column 0.
         * @param fEntry21 The element at row 2, column 1.
         * @param fEntry22 The element at row 2, column 2.
         */
        Matrix3( T fEntry00, T fEntry01, T fEntry02, T fEntry10, T fEntry11, T fEntry12, T fEntry20,
                 T fEntry21, T fEntry22 );

        /**
         * @brief Array subscript operator for accessing matrix rows.
         *
         * Allows matrix elements to be accessed using the [][] operator notation.
         * Example: matrix[0][1] accesses the element at row 0, column 1.
         *
         * @param rowIndex The row index (0-2) to access.
         * @return A pointer to the specified row, allowing further column indexing.
         *
         * @warning No bounds checking is performed. Ensure rowIndex is in range [0, 2].
         */
        T *operator[]( u32 rowIndex ) const;

        /**
         * @brief Assignment operator.
         *
         * Copies all elements from the source matrix to this matrix.
         *
         * @param rkMatrix The matrix to assign from.
         * @return A reference to this matrix after assignment.
         */
        Matrix3 &operator=( const Matrix3 &rkMatrix );

        /**
         * @brief Equality comparison operator.
         *
         * Tests whether two matrices are equal by comparing all elements.
         *
         * @param rkMatrix The matrix to compare with.
         * @return true if all corresponding elements are equal, false otherwise.
         */
        bool operator==( const Matrix3 &rkMatrix ) const;

        /**
         * @brief Inequality comparison operator.
         *
         * Tests whether two matrices are unequal.
         *
         * @param rkMatrix The matrix to compare with.
         * @return true if any corresponding elements differ, false otherwise.
         */
        bool operator!=( const Matrix3 &rkMatrix ) const;

        /**
         * @brief Matrix addition operator.
         *
         * Performs element-wise addition of two matrices.
         *
         * @param rkMatrix The matrix to add.
         * @return A new matrix containing the sum of the two matrices.
         */
        Matrix3 operator+( const Matrix3 &rkMatrix ) const;

        /**
         * @brief Matrix subtraction operator.
         *
         * Performs element-wise subtraction of two matrices.
         *
         * @param rkMatrix The matrix to subtract.
         * @return A new matrix containing the difference between the two matrices.
         */
        Matrix3 operator-( const Matrix3 &rkMatrix ) const;

        /**
         * @brief Matrix multiplication operator.
         *
         * Performs standard matrix multiplication (this * rkMatrix).
         *
         * @param rkMatrix The matrix to multiply by.
         * @return A new matrix containing the product of the two matrices.
         *
         * @note Matrix multiplication is not commutative: A * B ≠ B * A
         */
        Matrix3 operator*( const Matrix3 &rkMatrix ) const;

        /**
         * @brief Unary negation operator.
         *
         * Negates all elements of the matrix.
         *
         * @return A new matrix with all elements negated.
         */
        Matrix3 operator-() const;

        /**
         * @brief Matrix-vector multiplication operator.
         *
         * Multiplies this matrix by a 3D vector, treating the vector as a column vector.
         * Computes: result = matrix * vector
         *
         * @param rkVector The 3D vector to multiply by.
         * @return The resulting 3D vector after the transformation.
         */
        Vector3<T> operator*( const Vector3<T> &rkVector ) const;

        /**
         * @brief Vector-matrix multiplication operator (friend function).
         *
         * Multiplies a 3D vector by a matrix, treating the vector as a row vector.
         * Computes: result = vector * matrix
         *
         * @tparam B The data type of the vector and matrix elements.
         * @param rkVector The 3D vector to multiply.
         * @param rkMatrix The matrix to multiply by.
         * @return The resulting 3D vector after the transformation.
         */
        template <class B>
        friend Vector3<B> operator*( const Vector3<B> &rkVector, const Matrix3<B> &rkMatrix );

        /**
         * @brief Scalar multiplication operator.
         *
         * Multiplies all elements of the matrix by a scalar value.
         *
         * @param fScalar The scalar value to multiply by.
         * @return A new matrix with all elements scaled by the scalar.
         */
        Matrix3 operator*( T fScalar ) const;

        /**
         * @brief Scalar-matrix multiplication operator (friend function).
         *
         * Multiplies a scalar by a matrix, scaling all matrix elements.
         * Commutative with matrix * scalar.
         *
         * @tparam B The data type of the scalar and matrix elements.
         * @param fScalar The scalar to multiply.
         * @param rkMatrix The matrix to multiply by the scalar.
         * @return A new matrix with all elements scaled by the scalar.
         */
        template <class B>
        friend Matrix3<B> operator*( B fScalar, const Matrix3<B> &rkMatrix );

        /**
         * @brief Converts the rotation matrix to Euler angles using XYZ convention.
         *
         * Extracts Euler angles from an orthonormal rotation matrix with XYZ rotation order.
         * The rotations are applied in order: X (pitch), then Y (yaw), then Z (roll).
         *
         * @param[out] rfYAngle The yaw angle in radians (rotation about Y-axis).
         * @param[out] rfPAngle The pitch angle in radians (rotation about X-axis).
         * @param[out] rfRAngle The roll angle in radians (rotation about Z-axis).
         * @return true if the extraction succeeds, false if the matrix is not orthonormal.
         *
         * @warning The matrix must be orthonormal (a pure rotation matrix) for accurate results.
         */
        bool toEulerAnglesXYZ( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const;

        /**
         * @brief Converts the rotation matrix to Euler angles using XZY convention.
         *
         * Extracts Euler angles from an orthonormal rotation matrix with XZY rotation order.
         * The rotations are applied in order: X (pitch), then Z (roll), then Y (yaw).
         *
         * @param[out] rfYAngle The yaw angle in radians (rotation about Y-axis).
         * @param[out] rfPAngle The pitch angle in radians (rotation about X-axis).
         * @param[out] rfRAngle The roll angle in radians (rotation about Z-axis).
         * @return true if the extraction succeeds, false if the matrix is not orthonormal.
         *
         * @warning The matrix must be orthonormal (a pure rotation matrix) for accurate results.
         */
        bool toEulerAnglesXZY( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const;

        /**
         * @brief Converts the rotation matrix to Euler angles using YXZ convention.
         *
         * Extracts Euler angles from an orthonormal rotation matrix with YXZ rotation order.
         * The rotations are applied in order: Y (yaw), then X (pitch), then Z (roll).
         *
         * @param[out] rfYAngle The yaw angle in radians (rotation about Y-axis).
         * @param[out] rfPAngle The pitch angle in radians (rotation about X-axis).
         * @param[out] rfRAngle The roll angle in radians (rotation about Z-axis).
         * @return true if the extraction succeeds, false if the matrix is not orthonormal.
         *
         * @warning The matrix must be orthonormal (a pure rotation matrix) for accurate results.
         */
        bool toEulerAnglesYXZ( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const;

        /**
         * @brief Converts the rotation matrix to Euler angles using YZX convention.
         *
         * Extracts Euler angles from an orthonormal rotation matrix with YZX rotation order.
         * The rotations are applied in order: Y (yaw), then Z (roll), then X (pitch).
         *
         * @param[out] rfYAngle The yaw angle in radians (rotation about Y-axis).
         * @param[out] rfPAngle The pitch angle in radians (rotation about Z-axis).
         * @param[out] rfRAngle The roll angle in radians (rotation about X-axis).
         * @return true if the extraction succeeds, false if the matrix is not orthonormal.
         *
         * @warning The matrix must be orthonormal (a pure rotation matrix) for accurate results.
         */
        bool toEulerAnglesYZX( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const;

        /**
         * @brief Converts the rotation matrix to Euler angles using ZXY convention.
         *
         * Extracts Euler angles from an orthonormal rotation matrix with ZXY rotation order.
         * The rotations are applied in order: Z (roll), then X (pitch), then Y (yaw).
         *
         * @param[out] rfYAngle The yaw angle in radians (rotation about Y-axis).
         * @param[out] rfPAngle The pitch angle in radians (rotation about X-axis).
         * @param[out] rfRAngle The roll angle in radians (rotation about Z-axis).
         * @return true if the extraction succeeds, false if the matrix is not orthonormal.
         *
         * @warning The matrix must be orthonormal (a pure rotation matrix) for accurate results.
         */
        bool toEulerAnglesZXY( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const;

        /**
         * @brief Converts the rotation matrix to Euler angles using ZYX convention.
         *
         * Extracts Euler angles from an orthonormal rotation matrix with ZYX rotation order.
         * The rotations are applied in order: Z (roll), then Y (yaw), then X (pitch).
         * This is a common convention also known as "Tait-Bryan angles" or "aerospace sequence".
         *
         * @param[out] rfYAngle The yaw angle in radians (rotation about Y-axis/heading).
         * @param[out] rfPAngle The pitch angle in radians (rotation about X-axis/attitude).
         * @param[out] rfRAngle The roll angle in radians (rotation about Z-axis/bank).
         * @return true if the extraction succeeds, false if the matrix is not orthonormal.
         *
         * @warning The matrix must be orthonormal (a pure rotation matrix) for accurate results.
         */
        bool toEulerAnglesZYX( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const;

        /**
         * @brief Sets the rotation matrix from Euler angles using XYZ convention.
         *
         * Constructs a rotation matrix from Euler angles with XYZ rotation order.
         * The rotations are applied in order: X (pitch), then Y (yaw), then Z (roll).
         *
         * @param fYAngle The yaw angle in radians (rotation about Y-axis).
         * @param fPAngle The pitch angle in radians (rotation about X-axis).
         * @param fRAngle The roll angle in radians (rotation about Z-axis).
         */
        void fromEulerAnglesXYZ( const T &fYAngle, const T &fPAngle, const T &fRAngle );

        /**
         * @brief Sets the rotation matrix from Euler angles using XZY convention.
         *
         * Constructs a rotation matrix from Euler angles with XZY rotation order.
         * The rotations are applied in order: X (pitch), then Z (roll), then Y (yaw).
         *
         * @param fYAngle The yaw angle in radians (rotation about Y-axis).
         * @param fPAngle The pitch angle in radians (rotation about X-axis).
         * @param fRAngle The roll angle in radians (rotation about Z-axis).
         */
        void fromEulerAnglesXZY( const T &fYAngle, const T &fPAngle, const T &fRAngle );

        /**
         * @brief Sets the rotation matrix from Euler angles using YXZ convention.
         *
         * Constructs a rotation matrix from Euler angles with YXZ rotation order.
         * The rotations are applied in order: Y (yaw), then X (pitch), then Z (roll).
         *
         * @param fYAngle The yaw angle in radians (rotation about Y-axis).
         * @param fPAngle The pitch angle in radians (rotation about X-axis).
         * @param fRAngle The roll angle in radians (rotation about Z-axis).
         */
        void fromEulerAnglesYXZ( const T &fYAngle, const T &fPAngle, const T &fRAngle );

        /**
         * @brief Sets the rotation matrix from Euler angles using YZX convention.
         *
         * Constructs a rotation matrix from Euler angles with YZX rotation order.
         * The rotations are applied in order: Y (yaw), then Z (roll), then X (pitch).
         *
         * @param fYAngle The yaw angle in radians (rotation about Y-axis).
         * @param fPAngle The pitch angle in radians (rotation about Z-axis).
         * @param fRAngle The roll angle in radians (rotation about X-axis).
         */
        void fromEulerAnglesYZX( const T &fYAngle, const T &fPAngle, const T &fRAngle );

        /**
         * @brief Sets the rotation matrix from Euler angles using ZXY convention.
         *
         * Constructs a rotation matrix from Euler angles with ZXY rotation order.
         * The rotations are applied in order: Z (roll), then X (pitch), then Y (yaw).
         *
         * @param fYAngle The yaw angle in radians (rotation about Y-axis).
         * @param fPAngle The pitch angle in radians (rotation about X-axis).
         * @param fRAngle The roll angle in radians (rotation about Z-axis).
         */
        void fromEulerAnglesZXY( const T &fYAngle, const T &fPAngle, const T &fRAngle );

        /**
         * @brief Sets the rotation matrix from Euler angles using ZYX convention.
         *
         * Constructs a rotation matrix from Euler angles with ZYX rotation order.
         * The rotations are applied in order: Z (roll), then Y (yaw), then X (pitch).
         * This is a common convention also known as "Tait-Bryan angles" or "aerospace sequence".
         *
         * @param fYAngle The yaw angle in radians (rotation about Y-axis/heading).
         * @param fPAngle The pitch angle in radians (rotation about X-axis/attitude).
         * @param fRAngle The roll angle in radians (rotation about Z-axis/bank).
         */
        void fromEulerAnglesZYX( const T &fYAngle, const T &fPAngle, const T &fRAngle );

        /**
         * @brief Performs QDU decomposition of the matrix.
         *
         * Decomposes this matrix into Q*D*U where:
         * - Q is an orthogonal matrix (rotation)
         * - D is a diagonal matrix (scale)
         * - U is an upper triangular matrix (shear)
         *
         * This decomposition is useful for extracting rotation, scale, and shear components
         * from a transformation matrix.
         *
         * @param[out] kQ The orthogonal (rotation) component matrix.
         * @param[out] kD The diagonal (scale) component as a 3D vector.
         * @param[out] kU The upper triangular (shear) component as a 3D vector.
         *
         * @note This is also known as polar decomposition with additional scale extraction.
         */
        void QDUDecomposition( Matrix3<T> &kQ, Vector3<T> &kD, Vector3<T> &kU ) const;

        /**
         * @brief Computes the transpose of the matrix.
         *
         * Returns a new matrix where rows and columns are swapped.
         * For a matrix M, the transpose M^T has elements: M^T[i][j] = M[j][i]
         *
         * @return The transposed matrix.
         *
         * @note For orthonormal rotation matrices, the transpose equals the inverse.
         */
        Matrix3<T> transpose() const;

        /**
         * @brief Returns the identity matrix.
         *
         * Creates and returns a 3x3 identity matrix where diagonal elements are 1
         * and all other elements are 0.
         *
         * @return A static identity matrix.
         *
         * @note This returns a reference to a static instance, so it's efficient to call repeatedly.
         */
        static Matrix3 identity();

        /**
         * @brief Union providing two ways to access matrix data.
         *
         * - m[3][3]: Access elements as a 2D array (row-major order)
         * - m_[9]: Access elements as a flat 1D array
         */
        union
        {
            T m[3][3];  ///< 2D array representation: m[row][column]
            T m_[9];    ///< 1D array representation: row-major order
        };
    };

    template <class T>
    Vector3<T> operator*( const Vector3<T> &vector, const Matrix3<T> &matrix )
    {
        Vector3<T> product;
        for( u32 row = 0; row < 3; ++row )
        {
            product[row] = vector[0] * matrix.m[0][row] + vector[1] * matrix.m[1][row] +
                           vector[2] * matrix.m[2][row];
        }

        return product;
    }

    template <class T>
    Matrix3<T> operator*( T scalar, const Matrix3<T> &matrix )
    {
        Matrix3<T> product;
        for( u32 row = 0; row < 3; ++row )
        {
            for( u32 column = 0; column < 3; ++column )
            {
                product.m[row][column] = scalar * matrix.m[row][column];
            }
        }

        return product;
    }

    /**
     * @typedef Matrix3F
     * @brief Single-precision floating-point 3x3 matrix.
     *
     * Convenient alias for Matrix3<float> commonly used in graphics applications.
     */
    using Matrix3F = Matrix3<f32>;

    /**
     * @typedef Matrix3D
     * @brief Double-precision floating-point 3x3 matrix.
     *
     * Convenient alias for Matrix3<double> for applications requiring higher precision.
     */
    using Matrix3D = Matrix3<f64>;

}  // namespace workphone

#endif
