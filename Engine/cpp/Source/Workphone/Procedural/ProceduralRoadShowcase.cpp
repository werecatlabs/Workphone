#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    namespace scene_build
    {
        void buildRoadAuthoringShowcase( SceneAssets &assets, SmartPtr<scene::IGameActor> vehicle, u32,
                                         VehicleAppearanceQuality )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto manager = app->getGameManager();
            auto scene = manager->getCurrentScene();
            if( !detail::runtimeActorPublished( vehicle ) || vehicle->getScene() != scene )
            {
                throw std::runtime_error(
                    "Attach the road showcase owner to the current scene before generating." );
            }
            const auto root = [&]( const String &name ) {
                reserveOwnerSlot( assets.actors );
                auto actor = manager->createActor();
                if( !actor )
                {
                    throw std::runtime_error( "Cannot create road authoring showcase root." );
                }
                assets.actors.push_back( actor );
                actor->setName( name );
                actor->setStatic( true );
                actor->setPosition( Vector3F::zero() );
                actor->setOrientation( QuaternionF::identity() );
                actor->setScale( Vector3F::unit() );
                actor->setFlag( IGameActor::ActorFlagDontSave, true );
                attachWorldActor( scene, vehicle, actor );
                return actor;
            };
            auto terrainActor = root( "Road authoring showcase terrain" );
            assets.ground = terrainActor;
            assets.roadTerrain = terrainActor->addComponent<TerrainSystem>();
            if( !assets.roadTerrain || !terrainActor->addComponent<CollisionTerrain>() ||
                !terrainActor->addComponent<Rigidbody>() )
            {
                throw std::runtime_error(
                    "Road showcase requires native terrain render and collision components." );
            }
            if( !assets.roadTerrain->isLoaded() )
            {
                assets.roadTerrain->load( nullptr );
            }
            auto terrainMaterial = terrainActor->addComponent<scene::Material>();
            if( terrainMaterial )
            {
                terrainMaterial->setMaterialPath( "DefaultTerrain.mat" );
            }
            assets.roadTerrain->setTreesEnabled( false );
            assets.roadTerrain->setGrassEnabled( false );
            render::TerrainData terrain;
            terrain.dimensions = { 97, 97 };
            terrain.spacing = { 2, 2 };
            terrain.origin = { -96, -96 };
            terrain.heights.resize( 97 * 97 );
            terrain.layers[0].tint = { .34f, .40f, .22f, 1 };
            terrain.layers[1].tint = { .30f, .25f, .17f, 1 };
            terrain.layerWeights.resize( 97 * 97 );
            terrain.appearance.macroScale = .025f;
            terrain.appearance.macroStrength = .22f;
            terrain.appearance.macroSeed = 42;
            terrain.appearance.macroTint = { .32f, .39f, .22f, 1 };
            for( int z = 0; z < 97; ++z )
            {
                for( int x = 0; x < 97; ++x )
                {
                    const float px = -96 + x * 2.f, pz = -96 + z * 2.f, r = std::hypot( px, pz ),
                                outside = std::clamp( ( r - 40 ) / 56.f, 0.f, 1.f );
                    terrain.heights[size_t( z ) * 97 + x] =
                        3 + .015f * px + .008f * pz +
                        2.1f * outside * outside * std::sin( px * .035f ) * std::cos( pz * .035f );
                    terrain.layerWeights[size_t( z ) * 97 + x] = { 1 - outside * .35f, outside * .35f, 0,
                                                                   0 };
                }
            }
            // This new root has no visible generation yet. Publish its initial CPU/
            // collision snapshot before the first deferred GPU upload can run.
            auto terrainRenderer =
                dynamic_pointer_cast<render::Terrain>( assets.roadTerrain->getTerrain() );
            const bool deferred = terrainRenderer && terrainRenderer->isTerrainPublicationDeferred();
            if( deferred )
            {
                terrainRenderer->setDeferredTerrainPublication( false );
            }
            String error;
            const bool terrainApplied = assets.roadTerrain->applyTerrainData( terrain, error );
            if( deferred )
            {
                terrainRenderer->setDeferredTerrainPublication( true );
            }
            if( !terrainApplied )
            {
                throw std::runtime_error( error.c_str() );
            }
            if( !assets.roadTerrain->getTerrain() )
            {
                throw std::runtime_error(
                    "Road showcase native terrain renderer is unavailable after component load." );
            }
            auto authoringActor = root( "Road authoring showcase source" );
            assets.roadAuthoring = authoringActor->addComponent<CityRoadAuthoring>();
            if( !assets.roadAuthoring ||
                !assets.roadAuthoring->bindTerrain( assets.roadTerrain, error ) )
            {
                throw std::runtime_error( error.empty() ? "Cannot bind showcase authoring terrain."
                                                        : error.c_str() );
            }
            const auto id = []( unsigned value ) {
                char text[37];
                std::snprintf( text, sizeof( text ), "f1a00000-2026-4000-8000-%012u", value );
                return SourceBudgetText( text );
            };
            procedural::CityRoadSplineSource source;
            source.version = 2;
            source.id = id( 1 );
            const auto binding = assets.roadAuthoring->getTerrainBindingUUID();
            source.terrainResourceUuid.assign( binding.data(), binding.size() );
            procedural::CityRoadRoundaboutSpec junction;
            junction.junctionId = id( 2 );
            junction.center = { 0, 3.025f, 0 };
            junction.outerRadius = 14;
            junction.islandRadius = 8;
            junction.clockwise = true;
            junction.gradeX = .015f;
            junction.gradeZ = .008f;
            junction.maxGrade = .08f;
            const auto snapshot = assets.roadTerrain->getTerrainSnapshot();
            for( unsigned i = 0; i < 4; ++i )
            {
                const Vector3F outward = i == 0   ? Vector3F( 1, 0, 0 )
                                         : i == 1 ? Vector3F( 0, 0, 1 )
                                         : i == 2 ? Vector3F( -1, 0, 0 )
                                                  : Vector3F( 0, 0, -1 );
                const Vector3F side( -outward.z, 0, outward.x );
                const float bend = i == 3 ? 1.5f : ( i % 2 ? 10.f : -10.f );
                procedural::CityRoadSpline road;
                road.id = id( 10 + i );
                road.width = 6;
                road.maxGrade = .08f;
                road.heightOffset = .025f;
                procedural::CityRoadRoundaboutPort port;
                port.approachId = road.id;
                port.outward = outward;
                port.width = 6;
                port.length = 18;
                junction.ports.push_back( port );
                auto start = outward * 32.f, end = outward * 85.f + side * bend;
                if( !snapshot ||
                    !render::sampleTerrainHeight( *snapshot, Transform3<real_Num>(), start, start.y ) ||
                    !render::sampleTerrainHeight( *snapshot, Transform3<real_Num>(), end, end.y ) )
                {
                    throw std::runtime_error( "Showcase spline endpoint escaped terrain coverage." );
                }
                auto out = outward * 18.f, in = -outward * 18.f - side * ( bend * .5f );
                out.y = .015f * out.x + .008f * out.z;
                in.y = .015f * in.x + .008f * in.z;
                road.knots.push_back( { id( 20 + i * 2 ), start, {}, out } );
                road.knots.push_back( { id( 21 + i * 2 ), end, in, {} } );
                source.roads.push_back( std::move( road ) );
            }
            source.roundabouts.push_back( junction );
            SourceBudgetText text;
            if( !procedural::serializeCityRoadSplineSource( source, text, error ) ||
                !assets.roadAuthoring->setSourceJSON( String( text.data(), text.size() ),
                                                      assets.roadAuthoring->getSourceRevision(),
                                                      error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            assets.triangles += 96 * 96 * 2 + assets.roadAuthoring->getRuntimeTriangleCount();
            assets.useFoliageWorld = false;
            assets.circuit = {};
            for( unsigned i = 0; i < 96; ++i )
            {
                const float direction = junction.clockwise ? 1.f : -1.f,
                            angle = direction * i * 2 * pi / 96,
                            next = direction * ( i + 1 ) * 2 * pi / 96, radius = 11;
                Vector3F p( std::cos( angle ) * radius, 0, std::sin( angle ) * radius ),
                    q( std::cos( next ) * radius, 0, std::sin( next ) * radius );
                bool pDriveable = false, qDriveable = false;
                if( !assets.roadAuthoring->getRuntimeSurface( p.x, p.z, p.y, pDriveable ) ||
                    !pDriveable ||
                    !assets.roadAuthoring->getRuntimeSurface( q.x, q.z, q.y, qDriveable ) ||
                    !qDriveable )
                {
                    throw std::runtime_error(
                        "Showcase circulation route left the published driveable roundabout surface." );
                }
                const auto forward = ( q - p ).normaliseCopy();
                assets.circuit.samples.push_back(
                    { p, { -forward.z, 0, forward.x }, assets.circuit.length } );
                assets.circuit.length += ( q - p ).length();
            }
            f32 spawnY = 0;
            if( !render::sampleTerrainHeight( *snapshot, Transform3<real_Num>(), { 0, 0, -60 },
                                              spawnY ) )
            {
                throw std::runtime_error( "Showcase spawn has no terrain." );
            }
            vehicle->setPosition( { 0, spawnY + .445f, -60 } );
            vehicle->updateTransform();
            if( auto rigidbody = vehicle->getComponent<Rigidbody>() )
            {
                if( auto body = rigidbody->getRigidDynamic() )
                {
                    body->setTransform( Transform3F( vehicle->getPosition(), QuaternionF::identity() ) );
                }
            }
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
