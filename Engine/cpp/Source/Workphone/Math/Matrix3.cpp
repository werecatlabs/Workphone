#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Matrix3.hpp>
#include <Workphone/Memory/Memory.hpp>

namespace workphone
{

    template <class T>
    Matrix3<T>::Matrix3( const Matrix3 &other )
    {
        Memory::Memcpy( m_, other.m_, sizeof( T ) * 9 );
    }

    template <class T>
    auto Matrix3<T>::operator==( const Matrix3<T> &rkMatrix ) const -> bool
    {
        for( u32 iRow = 0; iRow < 3; iRow++ )
        {
            for( u32 iCol = 0; iCol < 3; iCol++ )
            {
                if( m[iRow][iCol] != rkMatrix.m[iRow][iCol] )
                {
                    return false;
                }
            }
        }

        return true;
    }

    template <class T>
    auto Matrix3<T>::operator+( const Matrix3<T> &rkMatrix ) const -> Matrix3<T>
    {
        Matrix3<T> kSum;
        for( u32 iRow = 0; iRow < 3; iRow++ )
        {
            for( u32 iCol = 0; iCol < 3; iCol++ )
            {
                kSum.m[iRow][iCol] = m[iRow][iCol] + rkMatrix.m[iRow][iCol];
            }
        }

        return kSum;
    }

    template <class T>
    auto Matrix3<T>::operator-( const Matrix3<T> &rkMatrix ) const -> Matrix3<T>
    {
        Matrix3<T> kDiff;
        for( u32 iRow = 0; iRow < 3; iRow++ )
        {
            for( u32 iCol = 0; iCol < 3; iCol++ )
            {
                kDiff.m[iRow][iCol] = m[iRow][iCol] - rkMatrix.m[iRow][iCol];
            }
        }

        return kDiff;
    }

    template <class T>
    auto Matrix3<T>::operator*( const Matrix3<T> &rkMatrix ) const -> Matrix3<T>
    {
        Matrix3<T> kProd;
        for( u32 iRow = 0; iRow < 3; iRow++ )
        {
            for( u32 iCol = 0; iCol < 3; iCol++ )
            {
                kProd.m[iRow][iCol] = m[iRow][0] * rkMatrix.m[0][iCol] +
                                      m[iRow][1] * rkMatrix.m[1][iCol] +
                                      m[iRow][2] * rkMatrix.m[2][iCol];
            }
        }

        return kProd;
    }

    template <class T>
    auto Matrix3<T>::operator*( const Vector3<T> &rkPoint ) const -> Vector3<T>
    {
        Vector3<T> kProd;
        for( u32 iRow = 0; iRow < 3; iRow++ )
        {
            kProd[iRow] = m[iRow][0] * rkPoint[0] + m[iRow][1] * rkPoint[1] + m[iRow][2] * rkPoint[2];
        }

        return kProd;
    }

    template <class T>
    auto Matrix3<T>::operator-() const -> Matrix3<T>
    {
        Matrix3<T> kNeg;
        for( u32 iRow = 0; iRow < 3; iRow++ )
        {
            for( u32 iCol = 0; iCol < 3; iCol++ )
            {
                kNeg[iRow][iCol] = -m[iRow][iCol];
            }
        }

        return kNeg;
    }

    template <class T>
    auto Matrix3<T>::operator*( T fScalar ) const -> Matrix3<T>
    {
        Matrix3<T> kProd;
        for( u32 iRow = 0; iRow < 3; iRow++ )
        {
            for( u32 iCol = 0; iCol < 3; iCol++ )
            {
                kProd[iRow][iCol] = fScalar * m[iRow][iCol];
            }
        }
        return kProd;
    }

