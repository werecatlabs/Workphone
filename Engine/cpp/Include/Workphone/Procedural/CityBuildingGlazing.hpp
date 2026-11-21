#pragma once

namespace cityBuildingGlazing
{
    // Commercial and domestic panes each have a bare and a framed bank. The
    // office bank sits behind the curtain wall's separately modelled mullions.
    // Each tile stays independent down to the last mip of the horizontal strip.
    constexpr u32 paneVariants = 8, domesticStart = 2 * paneVariants;
    constexpr u32 officeStart = 4 * paneVariants, atlasColumns = officeStart + paneVariants, atlasCell = 64;
    constexpr float padding = 5.5f / atlasCell;

    constexpr float officeBlindHem( u32 tile )
    {
        return tile == 6 ? .56f : .31f;
    }

    void pane( Geometry &geometry, u32 tile, Vector3F a, Vector3F b, Vector3F c, Vector3F d,
               Vector3F normal, float bottom = 0.f )
    {
        geometry.quad( a, b, c, d, normal, ( tile + padding ) / atlasColumns,
                       ( tile + 1.f - padding ) / atlasColumns,
                       padding + bottom * ( 1.f - 2.f * padding ), 1.f - padding );
    }

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

    SmartPtr<render::IMaterial> clearGlass( SceneAssets &assets )
    {
        auto glass = material( assets, { .22f, .30f, .32f, .28f }, .17f );
        glass->setSpecular( { .092f, .101f, .105f, 1 } );
        glass->setTransparent( true );
        glass->setBlendMode( 1 );
        glass->setDepthWrite( false );
        return glass;
    }

    Vector3F domesticPane( u32 tile, float u, float v )
    {
        static const std::array<Vector3F, 4> plaster{
            Vector3F( 133, 124, 108 ), Vector3F( 111, 126, 125 ),
            Vector3F( 143, 126, 111 ), Vector3F( 126, 132, 119 ) };
        const float across = tile % 2 ? 1.f - u : u;
        const float corner = .13f + .07f * ( tile % 3 );
        const float returnWall = 1.f - smooth( ( across - corner ) * 44.f );
        const float ceiling = smooth( ( v - .87f - .24f * std::max( 0.f, corner - across ) ) * 42.f );
        const float daylight = smooth( ( across + v * .40f - .31f ) * 1.75f );
        auto colour = plaster[tile % plaster.size()] * ( .61f + .29f * daylight - .19f * returnWall );
        colour += ( Vector3F( 111, 111, 103 ) - colour ) * ( .81f * ceiling );

        // A low cabinet on the back wall gives the opening a domestic scale.
        if( tile % 3 != 1 )
        {
            const float end = .56f + .13f * ( tile % 3 );
            const float top = .23f + .06f * ( tile % 2 );
            const float cabinet = smooth( ( across - corner ) * 40.f ) *
                                  smooth( ( end - across ) * 40.f ) * smooth( ( top - v ) * 60.f );
            const auto wood = Vector3F( 76, 66, 54 ) *
                              ( .89f + .28f * smooth( ( v - top + .05f ) * 28.f ) );
            colour += ( wood - colour ) * cabinet;
        }
        if( tile == 2 || tile == 3 )
        {
            const float hem = tile == 2 ? .59f : .36f;
            const float slat = .95f + .05f * std::cos( ( v - hem ) * 11.f * 2.f * pi );
            const auto shade = Vector3F( 165, 160, 142 ) * ( slat * ( .92f + .08f * v ) );
            colour += ( shade - colour ) * smooth( ( v - hem ) * 58.f );
        }
        if( tile >= 4 )
        {
            // Each panel gathers towards its tie, with a wider hem and header.
            const float waist = std::abs( v - .43f );
            const float left = .10f + .048f * ( tile - 4 ) + .44f * waist * waist;
            const float right = tile == 5 ? .02f : .13f + .05f * ( tile % 3 ) + .36f * waist * waist;
            const float edge = std::max( left - across, right - ( 1.f - across ) );
            const float fromJamb = std::min( across, 1.f - across );
            const float fold = .84f + .16f * std::cos( 2.f * pi * fromJamb * ( 11.f - 4.f * waist ) );
            const auto linen = tile == 6 ? Vector3F( 164, 177, 162 ) : Vector3F( 202, 187, 165 );
            colour += ( linen * ( fold * ( .85f + .15f * v ) ) - colour ) * smooth( edge * 56.f );
        }
        return colour;
    }

