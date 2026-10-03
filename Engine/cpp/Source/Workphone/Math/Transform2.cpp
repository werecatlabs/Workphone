#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Transform2.hpp>

namespace workphone
{
    template <class T>
    Transform2<T>::Transform2() = default;

    template <class T>
    Transform2<T>::Transform2( const Vector2<T> &position, T rotation ) :
        m_position( position ),
        m_rotation( rotation )
    {
    }

    template <class T>
    Transform2<T>::~Transform2() = default;

    template <class T>
    Vector2<T> Transform2<T>::getPosition() const
    {
        return m_position;
    }

    template <class T>
    void Transform2<T>::setPosition( const Vector2<T> &position )
    {
        m_position = position;
    }

    template <class T>
    T Transform2<T>::getRotation() const
    {
        return m_rotation;
    }

    template <class T>
    void Transform2<T>::setRotation( T rotation )
    {
        m_rotation = rotation;
    }

    // explicit instantiation
    template class Transform2<f32>;
    template class Transform2<f64>;
}  // namespace workphone
