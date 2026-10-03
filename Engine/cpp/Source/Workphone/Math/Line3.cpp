#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Line3.hpp>

namespace workphone
{
    template <class T>
    Line3<T>::Line3() : m_start( 0, 0, 0 ), m_end( 1, 1, 1 )
    {
    }

    template <class T>
    Line3<T>::Line3( T xa, T ya, T za, T xb, T yb, T zb ) : m_start( xa, ya, za ), m_end( xb, yb, zb )
    {
    }

    template <class T>
    Line3<T>::Line3( const Vector3<T> &start, const Vector3<T> &end ) : m_start( start ), m_end( end )
    {
    }

    template <class T>
    Line3<T>::Line3( const Line3<T> &other ) : m_start( other.m_start ), m_end( other.m_end )
    {
    }

    template <class T>
    Line3<T> Line3<T>::operator+( const Vector3<T> &point ) const
    {
        return Line3<T>( m_start + point, m_end + point );
    }

    template <class T>
    Line3<T> &Line3<T>::operator+=( const Vector3<T> &point )
    {
        m_start += point;
        m_end += point;
        return *this;
    }

    template <class T>
    Line3<T> Line3<T>::operator-( const Vector3<T> &point ) const
    {
        return Line3<T>( m_start - point, m_end - point );
    }

    template <class T>
    Line3<T> &Line3<T>::operator-=( const Vector3<T> &point )
    {
        m_start -= point;
        m_end -= point;
        return *this;
    }

    template <class T>
    bool Line3<T>::operator==( const Line3<T> &other ) const
    {
        return ( m_start == other.m_start && m_end == other.m_end ) ||
               ( m_end == other.m_start && m_start == other.m_end );
    }

    template <class T>
    bool Line3<T>::operator!=( const Line3<T> &other ) const
    {
        return !( m_start == other.m_start && m_end == other.m_end ) ||
               ( m_end == other.m_start && m_start == other.m_end );
    }

    template <class T>
    void Line3<T>::setLine( const T &xa, const T &ya, const T &za, const T &xb, const T &yb,
                            const T &zb )
    {
        m_start.set( xa, ya, za );
        m_end.set( xb, yb, zb );
    }

    template <class T>
    void Line3<T>::setLine( const Vector3<T> &nstart, const Vector3<T> &nend )
    {
        m_start.set( nstart );
        m_end.set( nend );
    }

    template <class T>
    void Line3<T>::setLine( const Line3<T> &line )
    {
        m_start.set( line.m_start );
        m_end.set( line.m_end );
    }

    template <class T>
    f64 Line3<T>::getLength() const
    {
        return m_start.getDistanceFrom( m_end );
    }

    template <class T>
    T Line3<T>::getLengthSQ() const
    {
        return m_start.getDistanceFromSQ( m_end );
    }

    template <class T>
    Vector3<T> Line3<T>::getMiddle() const
    {
        return ( m_start + m_end ) * static_cast<T>( 0.5 );
    }

    template <class T>
    Vector3<T> Line3<T>::getVector() const
    {
        return m_end - m_start;
    }

    template <class T>
    bool Line3<T>::isPointBetweenStartAndEnd( const Vector3<T> &point ) const
    {
        return point.isBetweenPoints( m_start, m_end );
    }

    template <class T>
    Vector3<T> Line3<T>::getClosestPoint( const Vector3<T> &point ) const
    {
        Vector3<T> c = point - m_start;
        Vector3<T> v = m_end - m_start;
        T d = static_cast<T>( v.length() );
        v /= d;
        T t = v.dotProduct( c );

        if( t < static_cast<T>( 0.0 ) )
            return m_start;

        if( t > d )
            return m_end;

        v *= t;
        return m_start + v;
    }

    template <class T>
    bool Line3<T>::getIntersectionWithSphere( Vector3<T> sorigin, T sradius, f64 &outdistance ) const
    {
        Vector3<T> q = sorigin - m_start;
        f64 c = q.length();
        f64 v = q.dotProduct( getVector().normaliseCopy() );
        f64 d = sradius * sradius - ( c * c - v * v );

        if( d < 0.0 )
            return false;

        outdistance = static_cast<T>( v - Math<T>::Sqrt( d ) );
        return true;
    }

    template <class T>
    Vector3<T> Line3<T>::getStart() const
    {
        return m_start;
    }

    template <class T>
    void Line3<T>::setStart( const Vector3<T> start )
    {
        m_start = start;
    }

    template <class T>
    Vector3<T> Line3<T>::getEnd() const
    {
        return m_end;
    }

    template <class T>
    void Line3<T>::setEnd( const Vector3<T> end )
    {
        m_end = end;
    }

    template <class T>
    Vector3<T> Line3<T>::getDirection() const
    {
        return ( m_end - m_start ).normaliseCopy();
    }

    // explicit instantiation
    template class Line3<s32>;
    template class Line3<f32>;
    template class Line3<f64>;
}  // namespace workphone
