#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Segment.hpp>

namespace workphone
{
    template <class T>
    Segment<T>::Segment( const Vector3<T> &start, const Vector3<T> &direction, T extent ) :
        m_start( start ),
        m_direction( direction ),
        m_extent( extent )
    {
    }

    template <class T>
    Vector3<T> Segment<T>::getStart() const
    {
        return m_start;
    }

    template <class T>
    void Segment<T>::setStart( const Vector3<T> &start )
    {
        m_start = start;
    }

    template <class T>
    Vector3<T> Segment<T>::getDirection() const
    {
        return m_direction;
    }

    template <class T>
    void Segment<T>::setDirection( const Vector3<T> &direction )
    {
        m_direction = direction;
    }

    template <class T>
    T Segment<T>::getExtent() const
    {
        return m_extent;
    }

    template <class T>
    void Segment<T>::setExtent( T extent )
    {
        m_extent = extent;
    }

    // explicit instantiation
    template class Segment<s32>;
    template class Segment<f32>;
    template class Segment<f64>;
}  // namespace workphone
