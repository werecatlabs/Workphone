#pragma once

        namespace cityFacadeDetail
        {
            struct Materials
            {
                SmartPtr<render::IMaterial> stone, dressed, metal;
            };

            namespace mineral
            {
                float noise( float u, float v, u32 cells, u64 key )
                {
                    const float x = u * cells, y = v * cells;
                    const u32 ix = u32( x ), iy = u32( y );
                    const auto fade = []( float t ) { return t * t * ( 3.f - 2.f * t ); };
                    const float sx = fade( x - ix ), sy = fade( y - iy );
                    const auto grain = [&]( u32 a, u32 b ) {
                        return float( render::foliageStableHash( key ^ ( ( u64( b % cells ) << 32 ) | ( a % cells ) ) ) >> 40 ) /
                               16777216.f;
                    };
                    const float a = grain( ix, iy ), b = grain( ix + 1, iy );
                    const float c = grain( ix, iy + 1 ), d = grain( ix + 1, iy + 1 );
                    const float upper = a + ( b - a ) * sx, lower = c + ( d - c ) * sx;
                    return upper + ( lower - upper ) * sy;
                }

                u8 channel( float value )
                {
                    return u8( std::clamp( std::round( value ), 0.f, 255.f ) );
                }
            }  // namespace mineral

            Materials materials( SceneAssets &assets, SmartPtr<render::IMaterial> baseMasonry, u32 seed )
            {
                constexpr u32 size = 128;
                TextureBuffer colour( size, size ), normal( size, size ), roughness( size, size );
                SourceBudgetVector<float> height( size_t( size ) * size );
                const u64 key = ( u64( seed ) << 32 ) ^ 0x6d696e6572616cull;
                for( u32 y = 0; y < size; ++y )
                    for( u32 x = 0; x < size; ++x )
                    {
                        const float u = ( x + .5f ) / size, v = ( y + .5f ) / size;
                        const float mottling = mineral::noise( u, v, 5, key + 3 );
                        const float sediment = mineral::noise( u, v, 17, key + 719 );
                        const float sand = mineral::noise( u, v, 67, key + 3121 );
                        const float pit = std::clamp( ( sand - .71f ) * 3.8f, 0.f, 1.f );
                        const float luminance = 178.f + 10.f * ( mottling - .5f ) + 9.f * ( sediment - .5f ) +
                                                13.f * ( sand - .5f ) - 13.f * pit;
                        const float warm = 2.f + 3.5f * sediment;
                        auto pixel = colour.pixel( x, y );
                        pixel[0] = mineral::channel( luminance + warm );
                        pixel[1] = mineral::channel( luminance );
                        pixel[2] = mineral::channel( luminance - 2.f * warm );
                        pixel[3] = 255;
                        pixel = roughness.pixel( x, y );
                        pixel[0] = pixel[1] = pixel[2] = mineral::channel( 255.f * ( .81f + .105f * sediment + .048f * pit ) );
                        pixel[3] = 255;
                        height[size_t( y ) * size + x] = .0019f * sediment + .0015f * sand - .0014f * pit;
                    }
                const auto sample = [&]( u32 x, u32 y ) { return height[size_t( y ) * size + x]; };
                for( u32 y = 0; y < size; ++y )
                    for( u32 x = 0; x < size; ++x )
                    {
                        // Millimetre surface relief. Both derivatives wrap the tile.
                        const float dx = sample( ( x + 1 ) % size, y ) - sample( ( x + size - 1 ) % size, y );
                        const float dy = sample( x, ( y + 1 ) % size ) - sample( x, ( y + size - 1 ) % size );
                        const float gradient = size / 2.4f;
                        const auto n = Vector3F( -dx * gradient, -dy * gradient, 1.f ).normaliseCopy();
                        const auto pixel = normal.pixel( x, y );
                        pixel[0] = mineral::channel( 127.5f * ( 1.f + n.x ) );
                        pixel[1] = mineral::channel( 127.5f * ( 1.f + n.y ) );
                        pixel[2] = mineral::channel( 127.5f * ( 1.f + n.z ) );
                        pixel[3] = 255;
                    }
                const auto albedoMap = upload( assets, colour );
                const auto normalMap = upload( assets, normal, render::TextureMipFilter::Normal );
                const auto roughnessMap = upload( assets, roughness, render::TextureMipFilter::Roughness );

                const float baseResponse = baseMasonry ? std::clamp( baseMasonry->getRoughness(), .75f, 1.f ) : 1.f;
                const auto stone = [&]( ColourF tint, float textureScale, float relief, float finish ) {
                    auto surface = material( assets, tint, finish * ( .94f + .06f * baseResponse ) );
                    surface->setTexture( albedoMap, static_cast<u32>( PbsTextureTypes::PBSM_DIFFUSE ) );
                    surface->setTexture( normalMap, static_cast<u32>( PbsTextureTypes::PBSM_NORMAL ) );
                    surface->setTexture( roughnessMap, static_cast<u32>( PbsTextureTypes::PBSM_ROUGHNESS ) );
                    surface->setUVProjection( 1 );
                    surface->setTriplanarScale( textureScale );
                    surface->setNormalStrength( relief );
                    surface->setEditorUInt( "uvFilter", 3 );
                    surface->setEditorFloat( "uvAniso", 8.f );
                    return surface;
                };
                auto metal = material( assets, { .32f, .305f, .28f, 1 }, .54f, .33f );
                metal->setSpecular( { .055f, .051f, .046f, 1 } );
                return { stone( { .92f, .91f, .90f, 1 }, .83f, .53f, 1.f ),
                         stone( { 1.f, .988f, .963f, 1 }, 1.12f, .31f, .96f ), metal };
            }
        }  // namespace cityFacadeDetail
