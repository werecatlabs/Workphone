#pragma once

namespace cityGroundContact
{
    class FrontagePlanting
    {
        struct Site
        {
            Vector3F centre, along, outward, low, high;
            float halfSpan, halfDepth;
            u64 key;
            bool formal, court = false, streetCourt = false, blocked = false;
        };

    public:
        explicit FrontagePlanting( int detail ) :
            m_limit( ( 112u * std::clamp( detail, 0, 100 ) + 99 ) / 100 )
        {
        }

        void reserveAccess( Vector3F a, Vector3F b )
        {
            m_access.push_back( { { std::min( a.x, b.x ), 0, std::min( a.z, b.z ) },
                                  { std::max( a.x, b.x ), 0, std::max( a.z, b.z ) } } );
        }

        template <class Reach, class GroundClear>
        void offer( const OpenCityLayout::Lot &lot, Vector3F door, Vector3F along, Vector3F outward,
                    float pathHalf, float facadeLeft, float facadeRight, float setbackDepth,
                    float drainDepth, const Reach &reach, const GroundClear &groundClear )
        {
            if( !m_limit || m_sites.size() >= 352 )
            {
                return;
            }
            const bool formal = lot.district != OpenCityLayout::District::Residential;
            const float start = std::max( setbackDepth + .26f, drainDepth + .24f );
            for( const int side : { -1, 1 } )
            {
                if( m_sites.size() >= 352 )
                {
                    break;
                }
                const u64 key =
                    render::foliageStableHash( lot.id ^ ( side > 0 ? 0xc517ac23u : 0x69318be9u ) );
                const float space = ( side < 0 ? -facadeLeft : facadeRight ) - pathHalf - .63f;
                if( space < 1.22f )
                {
                    continue;
                }
                const float halfSpan =
                    std::min( space * .5f, ( formal ? 1.42f : .95f ) + .29f * random( key ) );
                const float offset = side * ( pathHalf + .34f + halfSpan +
                                              .55f * ( space - 2.f * halfSpan ) * random( key + 1 ) );
                const auto axis = door + along * offset;
                const float walk =
                    std::min( { reach( axis - along * ( halfSpan + .20f ) ), reach( axis ),
                                reach( axis + along * ( halfSpan + .20f ) ) } );
                const float room = walk - .33f - start;
                if( room < .88f )
                {
                    continue;
                }
                const float halfDepth = std::min( room * .5f, .54f + .20f * random( key + 2 ) );
                const auto centre = axis + outward * ( start + halfDepth +
                                                       ( room - halfDepth * 2.f ) *
                                                           ( .25f + .52f * random( key + 3 ) ) );
                addSite( centre, along, outward, halfSpan, halfDepth, key, formal, false, false,
                         groundClear );

                if( room < 5.2f || space < 3.6f )
                {
                    continue;
                }
                const float courtSpan = std::min( 2.24f, space * .42f );
                const auto courtAxis = door + along * ( side * ( pathHalf + .52f + space * .5f ) );
                const float front =
                    std::min( { reach( courtAxis - along * ( courtSpan + .23f ) ), reach( courtAxis ),
                                reach( courtAxis + along * ( courtSpan + .23f ) ) } );
                const float back = ( centre - door ).dotProduct( outward ) + halfDepth + .53f;
                const float depth = std::min( 1.04f, ( front - .47f - back ) * .5f );
                if( depth < .73f )
                {
                    continue;
                }
                const auto court = courtAxis + outward * ( front - .47f - depth );
                if( m_sites.size() < 352 )
                {
                    addSite( court, along, outward, courtSpan, depth, key + 31, false, true, false,
                             groundClear );
                }
            }
        }

        template <class Curb, class GroundClear>
        void offerStreetCourt( Vector3F point, Vector3F along, Vector3F outward, float roadHalf, u64 key,
                               const Curb &curb, const GroundClear &groundClear )
        {
            if( !m_limit || m_sites.size() == 384 )
            {
                return;
            }
            const float span = 1.95f + .33f * random( key );
            const float depth = .85f + .17f * random( key + 1 );
            const float a = curb( point - along * ( span + .22f ), outward );
            const float b = curb( point, outward );
            const float c = curb( point + along * ( span + .22f ), outward );
            const float inside = std::min( { a, b, c } ), outside = std::max( { a, b, c } );
            if( inside < roadHalf + 1.58f || outside > roadHalf + 3.45f || outside - inside > .52f )
            {
                return;
            }
            const auto centre = point + outward * ( outside + .47f + depth );
            addSite( centre, along, outward, span, depth, key, false, true, true, groundClear );
        }

