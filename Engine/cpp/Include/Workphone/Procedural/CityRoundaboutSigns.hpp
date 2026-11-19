#pragma once

namespace cityRoundaboutSigns
{
    enum class Kind
    {
        GiveWay,
        Warning
    };

    struct Site
    {
        Vector3F position, outward;
        size_t port;
        float radius;
        Kind kind;
        bool clockwise, blocked = false;
    };

    struct SignFace
    {
        Vector3F centre, outward;

        Vector3F right() const
        {
            return { outward.z, 0, -outward.x };
        }

        Vector3F point( Vector2F p, float offset ) const
        {
            return centre + right() * p.x + Vector3F( 0, p.y, 0 ) + outward * offset;
        }

        void triangle( Geometry &geometry, Vector2F a, Vector2F b, Vector2F c, float offset,
                       bool back = false ) const
        {
            const auto normal = back ? -outward : outward;
            const auto tangent = right();
            const auto index = u32( geometry.positions.size() );
            const auto p = point( a, offset ), q = point( b, offset ), r = point( c, offset );
            for( const auto &vertex : { p, q, r } )
            {
                geometry.positions.push_back( vertex );
                geometry.normals.push_back( normal );
                geometry.uv.push_back( { 0, 0, 0 } );
                geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
            }
            const bool forward = ( q - p ).crossProduct( r - p ).dotProduct( normal ) > 0;
            geometry.indices.push_back( index );
            geometry.indices.push_back( index + ( forward ? 1 : 2 ) );
            geometry.indices.push_back( index + ( forward ? 2 : 1 ) );
        }

        void stroke( Geometry &geometry, Vector2F a, Vector2F b, float width ) const
        {
            const auto line = b - a;
            const auto n = Vector2F( -line.y, line.x ).normaliseCopy() * ( width * .5f );
            geometry.quad( point( a - n, .020f ), point( b - n, .020f ), point( b + n, .020f ),
                           point( a + n, .020f ), outward );
        }
    };

    void roundaboutArrows( const SignFace &face, Geometry &ink, float scale, bool clockwise )
    {
        const float direction = clockwise ? -1.f : 1.f;
        for( u32 arrow = 0; arrow < 3; ++arrow )
        {
            const float start = pi * ( .5f + float( arrow ) * 2.f / 3.f );
            const auto at = [&]( float step ) {
                const float angle = start + direction * step;
                return Vector2F( std::cos( angle ), std::sin( angle ) ) * ( .234f * scale ) +
                       Vector2F( 0, -.11f * scale );
            };
            for( u32 edge = 0; edge < 5; ++edge )
            {
                face.stroke( ink, at( .286f * edge ), at( .286f * ( edge + 1 ) ), .052f * scale );
            }
            const auto head = at( 1.43f );
            const auto forward = ( at( 1.50f ) - head ).normaliseCopy();
            const Vector2F side( -forward.y, forward.x );
            face.triangle( ink, head + forward * ( .085f * scale ), head + side * ( .053f * scale ),
                           head - side * ( .053f * scale ), .020f );
        }
    }

    void giveWayLabel( const SignFace &face, Geometry &ink, float scale )
    {
        // Five-by-seven capitals as horizontal runs keep the sign texture-free.
        constexpr u8 glyphs[][7] = { { 14, 17, 16, 23, 17, 17, 14 }, { 14, 4, 4, 4, 4, 4, 14 },
                                     { 17, 17, 17, 17, 10, 10, 4 },  { 31, 16, 16, 30, 16, 16, 31 },
                                     { 17, 17, 17, 21, 21, 21, 10 }, { 14, 17, 17, 31, 17, 17, 17 },
                                     { 17, 17, 10, 4, 4, 4, 4 } };
        constexpr float pixel = .014f;
        for( u32 glyph = 0; glyph < 7; ++glyph )
        {
            const bool top = glyph < 4;
            const float x0 = ( ( top ? int( glyph ) - 2.f : int( glyph ) - 5.5f ) * 6.f + .5f ) * pixel;
            const float y0 = top ? .204f : .062f;
            for( u32 row = 0; row < 7; ++row )
            {
                const float y = ( y0 - row * pixel ) * scale;
                for( u32 c = 0; c < 5; )
                {
                    const u32 begin = c;
                    if( !( glyphs[glyph][row] & ( 16u >> c ) ) )
                    {
                        ++c;
                        continue;
                    }
                    while( c < 5 && glyphs[glyph][row] & ( 16u >> c ) )
                    {
                        ++c;
                    }
                    face.stroke( ink, { ( x0 + begin * pixel ) * scale, y },
                                 { ( x0 + c * pixel ) * scale, y }, pixel * scale );
                }
            }
        }
    }

