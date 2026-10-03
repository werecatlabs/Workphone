#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    template <>
    const Vector3<s32> Vector3<s32>::ZERO( 0, 0, 0 );
    template <>
    const Vector3<s32> Vector3<s32>::UNIT_X( 1, 0, 0 );
    template <>
    const Vector3<s32> Vector3<s32>::UNIT_Y( 0, 1, 0 );
    template <>
    const Vector3<s32> Vector3<s32>::UNIT_Z( 0, 0, 1 );
    template <>
    const Vector3<s32> Vector3<s32>::UNIT( 1, 1, 1 );

    template <>
    const Vector3<f32> Vector3<f32>::ZERO( 0.0f, 0.0f, 0.0f );
    template <>
    const Vector3<f32> Vector3<f32>::UNIT_X( 1.0f, 0.0f, 0.0f );
    template <>
    const Vector3<f32> Vector3<f32>::UNIT_Y( 0.0f, 1.0f, 0.0f );
    template <>
    const Vector3<f32> Vector3<f32>::UNIT_Z( 0.0f, 0.0f, 1.0f );
    template <>
    const Vector3<f32> Vector3<f32>::UNIT( 1.0f, 1.0f, 1.0f );

    template <>
    const Vector3<f64> Vector3<f64>::ZERO( 0.0, 0.0, 0.0 );
    template <>
    const Vector3<f64> Vector3<f64>::UNIT_X( 1.0, 0.0, 0.0 );
    template <>
    const Vector3<f64> Vector3<f64>::UNIT_Y( 0.0, 1.0, 0.0 );
    template <>
    const Vector3<f64> Vector3<f64>::UNIT_Z( 0.0, 0.0, 1.0 );
    template <>
    const Vector3<f64> Vector3<f64>::UNIT( 1.0, 1.0, 1.0 );

    template <class T>
    Vector3<T>::Vector3() : x( 0 ), y( 0 ), z( 0 )
    {
    }

    template <class T>
    Vector3<T>::Vector3( T x, T y, T z ) : x( x ), y( y ), z( z )
    {
    }

    template <class T>
    Vector3<T>::Vector3( const Vector3<T> &other )
    {
        *this = other;
    }

    template <class T>
    Vector3<T>::Vector3( const T *ptr )
    {
        auto p = this->ptr();
        p[0] = ptr[0];
        p[1] = ptr[1];
        p[2] = ptr[2];
    }

    //
    // operators
    //

    template <class T>
    Vector3<T> Vector3<T>::operator-() const
    {
        return Vector3<T>( -x, -y, -z );
    }

    template <class T>
    Vector3<T> &Vector3<T>::operator=( const Vector3<T> &other )
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    template <class T>
    Vector3<T> Vector3<T>::operator+( const Vector3<T> &other ) const
    {
        return Vector3<T>( x + other.x, y + other.y, z + other.z );
    }

    template <class T>
    Vector3<T> &Vector3<T>::operator+=( const Vector3<T> &other )
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    template <class T>
    Vector3<T> Vector3<T>::operator-( const Vector3<T> &other ) const
    {
        return Vector3<T>( x - other.x, y - other.y, z - other.z );
    }

    template <class T>
    Vector3<T> &Vector3<T>::operator-=( const Vector3<T> &other )
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    template <class T>
    Vector3<T> Vector3<T>::operator*( const Vector3<T> &other ) const
    {
        return Vector3<T>( x * other.x, y * other.y, z * other.z );
    }

    template <class T>
    Vector3<T> &Vector3<T>::operator*=( const Vector3<T> &other )
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

    template <class T>
    Vector3<T> Vector3<T>::operator*( const T v ) const
    {
        return Vector3<T>( x * v, y * v, z * v );
    }

    template <class T>
    Vector3<T> &Vector3<T>::operator*=( const T v )
    {
        x *= v;
        y *= v;
        z *= v;
        return *this;
    }

    template <class T>
    Vector3<T> Vector3<T>::operator/( const Vector3<T> &other ) const
    {
        return Vector3<T>( x / other.x, y / other.y, z / other.z );
    }

    template <class T>
    Vector3<T> &Vector3<T>::operator/=( const Vector3<T> &other )
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        return *this;
    }

    template <class T>
    Vector3<T> Vector3<T>::operator/( const T v ) const
    {
        auto invValue = T( 1.0 ) / v;
        return Vector3<T>( x * invValue, y * invValue, z * invValue );
    }

    template <class T>
    Vector3<T> &Vector3<T>::operator/=( const T v )
    {
        auto invValue = T( 1.0 ) / v;
        x *= invValue;
        y *= invValue;
        z *= invValue;
        return *this;
    }

    template <class T>
    bool Vector3<T>::operator<=( const Vector3<T> &other ) const
    {
        return compare( other ) <= 0;
    }

    template <class T>
    bool Vector3<T>::operator>=( const Vector3<T> &other ) const
    {
        return compare( other ) >= 0;
    }

    template <class T>
    bool Vector3<T>::operator<( const Vector3<T> &other ) const
    {
        return compare( other ) < 0;
    }

    template <class T>
    bool Vector3<T>::operator>( const Vector3<T> &other ) const
    {
        return compare( other ) > 0;
    }

    template <class T>
    bool Vector3<T>::operator<( const T value ) const
    {
        return lengthSquared() < value;
    }

    template <class T>
    bool Vector3<T>::operator>( const T value ) const
    {
        return lengthSquared() > value;
    }

    template <class T>
    Vector3<T>::operator const T *() const
    {
        const auto p = ptr();
        return p;
    }

    template <class T>
    Vector3<T>::operator T *()
    {
        auto p = ptr();
        return p;
    }

    template <class T>
    T Vector3<T>::operator[]( s32 i ) const
    {
        const auto p = ptr();
        return p[i];
    }

    template <class T>
    T &Vector3<T>::operator[]( s32 i )
    {
        auto p = ptr();
        return p[i];
    }

    template <class T>
    T Vector3<T>::X() const
    {
        return x;
    }

    template <class T>
    T &Vector3<T>::X()
    {
        return x;
    }

    template <class T>
    T Vector3<T>::Y() const
    {
        return y;
    }

    template <class T>
    T &Vector3<T>::Y()
    {
        return y;
    }

    template <class T>
    T Vector3<T>::Z() const
    {
        return z;
    }

    template <class T>
    T &Vector3<T>::Z()
    {
        return z;
    }

    template <class T>
    bool Vector3<T>::equals( const Vector3<T> &other, T tolerance ) const
    {
        WP_UNUSED( tolerance );

        return Math<T>::equals( x, other.x ) && Math<T>::equals( y, other.y ) &&
               Math<T>::equals( z, other.z );
    }

    template <class T>
    void Vector3<T>::set( const T x, const T y, const T z )
    {
        auto p = ptr();
        p[0] = x;
        p[1] = y;
        p[2] = z;
    }

    template <class T>
    void Vector3<T>::set( const Vector3<T> &other )
    {
        x = other.x;
        y = other.y;
        z = other.z;
    }

    template <class T>
    T Vector3<T>::length() const
    {
        return Math<T>::Sqrt( lengthSquared() );
    }

    template <class T>
    T Vector3<T>::lengthSquared() const
    {
        return x * x + y * y + z * z;
    }

    template <class T>
    T Vector3<T>::dotProduct( const Vector3<T> &other ) const
    {
        return x * other.x + y * other.y + z * other.z;
    }

    template <class T>
    T Vector3<T>::dotProductABS( const Vector3<T> &vec ) const
    {
        return Math<T>::Abs( x * vec.x ) + Math<T>::Abs( y * vec.y ) + Math<T>::Abs( z * vec.z );
    }

    template <class T>
    T Vector3<T>::getDistanceFrom( const Vector3<T> &other ) const
    {
        return Vector3<T>( x - other.x, y - other.y, z - other.z ).length();
    }

    template <class T>
    T Vector3<T>::getDistanceFromSQ( const Vector3<T> &other ) const
    {
        return Vector3<T>( x - other.x, y - other.y, z - other.z ).lengthSquared();
    }

    template <class T>
    Vector3<T> Vector3<T>::crossProduct( const Vector3<T> &p ) const
    {
        return Vector3<T>( y * p.z - z * p.y, z * p.x - x * p.z, x * p.y - y * p.x );
    }

    template <class T>
    bool Vector3<T>::isBetweenPoints( const Vector3<T> &begin, const Vector3<T> &end ) const
    {
        T f = ( end - begin ).lengthSquared();
        return getDistanceFromSQ( begin ) < f && getDistanceFromSQ( end ) < f;
    }

    template <class T>
    Vector3<T> Vector3<T>::normaliseCopy() const
    {
        auto ret = *this;
        ret.normalise();
        return ret;
    }

    template <class T>
    Vector3<T> &Vector3<T>::normaliseFast()
    {
        const T lengthSQ = x * x + y * y + z * z;

        // Early exit for zero-length vectors
        if( lengthSQ < Math<T>::epsilon() )
        {
            return *this;
        }

        // Use inverse square root for single multiplication instead of three divisions
        const T invLength = Math<T>::SqrtInv( lengthSQ );
        x *= invLength;
        y *= invLength;
        z *= invLength;

        return *this;
    }

    template <class T>
    T Vector3<T>::normaliseLength()
    {
        auto lengthSQ = lengthSquared();
        if( lengthSQ < Math<T>::epsilon() )
        {
            return T( 0.0 );
        }

        auto l = Math<T>::SqrtInv( lengthSQ );
        x *= l;
        y *= l;
        z *= l;

        return T( 1.0 ) / l;
    }

    template <class T>
    void Vector3<T>::normalise()
    {
        auto lengthSQ = lengthSquared();
        if( lengthSQ < Math<T>::epsilon() )
        {
            return;
        }

        auto l = Math<T>::Sqrt( lengthSQ );
        x /= l;
        y /= l;
        z /= l;
    }

    template <class T>
    void Vector3<T>::setLength( T newlength )
    {
        normalise();
        *this *= newlength;
    }

    template <class T>
    void Vector3<T>::invert()
    {
        x *= T( -1.0 );
        y *= T( -1.0 );
        z *= T( -1.0 );
    }

    template <class T>
    Vector3<T> Vector3<T>::getInterpolated( const Vector3<T> &other, const T d ) const
    {
        const T inv = static_cast<T>( 1.0 ) - d;
        return Vector3<T>( other.X() * inv + X() * d, other.Y() * inv + Y() * d,
                           other.Z() * inv + Z() * d );
    }

    template <class T>
    Vector3<T> Vector3<T>::getInterpolated_quadratic( const Vector3<T> &v2, const Vector3<T> &v3,
                                                      const T d ) const
    {
        // this*(1-d)*(1-d) + 2 * v2 * (1-d) + v3 * d * d;
        const T inv = static_cast<T>( 1.0 ) - d;
        const T mul0 = inv * inv;
        const T mul1 = static_cast<T>( 2.0 ) * d * inv;
        const T mul2 = d * d;

        return Vector3<T>( X() * mul0 + v2.X() * mul1 + v3.X() * mul2,
                           Y() * mul0 + v2.Y() * mul1 + v3.Y() * mul2,
                           Z() * mul0 + v2.Z() * mul1 + v3.Z() * mul2 );
    }

    template <class T>
    Vector3<T> Vector3<T>::getHorizontalAngle()
    {
        Vector3<T> angle;

        angle.Y() = Math<T>::ATan2( X(), Z() );
        angle.Y() *= Math<T>::rad_to_deg();

        if( angle.Y() < T( 0.0 ) )
        {
            angle.Y() += T( 360.0 );
        }

        if( angle.Y() >= T( 360.0 ) )
        {
            angle.Y() -= T( 360.0 );
        }

        auto z1 = Math<T>::Sqrt( X() * X() + Z() * Z() );

        angle.X() = Math<T>::ATan2( z1, Y() );
        angle.X() *= Math<T>::rad_to_deg();
        angle.X() -= T( 90.0 );

        if( angle.X() < T( 0.0 ) )
        {
            angle.X() += T( 360.0 );
        }

        if( angle.X() >= T( 360.0 ) )
        {
            angle.X() -= T( 360.0 );
        }

        return angle;
    }

    template <class T>
    void Vector3<T>::getAs4Values( T *array ) const
    {
        array[0] = x;
        array[1] = y;
        array[2] = z;
        array[3] = T( 0.0 );
    }

    template <class T>
    bool Vector3<T>::isZeroLength() const
    {
        auto sqlen = ( x * x ) + ( y * y ) + ( z * z );

        return sqlen < Math<T>::epsilon();
    }

    template <class T>
    s32 Vector3<T>::compare( const Vector3 &other ) const
    {
        return std::memcmp( ptr(), other.ptr(), 3 * sizeof( T ) );
    }

    template <class T>
    T *Vector3<T>::ptr()
    {
        return &x;
    }

    template <class T>
    const T *Vector3<T>::ptr() const
    {
        return &x;
    }

    template <class T>
    void Vector3<T>::makeAbs()
    {
        x = Math<T>::Abs( x );
        y = Math<T>::Abs( y );
        z = Math<T>::Abs( z );
    }

    template <class T>
    void Vector3<T>::makeFloor( const Vector3 &cmp )
    {
        x = std::min( x, cmp.x );
        y = std::min( y, cmp.y );
        z = std::min( z, cmp.z );
    }

    template <class T>
    void Vector3<T>::makeCeil( const Vector3<T> &cmp )
    {
        x = std::max( x, cmp.x );
        y = std::max( y, cmp.y );
        z = std::max( z, cmp.z );
    }

    template <class T>
    Vector3<T> Vector3<T>::perpendicular( void ) const
    {
        static const auto fSquareZero = static_cast<T>( 1e-06 * 1e-06 );

        auto perp = this->crossProduct( UNIT_X );

        // Check length
        if( perp.lengthSquared() < fSquareZero )
        {
            /* This vector is the Y axis multiplied by a scalar, so we have
               to use another axis.
            */
            perp = this->crossProduct( UNIT_Y );
        }

        perp.normalise();

        return perp;
    }

    template <class T>
    Vector3<T> Vector3<T>::midPoint( const Vector3<T> &vec ) const
    {
        return Vector3<T>( ( x + vec.x ) * T( 0.5 ), ( y + vec.y ) * T( 0.5 ),
                           ( z + vec.z ) * T( 0.5 ) );
    }

    template <class T>
    T Vector3<T>::distance( const Vector3<T> &v1, const Vector3<T> &v2 )
    {
        return ( v1 - v2 ).length();
    }

    template <class T>
    auto Vector3<T>::fromCoords( T radius, T latitude, T longitude ) -> Vector3<T>
    {
        auto xPos = radius * Math<T>::Cos( latitude ) * Math<T>::Cos( longitude );
        auto zPos = radius * Math<T>::Cos( latitude ) * Math<T>::Sin( longitude );
        auto yPos = radius * Math<T>::Sin( latitude );

        return Vector3( xPos, zPos, yPos );
    }

    template <class T>
    auto Vector3<T>::isValid() const -> bool
    {
        return isFinite();
    }

    template <class T>
    auto Vector3<T>::isFinite() const -> bool
    {
        return Math<T>::isFinite( x ) && Math<T>::isFinite( y ) && Math<T>::isFinite( z );
    }

    template <class T>
    auto Vector3<T>::operator==( const Vector3<T> &other ) const -> bool
    {
        const auto values = ptr();
        return Math<T>::equals( values[0], other[0] ) && Math<T>::equals( values[1], other[1] ) &&
               Math<T>::equals( values[2], other[2] );
    }

    template <class T>
    auto Vector3<T>::operator!=( const Vector3<T> &other ) const -> bool
    {
        const auto values = ptr();
        return !Math<T>::equals( values[0], other[0] ) || !Math<T>::equals( values[1], other[1] ) ||
               !Math<T>::equals( values[2], other[2] );
    }

    template <class T>
    void Vector3<T>::rotateYZBy( T degrees, const Vector3<T> &center )
    {
        degrees *= Math<T>::deg_to_rad();
        T cs = Math<T>::Cos( degrees );
        T sn = Math<T>::Sin( degrees );
        Z() -= center.Z();
        Y() -= center.Y();
        set( X(), Y() * cs - Z() * sn, Y() * sn + Z() * cs );
        Z() += center.Z();
        Y() += center.Y();
    }

    template <class T>
    void Vector3<T>::rotateXZBy( T degrees, const Vector3<T> &center )
    {
        degrees *= Math<T>::deg_to_rad();
        T cs = Math<T>::Cos( degrees );
        T sn = Math<T>::Sin( degrees );
        X() -= center.X();
        Z() -= center.Z();
        set( X() * cs - Z() * sn, Y(), X() * sn + Z() * cs );
        X() += center.X();
        Z() += center.Z();
    }

    template <class T>
    void Vector3<T>::rotateXYBy( T degrees, const Vector3<T> &center )
    {
        degrees *= Math<T>::deg_to_rad();
        T cs = Math<T>::Cos( degrees );
        T sn = Math<T>::Sin( degrees );
        X() -= center.X();
        Y() -= center.Y();
        set( X() * cs - Y() * sn, X() * sn + Y() * cs, Z() );
        X() += center.X();
        Y() += center.Y();
    }

    template <typename T>
    void Vector3<T>::generateComplementBasis( Vector3<T> &u, Vector3<T> &v, const Vector3<T> &w )
    {
        if( Math<T>::Abs( w[0] ) >= Math<T>::Abs( w[1] ) )
        {
            // W.x or W.z is the largest magnitude component, swap them
            auto invLength = Math<T>::SqrtInv( w[0] * w[0] + w[2] * w[2] );
            u[0] = -w[2] * invLength;
            u[1] = static_cast<T>( 0 );
            u[2] = +w[0] * invLength;
            v[0] = w[1] * u[2];
            v[1] = w[2] * u[0] - w[0] * u[2];
            v[2] = -w[1] * u[0];
        }
        else
        {
            // W.y or W.z is the largest magnitude component, swap them
            auto invLength = Math<T>::SqrtInv( w[1] * w[1] + w[2] * w[2] );
            u[0] = static_cast<T>( 0 );
            u[1] = +w[2] * invLength;
            u[2] = -w[1] * invLength;
            v[0] = w[1] * u[2] - w[2] * u[1];
            v[1] = -w[0] * u[2];
            v[2] = w[0] * u[1];
        }
    }

    template <class T>
    const Vector3<T> &Vector3<T>::zero()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( 0.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::unit()
    {
        static auto vec = Vector3<T>( T( 1.0 ), T( 1.0 ), T( 1.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::unitX()
    {
        static const auto vec = Vector3<T>( T( 1.0 ), T( 0.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::unitY()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( 1.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::unitZ()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( 0.0 ), T( 1.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::positiveX()
    {
        static const auto vec = Vector3<T>( T( 1.0 ), T( 0.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::negativeX()
    {
        static const auto vec = Vector3<T>( T( -1.0 ), T( 0.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::positiveY()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( 1.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::negativeY()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( -1.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::positiveZ()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( 0.0 ), T( 1.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::negativeZ()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( 0.0 ), T( -1.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::up()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( 1.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::down()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( -1.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::right()
    {
        static const auto vec = Vector3<T>( T( 1.0 ), T( 0.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::left()
    {
        static const auto vec = Vector3<T>( T( -1.0 ), T( 0.0 ), T( 0.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::back()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( 0.0 ), T( 1.0 ) );
        return vec;
    }

    template <class T>
    const Vector3<T> &Vector3<T>::forward()
    {
        static const auto vec = Vector3<T>( T( 0.0 ), T( 0.0 ), T( -1.0 ) );
        return vec;
    }

    // explicit instantiation
    template class Vector3<s32>;
    template class Vector3<f32>;
    template class Vector3<f64>;
}  // namespace workphone
