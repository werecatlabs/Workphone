#pragma once
#include <Workphone/System/SourceBudgetVector.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace workphone::procedural
{
    // Bounded planar street graph. Generated node indices describe this output,
    // not persistent authoring identities for editable road/spline sources.
    struct OpenCityRoadNetwork : SourceBudgetObject
    {
        struct Point
        {
            float x = 0, z = 0;
        };

        struct Edge
        {
            std::uint32_t a = 0, b = 0;
        };

        static constexpr std::size_t maximumNodes = 160, maximumEdges = 240;
        static constexpr float minimumEdgeLength = 4.f, roadHalfWidth = 6.f;
        int blocks = 0;
        float spacing = 0, extent = 0;
        SourceBudgetVector<Point> nodes, route;
        SourceBudgetVector<Edge> edges;

        OpenCityRoadNetwork() = default;
        OpenCityRoadNetwork( const OpenCityRoadNetwork & ) = default;
        OpenCityRoadNetwork( OpenCityRoadNetwork && ) noexcept = default;
        OpenCityRoadNetwork &operator=( OpenCityRoadNetwork && ) noexcept = default;

        OpenCityRoadNetwork &operator=( const OpenCityRoadNetwork &other )
        {
            if( this != &other )
            {
                OpenCityRoadNetwork candidate( other );
                swap( candidate );
            }
            return *this;
        }

        void swap( OpenCityRoadNetwork &other ) noexcept
        {
            using std::swap;
            swap( blocks, other.blocks );
            swap( spacing, other.spacing );
            swap( extent, other.extent );
            nodes.swap( other.nodes );
            edges.swap( other.edges );
            route.swap( other.route );
        }

        float distance( float x, float z ) const
        {
            if( !std::isfinite( x ) || !std::isfinite( z ) )
            {
                return 1000;
            }
            double closest = std::numeric_limits<double>::max();
            for( const auto &edge : edges )
            {
                if( edge.a >= nodes.size() || edge.b >= nodes.size() )
                {
                    continue;
                }
                const auto a = nodes[edge.a], b = nodes[edge.b];
                const double dx = b.x - a.x, dz = b.z - a.z, squared = dx * dx + dz * dz;
                if( squared <= 0 )
                {
                    continue;
                }
                const double t = std::clamp( ( ( x - a.x ) * dx + ( z - a.z ) * dz ) / squared, 0., 1. );
                const double px = x - a.x - t * dx, pz = z - a.z - t * dz;
                closest = std::min( closest, px * px + pz * pz );
            }
            return closest == std::numeric_limits<double>::max() ? 1000.f
                                                                 : float( std::sqrt( closest ) );
        }

        static OpenCityRoadNetwork generate( std::uint32_t seed, int requestedBlocks = 8,
                                             int routeChoice = 0 )
        {
            OpenCityRoadNetwork graph;
            graph.blocks = std::clamp( requestedBlocks / 2 * 2, 6, 10 );
            graph.spacing = 88.f + float( seed % 17 );
            graph.extent = graph.blocks * graph.spacing * .5f;
            graph.nodes.reserve( maximumNodes );
            graph.edges.reserve( maximumEdges );
            constexpr auto invalid = std::numeric_limits<std::uint32_t>::max();
            const auto squared = []( Point a, Point b ) {
                const double x = a.x - b.x, z = a.z - b.z;
                return x * x + z * z;
            };
            const auto node = [&]( Point point ) {
                for( std::uint32_t i = 0; i < graph.nodes.size(); ++i )
                {
                    if( squared( graph.nodes[i], point ) <= 1.e-6 )
                    {
                        return i;
                    }
                }
                if( graph.nodes.size() == maximumNodes )
                {
                    return invalid;
                }
                const auto index = std::uint32_t( graph.nodes.size() );
                graph.nodes.push_back( point );
                return index;
            };
            const auto edge = [&]( std::uint32_t a, std::uint32_t b ) {
                if( a == invalid || b == invalid || a == b ||
                    squared( graph.nodes[a], graph.nodes[b] ) <
                        minimumEdgeLength * minimumEdgeLength - .001 )
                {
                    return false;
                }
                for( const auto &value : graph.edges )
                {
                    if( ( value.a == a && value.b == b ) || ( value.a == b && value.b == a ) )
                    {
                        return true;
                    }
                }
                if( graph.edges.size() == maximumEdges )
                {
                    return false;
                }
                graph.edges.push_back( { a, b } );
                return true;
            };
            const auto random = [seed]( std::uint64_t domain ) {
                std::uint64_t x = domain ^ ( std::uint64_t( seed ) << 32 ) ^ seed;
                x += 0x9e3779b97f4a7c15ull;
                x = ( x ^ ( x >> 30 ) ) * 0xbf58476d1ce4e5b9ull;
                x = ( x ^ ( x >> 27 ) ) * 0x94d049bb133111ebull;
                return float( ( x ^ ( x >> 31 ) ) >> 40 ) / 16777216.f;
            };

            // Three adjoining districts share a central avenue, rather than three
            // scaled copies of a race track. The compact eastern old town, the
            // western neighbourhood and the broad eastern perimeter have their
            // own seeded proportions. All are published before route selection.
            std::array<std::array<Point, 8>, 3> loops = {{
                {{ { 0, 0 }, { 0, -.30f }, { .20f, -.42f }, { .47f, -.35f },
                   { .52f, 0 }, { .40f, .29f }, { .17f, .33f }, { 0, .27f } }},
                {{ { 0, 0 }, { 0, -.52f }, { -.31f, -.63f }, { -.70f, -.52f },
                   { -.77f, -.05f }, { -.65f, .50f }, { -.22f, .62f }, { 0, .52f } }},
                {{ { 0, 0 }, { 0, -.82f }, { .34f, -.85f }, { .78f, -.64f },
                   { .85f, .09f }, { .71f, .65f }, { .34f, .82f }, { 0, .75f } }}
            }};
            std::array<std::array<std::uint32_t, 8>, 3> loopNodes{};
            std::array<std::uint32_t, 7> spine{};
            spine[0] = node( { 0, 0 } );
            for( std::size_t district = 0; district < loops.size(); ++district )
            {
                for( std::size_t i = 0; i < loops[district].size(); ++i )
                {
                    auto &point = loops[district][i];
                    if( i != 0 )
                    {
                        const auto domain = 0x7100u + district * 1009 + i * 101;
                        if( i != 1 && i != 7 )
                        {
                            point.x += ( random( domain ) - .5f ) * .055f;
                        }
                        point.z += ( random( domain + 17 ) - .5f ) * .045f;
                    }
                    point.x *= graph.extent;
                    point.z *= graph.extent;
                    loopNodes[district][i] = node( point );
                }
                for( std::size_t i = 1; i < 7; ++i )
                {
                    edge( loopNodes[district][i], loopNodes[district][i + 1] );
                }
                spine[1 + district * 2] = loopNodes[district][1];
                spine[2 + district * 2] = loopNodes[district][7];
            }
            std::sort( spine.begin(), spine.end(),
                       [&]( auto a, auto b ) { return graph.nodes[a].z < graph.nodes[b].z; } );
            for( std::size_t i = 0; i + 1 < spine.size(); ++i )
            {
                edge( spine[i], spine[i + 1] );
            }

            // Width-aware local constraints keep growth out of narrow slivers.
            // The stricter generation limits leave room for editable conversion,
            // whose source codec permits 128 roads (not 240 independent edges).
            constexpr float junctionSpacing = 24.f, corridorClearance = 18.f;
            constexpr double maximumJunctionCosine = .6427876096865394; // cos(50 degrees)
            constexpr std::size_t generatedEdgeLimit = 120;
            const auto pointSegmentSquared = [&]( Point p, Point a, Point b ) {
                const double dx = b.x - a.x, dz = b.z - a.z;
                const double t = std::clamp( ( ( p.x - a.x ) * dx + ( p.z - a.z ) * dz ) /
                                                 ( dx * dx + dz * dz ), 0., 1. );
                return squared( p, { float( a.x + t * dx ), float( a.z + t * dz ) } );
            };
            const auto junctionAccepts = [&]( std::uint32_t at, Point destination ) {
                const auto center = graph.nodes[at];
                const double length = std::sqrt( squared( center, destination ) );
                unsigned degree = 0;
                for( const auto &existing : graph.edges )
                {
                    if( existing.a != at && existing.b != at ) continue;
                    ++degree;
                    const auto next = graph.nodes[existing.a == at ? existing.b : existing.a];
                    const double cosine = ( ( next.x - center.x ) * double( destination.x - center.x ) +
                                            ( next.z - center.z ) * double( destination.z - center.z ) ) /
                                          ( std::sqrt( squared( center, next ) ) * length );
                    if( cosine > maximumJunctionCosine ) return false;
                }
                return degree < 4;
            };
            struct Stroke
            {
                std::uint32_t node;
                bool connected;
            };
            // Intersect first, validate the entire proposal, then publish together.
            // A connection terminates its turtle: subsequent grammar symbols must
            // not start a new fan of branches from someone else's junction.
            const auto stroke = [&]( std::uint32_t start, Point requested,
                                     bool requireConnection = false ) -> Stroke {
                const Stroke stopped{ start, true };
                if( graph.nodes.size() + 1 > maximumNodes ||
                    graph.edges.size() + 2 > generatedEdgeLimit ) return stopped;
                const auto a = graph.nodes[start];
                const float bound = graph.extent - 12;
                double dx = requested.x - a.x, dz = requested.z - a.z, fraction = 1;
                if( dx > 0 ) fraction = std::min( fraction, ( bound - a.x ) / dx );
                if( dx < 0 ) fraction = std::min( fraction, ( -bound - a.x ) / dx );
                if( dz > 0 ) fraction = std::min( fraction, ( bound - a.z ) / dz );
                if( dz < 0 ) fraction = std::min( fraction, ( -bound - a.z ) / dz );
                Point b{ float( a.x + dx * std::clamp( fraction, 0., 1. ) ),
                         float( a.z + dz * std::clamp( fraction, 0., 1. ) ) };
                std::uint32_t endNode = invalid;
                std::size_t endEdge = maximumEdges;
                double bestSnap = junctionSpacing * junctionSpacing;
                for( std::uint32_t i = 0; i < graph.nodes.size(); ++i )
                {
                    if( i != start && squared( b, graph.nodes[i] ) < bestSnap )
                    {
                        bestSnap = squared( b, graph.nodes[i] );
                        endNode = i;
                    }
                }
                if( endNode != invalid ) b = graph.nodes[endNode];
                else
                {
                    for( std::size_t i = 0; i < graph.edges.size(); ++i )
                    {
                        const auto c = graph.nodes[graph.edges[i].a], d = graph.nodes[graph.edges[i].b];
                        const double sx = d.x - c.x, sz = d.z - c.z, ss = sx * sx + sz * sz;
                        const double t = std::clamp( ( ( b.x - c.x ) * sx + ( b.z - c.z ) * sz ) / ss, 0., 1. );
                        const Point p{ float( c.x + sx * t ), float( c.z + sz * t ) };
                        const auto distance = squared( p, b );
                        if( distance < bestSnap && squared( a, p ) >= junctionSpacing * junctionSpacing )
                        {
                            bestSnap = distance;
                            requested = p;
                            endEdge = i;
                        }
                    }
                    if( endEdge != maximumEdges ) b = requested;
                }
                dx = b.x - a.x;
                dz = b.z - a.z;
                const double lengthSquared = dx * dx + dz * dz;
                if( lengthSquared < junctionSpacing * junctionSpacing ) return stopped;
                double hit = 1;
                auto hitNode = endNode;
                auto hitEdge = endEdge;
                for( std::uint32_t i = 0; i < graph.nodes.size(); ++i )
                {
                    if( i == start ) continue;
                    const auto p = graph.nodes[i];
                    const double t = ( ( p.x - a.x ) * dx + ( p.z - a.z ) * dz ) / lengthSquared;
                    const double x = p.x - a.x - dx * t, z = p.z - a.z - dz * t;
                    if( t > 1.e-6 && t <= hit + 1.e-6 && x * x + z * z <= 1.e-6 )
                    {
                        hit = t;
                        hitNode = i;
                        hitEdge = maximumEdges;
                    }
                }
                for( std::size_t i = 0; i < graph.edges.size(); ++i )
                {
                    const auto c = graph.nodes[graph.edges[i].a], d = graph.nodes[graph.edges[i].b];
                    const double sx = d.x - c.x, sz = d.z - c.z, denominator = dx * sz - dz * sx;
                    if( std::abs( denominator ) < 1.e-8 ) continue;
                    const double t = ( ( c.x - a.x ) * sz - ( c.z - a.z ) * sx ) / denominator;
                    const double u = ( ( c.x - a.x ) * dz - ( c.z - a.z ) * dx ) / denominator;
                    if( t > 1.e-6 && t < hit - 1.e-6 && u >= -1.e-6 && u <= 1 + 1.e-6 )
                    {
                        hit = t;
                        hitNode = invalid;
                        hitEdge = i;
                    }
                }
                const Point destination{ float( a.x + dx * hit ), float( a.z + dz * hit ) };
                if( squared( a, destination ) < junctionSpacing * junctionSpacing - .001 ||
                    !junctionAccepts( start, destination ) ) return stopped;
                if( hitNode == invalid && hitEdge != maximumEdges )
                {
                    const auto old = graph.edges[hitEdge];
                    const auto da = squared( destination, graph.nodes[old.a] );
                    const auto db = squared( destination, graph.nodes[old.b] );
                    if( da <= 1.e-6 ) hitNode = old.a;
                    else if( db <= 1.e-6 ) hitNode = old.b;
                    else
                    {
                        if( da < junctionSpacing * junctionSpacing ||
                            db < junctionSpacing * junctionSpacing ) return stopped;
                        const auto c = graph.nodes[old.a], d = graph.nodes[old.b];
                        const double cosine = std::abs( ( d.x - c.x ) * double( a.x - destination.x ) +
                                                        ( d.z - c.z ) * double( a.z - destination.z ) ) /
                                              std::sqrt( squared( c, d ) * squared( a, destination ) );
                        if( cosine > maximumJunctionCosine ) return stopped;
                    }
                }
                if( hitNode != invalid && !junctionAccepts( hitNode, a ) ) return stopped;
                const bool connected = hitNode != invalid || hitEdge != maximumEdges;
                if( requireConnection && !connected ) return stopped;
                for( std::size_t i = 0; i < graph.edges.size(); ++i )
                {
                    const auto existing = graph.edges[i];
                    if( i == hitEdge || existing.a == start || existing.b == start ||
                        existing.a == hitNode || existing.b == hitNode ) continue;
                    const auto c = graph.nodes[existing.a], d = graph.nodes[existing.b];
                    if( std::min( { pointSegmentSquared( a, c, d ), pointSegmentSquared( destination, c, d ),
                                    pointSegmentSquared( c, a, destination ), pointSegmentSquared( d, a, destination ) } ) <
                        corridorClearance * corridorClearance ) return stopped;
                }
                if( hitNode == invalid )
                {
                    hitNode = node( destination );
                    if( hitEdge != maximumEdges )
                    {
                        const auto old = graph.edges[hitEdge];
                        graph.edges[hitEdge] = { old.a, hitNode };
                        edge( hitNode, old.b );
                    }
                }
                edge( start, hitNode );
                return { hitNode, connected || fraction < 1 };
            };

            // A real bounded L-system: rewriting creates nested branch structure,
            // then a turtle interprets F, +/- and the save/restore brackets.
            SourceBudgetVector<char> grammar( 1, 'F' );
            for( unsigned generation = 0; generation < 2; ++generation )
            {
                SourceBudgetVector<char> rewritten;
                rewritten.reserve( grammar.size() * 10 );
                std::size_t index = 0;
                for( const auto symbol : grammar )
                {
                    if( symbol != 'F' )
                    {
                        rewritten.push_back( symbol );
                    }
                    else
                    {
                        const char *rule =
                            random( generation * 701 + index * 19 ) < .5f ? "F[+F]F[-F]" : "F[-F]F[+F]";
                        while( *rule )
                        {
                            rewritten.push_back( *rule++ );
                        }
                    }
                    ++index;
                }
                grammar.swap( rewritten );
            }

            struct Turtle
            {
                std::uint32_t node;
                float heading, step;
                bool active = true;
            };

            SourceBudgetVector<Turtle> stack;
            stack.reserve( 8 );
            constexpr float pi = 3.14159265358979323846f;
            // Local streets inherit their district's direction. Near-right-angle
            // branches form usable blocks instead of repeated diagonal forks.
            const std::array<std::uint32_t, 8> anchors = {
                0, 0, loopNodes[1][3], loopNodes[1][5], loopNodes[2][3],
                loopNodes[2][5], loopNodes[0][2], loopNodes[0][6]
            };
            const std::array<float, 8> headings = { pi, 0, -.1f, .1f, pi + .2f,
                                                   pi - .2f, -pi * .5f, pi * .5f };
            for( std::size_t anchor = 0; anchor < anchors.size(); ++anchor )
            {
                float heading = headings[anchor];
                if( anchor >= 2 && anchor <= 5 )
                {
                    // Grow into the district along its corner bisector. A fixed
                    // compass heading can crowd one boundary edge and prevent an
                    // otherwise useful collector from entering a large block.
                    const auto district = anchor < 4 ? 1u : 2u;
                    const auto corner = anchor % 2 == 0 ? 3u : 5u;
                    const auto center = loops[district][corner];
                    const auto previous = loops[district][corner - 1];
                    const auto next = loops[district][corner + 1];
                    const auto first = std::sqrt( squared( center, previous ) );
                    const auto second = std::sqrt( squared( center, next ) );
                    heading = float( std::atan2( ( previous.z - center.z ) / first + ( next.z - center.z ) / second,
                                                ( previous.x - center.x ) / first + ( next.x - center.x ) / second ) );
                }
                Turtle turtle{ anchors[anchor], heading,
                               graph.spacing * ( .82f + .22f * random( anchor * 313 + 91 ) ) };
                stack.clear();
                std::size_t token = 0;
                for( const auto symbol : grammar )
                {
                    if( symbol == 'F' )
                    {
                        if( turtle.active )
                        {
                            const auto p = graph.nodes[turtle.node];
                            const auto result = stroke( turtle.node,
                                { p.x + std::cos( turtle.heading ) * turtle.step,
                                  p.z + std::sin( turtle.heading ) * turtle.step } );
                            turtle.active = !result.connected;
                            turtle.node = result.node;
                        }
                    }
                    else if( symbol == '[' )
                    {
                        stack.push_back( turtle );
                        turtle.step *= .94f;
                    }
                    else if( symbol == ']' )
                    {
                        turtle = stack.back();
                        stack.pop_back();
                    }
                    else
                    {
                        const float turn =
                            ( 80.f + 20.f * random( anchor * 1009 + token * 17 + 0x51u ) ) * pi / 180.f;
                        turtle.heading += symbol == '+' ? turn : -turn;
                    }
                    ++token;
                }
            }
            // Complete short local links where there is room for a real junction.
            // Proposals reuse the same intersection/angle/clearance gate, so this
            // does not stitch roads through blocks or create nearly touching forks.
            const auto grownNodes = graph.nodes.size();
            for( std::uint32_t at = 0; at < grownNodes; ++at )
            {
                unsigned degree = 0;
                for( const auto existing : graph.edges )
                    degree += existing.a == at || existing.b == at;
                if( degree != 1 ) continue;
                struct Connection { Point point; double distance; };
                SourceBudgetVector<Connection> proposals;
                proposals.reserve( graph.edges.size() );
                const auto p = graph.nodes[at];
                for( const auto existing : graph.edges )
                {
                    if( existing.a == at || existing.b == at ) continue;
                    const auto a = graph.nodes[existing.a], b = graph.nodes[existing.b];
                    const double dx = b.x - a.x, dz = b.z - a.z;
                    const double t = std::clamp( ( ( p.x - a.x ) * dx + ( p.z - a.z ) * dz ) /
                                                     ( dx * dx + dz * dz ), 0., 1. );
                    const Point point{ float( a.x + t * dx ), float( a.z + t * dz ) };
                    const double separation = squared( p, point );
                    if( separation >= junctionSpacing * junctionSpacing &&
                        separation <= graph.spacing * graph.spacing * 1.44 )
                        proposals.push_back( { point, separation } );
                }
                std::sort( proposals.begin(), proposals.end(),
                    []( const Connection &a, const Connection &b ) {
                        if( a.distance != b.distance ) return a.distance < b.distance;
                        return a.point.x != b.point.x ? a.point.x < b.point.x : a.point.z < b.point.z;
                    } );
                for( const auto &proposal : proposals )
                {
                    if( stroke( at, proposal.point, true ).node != at ) break;
                }
            }
            const auto &selected = loops[std::clamp( routeChoice, 0, 2 )];
            graph.route.reserve( 2048 );
            for( std::size_t i = 0; i < selected.size(); ++i )
            {
                const auto a = selected[i], b = selected[( i + 1 ) % selected.size()];
                const auto steps =
                    std::max( 1, int( std::ceil( std::hypot( b.x - a.x, b.z - a.z ) / 2 ) ) );
                for( int j = 0; j < steps; ++j )
                {
                    const float t = float( j ) / steps;
                    graph.route.push_back( { a.x + ( b.x - a.x ) * t, a.z + ( b.z - a.z ) * t } );
                }
            }
            return graph;
        }
    };
}  // namespace workphone::procedural
