#pragma once

namespace cityShopInterior
{
    struct Surfaces
    {
        Geometry lining, rearWall, timber, fittings, display, luminaires, glazing, packages;
    };

    struct Materials
    {
        SmartPtr<render::IMaterial> lining, rearWall, timber, fittings, display, luminaires, packages;
    };

    void shadeRearWall( SceneAssets &assets, SmartPtr<render::IMaterial> surface )
    {
        constexpr u32 size = 128;
        TextureBuffer paint( size, size ), bouncedLight( size, size );
        const auto channel = []( float value ) { return u8( std::clamp( value, 0.f, 255.f ) + .5f ); };
        for( u32 y = 0; y < size; ++y )
        {
            for( u32 x = 0; x < size; ++x )
            {
                const float u = ( x + .5f ) / size, v = ( y + .5f ) / size;
                const float fall = 1.f - v;
                const float spread = .071f + .22f * fall;
                const auto lamp = [&]( float at ) {
                    const float across = ( u - at ) / spread;
                    return std::exp( -.85f * across * across - 3.5f * fall ) *
                           std::min( 1.f, fall * 24.f );
                };
                const float pool = std::min( 1.f, lamp( .28f ) + lamp( .72f ) );
                const float corners = std::clamp( 4.f * std::min( u, 1.f - u ), 0.f, 1.f );
                const float grain =
                    float( render::foliageStableHash( y * size + x ) >> 40 ) / 16777216.f - .5f;
                const float plaster = 164.f + 12.f * corners + 19.f * pool + 5.f * grain;
                const auto colour = paint.pixel( x, y );
                colour[0] = channel( plaster + 9.f );
                colour[1] = channel( plaster + 3.f );
                colour[2] = channel( plaster - 6.f );
                colour[3] = 255;
                const auto light = bouncedLight.pixel( x, y );
                light[0] = channel( 91.f + 37.f * corners + 122.f * pool );
                light[1] = channel( 93.f + 34.f * corners + 104.f * pool );
                light[2] = channel( 92.f + 32.f * corners + 73.f * pool );
                light[3] = 255;
            }
        }
        surface->setTexture( upload( assets, paint ),
                             static_cast<u32>( PbsTextureTypes::PBSM_DIFFUSE ) );
        surface->setTexture( upload( assets, bouncedLight ),
                             static_cast<u32>( PbsTextureTypes::PBSM_EMISSIVE ) );
    }

    constexpr u32 labelCount = 8;

    SmartPtr<render::IMaterial> packageMaterial( SceneAssets &assets )
    {
        constexpr u32 size = 32;
        const std::array<Vector3F, labelCount> inks{
            Vector3F( 168, 66, 47 ), Vector3F( 74, 114, 83 ), Vector3F( 178, 133, 52 ),
            Vector3F( 58, 103, 134 ), Vector3F( 141, 78, 91 ), Vector3F( 59, 124, 123 ),
            Vector3F( 180, 99, 54 ), Vector3F( 114, 125, 70 ) };
        const Vector3F paper( 228, 215, 178 );
        TextureBuffer labels( size * labelCount, size );
        for( u32 tile = 0; tile < labelCount; ++tile )
        {
            const auto ink = inks[tile];
            for( u32 y = 0; y < size; ++y )
            {
                for( u32 x = 0; x < size; ++x )
                {
                    const float u = ( x + .5f ) / size, v = ( y + .5f ) / size;
                    auto colour = ink;
                    if( v < .13f || v > .9f )
                    {
                        colour = paper * .86f;
                    }
                    else if( tile % 2 == 0 && v > .41f && v < .79f )
                    {
                        colour = paper;
                    }
                    else if( tile % 2 && std::hypot( ( u - .5f ) * 1.4f, v - .57f ) < .27f )
                    {
                        colour = paper;
                    }
                    if( u > .29f && u < .71f && v > .53f && v < .60f )
                    {
                        colour = ink * .52f;
                    }
                    if( u > .22f && u < .78f && v > .23f && v < .27f )
                    {
                        colour = paper;
                    }
                    auto pixel = labels.pixel( tile * size + x, y );
                    pixel[0] = u8( colour.x );
                    pixel[1] = u8( colour.y );
                    pixel[2] = u8( colour.z );
                    pixel[3] = 255;
                }
            }
        }
        auto stock = material( assets, ColourF::White, .71f );
        stock->setUVProjection( 0 );
        stock->setEmissive( { .27f, .25f, .22f, 1 } );
        const auto print = upload( assets, labels, render::TextureMipFilter::Colour, labelCount );
        stock->setTexture( print, static_cast<u32>( PbsTextureTypes::PBSM_DIFFUSE ) );
        stock->setTexture( print, static_cast<u32>( PbsTextureTypes::PBSM_EMISSIVE ) );
        return stock;
    }

