#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Matrix4.hpp>

namespace workphone
{
    template <typename T>
    Matrix4<T>::~Matrix4() = default;

    template <typename T>
    void Matrix4<T>::makeTransform( const Vector3<T> &position, const Vector3<T> &scale,
                                    const Quaternion<T> &orientation )
    {
        // Ordering:
        //    1. Scale
        //    2. Rotate
        //    3. Translate

        Matrix3<T> rot3x3 = Matrix3<T>::identity();
        orientation.toRotationMatrix( rot3x3 );

        // Set up final matrix with scale, rotation and translation
        m[0][0] = scale[0] * rot3x3[0][0];
        m[0][1] = scale[1] * rot3x3[0][1];
        m[0][2] = scale[2] * rot3x3[0][2];
        m[0][3] = position[0];
        m[1][0] = scale[0] * rot3x3[1][0];
        m[1][1] = scale[1] * rot3x3[1][1];
        m[1][2] = scale[2] * rot3x3[1][2];
        m[1][3] = position[1];
        m[2][0] = scale[0] * rot3x3[2][0];
        m[2][1] = scale[1] * rot3x3[2][1];
        m[2][2] = scale[2] * rot3x3[2][2];
        m[2][3] = position[2];

        // No projection term
        m[3][0] = 0;
        m[3][1] = 0;
        m[3][2] = 0;
        m[3][3] = 1;
    }

    template <typename T>
    void Matrix4<T>::decomposition( Vector3<T> &position, Vector3<T> &scale,
                                    Quaternion<T> &orientation ) const
    {
        // WP_ASSERT(isAffine());

        Matrix3<T> m3x3;
        extract3x3Matrix( m3x3 );

        Matrix3<T> matQ;
        Vector3<T> vecU;
        m3x3.QDUDecomposition( matQ, scale, vecU );

        orientation = Quaternion( matQ );
        position = Vector3( m[0][3], m[1][3], m[2][3] );
    }

    template <typename T>
    void Matrix4<T>::extract3x3Matrix( Matrix3<T> &m3x3 ) const
    {
        m3x3.m[0][0] = m[0][0];
        m3x3.m[0][1] = m[0][1];
        m3x3.m[0][2] = m[0][2];
        m3x3.m[1][0] = m[1][0];
        m3x3.m[1][1] = m[1][1];
        m3x3.m[1][2] = m[1][2];
        m3x3.m[2][0] = m[2][0];
        m3x3.m[2][1] = m[2][1];
        m3x3.m[2][2] = m[2][2];
    }

    template <typename T>
    auto Matrix4<T>::transpose() const -> Matrix4<T>
    {
        return Matrix4<T>( m[0][0], m[1][0], m[2][0], m[3][0], m[0][1], m[1][1], m[2][1], m[3][1],
                           m[0][2], m[1][2], m[2][2], m[3][2], m[0][3], m[1][3], m[2][3], m[3][3] );
    }

    template <typename T>
    void Matrix4<T>::makeViewMatrix( const Vector3<T> &position, const Quaternion<T> &orientation,
                                     const Matrix4<T> *reflectMatrix )
    {
        Matrix4<T> viewMatrix;

        // View matrix is:
        //
        //  [ Lx  Uy  Dz  Tx  ]
        //  [ Lx  Uy  Dz  Ty  ]
        //  [ Lx  Uy  Dz  Tz  ]
        //  [ 0   0   0   1   ]
        //
        // Where T = -(Transposed(Rot) * Pos)

        // This is most efficiently done using 3x3 Matrices
        Matrix3<T> rot;
        orientation.toRotationMatrix( rot );

        // Make the translation relative to new axes
        Matrix3<T> rotT = rot.transpose();
        Matrix3<T> rotT2 = -rotT;
        Vector3<T> trans = rotT2 * position;

        // Make final matrix
        viewMatrix = Matrix4<T>::identity();
        viewMatrix = rotT;  // fills upper 3x3
        viewMatrix[0][3] = trans[0];
        viewMatrix[1][3] = trans[1];
        viewMatrix[2][3] = trans[2];

        // Deal with reflections
        if( reflectMatrix )
        {
            viewMatrix = viewMatrix * ( *reflectMatrix );
        }

        *this = viewMatrix;
    }

