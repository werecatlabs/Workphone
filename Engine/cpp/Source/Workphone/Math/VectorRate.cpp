#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/VectorRate.hpp>

namespace workphone
{
    template <class T>
    VectorRate3<T>::VectorRate3() = default;

    template <class T>
    VectorRate3<T>::~VectorRate3() = default;

    template <class T>
    Vector3<T> VectorRate3<T>::calculateChange( const Pair<T, Vector3<T>> &t )
    {
        // Calculate the average time passed between events of the given type
        // during the last mFrameSmoothingTime seconds.
        m_eventTimes.push_back( t );

        if( m_eventTimes.size() == 1 )
            return Vector3<T>::zero();

        // Find the oldest time to keep
        auto it = m_eventTimes.begin();
        auto end = m_eventTimes.end() - 2;  // We need at least two times
        while( it != end )
        {
            if( m_eventTimes.front().first - ( *it ).first > m_interval )
                ++it;
            else
                break;
        }

        // Remove old times
        m_eventTimes.erase( m_eventTimes.begin(), it );

        return ( ( m_eventTimes.back() ).second - ( m_eventTimes.front() ).second ) /
               (T)( m_eventTimes.size() - 1 );
    }

    template <class T>
    T VectorRate3<T>::getInterval() const
    {
        return m_interval;
    }

    template <class T>
    void VectorRate3<T>::setInterval( T interval )
    {
        m_interval = interval;
    }

    template <class T>
    void VectorRate3<T>::clear()
    {
        m_eventTimes.clear();
    }

    // explicit instantiation
    template class VectorRate3<f32>;
    template class VectorRate3<f64>;

}  // namespace workphone