    Vector3F officePane( u32 tile, float u, float v )
    {
        const float x = tile & 1u ? 1.f - u : u;
        const auto band = []( float value, float low, float high ) {
            return smooth( ( value - low ) * 56.f + .5f ) * smooth( ( high - value ) * 56.f + .5f );
        };
        // Adjacent bays look into the same floor. Hold the near ceiling,
        // back-wall head and floor at common heights across both pane edges.
        const bool hasPartition = tile == 1 || tile == 4 || tile == 7;
        const float jamb = hasPartition ? band( x, .13f, .27f ) : 0.f;
        const float returnDepth = jamb * std::clamp( ( x - .13f ) / .14f, 0.f, 1.f );
        const float head = .74f + .14f * returnDepth, foot = .12f - .07f * returnDepth;
        const float daylight = .79f + .13f * v + .08f * smooth( ( x + .35f * v ) * 1.2f );
        // Raking daylight from the window leaves a soft wedge on the plaster.
        const float wash = smooth( ( .85f * x + .50f * v - .59f ) * 3.4f );
        auto colour = Vector3F( 139, 135, 122 ) * ( daylight - .22f * jamb + .13f * wash );
        const auto finish = [&]( Vector3F surface, float coverage ) {
            colour += ( surface - colour ) * coverage;
        };
        finish( Vector3F( 82, 86, 84 ) * daylight, smooth( ( foot - v ) * 52.f ) );
        finish( Vector3F( 145, 147, 142 ) * ( .82f + .18f * v ),
                smooth( ( v - head ) * 52.f ) );
        // The shadow at the soffit and inset ceiling light both run back along
        // the partition return, where it projects towards the glass.
        finish( Vector3F( 92, 97, 95 ), .65f * band( v, head - .025f, head ) );
        if( tile != 2 && tile != 5 )
        {
            const float lightV = v - .048f * x;
            finish( Vector3F( 175, 178, 167 ), band( lightV, .836f, .861f ) * band( x, .36f, .66f ) );
        }
        const auto rect = [&]( float l, float b, float r, float t, Vector3F surface ) {
            finish( surface, band( x, l, r ) * band( v, b, t ) );
        };
        if( tile == 2 || tile == 4 )
        {
            // A partly glazed meeting-room door against the back wall.
            rect( .34f, .105f, .57f, .70f, { 81, 83, 76 } );
            rect( .355f, .12f, .554f, .682f, { 133, 117, 94 } );
            rect( .38f, .34f, .526f, .65f, { 91, 108, 106 } );
            rect( .49f, .30f, .535f, .317f, { 164, 161, 149 } );
        }
        if( tile == 0 || tile == 3 || tile == 5 || tile == 7 )
        {
            const bool meeting = tile == 5;
            const float end = meeting ? .82f : .66f;
            // Upholstered chair backs behind the timber edge soften its silhouette.
            const auto chair = [&]( float centre ) {
                const float dx = ( x - centre ) / .053f, dy = ( v - .315f ) / .075f;
                finish( { 80, 95, 91 }, smooth( ( 1.f - dx * dx - dy * dy ) * 4.f ) );
                rect( centre - .032f, .236f, centre + .032f, .296f, { 76, 88, 84 } );
            };
            chair( meeting ? .38f : .55f );
            if( meeting ) chair( .68f );
            rect( .249f, .047f, .268f, .243f, { 76, 79, 76 } );
            rect( end - .053f, .047f, end - .034f, .243f, { 76, 79, 76 } );
            const float edge = v - .035f * ( x - .20f );
            finish( { 150, 131, 105 }, band( x, .21f, end ) * band( edge, .231f, .255f ) );
            if( !meeting )
            {
                rect( .366f, .252f, .389f, .307f, { 73, 78, 77 } );
                rect( .290f, .302f, .464f, .433f, { 62, 70, 73 } );
                rect( .302f, .317f, .451f, .421f, { 78, 92, 97 } );
            }
        }
        if( tile >= 6 )
        {
            const float hem = officeBlindHem( tile );
            const auto fabric = Vector3F( 169, 168, 152 ) * ( .91f + .09f * v );
            finish( fabric, .96f * smooth( ( v - hem ) * 56.f ) );
            finish( Vector3F( 113, 115, 107 ), band( v, hem, hem + .027f ) );
        }
        return colour;
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

                    const float recess = 1.f - .10f * jamb - .09f * sill - .12f * head;
                    const float paneRoughness = roughness[tile] + .062f * jamb + .057f * sill +
                                                .040f * head + .017f * smooth( ( .35f - v ) * 3.f );
                    const float paneCavity = 1.f - .19f * jamb - .22f * sill - .20f * head;

                    // A 2 m wide opening has a roughly 55 mm meeting stile and 60 mm casing.
                    // Soften their outlines over one source texel before mip filtering.
                    const float casing = std::min( { u, 1.f - u, v, 1.f - v } ) - .030f;
                    const float stile = std::abs( u - .5f ) - .014f;
                    const float transom = std::abs( v - .72f ) - .013f;
                    const float distance = std::min( { casing, stile, transom } );
                    const float frame = 1.f - smooth( ( distance + .009f ) / .018f );
                    const float seal = 1.f - smooth( ( distance - .010f ) / .018f );
                    const auto paint = Vector3F( 157.f, 156.f, 146.f ) * ( .90f + .10f * v );
                    const auto writePair = [&]( u32 bank, Vector3F pane ) {
                        const auto atlasX = ( bank + tile ) * atlasCell + x;
                        pane *= recess;
                        writePane( atlasX, y, pane, paneRoughness, paneCavity );
                        auto framed = pane * ( 1.f - .33f * seal );
                        framed += ( paint - framed ) * frame;
                        writePane( atlasX + paneVariants * atlasCell, y, framed,
                                   paneRoughness + ( .63f - paneRoughness ) * frame,
                                   paneCavity + ( .97f - paneCavity ) * frame );
                    };
                    writePair( 0, colour );
                    writePair( domesticStart, domesticPane( tile, u, v ) );
                    writePane( ( officeStart + tile ) * atlasCell + x, y, officePane( tile, u, v ) * recess,
                               paneRoughness, paneCavity );
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
