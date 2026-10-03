#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/InterpolatorNonUniform3.hpp>

namespace workphone
{
    template <class T>
    WPCore_API InterpolatorNonUniform3<T>::InterpolatorNonUniform3() = default;

    template <class T>
    WPCore_API InterpolatorNonUniform3<T>::~InterpolatorNonUniform3() = default;

    template <class T>
    WPCore_API auto InterpolatorNonUniform3<T>::interpolate( const T &t ) -> Vector3<T>
    {
        if( m_values.empty() )
        {
            return Vector3<T>::ZERO;
        }

        if( t < T( 0.0 ) )
        {
            if( !m_values.empty() )
            {
                return m_values[0].second;
            }

            return Vector3<T>::ZERO;
        }

        if( t > T( 1.0 ) )
        {
            if( !m_values.empty() )
            {
                return m_values.back().second;
            }

            return Vector3<T>::ZERO;
        }

        Pair<T, Vector3<T>> pointA;
        Pair<T, Vector3<T>> pointB;

        bool bInterpolate = true;

        for( u32 i = 0; i < m_values.size(); ++i )
        {
            const Pair<T, Vector3<T>> &value = m_values[i];
            if( value.first >= t )
            {
                pointA = value;

                u32 nextIndex = i + 1;
                if( nextIndex <= m_values.size() )
                {
                    pointB = m_values[nextIndex];
                }
                else
                {
                    bInterpolate = false;
                }

                break;
            }
        }

        if( bInterpolate )
        {
            T time = t - pointA.first;
            return pointA.second + ( ( pointB.second - pointA.second ) * time );
        }

        return m_values.back().second;
    }

    template <class T>
    WPCore_API auto InterpolatorNonUniform3<T>::getValues() const -> const Array<Pair<T, Vector3<T>>> &
    {
        return m_values;
    }

    template <class T>
    void WPCore_API InterpolatorNonUniform3<T>::setValues( const Array<Pair<T, Vector3<T>>> &values )
    {
        m_values = values;
    }

    // explicit instantiation
    template class InterpolatorNonUniform3<f32>;
    template class InterpolatorNonUniform3<f64>;

}  // namespace workphone
