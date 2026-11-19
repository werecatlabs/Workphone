#pragma once

// Private terrain appearance helpers for newly generated terrain-city anchors.
namespace cityGround
{
    float ramp( float low, float high, float value )
    {
        const float t = std::clamp( ( value - low ) / ( high - low ), 0.f, 1.f );
        return t * t * ( 3.f - 2.f * t );
    }

    void seedLayers( render::TerrainData &data, const String &projectPath, u32 seed )
    {
        auto &meadow = data.layers[0];
        auto &damp = data.layers[1];
        auto &dry = data.layers[2];
        auto &lawn = data.layers[3];
        meadow.tint = { .36f, .40f, .235f, 1 };
        damp.tint = { .28f, .38f, .225f, 1 };
        dry.tint = { .48f, .40f, .285f, 1 };
        dry.roughness = .96f;
        lawn.tint = { .34f, .44f, .245f, 1 };
        lawn.roughness = .92f;
        data.appearance.macroScale = .016f;
        data.appearance.macroStrength = .13f;
        data.appearance.macroSeed = seed;
        data.appearance.macroTint = { .70f, .74f, .59f, 1 };

        SurfaceBakeResult scan;
        String diagnostic;
        if( !loadCitySurface( CitySurface::Ground, 512, scan, diagnostic ) )
        {
            return;
        }
        auto texturedMeadow = meadow;
        const String directory = "Assets/Landscape/CitySurfaces";
        auto meadowAlbedo = scan.albedo;
        const auto byte = []( float value ) { return u8( std::clamp( value, 0.f, 255.f ) ); };
        // Bring out the grass chroma already present in the photograph, leaving
        // redder soil and dried stems warm so the tile has real colour variation.
        for( size_t i = 0; i < meadowAlbedo.pixels.size(); i += 4 )
        {
            auto *pixel = meadowAlbedo.pixels.data() + i;
            const float vegetation = 1.f - ramp( 4.f, 25.f, float( pixel[0] ) - pixel[1] );
            pixel[0] = byte( pixel[0] - 18.f * vegetation );
            pixel[1] = byte( pixel[1] + 16.f * vegetation );
            pixel[2] = byte( pixel[2] + 2.f * vegetation );
        }
        if( !render::saveGeneratedTextureSource(
                meadowAlbedo, projectPath, directory, "grass-ground-meadow-albedo",
                render::TextureMipFilter::Colour, texturedMeadow.albedo, diagnostic ) ||
            !render::saveGeneratedTextureSource( scan.normal, projectPath, directory,
                                                 "grass-ground-normal", render::TextureMipFilter::Normal,
                                                 texturedMeadow.normal, diagnostic ) ||
            !render::saveGeneratedTextureSource( scan.orm, projectPath, directory, "grass-ground-orm",
                                                 render::TextureMipFilter::Data, texturedMeadow.orm,
                                                 diagnostic ) )
        {
            WP_LOG( String( "City terrain surface uses procedural fallback: " ) + diagnostic );
            return;
        }

        const float scale = 1.f / citySurfaceTileMetres( CitySurface::Ground );
        texturedMeadow.uvScale = { scale, scale };
        texturedMeadow.tint = { 1, 1, 1, 1 };
        texturedMeadow.normalStrength = .5f;
        meadow = std::move( texturedMeadow );
        damp = meadow;
        damp.uvScale = { scale * .72f, scale * .83f };
        damp.tint = { .84f, 1.f, .92f, 1 };
        damp.roughness = .92f;
        damp.normalStrength = .41f;

        // Keep straw and compact soil distinct at close range using local variation
        // from the same scan, with low-relief normal mapping and a rough response.
        TextureBuffer dryAlbedo( scan.albedo.width, scan.albedo.height );
        const auto luma = [&scan]( u32 x, u32 z ) {
            const auto pixel = scan.albedo.pixel( x, z );
            return .32f * pixel[0] + .53f * pixel[1] + .15f * pixel[2];
        };
        for( u32 z = 0; z < dryAlbedo.height; ++z )
        {
            for( u32 x = 0; x < dryAlbedo.width; ++x )
            {
                const auto input = scan.albedo.pixel( x, z );
                const float centre = luma( x, z );
                const float neighbourhood =
                    .25f * ( luma( ( x + dryAlbedo.width - 3 ) % dryAlbedo.width, z ) +
                             luma( ( x + 3 ) % dryAlbedo.width, z ) +
                             luma( x, ( z + dryAlbedo.height - 3 ) % dryAlbedo.height ) +
                             luma( x, ( z + 3 ) % dryAlbedo.height ) );
                const float grain = ( centre - 97.f ) * .94f + ( centre - neighbourhood ) * .49f +
                                    ( float( scan.orm.pixel( x, z )[0] ) - 205.f ) * .15f;
                const float straw = ramp( 8.f, 26.f, float( input[0] ) - input[1] );
                auto output = dryAlbedo.pixel( x, z );
                output[0] = byte( 119.f + grain + 9.f * straw );
                output[1] = byte( 101.f + grain * .87f + 6.f * straw );
                output[2] = byte( 76.f + grain * .77f + 3.f * straw );
                output[3] = 255;
            }
        }
        String dryTexture;
        if( render::saveGeneratedTextureSource(
                dryAlbedo, projectPath, directory, "grass-ground-dry-albedo",
                render::TextureMipFilter::Colour, dryTexture, diagnostic ) )
        {
            dry.albedo = std::move( dryTexture );
            dry.normal = meadow.normal;
            dry.uvScale = { scale * 1.18f, scale * .94f };
            dry.tint = { 1, 1, 1, 1 };
            dry.normalStrength = .31f;
        }
        else
        {
            WP_LOG( String( "City terrain dry ground uses procedural fallback: " ) + diagnostic );
        }

        // The shorter grass keeps the photograph's fine cuttings and pockets of straw,
        // while reducing the coarse shade variation of the loose meadow tile.
        TextureBuffer shortGrass( scan.albedo.width, scan.albedo.height );
        for( u32 z = 0; z < shortGrass.height; ++z )
        {
            for( u32 x = 0; x < shortGrass.width; ++x )
            {
                const auto pixel = scan.albedo.pixel( x, z );
                const float straw = ramp( 8.f, 26.f, float( pixel[0] ) - pixel[1] );
                const float grain = .73f * ( luma( x, z ) - 97.f ) +
                                    .13f * ( float( scan.orm.pixel( x, z )[0] ) - 205.f );
                auto output = shortGrass.pixel( x, z );
                output[0] = byte( 94.f + 9.f * straw + grain * .84f );
                output[1] = byte( 122.f - 12.f * straw + grain );
                output[2] = byte( 68.f + 7.f * straw + grain * .66f );
                output[3] = 255;
            }
        }
        String lawnTexture;
        if( render::saveGeneratedTextureSource(
                shortGrass, projectPath, directory, "grass-ground-mown-albedo",
                render::TextureMipFilter::Colour, lawnTexture, diagnostic ) )
        {
            lawn.albedo = std::move( lawnTexture );
            lawn.normal = meadow.normal;
            lawn.tint = { 1, 1, 1, 1 };
            lawn.uvScale = { scale * 1.58f, scale * 1.43f };
            lawn.normalStrength = .23f;
        }
        else
        {
            WP_LOG( String( "City terrain lawn uses procedural fallback: " ) + diagnostic );
        }
    }

