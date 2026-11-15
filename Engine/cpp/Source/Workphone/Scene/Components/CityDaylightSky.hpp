#pragma once

        const Vector3F citySkyHorizon( .70f, .81f, .94f );

        namespace cityDaylight
        {
            float smooth( float from, float to, float value )
            {
                const float t = std::clamp( ( value - from ) / ( to - from ), 0.f, 1.f );
                return t * t * ( 3.f - 2.f * t );
            }

            // Neighboring ellipsoids merge into larger cloud bodies, with whole cells
            // left empty. The same planar field is sampled across all six cube faces.
            float cloudBodies( u32 seed, float x, float z )
            {
                const auto a = s32( std::floor( x ) ), b = s32( std::floor( z ) );
                const float u = x - a, v = z - b;
                float density = 0.f;
                for( s32 row = -1; row <= 1; ++row )
                {
                    for( s32 column = -1; column <= 1; ++column )
                    {
                        const u64 cell = ( u64( u32( a + column ) ) << 32 ) | u32( b + row );
                        const u64 bits = render::foliageStableHash( cell ^ ( u64( seed ) * 0x9e3779b97f4a7c15ull ) );
                        if( ( bits >> 60 ) < 4 ) continue;
                        const float dx = u - column - .15f - float( bits & 255u ) * ( .70f / 255 );
                        const float dz = v - row - .15f - float( ( bits >> 8 ) & 255u ) * ( .70f / 255 );
                        const float width = .46f + float( ( bits >> 16 ) & 255u ) * ( .33f / 255 );
                        const float length = .43f + float( ( bits >> 24 ) & 255u ) * ( .29f / 255 );
                        density = std::max( density, 1.f - dx * dx / ( width * width ) - dz * dz / ( length * length ) );
                    }
                }
                return density;
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
                const auto blue = Vector3F( .21f, .43f, .80f ) * ( 1 - high ) + Vector3F( .095f, .29f, .63f ) * high;
                auto colour = citySkyHorizon * haze + blue * ( 1 - haze );
                colour += Vector3F( .16f, .115f, .05f ) * ( std::pow( alignment, 7.f ) * ( .55f + .45f * haze ) );

                float transmission = 1.f;
                if( altitude > .012f )
                {
                    const float east = .88f * direction.x + .475f * direction.z;
                    const float north = -.475f * direction.x + .88f * direction.z;
                    const float highDeck = 1.f / ( altitude + .29f );
                    const float a = ( .88f * east + .475f * north ) * 1.57f * highDeck + 8.23f;
                    const float b = ( -.475f * east + .88f * north ) * 4.2f * highDeck + 15.7f;
                    const float bend = cityValueNoise( seed + 41, a * .64f, b * .27f ) - .5f;
                    const float ice = .71f * cityValueNoise( seed + 61, a, b + bend * 1.35f ) +
                                      .29f * cityValueNoise( seed + 73, a * 2.7f + .41f, b * 3.4f + 1.1f );
                    const float veil = .33f * smooth( .49f, .78f, ice ) * smooth( .012f, .10f, altitude );
                    const Vector3F cirrus( 1.01f, 1.08f, 1.16f );
                    colour += ( cirrus - colour ) * veil;

                    const float lowDeck = 1.f / ( altitude + .155f );
                    const float u = east * 1.77f * lowDeck + 13.40f;
                    const float v = north * 1.47f * lowDeck - 7.73f;
                    const float x = u + .53f * ( cityValueNoise( seed + 11, u * 1.71f + 3.9f, v * 1.68f ) - .5f );
                    const float z = v + .44f * ( cityValueNoise( seed + 17, u * 1.57f, v * 1.65f + 3.4f ) - .5f );
                    const float bulk = cloudBodies( seed + 101, x, z );
                    if( bulk > .20f )
                    {
                        // Small billows and windward erosion split the bulk field into
                        // curved, overlapping masses. Far detail subsides into the haze.
                        const float along = .84f * x + .543f * z, across = -.543f * x + .84f * z;
                        const float billows = cloudBodies( seed + 113, along * 2.46f + 6.8f, across * 3.21f + 7.1f );
                        const float strands = cityValueNoise( seed + 31, along * 2.85f, across * 6.9f );
                        const float grit = cityValueNoise( seed + 43, along * 12.6f - 3.7f, across * 16.3f );
                        const float detail = .73f * strands + .27f * grit;
                        const float erosion = ( .64f * billows + .36f * detail - .48f ) *
                                              ( .26f + .22f * smooth( .05f, .30f, altitude ) );
                        const float profile = .94f * bulk + erosion;
                        const float body = smooth( .30f, .65f, profile );
                        const float edge = smooth( .30f, .53f, profile ) - smooth( .50f, .72f, profile );
                        const float opacity = body * ( .70f + .28f * smooth( .44f, .99f, profile ) ) *
                                              smooth( .026f, .13f, altitude );
                        if( opacity > .001f )
                        {
                            const float sunX = ( .88f * sun.x + .475f * sun.z ) * 1.77f;
                            const float sunZ = ( -.475f * sun.x + .88f * sun.z ) * 1.47f;
                            const float ahead = cloudBodies( seed + 101, x + sunX * .24f, z + sunZ * .24f );
                            const float shade = smooth( .21f, .94f, .58f * ahead + .24f * bulk + .18f * billows );
                            const float light = std::clamp( .76f - .63f * shade + .26f * smooth( -.06f, .43f, bulk - ahead ) +
                                .28f * ( strands - .44f ) + .18f * edge * alignment, 0.f, 1.f );
                            const Vector3F underside( .43f, .52f, .67f ), sunlight( 1.49f, 1.43f, 1.34f );
                            const auto clouds = ( underside + ( sunlight - underside ) * light ) *
                                                ( .83f + .32f * detail + .14f * billows );
                            const auto distant = citySkyHorizon + ( clouds - citySkyHorizon ) * smooth( .038f, .19f, altitude );
                            colour += ( distant - colour ) * opacity;
                            transmission *= 1.f - .68f * opacity;
                        }
                    }
                    transmission *= 1.f - .19f * veil;
                }

                const float aureole = std::pow( alignment, 105.f ) * transmission;
                colour += Vector3F( .56f, .46f, .31f ) * aureole;
                return { { colour.x, colour.y, colour.z, 1.f }, transmission };
            }

            u8 encodeDisplay( float linear )
            {
                // The visible sampler uses sRGB byte textures. Leave the linear probe
                // unchanged and roll the sky highlights gently into the available byte range.
                float display = std::max( 0.f, linear );
                if( display > .94f ) display = .94f + .06f * ( 1.f - std::exp( -( display - .94f ) / .095f ) );
                const float srgb = display <= .0031308f ? display * 12.92f
                    : 1.055f * std::pow( display, 1.f / 2.4f ) - .055f;
                return u8( std::clamp( std::round( srgb * 255.f ), 0.f, 255.f ) );
            }
        }
