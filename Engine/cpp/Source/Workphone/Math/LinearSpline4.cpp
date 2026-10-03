#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/LinearSpline4.hpp>

namespace workphone
{
    template <class T>
    LinearSpline4<T>::LinearSpline4()
    {
    }

    template <class T>
    LinearSpline4<T>::~LinearSpline4()
    {
    }

    template <class T>
    Vector4<T> LinearSpline4<T>::interpolate( T t )
    {
        // Perform linear interpolation
        if( m_points.size() < 2 )
            throw std::out_of_range( "LinearSpline4 requires at least 2 points for interpolation" );

        // Find the segment in which 't' lies
        auto numSegments = m_points.size() - 1;
        auto segmentWidth = static_cast<T>( 1.0 ) / numSegments;
        auto segmentIndex = static_cast<std::size_t>( t / segmentWidth );

        // Ensure 't' does not exceed the last segment
        if( segmentIndex >= numSegments )
            return m_points.back();

        // Calculate interpolation factor within the segment
        auto tInSegment = ( t - segmentIndex * segmentWidth ) / segmentWidth;

        // Perform linear interpolation between two points
        return m_points[segmentIndex] * ( 1 - tInSegment ) + m_points[segmentIndex + 1] * tInSegment;
    }

    template <class T>
    u32 LinearSpline4<T>::getNumPoints() const
    {
        return static_cast<u32>( m_points.size() );
    }

    template <class T>
    void LinearSpline4<T>::addPoint( const Vector4<T> &point )
    {
        m_points.push_back( point );
    }

    template <class T>
    Array<Vector4<T>> LinearSpline4<T>::getPoints() const
    {
        return m_points;
    }

    template <class T>
    void LinearSpline4<T>::setPoints( const Array<Vector4<T>> &points )
    {
        m_points = points;
    }

    // explicit instantiation
    template class LinearSpline4<f32>;
    template class LinearSpline4<f64>;
}  // namespace workphone
