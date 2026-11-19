#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    using namespace scene_build;

    namespace
    {
        #include "Workphone/Procedural/CityVergeTransitions.hpp"
        #include "Workphone/Procedural/CityRoundaboutLandscape.hpp"
    }

    namespace scene_build
    {
#include "Workphone/Procedural/CityCarriagewayUtilities.hpp"
#include "Workphone/Procedural/CityRoadEdgeSurface.hpp"
#include "Workphone/Procedural/CityRoadLaneWear.hpp"
#include "Workphone/Procedural/CityBuildingGroundContact.hpp"
#include "Workphone/Procedural/CityEntranceWayfinding.hpp"
#include "Workphone/Procedural/CityRoundaboutSigns.hpp"
#include "Workphone/Procedural/CityFootwayAppearance.hpp"
#include "Workphone/Procedural/CityKerbCoping.hpp"
#include "Workphone/Procedural/CityPavementRelief.hpp"
#include "Workphone/Procedural/CityCrossingLandings.hpp"
#include "Workphone/Procedural/CityCrossingBollards.hpp"
#include "Workphone/Procedural/CityParkLandscape.hpp"
#include "Workphone/Procedural/CityReserveLandscape.hpp"
#include "Workphone/Procedural/CityParkAmenities.hpp"
#include "Workphone/Procedural/CityStormDrains.hpp"
#include "Workphone/Procedural/CityHinterland.hpp"
#include "Workphone/Procedural/CityBuildingGlazing.hpp"
#include "Workphone/Procedural/CityShopfront.hpp"
#include "Workphone/Procedural/CityShopInterior.hpp"
#include "Workphone/Procedural/CityFacadeDetail.hpp"
#include "Workphone/Procedural/CityRoofDeck.hpp"
#include "Workphone/Procedural/CityRoofEquipment.hpp"
#include "Workphone/Procedural/CityPitchedRoofCladding.hpp"
#include "Workphone/Procedural/CityRoofDrainage.hpp"
#include "Workphone/Procedural/CityBuildingImposters.hpp"

        void buildCity( SceneAssets &assets, SmartPtr<IGameActor> sourceOwner,
                        SmartPtr<IGameActor> vehicle, u32 seed, VehicleAppearanceQuality quality )
        {
            auto city = OpenCityLayout::generate( seed, assets.cityBlocks, assets.cityRoute,
                                                  assets.cityNaturalRoads );
            SourceBudgetVector<Vector3F> fittedRoute;
            CityRoadBlockFaces terrainBlocks;
            if( assets.cityTerrainRoads )
            {
                fittedRoute =
                    prepareTerrainCity( assets, city, sourceOwner, vehicle, seed, terrainBlocks );
            }
            if( assets.cityBuildingDensity != 100 )
            {
                applyCityBuildingDensity( city, seed, assets.cityBuildingDensity );
                if( assets.cityTerrainRoads )
                {
                    assets.cityLayout = city;
                }
            }
            const auto buildingCount = std::count_if( city.lots.begin(), city.lots.end(),
                                                      []( const auto &lot ) { return !lot.park; } );
            auto app = core::IApplicationManager::instance();
            auto roads = app->getFactoryManager()->createObjectFromType<IRoadSystem>( "IRoadSystem" );
            if( !roads )
            {
                throw std::runtime_error( "Open city requires the WPProcedural road service." );
            }
            roads->setSeed( seed );
            auto forge =
                app->getFactoryManager()->createObjectFromType<ITextureForge>( "ITextureForge" );
            if( !forge )
            {
                throw std::runtime_error( "Open city requires the WPProcedural texture forge." );
            }
            forge->setSeed( seed );
            SurfaceBakeParams surfaceParams;
            // Shared tiled facade, paving and soil maps retain their world scale;
            // only the largest cities use smaller source images. Vehicle and
            // carriageway bakes keep their independent quality settings.
            surfaceParams.size =
                quality == VehicleAppearanceQuality::Preview || buildingCount > 256 ? 256 : 512;
            SmartPtr<render::IMaterial> cityAsphalt;
            if( !assets.cityTerrainRoads )
            {
                auto asphaltParams = surfaceParams;
                asphaltParams.size = quality == VehicleAppearanceQuality::Preview ? 256 : 512;
                cityAsphalt = raceAsphalt( assets, forge, asphaltParams, seed );
            }
            // World projection gives segments and intersections the same physical texture scale.
            if( cityAsphalt )
            {
                cityAsphalt->setUVProjection( 3 );
                const auto albedo = cityAsphalt->getTexture( 0 );
                if( !albedo || albedo->getName().find( "/city-surface/" ) == String::npos )
                {
                    cityAsphalt->setEditorFloat( "uvTriplanarScale", 1.f / 8.f );
                }
            }
            Geometry asphalt, pavement, markings, parks, courtyards, streetMetal, lampGlass;
            SourceBudgetVector<Vector3F> terrainStreetLamps;
            SourceBudgetVector<cityGroundContact::Entry> buildingEntries;
            SourceBudgetVector<cityEntranceWayfinding::Mount> entranceSigns;
            constexpr u32 paneAtlasColumns = cityBuildingGlazing::atlasColumns;
            constexpr float panePadding = cityBuildingGlazing::padding;

            struct CityPatch
            {
                bool customDetailOutsideFarMesh = false;
                Geometry walls[3], farWalls[3], glass[3], farGlass, roofs, farRoofs, frames, accents;
                Geometry windowPanes, timber, tiles, farTiles, officeJoinery;
                Geometry shopfrontFascia[static_cast<size_t>( cityShopfront::Style::Count )],
                    shopfrontLetters, shopfrontDisplays;
                cityShopInterior::Surfaces shopInterior;
                cityFacadeDetail::GeometryBatches masonry;
                cityRoofDeck::Surfaces finishedRoofs;
                cityRoofEquipment::Surfaces mechanicalRoofs;
                cityPitchedRoofCladding::Surfaces pitchedRoofs, farPitchedRoofs;
                cityRoofDrainage::Surfaces roofDrainage;
                Array<LODDetailBound> bounds{ Allocator<LODDetailBound>( sourceBudget ) };
            };

            SourceBudgetMap<std::pair<int, int>, CityPatch> patches;
            size_t emittedShopfronts = 0;
            size_t emittedDisplayBays = 0;
            bool emittedMasonry = false;
            bool emittedRoofFinish = false, emittedRoofEquipment = false;
            bool emittedPitchedRoofs = false, emittedRoofDrainage = false;
            const size_t displayBayLimit = buildingCount > 256 ? 96 : 144;
            size_t nominalMansardSilhouettes = 0, emittedMansardSilhouettes = 0;
            CityBuildingKit kit( assets, buildingCount );
            const bool detailedBuildings = quality != VehicleAppearanceQuality::Preview;
            const bool decorateShopfronts = detailedBuildings &&
                                            !kit.hasModule( CityBuildingPart::Window ) &&
                                            !kit.hasModule( CityBuildingPart::Entrance );
            const bool dressOfficeMasonry =
                detailedBuildings && !kit.hasModule( CityBuildingPart::Wall ) &&
                !kit.hasModule( CityBuildingPart::Window ) && !kit.hasModule( CityBuildingPart::Corner );
            const int buildingStyle = std::clamp( assets.cityBuildingStyle, 0, cityBuildingStyleMax );
            const auto streetDetails = OpenCityStreetDetails::generate(
                city, seed, quality != VehicleAppearanceQuality::Preview );
            // Match the existing flat physics proxy. Preserve library topology,
            // kerbs and markings without introducing unsupported road-height bumps.
            auto append = []( Geometry &g, const RoadMesh &source, bool flat = false,
                              float height = .012f ) {
                const auto base = static_cast<u32>( g.positions.size() );
                for( const auto &v : source.vertices )
                {
                    g.positions.push_back( { float( v.position.x ),
                                             flat ? height : float( v.position.y ),
                                             float( v.position.z ) } );
                    g.normals.push_back( flat ? Vector3F( 0, 1, 0 )
                                              : Vector3F( float( v.normal.x ), float( v.normal.y ),
                                                          float( v.normal.z ) ) );
                    g.uv.push_back( { float( v.uv.x ), float( v.uv.y ), 0 } );
                    g.tangents.push_back( { 1, 0, 0, 1 } );
                }
                if( source.indices.size() % 3 )
                {
                    throw std::runtime_error( "Incomplete city road triangles." );
                }
                for( size_t triangle = 0; triangle < source.indices.size(); triangle += 3 )
                {
                    if( source.indices[triangle] >= source.vertices.size() ||
                        source.indices[triangle + 1] >= source.vertices.size() ||
                        source.indices[triangle + 2] >= source.vertices.size() )
                    {
                        throw std::runtime_error( "Invalid city road triangle index." );
                    }
                    auto a = base + source.indices[triangle], b = base + source.indices[triangle + 1],
                         c = base + source.indices[triangle + 2];
                    if( a >= g.positions.size() || b >= g.positions.size() || c >= g.positions.size() )
                    {
                        throw std::runtime_error( "Invalid city road triangle index." );
                    }
                    // Marking meshes can carry the opposite winding to their authored
                    // up normal. Flattened city surfaces always face out of the ground.
                    if( flat && ( g.positions[b] - g.positions[a] )
                                        .crossProduct( g.positions[c] - g.positions[a] )
                                        .y < 0 )
                    {
                        std::swap( b, c );
                    }
                    g.indices.push_back( a );
                    g.indices.push_back( b );
                    g.indices.push_back( c );
                }
            };
            const int half = city.blocks / 2;
            u32 segmentSeed = seed;
            if( assets.cityTerrainRoads )
            {
                for( const auto &lot : city.lots )
                {
                    if( lot.park )
                    {
                        continue;  // Parks use the retained ground and its collider.
                    }
                    const float top = lot.groundHeight + .15f, bottom = lot.groundMinimumHeight - .25f;
                    const Vector3F centre( lot.parcelX, ( top + bottom ) * .5f, lot.parcelZ );
                    const Vector3F size( lot.parcelWidth, top - bottom, lot.parcelDepth );
                    // A zero-setback facade already closes the upper overlap on all four
                    // sides. The pad's visual top meets its base while its collider stays intact.
                    const bool covered = !lot.park && std::abs( lot.x - lot.parcelX ) < .001f &&
                                         std::abs( lot.z - lot.parcelZ ) < .001f &&
                                         std::abs( lot.width - lot.parcelWidth ) < .001f &&
                                         std::abs( lot.depth - lot.parcelDepth ) < .001f;
                    courtyards.box( centre - Vector3F( 0, covered ? .075f : 0, 0 ),
                                    size - Vector3F( 0, covered ? .15f : 0, 0 ) );
                    staticBox( assets, "City terrain foundation", centre, size );
                }
                SourceBudgetVector<Vector3F> lampCandidates;
                const auto &source = assets.roadAuthoring->getSource();
                const auto &bake = assets.roadAuthoring->getBake();
                for( size_t road = 0; road < bake.roads.size(); ++road )
                {
                    const auto &points = bake.roads[road].points;
                    float next = 24;
                    for( size_t i = 1; i < points.size(); ++i )
                    {
                        const auto &a = points[i - 1], &b = points[i];
                        while( next <= b.distance )
                        {
                            const float t = std::clamp(
                                ( next - a.distance ) / ( b.distance - a.distance ), 0.f, 1.f );
                            next += 60;
                            const auto centre = a.position + ( b.position - a.position ) * t;
                            auto forward = b.position - a.position;
                            forward.y = 0;
                            forward.normalise();
                            const Vector3F across( forward.z, 0, -forward.x );
                            for( int side : { -1, 1 } )
                            {
                                auto point =
                                    centre +
                                    across * ( side * ( source.roads[road].width * .5f + 1.3f ) );
                                bool clear = true;
                                for( const auto &junction : source.roundabouts )
                                {
                                    if( std::hypot( point.x - junction.center.x,
                                                    point.z - junction.center.z ) <
                                        junction.outerRadius + 2 )
                                    {
                                        clear = false;
                                    }
                                }
                                for( size_t other = 0; clear && other < bake.roads.size(); ++other )
                                {
                                    const auto &samples = bake.roads[other].points;
                                    for( size_t j = 1; j < samples.size(); ++j )
                                    {
                                        const auto u = samples[j - 1].position, v = samples[j].position;
                                        const float dx = v.x - u.x, dz = v.z - u.z;
                                        const float s = std::clamp(
                                            ( ( point.x - u.x ) * dx + ( point.z - u.z ) * dz ) /
                                                ( dx * dx + dz * dz ),
                                            0.f, 1.f );
                                        const float x = point.x - u.x - s * dx,
                                                    z = point.z - u.z - s * dz,
                                                    r = source.roads[other].width * .5f + .5f;
                                        if( x * x + z * z < r * r )
                                        {
                                            clear = false;
                                            break;
                                        }
                                    }
                                }
                                float height = 0;
                                if( clear && lampCandidates.size() < 2048 &&
                                    render::sampleTerrainHeight(
                                        *assets.roadAuthoring->getTerrainSnapshot(),
                                        assets.roadAuthoring->getTerrainPlacement(), point, height ) )
                                {
                                    point.y = height;
                                    lampCandidates.push_back( point );
                                }
                            }
                        }
                    }
                }
                std::sort( lampCandidates.begin(), lampCandidates.end(),
                           []( const auto &a, const auto &b ) {
                               return a.lengthSquared() < b.lengthSquared();
                           } );
                for( size_t i = 0;
                     i < std::min<size_t>( lampCandidates.size(),
                                           quality == VehicleAppearanceQuality::Preview ? 24 : 64 );
                     ++i )
                {
                    const auto ground = lampCandidates[i], base = ground + Vector3F( 0, .15f, 0 ),
                               top = base + Vector3F( 0, 6.2f, 0 );
                    if( assets.cityStreetDetail > 0 )
                    {
                        terrainStreetLamps.push_back( ground );
                    }
                    courtyards.box( ground + Vector3F( 0, .075f, 0 ), { .4f, .15f, .4f } );
                    streetMetal.beam( base, top, .055f );
                    const Vector3F arm( .8f, -.1f, .45f );
                    streetMetal.beam( top, top + arm, .04f );
                    lampGlass.box( top + arm, { .5f, .1f, .5f } );
                    staticBox( assets, "City lamp foundation", ground + Vector3F( 0, .075f, 0 ),
                               { .4f, .15f, .4f } );
                    staticBox( assets, "City lamp post", base + Vector3F( 0, 3.1f, 0 ),
                               { .12f, 6.2f, .12f } );
                }
            }
            else if( city.naturalRoads )
            {
                buildNaturalCityStreets( assets, city, asphalt, pavement, markings, streetMetal,
                                         lampGlass, quality );
                // Natural parcels are already filtered against the entire road
                // buffer. Their small raised courts anchor buildings and parks
                // without recreating the old rectangular block grid.
                for( const auto &lot : city.lots )
                {
                    const Vector3F centre( lot.parcelX, .075f, lot.parcelZ );
                    const Vector3F size( lot.parcelWidth, .15f, lot.parcelDepth );
                    courtyards.box( centre, size );
                    staticBox( assets, "City parcel", centre, size );
                }
            }
            else
            {
                for( int row = -half; row <= half; ++row )
                {
                    for( int col = -half; col < half; ++col )
                    {
                        for( int axis = 0; axis < 2; ++axis )
                        {
                            RoadSegmentSpec spec;
                            const float a = col * city.spacing + 6, b = ( col + 1 ) * city.spacing - 6;
                            spec.start = axis ? Vector3<real_Num>( row * city.spacing, 0, a )
                                              : Vector3<real_Num>( a, 0, row * city.spacing );
                            spec.end = axis ? Vector3<real_Num>( row * city.spacing, 0, b )
                                            : Vector3<real_Num>( b, 0, row * city.spacing );
                            spec.roadClass = RoadClass::Arterial;
                            spec.seed = segmentSeed++;
                            spec.generateDressing = false;
                            spec.generateSidewalks = false;
                            spec.generateKerbs = false;
                            const auto road = roads->generateSegment( spec );
                            append( asphalt, road.roadSurface, true );
                            // Shared city walks have one flat grade and own each corner once.
                            append( markings, road.markings, true, .018f );
                        }
                    }
                }
                for( int x = -half; x <= half; ++x )
                {
                    for( int z = -half; z <= half; ++z )
                    {
                        IntersectionSpec spec;
                        spec.center = { x * city.spacing, 0, z * city.spacing };
                        spec.roadClass = RoadClass::Arterial;
                        spec.seed = segmentSeed++;
                        const auto junction = roads->generateIntersection( spec );
                        append( asphalt, junction.surface, true );
                        append( markings, junction.markings, true, .018f );
                    }
                }
                const auto addWalk = [&]( const OpenCityStreetDetails::Corner &walk ) {
                    const Vector3F centre( walk.x, ( walk.bottom + walk.top ) * .5f, walk.z );
                    const Vector3F size( walk.width, walk.top - walk.bottom, walk.depth );
                    pavement.box( centre, size );
                };
                for( const auto &walk : streetDetails.sidewalks )
                {
                    addWalk( walk );
                }
                for( const auto &walk : streetDetails.corners )
                {
                    addWalk( walk );
                }
                // The inner block slabs meet the sidewalks at the same authored grade.
                for( int x = -half; x < half; ++x )
                {
                    for( int z = -half; z < half; ++z )
                    {
                        const Vector3F centre( ( x + .5f ) * city.spacing, .075f,
                                               ( z + .5f ) * city.spacing );
                        const Vector3F size( city.spacing - 17, .15f, city.spacing - 17 );
                        courtyards.box( centre, size );
                    }
                }
                // The adjoining plaza/walk pieces form one solid interior square per
                // block. Use that exact union to avoid thousands of scene-root actors.
                for( const auto &surface : streetDetails.collisionSurfaces( city ) )
                {
                    staticBox( assets, "City sidewalk",
                               { surface.x, ( surface.bottom + surface.top ) * .5f, surface.z },
                               { surface.width, surface.top - surface.bottom, surface.depth } );
                }
                for( const auto &item : streetDetails.furniture )
                {
                    const Vector3F p( item.x, item.baseY, item.z );
                    if( item.kind == OpenCityStreetDetails::FurnitureKind::Lamp )
                    {
                        const Vector3F top = p + Vector3F( 0, 6.2f, 0 );
                        const Vector3F arm( std::sin( item.yaw ) * 1.25f, -.15f,
                                            std::cos( item.yaw ) * 1.25f );
                        streetMetal.beam( p, top, .055f );
                        streetMetal.beam( top, top + arm, .045f );
                        lampGlass.box( top + arm, { .5f, .10f, .5f } );
                        staticBox( assets, "City lamp post", p + Vector3F( 0, 3.1f, 0 ),
                                   { .12f, 6.2f, .12f } );
                    }
                    else
                    {
                        streetMetal.box( p + Vector3F( 0, .45f, 0 ), { .13f, .9f, .13f } );
                    }
                }
            }
            for( const auto &lot : city.lots )
            {
                if( lot.park )
                {
                    if( !assets.cityTerrainRoads )
                    {
                        parks.box( { lot.parcelX, lot.groundHeight + .155f, lot.parcelZ },
                                   { lot.parcelWidth - 1, .01f, lot.parcelDepth - 1 } );
                    }
                    continue;
                }
                kit.beginBuilding();
                const auto modulePlacementsBefore = assets.cityBuildingModulePlacements;
                const auto key =
                    std::make_pair( int( std::floor( lot.x / 128 ) ), int( std::floor( lot.z / 128 ) ) );
                const Vector3F origin( key.first * 128.f + 64, 0, key.second * 128.f + 64 );
                auto &patch = patches[key];
                patch.bounds.push_back(
                    { Vector3F( lot.x, lot.groundHeight + ( lot.height + 6.f ) * .5f, lot.z ) - origin,
                      std::max( { lot.width + 2.5f, lot.depth + 2.5f, lot.height + 6.f } ) } );
                const auto volumes = OpenCityLayout::buildingVolumes( lot );
                // One stable kit per parcel, shared by every occupied tier. District mix
                // retains neighbourhood character while varying both roofline and frontage.
                const int residentialStyles[] = { 1, 4, 4, 5, 7, 9 };
                const int commercialStyles[] = { 3, 3, 6, 6, 8 };
                const int civicStyles[] = { 2, 2, 5, 8 };
                const int style = buildingStyle ? buildingStyle
                                  : lot.district == OpenCityLayout::District::Commercial
                                      ? commercialStyles[lot.variant % 5]
                                  : lot.district == OpenCityLayout::District::Civic
                                      ? civicStyles[lot.variant % 4]
                                      : residentialStyles[lot.variant % 6];
                const bool office = style == 3, civic = style == 2;
                const bool terrace = style == 4, parisian = style == 5, warehouse = style == 6;
                const bool mediterranean = style == 7, artdeco = style == 8, timber = style == 9;
                // Fine ornaments share a fixed city-wide allowance, divided fairly
                // before emission. Roof forms, occupied shells and glazing never
                // depend on this optional-detail budget. Dense ten-block cities
                // therefore cannot multiply railings or sash bars without bound.
                size_t facadeDetailVertices = 0, roofDetailVertices = 0;
                size_t masonryVerticesLeft = 250000u / std::max<size_t>( 1, buildingCount );
                cityFacadeDetail::Counts masonry;
                const size_t facadeDetailLimit =
                    ( buildingCount > 256 ? 110000u : 220000u ) / std::max<size_t>( 1, buildingCount );
                const size_t roofDetailLimit =
                    ( buildingCount > 256 ? 50000u : 100000u ) / std::max<size_t>( 1, buildingCount );
                const auto admitFacadeDetail = [&]( size_t vertices ) {
                    if( vertices > facadeDetailLimit - facadeDetailVertices )
                    {
                        return false;
                    }
                    facadeDetailVertices += vertices;
                    return true;
                };
                const auto admitRoofDetail = [&]( size_t vertices ) {
                    if( vertices > roofDetailLimit - roofDetailVertices )
                    {
                        return false;
                    }
                    roofDetailVertices += vertices;
                    return true;
                };
                size_t roofFinishVertices = 175000u / std::max<size_t>( 1, buildingCount );
                const auto admitRoofFinish = [&]( size_t vertices ) {
                    if( vertices > roofFinishVertices )
                    {
                        return false;
                    }
                    roofFinishVertices -= vertices;
                    return true;
                };
                size_t roofDrainageVertices = 120000u / std::max<size_t>( 1, buildingCount );
                const auto admitRoofDrainage = [&]( size_t vertices ) {
                    if( vertices > roofDrainageVertices )
                    {
                        return false;
                    }
                    roofDrainageVertices -= vertices;
                    return true;
                };
                const int entranceAxis = lot.frontage == OpenCityLayout::Frontage::West ||
                                                 lot.frontage == OpenCityLayout::Frontage::East
                                             ? 1
                                             : 0;
                const int entranceSign = lot.frontage == OpenCityLayout::Frontage::West ||
                                                 lot.frontage == OpenCityLayout::Frontage::North
                                             ? -1
                                             : 1;
                for( size_t tier = 0; tier < volumes.size(); ++tier )
                {
                    const auto &volume = volumes[tier];
                    const Vector3F centre( volume.x, volume.bottom + volume.height * .5f, volume.z );
                    const Vector3F size( volume.width, volume.height, volume.depth );
                    const bool groundFloor = volume.bottom == lot.groundHeight;
                    const u32 family =
                        civic || parisian || mediterranean || artdeco || timber ? 1u
                        : office
                            ? ( buildingStyle == 0 && groundFloor && lot.variant % 3 == 0 ? 1u : 2u )
                            : ( buildingStyle == 0 && lot.variant % 5 == 0 ? 1u : 0u );
                    auto &opaque = patch.walls[family];
                    patch.farWalls[family].box( centre - origin, size );
                    // Geometry and collision consume the identical stepped mass description.
                    staticBox( assets, "City building mass", centre, size );
                    const float cx = centre.x - origin.x, cz = centre.z - origin.z;
                    const float top = volume.bottom + volume.height;
                    // Horizontal caps close the shell. Near walls are segmented around
                    // genuine apertures; a solid wall never covers the recessed glazing.
                    opaque.quad( { cx - size.x * .5f, top, cz - size.z * .5f },
                                 { cx + size.x * .5f, top, cz - size.z * .5f },
                                 { cx + size.x * .5f, top, cz + size.z * .5f },
                                 { cx - size.x * .5f, top, cz + size.z * .5f }, { 0, 1, 0 } );
                    opaque.quad( { cx - size.x * .5f, volume.bottom, cz - size.z * .5f },
                                 { cx + size.x * .5f, volume.bottom, cz - size.z * .5f },
                                 { cx + size.x * .5f, volume.bottom, cz + size.z * .5f },
                                 { cx - size.x * .5f, volume.bottom, cz + size.z * .5f }, { 0, -1, 0 } );
                    patch.roofs.box( { cx, top + .06f, cz },
                                     { volume.width + .12f, .12f, volume.depth + .12f } );
                    patch.farRoofs.box( { cx, top + .06f, cz },
                                        { volume.width + .12f, .12f, volume.depth + .12f } );
                    const float nominalFloor = warehouse           ? 4.1f
                                               : office            ? 3.6f
                                               : civic || parisian ? 3.5f
                                                                   : 3.2f;
                    const int floors = std::max( 1, int( std::round( volume.height / nominalFloor ) ) );
                    const float floorHeight = volume.height / floors;
                    for( int axis : { entranceAxis, 1 - entranceAxis } )
                    {
                        for( int sign : { entranceSign, -entranceSign } )
                        {
                            const float span = axis ? volume.depth : volume.width;
                            const bool streetFace = axis == entranceAxis && sign == entranceSign;
                            const bool entrance = groundFloor && streetFace;
                            const Vector3F normal( axis ? float( sign ) : 0, 0,
                                                   axis ? 0 : float( sign ) );
                            const Vector3F along( axis ? 0 : 1, 0, axis ? 1 : 0 );
                            const float yaw = axis ? float( sign ) * pi * .5f : sign < 0 ? pi : 0;
                            const auto point = [&]( float s, float y, float inset ) {
                                return axis
                                           ? Vector3F( cx + sign * ( size.x * .5f - inset ), y, cz + s )
                                           : Vector3F( cx + s, y, cz + sign * ( size.z * .5f - inset ) );
                            };
                            const auto panel = [&]( Geometry &g, float left, float low, float right,
                                                    float high, float inset = 0.f ) {
                                if( right - left <= .001f || high - low <= .001f )
                                {
                                    return;
                                }
                                g.quad( point( left, low, inset ), point( right, low, inset ),
                                        point( right, high, inset ), point( left, high, inset ),
                                        normal );
                            };
                            const auto module = [&]( CityBuildingPart role, Geometry &geometry,
                                                     float left, float low, float right, float high,
                                                     float inset, float thickness ) {
                                return detailedBuildings && right > left && high > low &&
                                       kit.append(
                                           role, geometry,
                                           point( ( left + right ) * .5f, ( low + high ) * .5f, inset ),
                                           { right - left, high - low, thickness }, yaw );
                            };
                            const auto wallPanel = [&]( float left, float low, float right,
                                                        float high ) {
                                if( right - left >= 1.25f && high - low >= .60f &&
                                    module( CityBuildingPart::Wall, opaque, left, low, right, high, .06f,
                                            .12f ) )
                                {
                                    return;
                                }
                                panel( opaque, left, low, right, high );
                            };
                            const auto reveal = [&]( float left, float low, float right, float high,
                                                     float depth ) {
                                opaque.quad( point( left, low, 0 ), point( left, low, depth ),
                                             point( left, high, depth ), point( left, high, 0 ), along );
                                opaque.quad( point( right, low, 0 ), point( right, high, 0 ),
                                             point( right, high, depth ), point( right, low, depth ),
                                             -along );
                                opaque.quad( point( left, low, 0 ), point( right, low, 0 ),
                                             point( right, low, depth ), point( left, low, depth ),
                                             { 0, 1, 0 } );
                                opaque.quad( point( left, high, 0 ), point( left, high, depth ),
                                             point( right, high, depth ), point( right, high, 0 ),
                                             { 0, -1, 0 } );
                            };
                            const auto faceBox = [&]( Geometry &g, float s, float y, float width,
                                                      float height, float projection, float thickness ) {
                                g.box( point( s, y, -projection ),
                                       axis ? Vector3F( thickness, height, width )
                                            : Vector3F( width, height, thickness ) );
                            };
                            const float desiredBay = terrace || parisian || timber ? 4.2f
                                                     : warehouse                   ? 4.2f
                                                     : artdeco                     ? 4.2f
                                                     : office                      ? 3.8f
                                                     : civic ? 4.6f
                                                             : 4.4f + ( lot.variant % 3 ) * .45f;
                            const int nominalBays =
                                std::max( 1, int( std::floor( span / desiredBay ) ) );
                            const bool denseFacade = !office && buildingCount > 128;
                            const float density = denseFacade ? 128.f / float( buildingCount ) : 1.f;
                            // Preserve the six-block frontage exactly. Larger cities use
                            // fewer, normally sized apertures, with priority given to the
                            // street elevation. Storey heights and roof forms stay intact.
                            const int bays =
                                denseFacade
                                    ? std::max( 1, int( std::floor( nominalBays *
                                                                    ( streetFace ? std::sqrt( density )
                                                                                 : density ) ) ) )
                                    : nominalBays;
                            const float bayWidth = span / bays;
                            const float windowBayWidth = denseFacade ? span / nominalBays : bayWidth;
                            if( assets.cityTerrainRoads && entrance )
                            {
                                const float doorOffset =
                                    office ? 0.f : -span * .5f + ( bays / 2 + .5f ) * bayWidth;
                                const float doorHalfWidth =
                                    office ? std::min( .92f, ( span * .5f - .38f ) * .8f )
                                           : std::min( 2.05f, bayWidth * .72f ) * .5f;
                                buildingEntries.push_back(
                                    { &lot, point( doorOffset, volume.bottom, 0 ) + origin, normal,
                                      along, span, doorOffset, doorHalfWidth } );
                            }
                            for( int floor = 0; floor < floors; ++floor )
                            {
                                const float y0 = volume.bottom + floor * floorHeight;
                                const float y1 = floor + 1 == floors ? top : y0 + floorHeight;
                                const bool retail =
                                    entrance && floor == 0 &&
                                    ( office || civic || warehouse || parisian || artdeco ||
                                      ( !terrace && !timber && !mediterranean &&
                                        lot.variant % 3 != 0 ) );
                                const float low = y0 + ( retail ? .28f : office ? .62f : .78f );
                                const float high = y1 - ( retail ? .42f : office ? .48f : .62f );
                                auto &glass =
                                    patch.glass[( lot.variant / 7 + axis + ( sign > 0 ? 1 : 0 ) ) % 2];
                                const auto windowPane = [&]( Geometry &plain, float left, float bottom,
                                                             float right, float head, float depth,
                                                             int bay, bool usePlain ) {
                                    if( usePlain || !detailedBuildings )
                                    {
                                        panel( plain, left, bottom, right, head, depth );
                                        return;
                                    }
                                    if( right - left <= .001f || head - bottom <= .001f )
                                    {
                                        return;
                                    }
                                    const u64 building = ( u64( lot.variant ) << 32 ) | seed;
                                    const u64 pane = ( u64( tier ) << 40 ) | ( u64( floor ) << 24 ) |
                                                     ( u64( bay ) << 3 ) |
                                                     u64( axis * 2 + ( sign > 0 ) );
                                    const u32 variant =
                                        u32( render::foliageStableHash( building ^ pane ) );
                                    const u32 roll = variant % 20;
                                    const u32 face =
                                        u32( render::foliageStableHash( building ^ ( pane & 7u ) ) );
                                    const bool officeBlind =
                                        bays == 2 ? ( u32( bay + 2 * floor ) + face ) % 4 == 0
                                                  : roll < 4;
                                    u32 tile = variant % 4;
                                    if( !retail && office && officeBlind )
                                    {
                                        tile = 6 + ( variant >> 9 ) % 2;
                                    }
                                    if( !retail && !office && roll < 6 )
                                    {
                                        tile = 4 + ( variant >> 9 ) % 4;
                                    }
                                    const float u0 = ( tile + panePadding ) / paneAtlasColumns;
                                    const float u1 = ( tile + 1.f - panePadding ) / paneAtlasColumns;
                                    patch.windowPanes.quad(
                                        point( left, bottom, depth ), point( right, bottom, depth ),
                                        point( right, head, depth ), point( left, head, depth ), normal,
                                        u0, u1, panePadding, 1.f - panePadding );
                                };
                                bool entranceModule = false;
                                if( high - low < .45f )
                                {
                                    wallPanel( -span * .5f, y0, span * .5f, y1 );
                                    continue;
                                }
                                const float shopDoor =
                                    office ? 0.f : -span * .5f + ( bays / 2 + .5f ) * bayWidth;
                                const bool builtInShopfront =
                                    decorateShopfronts && retail && ( office || warehouse || artdeco ) &&
                                    y1 - y0 >= 3.05f &&
                                    cityShopfront::fasciaWidth( span, shopDoor ) >= 2.3f &&
                                    admitFacadeDetail( cityShopfront::faceBudget );
                                const auto shopStyle =
                                    builtInShopfront
                                        ? cityShopfront::choose( lot.id, seed, warehouse, artdeco )
                                        : cityShopfront::Style::Market;
                                u32 displayCount = 0, interiorCount = 0;
                                const auto interior = [&]( float left, float right, float bottom,
                                                           float head, float depth, int bay ) {
                                    if( !builtInShopfront || interiorCount + displayCount >= 2 ||
                                        right - left < 1.15f || head - bottom < 1.5f ||
                                        emittedDisplayBays >= displayBayLimit )
                                    {
                                        return false;
                                    }
                                    const u32 variation = u32( render::foliageStableHash(
                                        lot.id ^ ( u64( bay + 1 ) << 32 ) ^ interiorCount ) );
                                    cityShopInterior::append( patch.shopInterior, shopStyle, normal,
                                                              along, point, left, bottom, right, head,
                                                              depth, variation );
                                    ++interiorCount;
                                    ++emittedDisplayBays;
                                    return true;
                                };
                                const auto display = [&]( float left, float right, float bottom,
                                                          float head, float depth, int bay ) {
                                    if( !builtInShopfront || interiorCount + displayCount >= 2 ||
                                        right - left < 1.05f || head - bottom < 1.38f ||
                                        !admitFacadeDetail( cityShopfront::displayBudget ) )
                                    {
                                        return;
                                    }
                                    const u32 variation = u32( render::foliageStableHash(
                                        lot.id ^ ( u64( bay + 1 ) << 32 ) ^ displayCount ) );
                                    cityShopfront::appendDisplay(
                                        patch.shopfrontDisplays, patch.officeJoinery, shopStyle, faceBox,
                                        left, right, bottom, head, depth, variation );
                                    ++displayCount;
                                };
                                if( office )
                                {
                                    if( dressOfficeMasonry && ( floor > 0 || !groundFloor ) )
                                    {
                                        cityFacadeDetail::officeStorey(
                                            patch.masonry, masonry, point, normal, along, span, bays, y0,
                                            low, high, y1, masonryVerticesLeft );
                                    }
                                    // Curtain-wall kit: recessed ribbons, substantial spandrels
                                    // and fine mullions rather than hundreds of boxed windows.
                                    const float left = -span * .5f + .38f, right = span * .5f - .38f;
                                    const float doorHalfWidth = std::min( .92f, right * .8f );
                                    const float doorHead = std::min( high, y0 + 2.7f );
                                    const float paneDepth =
                                        builtInShopfront ? cityShopfront::glassDepth : .085f;
                                    if( entrance && floor == 0 )
                                    {
                                        entranceModule = module(
                                            CityBuildingPart::Entrance, patch.accents, -doorHalfWidth,
                                            y0 + .16f, doorHalfWidth, doorHead, .022f, .14f );
                                    }
                                    if( entranceModule )
                                    {
                                        wallPanel( -span * .5f, y0, -doorHalfWidth, low );
                                        panel( opaque, -doorHalfWidth, y0, doorHalfWidth, y0 + .16f );
                                        wallPanel( doorHalfWidth, y0, span * .5f, low );
                                        reveal( left, low, -doorHalfWidth, high, .085f );
                                        reveal( doorHalfWidth, low, right, high, .085f );
                                        reveal( -doorHalfWidth, y0 + .16f, doorHalfWidth, doorHead,
                                                .085f );
                                    }
                                    else
                                    {
                                        wallPanel( -span * .5f, y0, span * .5f, low );
                                        reveal( left, low, right, high, paneDepth );
                                    }
                                    wallPanel( -span * .5f, high, span * .5f, y1 );
                                    panel( opaque, -span * .5f, low, left, high );
                                    panel( opaque, right, low, span * .5f, high );
                                    const auto pane = [&]( float paneLeft, float paneRight, int surface,
                                                           int bay ) {
                                        if( paneRight - paneLeft <= .001f )
                                        {
                                            return false;
                                        }
                                        const bool custom =
                                            module( CityBuildingPart::Window, patch.frames, paneLeft,
                                                    low, paneRight, high, .050f, .07f );
                                        if( builtInShopfront && !custom )
                                        {
                                            const float leftEdge =
                                                std::clamp( -doorHalfWidth - .16f, paneLeft, paneRight );
                                            const float rightEdge =
                                                std::clamp( doorHalfWidth + .16f, paneLeft, paneRight );
                                            const auto displayPane = [&]( float a, float b ) {
                                                if( interior( a, b, low, high, paneDepth, bay ) )
                                                {
                                                    return;
                                                }
                                                windowPane( patch.glass[surface], a, low, b, high,
                                                            paneDepth, bay, false );
                                                display( a, b, low, high, paneDepth, bay );
                                            };
                                            displayPane( paneLeft, leftEdge );
                                            windowPane( patch.glass[surface], leftEdge, low, rightEdge,
                                                        high, paneDepth, bay, false );
                                            displayPane( rightEdge, paneRight );
                                        }
                                        else
                                        {
                                            windowPane( patch.glass[surface], paneLeft, low, paneRight,
                                                        high, custom ? .097f : paneDepth, bay, custom );
                                        }
                                        return custom;
                                    };
                                    // Keep the continuous ribbon aperture and individual pane treatments.
                                    bool previousModule = false, anyWindowModule = false;
                                    for( int bay = 0; bay < bays; ++bay )
                                    {
                                        const float paneLeft =
                                            std::max( left, -span * .5f + bay * bayWidth );
                                        const float paneRight =
                                            std::min( right, -span * .5f + ( bay + 1 ) * bayWidth );
                                        u32 variation =
                                            lot.variant ^ ( u32( floor + 1 ) * 0x9e3779b9u ) ^
                                            ( u32( bay + 1 ) * 0x85ebca6bu ) ^ u32( axis * 31 + sign );
                                        variation ^= variation >> 16;
                                        variation *= 0x7feb352du;
                                        variation ^= variation >> 15;
                                        const int surface = variation % 19 == 0 ? 2
                                                            : variation % 7 < 2 ? 1
                                                                                : 0;
                                        bool windowModule = false;
                                        if( entranceModule && paneRight > -doorHalfWidth &&
                                            paneLeft < doorHalfWidth )
                                        {
                                            windowModule =
                                                pane( paneLeft, std::min( paneRight, -doorHalfWidth ),
                                                      surface, bay );
                                            windowModule |= pane( std::max( paneLeft, doorHalfWidth ),
                                                                  paneRight, surface, bay );
                                            panel( patch.glass[surface],
                                                   std::max( paneLeft, -doorHalfWidth ), doorHead,
                                                   std::min( paneRight, doorHalfWidth ), high, .085f );
                                        }
                                        else
                                        {
                                            windowModule = pane( paneLeft, paneRight, surface, bay );
                                        }
                                        if( bay > 0 && !windowModule && !previousModule )
                                        {
                                            const float s = -span * .5f + bay * bayWidth;
                                            const float mullionBase =
                                                entranceModule && std::abs( s ) < doorHalfWidth + .035f
                                                    ? doorHead
                                                    : low;
                                            panel( patch.frames, s - .035f, mullionBase, s + .035f, high,
                                                   .045f );
                                        }
                                        previousModule = windowModule;
                                        anyWindowModule |= windowModule;
                                    }
                                    panel( patch.farGlass, left, low, right, high, -.018f );
                                    if( detailedBuildings && !anyWindowModule &&
                                        ( streetFace || floor < 3 ) )
                                    {
                                        // Narrow framing modules sit in front of the recessed panes.
                                        for( int bay = 0; bay < bays; ++bay )
                                        {
                                            const float a =
                                                std::max( left, -span * .5f + bay * bayWidth );
                                            const float b =
                                                std::min( right, -span * .5f + ( bay + 1 ) * bayWidth );
                                            const int divisions = std::clamp(
                                                int( std::ceil( ( b - a ) / 1.50f ) ), 1, 3 );
                                            for( int division = 1; division < divisions; ++division )
                                            {
                                                const float s =
                                                    a + ( b - a ) * float( division ) / divisions;
                                                if( retail && std::abs( s ) <= doorHalfWidth + .09f )
                                                {
                                                    continue;
                                                }
                                                if( !admitFacadeDetail( 24 ) )
                                                {
                                                    break;
                                                }
                                                faceBox( patch.officeJoinery, s, ( low + high ) * .5f,
                                                         .050f, high - low, -.047f, .060f );
                                            }
                                        }
                                        if( admitFacadeDetail( retail ? 72 : 48 ) )
                                        {
                                            faceBox( patch.officeJoinery, 0, high - .028f, right - left,
                                                     .056f, -.047f, .062f );
                                            if( retail )
                                            {
                                                for( float side : { -1.f, 1.f } )
                                                {
                                                    faceBox( patch.officeJoinery,
                                                             side * ( right + doorHalfWidth ) * .5f,
                                                             low + .028f, right - doorHalfWidth, .056f,
                                                             -.047f, .062f );
                                                }
                                            }
                                            else
                                            {
                                                faceBox( patch.officeJoinery, 0, low + .028f,
                                                         right - left, .056f, -.047f, .062f );
                                            }
                                        }
                                    }
                                    if( !builtInShopfront && !entranceModule && !anyWindowModule &&
                                        lot.variant % 3 == 1 )
                                    {
                                        panel( patch.frames, left, ( low + high ) * .5f - .025f, right,
                                               ( low + high ) * .5f + .025f, .045f );
                                    }
                                    panel( patch.frames, -span * .5f, y1 - .065f, span * .5f, y1,
                                           -.025f );
                                }
                                else
                                {
                                    for( int bay = 0; bay < bays; ++bay )
                                    {
                                        const float s0 = -span * .5f + bay * bayWidth,
                                                    s1 = s0 + bayWidth;
                                        const float s = ( s0 + s1 ) * .5f;
                                        const bool door = entrance && floor == 0 && bay == bays / 2;
                                        const float ratio = warehouse ? .74f
                                                            : artdeco ? .52f
                                                            : timber  ? .50f
                                                            : retail  ? .79f
                                                            : civic ? .56f
                                                                    : .48f + ( lot.variant % 3 ) * .05f;
                                        const float width = door ? std::min( 2.05f, bayWidth * .72f )
                                                                 : windowBayWidth * ratio;
                                        const float left = s - width * .5f, right = s + width * .5f;
                                        const float bottom = door ? y0 + .16f : low;
                                        const float head =
                                            door ? std::min( y1 - .35f, y0 + 2.65f ) : high;
                                        const float depth = builtInShopfront && !door
                                                                ? cityShopfront::glassDepth
                                                            : civic ? .24f
                                                                    : .16f;
                                        wallPanel( s0, y0, s1, bottom );
                                        wallPanel( s0, head, s1, y1 );
                                        panel( opaque, s0, bottom, left, head );
                                        panel( opaque, right, bottom, s1, head );
                                        reveal( left, bottom, right, head, depth );
                                        const bool hasModule = module(
                                            door ? CityBuildingPart::Entrance : CityBuildingPart::Window,
                                            door ? patch.accents : patch.frames, left, bottom, right,
                                            head, depth * .45f, depth );
                                        entranceModule |= door && hasModule;
                                        const bool bayWindow =
                                            terrace && streetFace && !door && !hasModule && floor < 2 &&
                                            ( bay + lot.variant ) % 2 == 0 && admitFacadeDetail( 132 );
                                        const bool room =
                                            !door && !hasModule && !bayWindow &&
                                            interior( left, right, bottom, head, depth, bay );
                                        if( ( !door || !hasModule ) && !bayWindow && !room )
                                        {
                                            windowPane( glass, left, bottom, right, head, depth, bay,
                                                        door || hasModule );
                                        }
                                        if( !room && !door )
                                        {
                                            display( left, right, bottom, head, depth, bay );
                                        }
                                        if( !hasModule )
                                        {
                                            panel( patch.frames, s - .032f, bottom, s + .032f, head,
                                                   depth - .025f );
                                        }
                                        panel( patch.farGlass, left, bottom, right, head, -.018f );
                                        if( !retail && !door && axis == entranceAxis &&
                                            sign == entranceSign )
                                        {
                                            // A shallow projecting sill gives a grazing highlight.
                                            faceBox( patch.frames, s, bottom - .035f, width + .18f, .07f,
                                                     .05f, .20f );
                                        }
                                        if( detailedBuildings && !hasModule && !door )
                                        {
                                            if( bayWindow )
                                            {
                                                // Three glazed faces form a shallow oriel. The
                                                // occupied wall aperture remains open behind it.
                                                const float projection = .42f, shoulder = width * .12f;
                                                panel( glass, left + shoulder, bottom, right - shoulder,
                                                       head, -projection );
                                                glass.quad(
                                                    point( left, bottom, 0 ),
                                                    point( left + shoulder, bottom, -projection ),
                                                    point( left + shoulder, head, -projection ),
                                                    point( left, head, 0 ),
                                                    ( normal * shoulder - along * projection )
                                                        .normaliseCopy() );
                                                glass.quad(
                                                    point( right - shoulder, bottom, -projection ),
                                                    point( right, bottom, 0 ), point( right, head, 0 ),
                                                    point( right - shoulder, head, -projection ),
                                                    ( normal * shoulder + along * projection )
                                                        .normaliseCopy() );
                                                for( float jamb :
                                                     { left + shoulder, s, right - shoulder } )
                                                {
                                                    faceBox( patch.frames, jamb, ( bottom + head ) * .5f,
                                                             .055f, head - bottom, projection + .015f,
                                                             .065f );
                                                }
                                                faceBox( patch.frames, s, bottom - .075f, width + .12f,
                                                         .15f, projection * .5f, projection + .14f );
                                                faceBox( patch.frames, s, head + .06f, width + .14f,
                                                         .12f, projection * .5f, projection + .16f );
                                            }
                                            if( warehouse && admitFacadeDetail( 120 ) )
                                            {
                                                // Tall steel factory sash with an actual multi-pane grid.
                                                for( int pane = 1; pane < 3; ++pane )
                                                {
                                                    faceBox( patch.accents, left + width * pane / 3.f,
                                                             ( bottom + head ) * .5f, .045f,
                                                             head - bottom, -depth + .035f, .055f );
                                                }
                                                for( int pane = 1; pane < 3; ++pane )
                                                {
                                                    faceBox( patch.accents, s,
                                                             bottom + ( head - bottom ) * pane / 3.f,
                                                             width, .045f, -depth + .035f, .055f );
                                                }
                                                faceBox( patch.frames, s, head + .07f, width + .22f,
                                                         .14f, .06f, .20f );
                                            }
                                            if( mediterranean && admitFacadeDetail( 240 ) )
                                            {
                                                const float shutter =
                                                    std::min( .48f, ( bayWidth - width ) * .34f );
                                                for( float side : { -1.f, 1.f } )
                                                {
                                                    const float shutterS =
                                                        s +
                                                        side * ( width * .5f + shutter * .55f + .05f );
                                                    faceBox( patch.accents, shutterS,
                                                             ( bottom + head ) * .5f, shutter,
                                                             head - bottom, .07f, .08f );
                                                    for( int slat = 1; slat <= 4; ++slat )
                                                    {
                                                        faceBox( patch.timber, shutterS,
                                                                 bottom + ( head - bottom ) * slat / 5.f,
                                                                 shutter * .86f, .045f, .12f, .035f );
                                                    }
                                                }
                                            }
                                            if( ( parisian || mediterranean ) && streetFace &&
                                                floor > 0 &&
                                                ( parisian ? floor == 1 || floor + 1 == floors
                                                           : ( bay + floor + lot.variant ) % 3 == 0 ) &&
                                                admitFacadeDetail( 216 ) )
                                            {
                                                // Balconettes alternate with deeper slabs; rail spacing
                                                // stays capped independently of parcel dimensions.
                                                const float balconyWidth = width + .42f;
                                                const float projection = parisian ? .46f : .60f;
                                                faceBox( patch.frames, s, bottom - .16f, balconyWidth,
                                                         .14f, projection * .5f, projection + .12f );
                                                faceBox( patch.accents, s, bottom + .65f, balconyWidth,
                                                         .055f, projection, .055f );
                                                for( int rail = 0; rail <= 4; ++rail )
                                                {
                                                    faceBox( patch.accents,
                                                             s + balconyWidth * ( rail / 4.f - .5f ),
                                                             bottom + .25f, .035f, .80f, projection,
                                                             .035f );
                                                }
                                                for( float side : { -1.f, 1.f } )
                                                {
                                                    faceBox( patch.accents,
                                                             s + side * balconyWidth * .5f,
                                                             bottom + .65f, .045f, .055f,
                                                             projection * .5f, projection );
                                                }
                                            }
                                        }
                                    }
                                }
                                if( detailedBuildings && ( warehouse || artdeco || timber ) )
                                {
                                    for( int bay = 0; bay <= bays; ++bay )
                                    {
                                        const float s =
                                            std::clamp( -span * .5f + bay * bayWidth, -span * .5f + .16f,
                                                        span * .5f - .16f );
                                        if( timber )
                                        {
                                            if( floor == 0 && admitFacadeDetail( 24 ) )
                                            {
                                                faceBox( patch.timber, s,
                                                         volume.bottom + volume.height * .5f, .15f,
                                                         volume.height, .065f, .16f );
                                            }
                                            if( bay < bays && floor > 0 && admitFacadeDetail( 24 ) )
                                            {
                                                patch.timber.beam(
                                                    point( s + .16f, y0 + .14f, -.10f ),
                                                    point( s + std::min( bayWidth * .28f, .9f ),
                                                           low - .08f, -.10f ),
                                                    .05f );
                                            }
                                        }
                                        else if( warehouse && floor == 0 && admitFacadeDetail( 48 ) )
                                        {
                                            faceBox( opaque, s, volume.bottom + volume.height * .5f,
                                                     .32f, volume.height, .11f, .24f );
                                            faceBox( patch.frames, s, top - .12f, .46f, .16f, .12f,
                                                     .28f );
                                        }
                                        else if( artdeco && floor == 0 && admitFacadeDetail( 72 ) )
                                        {
                                            for( int flute = -1; flute <= 1; ++flute )
                                            {
                                                faceBox( patch.frames, s + flute * .095f,
                                                         volume.bottom + volume.height * .5f, .055f,
                                                         volume.height, .13f + ( flute == 0 ? .04f : 0 ),
                                                         .13f );
                                            }
                                        }
                                    }
                                    if( timber && admitFacadeDetail( 24 ) )
                                    {
                                        faceBox( patch.timber, 0, y1 - .10f, span, .20f, .055f, .16f );
                                    }
                                }
                                if( entrance && floor == 0 && !entranceModule )
                                {
                                    const float doorS =
                                        office ? 0.f : -span * .5f + ( bays / 2 + .5f ) * bayWidth;
                                    const float head = office ? std::min( y1 - .42f, y0 + 2.7f )
                                                              : std::min( y1 - .35f, y0 + 2.65f );
                                    const float doorHalfWidth =
                                        office ? .92f : std::min( 2.05f, bayWidth * .72f ) * .5f;
                                    const float depth = office ? .06f : civic ? .215f : .135f;
                                    panel( patch.frames, doorS - doorHalfWidth, y0 + .16f,
                                           doorS - doorHalfWidth + .06f, head, depth );
                                    panel( patch.frames, doorS + doorHalfWidth - .06f, y0 + .16f,
                                           doorS + doorHalfWidth, head, depth );
                                    panel( patch.frames, doorS - doorHalfWidth, head - .065f,
                                           doorS + doorHalfWidth, head, depth );
                                    faceBox( patch.frames, doorS - .16f, y0 + 1.3f, .025f, .40f,
                                             -depth + .015f, .025f );
                                    faceBox( patch.frames, doorS + .16f, y0 + 1.3f, .025f, .40f,
                                             -depth + .015f, .025f );
                                    if( office && admitFacadeDetail( 72 ) )
                                    {
                                        for( float side : { -1.f, 1.f } )
                                        {
                                            faceBox( patch.officeJoinery,
                                                     doorS + side * doorHalfWidth * .5f, y0 + .37f,
                                                     doorHalfWidth - .073f, .19f, -depth + .021f,
                                                     .035f );
                                        }
                                        faceBox( patch.officeJoinery, doorS, ( y0 + .28f + head ) * .5f,
                                                 .046f, head - y0 - .28f, -depth + .026f, .054f );
                                    }
                                    const float canopyWidth =
                                        std::min( span * .60f, civic               ? 11.f
                                                               : terrace || timber ? 3.2f
                                                                                   : 7.f );
                                    const float canopyDepth = civic ? 1.05f : timber ? .95f : .70f;
                                    if( builtInShopfront )
                                    {
                                        cityShopfront::appendFace(
                                            patch.shopfrontFascia[static_cast<u32>( shopStyle )],
                                            patch.officeJoinery, patch.shopfrontLetters, shopStyle,
                                            normal, along, point, faceBox, doorS, doorHalfWidth, span,
                                            y0, high, y1 );
                                        ++emittedShopfronts;
                                        if( interiorCount )
                                        {
                                            WP_LOG( String( "City commercial display rooms lot=" ) +
                                                    StringUtil::toString( lot.id ) + ", style=" +
                                                    cityShopInterior::styleName( shopStyle ) +
                                                    ", bays=" + StringUtil::toString( interiorCount ) );
                                        }
                                    }
                                    else
                                    {
                                        if( mediterranean && admitFacadeDetail( 168 ) )
                                        {
                                            for( int rafter = 0; rafter <= 5; ++rafter )
                                            {
                                                faceBox( patch.timber,
                                                         doorS + canopyWidth * ( rafter / 5.f - .5f ),
                                                         y1 - .08f, .10f, .14f, canopyDepth * .5f,
                                                         canopyDepth );
                                            }
                                            faceBox( patch.timber, doorS, y1 - .18f, canopyWidth + .16f,
                                                     .16f, canopyDepth * .82f, .13f );
                                        }
                                        else
                                        {
                                            faceBox( timber ? patch.timber : patch.accents, doorS,
                                                     y1 - .08f, canopyWidth, .16f, canopyDepth * .5f,
                                                     canopyDepth );
                                        }
                                        faceBox( patch.frames, doorS, y1 + .10f, canopyWidth * .82f,
                                                 .20f, .045f, .10f );
                                    }
                                    if( assets.cityTerrainRoads && detailedBuildings &&
                                        assets.cityStreetDetail > 0 &&
                                        ( office || civic || warehouse || artdeco ) &&
                                        assets.cityBuildingMeshes[size_t( CityBuildingPart::Wall )]
                                            .empty() &&
                                        assets.cityBuildingMeshes[size_t( CityBuildingPart::Entrance )]
                                            .empty() &&
                                        assets.cityBuildingMeshes[size_t( CityBuildingPart::Corner )]
                                            .empty() )
                                    {
                                        // The lettering is supported by the real metal fascia, outside the
                                        // central shop name and clear of door, glass, and moving leaves.
                                        const float fascia =
                                            builtInShopfront ? cityShopfront::fasciaWidth( span, doorS )
                                                             : canopyWidth;
                                        const float side =
                                            Vector3F::unitY().crossProduct( normal ).dotProduct(
                                                along ) >= 0
                                                ? 1.f
                                                : -1.f;
                                        if( fascia >= ( builtInShopfront ? 2.55f : 2.18f ) )
                                        {
                                            entranceSigns.push_back(
                                                { lot.id,
                                                  point( doorS + side * ( fascia * .5f -
                                                                          ( builtInShopfront ? .40f
                                                                                             : .33f ) ),
                                                         y1 - ( builtInShopfront ? .198f : .08f ),
                                                         -( builtInShopfront ? .152f
                                                                             : canopyDepth + .002f ) ) +
                                                      origin,
                                                  normal, along * -side, builtInShopfront ? 1.24f : 1.f,
                                                  lot.district ==
                                                      OpenCityLayout::District::Commercial } );
                                        }
                                    }
                                    if( civic )
                                    {
                                        for( int bay = 0; bay <= bays; ++bay )
                                        {
                                            const float s =
                                                std::clamp( -span * .5f + bay * bayWidth,
                                                            -span * .5f + .22f, span * .5f - .22f );
                                            // Engaged stone columns articulate a portico; the closed
                                            // facade never advertises an unsupported walkable interior.
                                            faceBox( patch.frames, s, y0 + floorHeight * .5f, .32f,
                                                     floorHeight - .20f, .12f, .36f );
                                            faceBox( patch.frames, s, y0 + .25f, .47f, .30f, .12f,
                                                     .46f );
                                        }
                                    }
                                    if( ( artdeco || warehouse || terrace || timber ) &&
                                        admitFacadeDetail( artdeco     ? 192
                                                           : warehouse ? 168
                                                           : terrace   ? 72
                                                                       : 48 ) )
                                    {
                                        for( float side : { -1.f, 1.f } )
                                        {
                                            const float jamb = doorS + side * ( doorHalfWidth + .18f );
                                            faceBox( timber ? patch.timber : patch.frames, jamb,
                                                     ( y0 + head ) * .5f, timber ? .17f : .23f,
                                                     head - y0, .13f, .24f );
                                            if( artdeco )
                                            {
                                                for( int step = 0; step < 3; ++step )
                                                {
                                                    faceBox(
                                                        patch.frames, jamb + side * step * .18f,
                                                        y0 + ( head - y0 + .55f - step * .20f ) * .5f,
                                                        .10f, head - y0 + .55f - step * .20f,
                                                        .22f - step * .035f, .16f );
                                                }
                                            }
                                        }
                                        if( warehouse )
                                        {
                                            for( int rib = 0; rib < 5; ++rib )
                                            {
                                                faceBox( patch.accents, doorS, y0 + .42f + rib * .39f,
                                                         doorHalfWidth * 1.8f, .055f, -depth + .025f,
                                                         .045f );
                                            }
                                        }
                                        if( terrace )
                                        {
                                            faceBox( patch.frames, doorS, head + .13f,
                                                     doorHalfWidth * 2 + .68f, .20f, .15f, .34f );
                                        }
                                    }
                                }
                            }
                            if( detailedBuildings && ( !office || entrance ) )
                            {
                                // Stone courses frame the elevations without filling an aperture.
                                const float courseY =
                                    civic && entrance ? volume.bottom + floorHeight + .08f : top - .19f;
                                if( courseY < top - .08f )
                                {
                                    faceBox( patch.frames, 0, courseY, span + .08f, .13f, .075f, .17f );
                                }
                                if( parisian && admitFacadeDetail( floors > 1 ? 96 : 72 ) )
                                {
                                    for( int course = 0; course < 3; ++course )
                                    {
                                        faceBox( patch.frames, 0, top - .35f + course * .12f,
                                                 span + .10f + course * .09f, .10f, .08f + course * .04f,
                                                 .22f + course * .08f );
                                    }
                                    if( floors > 1 )
                                    {
                                        faceBox( patch.frames, 0, volume.bottom + floorHeight + .10f,
                                                 span + .14f, .20f, .08f, .24f );
                                    }
                                }
                            }
                        }
                    }
                    // Corner piers give masonry real depth. Explicit corner modules can also
                    // dress the primary commercial tier and the simplified preview.
                    if( groundFloor )
                    {
                        const float pierHeight = std::min( volume.height - .25f, floorHeight * 2 );
                        auto &cornerGeometry = detailedBuildings ? patch.frames : patch.farRoofs;
                        for( const float xSide : { -1.f, 1.f } )
                        {
                            for( const float zSide : { -1.f, 1.f } )
                            {
                                const Vector3F corner( cx + xSide * ( size.x * .5f - .14f ),
                                                       volume.bottom + .18f + pierHeight * .5f,
                                                       cz + zSide * ( size.z * .5f - .14f ) );
                                const float yaw = zSide > 0 ? xSide * pi * .25f : xSide * pi * .75f;
                                if( !kit.append( CityBuildingPart::Corner, cornerGeometry, corner,
                                                 { .50f, pierHeight, .50f }, yaw ) &&
                                    detailedBuildings &&
                                    ( civic || ( !office && lot.variant % 3 == 1 ) ) )
                                {
                                    patch.frames.box( corner, { .39f, pierHeight, .39f } );
                                    patch.frames.box(
                                        corner + Vector3F( 0, -pierHeight * .5f + .14f, 0 ),
                                        { .49f, .28f, .49f } );
                                    patch.frames.box( corner + Vector3F( 0, pierHeight * .5f - .10f, 0 ),
                                                      { .47f, .20f, .47f } );
                                }
                            }
                        }
                    }
                    if( tier + 1 != volumes.size() )
                    {
                        continue;
                    }

                    // Keep the building's occupied tier and collision intact. Each LOD retains
                    // its roof silhouette, while the street-level kit adds smaller features.
                    const float deck = top + .12f;
                    auto &moduleRoof = detailedBuildings ? patch.roofs : patch.farRoofs;
                    const auto roofVertexStart = moduleRoof.positions.size(),
                               roofIndexStart = moduleRoof.indices.size();
                    const bool replacedRoof =
                        kit.append( CityBuildingPart::Roof, moduleRoof, { cx, deck + 1.25f, cz },
                                    { size.x, 2.5f, size.z }, 0 );
                    if( detailedBuildings && replacedRoof )
                    {
                        cityBuildingImposters::appendPlacedModule( patch.farRoofs, moduleRoof,
                                                                   roofVertexStart, roofIndexStart );
                    }
                    const bool hipped = style == 1 || mediterranean;
                    const bool gabled = terrace || timber;
                    const bool lantern = civic || ( office && lot.variant % 4 == 1 );
                    // Timber houses present their gable to the street. Other pitched
                    // roofs keep a ridge along the longer occupied dimension.
                    const bool longX = timber ? entranceAxis == 1 : size.x >= size.z;
                    const auto roofPoint = [&]( float u, float y, float v ) {
                        return longX ? Vector3F( cx + u, deck + y, cz + v )
                                     : Vector3F( cx + v, deck + y, cz + u );
                    };
                    const float halfLong = ( longX ? size.x : size.z ) * .5f;
                    const float halfShort = ( longX ? size.z : size.x ) * .5f;
                    const float rise = parisian  ? 2.65f
                                       : timber  ? std::clamp( halfShort * .55f, 1.8f, 3.8f )
                                       : terrace ? std::clamp( halfShort * .43f, 1.5f, 3.0f )
                                                 : std::clamp( halfShort * .31f, .9f, 2.5f );
                    const auto roofBox = [&]( Geometry &g, float u, float y, float v, float width,
                                              float height, float depth ) {
                        g.box( roofPoint( u, y, v ), longX ? Vector3F( width, height, depth )
                                                           : Vector3F( depth, height, width ) );
                    };
                    const auto slope = [&]( Geometry &g, Vector3F a, Vector3F b, Vector3F c,
                                            Vector3F d ) {
                        auto normal = ( b - a ).crossProduct( d - a ).normaliseCopy();
                        g.quad( a, b, c, d, normal.y < 0 ? -normal : normal );
                    };
                    const int roofBays = std::clamp( int( halfLong * 2 / 4.2f ), 2, 6 );
                    const int longRoofDormers = std::clamp( int( ( halfLong * 2 - 3.f ) / 3.2f ), 1, 6 );
                    const int endRoofBays = std::clamp( int( ( halfShort * 2 - 3.f ) / 3.2f ), 1, 4 );
                    const int sawTeeth = std::clamp( int( halfShort * 2 / 4.f ), 2, 4 );
                    const auto mansardDormers = [&]( auto visit, bool countSilhouettes = false ) {
                        const bool frontageEnd = entranceAxis == ( longX ? 1 : 0 );
                        // Visit the actual street frontage first so the finite raised
                        // sash budget is useful even when the street meets a short end.
                        for( bool endFace : { frontageEnd, !frontageEnd } )
                        {
                            const int nominalCount = endFace ? endRoofBays : longRoofDormers;
                            const float available =
                                std::max( 0.f, ( endFace ? halfShort : halfLong ) - 1.5f );
                            for( float side : { float( entranceSign ), float( -entranceSign ) } )
                            {
                                const bool streetFace =
                                    endFace == frontageEnd && side == float( entranceSign );
                                const int minimum = std::min( nominalCount, streetFace ? 2 : 1 );
                                const int count =
                                    buildingCount > 128
                                        ? std::clamp(
                                              int( std::floor( nominalCount *
                                                               ( 128.f / float( buildingCount ) ) ) ),
                                              minimum, nominalCount )
                                        : nominalCount;
                                if( countSilhouettes )
                                {
                                    nominalMansardSilhouettes += nominalCount;
                                    emittedMansardSilhouettes += count;
                                }
                                for( int bay = 0; bay < count; ++bay )
                                {
                                    visit( endFace,
                                           ( bay + .5f ) * ( 2 * available / count ) - available, side );
                                }
                            }
                        }
                    };
                    const auto dormerPoint = [&]( bool endFace, float along, float y, float side,
                                                  float inset ) {
                        return endFace ? roofPoint( side * ( halfLong - inset ), y, along )
                                       : roofPoint( along, y, side * ( halfShort - inset ) );
                    };
                    const auto dormerBox = [&]( Geometry &g, bool endFace, float along, float y,
                                                float side, float inset, float width, float height,
                                                float depth ) {
                        if( endFace )
                        {
                            roofBox( g, side * ( halfLong - inset ), y, along, depth, height, width );
                        }
                        else
                        {
                            roofBox( g, along, y, side * ( halfShort - inset ), width, height, depth );
                        }
                    };
                    const auto roofSilhouette = [&]( Geometry &roof ) {
                        if( parisian )
                        {
                            // A steep lower skirt and a shallow capped upper deck give a
                            // mansard an unmistakable silhouette at both render distances.
                            const float inset = std::min( 1.35f, halfShort * .28f );
                            for( float side : { -1.f, 1.f } )
                            {
                                slope(
                                    roof, roofPoint( -halfLong, 0, side * halfShort ),
                                    roofPoint( halfLong, 0, side * halfShort ),
                                    roofPoint( halfLong - inset, rise, side * ( halfShort - inset ) ),
                                    roofPoint( -halfLong + inset, rise, side * ( halfShort - inset ) ) );
                                slope(
                                    roof, roofPoint( side * halfLong, 0, -halfShort ),
                                    roofPoint( side * halfLong, 0, halfShort ),
                                    roofPoint( side * ( halfLong - inset ), rise, halfShort - inset ),
                                    roofPoint( side * ( halfLong - inset ), rise, -halfShort + inset ) );
                            }
                            mansardDormers(
                                [&]( bool endFace, float along, float side ) {
                                    dormerBox( roof, endFace, along, 1.18f, side, .43f, 1.26f, 1.66f,
                                               .76f );
                                    dormerBox( roof, endFace, along, 2.07f, side, .43f, 1.44f, .12f,
                                               .92f );
                                },
                                true );
                            roofBox( roof, 0, rise + .04f, 0, ( halfLong - inset ) * 2, .08f,
                                     ( halfShort - inset ) * 2 );
                            return;
                        }
                        if( warehouse )
                        {
                            const float toothWidth = 2 * halfShort / sawTeeth;
                            for( int tooth = 0; tooth < sawTeeth; ++tooth )
                            {
                                const float v0 = -halfShort + tooth * toothWidth, v1 = v0 + toothWidth;
                                slope( roof, roofPoint( -halfLong, .08f, v0 ),
                                       roofPoint( halfLong, .08f, v0 ), roofPoint( halfLong, 1.45f, v1 ),
                                       roofPoint( -halfLong, 1.45f, v1 ) );
                                const Vector3F faceNormal =
                                    longX ? Vector3F( 0, 0, 1 ) : Vector3F( 1, 0, 0 );
                                roof.quad( roofPoint( -halfLong, .08f, v1 ),
                                           roofPoint( halfLong, .08f, v1 ),
                                           roofPoint( halfLong, 1.45f, v1 ),
                                           roofPoint( -halfLong, 1.45f, v1 ), faceNormal );
                                for( float side : { -1.f, 1.f } )
                                {
                                    const auto a = roofPoint( side * halfLong, .08f, v0 );
                                    const auto b = roofPoint( side * halfLong, .08f, v1 );
                                    const auto c = roofPoint( side * halfLong, 1.45f, v1 );
                                    roof.quad( ( a + b ) * .5f, b, c, a,
                                               longX ? Vector3F( side, 0, 0 ) : Vector3F( 0, 0, side ) );
                                }
                            }
                            return;
                        }
                        if( artdeco )
                        {
                            float bottom = 0;
                            for( int step = 0; step < 3; ++step )
                            {
                                const float fraction = .78f - step * .20f;
                                const float height = .85f + step * .10f;
                                roof.box( { cx, deck + bottom + height * .5f, cz },
                                          { size.x * fraction, height, size.z * fraction } );
                                roof.box( { cx, deck + bottom + height + .04f, cz },
                                          { size.x * fraction + .14f, .08f, size.z * fraction + .14f } );
                                bottom += height;
                            }
                            return;
                        }
                        if( gabled )
                        {
                            for( float side : { -1.f, 1.f } )
                            {
                                slope( roof, roofPoint( -halfLong, 0, side * halfShort ),
                                       roofPoint( halfLong, 0, side * halfShort ),
                                       roofPoint( halfLong, rise, 0 ), roofPoint( -halfLong, rise, 0 ) );
                                const auto a = roofPoint( side * halfLong, 0, -halfShort );
                                const auto b = roofPoint( side * halfLong, 0, halfShort );
                                const auto c = roofPoint( side * halfLong, rise, 0 );
                                roof.quad( ( a + b ) * .5f, b, c, a,
                                           longX ? Vector3F( side, 0, 0 ) : Vector3F( 0, 0, side ) );
                            }
                            return;
                        }
                        if( hipped )
                        {
                            const float ridge = std::max( .1f, halfLong - halfShort * .75f );
                            for( float side : { -1.f, 1.f } )
                            {
                                const auto a = roofPoint( -halfLong, 0, side * halfShort );
                                const auto b = roofPoint( halfLong, 0, side * halfShort );
                                const auto c = roofPoint( ridge, rise, 0 );
                                const auto d = roofPoint( -ridge, rise, 0 );
                                auto n = ( b - a ).crossProduct( d - a );
                                n.normalise();
                                roof.quad( a, b, c, d, n.y < 0 ? -n : n );
                                const auto e = roofPoint( side * halfLong, 0, -halfShort );
                                const auto f = roofPoint( side * halfLong, 0, halfShort );
                                const auto g = roofPoint( side * ridge, rise, 0 );
                                auto endNormal = ( f - e ).crossProduct( g - e );
                                endNormal.normalise();
                                // Split the triangular hip at its eave midpoint to keep
                                // indexed quads nondegenerate in the standard mesh stream.
                                const auto mid = ( e + f ) * .5f;
                                roof.quad( mid, f, g, e, endNormal.y < 0 ? -endNormal : endNormal );
                            }
                            return;
                        }
                        const float parapet = office ? .52f : .42f;
                        for( float side : { -1.f, 1.f } )
                        {
                            roof.box( { cx, deck + parapet * .5f, cz + side * ( size.z * .5f - .10f ) },
                                      { size.x, parapet, .20f } );
                            roof.box( { cx + side * ( size.x * .5f - .10f ), deck + parapet * .5f, cz },
                                      { .20f, parapet, size.z - .40f } );
                        }
                        if( lantern )
                        {
                            const float height = civic ? 1.35f : 1.75f;
                            roof.box( { cx, deck + height * .5f, cz },
                                      { size.x * .43f, height, size.z * .36f } );
                            roof.box( { cx, deck + height + .09f, cz },
                                      { size.x * .43f + .28f, .18f, size.z * .36f + .28f } );
                        }
                    };
                    if( detailedBuildings && !replacedRoof )
                    {
                        roofSilhouette( mediterranean ? patch.tiles
                                        : artdeco     ? patch.walls[family]
                                                      : patch.roofs );
                    }
                    if( !replacedRoof )
                    {
                        roofSilhouette( mediterranean ? patch.farTiles
                                        : artdeco     ? patch.farWalls[family]
                                                      : patch.farRoofs );
                    }
                    if( !replacedRoof && ( gabled || hipped ) )
                    {
                        // Physical texture scale survives LOD changes. Only near roofs
                        // spend the finite trim budget on course lips and rainwater goods.
                        const auto distant = cityPitchedRoofCladding::append(
                            patch.farPitchedRoofs, roofPoint, halfLong, halfShort, rise, hipped,
                            mediterranean, lot.id, seed, admitRoofFinish, false );
                        emittedPitchedRoofs |= distant.slopes > 0;
                        if( detailedBuildings )
                        {
                            const auto finish = cityPitchedRoofCladding::append(
                                patch.pitchedRoofs, roofPoint, halfLong, halfShort, rise, hipped,
                                mediterranean, lot.id, seed, admitRoofFinish );
                            const bool clearWall = volumes.size() == 1 &&
                                                   !kit.hasModule( CityBuildingPart::Wall ) &&
                                                   !kit.hasModule( CityBuildingPart::Window ) &&
                                                   !kit.hasModule( CityBuildingPart::Corner );
                            const cityRoofDrainage::Roof roof{ { cx, deck, cz }, halfLong, halfShort,
                                                               volume.bottom,    longX,    hipped,
                                                               clearWall };
                            const auto drainage = cityRoofDrainage::append( patch.roofDrainage, roof,
                                                                            lot.id, admitRoofDrainage );
                            emittedRoofDrainage |= drainage.runs > 0;
                            WP_LOG( String( "City pitched roof lot=" ) + StringUtil::toString( lot.id ) +
                                    ", slopes=" + StringUtil::toString( finish.slopes ) +
                                    ", courses=" + StringUtil::toString( finish.courses ) +
                                    ", caps=" + StringUtil::toString( finish.caps ) +
                                    ", eaves=" + StringUtil::toString( finish.eaves ) +
                                    ", gutters=" + StringUtil::toString( u64( drainage.runs ) ) +
                                    ", brackets=" + StringUtil::toString( u64( drainage.brackets ) ) +
                                    ", downpipes=" + StringUtil::toString( u64( drainage.downpipes ) ) );
                        }
                    }
                    if( !detailedBuildings || replacedRoof )
                    {
                        continue;
                    }
                    if( !parisian && !warehouse && !artdeco && !gabled && !hipped )
                    {
                        const Vector3F roofCentre( cx, deck, cz );
                        auto finish = cityRoofDeck::append(
                            patch.finishedRoofs, roofCentre, size, deck, office ? .52f : .42f,
                            render::foliageStableHash( lot.id ^ seed ), admitRoofFinish );
                        if( lantern && finish.sheets )
                        {
                            cityRoofDeck::lanternCap( patch.finishedRoofs, finish, roofCentre, size,
                                                      deck + ( civic ? 1.35f : 1.75f ) + .18f,
                                                      admitRoofFinish );
                        }
                        if( finish.sheets )
                        {
                            emittedRoofFinish = true;
                            WP_LOG( String( "City roof finishing lot=" ) +
                                    StringUtil::toString( lot.id ) +
                                    ", sheets=" + StringUtil::toString( finish.sheets ) +
                                    ", seams=" + StringUtil::toString( finish.seams ) +
                                    ", coping=" + StringUtil::toString( finish.copingPieces ) +
                                    ", drains=" + StringUtil::toString( finish.drains ) );
                        }
                        const Vector2F lanternHalf =
                            lantern ? Vector2F( size.x * .215f + .14f, size.z * .18f + .14f )
                                    : Vector2F{};
                        const auto services = cityRoofEquipment::append(
                            patch.mechanicalRoofs, roofCentre, size, deck, lot.id, seed, office,
                            admitRoofDetail, lanternHalf );
                        if( services.airHandlers || services.condensers || services.vents )
                        {
                            emittedRoofEquipment = true;
                            WP_LOG( String( "City roof equipment lot=" ) +
                                    StringUtil::toString( lot.id ) +
                                    ", airHandlers=" + StringUtil::toString( services.airHandlers ) +
                                    ", condensers=" + StringUtil::toString( services.condensers ) +
                                    ", vents=" + StringUtil::toString( services.vents ) );
                        }
                    }
                    if( parisian )
                    {
                        mansardDormers( [&]( bool endFace, float along, float side ) {
                            // Every elevation has glazing. Only the optional raised
                            // sash consumes the existing per-building trim allowance.
                            const Vector3F normal =
                                endFace == longX ? Vector3F( side, 0, 0 ) : Vector3F( 0, 0, side );
                            patch.glass[1].quad(
                                dormerPoint( endFace, along - .46f, .62f, side, .035f ),
                                dormerPoint( endFace, along + .46f, .62f, side, .035f ),
                                dormerPoint( endFace, along + .46f, 1.82f, side, .035f ),
                                dormerPoint( endFace, along - .46f, 1.82f, side, .035f ), normal );
                            if( !admitRoofDetail( 120 ) )
                            {
                                return;
                            }
                            dormerBox( patch.frames, endFace, along, .59f, side, .035f, 1.12f, .10f,
                                       .12f );
                            dormerBox( patch.frames, endFace, along, 1.85f, side, .035f, 1.14f, .10f,
                                       .12f );
                            for( float jamb : { -.5f, 0.f, .5f } )
                            {
                                dormerBox( patch.frames, endFace, along + jamb, 1.22f, side, .015f,
                                           .065f, 1.30f, .07f );
                            }
                        } );
                    }
                    else if( warehouse )
                    {
                        const float toothWidth = 2 * halfShort / sawTeeth;
                        for( int tooth = 1; tooth <= sawTeeth; ++tooth )
                        {
                            const float v = -halfShort + tooth * toothWidth + .018f;
                            patch.glass[1].quad( roofPoint( -halfLong + .12f, .31f, v ),
                                                 roofPoint( halfLong - .12f, .31f, v ),
                                                 roofPoint( halfLong - .12f, 1.35f, v ),
                                                 roofPoint( -halfLong + .12f, 1.35f, v ),
                                                 longX ? Vector3F( 0, 0, 1 ) : Vector3F( 1, 0, 0 ) );
                            if( !admitRoofDetail( 24 * ( roofBays + 2 ) ) )
                            {
                                continue;
                            }
                            roofBox( patch.frames, 0, 1.44f, v, halfLong * 2 + .06f, .08f, .12f );
                            for( int post = 0; post <= roofBays; ++post )
                            {
                                roofBox( patch.accents,
                                         -halfLong + .14f + post * ( 2 * halfLong - .28f ) / roofBays,
                                         .82f, v + .025f, .055f, 1.12f, .065f );
                            }
                        }
                    }
                    else if( artdeco )
                    {
                        if( !admitRoofDetail( 240 ) )
                        {
                            continue;
                        }
                        for( int flute = -2; flute <= 2; ++flute )
                        {
                            const float height = 3.08f - std::abs( flute ) * .20f;
                            for( float side : { -1.f, 1.f } )
                            {
                                patch.frames.box( { cx + flute * .24f, deck + height * .5f,
                                                    cz + side * ( size.z * .39f + .03f ) },
                                                  { .10f, height, .10f } );
                            }
                        }
                    }
                    else if( gabled )
                    {
                        u32 chimneyFlashings = 0;
                        for( float side : { -1.f, 1.f } )
                        {
                            const float u = side * ( halfLong + .015f );
                            const auto a = roofPoint( u, 0, -halfShort ),
                                       b = roofPoint( u, 0, halfShort ), c = roofPoint( u, rise, 0 );
                            opaque.quad( ( a + b ) * .5f, b, c, a,
                                         longX ? Vector3F( side, 0, 0 ) : Vector3F( 0, 0, side ) );
                            if( !admitRoofDetail( timber ? 144 : 48 ) )
                            {
                                continue;
                            }
                            auto &verge = timber ? patch.timber : patch.frames;
                            verge.beam( a, c, timber ? .09f : .075f );
                            verge.beam( b, c, timber ? .09f : .075f );
                            if( timber )
                            {
                                verge.beam( roofPoint( u + side * .02f, .04f, 0 ),
                                            roofPoint( u + side * .02f, rise, 0 ), .07f );
                                verge.beam( a, b, .08f );
                                for( float brace : { -.5f, .5f } )
                                {
                                    verge.beam( roofPoint( u + side * .02f, .08f, brace * halfShort ),
                                                roofPoint( u + side * .02f, rise * .48f, 0 ), .055f );
                                }
                            }
                        }
                        const int chimneys = terrace ? std::clamp( roofBays / 2, 1, 3 ) : 1;
                        for( int chimney = 0; chimney < chimneys; ++chimney )
                        {
                            if( !admitRoofDetail( 96 ) )
                            {
                                break;
                            }
                            const float u = ( chimney + .5f ) * ( 2 * halfLong / chimneys ) - halfLong;
                            roofBox( opaque, u, rise + .45f, 0, .62f, 1.05f, .72f );
                            roofBox( patch.frames, u, rise + .96f, 0, .78f, .14f, .86f );
                            for( float pot : { -.16f, .16f } )
                            {
                                roofBox( patch.tiles, u + pot, rise + 1.14f, 0, .16f, .26f, .18f );
                            }
                            chimneyFlashings += cityPitchedRoofCladding::appendFlashing(
                                patch.pitchedRoofs, roofPoint, halfLong, halfShort, rise, false, u, 0,
                                .62f, .72f, admitRoofFinish );
                        }
                        if( chimneyFlashings )
                        {
                            WP_LOG( String( "City pitched roof flashing lot=" ) +
                                    StringUtil::toString( lot.id ) +
                                    ", chimneys=" + StringUtil::toString( chimneyFlashings ) );
                        }
                    }
                    else if( hipped )
                    {
                        const float u = halfLong * ( lot.variant % 2 ? .34f : -.34f );
                        const auto base = roofPoint( u, rise * .63f, halfShort * .20f );
                        patch.walls[family].box( base + Vector3F( 0, .53f, 0 ), { .54f, 1.24f, .61f } );
                        patch.frames.box( base + Vector3F( 0, 1.16f, 0 ), { .66f, .12f, .72f } );
                        if( cityPitchedRoofCladding::appendFlashing(
                                patch.pitchedRoofs, roofPoint, halfLong, halfShort, rise, true, u,
                                halfShort * .20f, longX ? .54f : .61f, longX ? .61f : .54f,
                                admitRoofFinish ) )
                        {
                            WP_LOG( String( "City pitched roof flashing lot=" ) +
                                    StringUtil::toString( lot.id ) + ", chimneys=1" );
                        }
                    }
                    else if( lantern )
                    {
                        const float width = size.x * .43f, depth = size.z * .36f;
                        const float y = deck + ( civic ? .85f : 1.08f );
                        for( const float side : { -1.f, 1.f } )
                        {
                            patch.glass[1].box( { cx, y, cz + side * ( depth * .5f + .012f ) },
                                                { width - .48f, .62f, .024f } );
                            patch.glass[1].box( { cx + side * ( width * .5f + .012f ), y, cz },
                                                { .024f, .62f, depth - .48f } );
                            for( int post = -1; post <= 1; ++post )
                            {
                                patch.frames.box(
                                    { cx + post * width * .22f, y, cz + side * ( depth * .5f + .027f ) },
                                    { .075f, .66f, .044f } );
                            }
                        }
                    }
                }
                if( masonry.storeys )
                {
                    emittedMasonry = true;
                    WP_LOG( String( "City masonry facade lot=" ) + StringUtil::toString( lot.id ) +
                            ", faces=" + StringUtil::toString( masonry.storeys ) +
                            ", panels=" + StringUtil::toString( masonry.panels ) +
                            ", profiles=" + StringUtil::toString( masonry.profiles ) +
                            ", mullions=" + StringUtil::toString( masonry.mullions ) );
                }
                for( size_t role = 0; role < size_t( CityBuildingPart::Roof ); ++role )
                {
                    if( detailedBuildings &&
                        modulePlacementsBefore[role] != assets.cityBuildingModulePlacements[role] )
                    {
                        patch.customDetailOutsideFarMesh = true;
                    }
                }
            }
            if( nominalMansardSilhouettes != emittedMansardSilhouettes )
            {
                WP_LOG( String( "City mansard density: buildings=" ) +
                        StringUtil::toString( u64( buildingCount ) ) +
                        " emitted/nominal dormers across LODs=" +
                        StringUtil::toString( u64( emittedMansardSilhouettes ) ) + "/" +
                        StringUtil::toString( u64( nominalMansardSilhouettes ) ) +
                        " silhouette triangles avoided=" +
                        StringUtil::toString(
                            u64( nominalMansardSilhouettes - emittedMansardSilhouettes ) * 24 ) );
            }
            kit.finish();
            // Contain exploration within the shared collision plane.
            const float boundary = city.extent + 20;
            std::array<Geometry, 4> terrainBoundaries;
            for( int axis = 0; axis < 2; ++axis )
            {
                for( int side : { -1, 1 } )
                {
                    Vector3F p =
                        axis ? Vector3F( 0, .6f, side * boundary ) : Vector3F( side * boundary, .6f, 0 );
                    if( assets.cityTerrainRoads )
                    {
                        float height = 0;
                        const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
                        const auto &placement = assets.roadAuthoring->getTerrainPlacement();
                        if( !render::sampleTerrainHeight( *terrain, placement, p, height ) )
                        {
                            throw std::runtime_error( "City boundary leaves its terrain." );
                        }
                        p.y += height;
                        const Vector3F along = axis ? Vector3F( 1, 0, 0 ) : Vector3F( 0, 0, 1 );
                        const Vector3F across( along.z, 0, along.x );
                        const Vector3F up( 0, 1.2f, 0 ), toe( 0, .22f, 0 );
                        auto &wall = terrainBoundaries[axis * 2 + ( side > 0 )];
                        const auto segments =
                            std::clamp( int( std::ceil( boundary * 2.f / 4.f ) ), 1, 512 );
                        Vector3F lastLeft, lastRight;
                        for( int i = 0; i <= segments; ++i )
                        {
                            const auto centre = p + along * ( boundary * ( 2.f * i / segments - 1.f ) );
                            auto left = centre - across * .5f, right = centre + across * .5f;
                            if( !render::sampleTerrainHeight( *terrain, placement, left, left.y ) ||
                                !render::sampleTerrainHeight( *terrain, placement, right, right.y ) )
                            {
                                wall = Geometry{};
                                break;
                            }
                            if( i )
                            {
                                wall.quad( lastLeft + up, left + up, right + up, lastRight + up,
                                           { 0, 1, 0 } );
                                wall.quad( lastLeft - toe, left - toe, left + up, lastLeft + up,
                                           -across );
                                wall.quad( right - toe, lastRight - toe, lastRight + up, right + up,
                                           across );
                            }
                            lastLeft = left;
                            lastRight = right;
                        }
                        if( !wall.indices.empty() )
                        {
                            continue;
                        }
                    }
                    const Vector3F size =
                        axis ? Vector3F( boundary * 2, 1.2f, 1 ) : Vector3F( 1, 1.2f, boundary * 2 );
                    pavement.box( p, size );
                    reserveOwnerSlot( assets.actors );
                    auto wall = app->getGameManager()->createActor();
                    if( !wall )
                    {
                        throw std::runtime_error( "Cannot create procedural actor" );
                    }
                    assets.actors.push_back( wall );
                    wall->setName( "City boundary" );
                    wall->setStatic( true );
                    wall->setPosition( p );
                    wall->addComponent<CollisionBox>()->setExtents( size );
                    wall->addComponent<Rigidbody>();
                }
            }
            if( !assets.cityTerrainRoads )
            {
                mesh( assets, "City streets", asphalt, cityAsphalt );
            }
            auto pavingMaterial = material( assets, { .54f, .53f, .48f, 1 }, .92f );
            if( !applyCitySurface( assets, pavingMaterial, CitySurface::Paving, surfaceParams.size, .45f,
                                   1 ) )
            {
                auto paving = forge->bakeSurface( SurfaceTag::Concrete, surfaceParams );
                maps( assets, pavingMaterial, paving.albedo, paving.normal, paving.orm );
                pavingMaterial->setDiffuse( { .54f, .53f, .48f, 1 } );
                pavingMaterial->setUVProjection( 1 );
                pavingMaterial->setTriplanarScale( .5f );
                pavingMaterial->setNormalStrength( .12f );
                pavingMaterial->setEditorFloat( "aoStrength", .3f );
            }
            mesh( assets, "City sidewalks", pavement, pavingMaterial );
            for( auto &wall : terrainBoundaries )
            {
                if( auto actor = mesh( assets, "Terrain city boundary", wall, pavingMaterial ) )
                {
                    actor->setStatic( true );
                    auto collision = actor->addComponent<CollisionMesh>();
                    if( !collision )
                    {
                        throw std::runtime_error( "Cannot create city boundary collision." );
                    }
                    auto resource = actor->getComponent<scene::Mesh>()->getMeshResource();
                    collision->setConvex( false );
                    collision->setMeshResource( resource );
                    if( auto shape = dynamic_pointer_cast<physics::IMeshShape>( collision->getShape() ) )
                    {
                        shape->setMeshResource( resource );
                    }
                    if( !actor->addComponent<Rigidbody>() )
                    {
                        throw std::runtime_error( "Cannot create city boundary rigidbody." );
                    }
                }
                wall = Geometry{};
            }
            if( !assets.cityTerrainRoads )
            {
                mesh( assets, "City lane markings", markings,
                      material( assets, { .85f, .82f, .65f, 1 }, .8f ) );
            }
            const ColourF colours[] = { { .78f, .68f, .57f, 1 },
                                        { .86f, .83f, .75f, 1 },
                                        { .58f, .65f, .69f, 1 } };
            const SurfaceTag facadeSurfaces[] = { SurfaceTag::Brick, SurfaceTag::Plaster,
                                                  SurfaceTag::Concrete };
            std::array<SmartPtr<render::IMaterial>, 3> facades;
            const CitySurface facadeFamilies[] = { CitySurface::Brick, CitySurface::Plaster,
                                                   CitySurface::Concrete };
            for( int i = 0; i < 3; ++i )
            {
                facades[i] = material( assets, colours[i], 1, 1 );
                if( applyCitySurface( assets, facades[i], facadeFamilies[i], surfaceParams.size,
                                      i == 0   ? .6f
                                      : i == 1 ? .12f
                                               : .25f,
                                      1 ) )
                {
                    // Scanned albedo already carries the surface colour. Restrained
                    // architectural tints preserve reflectance and mortar contrast.
                    facades[i]->setDiffuse( i == 1   ? ColourF( .96f, .94f, .90f, 1 )
                                            : i == 2 ? ColourF( .93f, .96f, 1, 1 )
                                                     : ColourF::White );
                    if( i == 1 )
                    {
                        facades[i]->setEditorFloat( "aoStrength", .25f );
                    }
                    continue;
                }
                // Complete bakes supply encoded albedo and linear normal/ORM data.
                // Each architectural kit has a different surface and physical scale.
                auto surface = forge->bakeSurface( facadeSurfaces[i], surfaceParams );
                if( surface.albedo.pixels.empty() || surface.normal.pixels.empty() ||
                    surface.orm.pixels.empty() )
                {
                    throw std::runtime_error( "City facades require complete PBR maps." );
                }
                if( i != 0 )
                {
                    // Large mineral panels need restrained weathering. The forge's
                    // full-range cracked plaster otherwise reads as mottled rock.
                    const float base = i == 1 ? .58f : .42f;
                    for( size_t pixel = 0; pixel < surface.albedo.pixels.size(); pixel += 4 )
                    {
                        for( size_t channel = 0; channel < 3; ++channel )
                        {
                            const float encoded = surface.albedo.pixels[pixel + channel] / 255.f;
                            const float source = encoded <= .04045f
                                                     ? encoded / 12.92f
                                                     : std::pow( ( encoded + .055f ) / 1.055f, 2.4f );
                            const float linear = base * .85f + source * .15f;
                            surface.albedo.pixels[pixel + channel] = u8(
                                std::clamp( ( 1.055f * std::pow( linear, 1.f / 2.4f ) - .055f ) * 255,
                                            0.f, 255.f ) );
                        }
                    }
                }
                maps( assets, facades[i], surface.albedo, surface.normal, surface.orm );
                facades[i]->setDiffuse( colours[i] );
                // Box projection uses facade height as well as width; XZ projection
                // had collapsed vertical walls into stretched texture columns.
                facades[i]->setUVProjection( 1 );
                facades[i]->setNormalStrength( i == 0 ? .2f : .1f );
                // Height-derived micro AO must not blanket-darken whole flat walls.
                facades[i]->setEditorFloat( "aoStrength", .35f );
                facades[i]->setEditorUInt( "uvFilter", 3 );
                facades[i]->setEditorFloat( "uvAniso", 8.f );
                facades[i]->setEditorFloat( "uvTriplanarScale", i == 0 ? 1.f : .25f );
            }
            const auto roofMaterial = material( assets, { .18f, .21f, .23f, 1 }, .78f );
            const auto frameMaterial = material( assets, { .68f, .67f, .61f, 1 }, .8f );
            const auto storefrontMetal = material( assets, { .37f, .39f, .40f, 1 }, .53f, .32f );
            const auto accentMaterial = material( assets, { .18f, .28f, .30f, 1 }, .55f );
            const auto timberMaterial = material( assets, { .24f, .15f, .085f, 1 }, .83f );
            const auto tileMaterial = material( assets, { .57f, .24f, .13f, 1 }, .82f );
            const auto [glassMaterials, paneMaterial] =
                cityBuildingGlazing::build( assets, detailedBuildings );
            const auto shops =
                emittedShopfronts ? cityShopfront::materials( assets ) : cityShopfront::Materials{};
            const auto interiors = emittedDisplayBays ? cityShopInterior::materials( assets )
                                                      : cityShopInterior::Materials{};
            const auto masonry = emittedMasonry ? cityFacadeDetail::materials( assets, facades[2], seed )
                                                : cityFacadeDetail::Materials{};
            const auto roofFinish =
                emittedRoofFinish ? cityRoofDeck::materials( assets, seed ) : cityRoofDeck::Materials{};
            const auto roofEquipment = emittedRoofEquipment ? cityRoofEquipment::materials( assets )
                                                            : cityRoofEquipment::Materials{};
            const auto pitchedRoof = emittedPitchedRoofs
                                         ? cityPitchedRoofCladding::materials( assets, seed )
                                         : cityPitchedRoofCladding::Materials{};
            const auto roofDrainage = emittedRoofDrainage ? cityRoofDrainage::materials( assets )
                                                          : cityRoofDrainage::Materials{};
            for( auto &[key, patch] : patches )
            {
                reserveOwnerSlot( assets.actors );
                auto root = app->getGameManager()->createActor();
                if( !root )
                {
                    throw std::runtime_error( "Cannot create city LOD patch." );
                }
                assets.actors.push_back( root );
                root->setName( "City LOD patch" );
                root->setPosition( { key.first * 128.f + 64, 0, key.second * 128.f + 64 } );
                SmartPtr<scene::MeshRenderer> imposter;
                if( assets.cityStreamingEnabled && !patch.customDetailOutsideFarMesh )
                {
                    imposter = cityBuildingImposters::build( assets, patch, root, quality );
                }
                const bool deferDetailNative =
                    imposter && assets.cityStreamingEnabled && !assets.cityRoadShowcase;
                auto nearRenderers = makeSourceBudgetSnapshot<SmartPtr<scene::Renderer>>();
                auto farRenderers = makeSourceBudgetSnapshot<SmartPtr<scene::Renderer>>();
                const auto add = [&]( const String &name, Geometry &geometry,
                                      SmartPtr<render::IMaterial> surface, auto &renderers,
                                      bool shadows = true ) {
                    if( auto actor = mesh( assets, name, geometry, surface, root, deferDetailNative ) )
                    {
                        auto renderer = actor->getComponent<scene::MeshRenderer>();
                        if( !shadows )
                        {
                            renderer->setCastShadows( scene::Renderer::CastShadows::Off );
                        }
                        renderers.push_back( renderer );
                    }
                    // MeshUtil owns a packed copy now. Release grown staging arrays
                    // before publishing the next patch instead of doubling all city
                    // vertex/index storage until the end of this function.
                    geometry = Geometry{};
                };
                if( quality != VehicleAppearanceQuality::Preview )
                {
                    for( int family = 0; family < 3; ++family )
                    {
                        add( "LOD0 city facade", patch.walls[family], facades[family], nearRenderers );
                    }
                    for( int glass = 0; glass < 3; ++glass )
                    {
                        add( "LOD0 city glazing", patch.glass[glass], glassMaterials[glass],
                             nearRenderers );
                    }
                    add( "LOD0 city glazing", patch.windowPanes, paneMaterial, nearRenderers );
                    add( "LOD0 city roof detail", patch.roofs, roofMaterial, nearRenderers );
                    add( "LOD0 city rooftop membrane", patch.finishedRoofs.membrane, roofFinish.membrane,
                         nearRenderers );
                    add( "LOD0 city rooftop seams", patch.finishedRoofs.seams, roofFinish.seams,
                         nearRenderers );
                    add( "LOD0 city rooftop coping", patch.finishedRoofs.coping, roofFinish.coping,
                         nearRenderers );
                    add( "LOD0 city rooftop drains", patch.finishedRoofs.drains, roofFinish.drains,
                         nearRenderers );
                    add( "LOD0 city rooftop mechanical curbs", patch.mechanicalRoofs.curbs,
                         roofEquipment.curbs, nearRenderers );
                    add( "LOD0 city rooftop equipment housings", patch.mechanicalRoofs.housings,
                         roofEquipment.housings, nearRenderers );
                    add( "LOD0 city rooftop equipment metal", patch.mechanicalRoofs.metal,
                         roofEquipment.metal, nearRenderers );
                    add( "LOD0 city rooftop equipment grilles", patch.mechanicalRoofs.grilles,
                         roofEquipment.grilles, nearRenderers );
                    add( "LOD0 city pitched roof slate", patch.pitchedRoofs.slate, pitchedRoof.slate,
                         nearRenderers );
                    add( "LOD0 city pitched roof clay", patch.pitchedRoofs.clay, pitchedRoof.clay,
                         nearRenderers );
                    add( "LOD0 city pitched roof slate caps", patch.pitchedRoofs.slateCaps,
                         pitchedRoof.slateCaps, nearRenderers );
                    add( "LOD0 city pitched roof clay caps", patch.pitchedRoofs.clayCaps,
                         pitchedRoof.clayCaps, nearRenderers );
                    add( "LOD0 city pitched roof eaves", patch.pitchedRoofs.eaves, pitchedRoof.eaves,
                         nearRenderers );
                    add( "LOD0 city pitched roof flashing", patch.pitchedRoofs.flashing,
                         pitchedRoof.flashing, nearRenderers );
                    add( "LOD0 city pitched roof gutters", patch.roofDrainage.gutters,
                         roofDrainage.gutters, nearRenderers );
                    add( "LOD0 city pitched roof hardware", patch.roofDrainage.hardware,
                         roofDrainage.hardware, nearRenderers );
                    add( "LOD0 city pitched roof pipes", patch.roofDrainage.pipes, roofDrainage.pipes,
                         nearRenderers );
                    add( "LOD0 city facade frames", patch.frames, frameMaterial, nearRenderers );
                    add( "LOD0 city masonry panels", patch.masonry.panels, masonry.stone,
                         nearRenderers );
                    add( "LOD0 city masonry sill and head profiles", patch.masonry.profiles,
                         masonry.dressed, nearRenderers );
                    add( "LOD0 city masonry capped mullions", patch.masonry.mullions, masonry.metal,
                         nearRenderers );
                    add( "LOD0 city storefront joinery", patch.officeJoinery, storefrontMetal,
                         nearRenderers );
                    for( size_t style = 0; style < shops.fascia.size(); ++style )
                    {
                        add( "LOD0 city commercial fascia and canopies", patch.shopfrontFascia[style],
                             shops.fascia[style], nearRenderers );
                    }
                    add( "LOD0 city commercial lettering", patch.shopfrontLetters, shops.lettering,
                         nearRenderers );
                    add( "LOD0 city commercial window displays", patch.shopfrontDisplays, shops.display,
                         nearRenderers );
                    add( "LOD0 city commercial display room lining", patch.shopInterior.lining,
                         interiors.lining, nearRenderers, false );
                    add( "LOD0 city commercial display room rear wall", patch.shopInterior.rearWall,
                         interiors.rearWall, nearRenderers, false );
                    add( "LOD0 city commercial display room timber", patch.shopInterior.timber,
                         interiors.timber, nearRenderers, false );
                    add( "LOD0 city commercial display room fittings", patch.shopInterior.fittings,
                         interiors.fittings, nearRenderers, false );
                    add( "LOD0 city commercial display room exhibits", patch.shopInterior.display,
                         interiors.display, nearRenderers, false );
                    add( "LOD0 city commercial display room lights", patch.shopInterior.luminaires,
                         interiors.luminaires, nearRenderers, false );
                    add( "LOD0 city commercial display glazing", patch.shopInterior.glazing,
                         interiors.glazing, nearRenderers, false );
                    add( "LOD0 city entrance fascia", patch.accents, accentMaterial, nearRenderers );
                    add( "LOD0 city timber details", patch.timber, timberMaterial, nearRenderers );
                    add( "LOD0 city terracotta roofs", patch.tiles, tileMaterial, nearRenderers );
                }
                for( int family = 0; family < 3; ++family )
                {
                    add( "LOD1 city massing", patch.farWalls[family], facades[family], farRenderers );
                }
                add( "LOD1 city roof silhouette", patch.farRoofs, roofMaterial, farRenderers );
                add( "LOD1 city pitched roof slate", patch.farPitchedRoofs.slate, pitchedRoof.slate,
                     farRenderers );
                add( "LOD1 city pitched roof clay", patch.farPitchedRoofs.clay, pitchedRoof.clay,
                     farRenderers );
                add( "LOD1 city terracotta silhouette", patch.farTiles, tileMaterial, farRenderers );
                add( "LOD1 city glazing ribbons", patch.farGlass, glassMaterials[0], farRenderers );
                auto lod = root->addComponent<scene::LODGroup>();
                lod->setSize( 128 );
                lod->setDetailBounds( patch.bounds );
                lod->setHysteresis( .18f );
                lod->setCullBelowLastLOD( false );
                if( assets.cityStreamingEnabled && patch.customDetailOutsideFarMesh )
                {
                    lod->setForcedLOD( 0 );
                }
                if( !nearRenderers.empty() )
                {
                    lod->addLevel( .11f, nearRenderers );
                }
                lod->addLevel( imposter ? cityBuildingImposters::minimumFarMeshCoverage : 0.f,
                               farRenderers );
                if( imposter )
                {
                    auto imposterRenderers = makeSourceBudgetSnapshot<SmartPtr<scene::Renderer>>();
                    imposterRenderers.push_back( imposter );
                    lod->addLevel( 0.f, imposterRenderers );
                }
                String error;
                if( !lod->validate( &error ) )
                {
                    throw std::runtime_error( "Invalid city LOD patch: " + error );
                }
            }
            mesh( assets, "City block plazas", courtyards, pavingMaterial );
            if( assets.cityTerrainRoads )
            {
                CityRoadTerrainMesh fitted;
                String error;
                const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
                const auto roadFootways = assets.roadAuthoring->getRuntimeFootways();
                const bool sharedFootways =
                    roadFootways && terrain &&
                    roadFootways->sourceRevision == assets.roadAuthoring->getActiveRevision() &&
                    roadFootways->terrainRevision == terrain->revision;
                if( sharedFootways )
                {
                    fitted = *roadFootways;
                }
                else if( !buildCityRoadFootways(
                             assets.roadAuthoring->getSource(), assets.roadAuthoring->getBake(), terrain,
                             assets.roadAuthoring->getTerrainPlacement(), fitted, error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                const auto walks =
                    cityFootwayAppearance::prepare( assets, fitted, pavingMaterial, seed );
                if( auto actor = publishMesh( assets, "Terrain city footways", walks.mesh,
                                              fitted.indices.size() / 3, walks.slab ) )
                {
                    // One native concave mesh shares every fitted top/kerb face;
                    // no per-paving-tile actor or simplified flat contact plane.
                    if( walks.curb )
                    {
                        auto stone = actor->addComponent<scene::Material>();
                        if( !stone )
                        {
                            throw std::runtime_error(
                                "Cannot create terrain-city kerb material binding." );
                        }
                        stone->setIndex( 1 );
                        stone->setMaterial( walks.curb );
                    }
                    actor->setStatic( true );
                    auto collision = actor->addComponent<CollisionMesh>();
                    if( !collision )
                    {
                        throw std::runtime_error( "Cannot create terrain-city footway collision." );
                    }
                    auto resource = actor->getComponent<scene::Mesh>()->getMeshResource();
                    collision->setConvex( false );
                    collision->setMeshResource( resource );
                    if( auto shape = dynamic_pointer_cast<physics::IMeshShape>( collision->getShape() ) )
                    {
                        shape->setMeshResource( resource );
                    }
                    if( !actor->addComponent<Rigidbody>() )
                    {
                        throw std::runtime_error( "Cannot create terrain-city footway rigidbody." );
                    }
                }
                if( sharedFootways )
                {
                    cityCrossingLandings::build( assets, fitted );
                    cityStormDrains::build( assets, fitted, seed, quality );
                }
                SourceBudgetVector<CityStreetFixture> installedFurniture;
                buildTerrainCityStreetDressing( assets, fitted, terrainStreetLamps, quality,
                                                installedFurniture );
                cityCrossingBollards::build( assets, fitted, installedFurniture, terrainStreetLamps,
                                             quality );
                cityCarriagewayUtilities::build( assets, fitted, seed, quality );
                cityRoundaboutLandscape::build( assets, seed );
                SourceBudgetVector<cityParkLandscape::Layout> parkLayouts;
                SourceBudgetVector<cityReserveLandscape::Site> reserveSites;
                {
                    const auto layers = cityCarriagewayUtilities::activeLayers( assets );
                    const cityRoadEdges::Surface roadSurface( layers.road );
                    SourceBudgetVector<cityRoadEdges::Bounds> pedestrianCorridors;
                    cityGroundContact::build( assets, buildingEntries, fitted, pavingMaterial, forge,
                                              layers.road ? &roadSurface : nullptr, walks.mesh, seed,
                                              pedestrianCorridors );
                    cityEntranceWayfinding::build( assets, entranceSigns );
                    cityRoundaboutSigns::build( assets, fitted, terrainStreetLamps, quality );
                    cityRoadEdges::buildGutters( assets, fitted, roadSurface, layers.roadMaterial );
                    cityKerbCoping::build( assets, fitted, roadSurface, walks.curb, seed );
                    cityPavementRelief::build( assets, fitted, walks.slab, seed );
                    cityRoadLaneWear::build( assets, fitted, layers, seed, quality );
                    buildTerrainCityVergeTransitions( assets, fitted, seed, quality, roadSurface );
                    parkLayouts =
                        cityParkLandscape::build( assets, city, fitted, walks.mesh, roadSurface,
                                                  pavingMaterial, seed, surfaceParams.size, quality );
                    if( layers.road )
                    {
                        reserveSites = cityReserveLandscape::build(
                            assets, city, fitted, walks.mesh, roadSurface, buildingEntries, parkLayouts,
                            pedestrianCorridors, seed, quality );
                    }
                }
                cityParkAmenities::build( assets, parkLayouts, seed );
                buildTerrainCityVerges( assets, fitted, seed, quality );
                buildFoliageWorld( assets, seed, quality, &city, &fitted, &parkLayouts, &reserveSites );
            }
            mesh( assets, "City street furniture", streetMetal,
                  material( assets, { .16f, .19f, .20f, 1 }, .48f, .6f ) );
            mesh( assets, "City lamp lenses", lampGlass,
                  material( assets, { .80f, .82f, .76f, 1 }, .35f ) );
            auto land = material( assets, { .34f, .48f, .25f, 1 }, 1 );
            if( !applyCitySurface( assets, land, CitySurface::Ground, surfaceParams.size, .5f, 3 ) )
            {
                auto soil = forge->bakeSurface( SurfaceTag::Dirt, surfaceParams );
                maps( assets, land, soil.albedo, soil.normal, soil.orm );
                land->setDiffuse( { .34f, .48f, .25f, 1 } );
                land->setUVProjection( 3 );
                land->setTriplanarScale( .1f );
                land->setNormalStrength( .15f );
                land->setEditorFloat( "aoStrength", .4f );
            }
            mesh( assets, "City parks", parks, land );
            buildCityStreetDressing( assets, city, quality );
            buildCityLayoutDebug(
                assets, city, assets.cityTerrainRoads && city.naturalRoads ? &terrainBlocks : nullptr );
            Geometry ground;
            ground.box( { 0, -.06f, 0 }, { city.extent * 2 + 80, .1f, city.extent * 2 + 80 } );
            if( !city.naturalRoads && !assets.cityTerrainRoads )
            {
                mesh( assets, "City ground", ground, material( assets, { .28f, .30f, .26f, 1 }, 1 ) );
            }
            // Bounded driving remains inside the collision walls. A surrounding ground
            // surface gives the skyline context instead of ending the city in the sky.
            Geometry hinterland;
            if( assets.cityTerrainRoads )
            {
                const auto snapshot = assets.roadAuthoring->getTerrainSnapshot();
                if( snapshot )
                {
                    cityHinterland::build( hinterland, *snapshot,
                                           assets.roadAuthoring->getTerrainPlacement(), city.extent,
                                           seed, quality );
                }
            }
            else
            {
                const float horizon = city.extent + 1400;
                hinterland.quad( { -horizon, -.12f, -horizon }, { horizon, -.12f, -horizon },
                                 { horizon, -.12f, horizon }, { -horizon, -.12f, horizon },
                                 { 0, 1, 0 } );
            }
            if( city.naturalRoads && !assets.cityTerrainRoads )
            {
                mesh( assets, "City ground", ground, land );
            }
            mesh( assets, "City surrounding landscape", hinterland, land );
            if( assets.useFoliageWorld && !assets.cityTerrainRoads )
            {
                buildFoliageWorld( assets, seed, quality, &city );
            }
            assets.circuit = {};
            for( size_t i = 0; i < city.route.size(); ++i )
            {
                const auto p = city.route[i], next = city.route[( i + 1 ) % city.route.size()];
                const auto position = assets.cityTerrainRoads ? fittedRoute[i] : Vector3F( p.x, 0, p.z );
                const auto nextPosition = assets.cityTerrainRoads
                                              ? fittedRoute[( i + 1 ) % fittedRoute.size()]
                                              : Vector3F( next.x, 0, next.z );
                Vector3F forward = nextPosition - position;
                forward.normalise();
                assets.circuit.samples.push_back(
                    { position, { -forward.z, 0, forward.x }, assets.circuit.length } );
                assets.circuit.length += ( nextPosition - position ).length();
            }
            Geometry arrows;
            for( size_t i = 0; i < assets.circuit.samples.size(); i += 20 )
            {
                const auto &sample = assets.circuit.samples[i];
                const auto forward = Vector3F( sample.right.z, 0, -sample.right.x );
                const auto p = sample.position + Vector3F( 0, .04f, 0 );
                const auto tip = p + forward * 2;
                arrows.quad( p - sample.right * 1.5f, p + sample.right * 1.5f, tip + sample.right * .01f,
                             tip - sample.right * .01f, { 0, 1, 0 } );
            }
            buildCityTraffic( assets, city, vehicle, seed );
            if( !assets.cityTerrainRoads )
            {
                mesh( assets, "City race route arrows", arrows,
                      material( assets, { .05f, .7f, .9f, 1 }, .6f ) );
                assets.cityLayout = std::move( city );
            }
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