    SourceBudgetVector<float> roadDistances( const render::TerrainData &data,
                                             const CityRoadSplineSource &source )
    {
        if( source.roads.empty() && source.roundabouts.empty() )
        {
            return {};
        }
        CityRoadSplineBake bake;
        CityRoadSplineSampling sampling;
        sampling.tolerance = .08f;
        sampling.maxStep = 5.f;
        String diagnostic;
        const auto plane = []( const Vector3F &, float &y ) {
            y = 2;
            return true;
        };
        if( !sampleCityRoadSplines( source, sampling, plane, bake, diagnostic ) ||
            bake.roads.size() != source.roads.size() )
        {
            return {};
        }

        constexpr float maximumReach = 64.f;
        SourceBudgetVector<float> distances( data.heights.size(), maximumReach );
        const s32 width = data.dimensions.x, depth = data.dimensions.y;
        const auto raster = [&]( Vector3F a, Vector3F b, float radius ) {
            const float reach = radius + maximumReach;
            const auto firstCell = []( float point, float origin, float step, s32 last ) {
                return s32( std::clamp( std::floor( ( point - origin ) / step ), 0.f, float( last ) ) );
            };
            const auto x0 =
                firstCell( std::min( a.x, b.x ) - reach, data.origin.x, data.spacing.x, width - 1 );
            const auto x1 =
                firstCell( std::max( a.x, b.x ) + reach, data.origin.x, data.spacing.x, width - 1 );
            const auto z0 =
                firstCell( std::min( a.z, b.z ) - reach, data.origin.y, data.spacing.y, depth - 1 );
            const auto z1 =
                firstCell( std::max( a.z, b.z ) + reach, data.origin.y, data.spacing.y, depth - 1 );
            const float vx = b.x - a.x, vz = b.z - a.z;
            const float lengthSquared = vx * vx + vz * vz;
            const float reciprocal = lengthSquared > 0 ? 1.f / lengthSquared : 0.f;
            for( s32 z = z0; z <= z1; ++z )
            {
                const float pz = data.origin.y + z * data.spacing.y - a.z;
                for( s32 x = x0; x <= x1; ++x )
                {
                    const float px = data.origin.x + x * data.spacing.x - a.x;
                    const float t = std::clamp( ( px * vx + pz * vz ) * reciprocal, 0.f, 1.f );
                    const float dx = px - vx * t, dz = pz - vz * t;
                    const float square = dx * dx + dz * dz;
                    auto &distance = distances[size_t( z ) * width + x];
                    const float threshold = radius + distance;
                    if( square < threshold * threshold )
                    {
                        distance = std::max( 0.f, std::sqrt( square ) - radius );
                    }
                }
            }
        };
        for( size_t i = 0; i < bake.roads.size(); ++i )
        {
            const auto &points = bake.roads[i].points;
            for( size_t j = 1; j < points.size(); ++j )
            {
                raster( points[j - 1].position, points[j].position, source.roads[i].width * .5f );
            }
        }
        for( const auto &roundabout : source.roundabouts )
        {
            raster( roundabout.center, roundabout.center, roundabout.outerRadius );
            for( const auto &port : roundabout.ports )
            {
                const auto start = roundabout.center + port.outward * roundabout.outerRadius;
                raster( start, start + port.outward * port.length, port.width * .5f );
            }
        }
        return distances;
    }

