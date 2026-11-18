#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CityRoadBlockFaces.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        using Point = procedural::OpenCityLayout::Point;
        using Path = SourceBudgetVector<Point>;

        Point xz( Vector3F p )
        {
            return { p.x, p.z };
        }

        double distance( Point a, Point b )
        {
            return std::hypot( double( a.x ) - b.x, double( a.z ) - b.z );
        }

        void append( Path &path, Point p )
        {
            if( !path.empty() && distance( path.back(), p ) < .00001 )
            {
                return;
            }
            if( path.size() >= 2 )
            {
                const Point a = path[path.size() - 2], b = path.back();
                const double dx = double( p.x ) - a.x, dz = double( p.z ) - a.z;
                const double bx = double( b.x ) - a.x, bz = double( b.z ) - a.z;
                const double along = bx * dx + bz * dz;
                if( along > 0 && along < dx * dx + dz * dz &&
                    std::abs( bx * dz - bz * dx ) < .00001 * std::hypot( dx, dz ) )
                {
                    path.pop_back();
                }
            }
            path.push_back( p );
        }

        struct Node
        {
            Point point;
            SourceBudgetVector<u32> fan;
        };

        struct Road
        {
            u32 first, last;
            Path path;
        };

        double area( const Path &path )
        {
            double result = 0;
            for( size_t i = 2; i < path.size(); ++i )
            {
                const auto a = path.front(), b = path[i - 1], c = path[i];
                result += ( double( b.x ) - a.x ) * ( double( c.z ) - a.z ) -
                          ( double( c.x ) - a.x ) * ( double( b.z ) - a.z );
            }
            return result * .5;
        }

        // A single editable spline can cross itself without producing a junction
        // in the road bake. That walk has no well-defined simple block interior.
        bool simpleContour( const Path &path )
        {
            struct Segment
            {
                Point a, b;
                size_t index;
                float left, right, bottom, top;
            };

            SourceBudgetVector<Segment> segments;
            segments.reserve( path.size() );
            const auto turn = []( Point a, Point b, Point c ) {
                return ( double( b.x ) - a.x ) * ( double( c.z ) - a.z ) -
                       ( double( b.z ) - a.z ) * ( double( c.x ) - a.x );
            };
            for( size_t i = 0; i < path.size(); ++i )
            {
                const auto a = path[i], b = path[( i + 1 ) % path.size()],
                           c = path[( i + 2 ) % path.size()];
                if( turn( a, b, c ) == 0 && ( double( a.x ) - b.x ) * ( double( c.x ) - b.x ) +
                                                    ( double( a.z ) - b.z ) * ( double( c.z ) - b.z ) >
                                                0 )
                {
                    return false;
                }
                segments.push_back( { a, b, i, std::min( a.x, b.x ), std::max( a.x, b.x ),
                                      std::min( a.z, b.z ), std::max( a.z, b.z ) } );
            }
            std::sort( segments.begin(), segments.end(),
                       []( const auto &a, const auto &b ) { return a.left < b.left; } );
            const auto sameSide = []( double a, double b ) {
                return ( a < 0 && b < 0 ) || ( a > 0 && b > 0 );
            };
            for( auto i = segments.begin(); i != segments.end(); ++i )
            {
                for( auto j = i + 1; j != segments.end() && j->left <= i->right; ++j )
                {
                    if( j->top < i->bottom || i->top < j->bottom ||
                        ( i->index + 1 ) % path.size() == j->index ||
                        ( j->index + 1 ) % path.size() == i->index )
                    {
                        continue;
                    }
                    if( !sameSide( turn( i->a, i->b, j->a ), turn( i->a, i->b, j->b ) ) &&
                        !sameSide( turn( j->a, j->b, i->a ), turn( j->a, j->b, i->b ) ) )
                    {
                        return false;
                    }
                }
            }
            return true;
        }
    }  // namespace

    CityRoadBlockFaces cityRoadBlockFaces( const procedural::CityRoadSplineSource &source,
                                           const procedural::CityRoadSplineBake &bake,
                                           const CityRoadJunctionBake &junctions )
    {
        if( bake.sourceRevision != source.revision || junctions.sourceRevision != source.revision ||
            bake.roads.size() != source.roads.size() )
        {
            throw std::invalid_argument(
                "Block boundaries require the matching road and junction bakes." );
        }

        // A trimmed run starts or ends at a junction port. Each port leads to the
        // same junction centre even when the source splines cross between knots.
        SourceBudgetMap<std::pair<u32, float>, Point> hubs;
        for( const auto &junction : junctions.junctions )
        {
            for( const auto &port : junction.ports )
            {
                hubs.emplace( std::make_pair( port.roadIndex, port.station ), xz( junction.center ) );
            }
        }
        for( const auto &circle : source.roundabouts )
        {
            for( const auto &port : circle.ports )
            {
                const auto it =
                    std::find_if( bake.roads.begin(), bake.roads.end(),
                                  [&]( const auto &road ) { return road.roadId == port.approachId; } );
                if( it == bake.roads.end() || it->points.empty() )
                {
                    continue;
                }
                const auto outward = xz( port.outward ), centre = xz( circle.center );
                const auto scale =
                    ( circle.outerRadius + port.length ) / std::hypot( outward.x, outward.z );
                const Point end{ centre.x + scale * outward.x, centre.z + scale * outward.z };
                const auto &first = it->points.front(), &last = it->points.back();
                const auto &sample =
                    distance( end, xz( first.position ) ) <= distance( end, xz( last.position ) ) ? first
                                                                                                  : last;
                // A disconnected or moved approach must not close a phantom block.
                if( distance( end, xz( sample.position ) ) <= .002 )
                {
                    hubs.emplace( std::make_pair( u32( it - bake.roads.begin() ), sample.distance ),
                                  centre );
                }
            }
        }

        SourceBudgetVector<Node> nodes;
        SourceBudgetVector<Road> roads;
        const auto node = [&]( Point p ) {
            // Only topological endpoints can meet. Close parallel streets and
            // intermediate samples never get snapped together.
            const auto it = std::find_if( nodes.begin(), nodes.end(), [&]( const Node &n ) {
                return distance( p, n.point ) <= .002;
            } );
            if( it != nodes.end() )
            {
                return u32( it - nodes.begin() );
            }
            nodes.push_back( { p, {} } );
            return u32( nodes.size() - 1 );
        };
        for( const auto &run : junctions.roadRuns )
        {
            if( run.points.size() < 2 )
            {
                continue;
            }
            Path path;
            const auto head = hubs.find( { run.roadIndex, run.points.front().distance } );
            const auto tail = hubs.find( { run.roadIndex, run.points.back().distance } );
            if( head != hubs.end() )
            {
                append( path, head->second );
            }
            for( const auto &sample : run.points )
            {
                append( path, xz( sample.position ) );
            }
            if( tail != hubs.end() )
            {
                append( path, tail->second );
            }
            if( path.size() < 2 )
            {
                continue;
            }
            const auto a = node( path.front() ), b = node( path.back() );
            if( a == b && path.size() < 4 )
            {
                continue;
            }
            path.front() = nodes[a].point;
            path.back() = nodes[b].point;
            const auto forward = u32( roads.size() * 2 );
            nodes[a].fan.push_back( forward );
            nodes[b].fan.push_back( forward + 1 );
            roads.push_back( { a, b, std::move( path ) } );
        }

        SourceBudgetVector<u32> next( roads.size() * 2 );
        const auto angle = [&]( u32 edge ) {
            const auto &p = roads[edge / 2].path;
            const Point a = edge & 1 ? p.back() : p.front(), b = edge & 1 ? p[p.size() - 2] : p[1];
            return std::atan2( double( b.z ) - a.z, double( b.x ) - a.x );
        };
        for( auto &n : nodes )
        {
            auto &fan = n.fan;
            std::sort( fan.begin(), fan.end(), [&]( u32 a, u32 b ) {
                const auto aa = angle( a ), ab = angle( b );
                return aa == ab ? a < b : aa < ab;
            } );
            for( size_t i = 0; i < fan.size(); ++i )
            {
                next[fan[i] ^ 1u] = fan[( i + fan.size() - 1 ) % fan.size()];
            }
        }

        CityRoadBlockFaces faces;
        SourceBudgetVector<u8> visited( next.size(), 0 );
        SourceBudgetVector<u32> walk;
        constexpr u32 unused = std::numeric_limits<u32>::max();
        SourceBudgetVector<u32> position( nodes.size(), unused );
        const auto from = [&]( u32 h ) { return h & 1 ? roads[h / 2].last : roads[h / 2].first; };
        for( u32 start = 0; start < next.size(); ++start )
        {
            for( auto h = start; !visited[h]; h = next[h] )
            {
                visited[h] = 1;
                position[from( h )] = u32( walk.size() );
                walk.push_back( h );
                const auto first = position[from( h ^ 1u )];
                if( first == unused )
                {
                    continue;
                }
                // Split walks at repeated junctions. An out-and-back cul-de-sac
                // cancels here, and touching independent loops keep simple rings.
                if( walk.size() != first + 2 || walk[first] != ( h ^ 1u ) )
                {
                    Path boundary;
                    for( size_t i = first; i < walk.size(); ++i )
                    {
                        const auto edge = walk[i];
                        const auto &p = roads[edge / 2].path;
                        for( size_t k = 0; k + 1 < p.size(); ++k )
                        {
                            append( boundary, p[edge & 1 ? p.size() - 1 - k : k] );
                        }
                    }
                    if( boundary.size() >= 3 && area( boundary ) > .000001 && simpleContour( boundary ) )
                    {
                        faces.push_back( std::move( boundary ) );
                    }
                }
                while( walk.size() > first )
                {
                    position[from( walk.back() )] = unused;
                    walk.pop_back();
                }
            }
        }
        std::sort( faces.begin(), faces.end(), []( const Path &a, const Path &b ) {
            const auto areaA = area( a ), areaB = area( b );
            if( areaA != areaB )
            {
                return areaA < areaB;
            }
            return std::lexicographical_compare(
                a.begin(), a.end(), b.begin(), b.end(),
                []( Point p, Point q ) { return p.x < q.x || ( p.x == q.x && p.z < q.z ); } );
        } );
        return faces;
    }
}  // namespace workphone::scene
