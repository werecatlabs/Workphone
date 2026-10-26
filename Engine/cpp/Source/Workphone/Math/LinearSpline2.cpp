#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/LinearSpline2.hpp>

namespace workphone
{
    template <class T>
    LinearSpline2<T>::LinearSpline2()
    {
        // Set up Hermite polynomial coefficient matrix
        // Hermite basis functions for cubic interpolation:
        // H₀(t) = 2t³ - 3t² + 1 (first point)
        // H₁(t) = -2t³ + 3t² (second point)
        // H₂(t) = t³ - 2t² + t (first tangent)
        // H₃(t) = t³ - t² (second tangent)
        m_coeffs[0][0] = 2;   // t³ coefficient for H₀
        m_coeffs[0][1] = -2;  // t³ coefficient for H₁
        m_coeffs[0][2] = 1;   // t³ coefficient for H₂
        m_coeffs[0][3] = 1;   // t³ coefficient for H₃
        m_coeffs[1][0] = -3;  // t² coefficient for H₀
        m_coeffs[1][1] = 3;   // t² coefficient for H₁
        m_coeffs[1][2] = -2;  // t² coefficient for H₂
        m_coeffs[1][3] = -1;  // t² coefficient for H₃
        m_coeffs[2][0] = 0;   // t¹ coefficient for H₀
        m_coeffs[2][1] = 0;   // t¹ coefficient for H₁
        m_coeffs[2][2] = 1;   // t¹ coefficient for H₂
        m_coeffs[2][3] = 0;   // t¹ coefficient for H₃
        m_coeffs[3][0] = 1;   // t⁰ coefficient for H₀
        m_coeffs[3][1] = 0;   // t⁰ coefficient for H₁
        m_coeffs[3][2] = 0;   // t⁰ coefficient for H₂
        m_coeffs[3][3] = 0;   // t⁰ coefficient for H₃
    }

    template <class T>
    LinearSpline2<T>::~LinearSpline2() = default;

    template <class T>
    LinearSpline2<T>::LinearSpline2( const LinearSpline2 &other ) :
        ISharedObject(),
        m_autoCalc( other.m_autoCalc ),
        m_times( makeSourceBudgetSnapshot( other.m_times ) ),
        m_points( makeSourceBudgetSnapshot( other.m_points ) ),
        m_tangents( makeSourceBudgetSnapshot( other.m_tangents ) ),
        m_coeffs( other.m_coeffs )
    {
    }

    template <class T>
    LinearSpline2<T> &LinearSpline2<T>::operator=( const LinearSpline2 &other )
    {
        if( this != &other )
        {
            auto times = makeSourceBudgetSnapshot( other.m_times );
            auto points = makeSourceBudgetSnapshot( other.m_points );
            auto tangents = makeSourceBudgetSnapshot( other.m_tangents );
            m_times.swap( times );
            m_points.swap( points );
            m_tangents.swap( tangents );
            m_autoCalc = other.m_autoCalc;
            m_coeffs = other.m_coeffs;
        }
        return *this;
    }

    template <class T>
    void LinearSpline2<T>::unload( SmartPtr<ISharedObject> data )
    {
        m_times = makeSourceBudgetSnapshot<T>();
        m_points = makeSourceBudgetSnapshot<Vector3<T>>();
        m_tangents = makeSourceBudgetSnapshot<Vector3<T>>();
    }

    namespace
    {
        template <class T>
        Array<Vector3<T>> prepareLinear2Tangents( const Array<Vector3<T>> &tangents, size_t count,
                                                  bool autoCalculate )
        {
            auto result = makeSourceBudgetSnapshot<Vector3<T>>();
            if( autoCalculate && count >= 2 && count > tangents.capacity() )
            {
                result.resize( count );
            }
            return result;
        }

        template <class T, class U, class Convert>
        Array<Vector3<T>> appendLinear2Points( const Array<Vector3<T>> &current, const Array<U> &input,
                                               Convert convert )
        {
            auto result = makeSourceBudgetSnapshot<Vector3<T>>();
            if( input.capacity() > result.max_size() - current.size() )
            {
                throw std::length_error( "Spline point capacity overflow" );
            }
            result.reserve( std::max( current.capacity(), current.size() + input.capacity() ) );
            for( const auto &point : current )
            {
                result.push_back( point );
            }
            for( const auto &value : input )
            {
                result.push_back( convert( value ) );
            }
            return result;
        }
    }  // namespace

