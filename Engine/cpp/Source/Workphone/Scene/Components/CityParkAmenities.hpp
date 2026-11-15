#pragma once

        namespace cityParkAmenities
        {
            SmartPtr<render::IMaterial> seatTimber( SceneAssets &assets, u32 seed )
            {
                constexpr u32 width = 128, height = 32;
                TextureBuffer texture( width, height );
                const auto noise = [seed]( u32 x, u32 y ) {
                    return float( render::foliageStableHash( ( u64( seed ) << 32 ) ^
                                                            ( u64( y ) << 16 ) ^ x ) >> 40 ) / 16777216.f;
                };
                const auto byte = []( float value ) { return u8( std::clamp( value, 0.f, 255.f ) ); };
                for( u32 y = 0; y < height; ++y )
                    for( u32 x = 0; x < width; ++x )
                    {
                        const float u = float( x ) / width;
                        const float warp = std::sin( 2.f * pi * u ) * .85f +
                                           std::sin( 6.f * pi * u + .2f * y ) * .34f;
                        const float coordinate = y + height + warp;
                        const u32 fibre = u32( coordinate );
                        const float blend = coordinate - fibre;
                        const float grain = ( noise( 0, fibre % height ) * ( 1.f - blend ) +
                                              noise( 0, ( fibre + 1 ) % height ) * blend - .5f ) * 25.f;
                        const float weather = ( noise( x, y ) - .5f ) * 5.f;
                        auto *pixel = texture.pixel( x, y );
                        pixel[0] = byte( 156.f + grain + weather );
                        pixel[1] = byte( 115.f + grain * .83f + weather );
                        pixel[2] = byte( 81.f + grain * .65f + weather );
                        pixel[3] = 255;
                    }
                auto timber = material( assets, ColourF::White, .73f );
                timber->setTexture( upload( assets, texture ), 0 );
                timber->setUVProjection( 0 );
                timber->setEditorUInt( "uvFilter", 3 );
                timber->setEditorFloat( "uvAniso", 8.f );
                return timber;
            }

            void plank( Geometry &timber, Vector3F centre, Vector3F across, Vector3F face,
                        Vector3F normal, float length, float width, float thickness )
            {
                const auto a = across * ( length * .5f ), b = face * ( width * .5f );
                const auto offset = normal * ( thickness * .5f );
                const auto top = centre + offset, bottom = centre - offset;
                timber.quad( top - a - b, top + a - b, top + a + b, top - a + b, normal );
                timber.quad( bottom - a - b, bottom + a - b, bottom + a + b, bottom - a + b, -normal );
                timber.quad( bottom - a - b, bottom + a - b, top + a - b, top - a - b, -face );
                timber.quad( top - a + b, top + a + b, bottom + a + b, bottom - a + b, face );
                timber.quad( bottom - a + b, bottom - a - b, top - a - b, top - a + b, -across );
                timber.quad( bottom + a - b, bottom + a + b, top + a + b, top + a - b, across );
            }

            void build( SceneAssets &assets, const SourceBudgetVector<cityParkLandscape::Layout> &layouts, u32 seed )
            {
                const auto detail = std::clamp( assets.cityStreetDetail, 0, 100 );
                if( !detail || layouts.empty() ) return;
                Geometry timber, metal;
                const auto limit = ( layouts.size() * size_t( detail ) + 99 ) / 100;
                size_t seats = 0;
                const Vector3F up( 0, 1, 0 );
                for( const auto &park : layouts )
                {
                    if( seats >= limit ) break;
                    if( !park.hasSeat ) continue;
                    const auto front = park.seatForward;
                    const Vector3F across( -front.z, 0, front.x );
                    const auto point = [&]( float x, float y, float z ) {
                        return park.seat + across * x + up * y + front * z;
                    };
                    for( int slat = -2; slat <= 2; ++slat )
                        plank( timber, point( 0, .445f, slat * .082f ), across, front, up, 1.82f, .072f, .035f );
                    const auto backRise = ( up - front * .18f ).normaliseCopy();
                    const auto backNormal = ( front + up * .18f ).normaliseCopy();
                    for( int slat = 0; slat < 3; ++slat )
                        plank( timber, point( 0, .637f + slat * .108f, -.210f - slat * .020f ),
                               across, backRise, backNormal, 1.82f, .086f, .034f );
                    for( size_t foot = 0; foot < 2; ++foot )
                    {
                        const float side = foot ? .69f : -.69f;
                        metal.beam( point( side, park.seatFootOffsets[foot * 2] - .018f, .158f ),
                                    point( side, .423f, .126f ), .032f );
                        metal.beam( point( side, park.seatFootOffsets[foot * 2 + 1] - .018f, -.170f ),
                                    point( side, .894f, -.285f ), .030f );
                        metal.beam( point( side, .414f, .172f ), point( side, .414f, -.228f ), .030f );
                        metal.beam( point( side, .638f, -.249f ), point( side, .638f, .086f ), .025f );
                        metal.beam( point( side, .638f, .086f ), point( side, .418f, .156f ), .025f );
                    }
                    ++seats;
                }
                if( !seats ) return;
                mesh( assets, "Terrain city park timber benches", timber, seatTimber( assets, seed ) );
                mesh( assets, "Terrain city park cast bench frames", metal,
                      material( assets, { .162f, .178f, .164f, 1 }, .56f, .17f ) );
                WP_LOG( String( "Terrain city park benches=" ) + StringUtil::toString( seats ) );
            }
        }  // namespace cityParkAmenities