    template <typename T>
    void Matrix4<T>::operator=( const Matrix3<T> &mat3 )
    {
        m[0][0] = mat3.m[0][0];
        m[0][1] = mat3.m[0][1];
        m[0][2] = mat3.m[0][2];
        m[1][0] = mat3.m[1][0];
        m[1][1] = mat3.m[1][1];
        m[1][2] = mat3.m[1][2];
        m[2][0] = mat3.m[2][0];
        m[2][1] = mat3.m[2][1];
        m[2][2] = mat3.m[2][2];
    }

    template <typename T>
    auto Matrix4<T>::inverse() const -> Matrix4<T>
    {
        T m00 = m[0][0], m01 = m[0][1], m02 = m[0][2], m03 = m[0][3];
        T m10 = m[1][0], m11 = m[1][1], m12 = m[1][2], m13 = m[1][3];
        T m20 = m[2][0], m21 = m[2][1], m22 = m[2][2], m23 = m[2][3];
        T m30 = m[3][0], m31 = m[3][1], m32 = m[3][2], m33 = m[3][3];

        T v0 = m20 * m31 - m21 * m30;
        T v1 = m20 * m32 - m22 * m30;
        T v2 = m20 * m33 - m23 * m30;
        T v3 = m21 * m32 - m22 * m31;
        T v4 = m21 * m33 - m23 * m31;
        T v5 = m22 * m33 - m23 * m32;

        T t00 = +( v5 * m11 - v4 * m12 + v3 * m13 );
        T t10 = -( v5 * m10 - v2 * m12 + v1 * m13 );
        T t20 = +( v4 * m10 - v2 * m11 + v0 * m13 );
        T t30 = -( v3 * m10 - v1 * m11 + v0 * m12 );

        T invDet = T( 1.0 ) / ( t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03 );

        T d00 = t00 * invDet;
        T d10 = t10 * invDet;
        T d20 = t20 * invDet;
        T d30 = t30 * invDet;

        T d01 = -( v5 * m01 - v4 * m02 + v3 * m03 ) * invDet;
        T d11 = +( v5 * m00 - v2 * m02 + v1 * m03 ) * invDet;
        T d21 = -( v4 * m00 - v2 * m01 + v0 * m03 ) * invDet;
        T d31 = +( v3 * m00 - v1 * m01 + v0 * m02 ) * invDet;

        v0 = m10 * m31 - m11 * m30;
        v1 = m10 * m32 - m12 * m30;
        v2 = m10 * m33 - m13 * m30;
        v3 = m11 * m32 - m12 * m31;
        v4 = m11 * m33 - m13 * m31;
        v5 = m12 * m33 - m13 * m32;

        T d02 = +( v5 * m01 - v4 * m02 + v3 * m03 ) * invDet;
        T d12 = -( v5 * m00 - v2 * m02 + v1 * m03 ) * invDet;
        T d22 = +( v4 * m00 - v2 * m01 + v0 * m03 ) * invDet;
        T d32 = -( v3 * m00 - v1 * m01 + v0 * m02 ) * invDet;

        v0 = m21 * m10 - m20 * m11;
        v1 = m22 * m10 - m20 * m12;
        v2 = m23 * m10 - m20 * m13;
        v3 = m22 * m11 - m21 * m12;
        v4 = m23 * m11 - m21 * m13;
        v5 = m23 * m12 - m22 * m13;

        T d03 = -( v5 * m01 - v4 * m02 + v3 * m03 ) * invDet;
        T d13 = +( v5 * m00 - v2 * m02 + v1 * m03 ) * invDet;
        T d23 = -( v4 * m00 - v2 * m01 + v0 * m03 ) * invDet;
        T d33 = +( v3 * m00 - v1 * m01 + v0 * m02 ) * invDet;

        return Matrix4<T>( d00, d01, d02, d03, d10, d11, d12, d13, d20, d21, d22, d23, d30, d31, d32,
                           d33 );
    }