        void publish( SceneAssets &assets, const CityRoadTerrainMesh &walks,
                      const render::TerrainData &terrain, const Transform3<real_Num> &placement,
                      SmartPtr<ITextureForge> forge )
        {
            if( m_sites.empty() )
            {
                return;
            }
            // Index the small set of candidate footprints. Check conservative
            // boxes of the actual fitted tops, including rounded junction walks.
            using Cell = std::pair<s32, s32>;
            const auto cell = []( float value ) { return s32( std::floor( value / 8.f ) ); };
            SourceBudgetMap<Cell, SourceBudgetVector<size_t>> candidates;
            for( size_t i = 0; i < m_sites.size(); ++i )
            {
                const auto &site = m_sites[i];
                for( s32 x = cell( site.low.x ); x <= cell( site.high.x ); ++x )
                {
                    for( s32 z = cell( site.low.z ); z <= cell( site.high.z ); ++z )
                    {
                        candidates[{ x, z }].push_back( i );
                    }
                }
            }
            for( size_t i = 0; i + 2 < walks.indices.size(); i += 3 )
            {
                const auto &a = walks.vertices[walks.indices[i]];
                const auto &b = walks.vertices[walks.indices[i + 1]];
                const auto &c = walks.vertices[walks.indices[i + 2]];
                if( a.normal.y <= .1f || b.normal.y <= .1f || c.normal.y <= .1f )
                {
                    continue;
                }
                const Vector3F low( std::min( { a.position.x, b.position.x, c.position.x } ), 0,
                                    std::min( { a.position.z, b.position.z, c.position.z } ) );
                const Vector3F high( std::max( { a.position.x, b.position.x, c.position.x } ), 0,
                                     std::max( { a.position.z, b.position.z, c.position.z } ) );
                const auto endX = cell( high.x ), endZ = cell( high.z ), startZ = cell( low.z );
                for( auto found =
                         candidates.lower_bound( { cell( low.x ), std::numeric_limits<s32>::min() } );
                     found != candidates.end() && found->first.first <= endX; ++found )
                {
                    if( found->first.second >= startZ && found->first.second <= endZ )
                    {
                        for( const auto index : found->second )
                        {
                            auto &site = m_sites[index];
                            site.blocked = site.blocked || overlaps( site, low, high );
                        }
                    }
                }
            }
            std::sort( m_sites.begin(), m_sites.end(), []( const auto &a, const auto &b ) {
                if( a.streetCourt != b.streetCourt )
                {
                    return a.streetCourt;
                }
                const float p = a.centre.x * a.centre.x + a.centre.z * a.centre.z;
                const float q = b.centre.x * b.centre.x + b.centre.z * b.centre.z;
                return std::tie( p, a.key ) < std::tie( q, b.key );
            } );
            Geometry mulch, edging, evergreen, perennials, leafTips, flowers;
            SourceBudgetVector<Site> accepted;
            size_t beds = 0, courts = 0;
            const size_t courtLimit = ( m_limit + 6 ) / 7;
            for( const auto &site : m_sites )
            {
                if( site.court ? courts >= courtLimit : beds >= m_limit )
                {
                    continue;
                }
                if( site.blocked ||
                    std::any_of(
                        accepted.begin(), accepted.end(),
                        [&]( const auto &prior ) { return overlaps( site, prior.low, prior.high ); } ) ||
                    std::any_of( m_access.begin(), m_access.end(),
                                 [&]( const auto &path ) {
                                     return overlaps( site, path.first, path.second );
                                 } ) ||
                    !supportedCells( site, terrain, placement ) )
                {
                    continue;
                }
                if( emit( site, terrain, placement, mulch, edging, evergreen, perennials, leafTips,
                          flowers ) )
                {
                    accepted.push_back( site );
                    if( site.court )
                    {
                        ++courts;
                    }
                    else
                    {
                        ++beds;
                    }
                }
            }
            const auto add = [&]( const char *name, const Geometry &geometry, ColourF tint ) {
                if( !geometry.indices.empty() )
                {
                    mesh( assets, name, geometry, material( assets, tint, .94f ) );
                }
            };
            add( "Terrain city frontage mulch beds", mulch, { .39f, .29f, .21f, 1 } );
            add( "Terrain city frontage stone edging", edging, { .52f, .49f, .42f, 1 } );
            if( !leafTips.indices.empty() )
            {
                SurfaceBakeParams params;
                params.size = 128;
                SurfaceBakeResult crown;
                forge->bakeFoliage( crown, params );
                const auto canopy = upload( assets, crown.albedo );
                const auto addCrown = [&]( const char *name, const Geometry &geometry, ColourF tint ) {
                    if( !geometry.indices.empty() )
                    {
                        auto surface = material( assets, tint, .91f );
                        surface->setTexture( canopy, 0 );
                        surface->setUVProjection( 3 );
                        surface->setTriplanarScale( 2.8f );
                        mesh( assets, name, geometry, surface );
                    }
                };
                addCrown( "Terrain city frontage clipped evergreens", evergreen, { .67f, .67f, .68f, 1 } );
                addCrown( "Terrain city frontage perennials", perennials, { .79f, .72f, .71f, 1 } );
                auto sprigs = cityLeafSprig::surface( assets );
                mesh( assets, "Terrain city frontage leaf tips", leafTips, sprigs );
            }
            add( "Terrain city frontage flowers", flowers, { .73f, .63f, .35f, 1 } );
            WP_LOG( String( "Terrain city planted frontages=" ) +
                    StringUtil::toString( accepted.size() ) +
                    " landscaped courts=" + StringUtil::toString( courts ) );
        }

