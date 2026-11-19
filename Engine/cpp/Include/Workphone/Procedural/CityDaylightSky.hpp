#pragma once

const Vector3F citySkyHorizon( .70f, .81f, .94f );

namespace cityDaylight
{
    float smooth( float from, float to, float value )
    {
        const float t = std::clamp( ( value - from ) / ( to - from ), 0.f, 1.f );
        return t * t * ( 3.f - 2.f * t );
    }

    Vector3F cloudFrame( Vector3F v )
    {
        return { .88f * v.x + .474974f * v.z, v.y, -.474974f * v.x + .88f * v.z };
    }

    struct CloudRay
    {
        float optical = 0.f, light = 0.f, underside = 0.f;
    };

    void billow( CloudRay &cloud, Vector3F ray, Vector3F sun, Vector3F centre, Vector3F radius,
                 float density )
    {
        const Vector3F d( ray.x / radius.x, ray.y / radius.y, ray.z / radius.z );
        const Vector3F c( centre.x / radius.x, centre.y / radius.y, centre.z / radius.z );
        const float inverse = 1.f / d.lengthSquared();
        const float station = d.dotProduct( c ) * inverse;
        const auto nearest = d * station - c;
        const float chord2 = 1.f - nearest.lengthSquared();
        if( chord2 <= 0.f || station <= 0.f )
        {
            return;
        }

        const float halfChord = std::sqrt( chord2 * inverse );
        const auto p = nearest - d * halfChord;
        const auto normal = Vector3F( p.x / radius.x, p.y / radius.y, p.z / radius.z ).normaliseCopy();
        const float weight = halfChord * density;
        cloud.optical += weight;
        cloud.light += weight * smooth( -.35f, .74f, normal.dotProduct( sun ) );
        cloud.underside += weight * smooth( -.76f, .44f, normal.y );
    }

    CloudRay cumulus( Vector3F ray, Vector3F sun, u32 seed )
    {
        CloudRay cloud;
        if( ray.y < .141f )
        {
            return cloud;
        }
        constexpr float driftX = 18.73f, driftZ = -11.41f;
        // Only cells whose footprints can intersect this ray in the cloud
        // layer are visited. Elevation fade keeps that region bounded.
        const float nearT = 1.15f / ray.y, farT = 2.30f / ray.y;
        const float startX = ray.x * nearT, endX = ray.x * farT;
        const float startZ = ray.z * nearT, endZ = ray.z * farT;
        constexpr float margin = 1.22f;
        const s32 x0 = s32( std::floor( driftX + std::min( startX, endX ) - margin ) );
        const s32 x1 = s32( std::floor( driftX + std::max( startX, endX ) + margin ) );
        const s32 z0 = s32( std::floor( driftZ + std::min( startZ, endZ ) - margin ) );
        const s32 z1 = s32( std::floor( driftZ + std::max( startZ, endZ ) + margin ) );
        const auto hashSeed = u64( seed ) * 0x9e3779b97f4a7c15ull;
        const float horizontal = std::sqrt( ray.x * ray.x + ray.z * ray.z );
        for( s32 z = z0; z <= z1; ++z )
        {
            for( s32 x = x0; x <= x1; ++x )
            {
                const float gridX = x + .5f - driftX, gridZ = z + .5f - driftZ;
                if( std::abs( gridX * ray.z - gridZ * ray.x ) > 1.62f * horizontal )
                {
                    continue;
                }
                const u64 cell = ( u64( u32( x ) ) << 32 ) | u32( z );
                const u64 bits = render::foliageStableHash( cell ^ hashSeed );
                if( ( bits >> 60 ) < 5 )
                {
                    continue;
                }
                const float cluster = cityValueNoise( seed + 71u, x * .19f, z * .23f );
                if( float( bits >> 60 ) < 13.5f - 9.3f * cluster )
                {
                    continue;
                }
                const auto fraction = [bits]( u32 offset ) {
                    return float( ( bits >> offset ) & 255u ) / 255.f;
                };
                const float shift = cityValueNoise( seed + 97u, x * .23f + 7.4f, z * .21f - 4.1f ) - .5f;
                // Neighboring occupied cells drift together, leaving broader
                // gaps between clusters instead of equally spaced rows.
                const Vector3F c( gridX + ( fraction( 0 ) - .5f ) * .82f + .34f * shift,
                                  1.48f + .25f * fraction( 8 ),
                                  gridZ + ( fraction( 16 ) - .5f ) * .82f + .33f * ( cluster - .5f ) );
                const float size = .57f + .59f * fraction( 24 );
                const float trim = ( bits & ( 1ull << 37u ) ) ? -1.f : 1.f;
                const float depth = .72f + .46f * fraction( 32 );
                billow( cloud, ray, sun, c, Vector3F( .488f, .212f, .389f ) * size, depth * 1.94f );
                billow( cloud, ray, sun,
                        c + Vector3F( -trim * ( .135f + .13f * fraction( 18 ) ),
                                      .093f + .079f * fraction( 4 ), .028f + .143f * fraction( 12 ) ),
                        Vector3F( .265f, .241f, .262f ) * size, depth * 1.59f );
                billow( cloud, ray, sun,
                        c + Vector3F( trim * ( .197f + .087f * fraction( 15 ) ),
                                      .042f + .063f * fraction( 23 ), -.145f + .125f * fraction( 44 ) ),
                        Vector3F( .268f, .166f, .218f ) * size, depth * 1.36f );
                if( fraction( 39 ) > .52f )
                {
                    billow( cloud, ray, sun, c + Vector3F( -.025f * trim, .217f, -.168f ),
                            Vector3F( .208f, .179f, .228f ) * size, depth * 1.24f );
                }
            }
        }
        return cloud;
    }

