#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Sphere2.hpp>

namespace workphone
{
    template <class T>
    Sphere2<T>::Sphere2() = default;

    template <class T>
    Sphere2<T>::Sphere2( const Vector2<T> &center, const f32 radius ) :
        m_center( center ),
        m_radius( radius )
    {
    }

    template <class T>
    Sphere2<T>::Sphere2( const Sphere2<T> &other ) :
        m_center( other.getCenter() ),
        m_radius( other.getRadius() )
    {
    }

    template <class T>
    Sphere2<T> &Sphere2<T>::operator=( const Sphere2<T> &other )
    {
        m_center = other.getCenter();
        m_radius = other.getRadius();
        return *this;
    }

    template <class T>
    bool Sphere2<T>::operator==( const Sphere2<T> &other ) const
    {
        return ( m_center == other.getCenter() && m_radius == other.getRadius() );
    }

    template <class T>
    bool Sphere2<T>::operator!=( const Sphere2<T> &other ) const
    {
        return !( m_center == other.getCenter() && m_radius == other.getRadius() );
    }

    template <class T>
    Vector2<T> Sphere2<T>::getCenter() const
    {
        return m_center;
    }

    template <class T>
    void Sphere2<T>::setCenter( Vector2<T> center )
    {
        m_center = center;
    }

    template <class T>
    T Sphere2<T>::getRadius() const
    {
        return m_radius;
    }

    template <class T>
    void Sphere2<T>::setRadius( T radius )
    {
        m_radius = radius;
    }

    template <class T>
    bool Sphere2<T>::intersects( const Sphere2<T> &other ) const
    {
        Vector2<T> vec = m_center - other.getCenter();
        T radii = m_radius + other.getRadius();
        if( vec.length() > radii )
        {
            return false;
        }

        return true;
    }

    template <class T>
    bool Sphere2<T>::intersectsSQ( const Sphere2<T> &other ) const
    {
        Vector2<T> vec = m_center - other.getCenter();
        T radii = m_radius + other.getRadius();
        if( vec.lengthSquared() > ( radii * radii ) )
        {
            return false;
        }

        return true;
    }

    // explicit instantiation
    template class Sphere2<s32>;
    template class Sphere2<f32>;
    template class Sphere2<f64>;
}  // namespace workphone
