#pragma once

        namespace cityParkPlanting
        {
            struct Pocket
            {
                Vector3F centre, along, outward;
                u64 key;
                float distance;
            };

            bool insideLot( const OpenCityLayout::Lot &lot, Vector3F point, float margin )
            {
                return std::abs( point.x - lot.parcelX ) + margin < lot.parcelWidth * .5f &&
                       std::abs( point.z - lot.parcelZ ) + margin < lot.parcelDepth * .5f;
            }

            void scatter( SceneAssets &assets, const SourceBudgetVector<cityParkLandscape::Layout> &layouts,
                          const cityHillside::Exclusions &exclusions, render::FoliageWorldData &world,
                          u32 seed, VehicleAppearanceQuality quality )
            {
                if( layouts.empty() ) return;
                const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
                if( !terrain ) return;
                const auto &placement = assets.roadAuthoring->getTerrainPlacement();
                const bool preview = quality == VehicleAppearanceQuality::Preview;
                const std::array<size_t, 3> limits = preview ? std::array<size_t, 3>{ 24, 96, 224 }
                                                            : std::array<size_t, 3>{ 46, 176, 416 };
                std::array<size_t, 3> counts = {};
                std::array<float, 8> radialBounds = {};
                for( size_t i = 0; i < radialBounds.size(); ++i )
                {
                    float squared = 0;
                    for( const auto &lod : world.species[i]->lods )
                        for( const auto &vertex : lod.positions )
                            squared = std::max( squared, vertex.x * vertex.x + vertex.z * vertex.z );
                    radialBounds[i] = std::sqrt( squared );
                }
                size_t plantedParks = 0;
                using cityHillside::unit;

                for( const auto &layout : layouts )
                {
                    if( !layout.lot || layout.paths.empty() ) continue;
                    if( counts[0] >= limits[0] && counts[1] >= limits[1] && counts[2] >= limits[2] ) break;
                    const auto &lot = *layout.lot;
                    const auto site = render::foliageStableHash( lot.id ^ ( u64( seed ) << 32 ) ^ 0x7061726b706c616eull );
                    SourceBudgetVector<Pocket> pockets;
                    const size_t stride = ( layout.paths.size() + 63 ) / 64;
                    for( size_t i = 0; i < layout.paths.size(); i += stride )
                    {
                        const auto &path = layout.paths[i];
                        auto direction = path.b - path.a;
                        direction.y = 0;
                        if( direction.lengthSquared() < .04f ) continue;
                        direction.normalise();
                        const Vector3F across( -direction.z, 0, direction.x );
                        for( const int side : { -1, 1 } )
                        {
                            const u64 key = render::foliageStableHash( site + 7u * i + ( side > 0 ? 2u : 5u ) );
                            const auto normal = across * float( side );
                            const auto station = path.a + ( path.b - path.a ) * ( .30f + .40f * unit( key + 1 ) );
                            for( const float retreat : { .52f, 1.14f, 1.91f, 2.86f } )
                            {
                                const auto centre = station + normal * ( path.halfWidth + retreat + .18f * unit( key + 2 ) );
                                const float distance = std::sqrt( cityHillside::segmentDistance( centre, layout.entry, layout.entry ) );
                                if( distance < 2.3f || !insideLot( lot, centre, .32f ) ||
                                    !cityParkLandscape::clearOfPaths( layout, centre, .32f ) ||
                                    !exclusions.clear( centre, .32f, true ) ) continue;
                                pockets.push_back( { centre, direction, normal, key, distance } );
                            }
                        }
                    }

                    auto intoPark = layout.centre - layout.entry;
                    intoPark.y = 0;
                    const float entryDistance = intoPark.length();
                    if( entryDistance > .1f )
                    {
                        intoPark /= entryDistance;
                        const Vector3F sideways( -intoPark.z, 0, intoPark.x );
                        for( u32 i = 0; i < 12; ++i )
                        {
                            const auto key = render::foliageStableHash( site ^ ( 0x6300u + i ) );
                            const float angle = ( i + unit( site + 21 ) ) * ( 2.f * pi / 12.f );
                            const auto outward = sideways * std::cos( angle ) + intoPark * std::sin( angle );
                            const auto centre = layout.centre + outward * ( .78f + .91f * unit( key + 1 ) );
                            const float distance = std::sqrt( cityHillside::segmentDistance( centre, layout.entry, layout.entry ) );
                            if( insideLot( lot, centre, .32f ) && cityParkLandscape::clearOfPaths( layout, centre, .32f ) )
                                pockets.push_back( { centre, Vector3F( -outward.z, 0, outward.x ), outward, key, distance } );
                        }
                    }

                    struct Planted { Vector3F position; float crown; u32 tier; };
                    SourceBudgetVector<Planted> plants;
                    const auto place = [&]( Vector3F position, u64 candidate, u32 tier ) {
                        if( counts[tier] >= limits[tier] ) return false;
                        const u32 speciesIndex = tier == 0 ? u32( candidate & 3u ) : 2 + tier * 2 + u32( candidate & 1u );
                        const float preferred = tier == 0 ? .42f + .19f * unit( candidate + 1 ) :
                                                tier == 1 ? .39f + .24f * unit( candidate + 1 ) : .42f + .35f * unit( candidate + 1 );
                        const float minimum = tier == 0 ? .28f : tier == 1 ? .25f : .29f;
                        const float radius = radialBounds[speciesIndex];
                        const float gateDistance = cityHillside::segmentDistance( position, layout.entry, layout.entry );
                        const auto fits = [&]( float size ) {
                            const float crown = size * radius;
                            const float margin = crown + ( tier == 0 ? .25f : .13f );
                            if( !insideLot( lot, position, margin ) ||
                                !cityParkLandscape::clearOfPaths( layout, position, margin ) ||
                                !exclusions.clear( position, margin, true ) ||
                                gateDistance < ( margin + 1.65f ) * ( margin + 1.65f ) ) return false;
                            for( const auto &plant : plants )
                            {
                                float separation = .53f * ( plant.crown + crown );
                                if( ( plant.tier == 0 ) != ( tier == 0 ) )
                                    separation = .52f * ( tier == 0 ? plant.crown : crown ) + .29f;
                                if( cityHillside::segmentDistance( position, plant.position, plant.position ) < separation * separation )
                                    return false;
                            }
                            return true;
                        };
                        float size = preferred;
                        if( !fits( size ) )
                        {
                            size = minimum;
                            if( !fits( size ) ) return false;
                            float upper = preferred;
                            for( int step = 0; step < 4; ++step )
                            {
                                const float middle = ( size + upper ) * .5f;
                                if( fits( middle ) ) size = middle;
                                else upper = middle;
                            }
                        }
                        const float crown = size * radius;
                        const float footing = std::max( .18f, crown * ( tier == 0 ? .27f : .43f ) );
                        if( !render::sampleTerrainHeight( *terrain, placement, position, position.y ) ||
                            !cityHillside::uninterruptedFooting( *terrain, placement, position, footing ) ) return false;
                        for( const auto &offset : { Vector3F( footing, 0, 0 ), Vector3F( -footing, 0, 0 ),
                                                    Vector3F( 0, 0, footing ), Vector3F( 0, 0, -footing ) } )
                        {
                            float height = 0;
                            if( !render::sampleTerrainHeight( *terrain, placement, position + offset, height ) ||
                                std::abs( height - position.y ) > ( tier == 0 ? .45f : .55f ) * footing ) return false;
                        }

                        render::FoliageInstance instance;
                        instance.id = candidate;
                        instance.species = speciesIndex;
                        instance.seed = u32( candidate );
                        instance.position = position - Vector3F( 0, tier == 2 ? .035f : .065f, 0 );
                        instance.rotation = QuaternionF::eulerDegrees( 0, 360.f * unit( candidate + 2 ), 0 );
                        const float vertical = std::max( size, tier == 0 ? .38f : tier == 1 ? .36f : .32f ) *
                                               ( .91f + .22f * unit( candidate + 3 ) );
                        instance.scale = { size, vertical, size };
                        const float tint = .88f + .12f * unit( candidate + 4 );
                        const float dry = tier == 2 ? .80f + .20f * unit( candidate + 5 ) : .94f + .06f * unit( candidate + 5 );
                        instance.tint = { tint, tint * ( .94f + .06f * dry ), tint * dry, 1 };
                        world.instances.push_back( instance );
                        plants.push_back( { position, crown, tier } );
                        ++counts[tier];
                        if( tier == 0 && assets.useFoliageWorld )
                        {
                            const float trunk = ( 6 + speciesIndex ) * vertical * .6f;
                            staticBox( assets, "Foliage world trunk", position + Vector3F( 0, trunk * .5f, 0 ),
                                       { .24f * size, trunk, .24f * size } );
                        }
                        return true;
                    };

                    const float parkSpan = std::max( lot.parcelWidth, lot.parcelDepth );
                    if( std::min( lot.parcelWidth, lot.parcelDepth ) >= 8.8f )
                    {
                        const bool spacious = parkSpan >= 21.f && std::min( lot.parcelWidth, lot.parcelDepth ) >= 13.5f;
                        Vector3F firstTree = Vector3F::zero();
                        for( u32 tree = 0; tree < ( spacious ? 2u : 1u ); ++tree )
                        {
                            if( counts[0] >= limits[0] ) break;
                            constexpr u64 canopy = 0x62726f61646c6561ull;
                            const auto hashed = render::foliageStableHash( site ^ ( canopy + tree ) );
                            const auto key = hashed ? hashed : canopy + tree;
                            const auto canopyAt = [&]( Vector3F position ) {
                                if( tree > 0 && cityHillside::segmentDistance( position, firstTree, firstTree ) < 49.f )
                                    return false;
                                if( !place( position, key, 0 ) ) return false;
                                firstTree = position;
                                return true;
                            };
                            bool accepted = false;
                            if( tree == 0 )
                            {
                                // Reserve one offset place within the lawn before filling the understory.
                                for( u32 probe = 0; probe < 10 && !accepted; ++probe )
                                {
                                    const float angle = ( probe + unit( key + 17 ) ) * ( 2.f * pi / 10.f );
                                    const auto position = layout.centre +
                                        Vector3F( std::cos( angle ), 0, std::sin( angle ) ) * ( .67f + .62f * unit( key + probe + 71 ) );
                                    accepted = canopyAt( position );
                                }
                                if( !accepted ) accepted = canopyAt( layout.centre );
                            }
                            u32 attempts = 0;
                            for( const auto &pocket : pockets )
                            {
                                if( accepted || attempts >= 24 ) break;
                                if( pocket.distance < 4.2f ) continue;
                                const auto position = pocket.centre + pocket.outward * ( .30f + .51f * unit( key + 41 ) );
                                if( tree > 0 && cityHillside::segmentDistance( position, firstTree, firstTree ) < 49.f ) continue;
                                accepted = canopyAt( position );
                                ++attempts;
                            }
                            if( !accepted ) break;
                        }
                    }

                    // The near group stays low at the gate. Keep open sightlines
                    // through the lawn between the planting pockets.
                    const u32 pocketCount = std::min( lot.parcelWidth, lot.parcelDepth ) < 9.5f ? 1u : 2u;
                    SourceBudgetVector<Vector3F> occupiedPockets;
                    for( u32 group = 0; group < pocketCount; ++group )
                    {
                        const float target = std::max( 3.f, entryDistance * ( group == 0 ? .79f : 1.24f ) );
                        const auto rank = [target]( const Pocket &pocket ) {
                            return std::abs( pocket.distance - target ) + .56f * unit( pocket.key + 3 );
                        };
                        std::sort( pockets.begin(), pockets.end(), [&]( const Pocket &a, const Pocket &b ) {
                            const float ra = rank( a ), rb = rank( b );
                            if( ra != rb ) return ra < rb;
                            if( a.key != b.key ) return a.key < b.key;
                            return a.centre.x != b.centre.x ? a.centre.x < b.centre.x : a.centre.z < b.centre.z;
                        } );
                        u32 attempts = 0;
                        size_t shrubs = 0, grasses = 0;
                        Vector3F anchor = Vector3F::zero();
                        for( const auto &pocket : pockets )
                        {
                            if( shrubs + grasses > 0 && cityHillside::segmentDistance( anchor, pocket.centre, pocket.centre ) > 2.1f * 2.1f )
                                continue;
                            if( std::any_of( occupiedPockets.begin(), occupiedPockets.end(), [&]( Vector3F p ) {
                                    return cityHillside::segmentDistance( p, pocket.centre, pocket.centre ) < 2.6f * 2.6f;
                                } ) ) continue;
                            const size_t first = plants.size();
                            const u64 groupKey = render::foliageStableHash( site ^ ( ( group + 1u ) * 0x9e3779b97f4a7c15ull ) );
                            const bool newAnchor = shrubs + grasses == 0;
                            for( u32 i = 0; i < 19 && ( shrubs < 2 || grasses < 6 ); ++i )
                            {
                                const auto id = 32u * attempts + i + 1u;
                                const auto hashed = render::foliageStableHash( groupKey + id );
                                const auto key = hashed ? hashed : site + id;
                                const bool shrub = i < 6;
                                if( ( shrub && shrubs >= 2 ) || ( !shrub && grasses >= 6 ) ) continue;
                                const float along = i == 0 ? 0.f : ( unit( key + 31 ) - .5f ) * ( shrub ? 2.5f : 3.3f );
                                const float across = i == 0 ? 0.f : ( unit( key + 13 ) - .5f ) * ( shrub ? 1.6f : 2.3f );
                                const auto position = pocket.centre + pocket.along * along + pocket.outward * across;
                                if( place( position, key, shrub ? 1u : 2u ) )
                                    ++( shrub ? shrubs : grasses );
                            }
                            if( newAnchor && plants.size() != first ) anchor = pocket.centre;
                            if( ++attempts >= 10 || ( shrubs >= 2 && grasses >= 6 ) ) break;
                        }
                        if( shrubs + grasses > 0 ) occupiedPockets.push_back( anchor );
                    }

                    if( !plants.empty() ) ++plantedParks;
                }
                WP_LOG( String( "Terrain city park foliage sites=" ) + StringUtil::toString( plantedParks ) +
                        " trees=" + StringUtil::toString( counts[0] ) + " shrubs=" + StringUtil::toString( counts[1] ) +
                        " grass=" + StringUtil::toString( counts[2] ) );
            }
        }  // namespace cityParkPlanting