    void build( SceneAssets &assets, const CityRoadTerrainMesh &footways,
                const SourceBudgetVector<Vector3F> &lamps, VehicleAppearanceQuality quality )
    {
        const int detail = std::clamp( assets.cityStreetDetail, 0, 100 );
        const auto ground = assets.roadAuthoring->getTerrainSnapshot();
        const auto &source = assets.roadAuthoring->getSource();
        if( !detail || !ground || source.roundabouts.empty() )
        {
            return;
        }
        const auto &placement = assets.roadAuthoring->getTerrainPlacement();
        const cityGroundContact::WalkEdges kerbs( footways );
        SourceBudgetVector<Site> sites;
        const bool preview = quality == VehicleAppearanceQuality::Preview;
        const size_t portLimit = preview ? 24 : 48;
        size_t observedPorts = 0;
        const auto sample = [&]( Vector3F p, float &height ) {
            return render::sampleTerrainHeight( *ground, placement, p, height );
        };
        const auto offer = [&]( const auto &junction, const auto &port, Vector3F direction,
                                Vector3F roadside, float row, float radius, Kind kind ) {
            const auto centre = junction.center + direction * ( junction.outerRadius + row );
            const auto extent = direction * radius;
            const float a = kerbs.reach( centre - extent, roadside, true );
            const float b = kerbs.reach( centre, roadside, true );
            const float c = kerbs.reach( centre + extent, roadside, true );
            if( std::min( { a, b, c } ) < port.width * .5f + 1.55f ||
                std::max( { a, b, c } ) > port.width * .5f + 3.6f )
            {
                return;
            }
            auto position = centre + roadside * ( std::max( { a, b, c } ) + radius + .24f );
            if( !sample( position, position.y ) ||
                std::any_of( lamps.begin(), lamps.end(),
                             [&]( Vector3F lamp ) {
                                 return cityHillside::segmentDistance( position, lamp, lamp ) <
                                        1.25f * 1.25f;
                             } ) ||
                std::any_of(
                    assets.cityLayout.lots.begin(), assets.cityLayout.lots.end(),
                    [&]( const auto &lot ) {
                        return std::abs( position.x - lot.parcelX ) < lot.parcelWidth * .5f + radius &&
                               std::abs( position.z - lot.parcelZ ) < lot.parcelDepth * .5f + radius;
                    } ) )
            {
                return;
            }
            for( const auto &offset :
                 { direction * .14f, direction * -.14f, roadside * .14f, roadside * -.14f } )
            {
                float h = 0;
                if( !sample( position + offset, h ) || std::abs( h - position.y ) > .075f )
                {
                    return;
                }
            }
            const auto face = ( direction + roadside * -.085f ).normaliseCopy();
            sites.push_back( { position, face, observedPorts, radius, kind, junction.clockwise } );
        };
        for( const auto &junction : source.roundabouts )
        {
            for( const auto &port : junction.ports )
            {
                if( observedPorts >= portLimit )
                {
                    break;
                }
                ++observedPorts;
                auto direction = Vector3F( port.outward.x, 0, port.outward.z ).normaliseCopy();
                const auto roadside =
                    Vector3F( direction.z, 0, -direction.x ) * ( junction.clockwise ? 1.f : -1.f );
                const bool split = port.width >= 6.f && port.length >= 12.f;
                // Give Way signs are inside the crossing setback. Long arms also
                // have room for an advance warning on the far side of the crossing.
                for( const float row : { 2.46f, 1.91f, 2.75f, 4.18f } )
                {
                    if( row < port.length - .52f && ( row < 3.f || split ) )
                    {
                        offer( junction, port, direction, roadside, row, .45f, Kind::GiveWay );
                    }
                }
                const float crossingEnd = split ? std::min( 8.f, port.length * .5f ) + 3.65f : 6.15f;
                if( detail >= 40 && port.length >= 12.f )
                {
                    for( const float fraction : { .88f, .79f } )
                    {
                        const float row = port.length * fraction;
                        if( row > crossingEnd + .85f )
                        {
                            offer( junction, port, direction, roadside, row, .42f, Kind::Warning );
                        }
                    }
                }
            }
        }

        // Check the complete plate footprint against every retained fitted top,
        // including private entrance paving added to this clearance mesh.
        SourceBudgetMap<cityHillside::Cell, SourceBudgetVector<size_t>> candidates;
        for( size_t i = 0; i < sites.size(); ++i )
        {
            const auto &site = sites[i];
            const Vector3F r( site.radius, 0, site.radius );
            cityHillside::visitCells( site.position - r, site.position + r, [&]( auto cell ) {
                candidates[cell].push_back( i );
                return true;
            } );
        }
        for( size_t i = 0; !candidates.empty() && i + 2 < footways.indices.size(); i += 3 )
        {
            const auto a = footways.vertices[footways.indices[i]].position;
            const auto b = footways.vertices[footways.indices[i + 1]].position;
            const auto c = footways.vertices[footways.indices[i + 2]].position;
            if( std::abs( cityHillside::cross( b - a, c - a ) ) <= .00001f )
            {
                continue;
            }
            const Vector3F low( std::min( { a.x, b.x, c.x } ), 0, std::min( { a.z, b.z, c.z } ) );
            const Vector3F high( std::max( { a.x, b.x, c.x } ), 0, std::max( { a.z, b.z, c.z } ) );
            cityHillside::visitCells( low, high, [&]( auto cell ) {
                const auto found = candidates.find( cell );
                if( found == candidates.end() )
                {
                    return true;
                }
                for( const auto index : found->second )
                {
                    auto &site = sites[index];
                    const auto p = site.position;
                    if( site.blocked || p.x + site.radius < low.x || p.x - site.radius > high.x ||
                        p.z + site.radius < low.z || p.z - site.radius > high.z )
                    {
                        continue;
                    }
                    const auto x = cityHillside::cross( b - a, p - a );
                    const auto y = cityHillside::cross( c - b, p - b );
                    const auto z = cityHillside::cross( a - c, p - c );
                    site.blocked = ( x >= 0 && y >= 0 && z >= 0 ) || ( x <= 0 && y <= 0 && z <= 0 ) ||
                                   std::min( { cityHillside::segmentDistance( p, a, b ),
                                               cityHillside::segmentDistance( p, b, c ),
                                               cityHillside::segmentDistance( p, c, a ) } ) <
                                       site.radius * site.radius;
                }
                return true;
            } );
        }

        Geometry posts, metal, red, white, blue, ink;
        SourceBudgetVector<Vector3F> installed;
        std::array<std::array<bool, 2>, 49> used{};
        size_t roadsideChecks = 0;
        u32 yields = 0, warnings = 0, arrows = 0;
        for( const auto &site : sites )
        {
            const bool yield = site.kind == Kind::GiveWay;
            if( used[site.port][yield] || site.blocked || roadsideChecks >= 160 ||
                std::any_of( installed.begin(), installed.end(), [&]( Vector3F p ) {
                    return cityHillside::segmentDistance( p, site.position, site.position ) <
                           3.3f * 3.3f;
                } ) )
            {
                continue;
            }
            bool clear = true;
            const Vector3F side( site.outward.z, 0, -site.outward.x );
            for( const float x : { 0.f, -.37f, .37f } )
            {
                const auto p = site.position + side * x;
                float height = 0;
                bool drivable = false;
                ++roadsideChecks;
                if( assets.roadAuthoring->getRuntimeSurface( p.x, p.z, height, drivable ) )
                {
                    clear = false;
                    break;
                }
            }
            if( !clear )
            {
                continue;
            }
            const float scale = yield ? 1.f : .88f;
            const float signHeight = yield ? 2.02f : 2.12f;
            const SignFace face{ site.position + Vector3F( 0, signHeight, 0 ), site.outward };
            posts.beam( site.position - Vector3F( 0, .035f, 0 ), face.centre - site.outward * .037f,
                        .031f );
            const float sign = yield ? 1.f : -1.f;
            const Vector2F a( -.383f * scale, .267f * sign * scale );
            const Vector2F b( .383f * scale, .267f * sign * scale );
            const Vector2F c( 0, -.399f * sign * scale );
            face.triangle( metal, a, b, c, -.017f, true );
            face.triangle( red, a, b, c, .002f );
            const float inner = .77f;
            const Vector2F field( 0, .038f * sign * scale );
            face.triangle( white, a * inner + field, b * inner + field, c * inner + field, .013f );
            if( yield )
            {
                giveWayLabel( face, ink, scale );
                ++yields;
            }
            else
            {
                roundaboutArrows( face, ink, .56f * scale, site.clockwise );
                ++warnings;
            }
            used[site.port][yield] = true;
            installed.push_back( site.position );
        }

        size_t splitterPorts = 0;
        for( const auto &junction : source.roundabouts )
        {
            for( const auto &port : junction.ports )
            {
                if( splitterPorts >= portLimit )
                {
                    break;
                }
                ++splitterPorts;
                if( port.width < 6.f || port.length < 12.f )
                {
                    continue;
                }
                const auto direction = Vector3F( port.outward.x, 0, port.outward.z ).normaliseCopy();
                const Vector3F across( direction.z, 0, -direction.x );
                const auto start = std::min( 8.f, port.length * .5f ) - 1.48f;
                auto position = junction.center + direction * ( junction.outerRadius + start );
                bool refuge = true;
                for( const auto &offset : { Vector3F::zero(), across * .22f, across * -.22f,
                                            direction * .18f, direction * -.18f } )
                {
                    const auto p = position + offset;
                    float height = 0;
                    bool drivable = false;
                    if( !assets.roadAuthoring->getRuntimeSurface( p.x, p.z, height, drivable ) ||
                        drivable )
                    {
                        refuge = false;
                        break;
                    }
                    if( offset.lengthSquared() < .0001f )
                    {
                        position.y = height;
                    }
                    else if( std::abs( height - position.y ) > .05f )
                    {
                        refuge = false;
                        break;
                    }
                }
                if( !refuge )
                {
                    continue;
                }
                const SignFace face{ position + Vector3F( 0, .75f, 0 ), direction };
                posts.beam( position - Vector3F( 0, .018f, 0 ), face.centre - direction * .039f, .031f );
                const auto rim = []( u32 i ) {
                    const float angle = i * 2.f * pi / 20;
                    return Vector2F( std::cos( angle ), std::sin( angle ) ) * .218f;
                };
                for( u32 i = 0; i < 20; ++i )
                {
                    face.triangle( metal, {}, rim( i ), rim( i + 1 ), -.013f, true );
                    face.triangle( blue, {}, rim( i ), rim( i + 1 ), .005f );
                }
                // Mirror from the actual authored incoming lane, in the sign's own facing frame.
                const auto forward = Vector2F( junction.clockwise ? .707f : -.707f, -.707f );
                const Vector2F side( -forward.y, forward.x );
                face.stroke( white, forward * -.123f, forward * .050f, .053f );
                face.triangle( white, forward * .144f, forward * .032f + side * .063f,
                               forward * .032f - side * .063f, .020f );
                ++arrows;
            }
        }

        const auto publish = [&]( const char *name, const Geometry &geometry, ColourF tint,
                                  float roughness, ColourF emission = ColourF( 0, 0, 0, 1 ) ) {
            if( geometry.indices.empty() )
            {
                return;
            }
            auto surface = material( assets, tint, roughness );
            surface->setCullMode( 2 );
            surface->setEmissive( emission );
            if( auto actor = mesh( assets, name, geometry, surface ) )
            {
                actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                    scene::Renderer::CastShadows::Off );
            }
        };
        publish( "Terrain city roundabout sign posts", posts, { .47f, .49f, .49f, 1 }, .71f );
        publish( "Terrain city roundabout sign backs", metal, { .45f, .47f, .46f, 1 }, .72f );
        publish( "Terrain city roundabout sign red borders", red, { .80f, .080f, .062f, 1 }, .56f,
                 { .021f, .001f, .001f, 1 } );
        publish( "Terrain city roundabout sign white faces", white, { .93f, .92f, .88f, 1 }, .54f,
                 { .018f, .017f, .016f, 1 } );
        publish( "Terrain city roundabout sign blue faces", blue, { .085f, .31f, .65f, 1 }, .51f,
                 { .001f, .006f, .014f, 1 } );
        publish( "Terrain city roundabout sign lettering", ink, { .090f, .087f, .081f, 1 }, .78f );
        WP_LOG( String( "Terrain city roundabout signs yield=" ) + StringUtil::toString( yields ) +
                " warnings=" + StringUtil::toString( warnings ) +
                " passSide=" + StringUtil::toString( arrows ) );
    }
}  // namespace cityRoundaboutSigns
