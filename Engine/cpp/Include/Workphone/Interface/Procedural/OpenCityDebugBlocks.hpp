#pragma once

#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace workphone::procedural
{
    // Closed planning blocks. Grid streets use their explicit cells; natural
    // streets use the bounded faces of the generated planar road graph.
    inline std::vector<std::vector<OpenCityLayout::Point>> openCityDebugBlocks( const OpenCityLayout &city )
    {
        using Point = OpenCityLayout::Point;
        std::vector<std::vector<Point>> faces;
        if( !city.naturalRoads )
        {
            faces.reserve( size_t( city.blocks * city.blocks ) );
            for( int x = 0; x < city.blocks; ++x )
                for( int z = 0; z < city.blocks; ++z )
                {
                    const float x0 = -city.extent + x * city.spacing;
                    const float z0 = -city.extent + z * city.spacing;
                    faces.push_back( { { x0, z0 }, { x0 + city.spacing, z0 },
                                       { x0 + city.spacing, z0 + city.spacing }, { x0, z0 + city.spacing } } );
                }
            return faces;
        }

        struct HalfEdge { std::uint32_t from, to; };
        const auto &graph = city.roads;
        std::vector<HalfEdge> edges;
        std::vector<std::vector<std::uint32_t>> outgoing( graph.nodes.size() );
        edges.reserve( graph.edges.size() * 2 );
        for( const auto &road : graph.edges )
        {
            if( road.a >= graph.nodes.size() || road.b >= graph.nodes.size() || road.a == road.b )
                continue;
            const auto index = std::uint32_t( edges.size() );
            edges.push_back( { road.a, road.b } );
            edges.push_back( { road.b, road.a } );
            outgoing[road.a].push_back( index );
            outgoing[road.b].push_back( index + 1 );
        }
        const auto angle = [&]( std::uint32_t index ) {
            const auto &edge = edges[index];
            const auto &a = graph.nodes[edge.from], &b = graph.nodes[edge.to];
            return std::atan2( b.z - a.z, b.x - a.x );
        };
        std::vector<std::uint32_t> position( edges.size() );
        for( auto &fan : outgoing )
        {
            std::sort( fan.begin(), fan.end(), [&]( auto a, auto b ) {
                const auto aa = angle( a ), bb = angle( b );
                return aa == bb ? a < b : aa < bb;
            } );
            for( std::uint32_t i = 0; i < fan.size(); ++i ) position[fan[i]] = i;
        }
        std::vector<bool> visited( edges.size(), false );
        for( std::uint32_t start = 0; start < edges.size(); ++start )
        {
            if( visited[start] ) continue;
            std::vector<Point> boundary;
            auto current = start;
            bool closed = false;
            for( size_t step = 0; step <= edges.size(); ++step )
            {
                if( visited[current] )
                {
                    closed = current == start;
                    break;
                }
                visited[current] = true;
                const auto &edge = edges[current];
                const auto &p = graph.nodes[edge.from];
                boundary.push_back( { p.x, p.z } );
                const auto &fan = outgoing[edge.to];
                const auto reverse = current ^ 1u;
                current = fan[( position[reverse] + fan.size() - 1 ) % fan.size()];
            }
            if( !closed || boundary.size() < 3 ) continue;
            double twiceArea = 0;
            for( size_t i = 0; i < boundary.size(); ++i )
            {
                const auto &a = boundary[i], &b = boundary[( i + 1 ) % boundary.size()];
                twiceArea += double( a.x ) * b.z - double( b.x ) * a.z;
            }
            // The outer, unbounded face walks in the opposite direction.
            if( twiceArea > 1.0 ) faces.push_back( std::move( boundary ) );
        }
        return faces;
    }

    // Move the diagnostic outline from a road centreline to the block side.
    // Bound the inset by the face dimensions so small irregular blocks do not
    // turn inside out under a wide-road offset.
    template <class Allocator>
    inline std::vector<OpenCityLayout::Point, Allocator> insetOpenCityDebugBlock(
        const std::vector<OpenCityLayout::Point, Allocator> &samples, float requestedInset )
    {
        if( samples.size() < 3 || requestedInset <= 0 ) return samples;
        // A short, nearly straight spline sample next to a corner can reverse
        // its inset edge. Merge straight runs before offsetting the visible line.
        const auto straight = []( auto a, auto b, auto c ) {
            const double dx = double( c.x ) - a.x, dz = double( c.z ) - a.z;
            const double bx = double( b.x ) - a.x, bz = double( b.z ) - a.z;
            const double along = bx * dx + bz * dz;
            return along >= 0 && along <= dx * dx + dz * dz &&
                   std::abs( bx * dz - bz * dx ) <= .005 * std::hypot( dx, dz ) &&
                   ( dx != 0 || dz != 0 );
        };
        std::vector<OpenCityLayout::Point, Allocator> boundary( samples.get_allocator() );
        boundary.reserve( samples.size() );
        for( const auto p : samples )
        {
            while( boundary.size() > 1 && straight( boundary[boundary.size()-2], boundary.back(), p ) )
                boundary.pop_back();
            boundary.push_back( p );
        }
        while( boundary.size() > 3 && straight( boundary[boundary.size()-2], boundary.back(), boundary.front() ) )
            boundary.pop_back();
        while( boundary.size() > 3 && straight( boundary.back(), boundary.front(), boundary[1] ) )
            boundary.erase( boundary.begin() );
        if( boundary.size() < 3 ) return samples;
        float minX = boundary[0].x, maxX = minX, minZ = boundary[0].z, maxZ = minZ;
        for( const auto &p : boundary )
        {
            minX = std::min( minX, p.x ); maxX = std::max( maxX, p.x );
            minZ = std::min( minZ, p.z ); maxZ = std::max( maxZ, p.z );
        }
        const float inset = std::min( requestedInset, .15f * std::min( maxX - minX, maxZ - minZ ) );
        if( inset <= 0 ) return boundary;
        std::vector<OpenCityLayout::Point, Allocator> result( boundary.get_allocator() );
        result.reserve( boundary.size() );
        for( size_t i = 0; i < boundary.size(); ++i )
        {
            const auto &a = boundary[( i + boundary.size() - 1 ) % boundary.size()];
            const auto &b = boundary[i], &c = boundary[( i + 1 ) % boundary.size()];
            const float length0 = std::hypot( b.x - a.x, b.z - a.z );
            const float length1 = std::hypot( c.x - b.x, c.z - b.z );
            if( length0 < 1e-4f || length1 < 1e-4f )
            {
                result.push_back( b );
                continue;
            }
            const float nx0 = -( b.z - a.z ) / length0, nz0 = ( b.x - a.x ) / length0;
            const float nx1 = -( c.z - b.z ) / length1, nz1 = ( c.x - b.x ) / length1;
            const float bx = nx0 + nx1, bz = nz0 + nz1;
            const float denominator = bx * nx1 + bz * nz1;
            float dx = nx1 * inset, dz = nz1 * inset;
            if( denominator > .15f )
            {
                const float scale = std::min( inset / denominator, inset * 3.f /
                    std::max( std::hypot( bx, bz ), 1e-4f ) );
                dx = bx * scale; dz = bz * scale;
            }
            result.push_back( { b.x + dx, b.z + dz } );
        }
        return result;
    }
}
