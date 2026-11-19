#pragma once

namespace cityBuildingGlazing
{
    // Clear/curtained units followed by the same eight interiors in a sash.
    // Each tile stays independent down to the last mip of the horizontal strip.
    constexpr u32 paneVariants = 8, atlasColumns = 2 * paneVariants, atlasCell = 64;
    constexpr float padding = 5.5f / atlasCell;

    struct Materials
    {
        std::array<SmartPtr<render::IMaterial>, 3> plain;
        SmartPtr<render::IMaterial> panes;
    };

    // At middle distance a curtain wall keeps groups of real bays. The gaps
    // between groups expose the backing wall and read as structural divisions.
    struct OfficeRibbon
    {
        Vector3F left, right, normal;
        f32 height;
        // Up to four groups. Internal boundaries are actual bay divisions
        // expressed as fractions of the glass span (after the corner piers).
        std::array<f32, 5> boundaries{};
        std::array<bool, 4> shaded{};
        int groups = 1;
        bool centreShaded = false;
    };

    template <class Shade>
    OfficeRibbon officeRibbon( Vector3F left, Vector3F right, Vector3F normal, f32 height,
                               f32 span, f32 pier, int bays, const Shade &shade )
    {
        OfficeRibbon ribbon{ left, right, normal, height };
        ribbon.groups = std::min( 4, bays );
        ribbon.centreShaded = shade( bays / 2 );
        for( int i = 0; i < ribbon.groups; ++i )
        {
            const int first = i * bays / ribbon.groups;
            const int next = ( i + 1 ) * bays / ribbon.groups;
            ribbon.shaded[i] = shade( ( first + next - 1 ) / 2 );
            if( i > 0 )
            {
                ribbon.boundaries[i] =
                    std::clamp( ( span * first / bays - pier ) / ( span - 2.f * pier ), 0.f, 1.f );
            }
        }
        ribbon.boundaries[ribbon.groups] = 1.f;
        return ribbon;
    }

    inline void emitOfficeRibbons( SourceBudgetVector<OfficeRibbon> &ribbons, Geometry ( &glass )[2],
                                   size_t additionalQuads )
    {
        for( const auto &ribbon : ribbons )
        {
            const auto across = ribbon.right - ribbon.left;
            const Vector3F up( 0, ribbon.height, 0 );
            const auto emit = [&]( bool shaded, float a, float b ) {
                const auto left = ribbon.left + across * a;
                const auto right = ribbon.left + across * b;
                glass[shaded ? 1 : 0].quad( left, right, right + up, left + up, ribbon.normal );
            };
            if( size_t( ribbon.groups - 1 ) > additionalQuads )
            {
                emit( ribbon.centreShaded, 0.f, 1.f );
                continue;
            }
            additionalQuads -= size_t( ribbon.groups - 1 );
            // 14 cm divisions survive the transition without drawing subpixel bars.
            const float gap = .07f / std::max( 1.f, across.length() );
            for( int i = 0; i < ribbon.groups; ++i )
            {
                const float a = ribbon.boundaries[i] + ( i > 0 ? gap : 0.f );
                const float b = ribbon.boundaries[i + 1] - ( i + 1 < ribbon.groups ? gap : 0.f );
                emit( ribbon.shaded[i], a, b );
            }
        }
        ribbons.releaseStorage();
    }

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
        const std::array<float, paneVariants> roughness = { .125f, .168f, .194f, .233f,
                                                             .147f, .185f, .163f, .205f };
        const auto scalar = []( TextureBuffer &image, u32 x, u32 y, float value ) {
            const auto pixel = image.pixel( x, y );
            pixel[0] = pixel[1] = pixel[2] = u8( 255.f * std::clamp( value, 0.f, 1.f ) + .5f );
            pixel[3] = 255;
        };
        const auto writePane = [&]( u32 x, u32 y, const Vector3F &colour, float rough, float occlusion ) {
            auto pixel = interior.pixel( x, y );
            pixel[0] = u8( colour.x + .5f );
            pixel[1] = u8( colour.y + .5f );
            pixel[2] = u8( colour.z + .5f );
            pixel[3] = 255;
            scalar( microsurface, x, y, rough );
            scalar( cavity, x, y, occlusion );
        };
        for( u32 tile = 0; tile < paneVariants; ++tile )
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
                    auto colour = room[tile % room.size()] + Vector3F( 26.f, 29.f, 32.f ) * topLight +
                                  Vector3F( bounce, bounce, bounce );
                    if( tile == 4 || tile == 5 )
                    {
                        const float fromSide = tile == 4 ? u : std::min( u, 1.f - u );
                        const float opening = ( tile == 4 ? .38f : .24f ) + .047f * std::cos( pi * v );
                        const float cover = smooth( ( opening - fromSide ) * 28.f );
                        const float folds =
                            .91f + .09f * std::cos( ( fromSide * 6.f + .084f * v ) * 2.f * pi );
                        const auto fabric =
                            Vector3F( 118.f, 110.f, 96.f ) * ( folds * ( .85f + .15f * topLight ) );
                        colour += ( fabric - colour ) * ( .85f * cover );
                    }
                    else if( tile >= 6 )
                    {
                        const float hem = tile == 6 ? .48f : .26f;
                        const float cover = smooth( ( v - hem ) * 30.f );
                        const float slats =
                            tile == 6 ? 1.f : .94f + .06f * std::cos( ( v - hem ) * 12.5f * 2.f * pi );
                        const auto blind =
                            Vector3F( 121.f, 122.f, 115.f ) * ( slats * ( .83f + .17f * topLight ) );
                        colour += ( blind - colour ) * ( .91f * cover );
                    }

                    const auto atlasX = tile * atlasCell + x;
                    const float recess = 1.f - .10f * jamb - .09f * sill - .12f * head;
                    colour *= recess;
                    const float paneRoughness = roughness[tile] + .062f * jamb + .057f * sill +
                                                .040f * head + .017f * smooth( ( .35f - v ) * 3.f );
                    const float paneCavity = 1.f - .19f * jamb - .22f * sill - .20f * head;
                    writePane( atlasX, y, colour, paneRoughness, paneCavity );

                    // A 2 m wide opening has a roughly 55 mm meeting stile and 60 mm casing.
                    // Soften their outlines over one source texel before mip filtering.
                    const float casing = std::min( { u, 1.f - u, v, 1.f - v } ) - .030f;
                    const float stile = std::abs( u - .5f ) - .014f;
                    const float transom = std::abs( v - .72f ) - .013f;
                    const float distance = std::min( { casing, stile, transom } );
                    const float frame = 1.f - smooth( ( distance + .009f ) / .018f );
                    const float seal = 1.f - smooth( ( distance - .010f ) / .018f );
                    auto framed = colour * ( 1.f - .33f * seal );
                    const auto paint = Vector3F( 157.f, 156.f, 146.f ) * ( .90f + .10f * v );
                    framed += ( paint - framed ) * frame;
                    writePane( atlasX + paneVariants * atlasCell, y, framed,
                               paneRoughness + ( .63f - paneRoughness ) * frame,
                               paneCavity + ( .97f - paneCavity ) * frame );
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