    template <class T>
    void LinearSpline2<T>::setPoints( const Array<Pair<T, Vector2<T>>> &points )
    {
        auto times = makeSourceBudgetSnapshot<T>();
        if( points.capacity() > times.max_size() - m_times.size() )
        {
            throw std::length_error( "Spline time capacity overflow" );
        }
        times.reserve( std::max( m_times.capacity(), m_times.size() + points.capacity() ) );
        for( const auto &time : m_times )
        {
            times.push_back( time );
        }
        for( const auto &p : points )
        {
            times.push_back( p.first );
        }
        auto positions = appendLinear2Points( m_points, points, []( const auto &p ) {
            return Vector3<T>( p.second.x, p.second.y, T( 0 ) );
        } );
        auto tangents = prepareLinear2Tangents( m_tangents, positions.size(), m_autoCalc );
        m_times.swap( times );
        m_points.swap( positions );
        if( !tangents.empty() )
        {
            m_tangents.swap( tangents );
        }
        if( m_autoCalc )
        {
            recalcTangents();
        }
    }

    template <class T>
    void LinearSpline2<T>::setPoints( const Array<T> &keys, const Array<Vector2<T>> &points )
    {
        auto times = makeSourceBudgetSnapshot( keys );
        auto positions = appendLinear2Points(
            m_points, points, []( const auto &p ) { return Vector3<T>( p.x, p.y, T( 0 ) ); } );
        auto tangents = prepareLinear2Tangents( m_tangents, positions.size(), m_autoCalc );
        m_times.swap( times );
        m_points.swap( positions );
        if( !tangents.empty() )
        {
            m_tangents.swap( tangents );
        }
        if( m_autoCalc )
        {
            recalcTangents();
        }
    }

    template <class T>
    void LinearSpline2<T>::setPoints( const Array<Vector2<T>> &points )
    {
        auto positions = appendLinear2Points(
            m_points, points, []( const auto &p ) { return Vector3<T>( p.x, p.y, T( 0 ) ); } );
        auto tangents = prepareLinear2Tangents( m_tangents, positions.size(), m_autoCalc );
        m_points.swap( positions );
        if( !tangents.empty() )
        {
            m_tangents.swap( tangents );
        }
        if( m_autoCalc )
        {
            recalcTangents();
        }
    }

    template <class T>
    Vector2<T> LinearSpline2<T>::interpolate( T t )
    {
        // Work out which segment this is in
        auto fSeg = t * ( m_points.size() - 1 );
        auto segIdx = (u32)fSeg;

        // Apportion t
        auto p = fSeg - segIdx;

        return interpolate( segIdx, p );
    }

    template <class T>
    Vector2<T> LinearSpline2<T>::interpolate( u32 fromIndex, T t ) const
    {
        // Bounds check
        WP_ASSERT( fromIndex < m_points.size() && "fromIndex out of bounds" );

        if( ( fromIndex + 1 ) == m_points.size() )
        {
            // Duff request, cannot blend to nothing
            // Just return source
            auto p = m_points[fromIndex];
            return Vector2<T>( p.x, p.y );
        }

        // Fast special cases
        if( t == T( 0.0 ) )
        {
            auto p = m_points[fromIndex];
            return Vector2<T>( p.x, p.y );
        }
        else if( t == T( 1.0 ) )
        {
            auto p = m_points[fromIndex + 1];
            return Vector2<T>( p.x, p.y );
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
        return Vector2<T>( ret.X(), ret.Y() );
    }

    template <class T>
    u32 LinearSpline2<T>::getNumPoints() const
    {
        return (u32)m_points.size();
    }

    template <class T>
    void LinearSpline2<T>::clear()
    {
        m_times.clear();
        m_points.clear();
        m_tangents.clear();
    }

    template <class T>
    void LinearSpline2<T>::setAutoCalculate( bool autoCalc )
    {
        m_autoCalc = autoCalc;
    }

    template <class T>
    void LinearSpline2<T>::recalcTangents()
    {
        // Catmull-Rom tangent calculation approach
        //
        // For interior points: tangent[i] = 0.5 * (point[i+1] - point[i-1])
        // For endpoints: tangent is based on neighboring point differences
        // Assumes endpoint tangents are parallel with line to neighbor

        size_t i, numPoints;
        bool isClosed;

        numPoints = m_points.size();
        if( numPoints < 2 )
        {
            m_tangents.clear();
            return;
        }

        // Determine if this is a closed spline (first and last points are the same)
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
                // Special case for start point
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
                // Special case for end point
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
                // Interior point: standard Catmull-Rom tangent calculation
                m_tangents[i] = T( 0.5 ) * ( m_points[i + 1] - m_points[i - 1] );
            }
        }
    }

    // explicit instantiation
    template class LinearSpline2<f32>;
    template class LinearSpline2<f64>;
}  // namespace workphone
