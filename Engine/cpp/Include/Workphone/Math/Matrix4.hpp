#ifndef Matrix4_h__
#define Matrix4_h__

#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Plane3.hpp>

#include <cstring>

namespace workphone
{

    /**
     * @brief A 4x4 matrix class for transformations and projections in 3D space.
     *
     * This class provides functionality for 3D transformations and projections.
     * The matrix is stored in row-major order in a union with an array of size 16.
     * The data can be accessed both as a two-dimensional array of elements or a one-dimensional array.
     * The type of the elements in the matrix is specified by the template parameter T.
     *
     * @tparam T The type of the elements in the matrix.
     */
    template <typename T>
    class WPCore_API Matrix4
    {
    public:
        /**
         * @brief Default constructor. Initializes a new 4x4 matrix as an identity matrix.
         */
        Matrix4();

        /**
         * @brief Copy constructor. Initializes a new 4x4 matrix as a copy of another matrix.
         * @param other The other matrix to copy.
         */
        Matrix4( const Matrix4<T> &other );

        /**
         * @brief Parameterized constructor. Initializes a new 4x4 matrix with the specified values.
         * @param m00 The value at row 0, column 0.
         * @param m01 The value at row 0, column 1.
         * @param m02 The value at row 0, column 2.
         * @param m03 The value at row 0, column 3.
         * @param m10 The value at row 1, column 0.
         * @param m11 The value at row 1, column 1.
         * @param m12 The value at row 1, column 2.
         * @param m13 The value at row 1, column 3.
         * @param m20 The value at row 2, column 0.
         * @param m21 The value at row 2, column 1.
         * @param m22 The value at row 2, column 2.
         * @param m23 The value at row 2, column 3.
         * @param m30 The value at row 3, column 0.
         * @param m31 The value at row 3, column 1.
         * @param m32 The value at row 3, column 2.
         * @param m33 The value at row 3, column 3.
         */
        Matrix4( T m00, T m01, T m02, T m03, T m10, T m11, T m12, T m13, T m20, T m21, T m22, T m23,
                 T m30, T m31, T m32, T m33 );

        /**
         * @brief Construct a matrix from a pointer to an array of 16 elements in row-major order.
         * @param p The pointer to the array of 16 elements.
         */
        explicit Matrix4( const T *p );

        /**
         * @brief Destructor.
         */
        ~Matrix4();

        /**
         * @brief Returns a pointer to the matrix data.
         *
         * @return T* A pointer to the matrix data.
         */
        T *ptr();

        /**
         * @brief Returns a const pointer to the matrix data.
         *
         * @return const T* A const pointer to the matrix data.
         */
        const T *ptr() const;

        /**
         * @brief Returns a pointer to the specified row of the matrix.
         *
         * @param iRow The row index.
         * @return T* A pointer to the row data.
         */
        T *operator[]( u32 iRow );

        /**
         * @brief Returns a const pointer to the specified row of the matrix.
         *
         * @param iRow The row index.
         * @return const T* A const pointer to the row data.
         */
        const T *operator[]( u32 iRow ) const;

        /** Check whether or not the matrix is affine matrix.
        @remarks
            An affine matrix is a 4x4 matrix with row 3 equal to (0, 0, 0, 1),
            e.g. no projective coefficients.
        */
        bool isAffine() const;

        /** 3-D Vector transformation specially for an affine matrix.
        @remarks
            Transforms the given 3-D vector by the matrix, projecting the
            result back into <i>w</i> = 1.
        @note
            The matrix must be an affine matrix. @see Matrix4::isAffine.
        */
        Vector3<T> transformAffine( const Vector3<T> &v ) const;

        /** 4-D Vector transformation specially for an affine matrix.
        @note
            The matrix must be an affine matrix. @see Matrix4::isAffine.
        */
        Vector4<T> transformAffine( const Vector4<T> &v ) const;

        /**
         * @brief Concatenates this matrix with another matrix and returns the result.
         *
         * @param m2 The other matrix.
         * @return Matrix4<T> The concatenated matrix.
         */
        Matrix4 concatenate( const Matrix4 &m2 ) const;

        /**
         * @brief Concatenates this matrix with another affine matrix and returns the result.
         *
         * @param m2 The other affine matrix.
         * @return Matrix4<T> The concatenated matrix.
         */
        Matrix4 concatenateAffine( const Matrix4 &m2 ) const;

        /**
         * @brief Multiplies this matrix by another matrix using the '*' operator.
         *
         * @param m2 The other matrix.
         * @return Matrix4<T> The result of the matrix multiplication.
         */
        Matrix4 operator*( const Matrix4 &m2 ) const;

