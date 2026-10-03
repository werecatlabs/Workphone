#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Cylinder3.hpp>

namespace workphone
{
    template <typename T>
    Cylinder3<T>::Cylinder3() = default;

    template <typename T>
    Cylinder3<T>::Cylinder3( const Line3<T> &axis, T radius, T height ) :
        m_axis( axis ),
        m_radius( radius ),
        m_height( height )
    {
    }

    template <typename T>
    Line3<T> Cylinder3<T>::getAxis() const
    {
        return m_axis;
    }

    template <typename T>
    void Cylinder3<T>::setAxis( const Line3<T> &axis )
    {
        m_axis = axis;
    }

    template <typename T>
    T Cylinder3<T>::getRadius() const
    {
        return m_radius;
    }

    template <typename T>
    void Cylinder3<T>::setRadius( T radius )
    {
        m_radius = radius;
    }

    template <typename T>
    T Cylinder3<T>::getHeight() const
    {
        return m_height;
    }

    template <typename T>
    void Cylinder3<T>::setHeight( T height )
    {
        m_height = height;
    }

    // explicit instantiation
    template class Cylinder3<s32>;
    template class Cylinder3<f32>;
    template class Cylinder3<f64>;
}  // namespace workphone
