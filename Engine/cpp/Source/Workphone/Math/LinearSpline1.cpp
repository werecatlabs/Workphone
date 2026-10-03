#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/LinearSpline1.hpp>

namespace workphone
{
    template <class T>
    LinearSpline1<T>::LinearSpline1()
    {
        // Set up matrix
        // Hermite polynomial
        m_coeffs[0][0] = 2;
        m_coeffs[0][1] = -2;
        m_coeffs[0][2] = 1;
        m_coeffs[0][3] = 1;
        m_coeffs[1][0] = -3;
        m_coeffs[1][1] = 3;
        m_coeffs[1][2] = -2;
        m_coeffs[1][3] = -1;
        m_coeffs[2][0] = 0;
        m_coeffs[2][1] = 0;
        m_coeffs[2][2] = 1;
        m_coeffs[2][3] = 0;
        m_coeffs[3][0] = 1;
        m_coeffs[3][1] = 0;
        m_coeffs[3][2] = 0;
        m_coeffs[3][3] = 0;
    }

    template <class T>
    LinearSpline1<T>::~LinearSpline1() = default;

    template <class T>
    void LinearSpline1<T>::setPoints( const Array<Pair<T, T>> &points )
    {
        for( auto &p : points )
        {
            m_times.push_back( p.first );
            m_points.push_back( Vector3<T>( p.second, 0.0, 0.0 ) );
        }
    }

    template <class T>
    void LinearSpline1<T>::setPoints( const Array<T> &keys, const Array<T> &points )
    {
        m_times = keys;

        for( auto &p : points )
        {
            m_points.push_back( Vector3<T>( p, 0.0, 0.0 ) );
        }
    }

    template <class T>
    void LinearSpline1<T>::setPoints( const Array<T> &points )
    {
        for( auto &p : points )
        {
            m_points.push_back( Vector3<T>( p, 0.0, 0.0 ) );
        }
    }

    template <class T>
    T LinearSpline1<T>::interpolate( T t )
    {
        // Work out which segment this is in
        auto fSeg = t * ( m_points.size() - 1 );
        auto segIdx = (u32)fSeg;

        // Apportion t
        auto p = fSeg - segIdx;

        return interpolate( segIdx, p );
    }

    template <class T>
    T LinearSpline1<T>::interpolate( u32 fromIndex, T t ) const
    {
        // Bounds check
        WP_ASSERT( fromIndex < m_points.size() && "fromIndex out of bounds" );

        if( ( fromIndex + 1 ) == m_points.size() )
        {
            // Duff request, cannot blend to nothing
            // Just return source
            return m_points[fromIndex].x;
        }

        // Fast special cases
        if( t == T( 0.0 ) )
        {
            return m_points[fromIndex].x;
        }
        else if( t == T( 1.0 ) )
        {
            return m_points[fromIndex + 1].x;
        }

        // Real interpolation
        // Form a vector of powers of t
        T t2, t3;
        t2 = t * t;
        t3 = t2 * t;

        Vector4<T> powers( t3, t2, t, 1 );

        // Algorithm is ret = powers * mCoeffs * Matrix4(point1, point2, tangent1, tangent2)
        const auto point1 = m_points[fromIndex];
        const auto point2 = m_points[fromIndex + 1];
        const auto tan1 = m_tangents[fromIndex];
        const auto tan2 = m_tangents[fromIndex + 1];
        auto pt = Matrix4<T>::identity();

        pt[0][0] = point1.X();
        pt[0][1] = point1.Y();
        pt[0][2] = point1.Z();
        pt[0][3] = T( 1.0 );
        pt[1][0] = point2.X();
        pt[1][1] = point2.Y();
        pt[1][2] = point2.Z();
        pt[1][3] = T( 1.0 );
        pt[2][0] = tan1.X();
        pt[2][1] = tan1.Y();
        pt[2][2] = tan1.Z();
        pt[2][3] = T( 1.0 );
        pt[3][0] = tan2.X();
        pt[3][1] = tan2.Y();
        pt[3][2] = tan2.Z();
        pt[3][3] = T( 1.0 );

        Vector4<T> ret = powers * m_coeffs * pt;
        return ret.X();
    }

    template <class T>
    u32 LinearSpline1<T>::getNumPoints() const
    {
        return (u32)m_points.size();
    }

    template <class T>
    void LinearSpline1<T>::clear()
    {
        m_points.clear();
    }

    template <class T>
    void LinearSpline1<T>::setAutoCalculate( bool autoCalc )
    {
        m_autoCalc = autoCalc;
    }

    template <class T>
    void LinearSpline1<T>::recalcTangents()
    {
        // Catmull-Rom approach
        //
        // tangent[i] = 0.5 * (point[i+1] - point[i-1])
        //
        // Assume endpoint tangents are parallel with line with neighbour

        size_t i, numPoints;
        bool isClosed;

        numPoints = m_points.size();
        if( numPoints < 2 )
        {
            // Can't do anything yet
            return;
        }

        // Closed or open?
        if( m_points[0] == m_points[numPoints - 1] )
        {
            isClosed = true;
        }
        else
        {
            isClosed = false;
        }

        m_tangents.resize( numPoints );

        for( i = 0; i < numPoints; ++i )
        {
            if( i == 0 )
            {
                // Special case start
                if( isClosed )
                {
                    // Use numPoints-2 since numPoints-1 is the last point and == [0]
                    m_tangents[i] = T( 0.5 ) * ( m_points[1] - m_points[numPoints - 2] );
                }
                else
                {
                    m_tangents[i] = T( 0.5 ) * ( m_points[1] - m_points[0] );
                }
            }
            else if( i == numPoints - 1 )
            {
                // Special case end
                if( isClosed )
                {
                    // Use same tangent as already calculated for [0]
                    m_tangents[i] = m_tangents[0];
                }
                else
                {
                    m_tangents[i] = T( 0.5 ) * ( m_points[i] - m_points[i - 1] );
                }
            }
            else
            {
                m_tangents[i] = T( 0.5 ) * ( m_points[i + 1] - m_points[i - 1] );
            }
        }
    }

    // explicit instantiation
    template class LinearSpline1<f32>;
    template class LinearSpline1<f64>;
}  // namespace workphone