    private:
        template <class GroundClear>
        void addSite( Vector3F centre, Vector3F along, Vector3F outward, float halfSpan, float halfDepth,
                      u64 key, bool formal, bool court, bool streetCourt,
                      const GroundClear &groundClear )
        {
            if( m_sites.size() == 384 )
            {
                return;
            }
            const Vector3F margin(
                std::abs( along.x ) * halfSpan + std::abs( outward.x ) * halfDepth + .16f, 0,
                std::abs( along.z ) * halfSpan + std::abs( outward.z ) * halfDepth + .16f );
            const auto low = centre - margin, high = centre + margin;
            if( groundClear( low, high ) )
            {
                m_sites.push_back( { centre, along, outward, low, high, halfSpan, halfDepth, key, formal,
                                     court, streetCourt } );
            }
        }

        static float random( u64 key )
        {
            return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
        }

        static bool overlaps( const Site &site, Vector3F low, Vector3F high )
        {
            return low.x <= site.high.x && high.x >= site.low.x && low.z <= site.high.z &&
                   high.z >= site.low.z;
        }

        static bool supportedCells( const Site &site, const render::TerrainData &terrain,
                                    const Transform3<real_Num> &placement )
        {
            const auto inverse = placement.getOrientation().inverse();
            const auto scale = placement.getScale();
            Vector2F low( std::numeric_limits<float>::max(), std::numeric_limits<float>::max() );
            Vector2F high( -low.x, -low.y );
            for( const float x : { site.low.x, site.high.x } )
            {
                for( const float z : { site.low.z, site.high.z } )
                {
                    const auto local = inverse * ( Vector3F( x, 0, z ) - placement.getPosition() );
                    const float gx = ( local.x / scale.x - terrain.origin.x ) / terrain.spacing.x;
                    const float gz = ( local.z / scale.z - terrain.origin.y ) / terrain.spacing.y;
                    low = { std::min( low.x, gx ), std::min( low.y, gz ) };
                    high = { std::max( high.x, gx ), std::max( high.y, gz ) };
                }
            }
            if( low.x < 0 || low.y < 0 || high.x >= terrain.dimensions.x - 1 ||
                high.y >= terrain.dimensions.y - 1 || high.x - low.x > 12.f || high.y - low.y > 12.f )
            {
                return false;
            }
            for( auto z = s32( std::floor( low.y ) ); z <= s32( std::floor( high.y ) ); ++z )
            {
                for( auto x = s32( std::floor( low.x ) ); x <= s32( std::floor( high.x ) ); ++x )
                {
                    if( render::terrainCellIsHole( terrain, x, z ) )
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        static void shrub( Geometry &geometry, Vector3F centre, Vector3F along, Vector3F outward,
                           float xRadius, float zRadius, float height, u64 key )
        {
            constexpr unsigned sides = 8;
            const u32 bottom = u32( geometry.positions.size() );
            const auto vertex = [&]( Vector3F offset, Vector3F normal ) {
                geometry.positions.push_back( centre + offset );
                geometry.normals.push_back( normal.normaliseCopy() );
                geometry.uv.push_back( { 0, 0, 0 } );
                geometry.tangents.push_back( { along.x, along.y, along.z, 1 } );
            };
            vertex( { 0, -height, 0 }, { 0, -1, 0 } );
            for( unsigned ring = 0; ring < 3; ++ring )
            {
                const float pitch = ( float( ring ) - 1.f ) * ( pi * .25f );
                for( unsigned side = 0; side < sides; ++side )
                {
                    const float angle = 2.f * pi * ( side + .12f * random( key ) ) / sides;
                    const float x = std::cos( angle ) * std::cos( pitch );
                    const float z = std::sin( angle ) * std::cos( pitch );
                    const float y = std::sin( pitch );
                    const float ripple = .82f + .18f * random( key + side + ring * sides + 1 );
                    vertex( ( along * ( x * xRadius ) + outward * ( z * zRadius ) ) * ripple +
                                Vector3F( 0, y * height, 0 ),
                            along * ( x / xRadius ) + outward * ( z / zRadius ) +
                                Vector3F( 0, y / height, 0 ) );
                }
            }
            const u32 top = u32( geometry.positions.size() );
            vertex( { 0, height, 0 }, { 0, 1, 0 } );
            const auto triangle = [&]( u32 a, u32 b, u32 c ) {
                const auto face = ( geometry.positions[b] - geometry.positions[a] )
                                      .crossProduct( geometry.positions[c] - geometry.positions[a] );
                const auto normal = geometry.normals[a] + geometry.normals[b] + geometry.normals[c];
                const bool forward = face.dotProduct( normal ) >= 0;
                geometry.indices.push_back( a );
                geometry.indices.push_back( forward ? b : c );
                geometry.indices.push_back( forward ? c : b );
            };
            for( u32 i = 0; i < sides; ++i )
            {
                const u32 a = bottom + 1 + i, b = bottom + 1 + ( i + 1 ) % sides;
                triangle( bottom, b, a );
                for( const u32 ring : { 0u, sides } )
                {
                    triangle( a + ring, b + ring, b + ring + sides );
                    triangle( a + ring, b + ring + sides, a + ring + sides );
                }
                triangle( top, a + 2 * sides, b + 2 * sides );
            }
        }

        static void shrubLeaves( Geometry &tips, const Site &site, Vector3F centre,
                                 float xRadius, float zRadius, float height, u64 key )
        {
            const Vector3F up( 0, 1, 0 );
            const float phase = 2.f * pi * random( key + 48 );
            const int shoots = std::clamp( int( 60 * ( xRadius + zRadius ) ), 18, 48 );
            // Each card follows the crown from inside the foliage to a projecting tip.
            for( int i = 0; i < shoots; ++i )
            {
                const u64 shootKey = render::foliageStableHash( key + 51 + i );
                const float y = -.56f + 1.55f * ( i + .15f + .70f * random( shootKey ) ) / shoots;
                const float radius = std::sqrt( 1.f - y * y );
                const float angle = phase + i * 2.399963f + .32f * random( shootKey + 1 );
                const float x = std::cos( angle ), z = std::sin( angle );
                const auto radial = site.along * x + site.outward * z;
                const auto tangent = site.outward * x - site.along * z;
                const auto root = centre + ( ( site.along * ( x * xRadius ) +
                                               site.outward * ( z * zRadius ) ) * radius +
                                             up * ( y * height ) ) * .82f;
                const auto surfaceNormal = radial * radius + up * y;
                const auto meridian = up * radius - radial * y;
                const float heading = ( random( shootKey + 2 ) - .5f ) * 2.8f;
                const auto stem = ( meridian * std::cos( heading ) + tangent * std::sin( heading ) +
                                    surfaceNormal * .46f ).normaliseCopy();
                const float length = .091f + .067f * random( shootKey + 3 );
                const auto nearSide = surfaceNormal.crossProduct( stem ).normaliseCopy() * ( length * .44f );
                const auto normal = ( surfaceNormal + up * .34f ).normaliseCopy();
                const auto tip = root + stem * length;
                tips.quad( root - nearSide, root + nearSide, tip + nearSide, tip - nearSide, normal );
            }
        }

        static bool emit( const Site &site, const render::TerrainData &terrain,
                          const Transform3<real_Num> &placement, Geometry &mulch, Geometry &edging,
                          Geometry &evergreen, Geometry &perennials, Geometry &leafTips, Geometry &flowers )
        {
            const Vector3F up( 0, 1, 0 );
            const auto surface = [&]( Vector3F &p ) {
                return render::sampleTerrainHeight( terrain, placement, p, p.y );
            };
            Vector3F centre = site.centre;
            if( !surface( centre ) )
            {
                return false;
            }
            const Vector2F chamfer[] = { { -.77f, -1 }, { .77f, -1 }, { 1, -.63f }, { 1, .63f },
                                         { .77f, 1 },   { -.77f, 1 }, { -1, .63f }, { -1, -.63f } };
            const unsigned count = site.formal ? 8 : 12;
            std::array<Vector3F, 12> outside, inside;
            for( unsigned i = 0; i < count; ++i )
            {
                const float angle = float( i ) * 2.f * pi / count;
                const float silhouette = site.court ? .93f + .07f * std::cos( angle * 3.f ) : 1.f;
                const auto shape = site.formal
                                       ? chamfer[i]
                                       : Vector2F( std::cos( angle ), std::sin( angle ) ) * silhouette;
                const auto offset = site.along * ( shape.x * site.halfSpan ) +
                                    site.outward * ( shape.y * site.halfDepth );
                outside[i] = centre + offset;
                inside[i] = centre + offset * .85f;
                if( !surface( outside[i] ) || !surface( inside[i] ) ||
                    std::abs( outside[i].y - centre.y ) > .36f )
                {
                    return false;
                }
                outside[i].y += .031f;
                inside[i].y += .067f;
            }
            for( unsigned i = 0; i < count; ++i )
            {
                const auto j = ( i + 1 ) % count;
                for( auto probe :
                     { ( centre + inside[i] + inside[j] ) / 3.f, ( outside[i] + outside[j] ) * .5f } )
                {
                    const float interpolated = probe.y;
                    if( !surface( probe ) || probe.y > interpolated + .014f ||
                        probe.y < interpolated - .12f )
                    {
                        return false;
                    }
                }
            }
            for( unsigned i = 0; i < count; ++i )
            {
                const auto j = ( i + 1 ) % count;
                edging.quad( outside[i], outside[j], inside[j], inside[i], up );
                if( i % 2 == 0 )
                {
                    mulch.quad( centre + up * .058f, inside[i], inside[j], inside[( i + 2 ) % count],
                                up );
                }
            }
            const unsigned plants = site.court ? 5 : site.halfSpan > 1.f ? 3 : 2;
            for( unsigned i = 0; i < plants; ++i )
            {
                const u64 key = site.key + i * 37u;
                const float side = 2.f * i / ( plants - 1 ) - 1.f;
                auto plant = centre + site.along * ( site.halfSpan * side * .53f ) +
                             site.outward * ( site.halfDepth * ( random( key + 4 ) - .5f ) *
                                              ( site.court ? .73f : .24f ) );
                if( !surface( plant ) )
                {
                    continue;
                }
                const float height = site.court
                                         ? .29f + .16f * random( key + 5 ) - .10f * std::abs( side )
                                     : site.formal ? .23f + .046f * random( key + 5 )
                                                   : .17f + .080f * random( key + 5 );
                const float x = std::min( site.court ? .56f : .46f,
                                          site.halfSpan * ( .28f + .06f * random( key + 6 ) ) );
                const float z = site.halfDepth * ( .49f + .12f * random( key + 7 ) );
                plant.y += height * .78f + .06f;
                auto &mature = site.formal || i % 2 == 0 ? evergreen : perennials;
                shrub( mature, plant, site.along, site.outward, x * .87f, z * .87f, height * .88f, key );
                shrubLeaves( leafTips, site, plant, x, z, height, key );
                if( site.court )
                {
                    // Low sprays connect the mounds at the street-facing edge.
                    for( unsigned blade = 0; blade < 4; ++blade )
                    {
                        const auto direction =
                            site.along * ( ( float( blade ) - 1.5f ) * .49f ) - site.outward;
                        auto base = plant + direction * .12f;
                        auto leaf = plant + direction * ( .24f + .13f * random( key + blade + 22 ) );
                        if( !surface( base ) || !surface( leaf ) )
                        {
                            continue;
                        }
                        base.y += .07f;
                        const auto middle =
                            ( base + leaf ) * .5f + up * ( .21f + .11f * random( key + blade + 27 ) );
                        leaf.y += .13f + .12f * random( key + blade + 31 );
                        const auto bladeWidth =
                            Vector3F( -direction.z, 0, direction.x ).normaliseCopy() * .043f;
                        perennials.quad( base, middle - bladeWidth, leaf, middle + bladeWidth, up );
                    }
                }
                if( site.formal && random( site.key + 20 ) < .70f )
                {
                    continue;
                }
                for( unsigned flower = 0; flower < 3; ++flower )
                {
                    const float angle = 2.399963f * flower + 6.283185f * random( key + 10 );
                    const auto head = plant + site.along * ( std::cos( angle ) * x * .50f ) +
                                      site.outward * ( std::sin( angle ) * z * .50f ) +
                                      up * ( height * .88f );
                    const auto u = site.along * .031f, v = site.outward * .035f;
                    flowers.quad( head - u, head - v, head + u, head + v, up );
                }
            }
            return true;
        }

        size_t m_limit;
        SourceBudgetVector<Site> m_sites;
        SourceBudgetVector<std::pair<Vector3F, Vector3F>> m_access;
    };
}  // namespace cityGroundContact
