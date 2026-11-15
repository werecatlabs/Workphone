#pragma once

        namespace cityHillside
        {
            using Cell = std::pair<s32, s32>;

            Cell cellFor( Vector3F point )
            {
                return { s32( std::floor( point.x / 16.f ) ), s32( std::floor( point.z / 16.f ) ) };
            }

            template <class Visit>
            bool visitCells( Vector3F low, Vector3F high, const Visit &visit )
            {
                const auto first = cellFor( low ), last = cellFor( high );
                for( auto x = first.first; x <= last.first; ++x )
                {
                    for( auto z = first.second; z <= last.second; ++z )
                        if( !visit( Cell( x, z ) ) ) return false;
                }
                return true;
            }

            float unit( u64 key )
            {
                return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
            }

            float cross( Vector3F a, Vector3F b ) { return a.x * b.z - a.z * b.x; }

            float segmentDistance( Vector3F point, Vector3F a, Vector3F b )
            {
                auto direction = b - a;
                auto offset = point - a;
                direction.y = offset.y = 0;
                const float length = direction.lengthSquared();
                if( length > 0 )
                    offset -= direction * std::clamp( direction.dotProduct( offset ) / length, 0.f, 1.f );
                return offset.lengthSquared();
            }

            bool uninterruptedFooting( const render::TerrainData &ground, const Transform3<real_Num> &placement,
                                      Vector3F point, float radius )
            {
                const auto inverse = placement.getOrientation().inverse();
                const auto scale = placement.getScale();
                const auto local = inverse * ( point - placement.getPosition() );
                const float x = ( local.x / scale.x - ground.origin.x ) / ground.spacing.x;
                const float z = ( local.z / scale.z - ground.origin.y ) / ground.spacing.y;
                const float rx = radius / ( scale.x * ground.spacing.x );
                const float rz = radius / ( scale.z * ground.spacing.y );
                if( x - rx < 0 || z - rz < 0 || x + rx >= ground.dimensions.x - 1 ||
                    z + rz >= ground.dimensions.y - 1 || rx > 8 || rz > 8 ) return false;
                if( ground.holes.empty() ) return true;
                for( auto row = s32( std::floor( z - rz ) ); row <= s32( std::floor( z + rz ) ); ++row )
                    for( auto col = s32( std::floor( x - rx ) ); col <= s32( std::floor( x + rx ) ); ++col )
                        if( render::terrainCellIsHole( ground, col, row ) ) return false;
                return true;
            }

            struct Exclusions
            {
                enum class Shape { Road, Footway, Parcel };
                struct Footprint
                {
                    Shape shape;
                    Vector3F a, b, c;
                    float radius;
                };
                SourceBudgetVector<Footprint> footprints;
                SourceBudgetMap<Cell, SourceBudgetVector<u32>> index;

                void add( Shape shape, Vector3F a, Vector3F b, Vector3F c, float radius = 0 )
                {
                    const auto id = u32( footprints.size() );
                    footprints.push_back( { shape, a, b, c, radius } );
                    visitCells( { std::min( { a.x, b.x, c.x } ) - radius, 0,
                                  std::min( { a.z, b.z, c.z } ) - radius },
                                { std::max( { a.x, b.x, c.x } ) + radius, 0,
                                  std::max( { a.z, b.z, c.z } ) + radius }, [&]( const Cell &cell ) {
                        index[cell].push_back( id );
                        return true;
                    } );
                }

                bool clear( Vector3F point, float radius ) const
                {
                    const Vector3F margin( radius, 0, radius );
                    return visitCells( point - margin, point + margin, [&]( const Cell &cell ) {
                        const auto found = index.find( cell );
                        if( found == index.end() ) return true;
                        for( const auto id : found->second )
                        {
                            const auto &[shape, a, b, c, reserve] = footprints[id];
                            if( shape == Shape::Road )
                            {
                                const float width = radius + reserve;
                                if( segmentDistance( point, a, b ) < width * width ) return false;
                            }
                            else if( shape == Shape::Parcel )
                            {
                                if( point.x + radius > a.x && point.x - radius < b.x &&
                                    point.z + radius > a.z && point.z - radius < b.z ) return false;
                            }
                            else
                            {
                                const float ab = cross( b - a, point - a );
                                const float bc = cross( c - b, point - b );
                                const float ca = cross( a - c, point - c );
                                if( ( ab >= 0 && bc >= 0 && ca >= 0 ) || ( ab <= 0 && bc <= 0 && ca <= 0 ) ||
                                    std::min( { segmentDistance( point, a, b ), segmentDistance( point, b, c ),
                                                segmentDistance( point, c, a ) } ) < radius * radius ) return false;
                            }
                        }
                        return true;
                    } );
                }
            };

            void indexCity( Exclusions &exclusions, const SceneAssets &assets,
                            const CityRoadTerrainMesh &footways )
            {
                using Shape = Exclusions::Shape;
                const auto &source = assets.roadAuthoring->getSource();
                const auto &bake = assets.roadAuthoring->getBake();
                for( size_t road = 0; road < source.roads.size(); ++road )
                {
                    const auto &points = bake.roads[road].points;
                    const float half = source.roads[road].width * .5f;
                    for( size_t i = 1; i < points.size(); ++i )
                    {
                        const auto a = points[i - 1].position, b = points[i].position;
                        // The road-width capsule also encloses the shallow free-end oval.
                        exclusions.add( Shape::Road, a, b, b, half + 1.15f );
                    }
                }
                for( const auto &junction : source.roundabouts )
                {
                    const auto p = junction.center;
                    exclusions.add( Shape::Road, p, p, p, junction.outerRadius + 1.15f );
                    for( const auto &port : junction.ports )
                    {
                        auto direction = port.outward;
                        direction.y = 0;
                        direction.normalise();
                        const auto a = p + direction * junction.outerRadius;
                        const auto b = a + direction * port.length;
                        exclusions.add( Shape::Road, a, b, b, port.width * .5f + 1.15f );
                    }
                }
                for( size_t i = 0; i + 2 < footways.indices.size(); i += 3 )
                {
                    const auto &a = footways.vertices[footways.indices[i]].position;
                    const auto &b = footways.vertices[footways.indices[i + 1]].position;
                    const auto &c = footways.vertices[footways.indices[i + 2]].position;
                    if( std::abs( cross( b - a, c - a ) ) > .00001f )
                        exclusions.add( Shape::Footway, a, b, c );
                }
                for( const auto &lot : assets.cityLayout.lots )
                {
                    const Vector3F p( lot.parcelX, 0, lot.parcelZ );
                    const Vector3F extent( lot.parcelWidth * .5f + .6f, 0, lot.parcelDepth * .5f + .6f );
                    exclusions.add( Shape::Parcel, p - extent, p + extent, p );
                }
            }

            TextureBuffer grassAtlas( u32 seed )
            {
                constexpr u32 size = 128;
                TextureBuffer atlas( size, size );
                for( u32 blade = 0; blade < 17; ++blade )
                {
                    const u64 key = u64( seed ) * 31 + blade * 13;
                    const float root = .27f + .46f * unit( key );
                    const float height = .37f + .57f * unit( key + 1 );
                    const float lean = ( root - .5f ) * .7f + .36f * ( unit( key + 2 ) - .5f );
                    const float width = .010f + .014f * unit( key + 3 );
                    const float dry = unit( key + 4 );
                    for( u32 y = 2; y < size - 1; ++y )
                    {
                        const float t = ( .97f - ( y + .5f ) / size ) / height;
                        if( t < 0 || t > 1 ) continue;
                        const float x = ( root + lean * t * t ) * size;
                        const float half = width * ( 1.f - .90f * t ) * size;
                        const auto first = std::max( 1, int( x - half - 1 ) );
                        const auto last = std::min( int( size ) - 2, int( x + half + 1 ) );
                        for( int u = first; u <= last; ++u )
                        {
                            const auto alpha = u8( 255.f * std::clamp( half + .5f - std::abs( u + .5f - x ), 0.f, 1.f ) );
                            auto pixel = atlas.pixel( u32( u ), y );
                            if( alpha <= pixel[3] ) continue;
                            pixel[0] = u8( 58.f + 42.f * dry + 20.f * t );
                            pixel[1] = u8( 78.f + 23.f * dry + 18.f * t );
                            pixel[2] = u8( 30.f + 21.f * dry + 9.f * t );
                            pixel[3] = alpha;
                        }
                    }
                }
                return atlas;
            }

            void addSpecies( SceneAssets &assets, render::FoliageWorldData &world,
                             const String &leafTextureName, u32 seed )
            {
                const auto grassName = assets.resourcePrefix + "/texture/" +
                                       StringUtil::toString( assets.textures.size() );
                auto grass = upload( assets, grassAtlas( seed ), render::TextureMipFilter::Cutout );
                grass->setName( grassName );
                auto textures = core::IApplicationManager::instance()->getGraphicsSystem()->getTextureManager();
                const auto registeredGrass = textures->getByName( grassName );
                if( !registeredGrass || registeredGrass.get() != grass.get() )
                    throw std::runtime_error( "Generated hillside grass atlas lost its registered texture identity." );
                for( u32 variant = 0; variant < 4; ++variant )
                {
                    const bool shrub = variant < 2;
                    FoliagePlantRecipe recipe;
                    recipe.speciesId = assets.resourcePrefix + "/hillside/" + StringUtil::toString( variant );
                    recipe.kind = shrub ? render::FoliageKind::Bush : render::FoliageKind::Grass;
                    recipe.seed = seed ^ ( 17827u + variant * 7919u );
                    recipe.height = shrub ? ( variant == 0 ? 1.45f : .94f ) : ( variant == 2 ? .46f : .69f );
                    recipe.radius = shrub ? ( variant == 0 ? 1.21f : 1.37f ) : ( variant == 2 ? .65f : .81f );
                    recipe.trunkRadius = shrub ? .055f : .014f;
                    recipe.branches = shrub ? ( variant == 0 ? 15 : 11 ) : 5;
                    recipe.bark.tint = { .19f, .15f, .085f, 1 };
                    recipe.leaves.albedo = shrub ? leafTextureName : grassName;
                    if( variant % 2 != 0 ) recipe.leaves.tint = { 1, .90f, .74f, 1 };
                    recipe.wind.amplitude = shrub ? .07f : .12f;
                    recipe.wind.frequency = shrub ? .73f : .95f;
                    recipe.wind.flutter = .02f;
                    recipe.wind.bendStart = recipe.height * .22f;
                    recipe.wind.bendEnd = recipe.height;
                    auto species = std::make_shared<render::FoliageSpecies>();
                    String error;
                    if( !generateFoliagePlant( recipe, *species, error ) )
                        throw std::runtime_error( "Cannot generate city hillside foliage: " + error );
                    species->maximumDistance = shrub ? 420.f : 185.f;
                    species->shadowDistance = shrub ? 55.f : 0.f;
                    species->castShadows = shrub;
                    if( !shrub ) species->densityFractions = { 1, .83f, .55f };
                    for( const auto &lod : species->lods ) assets.triangles += lod.indices.size() / 3;
                    world.species.push_back( species );
                }
            }

            void scatter( SceneAssets &assets, const CityRoadTerrainMesh &footways,
                          render::FoliageWorldData &world, u32 seed, VehicleAppearanceQuality quality )
            {
                const auto ground = assets.roadAuthoring->getTerrainSnapshot();
                if( !ground || assets.roadAuthoring->getSource().roads.size() !=
                               assets.roadAuthoring->getBake().roads.size() ) return;
                const auto &placement = assets.roadAuthoring->getTerrainPlacement();
                const auto &scale = placement.getScale();
                Exclusions exclusions;
                indexCity( exclusions, assets, footways );
                struct Group
                {
                    Vector3F centre, bank;
                    u32 id;
                    float habitat, order;
                    bool approach;
                };
                SourceBudgetVector<Group> groups, approaches[8];
                const bool preview = quality == VehicleAppearanceQuality::Preview;
                const auto &source = assets.roadAuthoring->getSource();
                const auto &bake = assets.roadAuthoring->getBake();
                const float extent = assets.cityLayout.extent;
                const size_t earlierPlantCount = world.instances.size();
                const auto radius = []( Vector3F point ) {
                    return std::max( std::abs( point.x ), std::abs( point.z ) );
                };

                // Seed banks from the actual approach splines. A world-grid candidate can
                // miss miles of visible slope even when its hillside instance budget is full.
                for( u32 roadIndex = 0; roadIndex < source.roads.size(); ++roadIndex )
                {
                    const auto &road = source.roads[roadIndex];
                    const auto &samples = bake.roads[roadIndex].points;
                    if( road.closed || road.width < 10.f || samples.size() < 2 ) continue;
                    const bool fromFront = radius( samples.front().position ) > radius( samples.back().position );
                    const auto &outer = fromFront ? samples.front() : samples.back();
                    const auto &inner = fromFront ? samples.back() : samples.front();
                    const float reach = radius( outer.position );
                    const float length = samples.back().distance - samples.front().distance;
                    if( reach < extent * .77f || reach - radius( inner.position ) < 14.f || length < 19.f )
                        continue;
                    const u32 sector = u32( std::floor( ( std::atan2( outer.position.z, outer.position.x ) +
                                                         1.125f * pi ) * ( 4.f / pi ) ) ) % 8u;
                    for( u32 station = 0; station < 4; ++station )
                    {
                        const u32 id = 256u + roadIndex * 8u + station * 2u;
                        const u64 key = ( u64( seed ) << 32 ) | ( u64( id ) << 12 );
                        const float setback = 4.5f + 6.5f * unit( key + 11 ) +
                                              station * std::clamp( length * .23f, 13.f, 34.f );
                        if( setback > length - 3.f ) break;
                        const auto distance = outer.distance + ( fromFront ? setback : -setback );
                        const auto at = std::lower_bound( samples.begin(), samples.end(), distance,
                            []( const auto &point, float value ) { return point.distance < value; } );
                        if( at == samples.begin() || at == samples.end() ) continue;
                        const auto &a = *( at - 1 );
                        const float t = ( distance - a.distance ) / ( at->distance - a.distance );
                        const auto centre = a.position + ( at->position - a.position ) * t;
                        auto direction = at->position - a.position;
                        direction.y = 0;
                        direction.normalise();
                        for( u32 side = 0; side < 2; ++side )
                        {
                            const auto shoulder = Vector3F( -direction.z, 0, direction.x ) * ( side ? 1.f : -1.f );
                            const float offset = road.width * .5f + 5.7f + 9.1f * unit( key + 29 + side );
                            auto position = centre + shoulder * offset + direction * ( 5.f * unit( key + 51 + side ) - 2.5f );
                            if( !exclusions.clear( position, 2.8f ) ||
                                !render::sampleTerrainHeight( *ground, placement, position, position.y ) ) continue;
                            const float habitat = .38f + .47f * cityValueNoise(
                                seed + 173, position.x / 41.f, position.z / 46.f );
                            const float order = 3.5f * std::min( 1.15f, reach / extent ) - .24f * station +
                                                .21f * unit( key + 73 + side );
                            approaches[sector].push_back( { position, shoulder, id + side, habitat, order, true } );
                        }
                    }
                }
                constexpr u32 cells = 15;
                const float stepX = ground->spacing.x * ( ground->dimensions.x - 1 ) / cells;
                const float stepZ = ground->spacing.y * ( ground->dimensions.y - 1 ) / cells;
                for( u32 z = 0; z < cells; ++z )
                {
                    for( u32 x = 0; x < cells; ++x )
                    {
                        const u32 id = z * cells + x;
                        const u64 key = ( u64( seed ) << 32 ) | id;
                        const float px = ground->origin.x + ( x + .14f + .72f * unit( key + 991 ) ) * stepX;
                        const float pz = ground->origin.y + ( z + .14f + .72f * unit( key + 1907 ) ) * stepZ;
                        auto centre = placement.getPosition() + placement.getOrientation() *
                                      Vector3F( px * scale.x, 0, pz * scale.z );
                        if( !exclusions.clear( centre, 2.8f ) ||
                            !render::sampleTerrainHeight( *ground, placement, centre, centre.y ) ) continue;
                        const float habitat = .71f * cityValueNoise( seed + 73, px / 87.f, pz / 74.f ) +
                                              .29f * cityValueNoise( seed + 91, px / 34.f, pz / 51.f );
                        if( habitat < .30f ) continue;
                        const float fromHills = std::abs( std::max( std::abs( centre.x ), std::abs( centre.z ) ) -
                                                          assets.cityLayout.extent * .97f );
                        const float approach = 1.f - std::clamp( fromHills / 105.f, 0.f, 1.f );
                        groups.push_back( { centre, Vector3F::zero(), id, habitat,
                                            unit( key + 3001 ) + .62f * approach, false } );
                    }
                }
                const auto priority = []( const Group &a, const Group &b ) {
                    return a.order == b.order ? a.id < b.id : a.order > b.order;
                };
                std::sort( groups.begin(), groups.end(), priority );
                for( auto &sector : approaches ) std::sort( sector.begin(), sector.end(), priority );
                SourceBudgetVector<Group> ordered;
                const size_t approachLimit = preview ? 11 : 17;
                for( u32 round = 0; round < 5 && ordered.size() < approachLimit; ++round )
                {
                    for( u32 i = 0; i < 8 && ordered.size() < approachLimit; ++i )
                    {
                        auto &sector = approaches[( i + seed ) % 8u];
                        const auto next = std::find_if( sector.begin(), sector.end(), [&]( const Group &group ) {
                            return std::none_of( ordered.begin(), ordered.end(), [&]( const Group &used ) {
                                return segmentDistance( group.centre, used.centre, used.centre ) < 14.f * 14.f;
                            } );
                        } );
                        if( next == sector.end() ) continue;
                        ordered.push_back( *next );
                        sector.erase( next );
                    }
                }
                ordered.insert( ordered.end(), groups.begin(), groups.end() );

                struct Spacing { Vector3F position; float radius; };
                SourceBudgetMap<Cell, SourceBudgetVector<Spacing>> planted;
                const auto appendPlant = [&]( Vector3F position, u64 candidate, u32 speciesIndex, u32 tier,
                                              float size, float clearance, bool openBank ) {
                    const auto &species = *world.species[speciesIndex];
                    const float crown = size * std::hypot(
                        std::max( std::abs( species.boundsMinimum.x ), std::abs( species.boundsMaximum.x ) ),
                        std::max( std::abs( species.boundsMinimum.z ), std::abs( species.boundsMaximum.z ) ) ) + .20f;
                    if( !exclusions.clear( position, crown + clearance ) ||
                        !render::sampleTerrainHeight( *ground, placement, position, position.y ) ) return false;
                    const Vector3F margin( 12.f, 0, 12.f );
                    if( !visitCells( position - margin, position + margin, [&]( const Cell &cell ) {
                            const auto found = planted.find( cell );
                            if( found == planted.end() ) return true;
                            for( const auto &plant : found->second )
                            {
                                const float separation = .59f * ( crown + plant.radius );
                                if( segmentDistance( position, plant.position, plant.position ) < separation * separation )
                                    return false;
                            }
                            return true;
                        } ) ) return false;
                    const float footing = tier == 2 ? crown * .71f : std::max( .25f, .34f * crown );
                    if( openBank && !uninterruptedFooting( *ground, placement, position, footing ) ) return false;
                    for( const auto &offset : { Vector3F( footing, 0, 0 ), Vector3F( -footing, 0, 0 ),
                                                Vector3F( 0, 0, footing ), Vector3F( 0, 0, -footing ) } )
                    {
                        float height = 0;
                        if( !render::sampleTerrainHeight( *ground, placement, position + offset, height ) ||
                            std::abs( height - position.y ) >= .39f * footing ) return false;
                    }
                    planted[cellFor( position )].push_back( { position, crown } );
                    render::FoliageInstance instance;
                    instance.id = candidate;
                    instance.species = speciesIndex;
                    instance.seed = u32( candidate );
                    instance.position = position - Vector3F( 0, tier == 2 ? .045f : .075f, 0 );
                    instance.rotation = QuaternionF::eulerDegrees( 0, 360.f * unit( candidate + 3 ), 0 );
                    instance.scale = { size, size * ( .86f + .18f * unit( candidate + 4 ) ), size };
                    const float tint = .87f + .13f * unit( candidate + 5 );
                    instance.tint = { tint, tint, tint * .96f, 1 };
                    world.instances.push_back( instance );
                    return true;
                };
                const size_t limits[] = { preview ? 72u : 150u, preview ? 180u : 385u, preview ? 520u : 1120u };
                size_t counts[3] = {};
                size_t acceptedGroups = 0, acceptedBanks = 0;
                for( const auto &group : ordered )
                {
                    if( acceptedGroups >= ( preview ? 27u : 46u ) ) break;
                    const u64 key = ( u64( seed ) << 32 ) | ( u64( group.id ) << 12 );
                    const float yaw = 2.f * pi * unit( key + 1 );
                    const Vector3F along( std::cos( yaw ), 0, std::sin( yaw ) );
                    const Vector3F across( -along.z, 0, along.x );
                    const float length = group.approach ? 11.f + 11.f * unit( key + 2 ) : 15.f + 14.f * unit( key + 2 );
                    const float depth = group.approach ? 7.f + 7.f * unit( key + 3 ) : 9.f + 11.f * unit( key + 3 );
                    const auto start = world.instances.size();
                    for( u32 tier = 0; tier < 3; ++tier )
                    {
                        const unsigned attempts[] = { group.habitat > .50f ? ( group.approach ? 7u : 10u ) : 3u,
                                                      group.approach ? 25u : 23u, 52u };
                        const float spread[] = { .66f, .93f, 1.18f };
                        for( u32 i = 0; i < attempts[tier] && counts[tier] < limits[tier]; ++i )
                        {
                            const auto candidate = render::foliageStableHash( key | ( ( tier + 1u ) * 128 + i ) );
                            const float theta = 2.f * pi * unit( candidate );
                            const float reach = std::sqrt( unit( candidate + 1 ) ) * spread[tier];
                            auto position = group.centre + along * ( length * reach * std::cos( theta ) ) +
                                            across * ( depth * reach * std::sin( theta ) );
                            if( group.approach && tier == 0 ) position += group.bank * 4.f;
                            const float patch = cityValueNoise( seed + 197, position.x / 9.f, position.z / 11.f );
                            if( patch < ( group.approach ? .18f : .23f ) + .20f * reach ) continue;
                            const u32 speciesIndex = tier == 0 ? u32( candidate >> 32 ) % 4 :
                                                     4u + ( tier - 1u ) * 2u + u32( candidate & 1u );
                            const float size = .73f + .53f * unit( candidate + 2 );
                            if( appendPlant( position, candidate ? candidate : key + u64( tier * 128 + i + 1 ),
                                             speciesIndex, tier, size, tier == 2 ? 2.f : 3.f, false ) ) ++counts[tier];
                        }
                    }
                    if( world.instances.size() > start )
                    {
                        ++acceptedGroups;
                        if( group.approach ) ++acceptedBanks;
                    }
                }

                // Trees already placed in parks are not part of the hillside
                // quotas, but their crowns still participate in local spacing.
                for( size_t i = 0; i < earlierPlantCount; ++i )
                {
                    const auto &plant = world.instances[i];
                    const auto &species = *world.species[plant.species];
                    const float crown = std::hypot(
                        plant.scale.x * std::max( std::abs( species.boundsMinimum.x ), std::abs( species.boundsMaximum.x ) ),
                        plant.scale.z * std::max( std::abs( species.boundsMinimum.z ), std::abs( species.boundsMaximum.z ) ) );
                    planted[cellFor( plant.position )].push_back( { plant.position, crown } );
                }

                struct ForkArm
                {
                    Vector3F end, inward;
                    float halfWidth, length;
                    u32 id;
                };
                SourceBudgetVector<ForkArm> arms;
                for( u32 i = 0; i < source.roads.size(); ++i )
                {
                    const auto &road = source.roads[i];
                    const auto &samples = bake.roads[i].points;
                    if( road.closed || road.heightMode != CityRoadHeightMode::Drape || samples.size() < 2 ) continue;
                    const float length = samples.back().distance - samples.front().distance;
                    if( length < 27.f || road.width < 5.f ) continue;
                    for( const bool start : { true, false } )
                    {
                        const auto &end = start ? samples.front() : samples.back();
                        if( radius( end.position ) > extent * 1.025f ) continue;
                        for( size_t step = 1; step < samples.size(); ++step )
                        {
                            const auto &sample = samples[start ? step : samples.size() - 1 - step];
                            if( std::abs( sample.distance - end.distance ) < 13.f ) continue;
                            auto inward = sample.position - end.position;
                            inward.y = 0;
                            if( inward.lengthSquared() >= 4.f )
                                arms.push_back( { end.position, inward.normaliseCopy(), road.width * .5f,
                                                  length, i * 2 + ( start ? 0u : 1u ) } );
                            break;
                        }
                    }
                }
                struct Fork
                {
                    Vector3F end, along, across;
                    float front, back, bias, score;
                    u64 key;
                };
                SourceBudgetVector<Fork> forks[8];
                for( size_t i = 0; i < arms.size(); ++i )
                    for( size_t j = i + 1; j < arms.size(); ++j )
                    {
                        const auto &a = arms[i], &b = arms[j];
                        if( a.id / 2 == b.id / 2 || segmentDistance( a.end, b.end, b.end ) > .64f ) continue;
                        const float cosine = a.inward.dotProduct( b.inward );
                        if( cosine < -.10f || cosine > .85f ) continue;
                        const float front = std::max( 16.f, ( std::max( a.halfWidth, b.halfWidth ) + 4.6f ) /
                                                          std::sqrt( ( 1.f - cosine ) * .5f ) );
                        const float back = front + 9.3f;
                        if( back + 5.f > std::min( { a.length, b.length, 54.f } ) ) continue;
                        const auto end = ( a.end + b.end ) * .5f;
                        const auto along = ( a.inward + b.inward ).normaliseCopy();
                        const Vector3F across( -along.z, 0, along.x );
                        const float bias = ( b.halfWidth - a.halfWidth ) * a.inward.dotProduct( across );
                        auto centre = end + along * ( front + 3.f ) + across * bias;
                        if( !exclusions.clear( centre, 1.5f ) ||
                            !render::sampleTerrainHeight( *ground, placement, centre, centre.y ) ) continue;
                        float uphill = 0, downhill = 0;
                        if( !render::sampleTerrainHeight( *ground, placement, centre + along * 4.f, uphill ) ||
                            !render::sampleTerrainHeight( *ground, placement, centre - along * 4.f, downhill ) ||
                            std::abs( uphill - downhill ) > 2.6f ) continue;
                        const auto key = render::foliageStableHash( ( u64( seed ) << 32 ) ^ 0xf637b0940000ull ^
                                                                    ( u64( a.id ) << 12 ) ^ b.id );
                        const auto sector = u32( std::floor( ( std::atan2( end.z, end.x ) +
                                                             1.125f * pi ) * ( 4.f / pi ) ) ) % 8u;
                        if( forks[sector].size() >= 64 ) continue;
                        const float collector = std::max( a.halfWidth, b.halfWidth ) >= 4.45f &&
                                                std::min( a.halfWidth, b.halfWidth ) < 4.45f ? .40f : 0.f;
                        const float openness = exclusions.clear( end + along * back + across * bias, 4.f ) ? .48f : 0.f;
                        const float relief = std::min( std::abs( uphill - downhill ), 1.5f );
                        forks[sector].push_back( { end, along, across, front, back, bias,
                                                   openness + collector + .25f * relief + .16f * unit( key ), key } );
                    }
                for( auto &bin : forks )
                    std::sort( bin.begin(), bin.end(), []( const Fork &a, const Fork &b ) {
                        return a.score != b.score ? a.score > b.score : a.key < b.key;
                    } );
                size_t forkCursors[8] = {}, forkPlants[3] = {};
                SourceBudgetVector<Vector3F> plantedForks;
                const size_t forkLimit = preview ? 9 : 21;
                for( unsigned round = 0; round < 4 && plantedForks.size() < forkLimit; ++round )
                    for( u32 i = 0; i < 8 && plantedForks.size() < forkLimit; ++i )
                    {
                        const auto sector = ( i + seed ) % 8u;
                        auto &cursor = forkCursors[sector];
                        while( cursor < forks[sector].size() )
                        {
                            const auto &fork = forks[sector][cursor++];
                            const auto centre = fork.end + fork.along * ( fork.front + 4.f ) + fork.across * fork.bias;
                            if( std::any_of( plantedForks.begin(), plantedForks.end(), [&]( Vector3F used ) {
                                    return segmentDistance( centre, used, used ) < 13.f * 13.f;
                                } ) ) continue;
                            const auto before = world.instances.size();
                            // Put the highest crown at the back; keep the fork's
                            // opening and near-road sightline in low planting.
                            bool tree = false;
                            for( unsigned row = 0; row < 3 && !tree; ++row )
                                for( unsigned side = 0; side < 3 && !tree; ++side )
                                {
                                    const auto key = render::foliageStableHash( fork.key ^ ( 0x45c13bu + row * 3 + side ) );
                                    const float lateral = side == 0 ? 0.f : side == 1 ? -1.38f : 1.38f;
                                    const auto p = fork.end + fork.along * ( fork.back + row * 2.7f ) +
                                                   fork.across * ( fork.bias + lateral );
                                    tree = appendPlant( p, key ? key : fork.key + 1, u32( key & 3u ), 0,
                                                        .59f + .16f * unit( key + 11 ), 3.55f, true );
                                }
                            if( tree ) ++forkPlants[0];
                            for( u32 tier = 1; tier < 3; ++tier )
                            {
                                const unsigned limit = tier == 1 ? ( preview ? 4 : 6 ) : ( preview ? 7 : 11 );
                                for( unsigned trial = 0, saved = 0; trial < limit * 3 && saved < limit; ++trial )
                                {
                                    const auto key = render::foliageStableHash( fork.key + tier * 113u + trial );
                                    const float depth = tier == 1 ? 1.7f + 6.6f * unit( key + 1 )
                                                                  : -1.1f + 6.2f * unit( key + 1 );
                                    const float spread = tier == 1 ? 3.4f : 4.8f;
                                    const auto p = fork.end + fork.along * ( fork.front + depth ) +
                                                   fork.across * ( fork.bias + spread * ( unit( key + 2 ) - .5f ) );
                                    const u32 species = 4u + ( tier - 1u ) * 2u + u32( ( key >> 12 ) & 1u );
                                    const float size = tier == 1 ? .82f + .26f * unit( key + 6 )
                                                                  : .65f + .28f * unit( key + 6 );
                                    if( appendPlant( p, key ? key : fork.key + tier * 31u + trial,
                                                     species, tier, size, tier == 1 ? .81f : .47f, true ) )
                                    {
                                        ++saved;
                                        ++forkPlants[tier];
                                    }
                                }
                            }
                            if( world.instances.size() == before ) continue;
                            plantedForks.push_back( centre );
                            break;
                        }
                    }

                struct OpenBank
                {
                    Vector3F centre, along, outward;
                    u64 key = 0;
                    float score = -1.f;
                };
                SourceBudgetVector<OpenBank> openBanks[8], vergePockets[8];
                const auto offer = [&]( Vector3F station, Vector3F direction, float halfWidth,
                                        u64 key, bool pocket, bool roundabout ) {
                    if( radius( station ) > extent * 1.035f ) return;
                    for( const float side : { -1.f, 1.f } )
                    {
                        const auto outward = Vector3F( -direction.z, 0, direction.x ) * side;
                        const auto sideKey = render::foliageStableHash( key ^ ( side < 0 ? 0x6c96ac55u : 0x83b6a269u ) );
                        OpenBank best;
                        for( u32 lane = 0; lane < 3; ++lane )
                        {
                            const float offset = ( pocket ? 4.6f + 1.85f * lane : 9.2f + 4.1f * lane ) +
                                                 ( unit( sideKey + lane ) - .5f ) * ( pocket ? 1.30f : 3.4f );
                            auto centre = station + outward * ( halfWidth + offset );
                            if( !exclusions.clear( centre, pocket ? .95f : 2.6f ) ||
                                !render::sampleTerrainHeight( *ground, placement, centre, centre.y ) ) continue;
                            const float step = pocket ? 1.5f : 3.f;
                            float low = centre.y, high = centre.y;
                            bool supported = true;
                            for( const auto &offset : { direction * step, direction * -step,
                                                        outward * step, outward * -step } )
                            {
                                float height = 0;
                                if( !render::sampleTerrainHeight( *ground, placement, centre + offset, height ) )
                                {
                                    supported = false;
                                    break;
                                }
                                low = std::min( low, height );
                                high = std::max( high, height );
                            }
                            if( !supported || high - low > step * .79f ) continue;
                            const float land = exclusions.clear( centre, pocket ? 2.2f : 6.8f ) ? .62f : 0.f;
                            const float relief = std::clamp( ( high - low ) / step, 0.f, .45f );
                            const float score = land + ( pocket ? ( roundabout ? .75f : 0.f ) - .13f * lane
                                                               : 1.72f * relief + .09f * lane ) +
                                                .32f * unit( sideKey + 35 + lane );
                            if( score > best.score ) best = { centre, direction, outward, sideKey, score };
                        }
                        if( best.score == -1.f ) continue;
                        const u32 sector = u32( std::floor( ( std::atan2( best.centre.z, best.centre.x ) +
                                                             1.125f * pi ) * ( 4.f / pi ) ) ) % 8u;
                        auto &bin = pocket ? vergePockets[sector] : openBanks[sector];
                        if( bin.size() < 256 ) bin.push_back( best );
                    }
                };
                const u64 bankDomain = ( u64( seed ) << 32 ) ^ 0x48351a70df03d72bull;
                for( u32 roadIndex = 0; roadIndex < source.roads.size(); ++roadIndex )
                {
                    const auto &road = source.roads[roadIndex];
                    const auto &samples = bake.roads[roadIndex].points;
                    if( samples.size() < 2 || road.heightMode != CityRoadHeightMode::Drape ) continue;
                    const float first = samples.front().distance, length = samples.back().distance - first;
                    if( length < 14.f ) continue;
                    const unsigned stations = std::clamp( unsigned( std::ceil( length / 24.f ) ), 1u, 12u );
                    for( unsigned station = 0; station < stations; ++station )
                    {
                        const u64 key = render::foliageStableHash( bankDomain ^ ( u64( roadIndex ) << 9 ) ^ station );
                        const float distance = first + length * ( ( station + .26f + .48f * unit( key ) ) / stations );
                        const auto b = std::lower_bound( samples.begin(), samples.end(), distance,
                            []( const auto &sample, float value ) { return sample.distance < value; } );
                        if( b == samples.begin() || b == samples.end() ) continue;
                        const auto &a = *( b - 1 );
                        const float span = b->distance - a.distance;
                        if( span <= 0 ) continue;
                        const auto centre = a.position + ( b->position - a.position ) * ( ( distance - a.distance ) / span );
                        const auto direction = Vector3F( b->position.x - a.position.x, 0,
                                                         b->position.z - a.position.z ).normaliseCopy();
                        if( direction.lengthSquared() < .01f ) continue;
                        offer( centre, direction, road.width * .5f, key, false, false );
                        if( station % 2 == 0 ) offer( centre, direction, road.width * .5f, key ^ 0x97b26u, true, false );
                    }
                }
                u32 portIndex = 0;
                for( const auto &junction : source.roundabouts )
                    for( const auto &port : junction.ports )
                    {
                        const auto direction = Vector3F( port.outward.x, 0, port.outward.z ).normaliseCopy();
                        const u64 key = render::foliageStableHash( bankDomain ^ ( 0xe71b813f0000ull + ++portIndex ) );
                        if( port.length < 10.f ) continue;
                        for( const float station : { .42f, .69f } )
                            offer( junction.center + direction * ( junction.outerRadius + port.length * station ),
                                   direction, port.width * .5f, key + u64( station * 300.f ), true, true );
                    }

                size_t newGroups[2] = {}, additional[3] = {};
                for( const bool pocket : { false, true } )
                {
                    auto &sectors = pocket ? vergePockets : openBanks;
                    for( auto &sector : sectors )
                        std::sort( sector.begin(), sector.end(), []( const OpenBank &a, const OpenBank &b ) {
                            return a.score != b.score ? a.score > b.score : a.key < b.key;
                        } );
                    const size_t limit = pocket ? ( preview ? 9u : 19u ) : ( preview ? 12u : 25u );
                    const size_t budgets[] = { pocket ? 0u : ( preview ? 16u : 36u ),
                                                pocket ? ( preview ? 30u : 64u ) : ( preview ? 64u : 141u ),
                                                pocket ? ( preview ? 67u : 148u ) : ( preview ? 135u : 281u ) };
                    size_t cursors[8] = {}, used[3] = {};
                    SourceBudgetVector<Vector3F> occupied;
                    const float separation = pocket ? 10.f : 18.f;
                    for( unsigned round = 0; round < 4 && occupied.size() < limit; ++round )
                        for( u32 i = 0; i < 8 && occupied.size() < limit; ++i )
                        {
                            const auto sector = ( i + seed ) % 8u;
                            auto &cursor = cursors[sector];
                            const auto &bank = sectors[sector];
                            while( cursor < bank.size() )
                            {
                                const auto &group = bank[cursor++];
                                if( std::any_of( occupied.begin(), occupied.end(), [&]( Vector3F centre ) {
                                        return segmentDistance( group.centre, centre, centre ) < separation * separation;
                                    } ) ) continue;
                                const size_t start = world.instances.size();
                                const float length = pocket ? 4.f + 2.f * unit( group.key + 55 )
                                                            : 7.f + 5.f * unit( group.key + 55 );
                                const float depth = pocket ? 1.55f + 1.2f * unit( group.key + 91 )
                                                           : 3.4f + 3.2f * unit( group.key + 91 );
                                for( u32 tier = pocket ? 1u : 0u; tier < 3; ++tier )
                                {
                                    const unsigned trials[] = { 4, pocket ? 9u : 14u, pocket ? 15u : 23u };
                                    for( unsigned item = 0; item < trials[tier] && used[tier] < budgets[tier]; ++item )
                                    {
                                        const u64 key = render::foliageStableHash( group.key + 217u * ( tier + 1u ) + item );
                                        const float turn = 2.f * pi * unit( key );
                                        const float spread = std::sqrt( unit( key + 1 ) );
                                        const float offset = tier == 0 ? 2.65f : tier == 2 ? -.79f : .2f;
                                        const auto position = group.centre + group.along * ( length * spread * std::cos( turn ) ) +
                                            group.outward * ( depth * spread * std::sin( turn ) + offset );
                                        if( tier == 0 && std::any_of( source.roundabouts.begin(), source.roundabouts.end(),
                                                [&]( const auto &junction ) {
                                                    const float clear = junction.outerRadius + 14.f;
                                                    return segmentDistance( position, junction.center, junction.center ) < clear * clear;
                                                } ) ) continue;
                                        const u32 species = tier == 0 ? u32( ( key >> 32 ) & 3u )
                                                                     : 4 + ( tier - 1 ) * 2 + u32( key & 1u );
                                        const float size = tier == 0 ? .56f + .30f * unit( key + 2 )
                                            : pocket ? .52f + .26f * unit( key + 2 ) : .71f + .38f * unit( key + 2 );
                                        const float clearance = tier == 0 ? 4.8f : tier == 1 ? 1.05f : .46f;
                                        if( appendPlant( position, key ? key : group.key + item + 1,
                                                         species, tier, size, clearance, true ) ) ++used[tier];
                                    }
                                }
                                if( world.instances.size() == start ) continue;
                                occupied.push_back( group.centre );
                                ++newGroups[pocket ? 1 : 0];
                                break;
                            }
                        }
                    for( size_t tier = 0; tier < 3; ++tier ) additional[tier] += used[tier];
                }
                WP_LOG( String( "Terrain city hillside foliage groups=" ) + StringUtil::toString( acceptedGroups ) +
                        " trees=" + StringUtil::toString( counts[0] + additional[0] + forkPlants[0] ) +
                        " shrubs=" + StringUtil::toString( counts[1] + additional[1] + forkPlants[1] ) +
                        " grass=" + StringUtil::toString( counts[2] + additional[2] + forkPlants[2] ) +
                        " approachGroups=" + StringUtil::toString( acceptedBanks ) +
                        " openBanks=" + StringUtil::toString( newGroups[0] ) +
                        " lowPockets=" + StringUtil::toString( newGroups[1] ) +
                        " forks=" + StringUtil::toString( plantedForks.size() ) );
            }
        }  // namespace cityHillside