    Materials materials( SceneAssets &assets, bool hasStock )
    {
        const auto room = [&]( ColourF colour, ColourF ambient, float roughness ) {
            auto surface = material( assets, colour, roughness );
            surface->setEmissive( ambient );
            return surface;
        };
        auto rear = room( { .63f, .57f, .50f, 1 }, { .22f, .16f, .105f, 1 }, .82f );
        rear->setUVProjection( 0 );
        shadeRearWall( assets, rear );
        return { room( { .46f, .414f, .346f, 1 }, { .138f, .105f, .072f, 1 }, .82f ),
                 rear,
                 room( { .37f, .27f, .17f, 1 }, { .083f, .048f, .027f, 1 }, .72f ),
                 room( { .20f, .231f, .221f, 1 }, { .026f, .032f, .028f, 1 }, .58f ),
                 room( { .62f, .55f, .44f, 1 }, { .19f, .154f, .101f, 1 }, .70f ),
                 room( { .66f, .53f, .35f, 1 }, { .67f, .48f, .25f, 1 }, .48f ),
                 hasStock ? packageMaterial( assets ) : nullptr };
    }

    const char *styleName( cityShopfront::Style style )
    {
        if( style == cityShopfront::Style::Studio )
        {
            return "studio";
        }
        return style == cityShopfront::Style::Cafe ? "cafe" : "market";
    }

