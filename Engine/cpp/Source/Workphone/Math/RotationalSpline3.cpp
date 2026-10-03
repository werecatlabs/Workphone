#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/RotationalSpline3.hpp>

namespace workphone
{
    template <class T>
    RotationalSpline3<T>::RotationalSpline3() = default;

    template <class T>
    RotationalSpline3<T>::~RotationalSpline3() = default;

    template <class T>
    void RotationalSpline3<T>::addPoint( const Quaternion<T> &p )
    {
        m_points.push_back( p );
        if( m_autoCalc )
        {
            recalcTangents();
        }
    }

    template <class T>
    auto RotationalSpline3<T>::interpolate( f32 t, bool useShortestPath ) -> Quaternion<T>
    {
        // Work out which segment this is in
        f32 fSeg = t * ( m_points.size() - 1 );
        auto segIdx = static_cast<unsigned>( fSeg );
        // Apportion t
        t = fSeg - segIdx;

        return interpolate( segIdx, t, useShortestPath );
    }

    template <class T>
    auto RotationalSpline3<T>::interpolate( u32 fromIndex, f32 t, bool useShortestPath ) -> Quaternion<T>
    {
        if( ( fromIndex + 1 ) == m_points.size() )
        {
            // Duff request, cannot blend to nothing
            // Just return source
            return m_points[fromIndex];
        }

        // Fast special cases
        if( t == 0.0f )
        {
            return m_points[fromIndex];
        }
        if( t == 1.0f )
        {
            return m_points[fromIndex + 1];
        }

        // f32 interpolation
        // Use squad using tangents we've already set up
        Quaternion<T> &p = m_points[fromIndex];
        Quaternion<T> &q = m_points[fromIndex + 1];
        Quaternion<T> &a = m_tangents[fromIndex];
        Quaternion<T> &b = m_tangents[fromIndex + 1];

        // NB interpolate to nearest rotation
        return Quaternion<T>::squad( t, p, a, b, q, useShortestPath );
    }

    template <class T>
    void RotationalSpline3<T>::recalcTangents()
    {
        // ShoeMake (1987) approach
        // Just like Catmull-Rom really, just more gnarly
        // And no, I don't understand how to derive this!
        //
        // let p = point[i], pInv = p.Inverse
        // tangent[i] = p * exp( -0.25 * ( log(pInv * point[i+1]) + log(pInv * point[i-1]) ) )
        //
        // Assume endpoint tangents are parallel with line with neighbour

        unsigned int i, numPoints;
        bool isClosed;

        numPoints = static_cast<unsigned>( m_points.size() );

        if( numPoints < 2 )
        {
            // Can't do anything yet
            return;
        }

        m_tangents.resize( numPoints );

        if( m_points[0] == m_points[numPoints - 1] )
        {
            isClosed = true;
        }
        else
        {
            isClosed = false;
        }

        Quaternion<T> invp, part1, part2, preExp;
        for( i = 0; i < numPoints; ++i )
        {
            Quaternion<T> &p = m_points[i];
            invp = p.inverse();

            if( i == 0 )
            {
                // special case start
                part1 = ( invp * m_points[i + 1] ).log();
                if( isClosed )
                {
                    // Use numPoints-2 since numPoints-1 == end == start == this one
                    part2 = ( invp * m_points[numPoints - 2] ).log();
                }
                else
                {
                    part2 = ( invp * p ).log();
                }
            }
            else if( i == numPoints - 1 )
            {
                // special case end
                if( isClosed )
                {
                    // Wrap to [1] (not [0], this is the same as end == this one)
                    part1 = ( invp * m_points[1] ).log();
                }
                else
                {
                    part1 = ( invp * p ).log();
                }
                part2 = ( invp * m_points[i - 1] ).log();
            }
            else
            {
                part1 = ( invp * m_points[i + 1] ).log();
                part2 = ( invp * m_points[i - 1] ).log();
            }

            preExp = T( -0.25 ) * ( part1 + part2 );
            m_tangents[i] = p * preExp.exp();
        }
    }

    template <class T>
    auto RotationalSpline3<T>::getPoint( unsigned short index ) const -> const Quaternion<T> &
    {
        WP_ASSERT( index < m_points.size() && "Point index is out of bounds!!" );

        return m_points[index];
    }

    template <class T>
    auto RotationalSpline3<T>::getNumPoints() const -> unsigned short
    {
        return static_cast<unsigned short>( m_points.size() );
    }

    template <class T>
    void RotationalSpline3<T>::clear()
    {
        m_points.clear();
        m_tangents.clear();
    }

    template <class T>
    void RotationalSpline3<T>::updatePoint( unsigned short index, const Quaternion<T> &value )
    {
        WP_ASSERT( index < m_points.size() && "Point index is out of bounds!!" );

        m_points[index] = value;
        if( m_autoCalc )
        {
            recalcTangents();
        }
    }

    template <class T>
    void RotationalSpline3<T>::setAutoCalculate( bool autoCalc )
    {
        m_autoCalc = autoCalc;
    }

    template <class T>
    bool RotationalSpline3<T>::getUseShortestRoute() const
    {
        return m_useShortestRoute;
    }

    template <class T>
    void RotationalSpline3<T>::setUseShortestRoute( bool useShortestRoute )
    {
        m_useShortestRoute = useShortestRoute;
    }

    // explicit instantiation
    template class RotationalSpline3<f32>;
    template class RotationalSpline3<f64>;

}  // namespace workphone
