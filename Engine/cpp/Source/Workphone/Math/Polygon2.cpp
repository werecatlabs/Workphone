#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Polygon2.hpp>

namespace workphone
{

    template <class T>
    auto Polygon2<T>::getPoints() const -> Array<Vector2<T>>
    {
        return m_points;
    }

    template <class T>
    auto Polygon2<T>::intersects( const Polygon2 &polygon ) const -> bool
    {
        auto center = polygon.getCenter();
        auto numPoints0 = static_cast<size_t>( polygon.getNumPoints() );
        auto numPoints1 = m_points.size();
        auto totalPoints = numPoints0 + numPoints1;

        for( size_t polyIdx = 0; polyIdx < totalPoints; ++polyIdx )
        {
            auto point0 = Vector2<T>::zero();
            auto point1 = Vector2<T>::zero();

            if( polyIdx < numPoints0 )
            {
                point0 = polygon.getPoint( static_cast<u32>( polyIdx ) );
                point1 = polygon.getPoint( static_cast<u32>( ( polyIdx + 1 ) % numPoints0 ) );
            }
            else
            {
                auto curPointIndx = numPoints1 - ( totalPoints - polyIdx );
                point0 = getPoint( static_cast<u32>( curPointIndx ) );
                point1 = getPoint( static_cast<u32>( ( curPointIndx + 1 ) % numPoints1 ) );
            }

            auto axis = ( point1 - point0 ).perp();
            axis.normalise();

            auto min0 = static_cast<T>( 1e6 );
            auto max0 = static_cast<T>( -1e6 );

            auto min1 = static_cast<T>( 1e6 );
            auto max1 = static_cast<T>( -1e6 );

            getPolygonIntervals( *this, axis, min0, max0 );
            getPolygonIntervals( polygon, axis, min1, max1 );

            if( intervalDistance( min0, max0, min1, max1 ) > T( 0.0 ) )
            {
                return false;
            }
        }

        return true;
    }

    template <class T>
    auto Polygon2<T>::intervalDistance( T minA, T maxA, T minB, T maxB ) const -> T
    {
        if( minA < minB )
        {
            return minB - maxA;
        }

        return minA - maxB;
    }

    template <class T>
    void Polygon2<T>::getPolygonIntervals( const Polygon2<T> &polygon, const Vector2<T> &axis, T &min,
                                           T &max ) const
    {
        auto numPoints = polygon.getNumPoints();
        for( u32 polyIdx = 0; polyIdx < numPoints; ++polyIdx )
        {
            auto curPoint = polygon.getPoint( polyIdx );
            auto distance = axis.dotProduct( curPoint );

            if( min > distance )
            {
                min = distance;
            }

            if( max < distance )
            {
                max = distance;
            }
        }
    }

    template <class T>
    auto Polygon2<T>::isInside( const Polygon2<T> &polygon ) const -> bool
    {
#if 0
			bool inside = false;
			for (int j = 0, i = v.size()-1; j < v.size(); i = j++) {
				if (v[i][0] >= p[0] || v[j][0] >= p[0]) { // At least one endpoint >= px
					if (v[i][1] < p[1]) { // y1 below ray
						if (v[j][1] >= p[1]) { // y2 on or above ray, edge going 'up'
							if ((p[1]-v[i][1])*(v[j][0]-v[i][0]) >= (p[0]-v[i][0])*(v[j][1]-v[i][1])) {
								inside = !inside;
							}
						}
					} else if (v[j][1] < p[1]) { // y1 on or above ray, y2 below ray, edge going 'down'
						if ((p[1]-v[i][1])*(v[j][0]-v[i][0]) <= (p[0]-v[i][0])*(v[j][1]-v[i][1])) {
							inside = !inside;
						}
					}
				}
			}
			
			return inside;
#else
        auto numPoints = polygon.getNumPoints();
        for( u32 i = 0; i < numPoints; ++i )
        {
            auto point = polygon.getPoint( i );
            if( !isPointInside( point ) )
            {
                return false;
            }
        }

        return true;
#endif
    }

