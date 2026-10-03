#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Plane3.hpp>

namespace workphone
{
    template <class T>
    Plane3<T>::Plane3() : m_normal( 0, 1, 0 )
    {
        recalculateD( Vector3<T>( 0, 0, 0 ) );
    }

    template <class T>
    Plane3<T>::Plane3( const Vector3<T> &point, const Vector3<T> &normal ) : m_normal( normal )
    {
        recalculateD( point );
    }

    template <class T>
    Plane3<T>::Plane3( T px, T py, T pz, T nx, T ny, T nz ) : m_normal( nx, ny, nz )
    {
        recalculateD( Vector3<T>( px, py, pz ) );
    }

    template <class T>
    Plane3<T>::Plane3( const Plane3<T> &other ) :
        m_normal( other.m_normal ),
        m_distance( other.m_distance )
    {
    }

    template <class T>
    Plane3<T>::Plane3( const Vector3<T> &point1, const Vector3<T> &point2, const Vector3<T> &point3 )
    {
        setPlane( point1, point2, point3 );
    }

    template <class T>
    bool Plane3<T>::operator==( const Plane3<T> &other ) const
    {
        return ( m_distance == other.m_distance && m_normal == other.m_normal );
    }

    template <class T>
    bool Plane3<T>::operator!=( const Plane3<T> &other ) const
    {
        return !( m_distance == other.m_distance && m_normal == other.m_normal );
    }

    template <class T>
    void Plane3<T>::setPlane( const Vector3<T> &point, const Vector3<T> &nvector )
    {
        m_normal = nvector;
        m_normal.normalise();
        recalculateD( point );
    }

    template <class T>
    void Plane3<T>::setPlane( const Vector3<T> &nvect, T d )
    {
        m_normal = nvect;
        m_distance = d;
    }

    template <class T>
    void Plane3<T>::setPlane( const Vector3<T> &point1, const Vector3<T> &point2,
                              const Vector3<T> &point3 )
    {
        // creates the plane from 3 memberpoints
        m_normal = ( point2 - point1 ).crossProduct( point3 - point1 );
        m_normal.normalise();

        recalculateD( point1 );
    }

    template <class T>
    bool Plane3<T>::getIntersectionWithLine( const Vector3<T> &linePoint, const Vector3<T> &lineVect,
                                             Vector3<T> &outIntersection ) const
    {
        T t2 = m_normal.dotProduct( lineVect );

        if( t2 == 0 )
            return false;

        T t = -( m_normal.dotProduct( linePoint ) + m_distance ) / t2;
        outIntersection = linePoint + ( lineVect * t );
        return true;
    }

    template <class T>
    f32 Plane3<T>::getKnownIntersectionWithLine( const Vector3<T> &linePoint1,
                                                 const Vector3<T> &linePoint2 ) const
    {
        Vector3<T> vect = linePoint2 - linePoint1;
        T t2 = static_cast<T>( m_normal.dotProduct( vect ) );
        return static_cast<T>( -( ( m_normal.dotProduct( linePoint1 ) + m_distance ) / t2 ) );
    }

    template <class T>
    bool Plane3<T>::getIntersectionWithLimitedLine( const Vector3<T> &linePoint1,
                                                    const Vector3<T> &linePoint2,
                                                    Vector3<T> &outIntersection ) const
    {
        return ( getIntersectionWithLine( linePoint1, linePoint2 - linePoint1, outIntersection ) &&
                 outIntersection.isBetweenPoints( linePoint1, linePoint2 ) );
    }

    template <class T>
    s32 Plane3<T>::classifyPointRelation( const Vector3<T> &point ) const
    {
        const T d = m_normal.dotProduct( point ) + m_distance;

        if( d < -Math<T>::epsilon() )
        {
            return (s32)PlaneIntersectionRelation::ISREL3D_FRONT;
        }

        if( d > Math<T>::epsilon() )
        {
            return (s32)PlaneIntersectionRelation::ISREL3D_BACK;
        }

        return (s32)PlaneIntersectionRelation::ISREL3D_PLANAR;
    }

