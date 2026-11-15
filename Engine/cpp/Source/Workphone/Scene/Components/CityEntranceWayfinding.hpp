#pragma once

        namespace cityEntranceWayfinding
        {
            struct Mount
            {
                u64 lotId;
                Vector3F point, outward, towardDoor;
                float scale;
                bool commercial;
            };

            struct Plaque
            {
                static constexpr float halfWidth = .265f, halfHeight = .064f;
                static constexpr float thickness = .020f;
                Vector3F centre, right, outward;
                float scale;

                void ink( Geometry &geometry, Vector2F a, Vector2F b, float width ) const
                {
                    const auto direction = ( b - a ).normaliseCopy();
                    const Vector2F side( -direction.y * width * .5f, direction.x * width * .5f );
                    const auto point = [&]( Vector2F p ) {
                        return centre + ( right * p.x + Vector3F( 0, p.y, 0 ) ) * scale +
                               outward * ( thickness + .0015f );
                    };
                    geometry.quad( point( a - side ), point( b - side ), point( b + side ), point( a + side ), outward );
                }
            };

            void address( Geometry &ink, const Plaque &face, unsigned number, float direction )
            {
                constexpr u8 digits[10][7] = {
                    { 14, 17, 19, 21, 25, 17, 14 }, { 4, 12, 4, 4, 4, 4, 14 },
                    { 14, 17, 1, 2, 4, 8, 31 }, { 30, 1, 1, 14, 1, 1, 30 },
                    { 2, 6, 10, 18, 31, 2, 2 }, { 31, 16, 16, 30, 1, 1, 30 },
                    { 14, 16, 16, 30, 17, 17, 14 }, { 31, 1, 2, 4, 8, 8, 8 },
                    { 14, 17, 17, 14, 17, 17, 14 }, { 14, 17, 17, 15, 1, 1, 14 }
                };
                constexpr float pixel = .013f;
                const float x0 = -6.f * pixel - .080f * direction;
                for( unsigned glyph = 0; glyph < 2; ++glyph )
                {
                    const auto &rows = digits[glyph == 0 ? number / 10 : number % 10];
                    for( unsigned row = 0; row < 7; ++row )
                    {
                        const float y = ( 3.f - row ) * pixel;
                        unsigned column = 0;
                        while( column < 5 )
                        {
                            const auto start = column++;
                            if( !( rows[row] & ( 16u >> start ) ) ) continue;
                            while( column < 5 && ( rows[row] & ( 16u >> column ) ) ) ++column;
                            const float x = x0 + float( glyph * 7 ) * pixel;
                            face.ink( ink, { x + start * pixel, y }, { x + column * pixel, y }, pixel );
                        }
                    }
                }
                const auto arrow = [&]( float x, float y ) { return Vector2F( direction * x, y ); };
                face.ink( ink, arrow( .079f, 0 ), arrow( .184f, 0 ), .013f );
                face.ink( ink, arrow( .150f, .033f ), arrow( .186f, -.003f ), .012f );
                face.ink( ink, arrow( .150f, -.033f ), arrow( .186f, .003f ), .012f );
                for( const float side : { -1.f, 1.f } )
                {
                    face.ink( ink, { -.232f, side * .053f }, { .232f, side * .053f }, .003f );
                    face.ink( ink, { side * .224f, 0 }, { side * .234f, 0 }, .010f );
                }
            }

            void build( SceneAssets &assets, const SourceBudgetVector<Mount> &mounts )
            {
                if( mounts.empty() || assets.cityStreetDetail <= 0 ) return;
                const size_t maximum = ( std::min<size_t>( 96, mounts.size() ) *
                    size_t( std::clamp( assets.cityStreetDetail, 0, 100 ) ) + 99 ) / 100;
                Geometry plates, ink;
                size_t count = 0;
                const auto add = [&]( const Mount &mount ) {
                    const Vector3F up( 0, 1, 0 );
                    const auto right = up.crossProduct( mount.outward );
                    const Plaque face{ mount.point, right, mount.outward, mount.scale };
                    const float width = 2.f * Plaque::halfWidth * mount.scale;
                    const Vector3F size( width * std::abs( right.x ) + Plaque::thickness * std::abs( mount.outward.x ),
                                         2.f * Plaque::halfHeight * mount.scale,
                                         width * std::abs( right.z ) + Plaque::thickness * std::abs( mount.outward.z ) );
                    plates.box( mount.point + mount.outward * ( Plaque::thickness * .5f ), size );
                    const auto number = 10u + u32( render::foliageStableHash( mount.lotId ) % 90u );
                    const float direction = mount.towardDoor.dotProduct( right ) < 0 ? -1.f : 1.f;
                    address( ink, face, number, direction );
                    ++count;
                    WP_LOG( String( "Terrain city entrance address lot=" ) + StringUtil::toString( mount.lotId ) +
                            ", address=" + StringUtil::toString( number ) );
                };
                for( const bool commercial : { true, false } )
                    for( const auto &mount : mounts )
                        if( count < maximum && mount.commercial == commercial ) add( mount );
                if( auto actor = mesh( assets, "Terrain city entrance enamel address plates", plates,
                                       material( assets, { .044f, .106f, .116f, 1 }, .57f, .09f ) ) )
                    actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                if( auto actor = mesh( assets, "Terrain city entrance address lettering", ink,
                                       material( assets, { .94f, .89f, .73f, 1 }, .75f ) ) )
                    actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                WP_LOG( String( "Terrain city entrance address plates=" ) + StringUtil::toString( count ) );
            }
        }
