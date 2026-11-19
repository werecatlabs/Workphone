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
    void windowHeadBlock( Geometry &geometry, const Point &point, Vector3F normal, Vector3F along,
                          float left, float right, float lowerLeft, float lowerRight,
                          float upperLeft, float upperRight, float projection )
    {
        const auto a = point( left, lowerLeft, -projection );
        const auto b = point( right, lowerRight, -projection );
        const auto c = point( right, upperRight, -projection );
        const auto d = point( left, upperLeft, -projection );
        const auto rearA = point( left, lowerLeft, 0 );
        const auto rearB = point( right, lowerRight, 0 );
        const auto rearC = point( right, upperRight, 0 );
        const auto rearD = point( left, upperLeft, 0 );
        const auto up = Vector3F::unitY() * ( right - left );
        geometry.quad( a, b, c, d, normal );
        geometry.quad( rearD, d, c, rearC, ( up - along * ( upperRight - upperLeft ) ).normaliseCopy() );
        geometry.quad( rearA, a, b, rearB, ( along * ( lowerRight - lowerLeft ) - up ).normaliseCopy() );
        geometry.quad( rearA, a, d, rearD, -along );
        geometry.quad( rearB, b, c, rearC, along );
    }

    template <class Point>
    void dressWindow( Geometry &trim, Geometry &timber, cityBuildingWindowKit::Head headStyle,
                      const Point &point, Vector3F normal, Vector3F along, float left, float right,
                      float sill, float head )
    {
        using cityBuildingWindowKit::Head;
        const float width = right - left, centre = ( left + right ) * .5f;
        const float overhang = std::min( .18f, width * .09f );
        const float a = left - overhang, b = right + overhang;
        auto &finish = headStyle == Head::Timber ? timber : trim;
        moulding( finish, point, normal, along, a, b, sill - .14f, .14f, .20f );
        const auto block = [&]( float x0, float x1, float low0, float low1,
                                float high0, float high1, float depth ) {
            windowHeadBlock( finish, point, normal, along, x0, x1, low0, low1, high0, high1, depth );
        };
        const auto course = [&]( float x0, float x1, float low, float high, float depth ) {
            block( x0, x1, low, low, high, high, depth );
        };
        switch( headStyle )
        {
        case Head::Segmental:
        {
            // Five separate voussoirs follow a shallow camber above the
            // rectangular reveal. The wall remains visible in the mortar joints.
            const auto spring = [&]( float x ) {
                const float t = ( x - centre ) / ( b - centre );
                return head + .018f + .17f * ( 1.f - t * t );
            };
            for( int stone = 0; stone < 5; ++stone )
            {
                const float x0 = a + ( b - a ) * ( stone / 5.f ) + .008f;
                const float x1 = a + ( b - a ) * ( ( stone + 1 ) / 5.f ) - .008f;
                const bool key = stone == 2;
                const float height = key ? .21f : .16f;
                block( x0, x1, spring( x0 ), spring( x1 ), spring( x0 ) + height,
                       spring( x1 ) + height, key ? .18f : .12f );
            }
            break;
        }
        case Head::Pediment:
            course( a, b, head, head + .11f, .19f );
            block( a, centre, head + .11f, head + .11f, head + .17f, head + .40f, .14f );
            block( centre, b, head + .11f, head + .11f, head + .40f, head + .17f, .14f );
            break;
        case Head::Stepped:
            course( a, b, head, head + .12f, .19f );
            course( left + .08f, right - .08f, head + .12f, head + .24f, .14f );
            course( centre - width * .21f, centre + width * .21f, head + .24f, head + .36f, .09f );
            break;
        case Head::Timber:
            course( left - .11f, left, sill, head, .095f );
            course( right, right + .11f, sill, head, .095f );
            course( a, b, head, head + .16f, .13f );
            break;
        case Head::Lintel:
            moulding( trim, point, normal, along, a, b, head, .19f, .18f );
            break;
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