    template <class T>
    void Plane3<T>::recalculateD( const Vector3<T> &MPoint )
    {
        m_distance = -MPoint.dotProduct( m_normal );
    }

    template <class T>
    Vector3<T> Plane3<T>::getMemberPoint() const
    {
        return m_normal * -m_distance;
    }

    template <class T>
    bool Plane3<T>::existsInterSection( const Plane3<T> &other ) const
    {
        Vector3<T> cross = other.m_normal.crossProduct( m_normal );
        return cross.length() > MathF::epsilon();
    }

    template <class T>
    bool Plane3<T>::getIntersectionWithPlane( const Plane3<T> &other, Vector3<T> &outLinePoint,
                                              Vector3<T> &outLineVect ) const
    {
        T fn00 = m_normal.length();
        T fn01 = m_normal.dotProduct( other.m_normal );
        T fn11 = other.m_normal.length();
        T det = fn00 * fn11 - fn01 * fn01;

        if( fabs( det ) < Math<T>::epsilon() )
            return false;

        det = 1.0 / det;
        T fc0 = static_cast<T>( ( fn11 * -m_distance + fn01 * other.m_distance ) * det );
        T fc1 = static_cast<T>( ( fn00 * -other.m_distance + fn01 * m_distance ) * det );

        outLineVect = m_normal.crossProduct( other.m_normal );
        outLinePoint = m_normal * static_cast<T>( fc0 ) + other.m_normal * static_cast<T>( fc1 );
        return true;
    }

    template <class T>
    bool Plane3<T>::getIntersectionWithPlanes( const Plane3<T> &o1, const Plane3<T> &o2,
                                               Vector3<T> &outPoint ) const
    {
        Vector3<T> linePoint, lineVect;
        if( getIntersectionWithPlane( o1, linePoint, lineVect ) )
            return o2.getIntersectionWithLine( linePoint, lineVect, outPoint );

        return false;
    }

    template <class T>
    bool Plane3<T>::isFrontFacing( const Vector3<T> &lookDirection ) const
    {
        const T d = m_normal.dotProduct( lookDirection );
        return Math<T>::equals( d, T( 0.0 ) );
    }

    template <class T>
    T Plane3<T>::getDistance( const Vector3<T> &point ) const
    {
        return point.dotProduct( m_normal ) + m_distance;
    }

    template <class T>
    PlaneSide Plane3<T>::getSide( const Vector3<T> &centre, const Vector3<T> &halfSize ) const
    {
        // Calculate the distance between box centre and the plane
        T dist = getDistance( centre );

        // Calculate the maximise allows absolute distance for
        // the distance between box centre and plane
        T maxAbsDist = Math<T>::Abs( m_normal.X() * halfSize.X() ) +
                       Math<T>::Abs( m_normal.Y() * halfSize.Y() ) +
                       Math<T>::Abs( m_normal.Z() * halfSize.Z() );

        if( dist < -maxAbsDist )
        {
            return PlaneSide::NEGATIVE_SIDE;
        }

        if( dist > maxAbsDist )
        {
            return PlaneSide::POSITIVE_SIDE;
        }

        return PlaneSide::BOTH_SIDE;
    }

    template <class T>
    Vector3<T> Plane3<T>::getNormal() const
    {
        return m_normal;
    }

    template <class T>
    void Plane3<T>::setNormal( const Vector3<T> &normal )
    {
        m_normal = normal;
    }

    template <class T>
    T Plane3<T>::getDistance() const
    {
        return m_distance;
    }

    template <class T>
    void Plane3<T>::setDistance( T distance )
    {
        m_distance = distance;
    }

    // explicit instantiation
    template class Plane3<s32>;
    template class Plane3<f32>;
    template class Plane3<f64>;
}  // namespace workphone
