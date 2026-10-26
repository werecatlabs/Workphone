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

            // Three closed, nested trunk circuits share the origin's -Z spine.
            // All three are built before route selection, so route choice cannot
            // reshuffle the street graph or the parcels derived from it.
            std::array<Point, 8> shape = { Point{ 0, 0 },    { 0, -.55f },     { .45f, -1.f },
                                           { 1.25f, -.78f }, { 1.55f, -.10f }, { 1.10f, .72f },
                                           { .35f, .92f },   { 0, .42f } };
            for( std::size_t i = 1; i < shape.size(); ++i )
            {
                if( i != 1 && i != 7 )
                {
                    shape[i].x += ( random( i * 101 ) - .5f ) * .025f;
                }
                shape[i].z += ( random( i * 101 + 7 ) - .5f ) * .025f;
            }
            std::array<std::array<Point, 8>, 3> loops{};
            std::array<std::array<std::uint32_t, 8>, 3> loopNodes{};
            std::array<std::uint32_t, 7> spine{};
            spine[0] = node( { 0, 0 } );
            for( std::size_t circuit = 0; circuit < loops.size(); ++circuit )
            {
                const float radius = graph.extent * ( .33f + circuit * .115f );
                for( std::size_t i = 0; i < shape.size(); ++i )
                {
                    loops[circuit][i] = { shape[i].x * radius, shape[i].z * radius };
                    loopNodes[circuit][i] = node( loops[circuit][i] );
                }
                for( std::size_t i = 1; i < 7; ++i )
                {
                    edge( loopNodes[circuit][i], loopNodes[circuit][i + 1] );
                }
                spine[1 + circuit * 2] = loopNodes[circuit][1];
                spine[2 + circuit * 2] = loopNodes[circuit][7];
            }
            std::sort( spine.begin(), spine.end(),
                       [&]( auto a, auto b ) { return graph.nodes[a].z < graph.nodes[b].z; } );
            for( std::size_t i = 0; i + 1 < spine.size(); ++i )
            {
                edge( spine[i], spine[i + 1] );
            }

            // Publish one turtle stroke up to its first connection. Intersections
            // split existing edges; too-short joins are rejected rather than leave
            // overlapping geometry or disconnected tiny junction fragments.
            const auto stroke = [&]( std::uint32_t start, Point requested ) {
                if( graph.nodes.size() + 1 > maximumNodes || graph.edges.size() + 2 > maximumEdges )
                {
                    return start;
                }
                const auto a = graph.nodes[start];
                const float bound = graph.extent - 12;
                double dx = requested.x - a.x, dz = requested.z - a.z, fraction = 1;
                if( dx > 0 )
                {
                    fraction = std::min( fraction, ( bound - a.x ) / dx );
                }
                if( dx < 0 )
                {
                    fraction = std::min( fraction, ( -bound - a.x ) / dx );
                }
                if( dz > 0 )
                {
                    fraction = std::min( fraction, ( bound - a.z ) / dz );
                }
                if( dz < 0 )
                {
                    fraction = std::min( fraction, ( -bound - a.z ) / dz );
                }
                Point b{ float( a.x + dx * std::clamp( fraction, 0., 1. ) ),
                         float( a.z + dz * std::clamp( fraction, 0., 1. ) ) };
                // Do not turn a clipped growth step into a road-width-sized whisker.
                // Short pieces created by real junction splits retain their 4m policy.
                if( fraction < 1 && squared( a, b ) < 18.f * 18.f )
                {
                    return start;
                }
                std::uint32_t endNode = invalid;
                std::size_t endEdge = maximumEdges;
                double bestSnap = 64;
                for( std::uint32_t i = 0; i < graph.nodes.size(); ++i )
                {
                    if( i != start && squared( b, graph.nodes[i] ) < bestSnap )
                    {
                        bestSnap = squared( b, graph.nodes[i] );
                        endNode = i;
                    }
                }
                if( endNode != invalid )
                {
                    b = graph.nodes[endNode];
                }
                else
                {
                    for( std::size_t i = 0; i < graph.edges.size(); ++i )
                    {
                        const auto c = graph.nodes[graph.edges[i].a], d = graph.nodes[graph.edges[i].b];
                        const double sx = d.x - c.x, sz = d.z - c.z, ss = sx * sx + sz * sz;
                        const double t =
                            std::clamp( ( ( b.x - c.x ) * sx + ( b.z - c.z ) * sz ) / ss, 0., 1. );
                        const Point p{ float( c.x + sx * t ), float( c.z + sz * t ) };
                        const auto distance = squared( p, b );
                        if( distance < bestSnap && squared( a, p ) >= 16 )
                        {
                            bestSnap = distance;
                            requested = p;
                            endEdge = i;
                        }
                    }
                }
                if( endEdge != maximumEdges )
                {
                    b = requested;
                }
                dx = b.x - a.x;
                dz = b.z - a.z;
                const double lengthSquared = dx * dx + dz * dz;
                if( lengthSquared < 16 )
                {
                    return start;
                }
                double hit = 1;
                auto hitNode = endNode;
                auto hitEdge = endEdge;
                for( std::uint32_t i = 0; i < graph.nodes.size(); ++i )
                {
                    if( i == start )
                    {
                        continue;
                    }
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
                    if( std::abs( denominator ) < 1.e-8 )
                    {
                        continue;
                    }
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
                if( squared( a, destination ) < 16 - .001 )
                {
                    return start;
                }
                if( hitNode == invalid && hitEdge != maximumEdges )
                {
                    const auto old = graph.edges[hitEdge];
                    const auto da = squared( destination, graph.nodes[old.a] );
                    const auto db = squared( destination, graph.nodes[old.b] );
                    if( da <= 1.e-6 )
                    {
                        hitNode = old.a;
                    }
                    else if( db <= 1.e-6 )
                    {
                        hitNode = old.b;
                    }
                    else
                    {
                        if( da < 16 - .001 || db < 16 - .001 )
                        {
                            return start;
                        }
                        hitNode = node( destination );
                        if( hitNode == invalid )
                        {
                            return start;
                        }
                        graph.edges[hitEdge] = { old.a, hitNode };
                        edge( hitNode, old.b );
                    }
                }
                if( hitNode == invalid )
                {
                    hitNode = node( destination );
                }
                if( hitNode == invalid || !edge( start, hitNode ) )
                {
                    return start;
                }
                return hitNode;
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
            };

            SourceBudgetVector<Turtle> stack;
            stack.reserve( 8 );
            constexpr float pi = 3.14159265358979323846f;
            // Fewer well-spaced arterials leave buildable blocks between branches.
            // The eastern anchor grows inward through the ring roads rather than
            // repeatedly clipping a fan of short roads against the city boundary.
            const std::array<std::uint32_t, 6> anchors = {
                0, loopNodes[2][1], loopNodes[2][7], loopNodes[1][2], loopNodes[1][6], loopNodes[2][4]
            };
            const std::array<float, 6> headings = { pi, -pi * .5f, pi * .5f, -2.2f, 2.2f, pi };
            for( std::size_t anchor = 0; anchor < anchors.size(); ++anchor )
            {
                Turtle turtle{ anchors[anchor], headings[anchor],
                               graph.extent * ( .25f + .05f * random( anchor * 313 + 91 ) ) };
                stack.clear();
                std::size_t token = 0;
                for( const auto symbol : grammar )
                {
                    if( symbol == 'F' )
                    {
                        const auto p = graph.nodes[turtle.node];
                        turtle.node =
                            stroke( turtle.node, { p.x + std::cos( turtle.heading ) * turtle.step,
                                                   p.z + std::sin( turtle.heading ) * turtle.step } );
                    }
                    else if( symbol == '[' )
                    {
                        stack.push_back( turtle );
                        turtle.step *= .88f;
                    }
                    else if( symbol == ']' )
                    {
                        turtle = stack.back();
                        stack.pop_back();
                    }
                    else
                    {
                        const float turn =
                            ( 55.f + 25.f * random( anchor * 1009 + token * 17 + 0x51u ) ) * pi / 180.f;
                        turtle.heading += symbol == '+' ? turn : -turn;
                    }
                    ++token;
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