    template <class T>
    auto Polygon2<T>::isPointInside( const Vector2<T> &point ) const -> bool
    {
        WP_ASSERT( point.isValid() );

#if 0
			int pnpoly(int nvert, float *vertx, float *verty, float testx, float testy)
			{
				int i, j, c = 0;
				for (i = 0, j = nvert-1; i < nvert; j = i++) {
					if ( ((verty[i]>testy) != (verty[j]>testy)) &&
						(testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
						c = !c;
				}
				return c;
			}
#endif

#if 0
			u32 numPoints = Points.size();
			int i, j;
			bool c = 0;
			for (i = 0, j = numPoints-1; i < numPoints; j = i++)
			{
				const Vector2<T>& pointI = Points[i];
				const Vector2<T>& pointJ = Points[j];

				bool test0 = (pointI.Y() > point.Y()) != (pointJ.Y() > point.Y());
				f32 testVal = ((pointJ.X()-pointI.X()) * (point.Y() - pointI.Y()) / (pointJ.Y()-pointI.Y()) + pointI.X());
				if ( test0 && point.X() < testVal )
				{
					c = !c;
				}
			}

			return c;
#else
        auto counter = 0;
        auto i = 0;
        auto xinters = 0.0;
        auto p1 = Vector2<T>::zero();
        auto p2 = Vector2<T>::zero();

        auto numPoints = m_points.size();
        if( m_points.empty() )
        {
            return false;
        }

        p1 = m_points[0];

        for( i = 1; i <= static_cast<s32>( numPoints ); ++i )
        {
            p2 = m_points[i % numPoints];

            if( point.Y() > Math<T>::min( p1.Y(), p2.Y() ) )
            {
                if( point.Y() <= Math<T>::max( p1.Y(), p2.Y() ) )
                {
                    if( point.X() <= Math<T>::max( p1.X(), p2.X() ) )
                    {
                        if( p1.Y() != p2.Y() )
                        {
                            xinters =
                                ( point.Y() - p1.Y() ) * ( p2.X() - p1.X() ) / ( p2.Y() - p1.Y() ) +
                                p1.X();
                            if( p1.X() == p2.X() || point.X() <= xinters )
                            {
                                ++counter;
                            }
                        }
                    }
                }
            }

            p1 = p2;
        }

        if( counter % 2 == 0 )
        {
            return false;
        }

        return true;
#endif
    }

    template <class T>
    auto Polygon2<T>::polygonEdgeLength2D( s32 i ) -> T
    {
        WP_ASSERT( MathI::isFinite( i ) );

        auto v1 = m_points[i];
        WP_ASSERT( v1.isValid() );

        auto v2 = m_points[( i + 1 ) % static_cast<s32>( m_points.size() )];
        WP_ASSERT( v2.isValid() );

        auto x = v2.X() - v1.X();
        WP_ASSERT( Math<T>::isFinite( x ) );

        auto y = v2.Y() - v1.Y();
        WP_ASSERT( Math<T>::isFinite( y ) );

        auto sq = x * x + y * y;
        WP_ASSERT( Math<T>::isFinite( sq ) );
        return Math<T>::Sqrt( sq );
    }

    template <class T>
    void Polygon2<T>::polygonSearchEdges( s32 &i1, T &l1, s32 &i2, T &l2 )
    {
        auto index1 = 0;
        auto length1 = T( -1.0 );

        // first search the longest edge of the polygon
        for( size_t i = 0; i < m_points.size(); i++ )
        {
            auto temp = polygonEdgeLength2D( static_cast<u32>( i ) );
            if( temp > length1 )
            {
                index1 = static_cast<s32>( i );
                length1 = temp;
            }
        }

        // calculate the direction vector of this edge
        auto d0 =
            m_points[( index1 + 1 ) % static_cast<int>( m_points.size() )].X() - m_points[index1].X();
        auto d1 =
            m_points[( index1 + 1 ) % static_cast<int>( m_points.size() )].Y() - m_points[index1].Y();
        T dir1[2] = { d0, d1 };

        // now try to find a edge that is nearly parallel to the previous one
        auto index2 = -1;
        auto product = T( 0.0 );
        auto length2 = T( 0.0 );

        for( size_t i = 0; i < m_points.size(); i++ )
        {
            if( static_cast<s32>( i ) == index1 )
            {
                continue;
            }

            T dir2[2] = {
                m_points[( i + 1 ) % static_cast<int>( m_points.size() )].X() - m_points[i].X(),
                m_points[( i + 1 ) % static_cast<int>( m_points.size() )].Y() - m_points[i].Y()
            };

            auto templ = polygonEdgeLength2D( static_cast<u32>( i ) );
            auto temp = ( dir1[0] * dir2[0] + dir1[1] * dir2[1] ) / ( length1 * templ );

            if( temp < product )
            {
                index2 = static_cast<s32>( i );
                product = temp;
                length2 = templ;
            }
        }

        // if the step above fails, set index2 to a standard value
        if( index2 == -1 )
        {
            index2 = ( index1 + static_cast<s32>( m_points.size() ) / 2 ) %
                     static_cast<s32>( m_points.size() );
            length2 = polygonEdgeLength2D( index2 );
        }

        using Polygon2I = Polygon2<s32>;
        using Polygon2F = Polygon2<f32>;
        using Polygon2D = Polygon2<f64>;
    }

    template <class T>
    void Polygon2<T>::recursiveSubdivideInLots( s32 level, const Vector2<T> &minArea,
                                                const Vector2<T> &maxArea,
                                                Array<Polygon2<T>> &resultPolygons )
    {
        level++;

        auto minPolyArea = minArea.X() * minArea.Y();
        auto maxPolyArea = maxArea.X() * maxArea.Y();

        auto minXInterval = std::numeric_limits<T>::max();
        auto maxXInterval = std::numeric_limits<T>::min();
        auto minYInterval = std::numeric_limits<T>::max();
        auto maxYInterval = std::numeric_limits<T>::min();

        getPolygonIntervals( *this, Vector2<T>::UNIT_X, minXInterval, maxXInterval );
        getPolygonIntervals( *this, Vector2<T>::UNIT_Y, minYInterval, maxYInterval );

        auto xLength = maxXInterval - minXInterval;
        auto yLength = maxYInterval - minYInterval;

        // check if polygon area is already below threshold
        auto polyArea = Math<T>::Abs( getArea() );
        if( polyArea <= maxPolyArea )
        {
            if( polyArea >= minPolyArea )
            {
                if( ( xLength > minArea.X() && xLength < maxArea.X() ) &&
                    ( yLength > minArea.Y() && yLength < maxArea.Y() ) )
                {
                    // check ratios
                    auto maxAreaX = yLength * T( 1.5 );
                    auto maxAreaY = xLength * T( 1.5 );

                    if( xLength < maxAreaX && yLength < maxAreaY )
                    {
                        resultPolygons.push_back( *this );
                        return;
                    }
                }
            }
            else
            {
                return;
            }
        }

        if( level > 200 )
        {
            return;
        }

        /*if(m_points.size() > 100)
        return;*/

        // search two longest edges in polygon
        auto i1 = 0;
        auto i2 = 0;
        auto l1 = T( 0.0 );
        auto l2 = T( 0.0 );
        polygonSearchEdges( i1, l1, i2, l2 );

        // sort two longest edges
        if( i2 < i1 )
        {
            s32 te;
            te = i1;
            i1 = i2;
            i2 = te;
        }

        // calculate mid points of the two longest edges
        auto randomValue = Math<T>::RangedRandom( T( 0.0 ), T( 0.5 ) );
        auto offset = T( 0.5 ) + randomValue;

        auto edgePointA0 = m_points[i1];
        auto edgePointA1 = m_points[( i1 + 1 ) % static_cast<s32>( m_points.size() )];

        auto m = Vector2<T>::zero();
        m.X() = edgePointA0.X() + offset * ( edgePointA1.X() - edgePointA0.X() );
        m.Y() = edgePointA0.Y() + offset * ( edgePointA1.Y() - edgePointA0.Y() );

        auto edgePointB0 = m_points[i2];
        auto edgePointB1 = m_points[( i2 + 1 ) % static_cast<s32>( m_points.size() )];

        auto n = Vector2<T>::zero();
        n.X() = edgePointB0.X() + offset * ( edgePointB1.X() - edgePointB0.X() );
        n.Y() = edgePointB0.Y() + offset * ( edgePointB1.Y() - edgePointB0.Y() );

#if 1
        i2 = findOppositeEdge( edgePointA0, edgePointA1, m, n );
#endif

        // sort two longest edges
        if( i2 < i1 )
        {
            s32 te;
            te = i1;
            i1 = i2;
            i2 = te;

            Vector2<T> tempVect = m;
            m = n;
            n = tempVect;
        }

        // split polygon in two new polygons
        Polygon2<T> p1, p2;

        // p1 ------------------------------------------
        for( int i = 0; i < i1 + 1; i++ )
        {
            p1.addPoint( m_points[i] );
        }

        p1.addPoint( m );
        p1.addPoint( n );

        for( s32 i = i2 + 1; i < static_cast<s32>( m_points.size() ); i++ )
        {
            p1.addPoint( m_points[i] );
        }

        // p2 ------------------------------------------
        p2.addPoint( m );
        for( s32 i = i1 + 1; i < i2 + 1; i++ )
        {
            p2.addPoint( m_points[i] );
        }

        p2.addPoint( n );
    }

    template <class T>
    void Polygon2<T>::recursiveSubdivideInLots( s32 level, T minArea, T maxArea,
                                                Array<Polygon2<T>> &resultPolygons )
    {
        level++;

        // check if polygon area is already below threshold
        auto area = getArea();
        auto polyArea = Math<T>::Abs( area );
        if( polyArea <= maxArea )
        {
            if( polyArea >= minArea )
            {
                resultPolygons.push_back( *this );
            }

            return;
        }

        if( level > 200 )
        {
            return;
        }

        /*if(m_points.size() > 100)
        return;*/

        // search two longest edges in polygon
        auto i1 = 0;
        auto i2 = 0;
        auto l1 = T( 0.0 );
        auto l2 = T( 0.0 );
        polygonSearchEdges( i1, l1, i2, l2 );

        // sort two longest edges
        if( i2 < i1 )
        {
            s32 te;
            te = i1;
            i1 = i2;
            i2 = te;
        }

        // calculate mid points of the two longest edges
        auto randomValue = Math<T>::RangedRandom( T( 0.0 ), T( 0.5f ) );
        auto offset = T( 0.5 ) + randomValue;

        auto edgePointA0 = m_points[i1];
        auto edgePointA1 = m_points[( i1 + 1 ) % static_cast<int>( m_points.size() )];

        auto m = Vector2<T>::zero();
        m.X() = edgePointA0.X() + offset * ( edgePointA1.X() - edgePointA0.X() );
        m.Y() = edgePointA0.Y() + offset * ( edgePointA1.Y() - edgePointA0.Y() );

        auto edgePointB0 = m_points[i2];
        auto edgePointB1 = m_points[( i2 + 1 ) % static_cast<int>( m_points.size() )];

        auto n = Vector2<T>::zero();
        n.X() = edgePointB0.X() + offset * ( edgePointB1.X() - edgePointB0.X() );
        n.Y() = edgePointB0.Y() + offset * ( edgePointB1.Y() - edgePointB0.Y() );

#if 1
        i2 = findOppositeEdge( edgePointA0, edgePointA1, m, n );
#endif

        // sort two longest edges
        if( i2 < i1 )
        {
            s32 te;
            te = i1;
            i1 = i2;
            i2 = te;

            auto tempVect = m;
            m = n;
            n = tempVect;
        }

        // split polygon in two new polygons
        Polygon2<T> p1, p2;

        // p1 ------------------------------------------
        for( s32 i = 0; i < i1 + 1; i++ )
        {
            p1.addPoint( m_points[i] );
        }

        p1.addPoint( m );
        p1.addPoint( n );

        for( int i = i2 + 1; i < static_cast<int>( m_points.size() ); i++ )
        {
            p1.addPoint( m_points[i] );
        }

        // p2 ------------------------------------------
        p2.addPoint( m );

        for( s32 i = i1 + 1; i < i2 + 1; i++ )
        {
            p2.addPoint( m_points[i] );
        }

        p2.addPoint( n );

        // make recursive call
        p1.recursiveSubdivideInLots( level, minArea, maxArea, resultPolygons );
        p2.recursiveSubdivideInLots( level, minArea, maxArea, resultPolygons );
    }

    template <class T>
    void Polygon2<T>::subDivide( const Vector2<T> &minArea, const Vector2<T> &maxArea,
                                 Array<Polygon2<T>> &resultPolygons )
    {
        recursiveSubdivideInLots( 0, minArea, maxArea, resultPolygons );
    }

    template <class T>
    void Polygon2<T>::subDivide( T minArea, T maxArea, Array<Polygon2<T>> &resultPolygons )
    {
#if 0
			Array< polygon2<T> > polygons;
			polygons.push_back(*this);

			while (!polygons.empty())
			{
				polygon2<T>& curPolygon = polygons.getLast();

				// check if polygon area is already below threshold
				T polyArea = MathF::Abs(curPolygon.getArea());
				if (polyArea <= maxArea) 
				{
					if (polyArea >= minArea) 
					{
						resultPolygons.push_back(curPolygon);

						polygons.erase(polygons.size()-1);
						continue;
					}
					else if(polyArea < minArea) 
					{
						polygons.erase(polygons.size()-1);
						continue;
					}
				}

				// search two longest edges in polygon
				s32 i1, i2;
				T l1, l2;
				curPolygon.polygonSearchEdges(i1, l1, i2, l2);

				// sort two longest edges
				if (i2 < i1) 
				{
					s32 te;
					te = i1; 
					i1 = i2; 
					i2 = te;
				}

				// calculate mid points of the two longest edges
				T randomValue = MathF::rangedRandom(T(0.0), T(1.0));
				T offset = randomValue * T(0.4) - T(0.2);

				Vector2<T> edgePoint0 = Points[i1];
				Vector2<T> edgePoint1 = Points[(i1+1) % (int)Points.size()];

				Vector2<T> m;
				m.X() = edgePoint0.X() + (T(0.5) + offset) * (edgePoint1.X() - edgePoint0.X());
				m.Y() = edgePoint0.Y() + (T(0.5) + offset) * (edgePoint1.Y() - edgePoint0.Y());

				Vector2<T> n;
				n.X() = Points[i2].X() + (T(0.5) - offset) * (Points[(i2+1) % (int)Points.size()].X() - Points[i2].X());
				n.Y() = Points[i2].Y() + (T(0.5) - offset) * (Points[(i2+1) % (int)Points.size()].Y() - Points[i2].Y());

	        //i2 = curPolygon.findOppositeEdge(edgePoint0, edgePoint1, m, n);
                }
            }
        }

        // Placeholder implementation: return first edge index
        return 0;
				// sort two longest edges
				if (i2 < i1) 
				{
					s32 te;
					te = i1; 
					i1 = i2; 
					i2 = te;

					Vector2<T> tempVect = m;
					m = n;
					n = tempVect;
				}

				/// split polygon in two new polygons
				polygon2<T> p1, p2;

				// p1 ------------------------------------------
				for (int i = 0; i < i1 + 1; i++) 
				{
					p1.addPoint(Points[i]);
				}

				p1.addPoint(m);
				p1.addPoint(n);

				for (int i = i2 + 1; i < (int)Points.size(); i++) 
				{
					p1.addPoint(Points[i]);
				}

				// p2 ------------------------------------------
				p2.addPoint(m);
				for (int i = i1 + 1; i < i2 + 1; i++) 
				{
					p2.addPoint(Points[i]);
				}
				p2.addPoint(n);

				polygons.erase(polygons.size()-1);
				polygons.push_back(p1);
				polygons.push_back(p2);

				if(polygons.size() > 20000)
					break;
			}

#else
        recursiveSubdivideInLots( 0, minArea, maxArea, resultPolygons );
#endif
    }

    template <class T>
    void Polygon2<T>::changePointOrdering()
    {
        Array<Vector2<T>> tempPoints;
        auto numPoints = m_points.size();
        WP_ASSERT( MathI::isFinite( static_cast<s32>( numPoints ) ) );
        tempPoints.reserve( numPoints );

        for( u32 i = 0; i < m_points.size(); ++i )
        {
            auto p = m_points[i];
            WP_ASSERT( p.isValid() );
            tempPoints.push_back( p );
        }

        m_points = tempPoints;
    }

    template <class T>
    auto Polygon2<T>::getArea() const -> T
    {
        auto center = getCenter();

        // polygon area function
        // http://astronomy.swin.edu.au/~pbourke/geometry/polyarea/
        auto sum = T( 0.0 );
        auto numPoints = m_points.size();
        for( size_t i = 0; i < numPoints; i++ )
        {
            auto v1 = m_points[i];
            auto v2 = m_points[( i + 1 ) % numPoints];

            v1 = v1 - center;
            v2 = v2 - center;
            sum += v1.X() * v2.Y() - v2.X() * v1.Y();
        }

        // return area: if positive -> ccw ordering
        //              if negative -> cw  ordering
        return T( 0.5 ) * sum;
    }

    template <class T>
    auto Polygon2<T>::getNumPoints() const -> u32
    {
        return static_cast<u32>( m_points.size() );
    }

    template <class T>
    auto Polygon2<T>::getOffsetPolygon( T offset ) const -> Polygon2<T>
    {
        Polygon2<T> polygon;

#if 1
        auto numPolygonPoints = m_points.size();
        for( size_t pointIdx = 0; pointIdx < numPolygonPoints; ++pointIdx )
        {
            const auto &point0 = getPoint( static_cast<u32>( pointIdx ) );
            const auto &point1 = getPoint( static_cast<u32>( ( pointIdx + 1 ) % numPolygonPoints ) );

            auto prevPointIndex = pointIdx;
            auto prevIndex = static_cast<size_t>( 0 );

            if( prevPointIndex == 0 )
            {
                prevIndex = numPolygonPoints - 1;
            }
            else
            {
                prevIndex = ( prevPointIndex - 1 ) % numPolygonPoints;
            }

            const auto &prevPoint0 = getPoint( static_cast<u32>( prevIndex ) );
            const auto &prevPoint1 = point0;

            auto edgeDir = ( point1 - point0 ).normaliseCopy();
            auto edgeTangent = edgeDir.perp();

            auto newEdgeDir = ( prevPoint1 - prevPoint0 ).normaliseCopy();
            auto newEdgeTangent = newEdgeDir.perp();

            flipTangent( point0, point1, edgeTangent );
            flipTangent( prevPoint0, prevPoint1, newEdgeTangent );

            // calculate scale
            auto scale = offset;
            auto combinedTangent = ( edgeTangent + newEdgeTangent ) * T( 0.5 );
            combinedTangent.normalise();

            auto newPoint = point0 + ( combinedTangent * scale );
            polygon.addPoint( newPoint );
        }
#else
#    include "FBMath/line2.h"

        Array<Line2<T>> lines;
        u32 numPolygonPoints = Points.size();
        for( u32 pointIdx = 0; pointIdx < numPolygonPoints; ++pointIdx )
        {
            const Vector2<T> &point0 = getPoint( pointIdx );
            const Vector2<T> &point1 = getPoint( ( pointIdx + 1 ) % numPolygonPoints );

            Vector2<T> midPoint = ( point1 + point0 ) * 0.5f;
            Vector2<T> edgeDir = ( point1 - point0 ).normaliseCopy();
            Vector2<T> edgeTangent = edgeDir.perp();
            flipTangent( point0, point1, edgeTangent );

            Line2<T> line;
            line.m_start = point0;
            line.m_end = point1;

            line.m_start = line.m_start + edgeTangent * offset;
            line.m_end = line.m_end + edgeTangent * offset;

            lines.push_back( line );
        }

        u32 numLines = lines.size();
        for( u32 i = 0; i < numLines; ++i )
        {
            const Line2<T> &line = lines[i];
            const Line2<T> &line0 = lines[( i + 1 ) % numLines];

            s32 prevPointIndex = i;
            s32 prevIndex = 0;

            if( prevPointIndex == 0 )
            {
                prevIndex = numLines - 1;
            }
            else
            {
                prevIndex = ( prevPointIndex - 1 ) % numLines;
            }

            const Line2<T> &line1 = lines[prevIndex];

            Vector2<T> intersectionPoint0;
            Vector2<T> intersectionPoint1;

            if( line.intersectWith( line0, intersectionPoint0 ) &&
                line.intersectWith( line1, intersectionPoint1 ) )
            {
                // polygon.addPoint(intersectionPoint1);
                polygon.addPoint( intersectionPoint0 );
            }
        }

#endif

        return polygon;
    }

    template <class T>
    void Polygon2<T>::flipTangent( const Vector2<T> &edgePoint0, const Vector2<T> &edgePoint1,
                                   Vector2<T> &edgeTangent ) const
    {
        WP_ASSERT( edgePoint0.isValid() );
        WP_ASSERT( edgePoint1.isValid() );
        WP_ASSERT( edgeTangent.isValid() );

        auto halfEdgePoint = ( edgePoint0 + edgePoint1 ) * T( 0.5 );
        WP_ASSERT( halfEdgePoint.isValid() );

        auto polyTestPoint = halfEdgePoint + ( edgeTangent * T( 0.1 ) );
        WP_ASSERT( polyTestPoint.isValid() );

        if( !isPointInside( polyTestPoint ) )
        {
            edgeTangent = -edgeTangent;
        }
    }

    template <class T>
    auto Polygon2<T>::getScalePolygon( T scale ) const -> Polygon2<T>
    {
        Polygon2<T> polygon;

        auto center = getCenter();

        auto numPoints = m_points.size();
        for( size_t i = 0; i < numPoints; ++i )
        {
            auto point = m_points[i];
            auto pointVector = point - center;
            auto newPoint = center + Vector2<T>( pointVector.X() * scale, pointVector.Y() * scale );
            polygon.addPoint( newPoint );
        }

        return polygon;
    }

    template <class T>
    auto Polygon2<T>::getCenter() const -> Vector2<T>
    {
        Vector2<T> center;

        auto numPoints = m_points.size();
        for( size_t i = 0; i < numPoints; ++i )
        {
            const auto &point = m_points[i];
            center += point;
        }

        center /= static_cast<T>( numPoints );

        return center;
    }

    template <class T>
    void Polygon2<T>::setAsBox( const Vector2<T> &minPoint, const Vector2<T> &maxPoint )
    {
        m_points.resize( 4 );
        m_points[0] = minPoint;
        m_points[1] = Vector2<T>( maxPoint.X(), minPoint.Y() );
        m_points[2] = maxPoint;
        m_points[3] = Vector2<T>( minPoint.X(), maxPoint.Y() );
    }

    template <class T>
    auto Polygon2<T>::removePoint( u32 i ) -> bool
    {
        WP_ASSERT( i < m_points.size() );  // invalid index

        if( i < m_points.size() )
        {
            auto it = m_points.begin();
            std::advance( it, i );
            m_points.erase( it );
            return true;
        }

        return false;
    }

    template <class T>
    void Polygon2<T>::setPoints( const Array<Vector2<T>> &points )
    {
        m_points = points;
    }

    template <class T>
    void Polygon2<T>::addPoints( const Array<Vector2<T>> &points )
    {
        for( size_t i = 0; i < points.size(); ++i )
        {
            auto p = points[i];
            WP_ASSERT( p.isValid() );
            m_points.push_back( p );
        }
    }

    template <class T>
    void Polygon2<T>::addPoint( const Vector2<T> &point )
    {
        WP_ASSERT( point.isValid() );
        m_points.push_back( point );
    }

    template <class T>
    auto Polygon2<T>::operator!=( const Polygon2<T> &other ) const -> bool
    {
        return !( *this == other );
    }

    template <class T>
    auto Polygon2<T>::operator==( const Polygon2<T> &other ) const -> bool
    {
        auto numPoints0 = static_cast<size_t>( other.getNumPoints() );
        auto numPoints1 = m_points.size();

        if( numPoints0 != numPoints1 )
        {
            return false;
        }

        for( size_t polyIdx = 0; polyIdx < numPoints0; ++polyIdx )
        {
            auto curPoint = other.getPoint( static_cast<u32>( polyIdx ) );

            auto matchFound = false;

            for( size_t poly1Idx = 0; poly1Idx < numPoints1; ++poly1Idx )
            {
                auto curPoint1 = getPoint( static_cast<u32>( poly1Idx ) );

                if( curPoint == curPoint1 )
                {
                    matchFound = true;
                }
            }

            if( !matchFound )
            {
                return false;
            }
        }

        return true;
    }

    template <class T>
    Polygon2<T>::~Polygon2()
    {
        m_points.clear();
    }

    template <class T>
    Polygon2<T>::Polygon2( const Array<Vector2<T>> &points )
    {
        m_points = points;
    }

    template <class T>
    Polygon2<T>::Polygon2( const Polygon2 &polygon )
    {
        *this = polygon;
    }

    template <class T>
    Polygon2<T>::Polygon2( u32 capacity )
    {
        WP_ASSERT( MathI::isFinite( capacity ) );
        m_points.reserve( capacity );
    }

    template <class T>
    auto Polygon2<T>::getPoint( u32 i ) const -> Vector2<T>
    {
        WP_ASSERT( i < m_points.size() );  // invalid index
        return m_points[i];
    }

    template <class T>
    auto Polygon2<T>::findOppositeEdge( const Vector2<T> &edgePoint0, const Vector2<T> &edgePoint1,
                                        const Vector2<T> &pointOnEdge, Vector2<T> &collidingPoint )
        -> u32
    {
        Vector2<T> perp = ( edgePoint1 - edgePoint0 ).perp();
        perp.normalise();

        Vector2<T> tangent = perp;
        /*if(fabs(perp.X()) > 0.5)
        {
        tangent.X() = 1.0;
        tangent.Y() = 0.0;
        }
        else
        {
        tangent.X() = 0.0;
        tangent.Y() = 1.0;
        }*/

        flipTangent( edgePoint0, edgePoint1, tangent );

        auto lineStart = pointOnEdge;
        auto lineEnd = pointOnEdge + ( tangent * T( 1000.0 ) );

        Line2<T> testLine( lineStart, lineEnd );

        auto edgePointB0 = Vector2<T>::zero();
        auto edgePointB1 = Vector2<T>::zero();

        for( size_t pointIdx = 0; pointIdx < m_points.size(); ++pointIdx )
        {
            auto point0 = getPoint( static_cast<u32>( pointIdx ) );
            auto point1 = getPoint( static_cast<u32>( ( pointIdx + 1 ) % m_points.size() ) );

            if( edgePoint0 == point0 && edgePoint1 == point1 )
            {
                continue;
            }

            Line2<T> curLine;
            curLine.setStart( point0 );
            curLine.setEnd( point1 );

            Vector2<T> intersectionPoint;
            if( testLine.intersectWith( curLine, intersectionPoint ) )
            {
            }
        }

        // Placeholder implementation: return first edge index
        return 0;
    }

    // explicit instantiation
    template class Polygon2<s32>;
    template class Polygon2<f32>;
    template class Polygon2<f64>;

}  // namespace workphone