    template <class Point>
    void append( Surfaces &surfaces, cityShopfront::Style style, Vector3F normal, Vector3F along,
                 const Point &point, float left, float low, float right, float high, float front,
                 u32 variation )
    {
        const float width = right - left;
        const float height = high - low;
        const float mid = ( right + left ) * .5f;
        const float depth = std::clamp( width * .67f, .98f, 1.83f );
        const float rear = front + depth;
        const float innerLow = low - .10f, innerHigh = high + .025f;
        const auto panel = [&]( Geometry &g, float a, float bottom, float b, float top, float inset ) {
            const float orientation = normal.dotProduct( along.crossProduct( Vector3F::unitY() ) );
            if( orientation < 0 )
            {
                std::swap( a, b );
            }
            g.quad( point( a, bottom, inset ), point( b, bottom, inset ), point( b, top, inset ),
                    point( a, top, inset ), normal );
        };
        panel( surfaces.rearWall, left, innerLow, right, innerHigh, rear );
        surfaces.lining.quad( point( left, innerLow, front ), point( left, innerLow, rear ),
                              point( left, innerHigh, rear ), point( left, innerHigh, front ), along );
        surfaces.lining.quad( point( right, innerLow, front ), point( right, innerHigh, front ),
                              point( right, innerHigh, rear ), point( right, innerLow, rear ), -along );
        surfaces.lining.quad( point( left, innerHigh, front ), point( left, innerHigh, rear ),
                              point( right, innerHigh, rear ), point( right, innerHigh, front ),
                              -Vector3F::unitY() );
        surfaces.timber.quad( point( left, innerLow, front ), point( right, innerLow, front ),
                              point( right, innerLow, rear ), point( left, innerLow, rear ),
                              Vector3F::unitY() );
        panel( surfaces.glazing, left, low, right, high, front );

        const auto item = [&]( Geometry &g, float s, float y, float inset, float itemWidth,
                               float itemHeight, float itemDepth ) {
            const auto centre = point( mid + s * width, low + y * height, front + inset * depth );
            const auto size = std::abs( normal.x ) > .5f ? Vector3F( itemDepth, itemHeight, itemWidth )
                                                         : Vector3F( itemWidth, itemHeight, itemDepth );
            g.box( centre, size );
        };
        item( surfaces.fittings, 0, .982f, .68f, width * .66f, .031f, .085f );
        for( float side : { -.22f, .22f } )
        {
            item( surfaces.fittings, side, .943f, .68f, .14f, .12f, .17f );
            item( surfaces.luminaires, side, .891f, .658f, .102f, .016f, .112f );
        }
        const float relativeSide = ( variation & 1u ) ? -1.f : 1.f;
        if( style == cityShopfront::Style::Studio )
        {
            const bool portrait = ( variation & 2u ) != 0;
            const float artWidth = std::min( width * .52f, portrait ? .73f : 1.42f );
            const float artHeight = std::min( height * .49f, portrait ? 1.02f : .73f );
            const float artSide = -.13f * relativeSide;
            item( surfaces.fittings, artSide, .57f, .974f, artWidth + .050f, artHeight + .050f, .052f );
            item( surfaces.display, artSide, .57f, .954f, artWidth, artHeight, .020f );
            item( surfaces.timber, artSide - artWidth * .12f / width, .57f, .936f, artWidth * .36f,
                  artHeight * .44f, .010f );
            item( surfaces.fittings, artSide + artWidth * .15f / width, .51f, .933f, artWidth * .23f,
                  artHeight * .26f, .009f );
            item( surfaces.display, .23f * relativeSide, .095f, .43f, width * .18f, height * .23f,
                  depth * .26f );
            item( surfaces.timber, .23f * relativeSide, .24f, .43f, width * .096f, height * .080f,
                  depth * .12f );
            item( surfaces.fittings, .23f * relativeSide, .30f, .43f, width * .054f, height * .043f,
                  depth * .074f );
        }
        else if( style == cityShopfront::Style::Cafe )
        {
            item( surfaces.timber, .055f, .115f, .68f, width * .70f, height * .29f, depth * .34f );
            item( surfaces.display, .055f, .273f, .67f, width * .74f, .066f, depth * .38f );
            item( surfaces.fittings, -.14f, .356f, .70f, width * .23f, height * .14f, depth * .19f );
            item( surfaces.display, -.14f, .46f, .72f, width * .17f, height * .064f, depth * .10f );
            for( float s : { .14f, .24f, .33f } )
            {
                item( surfaces.display, s, .321f, .67f - .3f * s, width * .08f, height * .072f,
                      depth * .12f );
            }
            item( surfaces.fittings, .23f, .65f, .971f, width * .32f, height * .23f, .042f );
            for( float y : { .606f, .651f, .696f } )
            {
                item( surfaces.display, .22f, y, .942f, width * ( .16f + y * .11f ), .017f, .012f );
            }
        }
        else
        {
            for( float side : { -.34f, .34f } )
            {
                item( surfaces.fittings, side, .32f, .78f, .070f, height * .72f, depth * .36f );
            }
            // Neighbouring facings carry the same line of goods. Keep each run
            // together while allowing the next shelf or window to carry different stock.
            const int facings = std::clamp( int( width * .69f / .23f ), 4, 8 );
            const float pitch = width * .69f / facings;
            for( int shelf = 0; shelf < 3; ++shelf )
            {
                const float y = .14f + .21f * shelf;
                item( surfaces.timber, 0, y, .77f, width * .72f, .053f, depth * .38f );
                for( int slot = 0; slot < facings; ++slot )
                {
                    const u64 key = render::foliageStableHash( ( u64( variation ) << 32 ) |
                                                              u32( shelf * 8 + slot / 2 ) );
                    const u32 tile = u32( key % labelCount );
                    const float packHeight = std::min( height * .175f, .19f + .027f * ( ( key >> 8 ) % 6 ) );
                    const float s = ( slot + .5f - facings * .5f ) * pitch / width;
                    const size_t first = surfaces.packages.uv.size();
                    item( surfaces.packages, s, y + ( packHeight + .053f ) / ( height * 2.f ),
                          .69f, std::min( pitch * .86f, .14f + .016f * ( ( key >> 16 ) % 5 ) ),
                          packHeight, std::min( depth * .20f, .19f ) );
                    for( size_t i = first; i < surfaces.packages.uv.size(); ++i )
                    {
                        auto &uv = surfaces.packages.uv[i];
                        const auto n = surfaces.packages.normals[i];
                        if( std::abs( n.y ) > .5f )
                        {
                            uv.y = .98f;
                        }
                        else if( std::abs( n.dotProduct( normal ) ) < .5f )
                        {
                            uv.x *= .18f;
                        }
                        uv.x = ( tile + .04f + .92f * uv.x ) / labelCount;
                    }
                }
            }
        }
    }
}  // namespace cityShopInterior