    template <typename T>
    auto Matrix4<T>::concatenate( const Matrix4 &m2 ) const -> Matrix4<T>
    {
        Matrix4 r;
        r.m[0][0] =
            m[0][0] * m2.m[0][0] + m[0][1] * m2.m[1][0] + m[0][2] * m2.m[2][0] + m[0][3] * m2.m[3][0];
        r.m[0][1] =
            m[0][0] * m2.m[0][1] + m[0][1] * m2.m[1][1] + m[0][2] * m2.m[2][1] + m[0][3] * m2.m[3][1];
        r.m[0][2] =
            m[0][0] * m2.m[0][2] + m[0][1] * m2.m[1][2] + m[0][2] * m2.m[2][2] + m[0][3] * m2.m[3][2];
        r.m[0][3] =
            m[0][0] * m2.m[0][3] + m[0][1] * m2.m[1][3] + m[0][2] * m2.m[2][3] + m[0][3] * m2.m[3][3];

        r.m[1][0] =
            m[1][0] * m2.m[0][0] + m[1][1] * m2.m[1][0] + m[1][2] * m2.m[2][0] + m[1][3] * m2.m[3][0];
        r.m[1][1] =
            m[1][0] * m2.m[0][1] + m[1][1] * m2.m[1][1] + m[1][2] * m2.m[2][1] + m[1][3] * m2.m[3][1];
        r.m[1][2] =
            m[1][0] * m2.m[0][2] + m[1][1] * m2.m[1][2] + m[1][2] * m2.m[2][2] + m[1][3] * m2.m[3][2];
        r.m[1][3] =
            m[1][0] * m2.m[0][3] + m[1][1] * m2.m[1][3] + m[1][2] * m2.m[2][3] + m[1][3] * m2.m[3][3];

        r.m[2][0] =
            m[2][0] * m2.m[0][0] + m[2][1] * m2.m[1][0] + m[2][2] * m2.m[2][0] + m[2][3] * m2.m[3][0];
        r.m[2][1] =
            m[2][0] * m2.m[0][1] + m[2][1] * m2.m[1][1] + m[2][2] * m2.m[2][1] + m[2][3] * m2.m[3][1];
        r.m[2][2] =
            m[2][0] * m2.m[0][2] + m[2][1] * m2.m[1][2] + m[2][2] * m2.m[2][2] + m[2][3] * m2.m[3][2];
        r.m[2][3] =
            m[2][0] * m2.m[0][3] + m[2][1] * m2.m[1][3] + m[2][2] * m2.m[2][3] + m[2][3] * m2.m[3][3];

        r.m[3][0] =
            m[3][0] * m2.m[0][0] + m[3][1] * m2.m[1][0] + m[3][2] * m2.m[2][0] + m[3][3] * m2.m[3][0];
        r.m[3][1] =
            m[3][0] * m2.m[0][1] + m[3][1] * m2.m[1][1] + m[3][2] * m2.m[2][1] + m[3][3] * m2.m[3][1];
        r.m[3][2] =
            m[3][0] * m2.m[0][2] + m[3][1] * m2.m[1][2] + m[3][2] * m2.m[2][2] + m[3][3] * m2.m[3][2];
        r.m[3][3] =
            m[3][0] * m2.m[0][3] + m[3][1] * m2.m[1][3] + m[3][2] * m2.m[2][3] + m[3][3] * m2.m[3][3];

        return r;
    }

