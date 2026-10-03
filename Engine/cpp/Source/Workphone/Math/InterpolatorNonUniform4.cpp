#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/InterpolatorNonUniform4.hpp>

namespace workphone
{
    template <class T>
    WPCore_API InterpolatorNonUniform4<T>::InterpolatorNonUniform4() = default;

    template <class T>
    WPCore_API InterpolatorNonUniform4<T>::~InterpolatorNonUniform4() = default;

    template <class T>
    void WPCore_API InterpolatorNonUniform4<T>::setValues( const Array<Pair<T, Vector4<T>>> &values )
    {
        m_values = values;
    }

    template <class T>
    WPCore_API const Array<Pair<T, Vector4<T>>> &InterpolatorNonUniform4<T>::getValues() const
    {
        return m_values;
    }

    template <class T>
    auto WPCore_API InterpolatorNonUniform4<T>::interpolate( const T &t ) -> Vector4<T>
    {
        if( m_values.empty() )
        {
            return Vector4<T>::ZERO;
        }

        if( t < T( 0.0 ) )
        {
            if( !m_values.empty() )
            {
                return m_values[0].second;
            }

            return Vector4<T>::ZERO;
        }

        if( t > T( 1.0 ) )
        {
            if( !m_values.empty() )
            {
                return m_values.back().second;
            }

            return Vector4<T>::ZERO;
        }

        Pair<T, Vector4<T>> pointA;
        Pair<T, Vector4<T>> pointB;

        bool bInterpolate = true;

        for( u32 i = 0; i < m_values.size(); ++i )
        {
            const Pair<T, Vector4<T>> &value = m_values[i];
            if( value.first >= t )
            {
                pointA = value;

                u32 nextIndex = i + 1;
                if( nextIndex < m_values.size() )
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

    // explicit instantiation
    template class InterpolatorNonUniform4<f32>;
    template class InterpolatorNonUniform4<f64>;

}  // namespace workphone
