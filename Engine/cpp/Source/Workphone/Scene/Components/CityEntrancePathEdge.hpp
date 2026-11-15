#pragma once

        namespace cityEntrancePathEdge
        {
            struct Batches
            {
                Geometry shoulders;
                size_t sections = 0;
            };

            template <class Surface, class Clear>
            size_t append( Batches &batches, const cityEntranceRoute::Route &route, u64 key,
                           const Surface &surface, const Clear &clear )
            {
                if( route.rowCount < 2 || route.length < 1.7f ) return 0;
                struct Row
                {
                    Vector3F side, across;
                    float station;
                };
                const float phase = 6.2831853f * cityVerge::unit( key );
                const auto profile = [&]( const Row &row ) {
                    const float ripple = std::sin( 2.2f * row.station + phase ) +
                                         .48f * std::sin( 5.2f * row.station - phase );
                    const float taper = std::clamp( ( std::min( row.station, route.length - row.station ) - .20f ) / .64f, 0.f, 1.f );
                    return ( .235f + .054f * ripple ) * taper;
                };
                size_t sections = 0;
                for( const bool right : { false, true } )
                {
                    float distance = 0.f;
                    const auto rowAt = [&]( const auto &row, float station ) {
                        const auto across = cityEntranceRoute::horizontal( row.right - row.left ).normaliseCopy();
                        return Row{ right ? row.right : row.left, right ? across : -across, station };
                    };
                    Row previous = rowAt( route.rows[0], 0.f );
                    for( size_t i = 1; i < route.rowCount; ++i )
                    {
                        const auto length = cityEntranceRoute::horizontal( route.rows[i].centre - route.rows[i - 1].centre ).length();
                        distance += length;
                        const auto next = rowAt( route.rows[i], distance );
                        const auto divisions = std::max( 1, int( std::ceil( length / .27f ) ) );
                        const auto interpolate = [&]( float t ) {
                            return Row{ previous.side + ( next.side - previous.side ) * t,
                                        ( previous.across + ( next.across - previous.across ) * t ).normaliseCopy(),
                                        previous.station + length * t };
                        };
                        for( int part = 0; part < divisions; ++part )
                        {
                            const auto a = interpolate( float( part ) / divisions ), b = interpolate( float( part + 1 ) / divisions );
                            const auto wa = profile( a ), wb = profile( b );
                            if( std::min( wa, wb ) < .020f ) continue;
                            // The strip begins beside the actual paver edge, with a small
                            // separation for tight mitres and terrain float roundoff.
                            constexpr float reveal = .017f;
                            std::array<Vector3F, 4> corners{ a.side + a.across * reveal,
                                a.side + a.across * ( reveal + wa ), b.side + b.across * ( reveal + wb ),
                                b.side + b.across * reveal };
                            if( !clear( std::array{ corners[0], corners[1], corners[2] } ) ||
                                !clear( std::array{ corners[0], corners[2], corners[3] } ) ||
                                !std::all_of( corners.begin(), corners.end(), [&]( auto &p ) { return surface( p ); } ) ) continue;
                            auto centre = ( corners[0] + corners[2] ) * .5f;
                            const float expected = centre.y;
                            if( !surface( centre ) || std::abs( centre.y - expected ) > .013f ) continue;
                            auto normal = ( corners[1] - corners[0] ).crossProduct( corners[2] - corners[0] ).normaliseCopy();
                            if( normal.y < 0 ) normal = -normal;
                            if( normal.y < .94f ) continue;
                            const auto v = [&]( float value ) { return value * .57f + float( key & 15u ) / 16.f; };
                            batches.shoulders.quad( corners[0], corners[1], corners[2], corners[3], normal,
                                                    .01f, .99f, v( a.station ), v( b.station ) );
                            ++sections;
                        }
                        previous = next;
                    }
                }
                batches.sections += sections;
                return sections;
            }

            void publish( SceneAssets &assets, const Batches &batches, u32 seed )
            {
                if( batches.shoulders.indices.empty() ) return;
                TextureBuffer grit( 128, 128 );
                for( u32 y = 0; y < grit.height; ++y )
                    for( u32 x = 0; x < grit.width; ++x )
                    {
                        const float u = ( x + .5f ) / grit.width, v = ( y + .5f ) / grit.height;
                        const float seam = .12f * std::sin( 25.132741f * v ) + .073f * std::cos( 43.982297f * v + 8.f * u );
                        const float grain = cityVerge::unit( ( u64( seed ) << 32 ) + y * grit.width + x );
                        const float soil = 98.f + 22.f * grain + 20.f * seam;
                        const float pebble = grain > .934f ? 26.f : 0.f;
                        auto *pixel = grit.pixel( x, y );
                        pixel[0] = u8( soil + 5.f + pebble );
                        pixel[1] = u8( soil - 6.f + pebble );
                        pixel[2] = u8( soil - 26.f + pebble * .83f );
                        pixel[3] = u8( 186.f * cityVerge::smooth( ( .88f - u + seam * .8f ) / .73f ) );
                    }
                auto finish = material( assets, ColourF::White, .96f );
                finish->setTexture( upload( assets, grit ), 0 );
                finish->setTransparent( true );
                finish->setBlendMode( 1 );
                finish->setDepthWrite( false );
                if( auto actor = mesh( assets, "Terrain city entrance path gravel shoulders", batches.shoulders, finish ) )
                    actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                WP_LOG( String( "Terrain city entrance gravel shoulder sections=" ) + StringUtil::toString( batches.sections ) );
            }
        }  // namespace cityEntrancePathEdge