    template <typename T>
    auto Matrix4<T>::concatenateAffine( const Matrix4<T> &m2 ) const -> Matrix4<T>
    {
        // assert(isAffine() && m2.isAffine());

        return Matrix4( m[0][0] * m2.m[0][0] + m[0][1] * m2.m[1][0] + m[0][2] * m2.m[2][0],
                        m[0][0] * m2.m[0][1] + m[0][1] * m2.m[1][1] + m[0][2] * m2.m[2][1],
                        m[0][0] * m2.m[0][2] + m[0][1] * m2.m[1][2] + m[0][2] * m2.m[2][2],
                        m[0][0] * m2.m[0][3] + m[0][1] * m2.m[1][3] + m[0][2] * m2.m[2][3] + m[0][3],

                        m[1][0] * m2.m[0][0] + m[1][1] * m2.m[1][0] + m[1][2] * m2.m[2][0],
                        m[1][0] * m2.m[0][1] + m[1][1] * m2.m[1][1] + m[1][2] * m2.m[2][1],
                        m[1][0] * m2.m[0][2] + m[1][1] * m2.m[1][2] + m[1][2] * m2.m[2][2],
                        m[1][0] * m2.m[0][3] + m[1][1] * m2.m[1][3] + m[1][2] * m2.m[2][3] + m[1][3],

                        m[2][0] * m2.m[0][0] + m[2][1] * m2.m[1][0] + m[2][2] * m2.m[2][0],
                        m[2][0] * m2.m[0][1] + m[2][1] * m2.m[1][1] + m[2][2] * m2.m[2][1],
                        m[2][0] * m2.m[0][2] + m[2][1] * m2.m[1][2] + m[2][2] * m2.m[2][2],
                        m[2][0] * m2.m[0][3] + m[2][1] * m2.m[1][3] + m[2][2] * m2.m[2][3] + m[2][3],

                        0, 0, 0, 1 );
    }

    template <typename T>
    void Matrix4<T>::makePerspective( T fovY, T aspect, T nearPlane, T farPlane )
    {
        const T INFINITE_FAR_PLANE_ADJUST = T( 0.0001 );

        T thetaY( fovY / T( 2.0 ) );
        T tanThetaY = Math<T>::Tan( thetaY );

        // Calc matrix elements
        T w = ( T( 1.0 ) / tanThetaY ) / aspect;
        T h = T( 1.0 ) / tanThetaY;
        T q, qn;
        if( farPlane == 0 )
        {
            // Infinite far plane
            q = INFINITE_FAR_PLANE_ADJUST - T( 1.0 );
            qn = nearPlane * ( INFINITE_FAR_PLANE_ADJUST - T( 2.0 ) );
        }
        else
        {
            q = -( farPlane + nearPlane ) / ( farPlane - nearPlane );
            qn = T( -2.0 ) * ( farPlane * nearPlane ) / ( farPlane - nearPlane );
        }

        // NB This creates Z in range [-1,1]
        //
        // [ w   0   0   0  ]
        // [ 0   h   0   0  ]
        // [ 0   0   q   qn ]
        // [ 0   0   -1  0  ]

        *this = Matrix4<T>::zero();
        m[0][0] = w;
        m[1][1] = h;
        m[2][2] = q;
        m[2][3] = qn;
        m[3][2] = -1;
    }

    template <class T>
    void Matrix4<T>::getRotationMat( Matrix3<T> &rotMat ) const
    {
        rotMat.m_[0] = m_[0];
        rotMat.m_[1] = m_[1];
        rotMat.m_[2] = m_[2];
        rotMat.m_[3] = m_[4];
        rotMat.m_[4] = m_[5];
        rotMat.m_[5] = m_[6];
        rotMat.m_[6] = m_[8];
        rotMat.m_[7] = m_[9];
        rotMat.m_[8] = m_[10];
    }

    template <typename T>
    auto Matrix4<T>::getTranslation() const -> Vector3<T>
    {
        return Vector3<T>( m_[12], m_[13], m_[14] );
    }

    template <typename T>
    workphone::Matrix4<T> Matrix4<T>::identity()
    {
        return Matrix4<T>( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );
    }

    template <typename T>
    workphone::Matrix4<T> Matrix4<T>::zeroaffine()
    {
        return Matrix4<T>( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 );
    }

