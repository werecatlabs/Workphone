#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{

    template <class T>
    AABB3<T>::AABB3() : m_minimum( -1, -1, -1 ), m_maximum( 1, 1, 1 )
    {
    }

    template <class T>
    AABB3<T>::AABB3( const AABB3<T> &other ) :
        m_minimum( other.m_minimum ),
        m_maximum( other.m_maximum ),
        m_extent( other.m_extent )
    {
    }

    template <class T>
    AABB3<T>::AABB3( const Vector3<T> &min, const Vector3<T> &max ) :
        m_minimum( min ),
        m_maximum( max ),
        m_extent( AabbExtent::Finite )
    {
    }

    template <class T>
    AABB3<T>::AABB3( const Vector3<T> &init ) :
        m_minimum( init ),
        m_maximum( init ),
        m_extent( AabbExtent::Finite )
    {
    }

    template <class T>
    AABB3<T>::AABB3( T minx, T miny, T minz, T maxx, T maxy, T maxz ) :
        m_minimum( minx, miny, minz ),
        m_maximum( maxx, maxy, maxz ),
        m_extent( AabbExtent::Finite )
    {
    }

    template <class T>
    bool AABB3<T>::operator==( const AABB3<T> &other ) const
    {
        return ( m_minimum == other.m_minimum && other.m_maximum == m_maximum );
    }

    template <class T>
    bool AABB3<T>::operator!=( const AABB3<T> &other ) const
    {
        return !( m_minimum == other.m_minimum && other.m_maximum == m_maximum );
    }

    template <class T>
    void AABB3<T>::merge( const Vector3<T> &point )
    {
        switch( m_extent )
        {
        case AabbExtent::Null:  // if null, use this point
            setExtents( point, point );
            return;

        case AabbExtent::Finite:
            m_maximum.makeCeil( point );
            m_minimum.makeFloor( point );
            return;

        case AabbExtent::Infinite:  // if infinite, makes no difference
            return;
        }
    }

    template <class T>
    void AABB3<T>::merge( const AABB3<T> &rhs )
    {
        // Do nothing if rhs null, or this is infinite
        if( ( rhs.m_extent == AabbExtent::Null ) || ( m_extent == AabbExtent::Infinite ) )
        {
            return;
        }
        // Otherwise if rhs is infinite, make this infinite, too
        else if( rhs.m_extent == AabbExtent::Infinite )
        {
            m_extent = AabbExtent::Infinite;
        }
        // Otherwise if current null, just take rhs
        else if( m_extent == AabbExtent::Null )
        {
            setExtents( rhs.m_minimum, rhs.m_maximum );
        }
        // Otherwise merge
        else
        {
            Vector3 min = m_minimum;
            Vector3 max = m_maximum;
            max.makeCeil( rhs.m_maximum );
            min.makeFloor( rhs.m_minimum );

            setExtents( min, max );
        }
    }

    template <class T>
    void AABB3<T>::reset( T x, T y, T z )
    {
        m_maximum.set( x, y, z );
        m_minimum = m_maximum;
    }

    template <class T>
    void AABB3<T>::reset( const AABB3<T> &initValue )
    {
        *this = initValue;
    }

    template <class T>
    void AABB3<T>::reset( const Vector3<T> &initValue )
    {
        m_maximum = initValue;
        m_minimum = initValue;
    }

    template <class T>
    void AABB3<T>::merge( T x, T y, T z )
    {
        merge( Vector3<T>( x, y, z ) );
    }

    template <class T>
    bool AABB3<T>::isPointInside( const Vector3<T> &p ) const
    {
        return ( p.X() >= m_minimum.X() && p.X() <= m_maximum.X() && p.Y() >= m_minimum.Y() &&
                 p.Y() <= m_maximum.Y() && p.Z() >= m_minimum.Z() && p.Z() <= m_maximum.Z() );
    };

    template <class T>
    bool AABB3<T>::isPointTotalInside( const Vector3<T> &p ) const
    {
        return ( p.X() > m_minimum.X() && p.X() < m_maximum.X() && p.Y() > m_minimum.Y() &&
                 p.Y() < m_maximum.Y() && p.Z() > m_minimum.Z() && p.Z() < m_maximum.Z() );
    };

    template <class T>
    bool AABB3<T>::intersects( const AABB3<T> &other ) const
    {
        return ( m_minimum.Z() <= other.m_maximum.Z() && m_maximum.Z() >= other.m_minimum.Z() ) &&
               ( m_minimum.X() <= other.m_maximum.X() && m_maximum.X() >= other.m_minimum.X() ) &&
               ( m_minimum.Y() <= other.m_maximum.Y() && m_maximum.Y() >= other.m_minimum.Y() );
    }

    template <class T>
    bool AABB3<T>::isFullInside( const AABB3<T> &other ) const
    {
        return ( m_minimum.Z() >= other.m_minimum.Z() && m_maximum.Z() <= other.m_maximum.Z() ) &&
               ( m_minimum.X() >= other.m_minimum.X() && m_maximum.X() <= other.m_maximum.X() ) &&
               ( m_minimum.Y() >= other.m_minimum.Y() && m_maximum.Y() <= other.m_maximum.Y() );
    }

    template <class T>
    bool AABB3<T>::intersectsWithLine( const Line3<T> &line ) const
    {
        auto lineVector = line.getVector();
        auto lineLength = lineVector.normaliseLength();
        return intersectsWithLine( line.getMiddle(), lineVector, lineLength * T( 0.5 ) );
    }

    template <class T>
    bool AABB3<T>::intersectsWithLine( const Vector3<T> &linemiddle, const Vector3<T> &linevect,
                                       T halflength ) const
    {
        const Vector3<T> e = getExtent() * static_cast<T>( 0.5 );
        const Vector3<T> t = getCenter() - linemiddle;
        T r;

        if( ( Math<T>::Abs( t.X() ) > e.X() + halflength * Math<T>::Abs( linevect.X() ) ) ||
            ( Math<T>::Abs( t.Y() ) > e.Y() + halflength * Math<T>::Abs( linevect.Y() ) ) ||
            ( Math<T>::Abs( t.Z() ) > e.Z() + halflength * Math<T>::Abs( linevect.Z() ) ) )
            return false;

        r = e.Y() * static_cast<T>( Math<T>::Abs( linevect.Z() ) ) +
            e.Z() * static_cast<T>( Math<T>::Abs( linevect.Y() ) );
        if( Math<T>::Abs( t.Y() * linevect.Z() - t.Z() * linevect.Y() ) > r )
            return false;

        r = e.X() * static_cast<T>( Math<T>::Abs( linevect.Z() ) ) +
            e.Z() * static_cast<T>( Math<T>::Abs( linevect.X() ) );
        if( Math<T>::Abs( t.Z() * linevect.X() - t.X() * linevect.Z() ) > r )
            return false;

        r = e.X() * static_cast<T>( Math<T>::Abs( linevect.Y() ) ) +
            e.Y() * static_cast<T>( Math<T>::Abs( linevect.X() ) );
        if( Math<T>::Abs( t.X() * linevect.Y() - t.Y() * linevect.X() ) > r )
            return false;

        return true;
    }

    template <class T>
    s32 AABB3<T>::classifyPlaneRelation( const Plane3<T> &plane ) const
    {
        auto nearPoint = Vector3<T>( m_maximum );
        auto farPoint = Vector3<T>( m_minimum );

        auto planeNormal = plane.getNormal();
        auto planeDistance = plane.getDistance();

        if( planeNormal.X() > T( 0.0 ) )
        {
            nearPoint.X() = m_minimum.X();
            farPoint.X() = m_maximum.X();
        }

        if( planeNormal.Y() > T( 0.0 ) )
        {
            nearPoint.Y() = m_minimum.Y();
            farPoint.Y() = m_maximum.Y();
        }

        if( planeNormal.Z() > T( 0.0 ) )
        {
            nearPoint.Z() = m_minimum.Z();
            farPoint.Z() = m_maximum.Z();
        }

        if( planeNormal.dotProduct( nearPoint ) + planeDistance > T( 0.0 ) )
        {
            return static_cast<s32>( PlaneIntersectionRelation::ISREL3D_FRONT );
        }

        if( planeNormal.dotProduct( farPoint ) + planeDistance > T( 0.0 ) )
        {
            return static_cast<s32>( PlaneIntersectionRelation::ISREL3D_CLIPPED );
        }

        return static_cast<s32>( PlaneIntersectionRelation::ISREL3D_BACK );
    }

    template <class T>
    Vector3<T> AABB3<T>::getCenter() const
    {
        return ( m_minimum + m_maximum ) / T( 2.0 );
    }

    template <class T>
    Vector3<T> AABB3<T>::getExtent() const
    {
        return m_maximum - m_minimum;
    }

    template <class T>
    void AABB3<T>::getEdges( Vector3<T> *edges ) const
    {
        const Vector3<T> middle = getCenter();
        const Vector3<T> diag = middle - m_maximum;

        /*
        s are stored in this way:
        Hey, am I an ascii artist, or what? :) niko.
        /4--------/0
        /  |      / |
        /   |     /  |
        6---------2  |
        |   5- - -| -1
        |  /      |  /
        |/        | /
        7---------3/
        */

        edges[0].set( middle.X() + diag.X(), middle.Y() + diag.Y(), middle.Z() + diag.Z() );
        edges[1].set( middle.X() + diag.X(), middle.Y() - diag.Y(), middle.Z() + diag.Z() );
        edges[2].set( middle.X() + diag.X(), middle.Y() + diag.Y(), middle.Z() - diag.Z() );
        edges[3].set( middle.X() + diag.X(), middle.Y() - diag.Y(), middle.Z() - diag.Z() );
        edges[4].set( middle.X() - diag.X(), middle.Y() + diag.Y(), middle.Z() + diag.Z() );
        edges[5].set( middle.X() - diag.X(), middle.Y() - diag.Y(), middle.Z() + diag.Z() );
        edges[6].set( middle.X() - diag.X(), middle.Y() + diag.Y(), middle.Z() - diag.Z() );
        edges[7].set( middle.X() - diag.X(), middle.Y() - diag.Y(), middle.Z() - diag.Z() );
    }

    template <class T>
    bool AABB3<T>::isEmpty() const
    {
        return m_minimum == m_maximum;
    }

    template <class T>
    void AABB3<T>::repair()
    {
        T t;

        if( m_minimum.X() > m_maximum.X() )
        {
            t = m_minimum.X();
            m_minimum.X() = m_maximum.X();
            m_maximum.X() = t;
        }

        if( m_minimum.Y() > m_maximum.Y() )
        {
            t = m_minimum.Y();
            m_minimum.Y() = m_maximum.Y();
            m_maximum.Y() = t;
        }

        if( m_minimum.Z() > m_maximum.Z() )
        {
            t = m_minimum.Z();
            m_minimum.Z() = m_maximum.Z();
            m_maximum.Z() = t;
        }
    }

    template <class T>
    AABB3<T> AABB3<T>::getInterpolated( const AABB3<T> &other, f32 d ) const
    {
        auto inv = T( 1.0 ) - d;

        auto minInv = Vector3<T>( static_cast<T>( static_cast<f32>( other.m_minimum.X() ) * inv ),
                                  static_cast<T>( static_cast<f32>( other.m_minimum.Y() ) * inv ),
                                  static_cast<T>( static_cast<f32>( other.m_minimum.Z() ) * inv ) );
        auto maxInv = Vector3<T>( static_cast<T>( static_cast<f32>( other.m_maximum.X() ) * inv ),
                                  static_cast<T>( static_cast<f32>( other.m_maximum.Y() ) * inv ),
                                  static_cast<T>( static_cast<f32>( other.m_maximum.Z() ) * inv ) );

        auto minD = Vector3<T>( static_cast<T>( static_cast<f32>( m_minimum.X() ) * d ),
                                static_cast<T>( static_cast<f32>( m_minimum.Y() ) * d ),
                                static_cast<T>( static_cast<f32>( m_minimum.Z() ) * d ) );
        auto maxD = Vector3<T>( static_cast<T>( static_cast<f32>( m_maximum.X() ) * d ),
                                static_cast<T>( static_cast<f32>( m_maximum.Y() ) * d ),
                                static_cast<T>( static_cast<f32>( m_maximum.Z() ) * d ) );

        return AABB3<T>( minInv + minD, maxInv + maxD );
    }

    template <class T>
    Vector3<T> AABB3<T>::getSize( void ) const
    {
        return ( m_maximum - m_minimum ) * T( 0.5 );
    }

    template <class T>
    AABB3<T> AABB3<T>::transform( const Matrix4<T> &matrix ) const
    {
        auto result =
            AABB3<T>( Vector3<T>( std::numeric_limits<T>::max(), std::numeric_limits<T>::max(),
                                  std::numeric_limits<T>::max() ),
                      Vector3<T>( std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest(),
                                  std::numeric_limits<T>::lowest() ) );

        for( s32 i = 0; i < 8; ++i )
        {
            auto corner = Vector3<T>( ( i & 1 ) ? m_maximum.X() : m_minimum.X(),
                                      ( i & 2 ) ? m_maximum.Y() : m_minimum.Y(),
                                      ( i & 4 ) ? m_maximum.Z() : m_minimum.Z() );
            result.merge( matrix * corner );
        }

        return result;
    }

    template <class T>
    void AABB3<T>::setExtents( const Vector3<T> &min, const Vector3<T> &max )
    {
        WP_ASSERT( ( min.X() <= max.X() && min.Y() <= max.Y() && min.Z() <= max.Z() ) );

        m_extent = AabbExtent::Finite;
        m_minimum = min;
        m_maximum = max;
    }

    template <class T>
    void AABB3<T>::setNull()
    {
        m_extent = AabbExtent::Null;
    }

    template <class T>
    bool AABB3<T>::isNull() const
    {
        return ( m_extent == AabbExtent::Null );
    }

    template <class T>
    bool AABB3<T>::isFinite() const
    {
        return ( m_extent == AabbExtent::Finite );
    }

    template <class T>
    void AABB3<T>::setInfinite()
    {
        m_extent = AabbExtent::Infinite;
    }

    template <class T>
    bool AABB3<T>::isInfinite() const
    {
        return ( m_extent == AabbExtent::Infinite );
    }

    template <class T>
    bool AABB3<T>::isValid() const
    {
        return isFinite() && m_minimum.isValid() && m_maximum.isValid();
    }

    template <class T>
    const Vector3<T> &AABB3<T>::getMinimum() const
    {
        WP_ASSERT( m_minimum.isValid() );
        return m_minimum;
    }

    template <class T>
    void AABB3<T>::setMinimum( const Vector3<T> &minimum )
    {
        m_minimum = minimum;
    }

    template <class T>
    const Vector3<T> &AABB3<T>::getMaximum() const
    {
        WP_ASSERT( m_maximum.isValid() );
        return m_maximum;
    }

    template <class T>
    void AABB3<T>::setMaximum( const Vector3<T> &maximum )
    {
        m_maximum = maximum;
    }

    template <class T>
    T AABB3<T>::getRadius() const
    {
        return ( m_maximum - m_minimum ).length() / T( 2.0 );
    }

    // explicit instantiation
    template class AABB3<s32>;
    template class AABB3<f32>;
    template class AABB3<f64>;
}  // namespace workphone
