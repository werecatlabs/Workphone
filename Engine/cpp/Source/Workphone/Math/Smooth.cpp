#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Smooth.hpp>

namespace workphone
{
    template <class T>
    Smooth<T>::Smooth() = default;

    template <class T>
    Smooth<T>::~Smooth() = default;

    template <class T>
    T Smooth<T>::getValue( const Pair<time_interval, T> &value )
    {
        m_values.push_back( value );

        if( m_values.size() == 1 )
        {
            return T();
        }

        auto it = m_values.begin(), end = m_values.end() - 2;
        while( it != end )
        {
            if( value.first - ( *it ).first > m_smoothInterval )
                ++it;
            else
                break;
        }

        m_values.erase( m_values.begin(), it );

        return ( m_values.back().second - m_values.front().second ) / ( m_values.size() - 1 );
    }

    template <class T>
    time_interval Smooth<T>::getSmoothInterval() const
    {
        return m_smoothInterval;
    }

    template <class T>
    void Smooth<T>::setSmoothInterval( time_interval smoothInterval )
    {
        m_smoothInterval = smoothInterval;
    }

    // explicit instantiation
    template class Smooth<f32>;
    template class Smooth<f64>;
}  // namespace workphone
