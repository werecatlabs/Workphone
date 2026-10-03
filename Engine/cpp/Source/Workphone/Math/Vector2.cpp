#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    template <>
    const Vector2<s32> Vector2<s32>::ZERO( 0, 0 );
    template <>
    const Vector2<s32> Vector2<s32>::UNIT_X( 1, 0 );
    template <>
    const Vector2<s32> Vector2<s32>::UNIT_Y( 0, 1 );
    template <>
    const Vector2<s32> Vector2<s32>::UNIT( 1, 1 );

    template <>
    const Vector2<f32> Vector2<f32>::ZERO( 0.0f, 0.0f );
    template <>
    const Vector2<f32> Vector2<f32>::UNIT_X( 1.0f, 0.0f );
    template <>
    const Vector2<f32> Vector2<f32>::UNIT_Y( 0.0f, 1.0f );
    template <>
    const Vector2<f32> Vector2<f32>::UNIT( 1.0f, 1.0f );

    template <>
    const Vector2<f64> Vector2<f64>::ZERO( 0.0, 0.0 );
    template <>
    const Vector2<f64> Vector2<f64>::UNIT_X( 1.0, 0.0 );
    template <>
    const Vector2<f64> Vector2<f64>::UNIT_Y( 0.0, 1.0 );
    template <>
    const Vector2<f64> Vector2<f64>::UNIT( 1.0, 1.0 );

    template <class T>
    Vector2<T>::Vector2() : x( 0 ), y( 0 )
    {
    }

    template <class T>
    Vector2<T>::Vector2( T x, T y ) : x( x ), y( y )
    {
        WP_ASSERT( Math<T>::isFinite( x ) );
        WP_ASSERT( Math<T>::isFinite( y ) );
    }

    template <class T>
    Vector2<T>::Vector2( const Vector2<T> &other )
    {
        WP_ASSERT( other.isValid() );

        x = other.x;
        y = other.y;
    }

    template <class T>
    Vector2<T> Vector2<T>::operator-() const
    {
        WP_ASSERT( isValid() );
        return Vector2<T>( -x, -y );
    }

    template <class T>
    Vector2<T> &Vector2<T>::operator=( const Vector2<T> &other )
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        x = other.x;
        y = other.y;
        return *this;
    }

    template <class T>
    Vector2<T> Vector2<T>::operator+( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return Vector2<T>( x + other.x, y + other.y );
    }

    template <class T>
    Vector2<T> &Vector2<T>::operator+=( const Vector2<T> &other )
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        x += other.x;
        y += other.y;
        return *this;
    }

    template <class T>
    Vector2<T> Vector2<T>::operator-( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return Vector2<T>( x - other.x, y - other.y );
    }

    template <class T>
    Vector2<T> &Vector2<T>::operator-=( const Vector2<T> &other )
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        x -= other.x;
        y -= other.y;
        return *this;
    }

    template <class T>
    Vector2<T> Vector2<T>::operator*( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return Vector2<T>( x * other.x, y * other.y );
    }

    template <class T>
    Vector2<T> &Vector2<T>::operator*=( const Vector2<T> &other )
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        x *= other.x;
        y *= other.y;
        return *this;
    }

    template <class T>
    Vector2<T> Vector2<T>::operator*( const T v ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( Math<T>::isFinite( v ) );

        return Vector2<T>( x * v, y * v );
    }

    template <class T>
    Vector2<T> &Vector2<T>::operator*=( const T v )
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( Math<T>::isFinite( v ) );

        x *= v;
        y *= v;
        return *this;
    }

    template <class T>
    Vector2<T> Vector2<T>::operator/( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return Vector2<T>( x / other.x, y / other.y );
    }

    template <class T>
    Vector2<T> &Vector2<T>::operator/=( const Vector2<T> &other )
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        x /= other.x;
        y /= other.y;
        return *this;
    }

    template <class T>
    Vector2<T> Vector2<T>::operator/( const T v ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( Math<T>::isFinite( v ) );

        return Vector2<T>( x / v, y / v );
    }

    template <class T>
    Vector2<T> &Vector2<T>::operator/=( const T v )
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( Math<T>::isFinite( v ) );

        x /= v;
        y /= v;
        return *this;
    }

    template <class T>
    bool Vector2<T>::operator<=( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return compare( other ) <= 0;
    }

    template <class T>
    bool Vector2<T>::operator>=( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return compare( other ) >= 0;
    }

    template <class T>
    bool Vector2<T>::operator<( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return compare( other ) < 0;
    }

    template <class T>
    bool Vector2<T>::operator>( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return compare( other ) > 0;
    }

    template <class T>
    bool Vector2<T>::operator==( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return Math<T>::equals( x, other.x ) && Math<T>::equals( y, other.y );
    }

    template <class T>
    bool Vector2<T>::operator!=( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return !Math<T>::equals( x, other.x ) || !Math<T>::equals( y, other.y );
    }

    template <class T>
    Vector2<T>::operator const T *() const
    {
        return &x;
    }

    template <class T>
    Vector2<T>::operator T *()
    {
        return &x;
    }

    template <class T>
    T &Vector2<T>::operator[]( s32 i )
    {
        WP_ASSERT( i < 2 );
        auto values = ptr();
        return values[i];
    }

    template <class T>
    T Vector2<T>::operator[]( s32 i ) const
    {
        WP_ASSERT( i < 2 );
        auto values = ptr();
        return values[i];
    }

    template <class T>
    T Vector2<T>::X() const
    {
        return x;
    }

    template <class T>
    T &Vector2<T>::X()
    {
        return x;
    }

    template <class T>
    T Vector2<T>::Y() const
    {
        return y;
    }

    template <class T>
    T &Vector2<T>::Y()
    {
        return y;
    }

    template <class T>
    bool Vector2<T>::equals( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return Math<T>::equals( x, other.x ) && Math<T>::equals( y, other.y );
    }

    template <class T>
    void Vector2<T>::set( T nx, T ny )
    {
        WP_ASSERT( Math<T>::isFinite( nx ) );
        WP_ASSERT( Math<T>::isFinite( ny ) );
        WP_ASSERT( isValid() );

        x = nx;
        y = ny;
    }

    template <class T>
    void Vector2<T>::set( const Vector2<T> &p )
    {
        WP_ASSERT( p.isValid() );
        x = p.x;
        y = p.y;
    }

    template <class T>
    T Vector2<T>::length() const
    {
        WP_ASSERT( isValid() );
        return Math<T>::Sqrt( x * x + y * y );
    }

    template <class T>
    T Vector2<T>::lengthSquared() const
    {
        WP_ASSERT( isValid() );
        return x * x + y * y;
    }

    template <class T>
    T Vector2<T>::dotProduct( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        return x * other.x + y * other.y;
    }

    template <class T>
    T Vector2<T>::getDistanceFrom( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        auto d = Vector2<T>( x - other.x, y - other.y );
        return d.length();
    }

    template <class T>
    T Vector2<T>::getDistanceFromSQ( const Vector2<T> &other ) const
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( other.isValid() );

        auto d = Vector2<T>( x - other.x, y - other.y );
        return d.lengthSquared();
    }

    template <class T>
    void Vector2<T>::rotateBy( T degrees, const Vector2<T> &center )
    {
        degrees *= Math<T>::deg_to_rad();
        auto cs = Math<T>::Cos( degrees );
        auto sn = Math<T>::Sin( degrees );

        X() -= center.X();
        Y() -= center.Y();

        set( X() * cs - Y() * sn, X() * sn + Y() * cs );

        X() += center.X();
        Y() += center.Y();
    }

    template <class T>
    void Vector2<T>::normalise()
    {
        auto lengthSQ = lengthSquared();
        if( lengthSQ < Math<T>::epsilon() )
        {
            return;
        }

        auto l = Math<T>::SqrtInv( lengthSQ );
        x *= l;
        y *= l;
    }

    template <class T>
    T Vector2<T>::normaliseLength()
    {
        auto lengthSQ = lengthSquared();
        if( lengthSQ < Math<T>::epsilon() )
        {
            return T( 0.0 );
        }

        auto l = Math<T>::SqrtInv( lengthSQ );
        x *= l;
        y *= l;

        return T( 1.0 ) / l;
    }

    template <class T>
    Vector2<T> Vector2<T>::normaliseCopy() const
    {
        auto lengthSQ = lengthSquared();
        if( lengthSQ < Math<T>::epsilon() )
        {
            return *this;
        }

        auto l = Math<T>::SqrtInv( lengthSQ );
        return Vector2<T>( x * l, y * l );
    }

    template <class T>
    void Vector2<T>::setLength( T newlength )
    {
        normalise();
        *this *= newlength;
    }

    template <class T>
    Vector2<T> Vector2<T>::perp() const
    {
        return Vector2<T>( y, -x );
    }

    template <class T>
    Vector2<T> Vector2<T>::unitPerp() const
    {
        Vector2<T> perp( y, -x );
        perp.normalise();
        return perp;
    }

    template <class T>
    T Vector2<T>::dotPerp( const Vector2<T> &other ) const
    {
        return x * other.y - y * other.x;
    }

    template <class T>
    T Vector2<T>::getAngleTrig() const
    {
        if( Math<T>::equals( X(), T( 0.0 ) ) )
        {
            return Y() < T( 0.0 ) ? T( 270.0 ) : T( 90.0 );
        }

        if( Math<T>::equals( Y(), T( 0.0 ) ) )
        {
            return X() < T( 0.0 ) ? T( 180.0 ) : T( 0.0 );
        }

        if( Y() > T( 0.0 ) )
        {
            if( X() > T( 0.0 ) )
            {
                return Math<T>::Atan( Y() / X() ) * Math<T>::rad_to_deg();
            }

            return T( 180.0 ) - Math<T>::Atan( Y() / -X() ) * Math<T>::rad_to_deg();
        }

        if( X() > T( 0.0 ) )
        {
            return T( 360.0 ) - Math<T>::Atan( -Y() / X() ) * Math<T>::rad_to_deg();
        }

        return T( 180.0 ) + Math<T>::Atan( -Y() / -X() ) * Math<T>::rad_to_deg();
    }

    template <class T>
    T Vector2<T>::getAngle() const
    {
        if( Math<T>::equals( Y(), T( 0.0 ) ) )
        {
            return X() < T( 0.0 ) ? T( 180.0 ) : T( 0.0 );
        }

        if( Math<T>::equals( X(), T( 0.0 ) ) )
        {
            return Y() < T( 0.0 ) ? T( 90.0 ) : T( 270.0 );
        }

        auto tmp = Y() / length();
        tmp = Math<T>::Atan( Math<T>::Sqrt( T( 1.0 ) - tmp * tmp ) / tmp ) * Math<T>::rad_to_deg();

        if( X() > T( 0.0 ) && Y() > T( 0.0 ) )
        {
            return tmp + T( 270.0 );
        }

        if( X() > T( 0.0 ) && Y() < T( 0.0 ) )
        {
            return tmp + T( 90.0 );
        }

        if( X() < 0.0 && Y() < 0.0 )
        {
            return T( 90.0 ) - tmp;
        }

        if( X() < T( 0.0 ) && Y() > T( 0.0 ) )
        {
            return T( 270.0 ) - tmp;
        }

        return tmp;
    }

    template <class T>
    T Vector2<T>::getAngleWith( const Vector2<T> &b ) const
    {
        auto tmp = X() * b.X() + Y() * b.Y();

        if( Math<T>::equals( tmp, T( 0.0 ) ) )
        {
            return T( 90.0 );
        }

        auto thisValue = ( X() * X() + Y() * Y() );
        auto otherValue = ( b.X() * b.X() + b.Y() * b.Y() );
        auto sqrtValue = Math<T>::Sqrt( thisValue * otherValue );

        tmp = tmp / sqrtValue;

        if( tmp < T( 0.0 ) )
        {
            tmp = -tmp;
        }

        return Math<T>::Atan( Math<T>::Sqrt( T( 1.0 ) - tmp * tmp ) / tmp ) * Math<T>::rad_to_deg();
    }

    template <class T>
    bool Vector2<T>::isBetweenPoints( const Vector2<T> &begin, const Vector2<T> &end ) const
    {
        auto f = ( end - begin ).lengthSquared();
        return getDistanceFromSQ( begin ) < f && getDistanceFromSQ( end ) < f;
    }

    template <class T>
    Vector2<T> Vector2<T>::getInterpolated( const Vector2<T> &other, T d ) const
    {
        auto inv = T( 1.0 ) - d;
        return Vector2<T>( other.X() * inv + X() * d, other.Y() * inv + Y() * d );
    }

    template <class T>
    Vector2<T> Vector2<T>::getInterpolated_quadratic( const Vector2<T> &v2, const Vector2<T> &v3,
                                                      const T d ) const
    {
        // this*(1-d)*(1-d) + 2 * v2 * (1-d) + v3 * d * d;
        const auto inv = T( 1.0 ) - d;
        const auto mul0 = inv * inv;
        const auto mul1 = T( 2.0 ) * d * inv;
        const auto mul2 = d * d;

        return Vector2<T>( X() * mul0 + v2.X() * mul1 + v3.X() * mul2,
                           Y() * mul0 + v2.Y() * mul1 + v3.Y() * mul2 );
    }

    template <class T>
    void Vector2<T>::interpolate( const Vector2<T> &a, const Vector2<T> &b, const T t )
    {
        X() = b.X() + ( ( a.X() - b.X() ) * t );
        Y() = b.Y() + ( ( a.Y() - b.Y() ) * t );
    }

    template <class T>
    s32 Vector2<T>::compare( const Vector2 &other ) const
    {
        return std::memcmp( ptr(), other.ptr(), 2 * sizeof( T ) );
    }

    template <class T>
    T *Vector2<T>::ptr()
    {
        return &x;
    }

    template <class T>
    const T *Vector2<T>::ptr() const
    {
        return &x;
    }

    template <class T>
    Vector2<T> Vector2<T>::zero()
    {
        return Vector2<T>( T( 0.0 ), T( 0.0 ) );
    }

    template <class T>
    Vector2<T> Vector2<T>::unit()
    {
        return Vector2<T>( T( 1.0 ), T( 1.0 ) );
    }

    template <class T>
    bool Vector2<T>::isValid() const
    {
        return isFinite();
    }

    template <class T>
    bool Vector2<T>::isFinite() const
    {
        return Math<T>::isFinite( x ) && Math<T>::isFinite( y );
    }

    template class Vector2<s32>;
    template class Vector2<f32>;
    template class Vector2<f64>;
}  // namespace workphone
