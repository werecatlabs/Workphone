#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    template <typename T>
    Quaternion<T>::Quaternion() : x( T( 0.0 ) ), y( T( 0.0 ) ), z( T( 0.0 ) ), w( T( 1.0 ) )
    {
    }

    template <typename T>
    Quaternion<T>::Quaternion( T w, T x, T y, T z ) : w( w ), x( x ), y( y ), z( z )
    {
    }

    template <typename T>
    Quaternion<T>::Quaternion( T x, T y, T z )
    {
        set( x, y, z );
    }

    template <typename T>
    Quaternion<T>::Quaternion( const Quaternion &other ) :
        w( other.w ),
        x( other.x ),
        y( other.y ),
        z( other.z )
    {
    }

    template <typename T>
    Quaternion<T>::Quaternion( const T *ptr )
    {
        x = ptr[0];
        y = ptr[1];
        z = ptr[1];
        w = ptr[3];
    }

    template <typename T>
    Quaternion<T>::Quaternion( const Matrix3<T> &mat )
    {
        fromRotationMatrix( mat );
    }

    template <typename T>
    Quaternion<T>::Quaternion( const Vector3<T> &xaxis, const Vector3<T> &yaxis,
                               const Vector3<T> &zaxis )
    {
        fromAxes( xaxis, yaxis, zaxis );
    }

    template <typename T>
    bool Quaternion<T>::operator==( const Quaternion &other ) const
    {
        return Math<T>::equals( X(), other.X() ) && Math<T>::equals( Y(), other.Y() ) &&
               Math<T>::equals( Z(), other.Z() ) && Math<T>::equals( W(), other.W() );
    }

    template <typename T>
    bool Quaternion<T>::operator!=( const Quaternion &other ) const
    {
        return !( Math<T>::equals( X(), other.X() ) && Math<T>::equals( Y(), other.Y() ) &&
                  Math<T>::equals( Z(), other.Z() ) && Math<T>::equals( W(), other.W() ) );
    }

    template <typename T>
    Quaternion<T> &Quaternion<T>::operator=( const Quaternion &other )
    {
        w = other.w;
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::operator*( const Quaternion &q ) const
    {
        // Perform the Hamilton product of the two quaternions
        Quaternion<T> product;
        product.w = w * q.w - x * q.x - y * q.y - z * q.z;
        product.x = w * q.x + x * q.w + y * q.z - z * q.y;
        product.y = w * q.y + y * q.w + z * q.x - x * q.z;
        product.z = w * q.z + z * q.w + x * q.y - y * q.x;

        return product;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::operator*( T s ) const
    {
        return Quaternion<T>( s * w, s * x, s * y, s * z );
    }

    template <typename T>
    Quaternion<T> &Quaternion<T>::operator*=( T s )
    {
        w *= s;
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    template <typename T>
    Quaternion<T> &Quaternion<T>::operator*=( const Quaternion &other )
    {
        *this = other * ( *this );
        return *this;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::operator+( const Quaternion &other ) const
    {
        return Quaternion<T>( w + other.w, x + other.x, y + other.y, z + other.z );
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::operator-( const Quaternion &other ) const
    {
        return Quaternion<T>( w - other.w, x - other.x, y - other.y, z - other.z );
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::operator-() const
    {
        return Quaternion( -w, -x, -y, -z );
    }

    template <typename T>
    void Quaternion<T>::set( T x, T y, T z, T w )
    {
        X() = x;
        Y() = y;
        Z() = z;
        W() = w;
    }

    template <typename T>
    void Quaternion<T>::normalise()
    {
        const T n = x * x + y * y + z * z + w * w;

        if( n <= T( 0.0 ) )
        {
            // Zero-length quaternion, reset to identity
            w = T( 1.0 );
            x = y = z = T( 0.0 );
            return;
        }

        if( Math<T>::equals( n, T( 1.0 ) ) )
        {
            // Already normalized
            return;
        }

        // Calculate inverse square root and apply
        const T invLen = Math<T>::SqrtInv( n );
        x *= invLen;
        y *= invLen;
        z *= invLen;
        w *= invLen;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::normaliseCopy() const
    {
        auto ret = *this;
        ret.normalise();
        return ret;
    }

    template <typename T>
    T Quaternion<T>::dotProduct( const Quaternion &q2 ) const
    {
        return x * q2.x + y * q2.y + z * q2.z + w * q2.w;
    }

    template <typename T>
    void Quaternion<T>::fromAngleAxis( T angle, const Vector3<T> &axis )
    {
        // assert:  axis[] is unit length
        //
        // The quaternion representing the rotation is
        //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

        auto halfAngle = T( 0.5 ) * angle;
        auto sn = Math<T>::Sin( halfAngle );
        w = Math<T>::Cos( halfAngle );
        x = sn * axis[0];
        y = sn * axis[1];
        z = sn * axis[2];
    }

    template <typename T>
    void Quaternion<T>::makeIdentity()
    {
        w = T( 1.0 );
        x = T( 0.0 );
        y = T( 0.0 );
        z = T( 0.0 );
    }

    template <typename T>
    Quaternion<T>::operator const T *() const
    {
        return &w;
    }

    template <typename T>
    Quaternion<T>::operator T *()
    {
        return &w;
    }

    template <typename T>
    T Quaternion<T>::operator[]( int i ) const
    {
        WP_ASSERT( i >= 0 );
        WP_ASSERT( i <= 3 );
        return ( &w )[i];
    }

    template <typename T>
    T &Quaternion<T>::operator[]( int i )
    {
        WP_ASSERT( i >= 0 );
        WP_ASSERT( i <= 3 );
        return ( &w )[i];
    }

    template <typename T>
    T Quaternion<T>::X() const
    {
        return x;
    }

    template <typename T>
    T &Quaternion<T>::X()
    {
        return x;
    }

    template <typename T>
    T Quaternion<T>::Y() const
    {
        return y;
    }

    template <typename T>
    T &Quaternion<T>::Y()
    {
        return y;
    }

    template <typename T>
    T Quaternion<T>::Z() const
    {
        return z;
    }

    template <typename T>
    T &Quaternion<T>::Z()
    {
        return z;
    }

    template <typename T>
    T Quaternion<T>::W() const
    {
        return w;
    }

    template <typename T>
    T &Quaternion<T>::W()
    {
        return w;
    }

    template <typename T>
    void Quaternion<T>::fromDegrees( const Vector3<T> &degrees )
    {
        auto radians = degrees * Math<T>::deg_to_rad();
        set( radians.x, radians.y, radians.z );
    }

    template <typename T>
    void Quaternion<T>::fromRadians( const Vector3<T> &radians )
    {
        set( radians.x, radians.y, radians.z );
    }

    template <typename T>
    void Quaternion<T>::set( T x, T y, T z )
    {
        T angle;

        angle = x * T( 0.5 );
        T sr = Math<T>::Sin( angle );
        T cr = Math<T>::Cos( angle );

        angle = y * T( 0.5 );
        T sp = Math<T>::Sin( angle );
        T cp = Math<T>::Cos( angle );

        angle = z * T( 0.5 );
        T sy = Math<T>::Sin( angle );
        T cy = Math<T>::Cos( angle );

        T cpcy = cp * cy;
        T spcy = sp * cy;
        T cpsy = cp * sy;
        T spsy = sp * sy;

        X() = ( sr * cpcy - cr * spsy );
        Y() = ( cr * spcy + sr * cpsy );
        Z() = ( cr * cpsy - sr * spcy );
        W() = ( cr * cpcy + sr * spsy );

        normalise();
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::inverse() const
    {
        // Calculate the norm of the quaternion
        auto fNorm = norm();

        // If the norm is greater than zero, calculate and return the inverse
        if( fNorm > T( 0.0 ) )
        {
            auto fInvNorm = T( 1.0 ) / fNorm;
            return Quaternion( w * fInvNorm, -x * fInvNorm, -y * fInvNorm, -z * fInvNorm );
        }

        // If the norm is negative, return an invalid result to flag an error
        return {};
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::exp() const
    {
        // If q = A*(x*i+y*j+z*k) where (x,y,z) is unit length, then
        // exp(q) = cos(A)+sin(A)*(x*i+y*j+z*k).  If sin(A) is near zero,
        // use exp(q) = cos(A)+A*(x*i+y*j+z*k) since A/sin(A) has limit 1.

        auto fAngle( Math<T>::Sqrt( X() * X() + Y() * Y() + Z() * Z() ) );
        auto fSin = Math<T>::Sin( fAngle );

        Quaternion kResult;
        kResult.W() = Math<T>::Cos( fAngle );

        if( Math<T>::Abs( fSin ) >= Math<T>::epsilon() )
        {
            auto fCoeff = fSin / fAngle;
            kResult.X() = fCoeff * X();
            kResult.Y() = fCoeff * Y();
            kResult.Z() = fCoeff * Z();
        }
        else
        {
            kResult.X() = X();
            kResult.Y() = Y();
            kResult.Z() = Z();
        }

        return kResult;
    }

    template <typename T>
    Vector3<T> Quaternion<T>::operator*( const Vector3<T> &v ) const
    {
        // Convert the quaternion to a 3D vector
        Vector3<T> qvec( x, y, z );

        // Calculate intermediate vectors
        Vector3<T> uv = qvec.crossProduct( v );
        Vector3<T> uuv = qvec.crossProduct( uv );

        // Scale the vectors and add them together to get the result
        uv *= T( 2.0 ) * w;
        uuv *= T( 2.0 );

        return v + uv + uuv;
    }

    template <typename T>
    T Quaternion<T>::norm() const
    {
        return w * w + x * x + y * y + z * z;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::angleAxis( const T &rfAngle, const Vector3<T> &rkAxis )
    {
        Quaternion r;
        r.fromAngleAxis( rfAngle, rkAxis );
        return r;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::euler( T x, T y, T z )
    {
        Matrix3<T> mat;
        mat.fromEulerAnglesXYZ( x, y, z );

        return Quaternion<T>( mat );
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::eulerDegrees( T x, T y, T z )
    {
        Matrix3<T> mat;
        mat.fromEulerAnglesXYZ( Math<T>::DegToRad( x ), Math<T>::DegToRad( y ), Math<T>::DegToRad( z ) );

        return Quaternion<T>( mat );
    }

    template <typename T>
    const Quaternion<T> Quaternion<T>::identity()
    {
        static const Quaternion qIdentity( T( 1.0 ), T( 0.0 ), T( 0.0 ), T( 0.0 ) );
        return qIdentity;
    }

    template <typename T>
    T *Quaternion<T>::ptr()
    {
        return &x;
    }

    template <typename T>
    const T *Quaternion<T>::ptr() const
    {
        return &x;
    }

    template <typename T>
    T Quaternion<T>::magnitudeSquared() const
    {
        return x * x + y * y + z * z + w * w;
    }

    template <typename T>
    T Quaternion<T>::magnitude() const
    {
        return Math<T>::Sqrt( magnitudeSquared() );
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::getConjugate() const
    {
        return Quaternion<T>( w, -x, -y, -z );
    }

    template <typename T>
    void Quaternion<T>::fromRotationMatrix( const Matrix3<T> &kRot )
    {
        // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
        // article "Quaternion Calculus and Fast Animation".

        T fTrace = kRot[0][0] + kRot[1][1] + kRot[2][2];
        T fRoot;

        if( fTrace > 0.0 )
        {
            // |w| > 1/2, may as well choose w > 1/2
            fRoot = Math<T>::Sqrt( fTrace + T( 1.0 ) );  // 2w
            W() = T( 0.5 ) * fRoot;
            fRoot = T( 0.5 ) / fRoot;  // 1/(4w)
            X() = ( kRot[2][1] - kRot[1][2] ) * fRoot;
            Y() = ( kRot[0][2] - kRot[2][0] ) * fRoot;
            Z() = ( kRot[1][0] - kRot[0][1] ) * fRoot;
        }
        else
        {
            // |w| <= 1/2
            static u32 s_iNext[3] = { 1, 2, 0 };
            u32 i = 0;
            if( kRot[1][1] > kRot[0][0] )
                i = 1;
            if( kRot[2][2] > kRot[i][i] )
                i = 2;
            u32 j = s_iNext[i];
            u32 k = s_iNext[j];

            fRoot = Math<T>::Sqrt( kRot[i][i] - kRot[j][j] - kRot[k][k] + T( 1.0 ) );
            T *apkQuat[3] = { &x, &y, &z };
            *apkQuat[i] = T( 0.5 ) * fRoot;
            fRoot = T( 0.5 ) / fRoot;
            W() = ( kRot[k][j] - kRot[j][k] ) * fRoot;
            *apkQuat[j] = ( kRot[j][i] + kRot[i][j] ) * fRoot;
            *apkQuat[k] = ( kRot[k][i] + kRot[i][k] ) * fRoot;
        }
    }

    template <typename T>
    auto Quaternion<T>::getYaw( bool reprojectAxis /*= true*/ ) const -> T
    {
        if( reprojectAxis )
        {
            // yaw = atan2(localz.x, localz.z)
            // pick parts of zAxis() implementation that we need
            auto fTx = T( 2.0 ) * X();
            auto fTy = T( 2.0 ) * Y();
            auto fTz = T( 2.0 ) * Z();
            auto fTwy = fTy * W();
            auto fTxx = fTx * X();
            auto fTxz = fTz * X();
            auto fTyy = fTy * Y();

            return Math<T>::ATan2( fTxz + fTwy, T( 1.0 ) - ( fTxx + fTyy ) );
        }

        // internal version
        return Math<T>::ASin( T( -2.0 ) * ( X() * Z() - W() * Y() ) );
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::getRotationTo( const Vector3<T> &src, const Vector3<T> &dest,
                                                const Vector3<T> &fallbackAxis )
    {
        // From Sam Hocevar's article "Quaternion from two vectors:
        // the final version"
        auto a = Math<T>::Sqrt( src.lengthSquared() * dest.lengthSquared() );
        auto b = a + src.dotProduct( dest );
        auto axis = Vector3<T>::zero();

        if( b < static_cast<T>( 1e-06 ) * a )
        {
            b = static_cast<T>( 0.0 );
            axis = fallbackAxis != Vector3<T>::zero() ? fallbackAxis
                   : Math<T>::Abs( src.X() ) > Math<T>::Abs( src.Z() )
                       ? Vector3<T>( -src.Y(), src.X(), static_cast<T>( 0.0 ) )
                       : Vector3<T>( static_cast<T>( 0.0 ), -src.Z(), src.Y() );
        }
        else
        {
            axis = src.crossProduct( dest );
        }

        Quaternion<T> q( b, axis.X(), axis.Y(), axis.Z() );
        q.normalise();
        return q;
    }

    template <typename T>
    void Quaternion<T>::toRotationMatrix( Matrix3<T> &kRot ) const
    {
        auto fTx = x + x;
        auto fTy = y + y;
        auto fTz = z + z;
        auto fTwx = fTx * w;
        auto fTwy = fTy * w;
        auto fTwz = fTz * w;
        auto fTxx = fTx * x;
        auto fTxy = fTy * x;
        auto fTxz = fTz * x;
        auto fTyy = fTy * y;
        auto fTyz = fTz * y;
        auto fTzz = fTz * z;

        kRot[0][0] = T( 1.0 ) - ( fTyy + fTzz );
        kRot[0][1] = fTxy - fTwz;
        kRot[0][2] = fTxz + fTwy;
        kRot[1][0] = fTxy + fTwz;
        kRot[1][1] = T( 1.0 ) - ( fTxx + fTzz );
        kRot[1][2] = fTyz - fTwx;
        kRot[2][0] = fTxz - fTwy;
        kRot[2][1] = fTyz + fTwx;
        kRot[2][2] = T( 1.0 ) - ( fTxx + fTyy );
    }

    template <typename T>
    void Quaternion<T>::fromAxes( const Vector3<T> &xaxis, const Vector3<T> &yaxis,
                                  const Vector3<T> &zaxis )
    {
        Matrix3<T> kRot;

        kRot[0][0] = xaxis[0];
        kRot[1][0] = xaxis[1];
        kRot[2][0] = xaxis[2];

        kRot[0][1] = yaxis[0];
        kRot[1][1] = yaxis[1];
        kRot[2][1] = yaxis[2];

        kRot[0][2] = zaxis[0];
        kRot[1][2] = zaxis[1];
        kRot[2][2] = zaxis[2];

        fromRotationMatrix( kRot );
    }

    template <typename T>
    void Quaternion<T>::toAxes( Vector3<T> &xaxis, Vector3<T> &yaxis, Vector3<T> &zaxis ) const
    {
        Matrix3<T> kRot;

        toRotationMatrix( kRot );

        xaxis[0] = kRot[0][0];
        xaxis[1] = kRot[1][0];
        xaxis[2] = kRot[2][0];

        yaxis[0] = kRot[0][1];
        yaxis[1] = kRot[1][1];
        yaxis[2] = kRot[2][1];

        zaxis[0] = kRot[0][2];
        zaxis[1] = kRot[1][2];
        zaxis[2] = kRot[2][2];
    }

    template <typename T>
    Vector3<T> Quaternion<T>::rotate( const Vector3<T> &v ) const
    {
        const auto vx = T( 2.0 ) * v.X();
        const auto vy = T( 2.0 ) * v.Y();
        const auto vz = T( 2.0 ) * v.Z();
        const auto w2 = W() * W() - T( 0.5 );
        const auto dot2 = ( X() * vx + Y() * vy + Z() * vz );

        return Vector3<T>( ( vx * w2 + ( Y() * vz - Z() * vy ) * W() + X() * dot2 ),
                           ( vy * w2 + ( Z() * vx - X() * vz ) * W() + Y() * dot2 ),
                           ( vz * w2 + ( X() * vy - Y() * vx ) * W() + Z() * dot2 ) );
    }

    template <typename T>
    Vector3<T> Quaternion<T>::rotateInv( const Vector3<T> &v ) const
    {
        const auto vx = T( 2.0 ) * v.X();
        const auto vy = T( 2.0 ) * v.Y();
        const auto vz = T( 2.0 ) * v.Z();
        const auto w2 = W() * W() - T( 0.5 );
        const auto dot2 = ( X() * vx + Y() * vy + Z() * vz );

        return Vector3<T>( ( vx * w2 - ( Y() * vz - Z() * vy ) * W() + X() * dot2 ),
                           ( vy * w2 - ( Z() * vx - X() * vz ) * W() + Y() * dot2 ),
                           ( vz * w2 - ( X() * vy - Y() * vx ) * W() + Z() * dot2 ) );
    }

    template <typename T>
    Vector3<T> Quaternion<T>::getBasisVector0() const
    {
        const auto x2 = X() * T( 2.0 );
        const auto w2 = W() * T( 2.0 );

        return Vector3<T>( ( W() * w2 ) - T( 1.0 ) + X() * x2, ( Z() * w2 ) + Y() * x2,
                           ( -Y() * w2 ) + Z() * x2 );
    }

    template <typename T>
    Vector3<T> Quaternion<T>::getBasisVector1() const
    {
        const auto y2 = Y() * T( 2.0 );
        const auto w2 = W() * T( 2.0 );

        return Vector3<T>( ( -Z() * w2 ) + X() * y2, ( W() * w2 ) - T( 1.0 ) + Y() * y2,
                           ( X() * w2 ) + Z() * y2 );
    }

    template <typename T>
    Vector3<T> Quaternion<T>::getBasisVector2() const
    {
        const auto z2 = Z() * T( 2.0 );
        const auto w2 = W() * T( 2.0 );

        return Vector3<T>( ( Y() * w2 ) + X() * z2, ( -X() * w2 ) + Y() * z2,
                           ( W() * w2 ) - T( 1.0 ) + Z() * z2 );
    }

    template <typename T>
    bool Quaternion<T>::isFinite() const
    {
        return Math<T>::isFinite( w ) && Math<T>::isFinite( x ) && Math<T>::isFinite( y ) &&
               Math<T>::isFinite( z );
    }

    template <typename T>
    bool Quaternion<T>::isValid() const
    {
        return isSane();
    }

    template <typename T>
    bool Quaternion<T>::isSane() const
    {
        constexpr auto defaultTolerance = T( 0.001 );
        constexpr auto defaultMagnitude = T( 1.0 );

        auto tolerance = defaultTolerance * T( 1.0 );  // Adjust as needed

        return isFinite() && ( Math<T>::Abs( magnitude() ) - defaultMagnitude ) < tolerance;
    }

    template <typename T>
    bool Quaternion<T>::isUnit() const
    {
        const T unitTolerance = T( 1e-4 );
        return isFinite() && Math<T>::Abs( magnitude() - T( 1.0 ) ) < unitTolerance;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::log() const
    {
        // If q = cos(A)+sin(A)*(x*i+y*j+z*k) where (x,y,z) is unit length, then
        // log(q) = A*(x*i+y*j+z*k).  If sin(A) is near zero, use log(q) =
        // sin(A)*(x*i+y*j+z*k) since sin(A)/A has limit 1.

        auto kResult = Quaternion<T>::identity();
        kResult.W() = T( 0.0 );

        if( Math<T>::Abs( W() ) < T( 1.0 ) )
        {
            auto fAngle( Math<T>::ACos( W() ) );
            auto fSin = Math<T>::Sin( fAngle );
            if( Math<T>::Abs( fSin ) >= Math<T>::epsilon() )
            {
                auto fCoeff = fAngle / fSin;
                kResult.X() = fCoeff * X();
                kResult.Y() = fCoeff * Y();
                kResult.Z() = fCoeff * Z();
                return kResult;
            }
        }

        kResult.X() = X();
        kResult.Y() = Y();
        kResult.Z() = Z();

        return kResult;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::slerp( T t, const Quaternion<T> &q1, const Quaternion<T> &q2,
                                        bool shortestPath )
    {
        auto cosTheta = q1.dotProduct( q2 );

        // Adjust the sign of q2 to take the shortest path
        auto q2Adjusted = q2;
        if( shortestPath && cosTheta < T( 0 ) )
        {
            q2Adjusted = -q2;
            cosTheta = -cosTheta;
        }

        auto theta = Math<T>::ACos( cosTheta );
        auto sinTheta = Math<T>::Sin( theta );

        if( sinTheta < std::numeric_limits<T>::epsilon() )
        {
            // Quaternions are very close, use linear interpolation
            return Math<T>::lerp( q1, q2Adjusted, t );
        }

        auto coeff1 = Math<T>::Sin( ( T( 1.0 ) - t ) * theta ) / sinTheta;
        auto coeff2 = Math<T>::Sin( t * theta ) / sinTheta;

        return coeff1 * q1 + coeff2 * q2Adjusted;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::squad( T fT, const Quaternion<T> &rkP, const Quaternion<T> &rkA,
                                        const Quaternion<T> &rkB, const Quaternion<T> &rkQ,
                                        bool shortestPath )
    {
        auto fSlerpT = T( 2.0 ) * fT * ( T( 1.0 ) - fT );
        auto kSlerpP = slerp( fT, rkP, rkQ, shortestPath );
        auto kSlerpQ = slerp( fT, rkA, rkB );
        return slerp( fSlerpT, kSlerpP, kSlerpQ );
    }

    // explicit instantiation
    template class Quaternion<s32>;
    template class Quaternion<f32>;
    template class Quaternion<f64>;

}  // namespace workphone