    struct Sample
    {
        ColourF radiance;
        float sunTransmission;
    };

    Sample sample( Vector3F direction, Vector3F sun, float alignment, u32 seed )
    {
        const float altitude = std::max( 0.f, direction.y );
        const float haze = std::exp( -altitude * 6.6f );
        const float high = smooth( .12f, .97f, altitude );
        const auto blue =
            Vector3F( .244f, .574f, 1.12f ) * ( 1.f - high ) + Vector3F( .118f, .35f, .83f ) * high;
        auto colour = citySkyHorizon * haze + blue * ( 1.f - haze );
        colour +=
            Vector3F( .16f, .115f, .05f ) * ( std::pow( alignment, 7.f ) * ( .55f + .45f * haze ) );
        float transmission = 1.f;
        if( altitude > .026f )
        {
            const auto ray = cloudFrame( direction );
            const auto light = cloudFrame( sun );
            // A faint advected ice layer behind the cumulus deck.
            const float deck = 1.f / ( altitude + .27f );
            const float x = ( .84f * ray.x + .5426f * ray.z ) * 2.15f * deck + 7.8f;
            const float z = ( -.5426f * ray.x + .84f * ray.z ) * 7.2f * deck + 13.5f;
            const float flow = cityValueNoise( seed + 41, x * .62f, z * .30f ) - .5f;
            const float fibre = .67f * cityValueNoise( seed + 61, x, z + flow * .74f ) +
                                .33f * cityValueNoise( seed + 73, x * 2.3f + 2.5f, z * 2.7f );
            const float veil = .18f * smooth( .596f, .84f, fibre ) * smooth( .026f, .17f, altitude );
            colour += ( Vector3F( 1.22f, 1.34f, 1.50f ) - colour ) * veil;

            const float fade = smooth( .148f, .289f, altitude );
            if( fade > 0.f )
            {
                const float scale = 1.6f / altitude;
                const float u = ray.x * scale + 18.73f, v = ray.z * scale - 11.41f;
                const auto advection = [&]( u32 salt, float x, float z ) {
                    const float roll = cityValueNoise( seed + salt, 4.2f * x, 4.2f * z ) - .5f;
                    const float eddy = cityValueNoise( seed + salt + 163u, 11.3f * x, 12.0f * z ) - .5f;
                    return ( .235f * roll + .048f * eddy ) / scale;
                };
                const auto cloudRay =
                    ( ray + Vector3F( advection( 193u, u, v ), 0.f, advection( 467u, v + 3.4f, u ) ) )
                        .normaliseCopy();
                const auto cloud = cumulus( cloudRay, light, seed + 167u );
                if( cloud.optical > .001f )
                {
                    // Erode the soft limbs without making a separate volume search.
                    const float wisps = cityValueNoise( seed + 217, u * 9.6f, v * 9.1f );
                    const float steam = cityValueNoise( seed + 231, u * 4.1f + 2.4f, v * 5.3f - 1.7f );
                    const float depth =
                        std::max( 0.f, cloud.optical * ( .86f + .28f * steam ) - .124f * wisps );
                    const float opacity = ( 1.f - std::exp( -2.17f * depth ) ) * fade;
                    const float facingSun = cloud.light / cloud.optical;
                    const float loft = cloud.underside / cloud.optical;
                    const float rim = std::pow( alignment, 27.f ) * std::exp( -2.3f * depth );
                    const float illumination =
                        std::clamp( .09f + .65f * facingSun + .23f * loft + .27f * rim, 0.f, 1.f );
                    const Vector3F underside( 1.20f, 1.34f, 1.51f ), diffuseSun( 3.38f, 3.19f, 2.89f );
                    const auto body = ( underside + ( diffuseSun - underside ) * illumination ) *
                                          ( .88f + .25f * steam ) +
                                      Vector3F( .56f, .46f, .34f ) * rim;
                    const auto distant =
                        citySkyHorizon + ( body - citySkyHorizon ) * smooth( .075f, .265f, altitude );
                    colour += ( distant - colour ) * opacity;
                    transmission *= 1.f - .81f * opacity;
                }
            }
            transmission *= 1.f - .14f * veil;
        }

        const float aureole = std::pow( alignment, 105.f ) * transmission;
        colour += Vector3F( .56f, .46f, .31f ) * aureole;
        return { { colour.x, colour.y, colour.z, 1.f }, transmission };
    }

    u8 encodeDisplay( float linear )
    {
        float display = std::max( 0.f, linear );
        if( display > .94f )
        {
            display = .94f + .06f * ( 1.f - std::exp( -( display - .94f ) / .095f ) );
        }
        const float srgb =
            display <= .0031308f ? display * 12.92f : 1.055f * std::pow( display, 1.f / 2.4f ) - .055f;
        return u8( std::clamp( std::round( srgb * 255.f ), 0.f, 255.f ) );
    }
}  // namespace cityDaylight
