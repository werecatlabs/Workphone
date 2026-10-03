#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Line2.hpp>

namespace workphone
{
    template <class T>
    void Line2<T>::setEnd( const Vector2<T> &end )
    {
        m_end = end;
    }

    template <class T>
    workphone::Vector2<T> Line2<T>::getEnd() const
    {
        return m_end;
    }

    template <class T>
    void Line2<T>::setStart( Vector2<T> &start )
    {
        m_start = start;
    }

    template <class T>
    workphone::Vector2<T> Line2<T>::getStart() const
    {
        return m_start;
    }

    template <class T>
    workphone::Vector2<T> Line2<T>::getClosestPoint( const Vector2<T> &point ) const
    {
        Vector2<T> c = point - m_start;
        Vector2<T> v = m_end - m_start;
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
    bool Line2<T>::isPointBetweenStartAndEnd( const Vector2<T> &point ) const
    {
        return point.isBetweenPoints( m_start, m_end );
    }

    template <class T>
    bool Line2<T>::isPointOnLine( const Vector2<T> &point )
    {
        T d = getPointOrientation( point );
        return ( d == 0 && point.isBetweenPoints( m_start, m_end ) );
    }

    template <class T>
    T Line2<T>::getPointOrientation( const Vector2<T> &point )
    {
        return ( ( m_end.X() - m_start.X() ) * ( point.Y() - m_start.Y() ) -
                 ( point.X() - m_start.X() ) * ( m_end.Y() - m_start.Y() ) );
    }

    template <class T>
    T Line2<T>::getAngleWith( const Line2<T> &l )
    {
        Vector2<T> vect = getVector();
        Vector2<T> vect2 = l.getVector();
        return vect.getAngleWith( vect2 );
    }

    template <class T>
    workphone::Vector2<T> Line2<T>::getDirection() const
    {
        T len = static_cast<T>( 1.0 / getLength() );
        return Vector2<T>( ( m_end.X() - m_start.X() ) * len, ( m_end.Y() - m_start.Y() ) * len );
    }

    template <class T>
    workphone::Vector2<T> Line2<T>::getUnitVector() const
    {
        T len = static_cast<T>( 1.0 / getLength() );
        return Vector2<T>( ( m_end.X() - m_start.X() ) * len, ( m_end.Y() - m_start.Y() ) * len );
    }

    template <class T>
    bool Line2<T>::intersectWith( const Vector2<T> &center, T radius ) const
    {
        Vector2<T> closestPoint = getClosestPoint( center );
        T distance = ( center - closestPoint ).length();
        if( Math<T>::Abs( distance ) <= radius )
        {
            return true;
        }

        return false;
    }

    template <class T>
    bool Line2<T>::intersectWith( const Line2<T> &l, Vector2<T> &out ) const
    {
        // Uses the method given at:
        // http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
        const T commonDenominator = ( l.m_end.Y() - l.m_start.Y() ) * ( m_end.X() - m_start.X() ) -
                                    ( l.m_end.X() - l.m_start.X() ) * ( m_end.Y() - m_start.Y() );

        const T numeratorA = ( l.m_end.X() - l.m_start.X() ) * ( m_start.Y() - l.m_start.Y() ) -
                             ( l.m_end.Y() - l.m_start.Y() ) * ( m_start.X() - l.m_start.X() );

        const T numeratorB = ( m_end.X() - m_start.X() ) * ( m_start.Y() - l.m_start.Y() ) -
                             ( m_end.Y() - m_start.Y() ) * ( m_start.X() - l.m_start.X() );

        if( Math<T>::equals( commonDenominator, T( 0.0 ) ) )
        {
            // The lines are either coincident or parallel
            if( Math<T>::equals( numeratorA, T( 0.0 ) ) && Math<T>::equals( numeratorB, T( 0.0 ) ) )
            {
                // Try and find a common Endpoint
                if( l.m_start == m_start || l.m_end == m_start )
                {
                    out = m_start;
                }
                else if( l.m_end == m_end || l.m_start == m_end )
                {
                    out = m_end;
                }
                else
                {
                    // one line is contained in the other, so for lack of a better
                    // answer, pick the average of both lines
                    out = ( ( m_start + m_end + l.m_start + l.m_end ) * T( 0.25 ) );

                    T minAX = Math<T>::min( m_start.X(), m_end.X() );
                    T maxAX = Math<T>::max( m_start.X(), m_end.X() );
                    T minAY = Math<T>::min( m_start.Y(), m_end.Y() );
                    T maxAY = Math<T>::max( m_start.Y(), m_end.Y() );
                    T minBX = Math<T>::min( l.m_start.X(), l.m_end.X() );
                    T maxBX = Math<T>::max( l.m_start.X(), l.m_end.X() );
                    T minBY = Math<T>::min( l.m_start.Y(), l.m_end.Y() );
                    T maxBY = Math<T>::max( l.m_start.Y(), l.m_end.Y() );

                    if( maxAX < minBX || minAX > maxBX || maxAY < minBY || minAY > maxBY )
                    {
                        return false;
                    }
                }

                return true;  // coincident
            }

            return false;  // parallel
        }

        // Get the point of intersection on this line, checking that
        // it is within the line segment.
        const T uA = numeratorA / commonDenominator;
        if( uA < -Math<T>::epsilon() || uA > ( T( 1.0 ) + Math<T>::epsilon() ) )
        {
            return false;  // Outside the line segment
        }

        const T uB = numeratorB / commonDenominator;
        if( uB < -Math<T>::epsilon() || uB > ( T( 1.0 ) + Math<T>::epsilon() ) )
        {
            return false;  // Outside the line segment
        }

        // Calculate the intersection point.
        out.X() = m_start.X() + uA * ( m_end.X() - m_start.X() );
        out.Y() = m_start.Y() + uA * ( m_end.Y() - m_start.Y() );

        return true;
    }

    template <class T>
    workphone::Vector2<T> Line2<T>::getVector() const
    {
        return Vector2<T>( m_start.X() - m_end.X(), m_start.Y() - m_end.Y() );
    }

    template <class T>
    workphone::Vector2<T> Line2<T>::getMiddle() const
    {
        return ( m_start + m_end ) * T( 0.5 );
    }

    template <class T>
    T Line2<T>::getLengthSQ() const
    {
        return m_start.getDistanceFromSQ( m_end );
    }

    template <class T>
    T Line2<T>::getLength() const
    {
        return m_start.getDistanceFrom( m_end );
    }

    template <class T>
    void Line2<T>::setLine( const Line2<T> &line )
    {
        m_start.set( line.m_start );
        m_end.set( line.m_end );
    }

    template <class T>
    void Line2<T>::setLine( const Vector2<T> &nstart, const Vector2<T> &nend )
    {
        m_start.set( nstart );
        m_end.set( nend );
    }

    template <class T>
    void Line2<T>::setLine( const T &xa, const T &ya, const T &xb, const T &yb )
    {
        m_start.set( xa, ya );
        m_end.set( xb, yb );
    }

    template <class T>
    bool Line2<T>::operator!=( const Line2<T> &other ) const
    {
        return !( m_start == other.m_start && m_end == other.m_end ) ||
               ( m_end == other.m_start && m_start == other.m_end );
    }

    template <class T>
    bool Line2<T>::operator==( const Line2<T> &other ) const
    {
        return ( m_start == other.m_start && m_end == other.m_end ) ||
               ( m_end == other.m_start && m_start == other.m_end );
    }

    template <class T>
    workphone::Line2<T> &Line2<T>::operator-=( const Vector2<T> &point )
    {
        m_start -= point;
        m_end -= point;
        return *this;
    }

    template <class T>
    workphone::Line2<T> Line2<T>::operator-( const Vector2<T> &point ) const
    {
        return Line2<T>( m_start - point, m_end - point );
    }

    template <class T>
    workphone::Line2<T> &Line2<T>::operator+=( const Vector2<T> &point )
    {
        m_start += point;
        m_end += point;
        return *this;
    }

    template <class T>
    workphone::Line2<T> Line2<T>::operator+( const Vector2<T> &point ) const
    {
        return Line2<T>( m_start + point, m_end + point );
    }

    template <class T>
    Line2<T>::Line2() : m_start( 0, 0 ), m_end( 1, 1 )
    {
    }

    template <class T>
    Line2<T>::Line2( T xa, T ya, T xb, T yb ) : m_start( xa, ya ), m_end( xb, yb )
    {
    }

    template <class T>
    Line2<T>::Line2( const Vector2<T> &start, const Vector2<T> &end ) : m_start( start ), m_end( end )
    {
    }

    template <class T>
    Line2<T>::Line2( const Line2<T> &other ) : m_start( other.m_start ), m_end( other.m_end )
    {
    }

    // explicit instantiation
    template class Line2<s32>;
    template class Line2<f32>;
    template class Line2<f64>;

}  // end namespace workphone
