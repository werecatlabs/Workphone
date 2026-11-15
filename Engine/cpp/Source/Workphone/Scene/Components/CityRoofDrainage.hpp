#pragma once

        namespace cityRoofDrainage
        {
            struct Surfaces { Geometry gutters, hardware, pipes; };
            struct Materials { SmartPtr<render::IMaterial> gutters, hardware, pipes; };
            struct Counts { size_t runs = 0, brackets = 0, downpipes = 0; };

            struct Roof
            {
                Vector3F deckCentre;
                float halfLong, halfShort, wallBase;
                bool longX, hipped, allowDownpipes;

                Vector3F point( float u, float y, float v ) const
                {
                    return deckCentre + ( longX ? Vector3F( u, y, v ) : Vector3F( v, y, u ) );
                }
            };

            struct Run
            {
                float halfLength, eave, lowEnd;
                bool alongU;

                Vector3F point( const Roof &roof, float station, float height, float outside ) const
                {
                    const float edge = eave + std::copysign( outside, eave );
                    return alongU ? roof.point( station, height, edge ) : roof.point( edge, height, station );
                }

                Vector3F along( const Roof &roof ) const
                {
                    return alongU == roof.longX ? Vector3F::unitX() : Vector3F::unitZ();
                }

                Vector3F outward( const Roof &roof ) const
                {
                    const auto axis = alongU == roof.longX ? Vector3F::unitZ() : Vector3F::unitX();
                    return axis * ( eave > 0 ? 1.f : -1.f );
                }

                float lip( float station ) const
                {
                    const float t = std::clamp( .5f + .5f * lowEnd * station / halfLength, 0.f, 1.f );
                    return -.039f - .027f * t;
                }
            };

            constexpr float troughOffset = .168f, troughRadius = .110f, metalThickness = .009f;
            constexpr float pipeOffset = .186f, pipeRadius = .041f;

            Vector3F profile( const Roof &roof, const Run &run, float end, float radius, float angle )
            {
                const float outside = troughOffset + radius * std::cos( angle );
                // Each profile ring shares the same position with its adjacent hip run.
                // The angle and height at the mating corner stay identical from either edge.
                const float reach = roof.hipped ? outside : .048f;
                const float station = end * ( run.halfLength + reach );
                return run.point( roof, station, run.lip( station ) - radius * std::sin( angle ), outside );
            }

            void trough( Geometry &geometry, const Roof &roof, const Run &run, int sides )
            {
                const auto outward = run.outward( roof ), up = Vector3F::unitY();
                for( int i = 0; i < sides; ++i )
                {
                    const float a = pi * i / sides, b = pi * ( i + 1 ) / sides;
                    const float middle = ( a + b ) * .5f;
                    const auto normal = outward * std::cos( middle ) - up * std::sin( middle );
                    for( const bool interior : { true, false } )
                    {
                        const float r = troughRadius + ( interior ? 0 : metalThickness );
                        geometry.quad( profile( roof, run, -1.f, r, a ), profile( roof, run, 1.f, r, a ),
                                       profile( roof, run, 1.f, r, b ), profile( roof, run, -1.f, r, b ),
                                       interior ? -normal : normal );
                    }
                }
                for( float angle : { 0.f, pi } )
                {
                    const float outer = troughRadius + metalThickness;
                    geometry.quad( profile( roof, run, -1.f, troughRadius, angle ),
                                   profile( roof, run, 1.f, troughRadius, angle ),
                                   profile( roof, run, 1.f, outer, angle ), profile( roof, run, -1.f, outer, angle ), up );
                }
                if( roof.hipped ) return;
                for( float end : { -1.f, 1.f } )
                {
                    const float station = end * ( run.halfLength + .048f );
                    const auto centre = run.point( roof, station, run.lip( station ), troughOffset );
                    for( int i = 0; i < sides; ++i )
                    {
                        const float r = troughRadius + metalThickness;
                        const auto a = profile( roof, run, end, r, pi * i / sides );
                        const auto b = profile( roof, run, end, r, pi * ( i + 1 ) / sides );
                        geometry.quad( centre, a, ( a + b ) * .5f, b, run.along( roof ) * end );
                    }
                }
            }

            void bracket( Geometry &geometry, const Roof &roof, const Run &run, float station )
            {
                const float h = run.lip( station );
                const auto strap = [&]( float a, float ay, float b, float by ) {
                    constexpr float half = .025f;
                    const auto p = run.point( roof, station - half, ay, a );
                    const auto q = run.point( roof, station + half, ay, a );
                    const auto r = run.point( roof, station + half, by, b );
                    const auto s = run.point( roof, station - half, by, b );
                    auto normal = ( q - p ).crossProduct( s - p ).normaliseCopy();
                    if( normal.y > 0 ) normal = -normal;
                    geometry.quad( p, q, r, s, normal );
                };
                // Hide the inner heel in the fascia/slab and align the strap with the trough underside.
                strap( .024f, -.079f, troughOffset, h - troughRadius - metalThickness - .002f );
                strap( troughOffset, h - troughRadius - metalThickness - .002f, .271f, h - .059f );
            }

            void downpipe( Surfaces &out, const Roof &roof, const Run &run, float station )
            {
                constexpr int sides = 6;
                const float egress = roof.wallBase - roof.deckCentre.y;
                const float top = run.lip( station ) - troughRadius;
                const std::array points{
                    run.point( roof, station, top + .004f, troughOffset ),
                    run.point( roof, station, top - .075f, troughOffset ),
                    run.point( roof, station, top - .170f, pipeOffset ),
                    run.point( roof, station, egress + .53f, pipeOffset ),
                    run.point( roof, station, egress + .365f, .218f ),
                    run.point( roof, station, egress + .288f, .300f ) };
                const auto axis = run.along( roof );
                const auto ring = [&]( size_t i, float angle ) {
                    const auto tangent = ( points[std::min( i + 1, points.size() - 1 )] - points[i ? i - 1 : 0] ).normaliseCopy();
                    return axis * std::cos( angle ) + tangent.crossProduct( axis ) * std::sin( angle );
                };
                for( size_t i = 1; i < points.size(); ++i )
                {
                    for( int side = 0; side < sides; ++side )
                    {
                        const float a = 2.f * pi * side / sides, b = 2.f * pi * ( side + 1 ) / sides;
                        out.pipes.quad( points[i - 1] + ring( i - 1, a ) * pipeRadius,
                                        points[i - 1] + ring( i - 1, b ) * pipeRadius,
                                        points[i] + ring( i, b ) * pipeRadius, points[i] + ring( i, a ) * pipeRadius,
                                        ( ring( i - 1, ( a + b ) * .5f ) + ring( i, ( a + b ) * .5f ) ).normaliseCopy() );
                    }
                }
                const size_t end = points.size() - 1;
                const auto face = ( points[end] - points[end - 1] ).normaliseCopy();
                for( int side = 0; side < sides; ++side )
                {
                    const float a = 2.f * pi * side / sides, b = 2.f * pi * ( side + 1 ) / sides;
                    const auto ra = ring( end, a ), rb = ring( end, b );
                    const auto centre = points.back();
                    out.hardware.quad( centre + ra * pipeRadius, centre + rb * pipeRadius,
                                       centre + rb * ( pipeRadius - .008f ), centre + ra * ( pipeRadius - .008f ), face );
                }
                for( float fraction : { .24f, .62f, .89f } )
                {
                    const float height = ( egress + .53f ) * fraction + ( top - .170f ) * ( 1.f - fraction );
                    out.hardware.beam( run.point( roof, station, height, .006f ),
                                       run.point( roof, station, height, pipeOffset - pipeRadius + .009f ), .014f );
                    const auto x = run.point( roof, station, height, pipeOffset );
                    out.hardware.box( x, { .083f, .025f, .083f } );
                }
            }

            template <class Admit>
            Counts append( Surfaces &out, const Roof &roof, u64 lotId, const Admit &admit )
            {
                Counts built;
                if( !std::isfinite( roof.deckCentre.x ) || !std::isfinite( roof.deckCentre.y ) ||
                    !std::isfinite( roof.deckCentre.z ) || !std::isfinite( roof.halfLong ) || !std::isfinite( roof.halfShort ) ||
                    std::min( roof.halfLong, roof.halfShort ) < 1.35f ) return built;
                const float diagonal = render::foliageStableHash( lotId ^ 0x8b47417474657273ull ) & 1u ? 1.f : -1.f;
                const std::array runs{ Run{ roof.halfLong, -roof.halfShort, -diagonal, true },
                                       Run{ roof.halfLong, roof.halfShort, diagonal, true },
                                       Run{ roof.halfShort, -roof.halfLong, -diagonal, false },
                                       Run{ roof.halfShort, roof.halfLong, diagonal, false } };
                const size_t runCount = roof.hipped ? runs.size() : 2;
                const auto brackets = [&]( const Run &run, int sides ) {
                    return sides > 3 ? int( std::clamp( std::ceil( run.halfLength * .67f ), 2.f, 6.f ) ) : 2;
                };
                int sides = 0;
                for( int candidate : { 8, 5, 3 } )
                {
                    size_t vertices = 0;
                    for( size_t i = 0; i < runCount; ++i )
                        vertices += size_t( 4 * ( 2 * candidate + 2 + ( roof.hipped ? 0 : 2 * candidate ) ) +
                                            8 * brackets( runs[i], candidate ) );
                    if( admit( vertices ) )
                    {
                        sides = candidate;
                        break;
                    }
                }
                if( sides == 0 ) return built;
                for( size_t i = 0; i < runCount; ++i )
                {
                    const auto &run = runs[i];
                    trough( out.gutters, roof, run, sides );
                    ++built.runs;
                    const int count = brackets( run, sides );
                    for( int j = 0; j < count; ++j )
                    {
                        const float t = ( j + .5f ) / count;
                        bracket( out.hardware, roof, run, run.halfLength * ( 2.f * t - 1.f ) );
                        ++built.brackets;
                    }
                }
                // Long eaves reach opposing low corners for either profile. Emit
                // both supported stacks together, leaving no unfinished drop in a patch.
                constexpr size_t verticesPerDownpipe = 5 * 6 * 4 + 6 * 4 + 3 * ( 24 + 24 );
                if( roof.allowDownpipes && std::isfinite( roof.wallBase ) && roof.deckCentre.y - roof.wallBase >= 2.25f &&
                    admit( 2 * verticesPerDownpipe ) )
                {
                    for( size_t i = 0; i < 2; ++i )
                    {
                        const auto &run = runs[i];
                        downpipe( out, roof, run, run.lowEnd * ( run.halfLength - .185f ) );
                        ++built.downpipes;
                    }
                }
                return built;
            }

            Materials materials( SceneAssets &assets )
            {
                return { material( assets, { .30f, .323f, .319f, 1 }, .69f, .24f ),
                         material( assets, { .35f, .367f, .358f, 1 }, .66f, .26f ),
                         material( assets, { .279f, .302f, .298f, 1 }, .73f, .19f ) };
            }
        }  // namespace cityRoofDrainage
