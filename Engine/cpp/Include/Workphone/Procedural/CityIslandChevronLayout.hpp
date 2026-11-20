#pragma once

#include <Workphone/Interface/Procedural/CityRoadRoundabout.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace workphone::scene::cityIslandChevrons
{
    struct Board
    {
        std::size_t portIndex{};
        Vector3F centre{}, outward{};
        float width{}, height{}, arrowSign{};

        Vector3F right() const { return { outward.z, 0, -outward.x }; }
    };

    struct Layout
    {
        std::array<Board, 6> boards{};
        std::size_t count{};
    };

    inline Vector3F footing( const procedural::CityRoadRoundaboutSpec &island, const Board &board,
                             float lateral, float faceOffset = 0.f )
    {
        auto p = board.centre + board.right() * lateral + board.outward * faceOffset;
        p.y = island.center.y + island.islandHeight + ( p.x - island.center.x ) * island.gradeX +
              ( p.z - island.center.z ) * island.gradeZ;
        return p;
    }

    inline Layout layout( const procedural::CityRoadRoundaboutSpec &island )
    {
        Layout result;
        const float turf = island.islandRadius - island.kerbWidth;
        if( turf < 3.f || island.ports.size() < 3 || island.ports.size() > result.boards.size() ||
            island.ringSamples < 32 )
        {
            return result;
        }
        // The sett strip connects the bake's regular turf samples. Port endpoints
        // can extend the turf farther out, but offer no extra strip for a footing.
        const auto segments = island.ringSamples - 2 * island.ports.size();
        const float step = 6.28318531f / float( segments );
        const float apothem = turf * std::cos( step * .5f );
        for( std::size_t i = 0; i < island.ports.size(); ++i )
        {
            const auto &port = island.ports[i];
            Board board;
            board.portIndex = i;
            board.outward = Vector3F( port.outward.x, 0, port.outward.z ).normaliseCopy();
            board.width = std::clamp( .30f * port.width, 1.45f, 2.1f );
            board.height = std::clamp( board.width * .26f, .42f, .54f );
            board.arrowSign = island.clockwise ? -1.f : 1.f;
            // Offset towards the arriving lane. Leave clearance beyond the
            // plate ends and keep the rear mount in the 0.8 m planting setback.
            const float offset = -board.arrowSign * std::min( .58f, .08f * port.width );
            float radius = turf - .46f;
            float localPerimeter = turf;
            for( std::size_t edge = 0; edge < segments; ++edge )
            {
                const float angle = ( float( edge ) + .5f ) * step;
                const Vector3F normal( std::cos( angle ), 0, std::sin( angle ) );
                const float advance = normal.dotProduct( board.outward );
                if( advance <= 0 ) continue;
                const float side = normal.dotProduct( board.right() );
                const float remaining = apothem - side * offset;
                localPerimeter = std::min( localPerimeter, remaining / advance );
                // Keep the middle at least 40 cm inboard and both ends 16 cm
                // clear, including where a wide board spans a polygon vertex.
                const float clearance = std::max( .40f, .16f + std::abs( side ) * board.width * .5f );
                radius = std::min( radius, ( remaining - clearance ) / advance );
            }
            if( radius < localPerimeter - .66f ) continue;
            board.centre = island.center + board.outward * radius + board.right() * offset;
            board.centre = footing( island, board, 0 ) + Vector3F( 0, .45f + board.height * .5f, 0 );
            const auto overlaps = [&]( const Board &previous ) {
                const float separation = .28f + .5f * ( previous.width + board.width );
                return std::hypot( board.centre.x - previous.centre.x, board.centre.z - previous.centre.z ) <
                       separation;
            };
            if( !std::any_of( result.boards.begin(), result.boards.begin() + result.count, overlaps ) )
            {
                result.boards[result.count++] = board;
            }
        }
        return result;
    }
}