        /**
         * @brief Transforms a 3-D vector by the matrix and returns the result.
         *
         * @param v The 3-D vector to transform.
         * @return Vector3<T> The transformed vector.
         *
         * @remarks
         * Transforms the given 3-D vector by the matrix, projecting the result back into <i>w</i> = 1.
         * This means that the initial <i>w</i> is considered to be 1.0,
         * and then all the three elements of the resulting 3-D vector are divided by the resulting
         * <i>w</i>.
         */
        Vector3<T> operator*( const Vector3<T> &v ) const;

        /**
         * @brief Assigns the values of a Matrix3 to the Matrix4.
         *
         * @param mat3 The Matrix3 to assign.
         */
        void operator=( const Matrix3<T> &mat3 );

        /**
         * @brief Sets the matrix to represent a transformation consisting of a translation, a scaling,
         * and a rotation.
         *
         * @param position The translation vector.
         * @param scale The scaling vector.
         * @param orientation The rotation quaternion.
         */
        void makeTransform( const Vector3<T> &position, const Vector3<T> &scale,
                            const Quaternion<T> &orientation );

        /**
         * @brief Decomposes the matrix into its translation, scaling, and rotation components.
         *
         * @param position The output translation vector.
         * @param scale The output scaling vector.
         * @param orientation The output rotation quaternion.
         */
        void decomposition( Vector3<T> &position, Vector3<T> &scale, Quaternion<T> &orientation ) const;

        /** Extract the 3x3 matrix from this matrix.
            @param m3x3 The output 3x3 matrix.
        */
        void extract3x3Matrix( Matrix3<T> &m3x3 ) const;

        /** Set the matrix to be a view matrix.
            @param position The position of the camera.
            @param orientation The orientation of the camera.
            @param reflectMatrix The optional reflection matrix.
        */
        void makeViewMatrix( const Vector3<T> &position, const Quaternion<T> &orientation,
                             const Matrix4<T> *reflectMatrix );

        /**
         * @brief Set the matrix to be a perspective projection matrix.
         *
         * @param fovY The field of view angle in degrees in the y direction.
         * @param aspect The aspect ratio (width/height) of the viewport.
         * @param nearPlane The near clipping plane distance.
         * @param farPlane The far clipping plane distance.
         */
        void makePerspective( T fovY, T aspect, T nearPlane, T farPlane );

        /**
         * @brief Get the transpose of this matrix.
         *
         * @return The transpose of this matrix.
         */
        Matrix4<T> transpose() const;

        /**
         * @brief Get the inverse of this matrix.
         *
         * @return The inverse of this matrix.
         */
        Matrix4<T> inverse() const;

        /**
         * @brief Extract the 3x3 rotation matrix from this matrix.
         *
         * @param rotMat The output 3x3 rotation matrix.
         */
        void getRotationMat( Matrix3<T> &rotMat ) const;

        /**
         * @brief Get the translation component of this matrix.
         * @return The translation component of this matrix.
         */
        Vector3<T> getTranslation() const;

        /**
         * @brief Get the scaling component of this matrix.
         * @return The scaling component of this matrix.
         */
        Vector4<T> getRow( u32 row ) const;

        /**
         * @brief Get a matrix with all elements set to 0.
         *
         * @return The matrix with all elements set to 0.
         */
        static Matrix4 zero();

        /**
         * @brief Get an affine matrix with all elements set to 0.
         *
         * @return The affine matrix with all elements set to 0.
         */
        static Matrix4 zeroaffine();

        /**
         * @brief Get an identity matrix.
         * @return The identity matrix.
         */
        static Matrix4 identity();

        /**
         * @brief A union containing the matrix elements.
         */
        union
        {
            T m[4][4];
            T m_[16];
        };
    };

    /**
     * Overloaded multiplication operator that multiplies a Vector4 by a Matrix4 and returns the result
     * as a Vector4.
     *
     * @param v The Vector4 to multiply by the Matrix4.
     * @param mat The Matrix4 to multiply with the Vector4.
     * @return The result of the multiplication as a new Vector4.
     */
    template <typename T>
    Vector4<T> operator*( const Vector4<T> &v, const Matrix4<T> &mat )
    {
        return Vector4<T>( v.x * mat[0][0] + v.y * mat[1][0] + v.z * mat[2][0] + v.w * mat[3][0],
                           v.x * mat[0][1] + v.y * mat[1][1] + v.z * mat[2][1] + v.w * mat[3][1],
                           v.x * mat[0][2] + v.y * mat[1][2] + v.z * mat[2][2] + v.w * mat[3][2],
                           v.x * mat[0][3] + v.y * mat[1][3] + v.z * mat[2][3] + v.w * mat[3][3] );
    }

    /**
     * Type alias for Matrix4 with f32 type.
     */
    using Matrix4F = Matrix4<f32>;

    /**
     * Type alias for Matrix4 with f64 type.
     */
    using Matrix4D = Matrix4<f64>;

}  // namespace workphone

#endif  // Matrix4_h__