    template <typename T>
    workphone::Matrix4<T> Matrix4<T>::zero()
    {
        return Matrix4<T>( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
    }

    template <typename T>
    workphone::Vector4<T> Matrix4<T>::getRow( u32 row ) const
    {
        return Vector4<T>( m[row][0], m[row][1], m[row][2], m[row][3] );
    }

    template <typename T>
    workphone::Vector3<T> Matrix4<T>::operator*( const Vector3<T> &v ) const
    {
        Vector3<T> r;

        T fInvW = T( 1.0 ) / ( m[3][0] * v[0] + m[3][1] * v[1] + m[3][2] * v[2] + m[3][3] );
        r[0] = ( m[0][0] * v[0] + m[0][1] * v[0] + m[0][2] * v[0] + m[0][3] ) * fInvW;
        r[1] = ( m[1][0] * v[1] + m[1][1] * v[1] + m[1][2] * v[1] + m[1][3] ) * fInvW;
        r[2] = ( m[2][0] * v[2] + m[2][1] * v[2] + m[2][2] * v[2] + m[2][3] ) * fInvW;

        return r;
    }

    template <typename T>
    workphone::Matrix4<T> Matrix4<T>::operator*( const Matrix4<T> &m2 ) const
    {
        return concatenate( m2 );
    }

    template <typename T>
    workphone::Vector4<T> Matrix4<T>::transformAffine( const Vector4<T> &v ) const
    {
        WP_ASSERT( isAffine() );

        return Vector4<T>( m[0][0] * v.X() + m[0][1] * v.Y() + m[0][2] * v.Z() + m[0][3] * v.W(),
                           m[1][0] * v.X() + m[1][1] * v.Y() + m[1][2] * v.Z() + m[1][3] * v.W(),
                           m[2][0] * v.X() + m[2][1] * v.Y() + m[2][2] * v.Z() + m[2][3] * v.W(),
                           v.W() );
    }

    template <typename T>
    workphone::Vector3<T> Matrix4<T>::transformAffine( const Vector3<T> &v ) const
    {
        WP_ASSERT( isAffine() );

        return Vector3<T>( m[0][0] * v.X() + m[0][1] * v.Y() + m[0][2] * v.Z() + m[0][3],
                           m[1][0] * v.X() + m[1][1] * v.Y() + m[1][2] * v.Z() + m[1][3],
                           m[2][0] * v.X() + m[2][1] * v.Y() + m[2][2] * v.Z() + m[2][3] );
    }

    template <typename T>
    bool Matrix4<T>::isAffine() const
    {
        return m[3][0] == 0 && m[3][1] == 0 && m[3][2] == 0 && m[3][3] == 1;
    }

    template <typename T>
    const T *Matrix4<T>::operator[]( u32 iRow ) const
    {
        WP_ASSERT( iRow < 4 );
        return m[iRow];
    }

    template <typename T>
    T *Matrix4<T>::operator[]( u32 iRow )
    {
        WP_ASSERT( iRow < 4 );
        return m[iRow];
    }

    template <typename T>
    const T *Matrix4<T>::ptr() const
    {
        return &m_[0];
    }

    template <typename T>
    T *Matrix4<T>::ptr()
    {
        return &m_[0];
    }

    template <typename T>
    Matrix4<T>::Matrix4( const T *p )
    {
        std::memcpy( m, p, 16 * sizeof( T ) );
    }

    template <typename T>
    Matrix4<T>::Matrix4( T m00, T m01, T m02, T m03, T m10, T m11, T m12, T m13, T m20, T m21, T m22,
                         T m23, T m30, T m31, T m32, T m33 )
    {
        m[0][0] = m00;
        m[0][1] = m01;
        m[0][2] = m02;
        m[0][3] = m03;
        m[1][0] = m10;
        m[1][1] = m11;
        m[1][2] = m12;
        m[1][3] = m13;
        m[2][0] = m20;
        m[2][1] = m21;
        m[2][2] = m22;
        m[2][3] = m23;
        m[3][0] = m30;
        m[3][1] = m31;
        m[3][2] = m32;
        m[3][3] = m33;
    }

    template <typename T>
    Matrix4<T>::Matrix4( const Matrix4<T> &other )
    {
        std::memcpy( m_, other.m_, sizeof( T ) * 16 );
    }

    template <typename T>
    Matrix4<T>::Matrix4() = default;

    template class Matrix4<s32>;
    template class Matrix4<f32>;
    template class Matrix4<f64>;

}  // namespace workphone
