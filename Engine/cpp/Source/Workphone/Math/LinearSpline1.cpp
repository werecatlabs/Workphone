#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/LinearSpline1.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

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
    LinearSpline1<T>::LinearSpline1( const LinearSpline1 &other ) :
        ISharedObject(),
        m_autoCalc( other.m_autoCalc ),
        m_times( makeSourceBudgetSnapshot( other.m_times ) ),
        m_points( makeSourceBudgetSnapshot( other.m_points ) ),
        m_tangents( makeSourceBudgetSnapshot( other.m_tangents ) ),
        m_coeffs( other.m_coeffs )
    {
    }

    template <class T>
    LinearSpline1<T> &LinearSpline1<T>::operator=( const LinearSpline1 &other )
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
    void LinearSpline1<T>::unload( SmartPtr<ISharedObject> data )
    {
        clear();
    }

    namespace
    {
        template<class T>
        Array<Vector3<T>> heightSplineTangents( const Array<Vector3<T>> &points,
                                               size_t retainedCapacity )
        {
            auto tangents = makeSourceBudgetSnapshot<Vector3<T>>();
            if( points.empty() ) return tangents;
            tangents.reserve( std::max( retainedCapacity, points.size() ) );
            tangents.resize( points.size() );
            if( points.size() == 1 )
            {
                tangents[0] = Vector3<T>( 0, 0, 0 );
                return tangents;
            }
            const bool closed = points.front() == points.back();
            for( size_t i = 0; i < points.size(); ++i )
            {
                if( i == 0 )
                    tangents[i] = T( 0.5 ) * ( points[1] - points[closed ? points.size() - 2 : 0] );
                else if( i + 1 == points.size() )
                    tangents[i] = closed ? tangents[0] : T( 0.5 ) * ( points[i] - points[i - 1] );
                else
                    tangents[i] = T( 0.5 ) * ( points[i + 1] - points[i - 1] );
            }
            return tangents;
        }

        template<class T>
        void publishHeightPoints( Array<T> &oldTimes, Array<Vector3<T>> &oldPoints,
                                  Array<Vector3<T>> &oldTangents, bool autoCalculate,
                                  Array<T> &times, Array<Vector3<T>> &points )
        {
            auto tangents = autoCalculate ? heightSplineTangents( points, oldTangents.capacity() ) :
                                            makeSourceBudgetSnapshot<Vector3<T>>();
            oldTimes.swap( times );
            oldPoints.swap( points );
            oldTangents.swap( tangents );
        }
    }

    template <class T>
    void LinearSpline1<T>::setPoints( const Array<Pair<T, T>> &input )
    {
        auto times = makeSourceBudgetSnapshot<T>();
        auto points = makeSourceBudgetSnapshot<Vector3<T>>();
        times.reserve( input.capacity() );
        points.reserve( input.capacity() );
        for( const auto &p : input )
        {
            times.push_back( p.first );
            points.push_back( Vector3<T>( p.second, 0, 0 ) );
        }
        publishHeightPoints( m_times, m_points, m_tangents, m_autoCalc, times, points );
    }

    template <class T>
    void LinearSpline1<T>::setPoints( const Array<T> &keys, const Array<T> &input )
    {
        if( keys.size() != input.size() )
            throw std::invalid_argument( "Height spline keys and values must have equal counts" );
        auto times = makeSourceBudgetSnapshot( keys );
        auto points = makeSourceBudgetSnapshot<Vector3<T>>();
        points.reserve( input.capacity() );
        for( const auto &p : input ) points.push_back( Vector3<T>( p, 0, 0 ) );
        publishHeightPoints( m_times, m_points, m_tangents, m_autoCalc, times, points );
    }

    template <class T>
    void LinearSpline1<T>::setPoints( const Array<T> &input )
    {
        auto times = makeSourceBudgetSnapshot<T>();
        auto points = makeSourceBudgetSnapshot<Vector3<T>>();
        times.reserve( input.capacity() );
        points.reserve( input.capacity() );
        for( size_t i = 0; i < input.size(); ++i )
        {
            times.push_back( static_cast<T>( i ) );
            points.push_back( Vector3<T>( input[i], 0, 0 ) );
        }
        publishHeightPoints( m_times, m_points, m_tangents, m_autoCalc, times, points );
    }

    template <class T>
    T LinearSpline1<T>::interpolate( T t )
    {
        if( m_points.empty() ) throw std::out_of_range( "Height spline has no control points" );
        if( !std::isfinite( t ) ) throw std::invalid_argument( "Height spline parameter must be finite" );
        if( m_points.size() == 1 || t <= T( 0 ) ) return m_points.front().X();
        if( t >= T( 1 ) ) return m_points.back().X();
        const auto segment = t * static_cast<T>( m_points.size() - 1 );
        const auto index = static_cast<u32>( segment );
        return interpolate( index, segment - static_cast<T>( index ) );
    }

    template <class T>
    T LinearSpline1<T>::interpolate( u32 fromIndex, T t ) const
    {
        // Bounds check
        if( fromIndex >= m_points.size() ) throw std::out_of_range( "Height spline index out of bounds" );
        if( !std::isfinite( t ) ) throw std::invalid_argument( "Height spline parameter must be finite" );
        t = std::clamp( t, T( 0 ), T( 1 ) );

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

        if( m_tangents.size() != m_points.size() )
            throw std::logic_error( "Height spline tangents require recalculation" );

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
        auto times = std::move( m_times );
        auto points = std::move( m_points );
        auto tangents = std::move( m_tangents );
    }

    template <class T>
    void LinearSpline1<T>::setAutoCalculate( bool autoCalc )
    {
        if( autoCalc && !m_autoCalc ) recalcTangents();
        m_autoCalc = autoCalc;
    }

    template <class T>
    void LinearSpline1<T>::recalcTangents()
    {
        auto tangents = heightSplineTangents( m_points, m_tangents.capacity() );
        m_tangents.swap( tangents );
    }

    // explicit instantiation
    template class LinearSpline1<f32>;
    template class LinearSpline1<f64>;
}  // namespace workphone
