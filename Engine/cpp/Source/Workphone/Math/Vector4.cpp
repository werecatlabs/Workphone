#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    template <class T>
    Vector4<T>::Vector4() : x( 0 ), y( 0 ), z( 0 ), w( 0 )
    {
    }

    template <class T>
    Vector4<T>::Vector4( T x, T y, T z, T w ) : x( x ), y( y ), z( z ), w( w )
    {
    }

    template <class T>
    Vector4<T>::Vector4( const Vector4<T> &other )
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
    }

    template <class T>
    Vector4<T>::Vector4( const Vector3<T> &vec, T value ) :
        x( vec.X() ),
        y( vec.Y() ),
        z( vec.Z() ),
        w( value )
    {
    }

    template <class T>
    Vector4<T>::Vector4( const T *ptr )
    {
        x = ptr[0];
        y = ptr[1];
        z = ptr[2];
        w = ptr[3];
    }

    template <class T>
    T Vector4<T>::dotProduct( const Vector4<T> &other ) const
    {
        return T( 0.0 );
    }

    template <class T>
    Vector4<T> Vector4<T>::operator-() const
    {
        return Vector4<T>( -X(), -Y(), -Z(), -W() );
    }

    template <class T>
    Vector4<T> &Vector4<T>::operator=( const Vector4<T> &other )
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }

    template <class T>
    Vector4<T> Vector4<T>::operator+( const Vector4<T> &other ) const
    {
        return Vector4<T>( x + other.x, y + other.y, z + other.z, w + other.w );
    }

    template <class T>
    Vector4<T> &Vector4<T>::operator+=( const Vector4<T> &other )
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    template <class T>
    Vector4<T> Vector4<T>::operator-( const Vector4<T> &other ) const
    {
        return Vector4<T>( x - other.x, y - other.y, z - other.z, w - other.w );
    }

    template <class T>
    Vector4<T> &Vector4<T>::operator-=( const Vector4<T> &other )
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    template <class T>
    Vector4<T> Vector4<T>::operator*( const Vector4<T> &other ) const
    {
        return Vector4<T>( x * other.x, y * other.y, z * other.z, w * other.w );
    }

    template <class T>
    Vector4<T> &Vector4<T>::operator*=( const Vector4<T> &other )
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }

    template <class T>
    Vector4<T> Vector4<T>::operator*( const T v ) const
    {
        return Vector4<T>( x * v, y * v, z * v, w * v );
    }

    template <class T>
    Vector4<T> &Vector4<T>::operator*=( const T v )
    {
        x *= v;
        y *= v;
        z *= v;
        w *= v;
        return *this;
    }

    template <class T>
    Vector4<T> Vector4<T>::operator/( const Vector4<T> &other ) const
    {
        return Vector4<T>( x / other.x, y / other.y, z / other.z, w / other.w );
    }

    template <class T>
    Vector4<T> &Vector4<T>::operator/=( const Vector4<T> &other )
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;
        return *this;
    }

    template <class T>
    Vector4<T> Vector4<T>::operator/( const T v ) const
    {
        auto invValue = T( 1.0 ) / v;
        return Vector4<T>( x * invValue, y * invValue, z * invValue, w * invValue );
    }

    template <class T>
    Vector4<T> &Vector4<T>::operator/=( const T v )
    {
        auto invValue = T( 1.0 ) / v;
        x *= invValue;
        y *= invValue;
        z *= invValue;
        w *= invValue;
        return *this;
    }

    template <class T>
    bool Vector4<T>::operator<=( const Vector4<T> &other ) const
    {
        return compare( other ) <= 0;
    }

    template <class T>
    bool Vector4<T>::operator>=( const Vector4<T> &other ) const
    {
        return compare( other ) >= 0;
    }

    template <class T>
    bool Vector4<T>::operator<( const Vector4<T> &other ) const
    {
        return compare( other ) < 0;
    }

    template <class T>
    bool Vector4<T>::operator>( const Vector4<T> &other ) const
    {
        return compare( other ) > 0;
    }

    template <class T>
    bool Vector4<T>::operator==( const Vector4<T> &other ) const
    {
        return Math<T>::equals( X(), other.X() ) && Math<T>::equals( Y(), other.Y() ) &&
               Math<T>::equals( Z(), other.Z() ) && Math<T>::equals( W(), other.W() );
    }

    template <class T>
    bool Vector4<T>::operator!=( const Vector4<T> &other ) const
    {
        return !Math<T>::equals( X(), other.X() ) || !Math<T>::equals( Y(), other.Y() ) ||
               !Math<T>::equals( Z(), other.Z() ) || !Math<T>::equals( W(), other.W() );
    }

    template <class T>
    Vector4<T>::operator const T *() const
    {
        const auto p = ptr();
        return p;
    }

    template <class T>
    Vector4<T>::operator T *()
    {
        auto p = ptr();
        return p;
    }

    template <class T>
    T Vector4<T>::operator[]( s32 i ) const
    {
        const auto values = ptr();
        return values[i];
    }

    template <class T>
    T &Vector4<T>::operator[]( s32 i )
    {
        auto values = ptr();
        return values[i];
    }

    template <class T>
    T Vector4<T>::X() const
    {
        return x;
    }

    template <class T>
    T &Vector4<T>::X()
    {
        return x;
    }

    template <class T>
    T Vector4<T>::Y() const
    {
        return y;
    }

    template <class T>
    T &Vector4<T>::Y()
    {
        return y;
    }

    template <class T>
    T Vector4<T>::Z() const
    {
        return z;
    }

    template <class T>
    T &Vector4<T>::Z()
    {
        return z;
    }

    template <class T>
    T Vector4<T>::W() const
    {
        return w;
    }

    template <class T>
    T &Vector4<T>::W()
    {
        return w;
    }

    template <class T>
    bool Vector4<T>::equals( const Vector4<T> &other, [[maybe_unused]] const f32 tolerance ) const
    {
        return Math<T>::equals( x, other.x ) && Math<T>::equals( y, other.y ) &&
               Math<T>::equals( z, other.z ) && Math<T>::equals( w, other.w );
    }

    template <class T>
    s32 Vector4<T>::compare( const Vector4 &other ) const
    {
        return std::memcmp( ptr(), other.ptr(), 4 * sizeof( T ) );
    }

    template <class T>
    T *Vector4<T>::ptr()
    {
        return &x;
    }

    template <class T>
    const T *Vector4<T>::ptr() const
    {
        return &x;
    }

    template <class T>
    Vector4<T> Vector4<T>::zero()
    {
        return Vector4<T>( T( 0.0 ), T( 0.0 ), T( 0.0 ), T( 0.0 ) );
    }

    template <class T>
    bool Vector4<T>::isValid() const
    {
        return isFinite();
    }

    template <class T>
    bool Vector4<T>::isFinite() const
    {
        return Math<T>::isFinite( x ) && Math<T>::isFinite( y ) && Math<T>::isFinite( z ) &&
               Math<T>::isFinite( w );
    }

    template <class T>
    Vector3<T> Vector4<T>::xyz() const
    {
        return Vector3<T>( x, y, z );
    }

    template <>
    const Vector4<s32> Vector4<s32>::ZERO( 0, 0, 0, 0 );
    template <>
    const Vector4<s32> Vector4<s32>::UNIT_X( 1, 0, 0, 0 );
    template <>
    const Vector4<s32> Vector4<s32>::UNIT_Y( 0, 1, 0, 0 );
    template <>
    const Vector4<s32> Vector4<s32>::UNIT_Z( 0, 0, 1, 0 );
    template <>
    const Vector4<s32> Vector4<s32>::UNIT( 1, 1, 1, 1 );

    template <>
    const Vector4<f32> Vector4<f32>::ZERO( 0.0f, 0.0f, 0.0f, 0.0f );
    template <>
    const Vector4<f32> Vector4<f32>::UNIT_X( 1.0f, 0.0f, 0.0f, 0.0f );
    template <>
    const Vector4<f32> Vector4<f32>::UNIT_Y( 0.0f, 1.0f, 0.0f, 0.0f );
    template <>
    const Vector4<f32> Vector4<f32>::UNIT_Z( 0.0f, 0.0f, 1.0f, 0.0f );
    template <>
    const Vector4<f32> Vector4<f32>::UNIT( 1.0f, 1.0f, 1.0f, 1.0f );

    template <>
    const Vector4<f64> Vector4<f64>::ZERO( 0.0, 0.0, 0.0, 0.0 );
    template <>
    const Vector4<f64> Vector4<f64>::UNIT_X( 1.0, 0.0, 0.0, 0.0 );
    template <>
    const Vector4<f64> Vector4<f64>::UNIT_Y( 0.0, 1.0, 0.0, 0.0 );
    template <>
    const Vector4<f64> Vector4<f64>::UNIT_Z( 0.0, 0.0, 1.0, 0.0 );
    template <>
    const Vector4<f64> Vector4<f64>::UNIT( 1.0, 1.0, 1.0, 1.0 );

    // explicit instantiation
    template class Vector4<s32>;
    template class Vector4<f32>;
    template class Vector4<f64>;
}  // namespace workphone
