#pragma once

        namespace cityRoofEquipment
        {
            struct Surfaces { Geometry curbs, housings, metal, grilles; };
            struct Materials { SmartPtr<render::IMaterial> curbs, housings, metal, grilles; };
            struct Counts { u32 airHandlers = 0, condensers = 0, vents = 0; };

            Materials materials( SceneAssets &assets )
            {
                auto galvanized = material( assets, { .57f, .592f, .580f, 1 }, .59f, .34f );
                galvanized->setSpecular( { .065f, .063f, .061f, 1 } );
                return { material( assets, { .51f, .48f, .43f, 1 }, .94f ),
                         material( assets, { .43f, .464f, .455f, 1 }, .78f ),
                         galvanized, material( assets, { .17f, .193f, .181f, 1 }, .75f, .18f ) };
            }

            struct Frame
            {
                Vector3F centre;
                float halfLong, halfShort;
                bool longX;

                Vector3F point( float u, float y, float v ) const
                {
                    return centre + ( longX ? Vector3F( u, y, v ) : Vector3F( v, y, u ) );
                }

                void box( Geometry &geometry, float u, float y, float v, float width, float height, float depth ) const
                {
                    geometry.box( point( u, y, v ), longX ? Vector3F( width, height, depth )
                                                           : Vector3F( depth, height, width ) );
                }
            };

            struct Mount
            {
                float u, v, width, depth;
            };

            void triangle( Geometry &geometry, Vector3F a, Vector3F b, Vector3F c, Vector3F normal )
            {
                if( ( b - a ).crossProduct( c - a ).dotProduct( normal ) < 0 ) std::swap( b, c );
                const auto tangent = ( b - a ).normaliseCopy();
                const u32 first = u32( geometry.positions.size() );
                for( const auto vertex : { a, b, c } )
                {
                    geometry.positions.push_back( vertex );
                    geometry.normals.push_back( normal );
                    geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
                    geometry.uv.push_back( { vertex.x * .37f, vertex.z * .37f, 0 } );
                }
                for( u32 i = 0; i < 3; ++i ) geometry.indices.push_back( first + i );
            }

            Vector3F radial( const Frame &frame, float u, float y, float v, float r, float angle )
            {
                return frame.point( u + r * std::cos( angle ), y, v + r * std::sin( angle ) );
            }

            void fan( Surfaces &out, const Frame &frame, float u, float y, float v, float radius, float phase )
            {
                constexpr u32 sides = 12;
                const auto up = Vector3F::unitY();
                const auto well = frame.point( u, y + .013f, v );
                for( u32 i = 0; i < sides; ++i )
                {
                    const float a = 2.f * pi * i / sides, b = 2.f * pi * ( i + 1u ) / sides;
                    triangle( out.grilles, well, radial( frame, u, y + .013f, v, radius * .965f, a ),
                              radial( frame, u, y + .013f, v, radius * .965f, b ), up );
                    out.metal.quad( radial( frame, u, y + .034f, v, radius, a ), radial( frame, u, y + .034f, v, radius, b ),
                                    radial( frame, u, y + .034f, v, radius * .82f, b ), radial( frame, u, y + .034f, v, radius * .82f, a ), up );
                }
                for( u32 blade = 0; blade < 5; ++blade )
                {
                    const float angle = phase + blade * ( 2.f * pi / 5.f );
                    const float rotor = y + .024f;
                    out.housings.quad( radial( frame, u, rotor, v, radius * .15f, angle ),
                        radial( frame, u, rotor, v, radius * .72f, angle + .19f ),
                        radial( frame, u, rotor, v, radius * .76f, angle + .46f ),
                        radial( frame, u, rotor, v, radius * .18f, angle + .60f ), up );
                }
                frame.box( out.metal, u, y + .036f, v, radius * .34f, .028f, radius * .34f );
                const float guardHeight = y + .057f;
                for( const float spacing : { -.42f, 0.f, .42f } )
                {
                    const float chord = radius * std::sqrt( .92f * .92f - spacing * spacing );
                    const float stripe = v + spacing * radius, width = .014f;
                    out.metal.quad( frame.point( u - chord, guardHeight, stripe - width ),
                                    frame.point( u + chord, guardHeight, stripe - width ),
                                    frame.point( u + chord, guardHeight, stripe + width ),
                                    frame.point( u - chord, guardHeight, stripe + width ), up );
                }
            }

            void intake( Surfaces &out, const Frame &frame, const Mount &mount, float base, float height,
                         bool dense, bool both )
            {
                const float width = mount.width * ( both ? .74f : .66f );
                const float centre = base + height * .52f;
                const float opening = height * .51f;
                for( const float side : { -1.f, 1.f } )
                {
                    if( !both && side > 0 ) break;
                    const float face = mount.v + side * ( mount.depth * .5f + .012f );
                    frame.box( out.grilles, mount.u, centre, face, width, opening, .025f );
                    if( !dense ) continue;
                    const u32 blades = height > .70f ? 7u : 5u;
                    const auto normal = ( frame.point( 0, .037f, side * .026f ) - frame.centre ).normaliseCopy();
                    for( u32 i = 0; i < blades; ++i )
                    {
                        const float y = centre + opening * ( ( i + .50f ) / blades - .5f );
                        const float u0 = mount.u - width * .48f, u1 = mount.u + width * .48f;
                        out.metal.quad( frame.point( u0, y + .014f, face ), frame.point( u1, y + .014f, face ),
                            frame.point( u1, y - .012f, face + side * .037f ),
                            frame.point( u0, y - .012f, face + side * .037f ), normal );
                    }
                }
            }

            void unit( Surfaces &out, const Frame &frame, const Mount &mount, bool handler, bool detailed, float phase )
            {
                const float height = handler ? .92f : .66f;
                const float curb = handler ? .18f : .14f;
                const float top = curb + height;
                frame.box( out.curbs, mount.u, curb * .5f - .012f, mount.v,
                           mount.width + .14f, curb + .024f, mount.depth + .14f );
                frame.box( out.housings, mount.u, curb + height * .5f, mount.v, mount.width, height, mount.depth );
                frame.box( out.metal, mount.u, top - .016f, mount.v, mount.width + .07f, .042f, mount.depth + .07f );
                intake( out, frame, mount, curb, height, detailed, handler );
                if( !detailed ) return;
                const float y = top + .004f;
                if( handler )
                {
                    const float offset = std::min( mount.width * .24f, .64f );
                    const float radius = std::min( .385f, mount.depth * .35f );
                    fan( out, frame, mount.u - offset, y, mount.v, radius, phase );
                    fan( out, frame, mount.u + offset, y, mount.v, radius * .92f, phase + .73f );
                    const float seam = mount.u - mount.width * .5f - .012f;
                    frame.box( out.grilles, seam, curb + height * .60f, mount.v, .025f, height * .55f, mount.depth * .49f );
                    frame.box( out.metal, seam - .024f, curb + height * .60f, mount.v + mount.depth * .154f, .041f, .13f, .032f );
                }
                else
                {
                    fan( out, frame, mount.u, y, mount.v, std::min( mount.width, mount.depth ) * .38f, phase );
                    const float face = mount.v + mount.depth * .5f + .014f;
                    frame.box( out.grilles, mount.u, curb + height * .44f, face, mount.width * .48f, .026f, .022f );
                }
            }

            void vent( Surfaces &out, const Frame &frame, const Mount &mount, bool tall, bool detailed )
            {
                frame.box( out.curbs, mount.u, .046f, mount.v, .45f, .116f, .45f );
                const float h = tall ? .48f : .32f;
                if( !detailed )
                {
                    frame.box( out.housings, mount.u, .11f + h * .43f, mount.v, .20f, h * .87f, .20f );
                    frame.box( out.metal, mount.u, .10f + h, mount.v, .35f, .074f, .35f );
                    return;
                }
                constexpr u32 sides = 10;
                const float bottom = .10f, eave = .09f + h;
                const float lip = tall ? .162f : .20f;
                for( u32 i = 0; i < sides; ++i )
                {
                    const float a = 2.f * pi * i / sides, b = 2.f * pi * ( i + 1u ) / sides;
                    const auto outward = ( radial( frame, 0, 0, 0, 1.f, ( a + b ) * .5f ) - frame.centre ).normaliseCopy();
                    out.housings.quad( radial( frame, mount.u, bottom, mount.v, .106f, a ),
                        radial( frame, mount.u, bottom, mount.v, .106f, b ), radial( frame, mount.u, eave, mount.v, .106f, b ),
                        radial( frame, mount.u, eave, mount.v, .106f, a ), outward );
                    out.metal.quad( radial( frame, mount.u, eave - .026f, mount.v, lip, a ),
                        radial( frame, mount.u, eave - .026f, mount.v, lip, b ), radial( frame, mount.u, eave + .021f, mount.v, lip, b ),
                        radial( frame, mount.u, eave + .021f, mount.v, lip, a ), outward );
                    triangle( out.metal, frame.point( mount.u, eave + .104f, mount.v ),
                              radial( frame, mount.u, eave + .021f, mount.v, lip, a ),
                              radial( frame, mount.u, eave + .021f, mount.v, lip, b ),
                              ( Vector3F::unitY() + outward * ( .083f / lip ) ).normaliseCopy() );
                }
            }

            template <class Admit>
            Counts append( Surfaces &out, Vector3F centre, Vector3F size, float deck, u64 lot, u32 seed,
                           bool office, const Admit &admit, Vector2F lanternHalfXZ = {} )
            {
                centre.y = deck;
                const Frame frame{ centre, std::max( size.x, size.z ) * .5f, std::min( size.x, size.z ) * .5f, size.x >= size.z };
                const u64 key = render::foliageStableHash( lot ^ ( u64( seed ) << 32 ) ^ 0x726f6f666d656368ull );
                const auto scalar = []( u64 id ) { return float( render::foliageStableHash( id ) >> 40 ) / 16777216.f; };
                const float flank = ( key & 1u ) ? 1.f : -1.f, back = ( key & 2u ) ? 1.f : -1.f;
                const float lane = std::min( 1.65f, frame.halfShort * .24f );
                const float phase = 2.f * pi * scalar( key + 12 );
                constexpr float rim = .74f, separation = .54f, lanternGap = .41f, curbProjection = .07f;
                const bool lantern = lanternHalfXZ.x > 0.f && lanternHalfXZ.y > 0.f;
                const float lanternLong = frame.longX ? lanternHalfXZ.x : lanternHalfXZ.y;
                const float lanternShort = frame.longX ? lanternHalfXZ.y : lanternHalfXZ.x;
                std::array<Mount, 5> used{};
                size_t occupied = 0;
                Counts counts;
                const auto fits = [&]( const Mount &mount ) {
                    const float nearU = std::abs( mount.u ) - mount.width * .5f - curbProjection;
                    const float nearV = std::abs( mount.v ) - mount.depth * .5f - curbProjection;
                    const float insetU = frame.halfLong - std::abs( mount.u ) - mount.width * .5f - curbProjection;
                    const float insetV = frame.halfShort - std::abs( mount.v ) - mount.depth * .5f - curbProjection;
                    // Keep the curb off the outer coping and both diagonal drain pockets.
                    if( insetU < rim || insetV < rim || ( insetU < 1.27f && insetV < 1.27f ) ) return false;
                    if( lantern && nearU < lanternLong + lanternGap && nearV < lanternShort + lanternGap ) return false;
                    return std::none_of( used.begin(), used.begin() + occupied, [&]( const Mount &other ) {
                        return std::abs( mount.u - other.u ) < ( mount.width + other.width ) * .5f + separation &&
                               std::abs( mount.v - other.v ) < ( mount.depth + other.depth ) * .5f + separation;
                    } );
                };
                const auto machinery = [&]( const Mount &mount, bool handler ) {
                    if( !fits( mount ) ) return false;
                    const bool detailed = admit( handler ? 510u : 285u );
                    if( !detailed && !admit( handler ? 120u : 96u ) ) return false;
                    unit( out, frame, mount, handler, detailed, phase );
                    used[occupied++] = mount;
                    if( handler ) ++counts.airHandlers;
                    else ++counts.condensers;
                    return true;
                };
                const float width = 2.25f + .45f * scalar( key + 23 );
                const auto cappedVent = [&]( const Mount &mount, bool tall ) {
                    if( !fits( mount ) ) return false;
                    const bool detailed = admit( 136u );
                    if( !detailed && !admit( 72u ) ) return false;
                    vent( out, frame, mount, tall, detailed );
                    used[occupied++] = mount;
                    ++counts.vents;
                    return true;
                };
                if( lantern )
                {
                    // Find the centre of the service space between an existing cap
                    // and the coping. Offsets lie well clear of either window face.
                    const auto endStation = [=]( float halfRoof, float halfLantern ) {
                        return ( halfLantern + lanternGap + halfRoof - rim ) * .5f;
                    };
                    const float end = endStation( frame.halfLong, lanternLong );
                    const float side = endStation( frame.halfShort, lanternShort );
                    for( const float endSign : { flank, -flank } )
                    {
                        if( counts.condensers > 0 && ( !office || frame.halfLong < 5.2f ) ) break;
                        const float offset = -back * endSign * .28f;
                        if( !machinery( { endSign * end, offset, 1.19f, .98f }, false ) )
                            machinery( { offset, endSign * side, 1.19f, .98f }, false );
                    }
                    for( const float endSign : { -flank, flank } )
                    {
                        const float offset = back * endSign * std::min( 1.67f, frame.halfShort * .46f );
                        if( !cappedVent( { endSign * end, offset, .45f, .45f }, endSign < 0.f ) )
                            cappedVent( { offset, endSign * side, .45f, .45f }, endSign < 0.f );
                    }
                    if( office && frame.halfLong > 6.2f )
                        machinery( { -flank * frame.halfLong * .23f, -back * side, width, 1.38f }, true );
                    return counts;
                }
                if( office || frame.halfLong > 5.6f )
                    machinery( { -flank * std::min( 1.76f, frame.halfLong * .33f ), back * lane,
                                 width, 1.37f + .15f * scalar( key + 27 ) }, true );
                if( !machinery( { flank * std::min( 2.10f, frame.halfLong * .39f ), -back * lane * .38f,
                                 1.21f + .14f * scalar( key + 31 ), .98f }, false ) && !counts.airHandlers )
                    machinery( { 0.f, 0.f, 1.17f, .96f }, false );
                if( office && frame.halfLong > 6.3f && frame.halfShort > 3.6f && scalar( key + 33 ) > .40f )
                    machinery( { flank * frame.halfLong * .57f, back * lane * 1.16f, 1.36f, 1.04f }, false );
                for( unsigned i = 0; i < ( office ? 2u : 1u ); ++i )
                {
                    Mount mount{ ( i ? flank : -flank ) * std::min( 4.04f, frame.halfLong * .62f ),
                                 -back * std::min( 2.83f, frame.halfShort * .61f ), .45f, .45f };
                    if( !fits( mount ) ) mount.v *= .77f;
                    cappedVent( mount, ( key & 4u ) != 0 ? i == 0 : i != 0 );
                }
                return counts;
            }
        }  // namespace cityRoofEquipment