    void seedWeights( render::TerrainData &data, float cityExtent, u32 seed,
                      const CityRoadSplineSource &source )
    {
        const auto width = u32( data.dimensions.x ), depth = u32( data.dimensions.y );
        const auto distances = roadDistances( data, source );
        data.layerWeights.resize( data.heights.size() );
        for( u32 z = 0; z < depth; ++z )
        {
            for( u32 x = 0; x < width; ++x )
            {
                const float px = data.origin.x + x * data.spacing.x;
                const float pz = data.origin.y + z * data.spacing.y;
                const u32 left = x == 0 ? x : x - 1, right = std::min( x + 1, width - 1 );
                const u32 back = z == 0 ? z : z - 1, front = std::min( z + 1, depth - 1 );
                const float dx = ( data.heights[size_t( z ) * width + right] -
                                   data.heights[size_t( z ) * width + left] ) /
                                 ( ( right - left ) * data.spacing.x );
                const float dz = ( data.heights[size_t( front ) * width + x] -
                                   data.heights[size_t( back ) * width + x] ) /
                                 ( ( front - back ) * data.spacing.y );
                const float incline = ramp( .032f, .115f, std::hypot( dx, dz ) );
                const float aspect = std::clamp( ( dx * .6f - dz * .8f ) / .075f, -1.f, 1.f );
                const float outskirts = ramp( cityExtent + 4.f, cityExtent + 84.f,
                                              std::max( std::abs( px ), std::abs( pz ) ) );
                const float warpX =
                    px + 21.f * ( cityValueNoise( seed + 1, px / 62.f, pz / 59.f ) - .5f );
                const float warpZ =
                    pz + 24.f * ( cityValueNoise( seed + 2, px / 55.f, pz / 68.f ) - .5f );
                const float dryField = .62f * cityValueNoise( seed + 3, warpX / 34.f, warpZ / 27.f ) +
                                       .27f * cityValueNoise( seed + 4, warpX / 13.f, warpZ / 17.f ) +
                                       .11f * cityValueNoise( seed + 5, px / 7.f, pz / 8.f );
                const float dampField = .74f * cityValueNoise( seed + 6, warpX / 44.f, warpZ / 35.f ) +
                                        .26f * cityValueNoise( seed + 7, warpX / 14.f, warpZ / 18.f );
                float dryWeight =
                    .81f *
                    ramp( .46f, .76f, dryField + .085f * incline + .043f * aspect + .10f * outskirts );
                const float moisture =
                    .72f * dampField + .28f * ( 1.f - dryField ) - .06f * incline - .042f * aspect;
                const auto index = size_t( z ) * width + x;
                const float roadDistance = distances.empty() ? 0.f : distances[index];
                // Hold the original paint through the pavement, verge and its bilinear
                // filter footprint. Changes to the other banks stay soft and irregular.
                const float meadowMargin = ramp( 10.3f, 17.f, roadDistance );
                float lawnWeight = 0;
                if( meadowMargin > 0 )
                {
                    const float district = 1.f - ramp( .61f * cityExtent, .96f * cityExtent,
                                                       std::max( std::abs( px ), std::abs( pz ) ) );
                    const float nearSettlement = 1.f - ramp( 34.f, 59.f, roadDistance );
                    const float mowingPattern =
                        .66f * cityValueNoise( seed + 23, warpX / 38.f, warpZ / 49.f ) +
                        .34f * cityValueNoise( seed + 29, warpX / 16.f, warpZ / 21.f );
                    lawnWeight = .87f * meadowMargin * nearSettlement * ( .48f + .52f * district ) *
                                 ramp( .34f, .69f, mowingPattern + .13f * district - .058f * incline );
                    const float breaks =
                        .68f * cityValueNoise( seed + 37, warpX / 12.5f, warpZ / 16.f ) +
                        .32f * cityValueNoise( seed + 41, px / 7.f, pz / 8.5f );
                    const float wear = ramp( .66f, .88f, breaks + .07f * incline );
                    const float meadowRetention = .18f + .28f * ramp( .29f, .74f, dampField );
                    dryWeight *= 1.f - meadowMargin * meadowRetention;
                    dryWeight += ( 1.f - dryWeight ) * meadowMargin * wear * .56f;
                }
                const float dampWeight = .94f * ( 1.f - dryWeight ) * ramp( .365f, .65f, moisture );
                const float wild = 1.f - lawnWeight;
                data.layerWeights[index] = { wild * ( 1.f - dryWeight - dampWeight ), wild * dampWeight,
                                             wild * dryWeight, lawnWeight };
            }
        }
    }
}  // namespace cityGround
