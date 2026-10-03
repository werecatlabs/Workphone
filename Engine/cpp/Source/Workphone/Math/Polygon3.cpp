#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Polygon3.hpp>

namespace workphone
{

    template <class T>
    Polygon3<T>::Polygon3() = default;

    template <class T>
    Polygon3<T>::Polygon3( const Polygon3 &polygon )
    {
        *this = polygon;
    }

    template <class T>
    Polygon3<T>::Polygon3( const Array<Vector3<T>> &points )
    {
        m_points = points;
    }

    template <class T>
    Polygon3<T>::~Polygon3()
    {
        m_points.clear();
    }

    template <class T>
    void Polygon3<T>::addPoint( const Vector3<T> &point )
    {
        m_points.push_back( point );
    }

    template <class T>
    void Polygon3<T>::addPoints( const Array<Vector3<T>> &points )
    {
        for( u32 i = 0; i < points.size(); ++i )
        {
            m_points.push_back( points[i] );
        }
    }

    template <class T>
    void Polygon3<T>::setPoints( const Array<Vector3<T>> &points )
    {
        m_points = points;
    }

    template <class T>
    auto Polygon3<T>::removePoint( u32 i ) -> bool
    {
        if( i >= m_points.size() )
        {
            return false;
        }

        m_points.erase( m_points.begin() + i );
        return true;
    }

    template <class T>
    auto Polygon3<T>::getPoint( u32 i ) const -> Vector3<T>
    {
        return m_points[i];
    }

    template <class T>
    auto Polygon3<T>::getNumPoints() const -> u32
    {
        return static_cast<u32>( m_points.size() );
    }

    template <class T>
    auto Polygon3<T>::getTriangles( const Plane3<T> &plane, Array<Triangle3<T>> &triangles ) -> bool
    {
        Vector2<T> points2d;

        auto normal = plane.getNormal();

        auto tangentVector = Vector3<T>( 1, 0, 0 );
        auto tangent = normal.crossProduct( tangentVector );
        auto binormal = tangent.crossProduct( normal );

        normal.normalise();
        tangent.normalise();
        binormal.normalise();

        Array<Vector2<T>> contour;

        // project point on a 2d plane
        for( u32 pointIdx = 0; pointIdx < m_points.size(); ++pointIdx )
        {
            Vector3<T> point = m_points[pointIdx];
            Vector2<T> point2d;
            point2d.X() = tangent.dotProduct( point );
            point2d.Y() = binormal.dotProduct( point );
            contour.push_back( point2d );
        }

        /* allocate and initialise list of Vertices in polygon */
        auto n = contour.size();
        if( n < 3 )
        {
            return false;
        }

        int *V = new int[n]; /* we want a counter-clockwise polygon in V */
        if( 0.0f < area( contour ) )
        {
            for( size_t v = 0; v < n; v++ )
            {
                V[v] = static_cast<s32>( v );
            }
        }
        else
        {
            for( size_t v = 0; v < n; v++ )
            {
                V[v] = static_cast<s32>( ( n - 1 ) - v );
            }
        }

        int nv = static_cast<s32>( n ); /*  remove nv-2 Vertices, creating 1 triangle every time */
        int count = 2 * nv;             /* error detection */

        for( int m = 0, v = nv - 1; nv > 2; )
        {
            /* if we loop, it is probably a non-simple polygon */
            if( 0 >= ( count-- ) )
            {
                //** Triangulate: ERROR - probable bad polygon!
                return false;
            }

            /* three consecutive vertices in current polygon, <u,v,w> */
            int u = v;
            if( nv <= u )
            {
                u = 0; /* previous */
            }
            v = u + 1;
            if( nv <= v )
            {
                v = 0; /* new v    */
            }
            int w = v + 1;

            if( nv <= w )
            {
                w = 0; /* next     */
            }

            if( snip( contour, u, v, w, nv, V ) )
            {
                int a, b, c, s, t; /* true names of the vertices */
                a = V[u];
                b = V[v];
                c = V[w]; /* output Triangle */

                Triangle3<T> triangle( m_points[a], m_points[b], m_points[c] );
                triangles.push_back( triangle );
                m++; /* remove v from remaining polygon */

                for( s = v, t = v + 1; t < nv; s++, t++ )
                {
                    V[s] = V[t];
                }

                --nv; /* resest error detection counter */

                count = 2 * nv;
            }
        }

        delete[] V;
        return true;
    }

    template <class T>
    auto Polygon3<T>::insideTriangle( T Ax, T Ay, T Bx, T By, T Cx, T Cy, T Px, T Py ) -> bool
    {
        T ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
        T cCROSSap, bCROSScp, aCROSSbp;

        ax = Cx - Bx;
        ay = Cy - By;
        bx = Ax - Cx;
        by = Ay - Cy;
        cx = Bx - Ax;
        cy = By - Ay;
        apx = Px - Ax;
        apy = Py - Ay;
        bpx = Px - Bx;
        bpy = Py - By;
        cpx = Px - Cx;
        cpy = Py - Cy;
        aCROSSbp = ax * bpy - ay * bpx;
        cCROSSap = cx * apy - cy * apx;
        bCROSScp = bx * cpy - by * cpx;

        return ( ( aCROSSbp >= T( 0.0 ) ) && ( bCROSScp >= T( 0.0 ) ) && ( cCROSSap >= T( 0.0 ) ) );
    };

    template <class T>
    auto Polygon3<T>::snip( const Array<Vector2<T>> &contour, s32 u, s32 v, s32 w, s32 n, s32 *V )
        -> bool
    {
        auto A = contour[V[u]];
        auto B = contour[V[v]];
        auto C = contour[V[w]];

        // triangle2df triangle(A, B, C);
        const auto EPSILON = std::numeric_limits<T>::epsilon();
        if( EPSILON > Math<T>::Abs( ( ( B.X() - A.X() ) * ( C.Y() - A.Y() ) ) -
                                    ( ( B.Y() - A.Y() ) * ( C.X() - A.X() ) ) ) )
        {
            return false;
        }

        for( auto p = 0; p < n; p++ )
        {
            if( ( p == u ) || ( p == v ) || ( p == w ) )
            {
                continue;
            }

            auto point = contour[V[p]];
            if( insideTriangle( A.X(), A.Y(), B.X(), B.Y(), C.X(), C.Y(), point.X(), point.Y() ) )
            {
                return false;
            }
        }

        return true;
    }

    template <class T>
    auto Polygon3<T>::area( const Array<Vector2<T>> &contour ) -> T
    {
        auto n = contour.size();
        auto A = T( 0.0 );

        for( size_t p = n - 1, q = 0; q < n; p = q++ )
        {
            auto p0 = contour[p];
            auto p1 = contour[q];
            A += p0.X() * p1.Y() - p1.X() * p0.Y();
        }

        return A * T( 0.5 );
    }

    template <class T>
    Vector3<T> Polygon3<T>::getCenter() const
    {
        auto center = Vector3<T>::zero();

        auto numPoints = m_points.size();
        for( size_t i = 0; i < numPoints; ++i )
        {
            auto point = m_points[i];
            center += point;
        }

        center /= static_cast<T>( numPoints );

        return center;
    }

    template <class T>
    Array<Vector3<T>> Polygon3<T>::getPoints() const
    {
        return m_points;
    }

    template class Polygon3<f32>;
    template class Polygon3<f64>;

}  // namespace workphone
