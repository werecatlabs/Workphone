#pragma once

        namespace cityReservePlanting
        {
            void scatter( SceneAssets &assets, const SourceBudgetVector<cityReserveLandscape::Site> &sites,
                          const cityHillside::Exclusions &exclusions, render::FoliageWorldData &world,
                          VehicleAppearanceQuality quality )
            {
                if( sites.empty() || world.species.size() < 8 ) return;
                const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
                if( !terrain ) return;
                const auto &placement = assets.roadAuthoring->getTerrainPlacement();
                const bool preview = quality == VehicleAppearanceQuality::Preview;
                const size_t shrubLimit = preview ? 38u : 88u, grassLimit = preview ? 226u : 524u;
                size_t shrubs = 0, grasses = 0, beds = 0;
                using cityHillside::unit;

                SourceBudgetVector<float> radii( world.species.size(), 0.f );
                for( size_t i = 0; i < world.species.size(); ++i )
                {
                    float squared = 0.f;
                    for( const auto &lod : world.species[i]->lods )
                        for( const auto &vertex : lod.positions )
                            squared = std::max( squared, vertex.x * vertex.x + vertex.z * vertex.z );
                    radii[i] = std::sqrt( squared );
                }
                struct Plant
                {
                    Vector3F position;
                    float radius;
                    render::FoliageKind kind;
                };
                SourceBudgetMap<cityHillside::Cell, SourceBudgetVector<Plant>> planted;
                for( const auto &instance : world.instances )
                    planted[cityHillside::cellFor( instance.position )].push_back( { instance.position,
                        radii[instance.species] * std::max( instance.scale.x, instance.scale.z ), world.species[instance.species]->kind } );

                size_t remaining = sites.size();
                for( const auto &site : sites )
                {
                    if( shrubs >= shrubLimit && grasses >= grassLimit ) break;
                    const size_t before = world.instances.size();
                    const auto place = [&]( Vector3F position, u64 key, u32 species, bool warm, float volume ) {
                        const bool woody = species < 6;
                        if( woody ? shrubs >= shrubLimit : grasses >= grassLimit ) return false;
                        const float preferred = ( woody ? .37f + .19f * unit( key + 1 ) :
                                                         .42f + .29f * unit( key + 1 ) ) * volume;
                        const float minimum = woody ? .24f : .28f;
                        const float padding = woody ? .16f : .14f;
                        const auto fits = [&]( float size ) {
                            const float margin = size * radii[species] + padding;
                            return cityReserveLandscape::contains( site, position, margin ) && exclusions.clear( position, margin );
                        };
                        float size = preferred;
                        if( !fits( size ) )
                        {
                            size = std::max( minimum, preferred * .74f );
                            if( !fits( size ) )
                            {
                                size = minimum;
                                if( !fits( size ) ) return false;
                            }
                        }
                        const float radius = size * radii[species];
                        const Vector3F search( 6.f, 0, 6.f );
                        if( !cityHillside::visitCells( position - search, position + search, [&]( const auto &cell ) {
                                const auto found = planted.find( cell );
                                if( found == planted.end() ) return true;
                                for( const auto &plant : found->second )
                                {
                                    float separation = .51f * ( radius + plant.radius );
                                    if( woody && plant.kind == render::FoliageKind::Bush ) separation *= 1.27f;
                                    if( plant.kind == render::FoliageKind::Tree ) separation = radius + .48f;
                                    if( cityHillside::segmentDistance( position, plant.position, plant.position ) < separation * separation )
                                        return false;
                                }
                                return true;
                            } ) ) return false;
                        const float footing = std::max( .18f, radius * .53f );
                        if( !render::sampleTerrainHeight( *terrain, placement, position, position.y ) ||
                            !cityHillside::uninterruptedFooting( *terrain, placement, position, footing ) ) return false;
                        for( const auto &offset : { site.along, -site.along, site.outward, -site.outward } )
                        {
                            float height = 0.f;
                            if( !render::sampleTerrainHeight( *terrain, placement, position + offset * footing, height ) ||
                                std::abs( height - position.y ) > footing * .58f ) return false;
                        }

                        render::FoliageInstance instance;
                        instance.id = key;
                        instance.seed = u32( key );
                        instance.species = species;
                        instance.position = position - Vector3F( 0, woody ? .057f : .038f, 0 );
                        instance.rotation = QuaternionF::eulerDegrees( 0, 360.f * unit( key + 4 ), 0 );
                        const float height = ( woody ? .37f + .19f * unit( key + 5 ) : .53f + .33f * unit( key + 5 ) ) * volume;
                        instance.scale = { size, std::min( height, woody && site.corner ? .46f : .98f ), size };
                        const float light = .99f + .17f * unit( key + 6 );
                        instance.tint = warm ? ColourF{ 1.22f * light, 1.07f * light, .97f * light, 1 } :
                                               ColourF{ .99f * light, 1.10f * light, 1.03f * light, 1 };
                        world.instances.push_back( instance );
                        planted[cityHillside::cellFor( position )].push_back( { position, radius, world.species[species]->kind } );
                        if( woody ) ++shrubs;
                        else ++grasses;
                        return true;
                    };

                    const size_t shrubStart = shrubs, grassStart = grasses;
                    const u32 driftCount = site.halfLength > 3.6f && site.halfDepth > 1.65f ? 3u : 2u;
                    const auto share = [remaining]( size_t budget, size_t maximum, size_t minimum ) {
                        // Leave a core planting budget for later beds after the corner sites.
                        const size_t reserved = std::min( budget, minimum * ( remaining - 1u ) );
                        return std::min( maximum, std::max( ( budget + remaining - 1u ) / remaining, budget - reserved ) );
                    };
                    const size_t shrubQuota = share( shrubLimit - shrubs, driftCount + 1u, 1u );
                    const size_t grassQuota = share( grassLimit - grasses, driftCount * ( preview ? 6u : 10u ), preview ? 4u : 5u );
                    --remaining;
                    const u64 bedKey = render::foliageStableHash( site.key ^ 0x7265736572766570ull );
                    for( u32 drift = 0; drift < driftCount; ++drift )
                    {
                        const u64 driftKey = bedKey + ( drift + 1u ) * 127u;
                        // Keep the walks open to view. Taller drifts sit back on the soft bed,
                        // with lower material spilling toward the walk in irregular groups.
                        const float axis = ( ( drift + .5f ) / float( driftCount ) - .5f ) * 1.39f +
                                           .19f * ( unit( driftKey + 1 ) - .5f );
                        const float back = .15f + .34f * unit( driftKey + 2 );
                        const float span = .22f + .14f * unit( driftKey + 3 );
                        const bool warm = unit( driftKey + 4 ) < .46f;
                        const bool woodEdge = site.halfDepth >= 1.03f && ( !site.corner || drift == 1u );
                        u32 woody = 0, grass = 0;
                        const size_t shrubGoal = woodEdge ? std::min<size_t>( shrubQuota - ( shrubs - shrubStart ),
                            driftCount == 3u && drift == 1u ? 2u : 1u ) : 0u;
                        const size_t driftsLeft = driftCount - drift;
                        const size_t grassGoal = ( grassQuota - ( grasses - grassStart ) + driftsLeft - 1u ) / driftsLeft;
                        for( u32 probe = 0; probe < 43 && ( woody < shrubGoal || grass < grassGoal ); ++probe )
                        {
                            const auto raw = render::foliageStableHash( driftKey + probe + 11u );
                            const auto key = raw ? raw : driftKey + probe + 11u;
                            const bool shrub = probe < 8u;
                            if( shrub ? woody >= shrubGoal : grass >= grassGoal ) continue;
                            const float angle = 2.f * pi * unit( key + 11 );
                            const float reach = probe == 0 ? 0.f : std::sqrt( unit( key + 12 ) );
                            const float lateral = axis + span * std::cos( angle ) * reach;
                            const float depth = ( shrub ? back : back - .27f ) +
                                                ( shrub ? .30f : .66f ) * std::sin( angle ) * reach;
                            const auto position = site.centre + site.along * ( site.halfLength * lateral ) +
                                                  site.outward * ( site.halfDepth * depth );
                            const bool low = !shrub && ( depth < -.16f || unit( key + 13 ) < .45f );
                            const u32 species = shrub ? ( site.corner ? 5u : 4u + u32( key & 1ull ) ) : ( low ? 6u : 7u );
                            const float volume = low ? .85f + .14f * unit( key + 14 ) : .98f + .18f * unit( key + 14 );
                            if( !place( position, key, species, warm, volume ) ) continue;
                            if( shrub ) ++woody;
                            else ++grass;
                        }
                    }
                    if( world.instances.size() != before ) ++beds;
                }
                WP_LOG( String( "Terrain city reserve foliage beds=" ) + StringUtil::toString( beds ) +
                        " shrubs=" + StringUtil::toString( shrubs ) + " grass=" + StringUtil::toString( grasses ) );
            }
        }  // namespace cityReservePlanting
