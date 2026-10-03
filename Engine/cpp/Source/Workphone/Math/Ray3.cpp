#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Ray3.hpp>

namespace workphone
{
    template <class T>
    Ray3<T>::Ray3() = default;

    template <class T>
    Ray3<T>::Ray3( const Vector3<T> &origin, const Vector3<T> &direction ) :
        m_origin( origin ),
        m_direction( direction )
    {
    }

    template <class T>
    Vector3<T> Ray3<T>::getOrigin() const
    {
        return m_origin;
    }

    template <class T>
    void Ray3<T>::setOrigin( const Vector3<T> &origin )
    {
        m_origin = origin;
    }

    template <class T>
    Vector3<T> Ray3<T>::getDirection() const
    {
        return m_direction;
    }

    template <class T>
    void Ray3<T>::setDirection( const Vector3<T> &direction )
    {
        m_direction = direction;
    }

    template <class T>
    bool Ray3<T>::isValid() const
    {
        return Math<T>::isFinite( m_origin.X() ) && Math<T>::isFinite( m_origin.Y() ) &&
               Math<T>::isFinite( m_origin.Z() ) && Math<T>::isFinite( m_direction.X() ) &&
               Math<T>::isFinite( m_direction.Y() ) && Math<T>::isFinite( m_direction.Z() );
    }

    // explicit instantiation
    template class Ray3<f32>;
    template class Ray3<f64>;
}  // namespace workphone
