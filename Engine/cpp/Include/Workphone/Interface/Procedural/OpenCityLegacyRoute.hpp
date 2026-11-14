#pragma once
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <array>

namespace workphone::procedural
{
    // Revision 2's route is not serialized in the editable v2 road source.
    // Reconstruct only its pinned path when reopening a saved natural city;
    // callers must still validate it against the current authored road surface.
    // Preserve float operation order and random domains for existing sources.
    inline SourceBudgetVector<OpenCityLayout::Point> legacyOpenCityNaturalRoute(
        std::uint32_t seed, int requestedBlocks, int routeChoice )
    {
        using Point = OpenCityLayout::Point;
        const int blocks = std::clamp( requestedBlocks / 2 * 2, 6, 10 );
        const float spacing = 88.f + float( seed % 17 );
        const float extent = blocks * spacing * .5f;
        const auto random = [seed]( std::uint64_t domain ) {
            std::uint64_t x = domain ^ ( std::uint64_t( seed ) << 32 ) ^ seed;
            x += 0x9e3779b97f4a7c15ull;
            x = ( x ^ ( x >> 30 ) ) * 0xbf58476d1ce4e5b9ull;
            x = ( x ^ ( x >> 27 ) ) * 0x94d049bb133111ebull;
            return float( ( x ^ ( x >> 31 ) ) >> 40 ) / 16777216.f;
        };
        std::array<Point, 8> shape = { Point{ 0, 0 }, { 0, -.55f }, { .45f, -1.f },
                                      { 1.25f, -.78f }, { 1.55f, -.10f }, { 1.10f, .72f },
                                      { .35f, .92f }, { 0, .42f } };
        for( std::size_t i = 1; i < shape.size(); ++i )
        {
            if( i != 1 && i != 7 )
            {
                shape[i].x += ( random( i * 101 ) - .5f ) * .025f;
            }
            shape[i].z += ( random( i * 101 + 7 ) - .5f ) * .025f;
        }
        const float radius = extent * ( .33f + std::size_t( std::clamp( routeChoice, 0, 2 ) ) * .115f );
        for( auto &point : shape )
        {
            point = { point.x * radius, point.z * radius };
        }
        SourceBudgetVector<Point> route;
        route.reserve( 2048 );
        for( std::size_t i = 0; i < shape.size(); ++i )
        {
            const auto a = shape[i], b = shape[( i + 1 ) % shape.size()];
            const auto steps = std::max( 1, int( std::ceil( std::hypot( b.x - a.x, b.z - a.z ) / 2 ) ) );
            for( int j = 0; j < steps; ++j )
            {
                const float t = float( j ) / steps;
                route.push_back( { a.x + ( b.x - a.x ) * t, a.z + ( b.z - a.z ) * t } );
            }
        }
        return route;
    }
}  // namespace workphone::procedural
