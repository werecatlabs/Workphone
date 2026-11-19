#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    namespace scene_build
    {
        void buildCityTraffic( SceneAssets &assets, const OpenCityLayout &city,
                               SmartPtr<IGameActor> player, u32 seed )
        {
            if( assets.cityTrafficDensity == 0 )
            {
                return;
            }
            auto app = core::IApplicationManager::instance();
            if( !app->getVehicleManager() )
            {
                WP_LOG( "City traffic requires a vehicle physics backend." );
                return;
            }

            procedural::CityRoadSplineSource flatSource;
            procedural::CityRoadSplineBake flatBake;
            procedural::CityRoadHeightSampler height = []( const Vector3F &, f32 &y ) {
                y = 0;
                return true;
            };
            const procedural::CityRoadSplineSource *source = nullptr;
            const procedural::CityRoadSplineBake *bake = nullptr;
            String error;
            if( assets.roadAuthoring )
            {
                source = &assets.roadAuthoring->getSource();
                bake = &assets.roadAuthoring->getBake();
                const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
                const auto placement = assets.roadAuthoring->getTerrainPlacement();
                if( !terrain )
                {
                    return;
                }
                height = [terrain, placement]( const Vector3F &point, f32 &y ) {
                    return render::sampleTerrainHeight( *terrain, placement, point, y );
                };
            }
            else
            {
                procedural::OpenCityRoadSourceOptions options;
                options.seed = seed;
                // The flat city's y=0 sampling plane is a stable virtual terrain
                // binding; spline validation still requires GUID provenance.
                options.sourceId = "10000000-0000-4000-8000-000000000001";
                options.terrainResourceUuid = "10000000-0000-4000-8000-000000000002";
                procedural::OpenCityRoadSourceResult generated;
                if( !procedural::buildOpenCityRoadSource( city, options, height, generated, error ) ||
                    !procedural::sampleCityRoadSplines( generated.source, {}, height, flatBake, error ) )
                {
                    WP_LOG( String( "City traffic source unavailable: " ) + error );
                    return;
                }
                flatSource = std::move( generated.source );
                source = &flatSource;
                bake = &flatBake;
            }

            scene::CityTrafficSettings settings;
            settings.densityPercent = std::min( assets.cityTrafficDensity, 100u );
            settings.maximumVehicles = 32;
            settings.driveOnLeft = assets.cityTrafficDriveOnLeft;
            auto traffic = std::make_shared<scene::CityTrafficSimulation>();
            if( player )
            {
                traffic->setExternalObstacle( player->getPosition(), 2.2f );
            }
            if( !traffic->initialize( *source, *bake, height, settings, seed, error ) )
            {
                WP_LOG( String( "City traffic unavailable: " ) + error );
                return;
            }
            assets.cityTraffic = traffic;
            if( traffic->vehicles().empty() )
            {
                return;
            }

            auto actorManager = app->getGameManager();
            auto meshManager = dynamic_pointer_cast<MeshManager>( app->getMeshManager() );
            if( !meshManager )
            {
                meshManager = make_ptr<MeshManager>();
                app->setMeshManager( meshManager );
            }
            const auto sharedMesh = [&]( const String &name, const Geometry &geometry ) {
                if( geometry.indices.empty() )
                {
                    throw std::runtime_error( "City traffic mesh has no triangles." );
                }
                auto generated =
                    MeshUtil::createMesh( geometry.positions, geometry.normals, geometry.tangents,
                                          geometry.uv, geometry.indices );
                generated->updateAABB( true );
                const auto path = assets.resourcePrefix + "/traffic/" + name + ".meshbin";
                reserveOwnerSlot( assets.meshes );
                auto resource =
                    dynamic_pointer_cast<IMeshResource>( meshManager->createOrRetrieve( path ).first );
                if( !resource )
                {
                    throw std::runtime_error( "Cannot create city traffic mesh resource." );
                }
                assets.meshes.push_back( resource );
                resource->setName( path );
                resource->setFilePath( path );
                resource->setMesh( generated );
                resource->setLoadingState( LoadingState::Loaded );
                assets.triangles += geometry.indices.size() / 3;
                return resource;
            };
            const auto visiblePart = [&]( const String &name, SmartPtr<IMeshResource> resource,
                                          SmartPtr<render::IMaterial> finish,
                                          SmartPtr<IGameActor> parent = nullptr ) {
                reserveOwnerSlot( assets.actors );
                auto actor = actorManager->createActor();
                if( !actor )
                {
                    throw std::runtime_error( "Cannot create city traffic actor." );
                }
                assets.actors.push_back( actor );
                actor->setName( name );
                if( parent )
                {
                    parent->addChild( actor );
                }
                actor->addComponent<scene::Mesh>()->setMeshResource( resource );
                actor->addComponent<scene::MeshRenderer>();
                actor->addComponent<scene::Material>()->setMaterial( finish );
                return actor;
            };

            std::array<SmartPtr<IMeshResource>, 3> shells, windows, fronts, rears;
            for( u32 style = 0; style < 3; ++style )
            {
                Geometry shell, glass, head, tail;
                cityTrafficAppearance::body( shell, style );
                cityTrafficAppearance::glazing( glass, style );
                cityTrafficAppearance::lamps( head, tail, style );
                const auto suffix = StringUtil::toString( style );
                shells[style] = sharedMesh( "body-" + suffix, shell );
                windows[style] = sharedMesh( "glass-" + suffix, glass );
                fronts[style] = sharedMesh( "front-lamp-" + suffix, head );
                rears[style] = sharedMesh( "rear-lamp-" + suffix, tail );
            }
            Geometry tireGeometry, hubGeometry;
            cityTrafficVehicle::wheelMeshes( tireGeometry, hubGeometry );
            const auto tires = sharedMesh( "tires", tireGeometry );
            const auto hubs = sharedMesh( "hubs", hubGeometry );
            const std::array<ColourF, 6> paintColours{
                ColourF( .60f, .64f, .67f, 1 ), ColourF( .08f, .11f, .15f, 1 ),
                ColourF( .76f, .77f, .72f, 1 ), ColourF( .06f, .20f, .38f, 1 ),
                ColourF( .36f, .08f, .10f, 1 ), ColourF( .56f, .39f, .14f, 1 )
            };
            std::array<SmartPtr<render::IMaterial>, 6> paints;
            for( size_t i = 0; i < paints.size(); ++i )
            {
                paints[i] = material( assets, paintColours[i], .28f, .27f );
            }
            const auto glazing = material( assets, { .075f, .125f, .15f, 1 }, .16f, .08f );
            const auto rubber = material( assets, { .025f, .027f, .03f, 1 }, .94f );
            const auto alloy = material( assets, { .39f, .42f, .43f, 1 }, .34f, .64f );
            const auto whiteLamp = material( assets, { .90f, .91f, .76f, 1 }, .17f );
            const auto redLamp = material( assets, { .65f, .045f, .035f, 1 }, .20f );

            assets.cityTrafficActors.reserve( traffic->vehicles().size() );
            for( const auto &pose : traffic->vehicles() )
            {
                const u32 style = pose.variant % 3;
                const auto label = "City traffic " + StringUtil::toString( pose.id );
                reserveOwnerSlot( assets.actors );
                auto car = actorManager->createActor();
                if( !car )
                {
                    throw std::runtime_error( "Cannot create city traffic vehicle actor." );
                }
                assets.actors.push_back( car );
                car->setName( label );
                car->setStatic( false );
                car->setPosition( pose.position + Vector3F( 0, cityTrafficAppearance::chassisLift, 0 ) );
                car->setOrientation( cityTrafficVehicle::orientation( pose.forward ) );
                auto shell = visiblePart( label + " body", shells[style],
                                          paints[pose.variant % paints.size()], car );
                shell->setLocalOrientation( cityTrafficVehicle::visualOrientation() );
                visiblePart( label + " windows", windows[style], glazing, shell );
                visiblePart( label + " headlights", fronts[style], whiteLamp, shell );
                visiblePart( label + " tail lamps", rears[style], redLamp, shell );
                for( u32 i = 0; i < 4; ++i )
                {
                    const auto wheelLabel = label + " wheel " + StringUtil::toString( i + 1 );
                    auto wheel = visiblePart( wheelLabel, tires, rubber, car );
                    wheel->setLocalPosition( cityTrafficVehicle::wheelPosition( i ) );
                    wheel->addComponent<scene::WheelController>();
                    visiblePart( wheelLabel + " alloy", hubs, alloy, wheel );
                }
                cityTrafficVehicle::configure( car, style );
                assets.cityTrafficActors.push_back( { pose.id, car } );
            }
            WP_LOG( "City traffic spawned " + StringUtil::toString( assets.cityTrafficActors.size() ) +
                    " FourWheelVehicle controllers." );
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
