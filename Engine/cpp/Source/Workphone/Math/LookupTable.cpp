#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/LookupTable.hpp>
#include <Workphone/Math/Math.hpp>

namespace workphone
{

    template <class T>
    auto LookupTable<T>::interpolate( T t ) -> T
    {
        size_t p0_ = 0;
        size_t p1_ = 0;

        const auto numServoPoints = m_points.size();
        for( size_t pointIdx = 0; pointIdx < numServoPoints; ++pointIdx )
        {
            auto &p = m_points[pointIdx];

            if( t < p.first )
            {
                p1_ = pointIdx;
                p0_ = pointIdx - 1;
                break;
            }
        }

        auto p0 = static_cast<size_t>(
            Math<s32>::clamp( static_cast<s32>( p0_ ), 0, static_cast<s32>( m_points.size() ) ) );
        auto p1 = static_cast<size_t>(
            Math<s32>::clamp( static_cast<s32>( p1_ ), 0, static_cast<s32>( m_points.size() ) ) );

        auto &f0 = m_points[p0];
        auto &f1 = m_points[p1];

        if( p0 != p1 )
        {
            auto delta = ( t - f0.first ) / ( f1.first - f0.first );
            return f0.second + ( f1.second - f0.second ) * delta;
        }

        return f0.second;
    }

    template <class T>
    void LookupTable<T>::setPoints( const Array<T> &points )
    {
        // Clear existing points
        m_points.clear();
        m_points.reserve( points.size() );

        // Create key-value pairs with indices as keys (0, 1, 2, ...)
        for( size_t i = 0; i < points.size(); ++i )
        {
            m_points.emplace_back( static_cast<T>( i ), points[i] );
        }
    }

    template <class T>
    void LookupTable<T>::setPoints( const Array<T> &keys, const Array<T> &points )
    {
        // Ensure both arrays have the same size
        auto size = std::min( keys.size(), points.size() );

        // Reserve space for efficiency
        m_points.clear();
        m_points.reserve( size );

        // Combine keys and points into pairs
        for( size_t i = 0; i < size; ++i )
        {
            m_points.emplace_back( keys[i], points[i] );
        }
    }

    template <class T>
    void LookupTable<T>::setPoints( const Array<Pair<T, T>> &points )
    {
        m_points = points;
    }

    template <class T>
    LookupTable<T>::LookupTable() = default;

    template <class T>
    LookupTable<T>::~LookupTable() = default;

    // explicit instantiation
    template class LookupTable<s32>;
    template class LookupTable<f32>;
    template class LookupTable<f64>;

}  // namespace workphone
