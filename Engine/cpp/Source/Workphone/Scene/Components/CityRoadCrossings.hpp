#pragma once

#include <Workphone/Scene/Components/CityRoadJunctions.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone::scene::cityRoadCrossings
{
    inline constexpr u32 noRoad = std::numeric_limits<u32>::max();

    using Crossing = CityRoadCrossing;

    // Refine the fitted walk only after a crossing has been accepted on the road.
    // Failure leaves the original shared render/collision mesh intact.
    WPCore_API bool fitDroppedKerbs( CityRoadTerrainMesh &footways, const CityRoadTerrainMesh &roadMesh,
                                     SourceBudgetVector<Crossing> &paintedCrossings, String &error );

    inline float crossXZ( Vector3F a, Vector3F b )
    {
        return a.x * b.z - a.z * b.x;
    }

    class WalkSupport
    {
    public:
        explicit WalkSupport( const CityRoadTerrainMesh &mesh ) : m_mesh( mesh )
        {
            for( u32 triangle = 0; triangle + 2 < mesh.indices.size(); triangle += 3 )
            {
                const auto &a = mesh.vertices[mesh.indices[triangle]];
                const auto &b = mesh.vertices[mesh.indices[triangle + 1]];
                const auto &c = mesh.vertices[mesh.indices[triangle + 2]];
                if( std::min( { a.normal.y, b.normal.y, c.normal.y } ) < .75f ) continue;
                const auto low = cell( { std::min( { a.position.x, b.position.x, c.position.x } ), 0,
                                         std::min( { a.position.z, b.position.z, c.position.z } ) } );
                const auto high = cell( { std::max( { a.position.x, b.position.x, c.position.x } ), 0,
                                          std::max( { a.position.z, b.position.z, c.position.z } ) } );
                for( s32 x = low.first; x <= high.first; ++x )
                    for( s32 z = low.second; z <= high.second; ++z )
                        m_cells[{ x, z }].push_back( triangle );
            }
        }

        bool hasLanding( const Crossing &crossing ) const
        {
            const float half = crossing.depth * .5f;
            for( const float sign : { -1.f, 1.f } )
            {
                for( const float station : { -half + .18f, 0.f, half - .18f } )
                {
                    for( const float inset : { .58f, 1.25f } )
                    {
                        const auto p = crossing.centre + crossing.outward * station +
                                       crossing.across * ( sign * ( crossing.width * .5f + inset ) );
                        if( !contains( p ) ) return false;
                    }
                }
            }
            return true;
        }

    private:
        using Cell = std::pair<s32, s32>;

        static Cell cell( Vector3F point )
        {
            return { s32( std::floor( point.x / 8.f ) ), s32( std::floor( point.z / 8.f ) ) };
        }

        bool contains( Vector3F point ) const
        {
            const auto found = m_cells.find( cell( point ) );
            if( found == m_cells.end() ) return false;
            for( const auto triangle : found->second )
            {
                const auto &a = m_mesh.vertices[m_mesh.indices[triangle]].position;
                const auto &b = m_mesh.vertices[m_mesh.indices[triangle + 1]].position;
                const auto &c = m_mesh.vertices[m_mesh.indices[triangle + 2]].position;
                if( std::abs( crossXZ( b - a, c - a ) ) < .00001f ) continue;
                const float ab = crossXZ( b - a, point - a );
                const float bc = crossXZ( c - b, point - b );
                const float ca = crossXZ( a - c, point - c );
                if( ( ab >= -.00001f && bc >= -.00001f && ca >= -.00001f ) ||
                    ( ab <= .00001f && bc <= .00001f && ca <= .00001f ) ) return true;
            }
            return false;
        }

        const CityRoadTerrainMesh &m_mesh;
        SourceBudgetMap<Cell, SourceBudgetVector<u32>> m_cells;
    };

    inline Vector3F positionAt( const SourceBudgetVector<procedural::CityRoadSplineSample> &points,
                                float station )
    {
        const auto end = std::lower_bound( points.begin(), points.end(), station,
            []( const auto &p, float value ) { return p.distance < value; } );
        if( end == points.begin() ) return end->position;
        if( end == points.end() ) return points.back().position;
        const auto &start = *( end - 1 );
        return start.position + ( end->position - start.position ) *
            ( ( station - start.distance ) / ( end->distance - start.distance ) );
    }

    inline SourceBudgetVector<Crossing> collect( const procedural::CityRoadSplineSource &source,
                                                 const CityRoadJunctionBake &connections,
                                                 const CityRoadTerrainMesh &footways )
    {
        SourceBudgetVector<Crossing> result;
        if( footways.indices.empty() || ( source.roundabouts.empty() && connections.junctions.empty() ) )
            return result;
        const WalkSupport support( footways );
        constexpr size_t roundaboutLimit = 32, totalLimit = 64;
        const auto admit = [&]( const Crossing &crossing ) {
            if( !support.hasLanding( crossing ) ) return;
            // Leave local room at tight multi-port junctions. Paint/crossings on
            // neighboring arms must not overlap at their outside corners.
            if( std::any_of( result.begin(), result.end(), [&]( const Crossing &previous ) {
                if( previous.roadIndex != noRoad && previous.roadIndex == crossing.roadIndex )
                    return previous.lastStation + .60f > crossing.firstStation &&
                           crossing.lastStation + .60f > previous.firstStation;
                auto offset = previous.centre - crossing.centre;
                offset.y = 0;
                const float separation = std::max( 3.4f, ( previous.width + crossing.width ) * .37f );
                return offset.lengthSquared() < separation * separation;
            } ) ) return;
            result.push_back( crossing );
        };

        for( const auto &junction : source.roundabouts )
        {
            for( const auto &port : junction.ports )
            {
                if( result.size() >= roundaboutLimit ) break;
                constexpr float depth = 3.f;
                const bool raisedRefuge = port.width >= 6.f && port.length >= 12.f;
                const float start = raisedRefuge ? std::min( 8.f, port.length * .5f ) + .65f : 3.15f;
                if( port.width < 5.5f || start + depth > port.length - .70f ) continue;
                auto direction = port.outward;
                direction.y = 0;
                direction.normalise();
                const auto p = junction.center + direction * ( junction.outerRadius + start + depth * .5f );
                admit( { p, direction, { direction.z, 0, -direction.x }, port.width, depth,
                         noRoad, 0, 0, true } );
            }
        }

        for( const auto &junction : connections.junctions )
        {
            for( const auto &port : junction.ports )
            {
                if( result.size() >= totalLimit ) return result;
                const float width = source.roads[port.roadIndex].width;
                if( width < 6.f ) continue;
                constexpr float offset = 1.25f, depth = 2.8f;
                const float sign = port.forward ? 1.f : -1.f;
                const float s0 = port.station + sign * offset;
                const float s1 = port.station + sign * ( offset + depth );
                const auto run = std::find_if( connections.roadRuns.begin(), connections.roadRuns.end(),
                    [&]( const auto &candidate ) {
                        return candidate.roadIndex == port.roadIndex && candidate.points.size() >= 2 &&
                               std::abs( port.station - ( port.forward ? candidate.points.front().distance :
                                                                      candidate.points.back().distance ) ) < .002f &&
                               std::min( s0, s1 ) > candidate.points.front().distance + .25f &&
                               std::max( s0, s1 ) < candidate.points.back().distance - .65f;
                    } );
                if( run == connections.roadRuns.end() ) continue;
                const auto a = positionAt( run->points, s0 ), b = positionAt( run->points, s1 );
                const auto mid = positionAt( run->points, ( s0 + s1 ) * .5f );
                auto direction = b - a;
                direction.y = 0;
                const float length = direction.length();
                if( length < depth * .965f ) continue;
                direction /= length;
                auto heading = port.boundary.tangent;
                heading.y = 0;
                heading.normalise();
                if( direction.dotProduct( heading ) * sign < .993f ||
                    std::abs( crossXZ( direction, mid - ( a + b ) * .5f ) ) > .085f ) continue;
                admit( { mid, direction, { direction.z, 0, -direction.x }, width, depth,
                         port.roadIndex, std::min( s0, s1 ), std::max( s0, s1 ), false } );
            }
        }
        return result;
    }
}  // namespace workphone::scene::cityRoadCrossings
