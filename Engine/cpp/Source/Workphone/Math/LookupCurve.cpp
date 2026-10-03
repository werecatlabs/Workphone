#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/LookupCurve.hpp>

namespace workphone
{
    template <class T>
    LookupCurve<T>::LookupCurve() = default;

    template <class T>
    LookupCurve<T>::~LookupCurve() = default;

    template <class T>
    void LookupCurve<T>::setPoints( const Array<Pair<T, T>> &points )
    {
        // Clear existing points
        m_points.clear();
        m_points.reserve( points.size() );

        // Convert to real_Num pairs
        for( size_t i = 0; i < points.size(); ++i )
        {
            m_points.emplace_back( static_cast<real_Num>( points[i].first ),
                                   static_cast<real_Num>( points[i].second ) );
        }
    }

    template <class T>
    void LookupCurve<T>::setPoints( const Array<T> &keys, const Array<T> &points )
    {
        // Ensure both arrays have the same size
        auto size = std::min( keys.size(), points.size() );

        // Reserve space for efficiency
        m_points.clear();
        m_points.reserve( size );

        // Combine keys and points into pairs
        for( size_t i = 0; i < size; ++i )
        {
            m_points.emplace_back( static_cast<real_Num>( keys[i] ),
                                   static_cast<real_Num>( points[i] ) );
        }
    }

    template <class T>
    void LookupCurve<T>::setPoints( const Array<T> &points )
    {
        // Clear existing points
        m_points.clear();
        m_points.reserve( points.size() );

        // Create key-value pairs with indices as keys (0, 1, 2, ...)
        for( size_t i = 0; i < points.size(); ++i )
        {
            m_points.emplace_back( static_cast<real_Num>( i ), static_cast<real_Num>( points[i] ) );
        }
    }

    template <class T>
    auto LookupCurve<T>::interpolate( T t ) -> T
    {
        // Handle empty or single point cases
        if( m_points.size() == 0 )
            return T( 0 );

        if( m_points.size() == 1 )
            return static_cast<T>( m_points[0].second );

        const auto tReal = static_cast<real_Num>( t );

        // Handle boundary cases
        if( tReal <= m_points[0].first )
            return static_cast<T>( m_points[0].second );

        if( tReal >= m_points[m_points.size() - 1].first )
            return static_cast<T>( m_points[m_points.size() - 1].second );

        // Find the segment containing t
        size_t p1 = 0;
        size_t p2 = 0;

        const auto numPoints = m_points.size();
        for( size_t pointIdx = 0; pointIdx < numPoints; ++pointIdx )
        {
            if( tReal < m_points[pointIdx].first )
            {
                p2 = pointIdx;
                p1 = pointIdx - 1;
                break;
            }
        }

        // For 2 points, use linear interpolation
        if( numPoints == 2 )
        {
            auto &f0 = m_points[p1];
            auto &f1 = m_points[p2];

            auto delta = ( tReal - f0.first ) / ( f1.first - f0.first );
            return static_cast<T>( f0.second + ( f1.second - f0.second ) * delta );
        }

        // For 3+ points, use Catmull-Rom cubic interpolation
        // Get the 4 control points (p0, p1, p2, p3)
        size_t p0 = ( p1 > 0 ) ? p1 - 1 : p1;
        size_t p3 = ( p2 < numPoints - 1 ) ? p2 + 1 : p2;

        auto &point0 = m_points[p0];
        auto &point1 = m_points[p1];
        auto &point2 = m_points[p2];
        auto &point3 = m_points[p3];

        // Normalize t to [0, 1] within the segment
        real_Num normalizedT = ( tReal - point1.first ) / ( point2.first - point1.first );

        // Catmull-Rom spline interpolation
        // Using the standard Catmull-Rom formula with tension = 0.5
        real_Num t2 = normalizedT * normalizedT;
        real_Num t3 = t2 * normalizedT;

        real_Num v0 = point0.second;
        real_Num v1 = point1.second;
        real_Num v2 = point2.second;
        real_Num v3 = point3.second;

        real_Num result = static_cast<real_Num>( 0.5 ) * ( ( 2.0 * v1 ) + ( -v0 + v2 ) * normalizedT +
                                                           ( 2.0 * v0 - 5.0 * v1 + 4.0 * v2 - v3 ) * t2 +
                                                           ( -v0 + 3.0 * v1 - 3.0 * v2 + v3 ) * t3 );

        return static_cast<T>( result );
    }

    // explicit instantiation
    template class LookupCurve<s32>;
    template class LookupCurve<f32>;
    template class LookupCurve<f64>;
}  // namespace workphone
