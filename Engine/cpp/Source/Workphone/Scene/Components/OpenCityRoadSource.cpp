#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/OpenCityRoadSource.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone::procedural
{
    namespace
    {
        constexpr u32 none = std::numeric_limits<u32>::max();

        struct Point
        {
            double x = 0, z = 0;
        };

        struct Edge
        {
            u32 a = 0, b = 0;
            OpenCityRoadNetwork::RoadClass roadClass = OpenCityRoadNetwork::RoadClass::Local;
            float width = 12;
        };

        struct Graph
        {
            SourceBudgetVector<Point> nodes;
            SourceBudgetVector<Edge> edges;
        };

        struct Junction
        {
            u32 node = 0;
            float radius = 14, island = 8;
            float maximumRoadWidth = 12;
        };

        struct Chain
        {
            u32 a = 0, b = 0;
            SourceBudgetVector<u32> edges;
        };

        void need( bool value, const char *message )
        {
            if( !value )
            {
                throw std::runtime_error( message );
            }
        }

        double distance( Point a, Point b )
        {
            return std::hypot( a.x - b.x, a.z - b.z );
        }

        Point interpolate( Point a, Point b, double t )
        {
            return { a.x + ( b.x - a.x ) * t, a.z + ( b.z - a.z ) * t };
        }

        u64 mix( u64 value )
        {
            value = ( value ^ ( value >> 30 ) ) * 0xbf58476d1ce4e5b9ull;
            value = ( value ^ ( value >> 27 ) ) * 0x94d049bb133111ebull;
            return value ^ ( value >> 31 );
        }

        u64 namespaceHash( const SourceBudgetText &id, u32 seed )
        {
            u64 value = 1469598103934665603ull;
            for( unsigned char c : id )
            {
                value = ( value ^ c ) * 1099511628211ull;
            }
            return mix( value ^ ( u64( seed ) << 32 ) ^ seed );
        }

        u64 pointHash( Point point )
        {
            return mix( u64( std::llround( point.x * 1000 ) ) ) ^
                   mix( u64( std::llround( point.z * 1000 ) ) ^ 0x9e3779b97f4a7c15ull );
        }

        bool before( Point a, Point b )
        {
            return a.x != b.x ? a.x < b.x : a.z < b.z;
        }

        SourceBudgetText guid( u64 space, u64 domain, Point a, Point b = {} )
        {
            u64 first = mix( space ^ domain ^ pointHash( a ) );
            u64 second = mix( space ^ ( domain * 0x9e3779b97f4a7c15ull ) ^ pointHash( b ) ^ first );
            std::array<unsigned char, 16> bytes{};
            for( unsigned i = 0; i < 8; ++i )
            {
                bytes[i] = static_cast<unsigned char>( first >> ( i * 8 ) );
                bytes[i + 8] = static_cast<unsigned char>( second >> ( i * 8 ) );
            }
            bytes[6] = ( bytes[6] & 15 ) | 128;
            bytes[8] = ( bytes[8] & 63 ) | 128;  // Custom UUID version 8.
            constexpr char hex[] = "0123456789abcdef";
            SourceBudgetText result;
            result.reserve( 36 );
            for( unsigned i = 0; i < bytes.size(); ++i )
            {
                if( i == 4 || i == 6 || i == 8 || i == 10 )
                {
                    result.push_back( '-' );
                }
                result.push_back( hex[bytes[i] >> 4] );
                result.push_back( hex[bytes[i] & 15] );
            }
            return result;
        }

        u32 other( Edge edge, u32 node )
        {
            return edge.a == node ? edge.b : edge.a;
        }

        double segmentDistance( Point p, Point a, Point b )
        {
            const double dx = b.x - a.x, dz = b.z - a.z;
            const double t = std::clamp(
                ( ( p.x - a.x ) * dx + ( p.z - a.z ) * dz ) / ( dx * dx + dz * dz ), 0., 1. );
            return distance( p, interpolate( a, b, t ) );
        }

        Graph graphFrom( const OpenCityLayout &city, const OpenCityRoadSourceOptions &options )
        {
            Graph graph;
            if( city.naturalRoads )
            {
                for( const auto &point : city.roads.nodes )
                {
                    graph.nodes.push_back( { point.x, point.z } );
                }
                for( auto edge : city.roads.edges )
                {
                    need( edge.roadClass == OpenCityRoadNetwork::RoadClass::Local ||
                              edge.roadClass == OpenCityRoadNetwork::RoadClass::Collector ||
                              edge.roadClass == OpenCityRoadNetwork::RoadClass::Arterial,
                          "City graph has an unsupported road class." );
                    const float width = std::max( 4.f, options.roadWidth *
                        ( OpenCityRoadNetwork::roadWidth( edge.roadClass ) / 12.f ) );
                    graph.edges.push_back( { edge.a, edge.b, edge.roadClass, width } );
                }
            }
            else
            {
                need( city.blocks >= 6 && city.blocks <= 10 && city.blocks % 2 == 0 &&
                          std::isfinite( city.spacing ) && city.spacing >= 32,
                      "Unsupported grid city dimensions." );
                const u32 side = u32( city.blocks + 1 );
                for( u32 z = 0; z < side; ++z )
                {
                    for( u32 x = 0; x < side; ++x )
                    {
                        graph.nodes.push_back(
                            { ( int( x ) - city.blocks / 2 ) * double( city.spacing ),
                              ( int( z ) - city.blocks / 2 ) * double( city.spacing ) } );
                    }
                }
                for( u32 z = 0; z < side; ++z )
                {
                    for( u32 x = 0; x < side; ++x )
                    {
                        if( x + 1 < side )
                        {
                            graph.edges.push_back( { z * side + x, z * side + x + 1,
                                OpenCityRoadNetwork::RoadClass::Local, options.roadWidth } );
                        }
                        if( z + 1 < side )
                        {
                            graph.edges.push_back( { z * side + x, ( z + 1 ) * side + x,
                                OpenCityRoadNetwork::RoadClass::Local, options.roadWidth } );
                        }
                    }
                }
            }
            need( graph.nodes.size() >= 3 && graph.nodes.size() <= 160 && graph.edges.size() <= 240 &&
                      !graph.edges.empty(),
                  "City graph exceeds the bounded 160-node/240-edge converter." );
            for( const auto &point : graph.nodes )
            {
                need( std::isfinite( point.x ) && std::isfinite( point.z ) &&
                          std::abs( point.x ) < 100000 && std::abs( point.z ) < 100000,
                      "City graph has invalid world coordinates." );
            }
            for( auto edge : graph.edges )
            {
                need( edge.a < graph.nodes.size() && edge.b < graph.nodes.size() && edge.a != edge.b &&
                          distance( graph.nodes[edge.a], graph.nodes[edge.b] ) >= .01,
                      "City graph contains an invalid/zero-length edge." );
            }
            return graph;
        }

        Vector3F grounded( Point point, const CityRoadHeightSampler &terrain, float offset )
        {
            float height = 0;
            need(
                terrain( { float( point.x ), 0, float( point.z ) }, height ) && std::isfinite( height ),
                "Generated road/route leaves supported finite terrain." );
            return { float( point.x ), height + offset, float( point.z ) };
        }

        SourceBudgetVector<Junction> chooseJunctions( const Graph &graph,
                                                      const OpenCityRoadSourceOptions &options,
                                                      const CityRoadHeightSampler &terrain, u64 space )
        {
            SourceBudgetVector<u32> order( graph.nodes.size() );
            for( u32 i = 0; i < order.size(); ++i )
            {
                order[i] = i;
            }
            std::sort( order.begin(), order.end(), [&]( u32 a, u32 b ) {
                const auto pa = graph.nodes[a], pb = graph.nodes[b];
                const auto da = pa.x * pa.x + pa.z * pa.z, db = pb.x * pb.x + pb.z * pb.z;
                return da != db ? da < db
                                : mix( space ^ pointHash( pa ) ) < mix( space ^ pointHash( pb ) );
            } );
            SourceBudgetVector<Junction> selected;
            for( u32 node : order )
            {
                if( selected.size() >= options.maximumRoundabouts )
                {
                    break;
                }
                SourceBudgetVector<u32> incident;
                for( u32 i = 0; i < graph.edges.size(); ++i )
                {
                    if( graph.edges[i].a == node || graph.edges[i].b == node )
                    {
                        incident.push_back( i );
                    }
                }
                if( incident.size() < 3 || incident.size() > 6 )
                {
                    continue;
                }
                float maximumRoadWidth = 0;
                for( const auto index : incident )
                {
                    maximumRoadWidth = std::max( maximumRoadWidth, graph.edges[index].width );
                }
                // The codec requires approach width strictly below outer radius.
                // A 12 m street therefore needs a little more than a 12 m fallback.
                for( const float radius : { 14.f, std::max( 12.f, maximumRoadWidth + .5f ) } )
                {
                    const float trim = radius + 4;
                    const auto center = graph.nodes[node];
                    bool valid = distance( center, {} ) > trim + maximumRoadWidth * .5f + 4;
                    for( auto edgeIndex : incident )
                    {
                        valid = valid &&
                                distance( center, graph.nodes[other( graph.edges[edgeIndex], node )] ) >
                                    trim * 2 + 2;
                    }
                    for( const auto &previous : selected )
                    {
                        valid = valid && distance( center, graph.nodes[previous.node] ) >
                                             trim + previous.radius + 4 +
                                                 std::max( maximumRoadWidth, previous.maximumRoadWidth ) + 6;
                    }
                    for( size_t i = 0; valid && i < incident.size(); ++i )
                    {
                        for( size_t j = 0; j < i; ++j )
                        {
                            const auto a = graph.nodes[other( graph.edges[incident[i]], node )],
                                       b = graph.nodes[other( graph.edges[incident[j]], node )];
                            const double cosine = ( ( a.x - center.x ) * ( b.x - center.x ) +
                                                    ( a.z - center.z ) * ( b.z - center.z ) ) /
                                                  ( distance( a, center ) * distance( b, center ) );
                            const double firstHalf = std::asin( graph.edges[incident[i]].width * .5 / radius );
                            const double secondHalf = std::asin( graph.edges[incident[j]].width * .5 / radius );
                            valid = valid && std::acos( std::clamp( cosine, -1., 1. ) ) >=
                                                 firstHalf + secondHalf + .122;
                        }
                    }
                    for( auto edge : graph.edges )
                    {
                        if( edge.a != node && edge.b != node )
                        {
                            valid = valid && segmentDistance( center, graph.nodes[edge.a],
                                                              graph.nodes[edge.b] ) >
                                                 radius + edge.width * .5f + 1;
                        }
                    }
                    if( !valid )
                    {
                        continue;
                    }
                    const auto h = grounded( center, terrain, options.heightOffset );
                    const auto xp = grounded( { center.x + trim, center.z }, terrain, 0 ),
                               xm = grounded( { center.x - trim, center.z }, terrain, 0 );
                    const auto zp = grounded( { center.x, center.z + trim }, terrain, 0 ),
                               zm = grounded( { center.x, center.z - trim }, terrain, 0 );
                    if( std::hypot( ( xp.y - xm.y ) / ( trim * 2 ), ( zp.y - zm.y ) / ( trim * 2 ) ) >
                        options.maxGrade )
                    {
                        continue;
                    }
                    (void)h;
                    selected.push_back( { node, radius, radius - 6, maximumRoadWidth } );
                    break;
                }
            }
            need( !options.maximumRoundabouts || !selected.empty(),
                  "No safe bounded roundabout junction fits this graph; existing city retained." );
            return selected;
        }

        SourceBudgetVector<Chain> chainsFrom( const Graph &graph,
                                              const SourceBudgetVector<int> &junctionAt )
        {
            SourceBudgetVector<Chain> chains;
            SourceBudgetVector<u8> used( graph.edges.size(), 0 );
            const auto next = [&]( u32 node, u32 incoming ) {
                if( junctionAt[node] >= 0 )
                {
                    return none;
                }
                const auto center = graph.nodes[node],
                           from = graph.nodes[other( graph.edges[incoming], node )];
                const double length = distance( center, from );
                u32 found = none;
                for( u32 i = 0; i < graph.edges.size(); ++i )
                {
                    if( i == incoming || graph.edges[i].roadClass != graph.edges[incoming].roadClass ||
                        ( graph.edges[i].a != node && graph.edges[i].b != node ) )
                    {
                        continue;
                    }
                    const auto to = graph.nodes[other( graph.edges[i], node )];
                    const double second = distance( center, to );
                    const double dot = ( ( from.x - center.x ) * ( to.x - center.x ) +
                                         ( from.z - center.z ) * ( to.z - center.z ) ) /
                                       ( length * second );
                    const double cross = ( ( from.x - center.x ) * ( to.z - center.z ) -
                                           ( from.z - center.z ) * ( to.x - center.x ) ) /
                                         ( length * second );
                    if( dot < -.999999 && std::abs( cross ) < .00001 )
                    {
                        if( found != none )
                        {
                            return none;
                        }
                        found = i;
                    }
                }
                return found;
            };
            for( u32 i = 0; i < graph.edges.size(); ++i )
            {
                if( !used[i] )
                {
                    Chain chain;
                    chain.a = graph.edges[i].a;
                    chain.b = graph.edges[i].b;
                    chain.edges.push_back( i );
                    used[i] = 1;
                    for( unsigned direction = 0; direction < 2; ++direction )
                    {
                        auto &node = direction ? chain.b : chain.a;
                        u32 incoming = i;
                        for( unsigned step = 0; step < graph.edges.size(); ++step )
                        {
                            const auto following = next( node, incoming );
                            if( following == none || used[following] )
                            {
                                break;
                            }
                            used[following] = 1;
                            chain.edges.push_back( following );
                            node = other( graph.edges[following], node );
                            incoming = following;
                        }
                    }
                    need( chain.a != chain.b, "Closed collinear road topology is unsupported." );
                    if( before( graph.nodes[chain.b], graph.nodes[chain.a] ) )
                    {
                        std::swap( chain.a, chain.b );
                    }
                    chains.push_back( std::move( chain ) );
                }
            }
            need( chains.size() <= 128,
                  "Generated topology needs more than v2's 128 editable roads; no branches were "
                  "discarded." );
            return chains;
        }

        Point unit( Point vector )
        {
            const double length = std::hypot( vector.x, vector.z );
            return length > 1e-8 ? Point{ vector.x / length, vector.z / length } : Point{};
        }

        // A degree-two bend is a continuation, not a junction. Both incident
        // roads use the same bisector tangent, while real attachments keep their
        // original approach direction and a straight collar.
        Point endDirection( const Graph &graph, u32 at, Point toward )
        {
            const auto center = graph.nodes[at];
            const auto direction = unit( { toward.x - center.x, toward.z - center.z } );
            std::array<u32, 2> incident{};
            unsigned count = 0;
            for( u32 i = 0; i < graph.edges.size(); ++i )
            {
                if( graph.edges[i].a == at || graph.edges[i].b == at )
                {
                    if( count == incident.size() ) return direction;
                    incident[count++] = i;
                }
            }
            if( count != 2 || distance( center, {} ) < .001 ||
                graph.edges[incident[0]].roadClass != graph.edges[incident[1]].roadClass ) return direction;
            const auto first = graph.nodes[other( graph.edges[incident[0]], at )];
            const auto second = graph.nodes[other( graph.edges[incident[1]], at )];
            const double minimumRun = std::max( 40., double( graph.edges[incident[0]].width * 6 ) );
            if( distance( center, first ) < minimumRun || distance( center, second ) < minimumRun ) return direction;
            const auto a = unit( { first.x - center.x, first.z - center.z } );
            const auto b = unit( { second.x - center.x, second.z - center.z } );
            if( a.x * b.x + a.z * b.z > .05 ) return direction;
            auto tangent = unit( { a.x - b.x, a.z - b.z } );
            if( tangent.x * direction.x + tangent.z * direction.z < 0 )
            {
                tangent.x = -tangent.x;
                tangent.z = -tangent.z;
            }
            return tangent;
        }

        struct CurvePath
        {
            u32 a = 0, b = 0;
            float width = 0;
            SourceBudgetVector<Vector3F> points;
            SourceBudgetVector<u32> edges;
        };

        struct CurveBounds
        {
            double minX, maxX, minZ, maxZ;

            bool contains( Point point ) const
            {
                return point.x >= minX && point.x <= maxX && point.z >= minZ && point.z <= maxZ;
            }
        };

        // Fit inside each existing corridor. Candidate costs depend on actual
        // longitudinal/crossfall samples and elevation-profile roughness, never
        // on random offsets. A flat isolated straight road stays straight.
        bool curveRoad( const Graph &graph, const Chain &chain,
                        const OpenCityRoadSourceOptions &options, const CityRoadHeightSampler &terrain,
                        const SourceBudgetVector<u8> &disabledCorners,
                        const SourceBudgetVector<CurvePath> &previous, CityRoadSpline &road,
                        CurvePath &published )
        {
            const auto a = graph.nodes[chain.a], b = graph.nodes[chain.b];
            const Point start{ road.knots.front().position.x, road.knots.front().position.z };
            const Point finish{ road.knots.back().position.x, road.knots.back().position.z };
            const auto axis = unit( { b.x - a.x, b.z - a.z } );
            const auto station = [&]( Point point ) {
                return ( point.x - start.x ) * axis.x + ( point.z - start.z ) * axis.z;
            };
            const double span = distance( start, finish );
            SourceBudgetVector<Point> pins{ start, finish };
            for( const auto edge : chain.edges )
            {
                for( const auto node : { graph.edges[edge].a, graph.edges[edge].b } )
                {
                    const auto point = graph.nodes[node];
                    const double along = station( point );
                    if( along > .001 && along < span - .001 ) pins.push_back( point );
                }
            }
            std::sort( pins.begin(), pins.end(), [&]( Point left, Point right ) { return station( left ) < station( right ); } );
            pins.erase( std::unique( pins.begin(), pins.end(), []( Point left, Point right ) {
                return distance( left, right ) < .001;
            } ), pins.end() );
            const auto roadClass = graph.edges[chain.edges.front()].roadClass;
            const double maxOffset = roadClass == OpenCityRoadNetwork::RoadClass::Arterial ? 4.
                                     : roadClass == OpenCityRoadNetwork::RoadClass::Collector ? 8. : 12.;
            const double maxDetour = roadClass == OpenCityRoadNetwork::RoadClass::Arterial ? 1.035 : 1.14;
            const auto startDirection = !disabledCorners[chain.a] && distance( start, a ) < .001
                                            ? endDirection( graph, chain.a, b ) : axis;
            auto endDirectionValue = !disabledCorners[chain.b] && distance( finish, b ) < .001
                                         ? endDirection( graph, chain.b, a )
                                                                 : Point{ -axis.x, -axis.z };
            endDirectionValue = { -endDirectionValue.x, -endDirectionValue.z };
            struct EdgeObstacle
            {
                Point a, b;
                CurveBounds bounds;
                double margin = 0;
                std::array<Point, 2> shared;
                unsigned sharedCount = 0;
            };
            struct PathObstacle
            {
                const CurvePath *path = nullptr;
                CurveBounds bounds;
                double margin = 0;
                SourceBudgetVector<Point> shared;
            };
            SourceBudgetVector<u8> ownedEdges( graph.edges.size(), 0 ), ownedNodes( graph.nodes.size(), 0 );
            for( const auto index : chain.edges )
            {
                ownedEdges[index] = 1;
                ownedNodes[graph.edges[index].a] = ownedNodes[graph.edges[index].b] = 1;
            }
            SourceBudgetVector<EdgeObstacle> edgeObstacles;
            edgeObstacles.reserve( graph.edges.size() - chain.edges.size() );
            for( size_t i = 0; i < graph.edges.size(); ++i )
            {
                if( ownedEdges[i] ) continue;
                const auto edge = graph.edges[i];
                EdgeObstacle obstacle;
                obstacle.a = graph.nodes[edge.a]; obstacle.b = graph.nodes[edge.b];
                obstacle.margin = ( road.width + edge.width ) * .5 + 6.5;
                obstacle.bounds = { std::min( obstacle.a.x, obstacle.b.x ) - obstacle.margin,
                                    std::max( obstacle.a.x, obstacle.b.x ) + obstacle.margin,
                                    std::min( obstacle.a.z, obstacle.b.z ) - obstacle.margin,
                                    std::max( obstacle.a.z, obstacle.b.z ) + obstacle.margin };
                for( const auto node : { edge.a, edge.b } )
                    if( ownedNodes[node] ) obstacle.shared[obstacle.sharedCount++] = graph.nodes[node];
                edgeObstacles.push_back( std::move( obstacle ) );
            }
            SourceBudgetVector<PathObstacle> pathObstacles;
            pathObstacles.reserve( previous.size() );
            for( const auto &path : previous )
            {
                if( path.points.empty() ) continue;
                PathObstacle obstacle;
                obstacle.path = &path;
                obstacle.margin = ( road.width + path.width ) * .5 + 6.5;
                obstacle.bounds = { path.points.front().x, path.points.front().x,
                                    path.points.front().z, path.points.front().z };
                for( const auto point : path.points )
                {
                    obstacle.bounds.minX = std::min( obstacle.bounds.minX, double( point.x ) );
                    obstacle.bounds.maxX = std::max( obstacle.bounds.maxX, double( point.x ) );
                    obstacle.bounds.minZ = std::min( obstacle.bounds.minZ, double( point.z ) );
                    obstacle.bounds.maxZ = std::max( obstacle.bounds.maxZ, double( point.z ) );
                }
                obstacle.bounds.minX -= obstacle.margin; obstacle.bounds.maxX += obstacle.margin;
                obstacle.bounds.minZ -= obstacle.margin; obstacle.bounds.maxZ += obstacle.margin;
                for( const auto edge : path.edges ) for( const auto node : { graph.edges[edge].a, graph.edges[edge].b } )
                    if( ownedNodes[node] ) obstacle.shared.push_back( graph.nodes[node] );
                pathObstacles.push_back( std::move( obstacle ) );
            }
            const auto sharedAttachment = []( Point point, const auto &nodes, size_t count, double radius ) {
                for( size_t i = 0; i < count; ++i ) if( distance( point, nodes[i] ) < radius ) return true;
                return false;
            };
            const auto ownerSpace = namespaceHash( road.id, options.seed );
            double bestCost = std::numeric_limits<double>::max();
            CityRoadSpline selected = road;
            SourceBudgetVector<Vector3F> selectedPoints;

            const auto evaluate = [&]( CityRoadSpline &candidate, bool curved ) {
                CityRoadSplineSource isolated;
                isolated.id = options.sourceId;
                isolated.terrainResourceUuid = options.terrainResourceUuid;
                isolated.roads.push_back( candidate );
                CityRoadSplineBake sampled;
                String candidateError;
                if( !sampleCityRoadSplines( isolated, {}, terrain, sampled, candidateError ) )
                {
                    // A resource denial must not silently choose a different
                    // generated shape merely because its candidate was cheaper.
                    if( candidateError == std::bad_alloc().what() ) throw std::bad_alloc();
                    return;
                }
                const auto &points = sampled.roads.front().points;
                if( points.size() < 2 || points.size() > 4096 ) return;
                double length = 0, terrainCost = 0, previousCost = 0;
                const double startHeight = points.front().position.y;
                const double heightDifference = points.back().position.y - startHeight;
                for( size_t p = 0; p < points.size(); ++p )
                {
                    const auto position = points[p].position;
                    const Point point{ position.x, position.z };
                    const double profile = startHeight + heightDifference * std::clamp( station( point ) / span, 0., 1. );
                    const double deviation = position.y - profile;
                    const double crossfall = ( points[p].left.y - points[p].right.y ) / road.width;
                    const double stationCost = deviation * deviation * .002 + crossfall * crossfall * .5;
                    if( p )
                    {
                        const auto last = points[p - 1].position;
                        const double run = std::hypot( position.x - last.x, position.z - last.z );
                        if( run <= 1e-7 ) return;
                        const double grade = ( position.y - last.y ) / run;
                        terrainCost += ( grade * grade + ( stationCost + previousCost ) * .5 ) * run;
                        length += run;
                    }
                    previousCost = stationCost;
                    if( !curved ) continue;
                    // Extra separation reserves both fitted footways and the
                    // sampling tolerance. Shared junction collars intentionally
                    // meet; unrelated corridors may never be crossed by a bow.
                    for( const auto &obstacle : edgeObstacles )
                    {
                        if( !obstacle.bounds.contains( point ) || sharedAttachment( point, obstacle.shared,
                            obstacle.sharedCount, obstacle.margin * 2 ) ) continue;
                        if( segmentDistance( point, obstacle.a, obstacle.b ) < obstacle.margin ) return;
                    }
                    for( const auto &obstacle : pathObstacles )
                    {
                        if( !obstacle.bounds.contains( point ) || sharedAttachment( point, obstacle.shared,
                            obstacle.shared.size(), obstacle.margin * 2 ) ) continue;
                        const auto &path = *obstacle.path;
                        const double margin = obstacle.margin;
                        for( size_t q = 1; q < path.points.size(); ++q )
                        {
                            const auto from = path.points[q - 1], to = path.points[q];
                            if( point.x < std::min( from.x, to.x ) - margin || point.x > std::max( from.x, to.x ) + margin ||
                                point.z < std::min( from.z, to.z ) - margin || point.z > std::max( from.z, to.z ) + margin ) continue;
                            if( segmentDistance( point, { from.x, from.z }, { to.x, to.z } ) < margin ) return;
                        }
                    }
                }
                if( length > span * maxDetour ) return;
                const double cost = terrainCost / length + .025 * ( length / span - 1 );
                if( cost >= bestCost - 1e-10 ) return;
                bestCost = cost;
                selected = candidate;
                selectedPoints.clear();
                selectedPoints.reserve( points.size() );
                for( const auto &point : points ) selectedPoints.push_back( point.position );
            };

            // The shape at degree-two corners is intrinsic to their continuation;
            // on open spans, zero lateral displacement wins an equal terrain cost.
            bool canDisplace = false;
            for( size_t i = 1; i < pins.size(); ++i ) canDisplace = canDisplace || distance( pins[i - 1], pins[i] ) >= 40;
            for( const double amount : { 0., -.5, .5, -1., 1. } )
            {
                if( amount != 0 && !canDisplace ) continue;
                CityRoadSpline candidate = road;
                candidate.knots.clear();
                candidate.knots.reserve( pins.size() * 4 );
                bool supported = true;
                const auto append = [&]( Point point, Point outgoing, Point incoming ) {
                    if( !supported ) return;
                    float height = 0;
                    if( !terrain( { float( point.x ), 0, float( point.z ) }, height ) || !std::isfinite( height ) )
                    {
                        supported = false;
                        return;
                    }
                    const Vector3F position{ float( point.x ), height, float( point.z ) };
                    if( candidate.knots.empty() )
                    {
                        candidate.knots.push_back( { {}, position, {}, {} } );
                        return;
                    }
                    auto &last = candidate.knots.back();
                    const double run = std::hypot( position.x - last.position.x, position.z - last.position.z );
                    if( run < .001 ) return;
                    const float vertical = ( position.y - last.position.y ) / 3.f;
                    last.outHandle = { float( outgoing.x * run / 3 ), vertical, float( outgoing.z * run / 3 ) };
                    candidate.knots.push_back( { {}, position,
                        { float( -incoming.x * run / 3 ), -vertical, float( -incoming.z * run / 3 ) }, {} } );
                };
                append( start, axis, axis );
                for( size_t interval = 1; interval < pins.size(); ++interval )
                {
                    const auto first = pins[interval - 1], last = pins[interval];
                    const double run = distance( first, last );
                    const auto firstDirection = interval == 1 ? startDirection : axis;
                    const auto lastDirection = interval + 1 == pins.size() ? endDirectionValue : axis;
                    const bool smoothFirst = std::abs( firstDirection.x * axis.z - firstDirection.z * axis.x ) > .001;
                    const bool smoothLast = std::abs( lastDirection.x * axis.z - lastDirection.z * axis.x ) > .001;
                    const double collarA = smoothFirst ? 0 : std::min( 10., run * .18 );
                    const double collarB = smoothLast ? 0 : std::min( 10., run * .18 );
                    const Point innerA{ first.x + axis.x * collarA, first.z + axis.z * collarA };
                    const Point innerB{ last.x - axis.x * collarB, last.z - axis.z * collarB };
                    if( collarA > 0 ) append( innerA, axis, axis );
                    auto middle = interpolate( innerA, innerB, .5 );
                    const double displacement = run >= 40 ? amount * std::min( maxOffset, run * .1 ) : 0;
                    middle.x -= axis.z * displacement;
                    middle.z += axis.x * displacement;
                    append( middle, smoothFirst ? firstDirection : axis, axis );
                    append( innerB, axis, smoothLast ? lastDirection : axis );
                    if( collarB > 0 ) append( last, axis, axis );
                }
                if( !supported ) continue;
                need( candidate.knots.size() <= 256, "Generated curved road exceeded its 256-knot budget." );
                for( size_t k = 0; k < candidate.knots.size(); ++k )
                    candidate.knots[k].id = guid( ownerSpace, 0x4355525600ull + k, a, b );
                candidate.knots.front().id = road.knots.front().id;
                candidate.knots.back().id = road.knots.back().id;
                evaluate( candidate, true );
            }
            // Geometry or terrain may leave no safe room for a curve. Retain the
            // full straight road rather than dropping an unsafe branch.
            const bool fitted = !selectedPoints.empty();
            if( !fitted ) evaluate( road, false );
            road = std::move( selected );
            published.a = chain.a;
            published.b = chain.b;
            published.width = road.width;
            published.points = std::move( selectedPoints );
            published.edges = chain.edges;
            return fitted;
        }

        bool followPinnedCurves( const OpenCityLayout &original, const CityRoadSplineSource &source,
                                 const CityRoadSplineBake &bake, OpenCityLayout &routed )
        {
            struct Projection
            {
                Point start, axis;
                double length = 0;
                bool curved = false, monotonic = true;
                SourceBudgetVector<double> stations;
            };
            need( bake.roads.size() == source.roads.size(), "Pinned curve bake does not match its source." );
            SourceBudgetVector<Projection> projections;
            projections.reserve( source.roads.size() );
            for( size_t r = 0; r < source.roads.size(); ++r )
            {
                Projection projection;
                const auto &road = source.roads[r];
                const auto first = road.knots.front().position, last = road.knots.back().position;
                projection.start = { first.x, first.z };
                projection.axis = unit( { last.x - first.x, last.z - first.z } );
                projection.length = std::hypot( last.x - first.x, last.z - first.z );
                if( !road.closed && projection.length > .001 )
                {
                    for( const auto &sample : bake.roads[r].points )
                    {
                        const double dx = sample.position.x - first.x, dz = sample.position.z - first.z;
                        projection.curved = projection.curved ||
                            std::abs( dx * projection.axis.z - dz * projection.axis.x ) > .001;
                    }
                    if( projection.curved )
                    {
                        projection.stations.reserve( bake.roads[r].points.size() );
                        for( const auto &sample : bake.roads[r].points )
                        {
                            const double station = ( sample.position.x - first.x ) * projection.axis.x +
                                                   ( sample.position.z - first.z ) * projection.axis.z;
                            if( !projection.stations.empty() && station <= projection.stations.back() + 1e-7 )
                                projection.monotonic = false;
                            projection.stations.push_back( station );
                        }
                    }
                }
                projections.push_back( std::move( projection ) );
            }
            // Retain the exact earlier route sampling for a wholly straight
            // source, including the explicit legacy opt-out.
            if( std::none_of( projections.begin(), projections.end(),
                              []( const Projection &projection ) { return projection.curved; } ) ) return false;
            need( original.route.size() >= 3 && original.route.size() <= 8192,
                  "City race route exceeds the converter's 8192-point input budget." );
            for( const auto point : original.route )
                need( std::isfinite( point.x ) && std::isfinite( point.z ), "City route contains a nonfinite point." );
            routed.route.reserve( std::min( size_t( 8192 ), original.route.size() * 2 ) );
            const auto append = [&]( OpenCityLayout::Point point )
            {
                Point mapped{ point.x, point.z };
                bool found = false;
                for( size_t r = 0; r < projections.size(); ++r )
                {
                    const auto &projection = projections[r];
                    if( !projection.curved ) continue;
                    const double dx = point.x - projection.start.x, dz = point.z - projection.start.z;
                    const double station = dx * projection.axis.x + dz * projection.axis.z;
                    if( station < -.001 || station > projection.length + .001 ||
                        std::abs( dx * projection.axis.z - dz * projection.axis.x ) > .002 ) continue;
                    need( projection.monotonic, "Edited road folds back across the pinned route stations." );
                    const double clamped = std::clamp( station, 0., projection.length );
                    const auto upper = std::upper_bound( projection.stations.begin(), projection.stations.end(), clamped );
                    const size_t after = std::clamp( size_t( upper - projection.stations.begin() ),
                                                    size_t( 1 ), projection.stations.size() - 1 );
                    const size_t before = after - 1;
                    const double length = projection.stations[after] - projection.stations[before];
                    need( length > 0, "Pinned curve station interval is empty." );
                    const double t = std::clamp( ( clamped - projection.stations[before] ) / length, 0., 1. );
                    const auto a = bake.roads[r].points[before].position, b = bake.roads[r].points[after].position;
                    const Point candidate{ a.x + ( b.x - a.x ) * t, a.z + ( b.z - a.z ) * t };
                    need( !found || distance( mapped, candidate ) < .002,
                          "Several edited road curves ambiguously cover the pinned route." );
                    mapped = candidate;
                    found = true;
                }
                if( routed.route.empty() && distance( mapped, {} ) < .001 ) mapped = {};
                need( routed.route.size() < 8192, "Pinned curve mapping exceeds its 8192-station budget." );
                routed.route.push_back( { float( mapped.x ), float( mapped.z ) } );
            };
            // Sparse pinned routes must follow the same interior curve as dense
            // generated routes; mapping endpoints alone would cut straight
            // across their bend before adaptRoute performs final resampling.
            for( size_t i = 0; i < original.route.size(); ++i )
            {
                const auto a = original.route[i], b = original.route[( i + 1 ) % original.route.size()];
                const double length = std::hypot( b.x - a.x, b.z - a.z );
                if( length < 1e-7 ) continue;
                need( length <= 16384, "Pinned curve mapping span exceeds its station budget." );
                const unsigned steps = unsigned( std::max( 1., std::ceil( length / 2 ) ) );
                for( unsigned step = 0; step < steps; ++step )
                {
                    const float t = float( step ) / steps;
                    append( { a.x + ( b.x - a.x ) * t, a.z + ( b.z - a.z ) * t } );
                }
            }
            return true;
        }

        struct Event
        {
            double arc = 0;
            u32 junction = 0;
            bool entry = false;
            Point position;
            u32 port = none;
        };

        void adaptRoute( const OpenCityLayout &city, const OpenCityRoadSourceOptions &options,
                         const CityRoadHeightSampler &terrain,
                         const SourceBudgetVector<CityRoadRoundaboutBake> &junctions,
                         OpenCityRoadSourceResult &result )
        {
            need( city.route.size() >= 3 && city.route.size() <= 8192,
                  "City race route exceeds the converter's 8192-point input budget." );
            SourceBudgetVector<Point> path;
            for( const auto &p : city.route )
            {
                need( std::isfinite( p.x ) && std::isfinite( p.z ),
                      "City route contains a nonfinite point." );
                if( path.empty() || distance( path.back(), { p.x, p.z } ) > .00001 )
                {
                    path.push_back( { p.x, p.z } );
                }
            }
            need( distance( path.front(), {} ) < .001,
                  "Generated city route must begin at the origin." );
            if( distance( path.front(), path.back() ) > .00001 )
            {
                path.push_back( path.front() );
            }
            SourceBudgetVector<double> cumulative( path.size(), 0 );
            SourceBudgetVector<Event> events;
            for( size_t i = 1; i < path.size(); ++i )
            {
                const auto a = path[i - 1], b = path[i];
                const double length = distance( a, b );
                cumulative[i] = cumulative[i - 1] + length;
                for( u32 j = 0; j < result.source.roundabouts.size(); ++j )
                {
                    const auto &spec = result.source.roundabouts[j];
                    const double radius = spec.outerRadius;
                    const double dx = b.x - a.x, dz = b.z - a.z, ox = a.x - spec.center.x,
                                 oz = a.z - spec.center.z;
                    const double ab = dx * dx + dz * dz, bb = 2 * ( ox * dx + oz * dz ),
                                 cc = ox * ox + oz * oz - radius * radius;
                    const double discriminant = bb * bb - 4 * ab * cc;
                    if( discriminant <= 1e-9 )
                    {
                        continue;
                    }
                    const double root = std::sqrt( discriminant );
                    const std::array<double, 2> times{ ( -bb - root ) / ( 2 * ab ),
                                                       ( -bb + root ) / ( 2 * ab ) };
                    for( unsigned k = 0; k < 2; ++k )
                    {
                        if( times[k] >= -1e-9 && times[k] < 1 - 1e-9 )
                        {
                            need( events.size() < 256, "Race route has too many roundabout crossings." );
                            const double t = std::clamp( times[k], 0., 1. );
                            events.push_back(
                                { cumulative[i - 1] + t * length, j, k == 0, interpolate( a, b, t ) } );
                        }
                    }
                }
            }
            // First find crossings of the ring, then move each cut out to that
            // port's actual endpoint. A single circle cannot represent unequal
            // approach lengths and would retain centreline points over refuges.
            for( auto &event : events )
            {
                const auto &spec = result.source.roundabouts[event.junction];
                const double dx = event.position.x - spec.center.x,
                             dz = event.position.z - spec.center.z;
                const double run = std::hypot( dx, dz );
                for( u32 i = 0; i < spec.ports.size(); ++i )
                {
                    const auto &port = spec.ports[i];
                    const double directionLength = std::hypot( port.outward.x, port.outward.z );
                    if( ( dx * port.outward.x + dz * port.outward.z ) / ( run * directionLength ) >
                        .999999 )
                    {
                        event.port = i;
                        break;
                    }
                }
                need( event.port != none, "Race route misses a solved roundabout approach port." );
                const auto &port = spec.ports[event.port];
                event.arc += event.entry ? -port.length : port.length;
                need( event.arc >= 0 && event.arc <= cumulative.back(),
                      "Edited roundabout approach consumes the race start or end." );
                const auto upper = std::upper_bound( cumulative.begin(), cumulative.end(), event.arc );
                const size_t after = std::min( size_t( upper - cumulative.begin() ), path.size() - 1 );
                const size_t previous = after ? after - 1 : 0;
                const double span = cumulative[after] - cumulative[previous];
                event.position = span > 0 ? interpolate( path[previous], path[after],
                                                         ( event.arc - cumulative[previous] ) / span )
                                          : path[after];
                const double scale =
                    ( spec.outerRadius + port.length ) / std::hypot( port.outward.x, port.outward.z );
                const Point expected{ spec.center.x + port.outward.x * scale,
                                      spec.center.z + port.outward.z * scale };
                need( distance( event.position, expected ) <= .001,
                      "Pinned route does not follow the full edited roundabout approach." );
            }
            std::sort( events.begin(), events.end(),
                       []( const Event &a, const Event &b ) { return a.arc < b.arc; } );
            const auto append = [&]( Vector3F point, bool onJunction ) {
                if( result.route.empty() )
                {
                    result.route.push_back( point );
                    return;
                }
                const auto previous = result.route.back();
                const float run = std::hypot( point.x - previous.x, point.z - previous.z );
                if( run < .00001f )
                {
                    return;
                }
                const unsigned steps = unsigned( std::ceil( run / 1.5f ) );
                need( result.route.size() + steps <= 16384,
                      "Adapted race route exceeds its 16384-point output budget." );
                for( unsigned step = 1; step <= steps; ++step )
                {
                    const auto p = previous + ( point - previous ) * ( float( step ) / steps );
                    result.route.push_back(
                        onJunction ? p : grounded( { p.x, p.z }, terrain, options.heightOffset ) );
                }
            };
            append( grounded( path.front(), terrain, options.heightOffset ), false );
            size_t cursor = 1;
            for( size_t e = 0; e < events.size(); e += 2 )
            {
                need( e + 1 < events.size() && events[e].entry && !events[e + 1].entry &&
                          events[e].junction == events[e + 1].junction,
                      "Race route has ambiguous/overlapping roundabout entry topology." );
                const auto &entry = events[e], &exit = events[e + 1];
                const auto &spec = result.source.roundabouts[entry.junction];
                while( cursor < path.size() && cumulative[cursor] < entry.arc - 1e-7 )
                {
                    append( grounded( path[cursor++], terrain, options.heightOffset ), false );
                }
                append( grounded( entry.position, terrain, options.heightOffset ), false );
                const auto &entryId = spec.ports[entry.port].approachId,
                           &exitId = spec.ports[exit.port].approachId;
                bool found = false;
                for( const auto &route : junctions[entry.junction].routes )
                {
                    if( route.entryApproachId == entryId && route.exitApproachId == exitId )
                    {
                        for( const auto &point : route.points )
                        {
                            append( point, true );
                        }
                        found = true;
                        break;
                    }
                }
                need( found,
                      "Race route requires an unsupported roundabout U-turn or missing movement." );
                append( grounded( exit.position, terrain, options.heightOffset ), true );
                while( cursor < path.size() && cumulative[cursor] <= exit.arc + 1e-7 )
                {
                    ++cursor;
                }
            }
            while( cursor < path.size() )
            {
                append( grounded( path[cursor++], terrain, options.heightOffset ), false );
            }
            need( result.route.size() > 2 &&
                      ( result.route.front() - result.route.back() ).length() < .001f,
                  "Adapted city race route is not closed." );
            for( const auto &point : result.route )
            {
                for( const auto &junction : result.source.roundabouts )
                {
                    need( std::hypot( point.x - junction.center.x, point.z - junction.center.z ) >=
                              junction.islandRadius - .001f,
                          "Adapted race route crosses a raised central island." );
                }
            }
        }
    }  // namespace

    void OpenCityRoadSourceResult::swap( OpenCityRoadSourceResult &other ) noexcept
    {
        source.swap( other.source );
        route.swap( other.route );
        std::swap( inputNodes, other.inputNodes );
        std::swap( inputEdges, other.inputEdges );
        std::swap( mergedChains, other.mergedChains );
    }

    bool buildOpenCityRoadSource( const OpenCityLayout &city, const OpenCityRoadSourceOptions &options,
                                  const CityRoadHeightSampler &terrain, OpenCityRoadSourceResult &output,
                                  String &error )
    {
        try
        {
            need( bool( terrain ) && options.revision > 0 && std::isfinite( options.roadWidth ) &&
                      options.roadWidth >= 4 && options.roadWidth <= 12 &&
                      std::isfinite( options.maxGrade ) && options.maxGrade > 0 &&
                      options.maxGrade <= .15f && std::isfinite( options.heightOffset ) &&
                      std::abs( options.heightOffset ) <= .03f && options.maximumRoundabouts <= 8,
                  "Invalid generated road-source conversion options." );
            OpenCityRoadSourceResult candidate;
            candidate.source.version = 2;
            candidate.source.id = options.sourceId;
            candidate.source.terrainResourceUuid = options.terrainResourceUuid;
            candidate.source.revision = options.revision;
            candidate.source.terrainRevision = options.terrainRevision;
            if( !validateCityRoadSplineSource( candidate.source, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            need( !options.terrainResourceUuid.empty(),
                  "Generated city roads require a stable terrain binding UUID." );
            const auto graph = graphFrom( city, options );
            const auto space = namespaceHash( options.sourceId, options.seed );
            candidate.inputNodes = u32( graph.nodes.size() );
            candidate.inputEdges = u32( graph.edges.size() );
            const auto junctions = chooseJunctions( graph, options, terrain, space );
            SourceBudgetVector<int> junctionAt( graph.nodes.size(), -1 );
            for( u32 i = 0; i < junctions.size(); ++i )
            {
                junctionAt[junctions[i].node] = int( i );
            }
            auto chains = chainsFrom( graph, junctionAt );
            candidate.mergedChains = u32( graph.edges.size() - chains.size() );
            SourceBudgetVector<u32> roadAtEdge( graph.edges.size(), none );
            SourceBudgetVector<std::array<float, 2>> portLengths;
            for( const auto &chain : chains )
            {
                const auto a = graph.nodes[chain.a], b = graph.nodes[chain.b];
                const float roadWidth = graph.edges[chain.edges.front()].width;
                const double length = distance( a, b );
                const auto radiusAt = [&]( u32 node ) {
                    return junctionAt[node] < 0 ? 0.f : junctions[junctionAt[node]].radius;
                };
                const float ra = radiusAt( chain.a ), rb = radiusAt( chain.b );
                const double minimumA = ra > 0 ? ra + 4 : 0, minimumB = rb > 0 ? rb + 4 : 0;
                const auto requestedExtra = [&]( Point point, float radius ) {
                    if( radius <= 0 )
                    {
                        return 0.;
                    }
                    const double resetMargin = 4 + roadWidth * .5;
                    return std::clamp( distance( point, {} ) - radius - resetMargin, 4., 18. ) - 4;
                };
                const double wantedA = requestedExtra( a, ra ), wantedB = requestedExtra( b, rb );
                // Solve both ends together. Equal requests receive equal shares;
                // a reset-limited end leaves the remaining room to its neighbour.
                const double available = ra > 0 || rb > 0 ? length - minimumA - minimumB - 4.01 : 0;
                need( available >= 0, "Selected roundabouts leave less than 4 m of source road." );
                double extraA = wantedA, extraB = wantedB;
                if( extraA + extraB > available )
                {
                    const double half = available * .5;
                    if( wantedA <= half )
                    {
                        extraB = std::min( wantedB, available - wantedA );
                    }
                    else if( wantedB <= half )
                    {
                        extraA = std::min( wantedA, available - wantedB );
                    }
                    else
                    {
                        extraA = extraB = half;
                    }
                }
                const std::array<float, 2> lengths{ ra > 0 ? float( 4 + extraA ) : 0.f,
                                                    rb > 0 ? float( 4 + extraB ) : 0.f };
                const double ta = ra > 0 ? ra + lengths[0] : 0, tb = rb > 0 ? rb + lengths[1] : 0;
                need( ( ra <= 0 && rb <= 0 ) || length - ta - tb >= 4,
                      "Roundabout approach rounding exhausted the remaining source road." );
                const auto start = interpolate( a, b, ta / length ),
                           finish = interpolate( a, b, 1 - tb / length );
                auto startPoint = grounded( start, terrain, 0 ),
                     finishPoint = grounded( finish, terrain, 0 );
                const auto tangent = ( finishPoint - startPoint ) / 3.f;
                CityRoadSpline road;
                road.id = guid( space, 0x524f4144, a, b );
                road.width = roadWidth;
                road.maxGrade = options.maxGrade;
                road.heightOffset = options.heightOffset;
                // Controls belong to their road, even when ordinary graph
                // junctions share endpoint positions. Do not XOR the point hash
                // into the namespace: guid's own point hash could cancel it and
                // alias controls on parallel grid roads with equal spans.
                const auto ownerSpace = namespaceHash( road.id, options.seed );
                road.knots.push_back(
                    { guid( ownerSpace, 0x4b4e4f5430, a, b ), startPoint, {}, tangent } );
                road.knots.push_back(
                    { guid( ownerSpace, 0x4b4e4f5431, a, b ), finishPoint, -tangent, {} } );
                for( auto edge : chain.edges )
                {
                    roadAtEdge[edge] = u32( candidate.source.roads.size() );
                }
                candidate.source.roads.push_back( std::move( road ) );
                portLengths.push_back( lengths );
            }
            if( city.naturalRoads && options.terrainCurves )
            {
                const auto straightRoads = candidate.source.roads;
                SourceBudgetVector<u8> disabledCorners( graph.nodes.size(), 0 );
                SourceBudgetVector<CurvePath> curvePaths;
                curvePaths.reserve( chains.size() );
                // A corner belongs to both incident roads. If one cannot keep
                // its shared tangent safely, refit both with the original
                // approach direction. Each retry disables at least one node,
                // giving this reconciliation a strict graph-sized bound.
                for( size_t pass = 0; pass <= graph.nodes.size(); ++pass )
                {
                    curvePaths.clear();
                    bool changed = false;
                    for( size_t r = 0; r < chains.size(); ++r )
                    {
                        const auto &chain = chains[r];
                        auto &road = candidate.source.roads[r];
                        road = straightRoads[r];
                        CurvePath path;
                        if( !curveRoad( graph, chain, options, terrain, disabledCorners,
                                        curvePaths, road, path ) )
                        {
                            const auto disable = [&]( u32 node, u32 otherNode, const Vector3F &endpoint ) {
                                if( disabledCorners[node] ||
                                    distance( graph.nodes[node], { endpoint.x, endpoint.z } ) > .001 ) return;
                                const auto toward = graph.nodes[otherNode];
                                const auto axis = unit( { toward.x - graph.nodes[node].x,
                                                          toward.z - graph.nodes[node].z } );
                                const auto tangent = endDirection( graph, node, toward );
                                if( std::abs( axis.x * tangent.z - axis.z * tangent.x ) <= .001 ) return;
                                disabledCorners[node] = 1;
                                changed = true;
                            };
                            disable( chain.a, chain.b, road.knots.front().position );
                            disable( chain.b, chain.a, road.knots.back().position );
                        }
                        curvePaths.push_back( std::move( path ) );
                    }
                    if( !changed ) break;
                    need( pass < graph.nodes.size(), "Generated road corner reconciliation did not converge." );
                }
            }
            for( const auto &junction : junctions )
            {
                CityRoadRoundaboutSpec spec;
                const auto center = graph.nodes[junction.node];
                const float trim = junction.radius + 4;
                spec.junctionId = guid( space, 0x4a554e4354, center );
                spec.center = grounded( center, terrain, options.heightOffset );
                spec.outerRadius = junction.radius;
                spec.islandRadius = junction.island;
                spec.clockwise = options.clockwise;
                spec.maxGrade = options.maxGrade;
                spec.maxTerrainDeviation = .08f;
                spec.gradeX = ( grounded( { center.x + trim, center.z }, terrain, 0 ).y -
                                grounded( { center.x - trim, center.z }, terrain, 0 ).y ) /
                              ( 2 * trim );
                spec.gradeZ = ( grounded( { center.x, center.z + trim }, terrain, 0 ).y -
                                grounded( { center.x, center.z - trim }, terrain, 0 ).y ) /
                              ( 2 * trim );
                for( u32 i = 0; i < graph.edges.size(); ++i )
                {
                    if( graph.edges[i].a == junction.node || graph.edges[i].b == junction.node )
                    {
                        const auto neighbor = graph.nodes[other( graph.edges[i], junction.node )];
                        const double length = distance( center, neighbor );
                        CityRoadRoundaboutPort port;
                        port.approachId = candidate.source.roads[roadAtEdge[i]].id;
                        port.outward = { float( ( neighbor.x - center.x ) / length ), 0,
                                         float( ( neighbor.z - center.z ) / length ) };
                        const auto roadIndex = roadAtEdge[i];
                        const auto &chain = chains[roadIndex];
                        port.width = candidate.source.roads[roadIndex].width;
                        port.length = portLengths[roadIndex][chain.a == junction.node ? 0 : 1];
                        spec.ports.push_back( std::move( port ) );
                    }
                }
                candidate.source.roundabouts.push_back( std::move( spec ) );
            }
            if( !validateCityRoadSplineSource( candidate.source, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            if( !fitOpenCityRoadRoute( city, candidate.source, terrain, candidate.route, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            output.swap( candidate );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool fitOpenCityRoadRoute( const OpenCityLayout &original, const CityRoadSplineSource &current,
                               const CityRoadHeightSampler &terrain,
                               SourceBudgetVector<Vector3F> &output, String &error )
    {
        try
        {
            need( bool( terrain ) && !current.roads.empty(),
                  "Pinned city race route is unavailable without supported roads and terrain." );
            CityRoadSplineBake roads;
            if( !sampleCityRoadSplines( current, {}, terrain, roads, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            SourceBudgetVector<CityRoadRoundaboutBake> junctions;
            for( const auto &spec : current.roundabouts )
            {
                CityRoadRoundaboutBake bake;
                if( !bakeCityRoadRoundabout( spec, terrain, bake, error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                junctions.push_back( std::move( bake ) );
            }
            OpenCityRoadSourceResult candidate;
            candidate.source = current;
            OpenCityRoadSourceOptions options;
            options.heightOffset = current.roads.front().heightOffset;
            OpenCityLayout curvedRoute;
            const bool followedCurves = followPinnedCurves( original, current, roads, curvedRoute );
            adaptRoute( followedCurves ? curvedRoute : original, options, terrain, junctions, candidate );

            struct Bounds
            {
                float minX, maxX, minZ, maxZ;
            };

            SourceBudgetVector<Bounds> bounds;
            SourceBudgetVector<Bounds> segmentBounds;
            SourceBudgetVector<size_t> segmentOffsets;
            size_t segmentCount = 0;
            for( const auto &road : roads.roads )
            {
                segmentCount += road.points.size() - 1;
            }
            need( segmentCount <= 65536, "Pinned road segment index exceeds its sample budget." );
            bounds.reserve( roads.roads.size() );
            segmentOffsets.reserve( roads.roads.size() );
            segmentBounds.reserve( segmentCount );
            const auto contains = []( const Bounds &box, Vector3F point, float tolerance ) {
                return point.x >= box.minX - tolerance && point.x <= box.maxX + tolerance &&
                       point.z >= box.minZ - tolerance && point.z <= box.maxZ + tolerance;
            };
            const auto intersects = []( const Bounds &a, const Bounds &b ) {
                return a.minX <= b.maxX && a.maxX >= b.minX && a.minZ <= b.maxZ && a.maxZ >= b.minZ;
            };
            const auto quadBounds = []( const CityRoadSplineSample &a, const CityRoadSplineSample &b ) {
                return Bounds{ std::min( { a.left.x, a.right.x, b.left.x, b.right.x } ),
                               std::max( { a.left.x, a.right.x, b.left.x, b.right.x } ),
                               std::min( { a.left.z, a.right.z, b.left.z, b.right.z } ),
                               std::max( { a.left.z, a.right.z, b.left.z, b.right.z } ) };
            };
            for( const auto &road : roads.roads )
            {
                Bounds box{ std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
                for( const auto &p : road.points )
                {
                    for( const auto v : { p.left, p.right } )
                    {
                        box.minX = std::min( box.minX, v.x );
                        box.maxX = std::max( box.maxX, v.x );
                        box.minZ = std::min( box.minZ, v.z );
                        box.maxZ = std::max( box.maxZ, v.z );
                    }
                }
                bounds.push_back( box );
                segmentOffsets.push_back( segmentBounds.size() );
                for( size_t i = 1; i < road.points.size(); ++i )
                {
                    need( segmentBounds.size() < 65536,
                          "Pinned road segment index exceeds its sample budget." );
                    segmentBounds.push_back( quadBounds( road.points[i - 1], road.points[i] ) );
                }
            }

            struct JunctionBounds
            {
                Bounds box;
                double squaredExtent;
            };

            SourceBudgetVector<JunctionBounds> junctionBounds;
            junctionBounds.reserve( junctions.size() );
            for( const auto &spec : current.roundabouts )
            {
                float extent = spec.outerRadius;
                for( const auto &port : spec.ports )
                {
                    extent = std::max( extent, spec.outerRadius + port.length + port.width * .5f );
                }
                extent += .001f;
                junctionBounds.push_back( { { spec.center.x - extent, spec.center.x + extent,
                                              spec.center.z - extent, spec.center.z + extent },
                                            double( extent ) * extent } );
            }
            constexpr u64 maximumGeometryVisits = 64000000;
            u64 geometryVisits = 0;
            const auto work = [&]( u64 amount = 1 ) {
                need( amount <= maximumGeometryVisits - geometryVisits,
                      "Pinned route geometric query work budget exceeded." );
                geometryVisits += amount;
            };
            const auto inTriangle = []( Vector3F p, Vector3F a, Vector3F b, Vector3F c, float &height ) {
                const double denominator =
                    ( b.z - c.z ) * double( a.x - c.x ) + ( c.x - b.x ) * double( a.z - c.z );
                if( std::abs( denominator ) < 1e-10 )
                {
                    return false;
                }
                const double wa =
                    ( ( b.z - c.z ) * double( p.x - c.x ) + ( c.x - b.x ) * double( p.z - c.z ) ) /
                    denominator;
                const double wb =
                    ( ( c.z - a.z ) * double( p.x - c.x ) + ( a.x - c.x ) * double( p.z - c.z ) ) /
                    denominator;
                if( wa < -1e-5 || wb < -1e-5 || wa + wb > 1.00001 )
                {
                    return false;
                }
                height = float( wa * a.y + wb * b.y + ( 1 - wa - wb ) * c.y );
                return true;
            };
            const auto fit = [&]( Vector3F &point ) {
                bool supported = false;
                float top = -std::numeric_limits<float>::max();
                for( size_t r = 0; r < roads.roads.size(); ++r )
                {
                    const auto &box = bounds[r];
                    if( !contains( box, point, .001f ) )
                    {
                        continue;
                    }
                    const auto &samples = roads.roads[r].points;
                    for( size_t i = 1; i < samples.size(); ++i )
                    {
                        work();
                        if( !contains( segmentBounds[segmentOffsets[r] + i - 1], point, .001f ) )
                        {
                            continue;
                        }
                        float height = 0;
                        if( !inTriangle( point, samples[i - 1].left, samples[i - 1].right,
                                         samples[i].left, height ) &&
                            !inTriangle( point, samples[i - 1].right, samples[i].right, samples[i].left,
                                         height ) )
                        {
                            continue;
                        }
                        if( current.roads[r].heightMode == CityRoadHeightMode::Drape )
                        {
                            float ground = 0;
                            need( terrain( point, ground ) && std::isfinite( ground ),
                                  "Pinned race route crosses unsupported terrain." );
                            height = ground + current.roads[r].heightOffset;
                        }
                        top = std::max( top, height );
                        supported = true;
                        break;
                    }
                }
                for( size_t j = 0; j < junctions.size(); ++j )
                {
                    const auto &spec = current.roundabouts[j];
                    const auto &box = junctionBounds[j];
                    if( !contains( box.box, point, 0 ) )
                    {
                        continue;
                    }
                    const double dx = point.x - double( spec.center.x ),
                                 dz = point.z - double( spec.center.z );
                    if( dx * dx + dz * dz > box.squaredExtent )
                    {
                        continue;
                    }
                    work( junctions[j].triangleKinds.size() * 2 );
                    if( !junctions[j].containsSurface( point.x, point.z ) )
                    {
                        continue;
                    }
                    top = std::max( top, spec.center.y + spec.gradeX * ( point.x - spec.center.x ) +
                                             spec.gradeZ * ( point.z - spec.center.z ) );
                    supported = true;
                }
                need( supported,
                      "Edited city source no longer supports the pinned race route; race is "
                      "unavailable." );
                need( std::isfinite( top ), "Pinned race route has a nonfinite surface height." );
                point.y = top;
            };
            for( auto &point : candidate.route )
            {
                fit( point );
            }

            struct Interval
            {
                double first = 0, last = 0;
            };

            SourceBudgetVector<Interval> coverage;
            coverage.reserve( 32 );
            SourceBudgetVector<double> stations;
            stations.reserve( 64 );
            Bounds routeBounds{};
            const auto coverTriangle = [&]( Vector3F start, Vector3F finish, Vector3F a, Vector3F b,
                                            Vector3F c ) {
                work();
                if( std::max( { a.x, b.x, c.x } ) < routeBounds.minX ||
                    std::min( { a.x, b.x, c.x } ) > routeBounds.maxX ||
                    std::max( { a.z, b.z, c.z } ) < routeBounds.minZ ||
                    std::min( { a.z, b.z, c.z } ) > routeBounds.maxZ )
                {
                    return;
                }
                const auto side = []( Vector3F u, Vector3F v, Vector3F p ) {
                    return ( v.x - double( u.x ) ) * ( p.z - double( u.z ) ) -
                           ( v.z - double( u.z ) ) * ( p.x - double( u.x ) );
                };
                const double signedArea = side( a, b, c );
                if( std::abs( signedArea ) < 1e-10 )
                {
                    return;
                }
                const double sign = signedArea > 0 ? 1 : -1;
                double first = 0, last = 1;
                const std::array<Vector3F, 3> triangle{ a, b, c };
                for( unsigned edge = 0; edge < 3; ++edge )
                {
                    const auto u = triangle[edge], v = triangle[( edge + 1 ) % 3];
                    const double tolerance = .0001 * std::hypot( v.x - u.x, v.z - u.z );
                    const double from = sign * side( u, v, start ) + tolerance;
                    const double to = sign * side( u, v, finish ) + tolerance;
                    if( from < 0 && to < 0 )
                    {
                        return;
                    }
                    if( from < 0 )
                    {
                        first = std::max( first, from / ( from - to ) );
                    }
                    else if( to < 0 )
                    {
                        last = std::min( last, from / ( from - to ) );
                    }
                    if( first > last )
                    {
                        return;
                    }
                }
                need( coverage.size() < 4096,
                      "Pinned route segment coverage exceeds its 4096-interval scratch budget." );
                coverage.push_back( { first, last } );
            };
            for( size_t i = 1; i < candidate.route.size(); ++i )
            {
                const auto a = candidate.route[i - 1], b = candidate.route[i];
                coverage.clear();
                routeBounds = { std::min( a.x, b.x ) - .0001f, std::max( a.x, b.x ) + .0001f,
                                std::min( a.z, b.z ) - .0001f, std::max( a.z, b.z ) + .0001f };
                const double run = std::hypot( b.x - a.x, b.z - a.z );
                need( run > 1e-8, "Pinned race route contains a zero horizontal step." );
                for( size_t r = 0; r < roads.roads.size(); ++r )
                {
                    const auto &box = bounds[r];
                    if( !intersects( routeBounds, box ) )
                    {
                        continue;
                    }
                    const auto &samples = roads.roads[r].points;
                    for( size_t p = 1; p < samples.size(); ++p )
                    {
                        work();
                        if( !intersects( routeBounds, segmentBounds[segmentOffsets[r] + p - 1] ) )
                        {
                            continue;
                        }
                        coverTriangle( a, b, samples[p - 1].left, samples[p - 1].right,
                                       samples[p].left );
                        coverTriangle( a, b, samples[p - 1].right, samples[p].right, samples[p].left );
                    }
                }
                for( size_t j = 0; j < junctions.size(); ++j )
                {
                    const auto &spec = current.roundabouts[j];
                    const Point center{ spec.center.x, spec.center.z };
                    const double clearance = segmentDistance( center, { a.x, a.z }, { b.x, b.z } );
                    need( clearance >= spec.islandRadius - .0001,
                          "Pinned race route segment crosses a raised central island." );
                    if( !intersects( routeBounds, junctionBounds[j].box ) ||
                        clearance * clearance > junctionBounds[j].squaredExtent )
                    {
                        continue;
                    }
                    const auto &junction = junctions[j];
                    for( size_t t = 0; t < junction.triangleKinds.size(); ++t )
                    {
                        const auto kind = junction.triangleKinds[t];
                        if( kind != CityRoadRoundaboutSurface::Circulation &&
                            kind != CityRoadRoundaboutSurface::Approach )
                        {
                            continue;
                        }
                        coverTriangle( a, b, junction.vertices[junction.indices[t * 3]].position,
                                       junction.vertices[junction.indices[t * 3 + 1]].position,
                                       junction.vertices[junction.indices[t * 3 + 2]].position );
                    }
                }
                std::sort( coverage.begin(), coverage.end(),
                           []( const Interval &left, const Interval &right ) {
                               return left.first != right.first ? left.first < right.first
                                                                : left.last < right.last;
                           } );
                const double tolerance = .0001 / run;
                double covered = 0;
                for( const auto interval : coverage )
                {
                    need( interval.first <= covered + tolerance,
                          "Edited city roads leave an unsupported gap in the pinned race route." );
                    covered = std::max( covered, interval.last );
                }
                need( covered >= 1 - tolerance,
                      "Edited city roads do not continuously cover the pinned race route." );
                auto middle = ( a + b ) * .5f;
                fit( middle );
                // Probe both sides of every exact triangle boundary. A geometric
                // contact with a height step is not a continuous running surface.
                stations.clear();
                for( const auto interval : coverage )
                {
                    stations.push_back( interval.first );
                    stations.push_back( interval.last );
                }
                std::sort( stations.begin(), stations.end() );
                stations.erase( std::unique( stations.begin(), stations.end() ), stations.end() );
                for( const double station : stations )
                {
                    const double delta = .001 / run;
                    const double beforeStation = std::max( 0., station - delta ),
                                 afterStation = std::min( 1., station + delta );
                    auto beforePoint = a + ( b - a ) * float( beforeStation );
                    auto afterPoint = a + ( b - a ) * float( afterStation );
                    if( beforeStation == 0 )
                    {
                        beforePoint = a;
                    }
                    else
                    {
                        fit( beforePoint );
                    }
                    if( afterStation == 1 )
                    {
                        afterPoint = b;
                    }
                    else
                    {
                        fit( afterPoint );
                    }
                    need(
                        std::abs( beforePoint.y - afterPoint.y ) <=
                            .001f + .35f * float( ( afterStation - beforeStation ) * run ),
                        "Edited city roads introduce a height discontinuity in the pinned race route." );
                }
            }
            output.swap( candidate.route );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
}  // namespace workphone::procedural
