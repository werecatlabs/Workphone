#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Polygon2.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <type_traits>

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

    namespace
    {
        template <class T>
        using Ring = Array<Vector2<T>>;

        template <class T>
        bool samePoint( const Vector2<T> &a, const Vector2<T> &b )
        {
            return a.x == b.x && a.y == b.y;
        }

        // Integer coordinates can span the full signed 32-bit range. Their edge
        // products and the sum around the polygon need more than 64 bits.
        template <class T>
        using AreaNumber =
            std::conditional_t<std::is_integral_v<T>, boost::multiprecision::int128_t, long double>;

        template <class T>
        AreaNumber<T> orientation( const Vector2<T> &a, const Vector2<T> &b, const Vector2<T> &c )
        {
            using N = AreaNumber<T>;
            return ( N( b.x ) - a.x ) * ( N( c.y ) - a.y ) - ( N( b.y ) - a.y ) * ( N( c.x ) - a.x );
        }

        template <class T>
        long double signedArea( const Ring<T> &ring )
        {
            AreaNumber<T> twiceArea = 0;
            for( size_t i = 2; i < ring.size(); ++i )
            {
                twiceArea += orientation( ring[0], ring[i - 1], ring[i] );
            }
            return static_cast<long double>( twiceArea ) * .5L;
        }

        template <class T>
        bool withinSegment( const Vector2<T> &a, const Vector2<T> &b, const Vector2<T> &p )
        {
            return p.x >= std::min( a.x, b.x ) && p.x <= std::max( a.x, b.x ) &&
                   p.y >= std::min( a.y, b.y ) && p.y <= std::max( a.y, b.y );
        }

        template <class T>
        bool segmentsMeet( const Vector2<T> &a, const Vector2<T> &b, const Vector2<T> &c,
                           const Vector2<T> &d )
        {
            const auto abc = orientation( a, b, c ), abd = orientation( a, b, d );
            const auto cda = orientation( c, d, a ), cdb = orientation( c, d, b );
            if( abc == 0 && withinSegment( a, b, c ) )
            {
                return true;
            }
            if( abd == 0 && withinSegment( a, b, d ) )
            {
                return true;
            }
            if( cda == 0 && withinSegment( c, d, a ) )
            {
                return true;
            }
            if( cdb == 0 && withinSegment( c, d, b ) )
            {
                return true;
            }
            return ( ( abc < 0 && abd > 0 ) || ( abc > 0 && abd < 0 ) ) &&
                   ( ( cda < 0 && cdb > 0 ) || ( cda > 0 && cdb < 0 ) );
        }

        template <class T>
        bool isSimpleRing( const Ring<T> &ring )
        {
            for( size_t i = 0; i < ring.size(); ++i )
            {
                const auto next = ( i + 1 ) % ring.size(), after = ( next + 1 ) % ring.size();
                if( orientation( ring[i], ring[next], ring[after] ) == 0 &&
                    !withinSegment( ring[i], ring[after], ring[next] ) )
                {
                    return false;  // The two adjacent edges double back.
                }
                for( size_t j = i + 2; j < ring.size(); ++j )
                {
                    const auto end = ( j + 1 ) % ring.size();
                    if( end != i && segmentsMeet( ring[i], ring[next], ring[j], ring[end] ) )
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        template <class T>
        Vector2<T> pointOnEdge( const Vector2<T> &a, const Vector2<T> &b, long double fraction,
                                long double parentArea )
        {
            if( fraction <= 0 )
            {
                return a;
            }
            if( fraction >= 1 )
            {
                return b;
            }
            const auto dx = static_cast<long double>( b.x ) - a.x;
            const auto dy = static_cast<long double>( b.y ) - a.y;
            long double unitX = 1, unitY = 1;
            if constexpr( std::is_floating_point_v<T> )
            {
                const Vector2<T> point{ static_cast<T>( a.x + fraction * dx ),
                                        static_cast<T>( a.y + fraction * dy ) };
                // Far from the origin, rounding X and Y independently can move a cut
                // several metres off its edge. Use the local representable coordinate
                // lattice when that displacement materially changes the parent's area.
                const auto tolerance =
                    std::sqrt( std::numeric_limits<T>::epsilon() ) * std::abs( parentArea );
                if( std::abs( orientation( a, b, point ) ) <= tolerance )
                {
                    return point;
                }
                const auto unit = []( T first, T last ) {
                    const auto magnitude = std::max( std::abs( first ), std::abs( last ) );
                    if( magnitude == 0 )
                    {
                        return 1.L;
                    }
                    const int exponent = std::max( std::ilogb( magnitude ) + 1,
                                                   std::numeric_limits<T>::min_exponent );
                    return std::ldexp( 1.L, exponent - std::numeric_limits<T>::digits );
                };
                unitX = unit( a.x, b.x );
                unitY = unit( a.y, b.y );
            }
            const auto stepsX = dx / unitX, stepsY = dy / unitY;
            // Endpoints in different exponent ranges need not share the coarser grid.
            const auto integralSteps = []( long double value ) {
                return std::abs( value ) < static_cast<long double>( std::numeric_limits<s64>::max() ) &&
                       std::trunc( value ) == value;
            };
            if( !integralSteps( stepsX ) || !integralSteps( stepsY ) )
            {
                return fraction < .5L ? a : b;
            }
            const auto ix = static_cast<s64>( stepsX ), iy = static_cast<s64>( stepsY );
            const auto steps = std::gcd( ix, iy );
            if( !steps )
            {
                return fraction < .5L ? a : b;
            }
            const auto step = static_cast<s64>( std::round( fraction * steps ) );
            const Vector2<T> point{ static_cast<T>( a.x + ( ix / steps ) * step * unitX ),
                                    static_cast<T>( a.y + ( iy / steps ) * step * unitY ) };
            if( orientation( a, b, point ) != 0 ) return fraction < .5L ? a : b;
            return point;
        }

        template <class T>
        Ring<T> boundaryArc( const Ring<T> &ring, size_t edge, const Vector2<T> &start, size_t lastEdge,
                             const Vector2<T> &end )
        {
            auto arc = makeSourceBudgetSnapshot<Vector2<T>>();
            arc.push_back( start );
            while( edge != lastEdge )
            {
                edge = ( edge + 1 ) % ring.size();
                if( !samePoint( arc.back(), ring[edge] ) )
                {
                    arc.push_back( ring[edge] );
                }
            }
            if( !samePoint( arc.back(), end ) )
            {
                arc.push_back( end );
            }
            return arc;
        }

        template <class T>
        struct PendingLot
        {
            Ring<T> ring;
            long double aspectScale;
        };

        template <class T>
        void splitLot( const Ring<T> &ring, long double area, long double fraction,
                       long double aspectScale, Array<PendingLot<T>> &pending )
        {
            auto tryChord = [&]( size_t edge, const Vector2<T> &p, size_t other, const Vector2<T> &q ) {
                if( samePoint( p, q ) )
                {
                    return false;
                }
                // Rounded edge points (especially lattice points) can move the cut toward a
                // concavity. Reject chords that cross or pass through another boundary vertex.
                for( size_t i = 0; i < ring.size(); ++i )
                {
                    if( i == edge || i == other )
                    {
                        continue;
                    }
                    const auto &a = ring[i], &b = ring[( i + 1 ) % ring.size()];
                    if( samePoint( a, p ) || samePoint( a, q ) || samePoint( b, p ) ||
                        samePoint( b, q ) )
                    {
                        if( !samePoint( a, p ) && !samePoint( a, q ) && orientation( p, q, a ) == 0 &&
                            withinSegment( p, q, a ) )
                        {
                            return false;
                        }
                        if( !samePoint( b, p ) && !samePoint( b, q ) && orientation( p, q, b ) == 0 &&
                            withinSegment( p, q, b ) )
                        {
                            return false;
                        }
                    }
                    else if( segmentsMeet( p, q, a, b ) )
                    {
                        return false;
                    }
                }
                auto left = boundaryArc( ring, edge, p, other, q );
                auto right = boundaryArc( ring, other, q, edge, p );
                const auto leftArea = signedArea( left ), rightArea = signedArea( right );
                const auto smaller = [&]( long double child ) {
                    return area > 0 ? child > 0 && child < area : child < 0 && child > area;
                };
                if( !smaller( leftArea ) || !smaller( rightArea ) )
                {
                    return false;
                }
                pending.push_back( { std::move( right ), aspectScale } );
                pending.push_back( { std::move( left ), aspectScale } );
                return true;
            };

            size_t longest = 0;
            long double maxLength = 0;
            for( size_t i = 0; i < ring.size(); ++i )
            {
                const auto &a = ring[i], &b = ring[( i + 1 ) % ring.size()];
                const auto length = std::hypot( static_cast<long double>( b.x ) - a.x,
                                                static_cast<long double>( b.y ) - a.y );
                if( length > maxLength )
                {
                    maxLength = length;
                    longest = i;
                }
            }
            for( size_t attempt = 0; attempt < ring.size(); ++attempt )
            {
                const auto edge = ( longest + attempt ) % ring.size();
                const auto &a = ring[edge], &b = ring[( edge + 1 ) % ring.size()];
                const auto p = pointOnEdge( a, b, fraction, area );
                if( samePoint( p, a ) || samePoint( p, b ) )
                {
                    continue;
                }
                const long double dx = static_cast<long double>( b.x ) - a.x;
                const long double dy = static_cast<long double>( b.y ) - a.y;
                size_t hit = ring.size();
                long double hitFraction = 0, nearest = std::numeric_limits<long double>::infinity();
                for( size_t i = 0; i < ring.size(); ++i )
                {
                    if( i == edge )
                    {
                        continue;
                    }
                    const auto &c = ring[i], &d = ring[( i + 1 ) % ring.size()];
                    const long double cx = static_cast<long double>( c.x ) - p.x;
                    const long double cy = static_cast<long double>( c.y ) - p.y;
                    const long double ex = static_cast<long double>( d.x ) - c.x;
                    const long double ey = static_cast<long double>( d.y ) - c.y;
                    const auto denominator = dx * ex + dy * ey;
                    if( denominator == 0 )
                    {
                        continue;
                    }
                    const auto t = -( dx * cx + dy * cy ) / denominator;
                    if( t < 0 || t > 1 )
                    {
                        continue;
                    }
                    const auto side = dx * ( cy + t * ey ) - dy * ( cx + t * ex );
                    const auto distance = area > 0 ? side : -side;
                    if( distance > 0 && distance < nearest )
                    {
                        nearest = distance;
                        hit = i;
                        hitFraction = t;
                    }
                }
                if( hit != ring.size() )
                {
                    const auto &c = ring[hit], &d = ring[( hit + 1 ) % ring.size()];
                    const auto q = pointOnEdge( c, d, hitFraction, area );
                    if( tryChord( edge, p, hit, q ) ) return;
                    // The nearest lattice point can be an endpoint on the starting
                    // edge. The far endpoint may still be a valid diagonal.
                    if( samePoint( q, c ) && tryChord( edge, p, hit, d ) )
                    {
                        return;
                    }
                    if( samePoint( q, d ) && tryChord( edge, p, hit, c ) )
                    {
                        return;
                    }
                }
            }
            // A short integer edge can have no interior lattice points. A diagonal
            // through existing vertices can still partition a simple polygon.
            for( size_t i = 0; ring.size() > 3 && i < ring.size(); ++i )
            {
                const auto j = ( i + 2 ) % ring.size();
                if( tryChord( i, ring[i], j, ring[j] ) )
                {
                    return;
                }
            }
        }

        struct LotLimits
        {
            long double minArea, maxArea;
            long double minWidth = 0, minHeight = 0, maxWidth = 0, maxHeight = 0;
        };

        template <class T>
        void subdivideLots( const Ring<T> &points, const LotLimits &limits, Array<Polygon2<T>> &output )
        {
            auto ring = makeSourceBudgetSnapshot<Vector2<T>>();
            for( const auto &p : points )
            {
                if( !std::isfinite( static_cast<long double>( p.x ) ) ||
                    !std::isfinite( static_cast<long double>( p.y ) ) )
                {
                    throw std::invalid_argument( "Lot polygon coordinates must be finite" );
                }
                if( ring.empty() || !samePoint( ring.back(), p ) )
                {
                    ring.push_back( p );
                }
            }
            if( ring.size() > 1 && samePoint( ring.front(), ring.back() ) )
            {
                ring.pop_back();
            }
            if( ring.size() < 3 || !isSimpleRing( ring ) )
            {
                return;
            }

            auto pending = makeSourceBudgetSnapshot<PendingLot<T>>();
            auto lots = makeSourceBudgetSnapshot<Polygon2<T>>();
            pending.push_back( { std::move( ring ), 0 } );
            while( !pending.empty() )
            {
                auto item = std::move( pending.back() );
                pending.pop_back();
                const auto &current = item.ring;
                const auto area = signedArea( current ), magnitude = std::abs( area );
                if( !std::isfinite( magnitude ) )
                {
                    throw std::overflow_error( "Lot polygon area exceeds the supported range" );
                }
                if( magnitude == 0 || magnitude < limits.minArea )
                {
                    continue;
                }
                bool fits = magnitude <= limits.maxArea;
                if( limits.maxWidth > 0 )
                {
                    auto min = current.front(), max = min;
                    for( const auto &p : current )
                    {
                        min.x = std::min( min.x, p.x );
                        min.y = std::min( min.y, p.y );
                        max.x = std::max( max.x, p.x );
                        max.y = std::max( max.y, p.y );
                    }
                    const auto width = static_cast<long double>( max.x ) - min.x;
                    const auto height = static_cast<long double>( max.y ) - min.y;
                    if( width < limits.minWidth || height < limits.minHeight )
                    {
                        continue;
                    }
                    fits = fits && width <= limits.maxWidth && height <= limits.maxHeight;
                    if( fits && ( width > 1.5L * height || height > 1.5L * width ) )
                    {
                        // Only shape needs refinement now. Aim for lots on the scale of
                        // the shorter side. An acute tip can have the same aspect ratio
                        // after every cut, even when the caller specified no minimum size.
                        if( item.aspectScale == 0 )
                        {
                            item.aspectScale = std::min( width, height );
                        }
                        if( std::max( width, height ) <= item.aspectScale )
                        {
                            continue;
                        }
                        fits = false;
                    }
                }
                if( fits )
                {
                    lots.emplace_back( current );
                    continue;
                }
                const auto count = std::ceil( magnitude / limits.maxArea );
                if( count >= static_cast<long double>( lots.max_size() ) )
                {
                    throw std::length_error( "Requested lot area exceeds output capacity" );
                }
                // Give each half a proportional share, including odd lot counts.
                const auto fraction = count > 1 ? std::floor( count / 2 ) / count : .5L;
                splitLot( current, area, fraction, item.aspectScale, pending );
            }
            // Reserve before moving any result so allocation failure leaves the caller's
            // existing entries intact, including when this polygon is itself an entry.
            if( lots.size() > output.max_size() - output.size() )
            {
                throw std::length_error( "Too many lot polygons" );
            }
            output.reserve( output.size() + lots.size() );
            for( auto &lot : lots )
            {
                output.push_back( std::move( lot ) );
            }
        }
    }  // namespace

    template <class T>
    void Polygon2<T>::subDivide( T minArea, T maxArea, Array<Polygon2<T>> &resultPolygons )
    {
        if( !( minArea >= 0 && maxArea > 0 && minArea <= maxArea ) ||
            !std::isfinite( static_cast<long double>( maxArea ) ) )
        {
            throw std::invalid_argument(
                "Lot area limits require 0 <= min <= max and a positive finite max" );
        }
        subdivideLots( m_points,
                       { static_cast<long double>( minArea ), static_cast<long double>( maxArea ) },
                       resultPolygons );
    }

    template <class T>
    void Polygon2<T>::subDivide( const Vector2<T> &min, const Vector2<T> &max,
                                 Array<Polygon2<T>> &resultPolygons )
    {
        if( !( min.x >= 0 && min.y >= 0 && max.x > 0 && max.y > 0 && min.x <= max.x &&
               min.y <= max.y ) ||
            !std::isfinite( static_cast<long double>( max.x ) ) ||
            !std::isfinite( static_cast<long double>( max.y ) ) )
        {
            throw std::invalid_argument(
                "Lot dimensions require 0 <= min <= max and positive finite maxima" );
        }
        const long double minX = min.x, minY = min.y, maxX = max.x, maxY = max.y;
        subdivideLots( m_points, { minX * minY, maxX * maxY, minX, minY, maxX, maxY }, resultPolygons );
    }

    template <class T>
    void Polygon2<T>::changePointOrdering()
    {
        std::reverse( m_points.begin(), m_points.end() );
    }

    template <class T>
    auto Polygon2<T>::getArea() const -> T
    {
        const auto area = signedArea( m_points );
        if constexpr( std::is_integral_v<T> )
        {
            if( std::trunc( area ) < std::numeric_limits<T>::lowest() ||
                std::trunc( area ) > std::numeric_limits<T>::max() )
            {
                throw std::overflow_error( "Polygon area cannot be represented by its coordinate type" );
            }
        }
        return static_cast<T>( area );
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

        Array<Line2<T>> lines{ Allocator<Line2<T>>( sourceBudget ) };
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
        auto candidate = makeSourceBudgetSnapshot( points );
        m_points.swap( candidate );
    }

    template <class T>
    void Polygon2<T>::addPoints( const Array<Vector2<T>> &points )
    {
        const auto count = points.size();
        if( count > std::numeric_limits<size_t>::max() - m_points.size() )
            throw std::length_error( "Polygon point count overflow" );
        m_points.reserve( m_points.size() + count );
        for( size_t i = 0; i < count; ++i ) m_points.push_back( points[i] );
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
        auto candidate = makeSourceBudgetSnapshot( points );
        m_points.swap( candidate );
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

    // explicit instantiation
    template class Polygon2<s32>;
    template class Polygon2<f32>;
    template class Polygon2<f64>;

}  // namespace workphone
