#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::procedural_scene
{
    namespace scene_build
    {
#include "Workphone/Procedural/CityGroundAppearance.hpp"

        SourceBudgetVector<Vector3F> prepareTerrainCity( SceneAssets &assets, OpenCityLayout &city,
                                                         SmartPtr<IGameActor> sourceOwner,
                                                         SmartPtr<IGameActor> vehicle, u32 seed,
                                                         CityRoadBlockFaces &blockFaces )
        {
            auto app = core::IApplicationManager::instance();
            auto manager = app->getGameManager();
            auto scene = manager->getCurrentScene();

            struct NewAnchorGuard
            {
                SceneAssets &assets;
                SmartPtr<IGameManager> manager;
                SmartPtr<IGameActor> roads, terrain;
                bool accepted = false;

                ~NewAnchorGuard()
                {
                    if( accepted )
                    {
                        return;
                    }
                    // Failed initial grading must be retryable. Existing artist
                    // anchors are never enrolled in this temporary ownership.
                    if( roads )
                    {
                        assets.roadAuthoring = nullptr;
                    }
                    if( terrain )
                    {
                        assets.roadTerrain = nullptr;
                    }
                    for( auto actor : { roads, terrain } )
                    {
                        if( actor )
                        {
                            try
                            {
                                manager->destroyActor( actor );
                            }
                            catch( const std::exception &failure )
                            {
                                WP_LOG( failure.what() );
                            }
                        }
                    }
                }
            } newAnchors{ assets, manager };

            const auto persistentActor = [&]( const String &name, SmartPtr<IGameActor> &owned ) {
                if( !detail::runtimeActorPublished( sourceOwner ) || sourceOwner->getScene() != scene )
                {
                    throw std::runtime_error(
                        "Attach the city source owner to the current scene before generating terrain "
                        "roads." );
                }
                auto actor = manager->createActor();
                if( !actor )
                {
                    throw std::runtime_error( "Cannot create persistent city source actor." );
                }
                owned = actor;
                actor->setName( name );
                actor->setStatic( true );
                actor->setPosition( Vector3F::zero() );
                actor->setOrientation( QuaternionF::identity() );
                actor->setScale( Vector3F::unit() );
                attachWorldActor( scene, sourceOwner, actor );
                return actor;
            };
            String error;
            const bool newSource = !assets.roadAuthoring;
            CityRoadTerrainFitResult initialFit;
            render::TerrainSnapshot originalTerrain;
            if( newSource )
            {
                persistentActor( "Editable generated city roads", newAnchors.roads );
                assets.roadAuthoring = newAnchors.roads->addComponent<CityRoadAuthoring>();
            }
            if( !assets.roadAuthoring )
            {
                throw std::runtime_error( "Terrain city requires a canonical road source." );
            }
            if( !assets.roadTerrain )
            {
                auto actor = persistentActor( "Generated city terrain", newAnchors.terrain );
                assets.roadTerrain = actor->addComponent<TerrainSystem>();
                if( !assets.roadTerrain || !actor->addComponent<CollisionTerrain>() ||
                    !actor->addComponent<Rigidbody>() )
                {
                    throw std::runtime_error(
                        "Terrain city requires native terrain and collision components." );
                }
                if( !assets.roadTerrain->isLoaded() )
                {
                    assets.roadTerrain->load( nullptr );
                }
                assets.roadTerrain->setTreesEnabled( false );
                assets.roadTerrain->setGrassEnabled( false );
                const float radius = std::ceil( ( city.extent + 96 ) / 4 ) * 4;
                const u32 side = u32( radius * .5f ) + 1;
                render::TerrainData data;
                data.dimensions = { s32( side ), s32( side ) };
                data.spacing = { 4, 4 };
                data.origin = { -radius, -radius };
                data.heights.resize( size_t( side ) * side );
                const float terrainPhase = float( seed % 997 ) * ( 6.28318530718f / 997.f );
                const float hillScale = std::max( 1.f, assets.cityTerrainRelief );
                const auto cityRelief = [&]( float x, float z ) {
                    // Preserve broad hills across the centre. The fitting pass
                    // below grades only road corridors and local junction pads.
                    return assets.cityTerrainRelief *
                           ( 14.f * ( std::sin( x / ( 240.f * hillScale ) + terrainPhase ) *
                                          std::cos( z / ( 300.f * hillScale ) - terrainPhase ) -
                                      std::sin( terrainPhase ) * std::cos( terrainPhase ) ) +
                             5.f * ( std::sin( ( x + z ) / ( 180.f * hillScale ) + terrainPhase ) -
                                     std::sin( terrainPhase ) ) );
                };
                for( u32 z = 0; z < side; ++z )
                {
                    for( u32 x = 0; x < side; ++x )
                    {
                        const float px = -radius + x * 4, pz = -radius + z * 4;
                        const float outside = std::clamp(
                            ( std::max( std::abs( px ), std::abs( pz ) ) - city.extent - 20 ) / 76, 0.f,
                            1.f );
                        data.heights[size_t( z ) * side + x] =
                            2 + .005f * px + .003f * pz + cityRelief( px, pz ) +
                            outside * outside * 3 * std::sin( px * .019f ) * std::cos( pz * .017f );
                    }
                }
                cityGround::seedLayers( data, app->getProjectPath(), seed );
                // Prepare both candidates before publishing the new terrain.
                // The planar planning surface establishes XZ topology only;
                // the fitter derives road profiles and local pads from the hills.
                procedural::OpenCityRoadSourceOptions initialOptions;
                initialOptions.sourceId = assets.roadAuthoring->getSource().id;
                initialOptions.terrainResourceUuid = "00000000-0000-4000-8000-000000000001";
                initialOptions.seed = seed;
                initialOptions.maximumRoundabouts = 4;
                procedural::OpenCityRoadSourceResult planned;
                if( !procedural::buildOpenCityRoadSource(
                        city, initialOptions,
                        []( const Vector3F &, float &h ) {
                            h = 2;
                            return true;
                        },
                        planned, error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                cityGround::seedWeights( data, city.extent, seed, planned.source );
                CityRoadTerrainFitOptions fitOptions;
                originalTerrain = std::make_shared<render::TerrainData>( std::move( data ) );
                if( !prepareCityRoadTerrainFit( planned.source, originalTerrain, Transform3<real_Num>(),
                                                fitOptions, initialFit, error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                assets.cityTerrainFitted = true;
                assets.cityTerrainFitReport = initialFit.report;
                auto renderer =
                    dynamic_pointer_cast<render::Terrain>( assets.roadTerrain->getTerrain() );
                const bool deferred = renderer && renderer->isTerrainPublicationDeferred();
                if( deferred )
                {
                    renderer->setDeferredTerrainPublication( false );
                }
                const bool applied = assets.roadTerrain->applyTerrainData( *initialFit.terrain, error );
                if( deferred )
                {
                    renderer->setDeferredTerrainPublication( true );
                }
                if( !applied )
                {
                    throw std::runtime_error( error.c_str() );
                }
            }
            if( assets.roadAuthoring->getTerrain() != assets.roadTerrain &&
                !assets.roadAuthoring->bindTerrain( assets.roadTerrain, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            const auto &currentSource = assets.roadAuthoring->getSource();
            const bool seedSource =
                newSource || ( currentSource.roads.empty() && currentSource.roundabouts.empty() &&
                               currentSource.revision <= 2 );
            assets.citySourceReused = !seedSource;
            if( !seedSource && !assets.roadAuthoring->rebuild( error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
            const auto placement = assets.roadAuthoring->getTerrainPlacement();
            if( !terrain )
            {
                throw std::runtime_error( "Terrain city has no retained terrain snapshot." );
            }
            const auto height = [&]( const Vector3F &point, float &y ) {
                return render::sampleTerrainHeight( *terrain, placement, point, y );
            };
            SourceBudgetVector<Vector3F> route;
            if( seedSource )
            {
                procedural::OpenCityRoadSourceOptions options;
                options.seed = seed;
                options.sourceId = assets.roadAuthoring->getSource().id;
                const auto binding = assets.roadAuthoring->getTerrainBindingUUID();
                options.terrainResourceUuid.assign( binding.data(), binding.size() );
                options.terrainRevision = terrain->revision;
                options.maximumRoundabouts = 4;
                procedural::OpenCityRoadSourceResult generated;
                if( initialFit.terrain )
                {
                    generated.source = std::move( initialFit.source );
                    generated.source.terrainResourceUuid = options.terrainResourceUuid;
                    generated.source.terrainRevision = terrain->revision;
                    if( !procedural::fitOpenCityRoadRoute( city, generated.source, height,
                                                           generated.route, error ) )
                    {
                        throw std::runtime_error( error.c_str() );
                    }
                }
                else if( !procedural::buildOpenCityRoadSource( city, options, height, generated,
                                                               error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                SourceBudgetText json;
                if( !procedural::serializeCityRoadSplineSource( generated.source, json, error ) ||
                    !assets.roadAuthoring->setSourceJSON( String( json.data(), json.size() ),
                                                          assets.roadAuthoring->getSourceRevision(),
                                                          error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                if( originalTerrain &&
                    !assets.roadAuthoring->rememberInitialTerrainGrading( originalTerrain, error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                route = std::move( generated.route );
            }
            else
            {
                assets.cityRaceRouteAvailable = procedural::fitOpenCityRoadRoute(
                    city, assets.roadAuthoring->getSource(), height, route, error );
                if( !assets.cityRaceRouteAvailable && city.naturalRoads )
                {
                    // Saved sources precede the new district layout and do not
                    // store generator provenance or their pinned race path.
                    // Keep their edits and validate the exact previous path;
                    // never replace their roads or bypass surface coverage.
                    OpenCityLayout legacy;
                    legacy.route =
                        procedural::legacyOpenCityNaturalRoute( seed, city.blocks, assets.cityRoute );
                    String legacyError;
                    assets.cityRaceRouteAvailable = procedural::fitOpenCityRoadRoute(
                        legacy, assets.roadAuthoring->getSource(), height, route, legacyError );
                    if( assets.cityRaceRouteAvailable )
                    {
                        error.clear();
                        assets.cityRaceRouteError.clear();
                        WP_LOG( "Terrain city: retained the validated revision-2 natural race route." );
                    }
                    else
                    {
                        error += String( " Legacy route: " ) + legacyError;
                    }
                }
                if( !assets.cityRaceRouteAvailable )
                {
                    assets.cityRaceRouteError = error;
                }
            }
            if( route.size() > 1 && ( route.front() - route.back() ).lengthSquared() < 1e-8f )
            {
                route.pop_back();
            }
            OpenCityLayout supported;
            CityRoadParcelSupportOptions parcelOptions;
            parcelOptions.shrinkForRelief = true;
            if( !prepareCityRoadParcels( city, assets.roadAuthoring->getSource(),
                                         assets.roadAuthoring->getBake(), terrain, placement, supported,
                                         assets.cityCulledParcels, assets.cityBuildingSupportChecks,
                                         error, parcelOptions, seed, &blockFaces ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            city = std::move( supported );
            city.route.clear();
            for( const auto &point : route )
            {
                city.route.push_back( { point.x, point.z } );
            }
            assets.cityLayout = city;
            assets.cityAppliedSourceRevision = assets.roadAuthoring->getSourceRevision();
            assets.ground = assets.roadTerrain->getActor();
            assets.triangles += assets.roadAuthoring->getRuntimeTriangleCount() +
                                u64( terrain->dimensions.x - 1 ) * ( terrain->dimensions.y - 1 ) * 2;
            if( !vehicle )
            {
                newAnchors.accepted = true;
                return route;
            }
            Vector3F spawn = route.empty() ? Vector3F( 0, 0, 0 ) : route.front();
            bool driveable = false;
            float y = 0;
            if( !assets.roadAuthoring->getRuntimeSurface( spawn.x, spawn.z, y, driveable ) ||
                !driveable )
            {
                if( !height( spawn, y ) )
                {
                    throw std::runtime_error( "Edited city has no supported vehicle spawn." );
                }
            }
            spawn.y = y + .42f;
            vehicle->setPosition( spawn );
            vehicle->updateTransform();
            if( auto rigidbody = vehicle->getComponent<Rigidbody>() )
            {
                if( auto body = rigidbody->getRigidDynamic() )
                {
                    body->setTransform( Transform3F( spawn, vehicle->getOrientation() ) );
                }
            }
            newAnchors.accepted = true;
            return route;
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
