#pragma once

#include "CityFacadeDetailMaterials.hpp"

namespace cityFacadeDetail
{
    struct GeometryBatches
    {
        Geometry panels, profiles, mullions;
    };

    struct Counts
    {
        size_t storeys = 0, panels = 0, profiles = 0, mullions = 0;
    };

    template <class Point>
    void moulding( Geometry &geometry, const Point &point, Vector3F normal, Vector3F along, float left,
                   float right, float bottom, float height, float projection )
    {
        const std::array<Vector2F, 4> section{ Vector2F( -.002f, 0 ), Vector2F( projection, .023f ),
                                               Vector2F( projection, height - .029f ),
                                               Vector2F( -.002f, height ) };
        const auto vertex = [&]( float side, const Vector2F &v ) {
            return point( side, bottom + v.y, -v.x );
        };
        for( size_t i = 1; i < section.size(); ++i )
        {
            const auto &a = section[i - 1], &b = section[i];
            const auto face =
                ( normal * ( b.y - a.y ) - Vector3F::unitY() * ( b.x - a.x ) ).normaliseCopy();
            geometry.quad( vertex( left, a ), vertex( right, a ), vertex( right, b ), vertex( left, b ),
                           face );
        }
        geometry.quad( vertex( left, section[0] ), vertex( left, section[1] ),
                       vertex( left, section[2] ), vertex( left, section[3] ), -along );
        geometry.quad( vertex( right, section[0] ), vertex( right, section[1] ),
                       vertex( right, section[2] ), vertex( right, section[3] ), along );
    }

    template <class Point>
    void pilaster( Geometry &geometry, const Point &point, Vector3F normal, Vector3F along, float centre,
                   float low, float high )
    {
        const std::array<Vector2F, 4> section{ Vector2F( -.145f, -.002f ), Vector2F( -.120f, .077f ),
                                               Vector2F( .120f, .077f ), Vector2F( .145f, -.002f ) };
        const auto vertex = [&]( const Vector2F &v, float y ) { return point( centre + v.x, y, -v.y ); };
        for( size_t i = 1; i < section.size(); ++i )
        {
            const auto &a = section[i - 1], &b = section[i];
            const auto face = ( normal * ( b.x - a.x ) - along * ( b.y - a.y ) ).normaliseCopy();
            geometry.quad( vertex( a, low ), vertex( b, low ), vertex( b, high ), vertex( a, high ),
                           face );
        }
        for( const float y : { low, high } )
        {
            geometry.quad( vertex( section[0], y ), vertex( section[1], y ), vertex( section[2], y ),
                           vertex( section[3], y ), Vector3F( 0, y == low ? -1.f : 1.f, 0 ) );
        }
    }

    template <class Point>
    void officeStorey( GeometryBatches &geometry, Counts &counts, const Point &point, Vector3F normal,
                       Vector3F along, float span, int bays, float y0, float sill, float head, float y1,
                       size_t &remaining )
    {
        if( span < 4.f || sill - y0 < .42f || y1 - head < .37f || head - sill < 1.2f )
        {
            return;
        }
        const int panelCount = std::clamp( bays * 2, 2, 16 );
        const size_t cost = size_t( 24 * ( 2 * panelCount + std::max( 0, bays - 1 ) ) + 80 );
        if( remaining < cost )
        {
            return;
        }
        remaining -= cost;

        const float ribbonLeft = -span * .5f + .38f, ribbonRight = -ribbonLeft;
        const auto box = [&]( Geometry &target, float s, float y, float outward, float width,
                              float height, float depth ) {
            const auto dimensions = std::abs( normal.x ) > .5f ? Vector3F( depth, height, width )
                                                               : Vector3F( width, height, depth );
            target.box( point( s, y, -outward ), dimensions );
        };
        const auto panels = [&]( float low, float high ) {
            const float step = ( ribbonRight - ribbonLeft ) / panelCount;
            for( int panel = 0; panel < panelCount; ++panel )
            {
                const float x = ribbonLeft + step * ( panel + .5f );
                box( geometry.panels, x, ( low + high ) * .5f, .020f, step - .025f, high - low, .043f );
            }
        };
        panels( y0 + .042f, sill - .142f );
        panels( head + .119f, y1 - .061f );
        for( const float side : { -1.f, 1.f } )
        {
            const float x = side * ( span * .5f - .195f );
            pilaster( geometry.panels, point, normal, along, x, y0 + .041f, y1 - .041f );
        }

        // The projecting lower nose and sloping top throw a small shadow
        // under the ribbon. Both profiles terminate inside the corner pier.
        moulding( geometry.profiles, point, normal, along, ribbonLeft - .044f, ribbonRight + .044f,
                  sill - .116f, .108f, .143f );
        moulding( geometry.profiles, point, normal, along, ribbonLeft - .042f, ribbonRight + .042f,
                  head + .020f, .083f, .080f );
        const float bayWidth = span / bays;
        for( int bay = 1; bay < bays; ++bay )
        {
            const float x = -span * .5f + bay * bayWidth;
            box( geometry.mullions, x, ( sill + head ) * .5f, .012f, .077f, head - sill - .014f, .088f );
        }

        ++counts.storeys;
        counts.panels += size_t( 2 * panelCount + 2 );
        counts.profiles += 2;
        counts.mullions += size_t( std::max( 0, bays - 1 ) );
    }
}  // namespace cityFacadeDetail