    template <class T>
    auto Matrix3<T>::toEulerAnglesXYZ( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const -> bool
    {
        // rot =  cy*cz          -cy*sz           sy
        //        cz*sx*sy+cx*sz  cx*cz-sx*sy*sz -cy*sx
        //       -cx*cz*sy+sx*sz  cz*sx+cx*sy*sz  cx*cy

        rfPAngle = Math<T>::ASin( m[0][2] );
        if( rfPAngle < static_cast<T>( Math<T>::half_pi() ) )
        {
            if( rfPAngle > static_cast<T>( -Math<T>::half_pi() ) )
            {
                rfYAngle = Math<T>::ATan2( -m[1][2], m[2][2] );
                rfRAngle = Math<T>::ATan2( -m[0][1], m[0][0] );
                return true;
            }
            // WARNING.  Not a unique solution.
            T fRmY = Math<T>::ATan2( m[1][0], m[1][1] );
            rfRAngle = T( 0.0 );  // any angle works
            rfYAngle = rfRAngle - fRmY;
            return false;
        }
        // WARNING.  Not a unique solution.
        T fRpY = Math<T>::ATan2( m[1][0], m[1][1] );
        rfRAngle = T( 0.0 );  // any angle works
        rfYAngle = fRpY - rfRAngle;
        return false;
    }

    template <class T>
    auto Matrix3<T>::toEulerAnglesXZY( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const -> bool
    {
        // rot =  cy*cz          -sz              cz*sy
        //        sx*sy+cx*cy*sz  cx*cz          -cy*sx+cx*sy*sz
        //       -cx*sy+cy*sx*sz  cz*sx           cx*cy+sx*sy*sz

        rfPAngle = Math<T>::ASin( -m[0][1] );
        if( rfPAngle < static_cast<T>( Math<T>::half_pi() ) )
        {
            if( rfPAngle > static_cast<T>( -Math<T>::half_pi() ) )
            {
                rfYAngle = Math<T>::ATan2( m[2][1], m[1][1] );
                rfRAngle = Math<T>::ATan2( m[0][2], m[0][0] );
                return true;
            }
            // WARNING.  Not a unique solution.
            T fRmY = Math<T>::ATan2( -m[2][0], m[2][2] );
            rfRAngle = T( 0.0 );  // any angle works
            rfYAngle = rfRAngle - fRmY;
            return false;
        }
        // WARNING.  Not a unique solution.
        T fRpY = Math<T>::ATan2( -m[2][0], m[2][2] );
        rfRAngle = T( 0.0 );  // any angle works
        rfYAngle = fRpY - rfRAngle;
        return false;
    }

    template <class T>
    auto Matrix3<T>::toEulerAnglesYXZ( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const -> bool
    {
        // rot =  cy*cz+sx*sy*sz  cz*sx*sy-cy*sz  cx*sy
        //        cx*sz           cx*cz          -sx
        //       -cz*sy+cy*sx*sz  cy*cz*sx+sy*sz  cx*cy

        rfPAngle = Math<T>::ASin( -m[1][2] );
        if( rfPAngle < Math<T>::half_pi() )
        {
            if( rfPAngle > static_cast<T>( -Math<T>::half_pi() ) )
            {
                rfYAngle = Math<T>::ATan2( m[0][2], m[2][2] );
                rfRAngle = Math<T>::ATan2( m[1][0], m[1][1] );
                return true;
            }

            // WARNING.  Not a unique solution.
            auto fRmY = Math<T>::ATan2( -m[0][1], m[0][0] );
            rfRAngle = T( 0.0 );  // any angle works
            rfYAngle = rfRAngle - fRmY;
            return false;
        }

        // WARNING.  Not a unique solution.
        auto fRpY = Math<T>::ATan2( -m[0][1], m[0][0] );
        rfRAngle = T( 0.0 );  // any angle works
        rfYAngle = fRpY - rfRAngle;
        return false;
    }

    template <class T>
    auto Matrix3<T>::toEulerAnglesYZX( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const -> bool
    {
        // rot =  cy*cz           sx*sy-cx*cy*sz  cx*sy+cy*sx*sz
        //        sz              cx*cz          -cz*sx
        //       -cz*sy           cy*sx+cx*sy*sz  cx*cy-sx*sy*sz

        rfPAngle = Math<T>::ASin( m[1][0] );
        if( rfPAngle < static_cast<T>( Math<T>::half_pi() ) )
        {
            if( rfPAngle > static_cast<T>( -Math<T>::half_pi() ) )
            {
                rfYAngle = Math<T>::ATan2( -m[2][0], m[0][0] );
                rfRAngle = Math<T>::ATan2( -m[1][2], m[1][1] );
                return true;
            }
            // WARNING.  Not a unique solution.
            T fRmY = Math<T>::ATan2( m[2][1], m[2][2] );
            rfRAngle = T( 0.0 );  // any angle works
            rfYAngle = rfRAngle - fRmY;
            return false;
        }
        // WARNING.  Not a unique solution.
        T fRpY = Math<T>::ATan2( m[2][1], m[2][2] );
        rfRAngle = T( 0.0 );  // any angle works
        rfYAngle = fRpY - rfRAngle;
        return false;
    }

    template <class T>
    auto Matrix3<T>::toEulerAnglesZXY( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const -> bool
    {
        // rot =  cy*cz-sx*sy*sz -cx*sz           cz*sy+cy*sx*sz
        //        cz*sx*sy+cy*sz  cx*cz          -cy*cz*sx+sy*sz
        //       -cx*sy           sx              cx*cy

        rfPAngle = Math<T>::ASin( m[2][1] );
        if( rfPAngle < static_cast<T>( Math<T>::half_pi() ) )
        {
            if( rfPAngle > static_cast<T>( -Math<T>::half_pi() ) )
            {
                rfYAngle = Math<T>::ATan2( -m[0][1], m[1][1] );
                rfRAngle = Math<T>::ATan2( -m[2][0], m[2][2] );
                return true;
            }
            // WARNING.  Not a unique solution.
            T fRmY = Math<T>::ATan2( m[0][2], m[0][0] );
            rfRAngle = T( 0.0 );  // any angle works
            rfYAngle = rfRAngle - fRmY;
            return false;
        }
        // WARNING.  Not a unique solution.
        T fRpY = Math<T>::ATan2( m[0][2], m[0][0] );
        rfRAngle = T( 0.0 );  // any angle works
        rfYAngle = fRpY - rfRAngle;
        return false;
    }

    template <class T>
    auto Matrix3<T>::toEulerAnglesZYX( T &rfYAngle, T &rfPAngle, T &rfRAngle ) const -> bool
    {
        // rot =  cy*cz           cz*sx*sy-cx*sz  cx*cz*sy+sx*sz
        //        cy*sz           cx*cz+sx*sy*sz -cz*sx+cx*sy*sz
        //       -sy              cy*sx           cx*cy

        rfPAngle = Math<T>::ASin( -m[2][0] );
        if( rfPAngle < static_cast<T>( Math<T>::half_pi() ) )
        {
            if( rfPAngle > static_cast<T>( -Math<T>::half_pi() ) )
            {
                rfYAngle = Math<T>::ATan2( m[1][0], m[0][0] );
                rfRAngle = Math<T>::ATan2( m[2][1], m[2][2] );
                return true;
            }
            // WARNING.  Not a unique solution.
            T fRmY = Math<T>::ATan2( -m[0][1], m[0][2] );
            rfRAngle = T( 0.0 );  // any angle works
            rfYAngle = rfRAngle - fRmY;
            return false;
        }
        // WARNING.  Not a unique solution.
        T fRpY = Math<T>::ATan2( -m[0][1], m[0][2] );
        rfRAngle = T( 0.0 );  // any angle works
        rfYAngle = fRpY - rfRAngle;
        return false;
    }

    template <class T>
    void Matrix3<T>::fromEulerAnglesXYZ( const T &fYAngle, const T &fPAngle, const T &fRAngle )
    {
        T fCos, fSin;

        fCos = Math<T>::Cos( fYAngle );
        fSin = Math<T>::Sin( fYAngle );
        Matrix3<T> kXMat( T( 1.0 ), T( 0.0 ), T( 0.0 ), T( 0.0 ), fCos, -fSin, T( 0.0 ), fSin, fCos );

        fCos = Math<T>::Cos( fPAngle );
        fSin = Math<T>::Sin( fPAngle );
        Matrix3<T> kYMat( fCos, T( 0.0 ), fSin, T( 0.0 ), T( 1.0 ), T( 0.0 ), -fSin, T( 0.0 ), fCos );

        fCos = Math<T>::Cos( fRAngle );
        fSin = Math<T>::Sin( fRAngle );
        Matrix3<T> kZMat( fCos, -fSin, T( 0.0 ), fSin, fCos, T( 0.0 ), T( 0.0 ), T( 0.0 ), T( 1.0 ) );

        *this = kXMat * ( kYMat * kZMat );
    }

    template <class T>
    void Matrix3<T>::fromEulerAnglesXZY( const T &fYAngle, const T &fPAngle, const T &fRAngle )
    {
        T fCos, fSin;

        fCos = Math<T>::Cos( fYAngle );
        fSin = Math<T>::Sin( fYAngle );
        Matrix3<T> kXMat( T( 1.0 ), T( 0.0 ), T( 0.0 ), T( 0.0 ), fCos, -fSin, T( 0.0 ), fSin, fCos );

        fCos = Math<T>::Cos( fPAngle );
        fSin = Math<T>::Sin( fPAngle );
        Matrix3<T> kZMat( fCos, -fSin, T( 0.0 ), fSin, fCos, T( 0.0 ), T( 0.0 ), T( 0.0 ), T( 1.0 ) );

        fCos = Math<T>::Cos( fRAngle );
        fSin = Math<T>::Sin( fRAngle );
        Matrix3<T> kYMat( fCos, T( 0.0 ), fSin, T( 0.0 ), T( 1.0 ), T( 0.0 ), -fSin, T( 0.0 ), fCos );

        *this = kXMat * ( kZMat * kYMat );
    }

    template <class T>
    void Matrix3<T>::fromEulerAnglesYXZ( const T &fYAngle, const T &fPAngle, const T &fRAngle )
    {
        T fCos, fSin;

        fCos = Math<T>::Cos( fYAngle );
        fSin = Math<T>::Sin( fYAngle );
        Matrix3 kYMat( fCos, T( 0.0 ), fSin, T( 0.0 ), T( 1.0 ), T( 0.0 ), -fSin, T( 0.0 ), fCos );

        fCos = Math<T>::Cos( fPAngle );
        fSin = Math<T>::Sin( fPAngle );
        Matrix3 kXMat( T( 1.0 ), T( 0.0 ), T( 0.0 ), T( 0.0 ), fCos, -fSin, T( 0.0 ), fSin, fCos );

        fCos = Math<T>::Cos( fRAngle );
        fSin = Math<T>::Sin( fRAngle );
        Matrix3 kZMat( fCos, -fSin, T( 0.0 ), fSin, fCos, T( 0.0 ), T( 0.0 ), T( 0.0 ), T( 1.0 ) );

        *this = kYMat * ( kXMat * kZMat );
    }

    template <class T>
    void Matrix3<T>::fromEulerAnglesYZX( const T &fYAngle, const T &fPAngle, const T &fRAngle )
    {
        T fCos, fSin;

        fCos = Math<T>::Cos( fYAngle );
        fSin = Math<T>::Sin( fYAngle );
        Matrix3<T> kYMat( fCos, T( 0.0 ), fSin, T( 0.0 ), T( 1.0 ), T( 0.0 ), -fSin, T( 0.0 ), fCos );

        fCos = Math<T>::Cos( fPAngle );
        fSin = Math<T>::Sin( fPAngle );
        Matrix3<T> kZMat( fCos, -fSin, T( 0.0 ), fSin, fCos, T( 0.0 ), T( 0.0 ), T( 0.0 ), T( 1.0 ) );

        fCos = Math<T>::Cos( fRAngle );
        fSin = Math<T>::Sin( fRAngle );
        Matrix3<T> kXMat( T( 1.0 ), T( 0.0 ), T( 0.0 ), T( 0.0 ), fCos, -fSin, T( 0.0 ), fSin, fCos );

        *this = kYMat * ( kZMat * kXMat );
    }

    template <class T>
    void Matrix3<T>::fromEulerAnglesZXY( const T &fYAngle, const T &fPAngle, const T &fRAngle )
    {
        T fCos, fSin;

        fCos = Math<T>::Cos( fYAngle );
        fSin = Math<T>::Sin( fYAngle );
        Matrix3<T> kZMat( fCos, -fSin, T( 0.0 ), fSin, fCos, T( 0.0 ), T( 0.0 ), T( 0.0 ), T( 1.0 ) );

        fCos = Math<T>::Cos( fPAngle );
        fSin = Math<T>::Sin( fPAngle );
        Matrix3<T> kXMat( T( 1.0 ), T( 0.0 ), T( 0.0 ), T( 0.0 ), fCos, -fSin, T( 0.0 ), fSin, fCos );

        fCos = Math<T>::Cos( fRAngle );
        fSin = Math<T>::Sin( fRAngle );
        Matrix3<T> kYMat( fCos, T( 0.0 ), fSin, T( 0.0 ), T( 1.0 ), T( 0.0 ), -fSin, T( 0.0 ), fCos );

        *this = kZMat * ( kXMat * kYMat );
    }

    template <class T>
    void Matrix3<T>::fromEulerAnglesZYX( const T &fYAngle, const T &fPAngle, const T &fRAngle )
    {
        T fCos, fSin;

        fCos = Math<T>::Cos( fYAngle );
        fSin = Math<T>::Sin( fYAngle );
        Matrix3<T> kZMat( fCos, -fSin, T( 0.0 ), fSin, fCos, T( 0.0 ), T( 0.0 ), T( 0.0 ), T( 1.0 ) );

        fCos = Math<T>::Cos( fPAngle );
        fSin = Math<T>::Sin( fPAngle );
        Matrix3<T> kYMat( fCos, T( 0.0 ), fSin, T( 0.0 ), T( 1.0 ), T( 0.0 ), -fSin, T( 0.0 ), fCos );

        fCos = Math<T>::Cos( fRAngle );
        fSin = Math<T>::Sin( fRAngle );
        Matrix3<T> kXMat( T( 1.0 ), T( 0.0 ), T( 0.0 ), T( 0.0 ), fCos, -fSin, T( 0.0 ), fSin, fCos );

        *this = kZMat * ( kYMat * kXMat );
    }

    template <class T>
    auto Matrix3<T>::transpose() const -> Matrix3<T>
    {
        Matrix3<T> kTranspose;
        for( u32 iRow = 0; iRow < 3; iRow++ )
        {
            for( u32 iCol = 0; iCol < 3; iCol++ )
            {
                kTranspose[iRow][iCol] = m[iCol][iRow];
            }
        }
        return kTranspose;
    }

    template <class T>
    void Matrix3<T>::QDUDecomposition( Matrix3<T> &kQ, Vector3<T> &kD, Vector3<T> &kU ) const
    {
        // Factor M = QR = QDU where Q is orthogonal, D is diagonal,
        // and U is upper triangular with ones on its diagonal.  Algorithm uses
        // Gram-Schmidt orthogonalization (the QR algorithm).
        //
        // If M = [ m0 | m1 | m2 ] and Q = [ q0 | q1 | q2 ], then
        //
        //   q0 = m0/|m0|
        //   q1 = (m1-(q0*m1)q0)/|m1-(q0*m1)q0|
        //   q2 = (m2-(q0*m2)q0-(q1*m2)q1)/|m2-(q0*m2)q0-(q1*m2)q1|
        //
        // where |V| indicates length of vector V and A*B indicates dot
        // product of vectors A and B.  The matrix R has entries
        //
        //   r00 = q0*m0  r01 = q0*m1  r02 = q0*m2
        //   r10 = 0      r11 = q1*m1  r12 = q1*m2
        //   r20 = 0      r21 = 0      r22 = q2*m2
        //
        // so D = diag(r00,r11,r22) and U has entries u01 = r01/r00,
        // u02 = r02/r00, and u12 = r12/r11.

        // Q = rotation
        // D = scaling
        // U = shear

        // D stores the three diagonal entries r00, r11, r22
        // U stores the entries U[0] = u01, U[1] = u02, U[2] = u12

        // build orthogonal matrix Q
        auto fInvLength = Math<T>::SqrtInv( m[0][0] * m[0][0] + m[1][0] * m[1][0] + m[2][0] * m[2][0] );

        kQ[0][0] = m[0][0] * fInvLength;
        kQ[1][0] = m[1][0] * fInvLength;
        kQ[2][0] = m[2][0] * fInvLength;

        auto fDot = kQ[0][0] * m[0][1] + kQ[1][0] * m[1][1] + kQ[2][0] * m[2][1];
        kQ[0][1] = m[0][1] - fDot * kQ[0][0];
        kQ[1][1] = m[1][1] - fDot * kQ[1][0];
        kQ[2][1] = m[2][1] - fDot * kQ[2][0];
        fInvLength = Math<T>::SqrtInv( kQ[0][1] * kQ[0][1] + kQ[1][1] * kQ[1][1] + kQ[2][1] * kQ[2][1] );

        kQ[0][1] *= fInvLength;
        kQ[1][1] *= fInvLength;
        kQ[2][1] *= fInvLength;

        fDot = kQ[0][0] * m[0][2] + kQ[1][0] * m[1][2] + kQ[2][0] * m[2][2];
        kQ[0][2] = m[0][2] - fDot * kQ[0][0];
        kQ[1][2] = m[1][2] - fDot * kQ[1][0];
        kQ[2][2] = m[2][2] - fDot * kQ[2][0];
        fDot = kQ[0][1] * m[0][2] + kQ[1][1] * m[1][2] + kQ[2][1] * m[2][2];
        kQ[0][2] -= fDot * kQ[0][1];
        kQ[1][2] -= fDot * kQ[1][1];
        kQ[2][2] -= fDot * kQ[2][1];
        fInvLength = Math<T>::SqrtInv( kQ[0][2] * kQ[0][2] + kQ[1][2] * kQ[1][2] + kQ[2][2] * kQ[2][2] );

        kQ[0][2] *= fInvLength;
        kQ[1][2] *= fInvLength;
        kQ[2][2] *= fInvLength;

        // guarantee that orthogonal matrix has determinant 1 (no reflections)
        auto fDet = kQ[0][0] * kQ[1][1] * kQ[2][2] + kQ[0][1] * kQ[1][2] * kQ[2][0] +
                    kQ[0][2] * kQ[1][0] * kQ[2][1] - kQ[0][2] * kQ[1][1] * kQ[2][0] -
                    kQ[0][1] * kQ[1][0] * kQ[2][2] - kQ[0][0] * kQ[1][2] * kQ[2][1];

        if( fDet < T( 0.0 ) )
        {
            for( u32 iRow = 0; iRow < 3; iRow++ )
            {
                for( u32 iCol = 0; iCol < 3; iCol++ )
                {
                    kQ[iRow][iCol] = -kQ[iRow][iCol];
                }
            }
        }

        // build "right" matrix R
        Matrix3<T> kR;
        kR[0][0] = kQ[0][0] * m[0][0] + kQ[1][0] * m[1][0] + kQ[2][0] * m[2][0];
        kR[0][1] = kQ[0][0] * m[0][1] + kQ[1][0] * m[1][1] + kQ[2][0] * m[2][1];
        kR[1][1] = kQ[0][1] * m[0][1] + kQ[1][1] * m[1][1] + kQ[2][1] * m[2][1];
        kR[0][2] = kQ[0][0] * m[0][2] + kQ[1][0] * m[1][2] + kQ[2][0] * m[2][2];
        kR[1][2] = kQ[0][1] * m[0][2] + kQ[1][1] * m[1][2] + kQ[2][1] * m[2][2];
        kR[2][2] = kQ[0][2] * m[0][2] + kQ[1][2] * m[1][2] + kQ[2][2] * m[2][2];

        // the scaling component
        kD[0] = kR[0][0];
        kD[1] = kR[1][1];
        kD[2] = kR[2][2];

        // the shear component
        auto fInvD0 = T( 1.0 ) / kD[0];
        kU[0] = kR[0][1] * fInvD0;
        kU[1] = kR[0][2] * fInvD0;
        kU[2] = kR[1][2] / kD[1];
    }

    template <class T>
    workphone::Matrix3<T> Matrix3<T>::identity()
    {
        static Matrix3<T> m( 1, 0, 0, 0, 1, 0, 0, 0, 1 );
        return m;
    }

    template <class T>
    bool Matrix3<T>::operator!=( const Matrix3<T> &rkMatrix ) const
    {
        return !operator==( rkMatrix );
    }

    template <class T>
    workphone::Matrix3<T> &Matrix3<T>::operator=( const Matrix3<T> &rkMatrix )
    {
        std::memcpy( m, rkMatrix.m, 9 * sizeof( T ) );
        return *this;
    }

    template <class T>
    T *Matrix3<T>::operator[]( u32 rowIndex ) const
    {
        auto p = const_cast<T *>( m[rowIndex] );
        return static_cast<T *>( p );
    }

    template <class T>
    Matrix3<T>::Matrix3( T fEntry00, T fEntry01, T fEntry02, T fEntry10, T fEntry11, T fEntry12,
                         T fEntry20, T fEntry21, T fEntry22 )
    {
        m[0][0] = fEntry00;
        m[0][1] = fEntry01;
        m[0][2] = fEntry02;
        m[1][0] = fEntry10;
        m[1][1] = fEntry11;
        m[1][2] = fEntry12;
        m[2][0] = fEntry20;
        m[2][1] = fEntry21;
        m[2][2] = fEntry22;
    }

    template <class T>
    Matrix3<T>::Matrix3()
    {
    }

    // explicit instantiation
    template class Matrix3<s32>;
    template class Matrix3<f32>;
    template class Matrix3<f64>;

}  // namespace workphone
