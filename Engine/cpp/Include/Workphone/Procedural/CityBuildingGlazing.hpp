#pragma once

namespace cityBuildingGlazing
{
    constexpr u32 atlasColumns = 8, atlasCell = 64;
    constexpr float padding = 5.5f / atlasCell;

    struct Materials
    {
        std::array<SmartPtr<render::IMaterial>, 3> plain;
        SmartPtr<render::IMaterial> panes;
    };

    float smooth( float value )
    {
        const float t = std::clamp( value, 0.f, 1.f );
        return t * t * ( 3.f - 2.f * t );
    }

    Materials build( SceneAssets &assets, bool detailed )
    {
        Materials result{ { material( assets, { .178f, .199f, .210f, 1 }, .18f ),
                            material( assets, { .218f, .225f, .223f, 1 }, .25f ),
                            material( assets, { .286f, .301f, .301f, 1 }, .40f ) },
                          {} };
        for( auto &glass : result.plain )
        {
            glass->setSpecular( { .071f, .074f, .076f, 1 } );
        }
        if( !detailed )
        {
            return result;
        }

        TextureBuffer interior( atlasColumns * atlasCell, atlasCell );
        TextureBuffer microsurface( interior.width, interior.height );
        TextureBuffer cavity( interior.width, interior.height );
        const auto coordinate = []( u32 index ) {
            // The apron around each pane repeats its edge values into the mip gutter.
            return std::clamp( ( ( index + .5f ) / atlasCell - padding ) / ( 1.f - 2.f * padding ), 0.f,
                               1.f );
        };
        const std::array<Vector3F, 4> room = { Vector3F( 46.f, 52.f, 55.f ),
                                               Vector3F( 51.f, 54.f, 54.f ),
                                               Vector3F( 46.f, 53.f, 58.f ),
                                               Vector3F( 49.f, 53.f, 54.f ) };
        const std::array<float, atlasColumns> roughness = { .125f, .168f, .194f, .233f,
                                                            .147f, .185f, .163f, .205f };
        const auto scalar = []( TextureBuffer &image, u32 x, u32 y, float value ) {
            const auto pixel = image.pixel( x, y );
            pixel[0] = pixel[1] = pixel[2] = u8( 255.f * std::clamp( value, 0.f, 1.f ) + .5f );
            pixel[3] = 255;
        };
        for( u32 tile = 0; tile < atlasColumns; ++tile )
        {
            for( u32 y = 0; y < atlasCell; ++y )
            {
                for( u32 x = 0; x < atlasCell; ++x )
                {
                    const float u = coordinate( x ), v = coordinate( y );
                    const float jamb = 1.f - smooth( 12.5f * std::min( u, 1.f - u ) );
                    const float sill = 1.f - smooth( 10.f * v );
                    const float head = 1.f - smooth( 9.f * ( 1.f - v ) );
                    const float topLight = smooth( ( v - .13f ) * 1.37f );
                    const float bounce = std::cos( ( u + .19f * tile ) * pi ) * ( 3.f + 4.f * topLight );
                    auto colour = room[tile % room.size()] + Vector3F( 24.f, 23.f, 21.f ) * topLight +
                                  Vector3F( bounce, bounce, bounce );
                    if( tile == 4 || tile == 5 )
                    {
                        const float fromSide = tile == 4 ? u : std::min( u, 1.f - u );
                        const float opening = ( tile == 4 ? .38f : .24f ) + .047f * std::cos( pi * v );
                        const float cover = smooth( ( opening - fromSide ) * 28.f );
                        const float folds =
                            .91f + .09f * std::cos( ( fromSide * 6.f + .084f * v ) * 2.f * pi );
                        const auto fabric =
                            Vector3F( 98.f, 92.f, 81.f ) * ( folds * ( .85f + .15f * topLight ) );
                        colour += ( fabric - colour ) * ( .85f * cover );
                    }
                    else if( tile >= 6 )
                    {
                        const float hem = tile == 6 ? .48f : .26f;
                        const float cover = smooth( ( v - hem ) * 30.f );
                        const float slats =
                            tile == 6 ? 1.f : .94f + .06f * std::cos( ( v - hem ) * 12.5f * 2.f * pi );
                        const auto blind =
                            Vector3F( 105.f, 106.f, 101.f ) * ( slats * ( .83f + .17f * topLight ) );
                        colour += ( blind - colour ) * ( .91f * cover );
                    }

                    const auto atlasX = tile * atlasCell + x;
                    const auto pixel = interior.pixel( atlasX, y );
                    const float recess = 1.f - .10f * jamb - .09f * sill - .12f * head;
                    pixel[0] = u8( colour.x * recess + .5f );
                    pixel[1] = u8( colour.y * recess + .5f );
                    pixel[2] = u8( colour.z * recess + .5f );
                    pixel[3] = 255;
                    // Slight differences between installed units and more diffuse
                    // edges retain the true cubemap/Fresnel response in daylight.
                    scalar( microsurface, atlasX, y,
                            roughness[tile] + .062f * jamb + .057f * sill + .040f * head +
                                .017f * smooth( ( .35f - v ) * 3.f ) );
                    scalar( cavity, atlasX, y, 1.f - .19f * jamb - .22f * sill - .20f * head );
                }
            }
        }

        auto &panes = result.panes;
        panes = material( assets, ColourF::White, 1.f );
        panes->setTexture( upload( assets, interior, render::TextureMipFilter::Colour, atlasColumns ),
                           static_cast<u32>( PbsTextureTypes::PBSM_DIFFUSE ) );
        panes->setTexture(
            upload( assets, microsurface, render::TextureMipFilter::Roughness, atlasColumns ),
            static_cast<u32>( PbsTextureTypes::PBSM_ROUGHNESS ) );
        panes->setTexture( upload( assets, cavity, render::TextureMipFilter::Data, atlasColumns ), 22 );
        panes->setEditorFloat( "aoStrength", .78f );
        panes->setSpecular( { .076f, .079f, .083f, 1 } );
        return result;
    }
}  // namespace cityBuildingGlazing
