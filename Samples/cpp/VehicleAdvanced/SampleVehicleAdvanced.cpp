#include "SampleVehicleAdvanced.h"
#include "ProceduralScene.h"
#include "FrameCapture.h"
#include "ProductionSceneProfile.h"
#include <WPProcedural/WPProcedural.hpp>
#include <Workphone/Workphone.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Physics/RaycastHit.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>

#if WP_GRAPHICS_SYSTEM_CLAW
#    include <WPGraphics/ClawMesh.hpp>
#    include <WPGraphics/ClawRendererDX11.hpp>
#    include <workphone_graphics_renderer.h>
#    include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#    include <WPGraphics/ClawSceneNode.hpp>
#    include <WPGraphics/ClawScene.hpp>
#    include <workphone_graphics_object.h>
#endif

#ifdef _WP_STATIC_LIB_
#    include <FBOISInput/FBOISInput.hpp>
#    include <WPSQLite/WPSQLite.hpp>
#    include <WPVehiclePhysics/WPVehiclePhysics.hpp>

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.hpp>
#    endif

#    if WP_BUILD_PHYSX
#        include <FBPhysx/FBPhysx.hpp>
#    elif WP_BUILD_ODE
#        include <FBODE3/CPhysicsManagerODE.hpp>
#    endif
#endif

namespace workphone
{
    namespace
    {
        constexpr auto physicsTargetFps = 120.0;
        // Run the sample tasks on one executor so controls, reset and camera poses stay coherent.
        constexpr auto activeThreadCount = 0u;
        constexpr hash_type debugTextId = 0x56454800;
        constexpr real_Num vehicleSpawnHeight = static_cast<real_Num>( 0.42 );
        constexpr real_Num cameraDistance = static_cast<real_Num>( 8.0 );
        constexpr real_Num cameraHeight = static_cast<real_Num>( 3.0 );

        advanced::ProductionSceneProfile::Workload describeProfileScene( const advanced::SceneAssets &assets )
        {
            advanced::ProductionSceneProfile::Workload result;
            const auto unique = []( const auto &objects ) {
                u64 count = 0;
                for( size_t i = 0; i < objects.size(); ++i )
                {
                    if( !objects[i] ) continue;
                    bool first = true;
                    for( size_t j = 0; j < i; ++j )
                        if( objects[j].get() == objects[i].get() ) { first = false; break; }
                    count += first;
                }
                return count;
            };
            result.actors = unique( assets.actors );
            result.materials = unique( assets.materials );
            result.textures = unique( assets.textures );
            result.meshes = unique( assets.meshes );
            result.materialReferences = assets.materials.size();
            result.textureReferences = assets.textures.size();
            result.meshReferences = assets.meshes.size();
            result.triangles = assets.triangles;
            result.authoredTexturePayloadBytes = assets.textureBytes;
            result.treeLodGroups = assets.treeLODs.size();
            const auto hash = []( u64 &digest, const auto &value ) {
                advanced::ProductionSceneProfile::hashBytes( digest, &value, sizeof( value ) );
            };
            for( const auto &sample : assets.circuit.samples )
            {
                hash( result.circuitDigest, sample.position.x ); hash( result.circuitDigest, sample.position.y );
                hash( result.circuitDigest, sample.position.z ); hash( result.circuitDigest, sample.right.x );
                hash( result.circuitDigest, sample.right.y ); hash( result.circuitDigest, sample.right.z );
                hash( result.circuitDigest, sample.distance );
            }
            for( size_t resourceIndex = 0; resourceIndex < assets.meshes.size(); ++resourceIndex )
            {
                auto resource = assets.meshes[resourceIndex];
                bool first = true;
                for( size_t previous = 0; previous < resourceIndex; ++previous )
                    if( assets.meshes[previous].get() == resource.get() ) { first = false; break; }
                if( !first ) continue;
                if( !resource || !resource->getMesh() ) continue;
                for( auto section : resource->getMesh()->getSubMeshes() )
                {
                    ++result.meshSections;
                    auto vertices = section->getVertexBuffer();
                    auto indices = section->getIndexBuffer();
                    auto declaration = vertices->getVertexDeclaration();
                    const auto position = declaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
                    const auto normal = declaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
                    const u32 vertexCount = vertices->getNumVertices(), indexCount = indices->getNumIndices();
                    result.vertices += vertexCount; result.indices += indexCount;
                    hash( result.geometryDigest, vertexCount ); hash( result.geometryDigest, indexCount );
                    for( u32 i = 0; i < vertexCount; ++i )
                    {
                        for( auto element : { position, normal } )
                        {
                            if( !element ) continue;
                            const auto value = reinterpret_cast<const f32 *>(
                                static_cast<const u8 *>( vertices->getVertexData() ) +
                                size_t( i ) * declaration->getSize() + element->getOffset() );
                            for( u32 component = 0; component < 3; ++component ) hash( result.geometryDigest, value[component] );
                        }
                    }
                    for( u32 i = 0; i < indexCount; ++i )
                    {
                        const u32 index = indices->getIndexType() == IIndexBuffer::Type::IT_16BIT
                            ? static_cast<const u16 *>( indices->getIndexData() )[i]
                            : static_cast<const u32 *>( indices->getIndexData() )[i];
                        hash( result.geometryDigest, index );
                    }
                }
            }
            return result;
        }

        bool checkSceneWinding( const advanced::SceneAssets &assets )
        {
            size_t checked = 0;
            for( const auto &resource : assets.meshes )
            {
                for( const auto &section : resource->getMesh()->getSubMeshes() )
                {
                    double alignment = 0;
                    auto vertices = section->getVertexBuffer();
                    auto declaration = vertices->getVertexDeclaration();
                    auto position = declaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
                    auto normal = declaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
                    auto indices = section->getIndexBuffer();
                    if( !position || !normal || !indices || indices->getNumIndices() % 3 != 0 )
                        return false;
                    auto readVector = [&]( u32 index, u32 offset ) {
                        const auto data = reinterpret_cast<const f32 *>(
                            static_cast<const u8 *>( vertices->getVertexData() ) +
                            size_t( index ) * declaration->getSize() + offset );
                        return Vector3F( data[0], data[1], data[2] );
                    };
                    auto readIndex = [&]( u32 index ) -> u32 {
                        return indices->getIndexType() == IIndexBuffer::Type::IT_16BIT
                                   ? static_cast<const u16 *>( indices->getIndexData() )[index]
                                   : static_cast<const u32 *>( indices->getIndexData() )[index];
                    };
                    for( u32 i = 0; i < indices->getNumIndices(); i += 3 )
                    {
                        const auto a = readIndex( i ), b = readIndex( i + 1 ), c = readIndex( i + 2 );
                        if( a >= vertices->getNumVertices() || b >= vertices->getNumVertices() ||
                            c >= vertices->getNumVertices() )
                            return false;
                        const auto face = ( readVector( b, position->getOffset() ) -
                                            readVector( a, position->getOffset() ) )
                                              .crossProduct( readVector( c, position->getOffset() ) -
                                                             readVector( a, position->getOffset() ) );
                        const auto outward = readVector( a, normal->getOffset() ) +
                                             readVector( b, normal->getOffset() ) +
                                             readVector( c, normal->getOffset() );
                        // Smooth normals at narrow/concave car seams can oppose an individual
                        // face. Area-weighted section alignment detects a reversed upload without
                        // requiring every authored smooth normal to equal its triangle normal.
                        alignment += face.dotProduct( outward );
                        ++checked;
                    }
                    if( alignment <= 0 )
                    {
                        WP_LOG_ERROR( "VehicleAdvanced: reversed winding in " + resource->getFilePath() );
                        return false;
                    }
                }
            }
            WP_LOG( "VehicleAdvanced outward winding validated: triangles=" + StringUtil::toString( checked ) );
            return checked > 0;
        }

        struct TerrainCityCheck
        {
            bool valid=false,binding=false,revisions=false,renderer=false,route=false,islands=false,support=false,flatFallbackAbsent=false;
            size_t roads=0,junctions=0,triangles=0,routePoints=0,islandsChecked=0,rayProbes=0,buildingBodies=0,expectedBuildingBodies=0;
            f32 routeHeightError=0;
        };
        TerrainCityCheck checkTerrainCity(const advanced::SceneAssets &assets,SmartPtr<physics::IPhysicsScene3> physics)
        {
            TerrainCityCheck result;auto authoring=assets.roadAuthoring;auto terrain=assets.roadTerrain;
            if(!authoring || !terrain || !physics)return result;
            const auto sourceTerrain=terrain->getTerrainSnapshot(),activeTerrain=terrain->getActiveTerrainSnapshot();
            if(!sourceTerrain)return result;
            const auto &source=authoring->getSource();const auto &bake=authoring->getBake();const auto binding=authoring->getTerrainBindingUUID();
            const auto same=[](const SourceBudgetText &a,const String &b){return std::string_view(a.data(),a.size())==std::string_view(b.data(),b.size());};
            result.binding=authoring->getTerrain()==terrain && terrain->getHandle() && binding==terrain->getHandle()->getUUIDAsString() && same(source.terrainResourceUuid,binding) && !source.id.empty();
            result.revisions=authoring->getSourceRevision()==authoring->getActiveRevision() && assets.cityAppliedSourceRevision==authoring->getSourceRevision() && source.terrainRevision==sourceTerrain->revision;
            result.renderer=terrain->getTerrain() && activeTerrain && activeTerrain->revision==sourceTerrain->revision;
            result.roads=source.roads.size();result.junctions=source.roundabouts.size();result.triangles=authoring->getRuntimeTriangleCount();result.routePoints=assets.circuit.samples.size();
            result.route=assets.cityRaceRouteAvailable && result.routePoints>=3;
            for(size_t i=0;i<result.routePoints;++i)
            {
                const auto p=assets.circuit.samples[i].position,q=assets.circuit.samples[(i+1)%result.routePoints].position;f32 height=0;bool driveable=false;
                const bool hit=authoring->getRuntimeSurface(p.x,p.z,height,driveable);const auto step=(q-p).length();
                if(hit && std::isfinite(p.y))result.routeHeightError=std::max(result.routeHeightError,std::abs(p.y-height));
                result.route=result.route && hit && driveable && p.isFinite() && std::isfinite(step) && step>.0001f && std::abs(p.y-height)<=.001f;
            }
            result.islands=!source.roundabouts.empty();
            for(const auto &junction:source.roundabouts)
            {f32 height=0;bool driveable=true;const bool valid=authoring->getRuntimeSurface(junction.center.x,junction.center.z,height,driveable) && !driveable;if(valid)++result.islandsChecked;result.islands=result.islands && valid;}
            const auto ray=[&](Vector3F p,bool driveable,bool ground=false)
            {
                f32 height=0;bool actualDriveable=false;
                bool valid=ground?render::sampleTerrainHeight(*sourceTerrain,authoring->getTerrainPlacement(),p,height):authoring->getRuntimeSurface(p.x,p.z,height,actualDriveable);
                valid=valid && (ground || actualDriveable==driveable);Vector3F hit,normal;SmartPtr<ISharedObject> object;
                valid=valid && physics->intersects({p.x,height+.25f,p.z},{p.x,height-.25f,p.z},hit,normal,object) && object && std::isfinite(hit.y) && std::abs(hit.y-height)<.035f && normal.y>.5f;
                if(valid)++result.rayProbes;
            };
            for(const auto &road:bake.roads)if(!road.points.empty()){ray(road.points[road.points.size()/2].position,true);break;}
            if(!source.roundabouts.empty())
            {const auto &j=source.roundabouts.front();const float radius=(j.outerRadius+j.islandRadius)*.5f;ray(j.center+Vector3F(radius*.70710678f,0,radius*.70710678f),true);ray(j.center,false);}
            bool terrainProbe=false;const auto placement=authoring->getTerrainPlacement().getTransformationMatrix();
            for(const auto uv:{Vector2F(.02f,.02f),Vector2F(.98f,.02f),Vector2F(.02f,.98f),Vector2F(.98f,.98f),Vector2F(.5f,.02f),Vector2F(.5f,.98f)})
            {
                auto p=placement.transformAffine(Vector3F(sourceTerrain->origin.x+uv.x*(sourceTerrain->dimensions.X()-1)*sourceTerrain->spacing.x,0,sourceTerrain->origin.y+uv.y*(sourceTerrain->dimensions.Y()-1)*sourceTerrain->spacing.y));
                bool clear=true;for(const auto &lot:assets.cityLayout.lots)if(std::abs(p.x-lot.x)<lot.width*.5f+2 && std::abs(p.z-lot.z)<lot.depth*.5f+2){clear=false;break;}
                f32 height=0;bool driveable=false;if(authoring->getRuntimeSurface(p.x,p.z,height,driveable) && driveable)clear=false;
                if(clear){const auto before=result.rayProbes;ray(p,false,true);if(result.rayProbes>before){terrainProbe=true;break;}}
            }
            for(const auto &lot:assets.cityLayout.lots)result.expectedBuildingBodies+=procedural::OpenCityLayout::buildingVolumes(lot).size();
            result.flatFallbackAbsent=assets.ground && terrain->getActor()==assets.ground;
            for(const auto &actor:assets.actors)if(actor)
            {if(actor->getName().find("Trackside collision: City building mass")==0)++result.buildingBodies;if(actor->getName()=="Circuit collision plane")result.flatFallbackAbsent=false;}
            result.support=assets.cityBuildingSupportValid && assets.cityBuildingSupportChecks>=assets.cityLayout.lots.size() && result.buildingBodies==result.expectedBuildingBodies && result.buildingBodies>0;
            result.valid=result.binding && result.revisions && result.renderer && result.route && result.islands && result.support && result.flatFallbackAbsent && terrainProbe && result.rayProbes==4 && result.roads>0 && result.triangles>0;
            return result;
        }

        Vector3<real_Num> getVehicleSpawnPosition()
        {
            return Vector3<real_Num>::unitY() * vehicleSpawnHeight;
        }

        Vector3<real_Num> getInitialCameraPosition()
        {
            return getVehicleSpawnPosition() +
                   Vector3<real_Num>( static_cast<real_Num>( 0.0 ), cameraHeight, cameraDistance );
        }

        bool checkMeshTransforms( SmartPtr<scene::IGameActor> actor,
                                  const Transform3<real_Num> &expected )
        {
            if( auto renderer = actor->getComponent<scene::MeshRenderer>() )
            {
                auto node = renderer->getGraphicsNode();
                if( !node )
                    return false;

                const auto actual = node->getTransform();
                if( ( actual.getPosition() - expected.getPosition() ).length() > 0.01f ||
                    ( actual.getScale() - expected.getScale() ).length() > 0.01f ||
                    ( actual.forward() - expected.forward() ).length() > 0.01f ||
                    ( actual.up() - expected.up() ).length() > 0.01f )
                {
                    WP_LOG_ERROR( "Vehicle scene-node mismatch: " + actor->getName() +
                                  " position error=" + StringUtil::toString( ( actual.getPosition() - expected.getPosition() ).length() ) +
                                  " orientation error=" + StringUtil::toString( ( actual.forward() - expected.forward() ).length() ) );
                    return false;
                }

#if WP_GRAPHICS_SYSTEM_CLAW
                auto mesh = dynamic_pointer_cast<render::ClawMesh>( renderer->getGraphicsObject() );
                auto nativeNode = dynamic_pointer_cast<render::ClawSceneNode>( node );
                auto object = mesh ? mesh->getNativeRenderObject() : nullptr;
                if( !object || !nativeNode || !nativeNode->getNativeNode() ||
                    wp_graphics_object_get_owner( object ) != nativeNode->getNativeNode() ||
                    !mesh->getNativeMesh() ||
                    wp_graphics_mesh_get_vertex_count( mesh->getNativeMesh() ) == 0 )
                    return false;

                // Read the same owner matrix that the native mesh draw call uses.
                wp_mat4f rendered;
                wp_scenenode_get_world_matrix( wp_graphics_object_get_owner( object ), &rendered );
                Matrix4F expectedMatrix;
                expectedMatrix.makeTransform( expected.getPosition(), expected.getScale(),
                                              expected.getOrientation() );
                for( size_t i = 0; i < 16; ++i )
                    if( Math<real_Num>::Abs( rendered.m[i / 4][i % 4] - expectedMatrix.ptr()[i] ) >
                        0.01f )
                    {
                        WP_LOG_ERROR( "Vehicle render mismatch: " + actor->getName() +
                                      " matrix element " + StringUtil::toString( i ) +
                                      " actual=" + StringUtil::toString( rendered.m[i / 4][i % 4] ) +
                                      " expected=" + StringUtil::toString( expectedMatrix.ptr()[i] ) );
                        return false;
                    }
#endif
            }

            for( auto child : actor->getChildren() )
            {
                Transform3<real_Num> childWorld;
                childWorld.transformFromParent( expected, child->getLocalTransform() );
                if( !checkMeshTransforms( child, childWorld ) )
                    return false;
            }

            return true;
        }

    }  // namespace

    SampleVehicleAdvanced::SampleVehicleAdvanced()
    {
        setPluginsConfigFilePath( "wp_plugins_samples.cfg" );
    }

    SampleVehicleAdvanced::~SampleVehicleAdvanced()
    {
        unload( nullptr );
    }

    void SampleVehicleAdvanced::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Loading || loadingState == LoadingState::Loaded )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            Thread::setCurrentTask( TaskId::Primary );
            Thread::setCurrentThreadId( Thread::ThreadId::Primary );
            Thread::setTaskFlags( std::numeric_limits<u32>::max() );

            auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
            WP_ASSERT( applicationManager );

            applicationManager->load( data );
            core::IApplicationManager::setInstance( applicationManager );
            m_applicationManager = applicationManager;
            applicationManager->setApplication( this );

            Application::load( data );
#if WP_GRAPHICS_SYSTEM_CLAW
            if( m_sceneProfile )
            {
                auto graphics = dynamic_pointer_cast<render::ClawHammerSystem>( applicationManager->getGraphicsSystem() );
                if( !graphics ) throw std::runtime_error( "Scene profile requires the Claw graphics system." );
                graphics->setSceneProfilingEnabled( true );
#if defined( _WIN32 )
                // Enable timing during the existing warmup so lazily created GPU
                // query resources are resident before the warmed memory sample.
                // The measurement boundary still resets statistics separately.
                if( auto renderer = dynamic_cast<render::ClawRendererDX11 *>( graphics->getRendererPtr() ) )
                    wp_renderer_dx11_reset_statistics(
                        wp_renderer_get_dx11( renderer->getNativeRenderer() ) );
#endif
            }
#endif
            // Application::load logs scene exceptions instead of propagating them.
            // Do not enter Play with a partially generated vehicle/circuit.
            if( !m_raceScene || !m_raceScene->isGenerated() || (!m_assets.cityTerrainRoads && m_assets.circuit.samples.empty()) ||
                !m_cameraActor )
            {
                const auto error = m_raceScene ? m_raceScene->getGenerationError() : String();
                throw std::runtime_error( error.empty() ? "Vehicle scene initialization failed." : error.c_str() );
            }

            m_inputListener = workphone::make_ptr<InputListener>();
            m_inputListener->setOwner( this );

            auto inputManager = applicationManager->getInputDeviceManager();
            WP_ASSERT( inputManager );
            if( inputManager )
            {
                inputManager->addListener( m_inputListener );
            }

            auto taskManager = applicationManager->getTaskManager();
            WP_ASSERT( taskManager );
            if( auto physicsTask = taskManager->getTask( TaskId::Physics ) )
            {
                physicsTask->setTargetFPS( physicsTargetFps );
            }

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );
            applicationManager->setPlaying( true );
            applicationManager->setPaused( false );
            sceneManager->play();

            auto car = m_vehicleActor->getComponent<scene::CarController>();
            auto vehicleController = car->getVehicleController();
            m_raceScene->configurePhysics();

            m_raceScene->initializePresentation();
            m_reviewStart = -1;
            m_profileStart = 0;
            m_profileFrames = 0;
            m_reviewFrames = 0;
            m_benchmarkStarted = false;
            m_captureAttempted = false;
            m_capturePassed = false;
            m_replayTick = 0;
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            unload( data );
        }
    }

    void SampleVehicleAdvanced::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded || loadingState == LoadingState::Unloading )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );


            if( m_inputListener )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto inputManager = applicationManager->getInputDeviceManager() )
                    {
                        inputManager->removeListener( m_inputListener );
                    }
                }

                m_inputListener->unload( data );
                m_inputListener = nullptr;
            }

            m_boxGround = nullptr;
            m_cameraActor = nullptr;
            m_vehicleActor = nullptr;
            m_chassisMeshActor = nullptr;
            for( auto &wheelActor : m_wheelActors )
                wheelActor = nullptr;
            m_cameraController = nullptr;
            m_raceScene = nullptr;
            m_assets = {};
            m_cityExtent = m_citySpacing = 0;
            m_cityOverviewExtent = 0;
            m_cityOverviewCentre = m_cityStreetCentre = Vector3F::zero();
            m_cityStreetForward = { 0, 0, -1 };
            m_cityHasJunction=false;m_cityJunctionCentre=m_cityBoulevardCentre={};m_generatedSpawnPosition={0,.42f,0};
            m_cityRetentionSmokePrepared=false;SourceBudgetText{}.swap(m_cityRetentionSmokeRoadId);m_cityRetentionSmokeWidth=0;

            if( core::IApplicationManager::instance() )
            {
                Application::unload( data );
            }

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void SampleVehicleAdvanced::update()
    {
        if( Thread::getCurrentTask() == TaskId::Physics )
        {
            if( !m_physicsConfigured &&
                core::IApplicationManager::instance()->getTimer()->getTimeSinceSceneLoad() > 3 )
            {
                m_raceScene->configurePhysics();
                m_physicsConfigured = true;
            }
            if( m_resetRequested.exchange( false ) )
            {
                performReset();
            }

            updateControls();

        }

        // The application camera is a render object, so only touch its scene node from the render
        // task. VehicleCameraController advances the smooth rig on the render task.
        if( Thread::getCurrentTask() == TaskId::Render )
        {
            // Physics listeners publish the root pose between scene updates. Resolve its dirty
            // descendants before drawing, preserving their local offsets and wheel rotations.
            if( m_vehicleActor )
                m_vehicleActor->updateTransform();
        }

        Application::update();
        if(m_collisionSmokeTest && m_assets.cityTerrainRoads && !m_cityRetentionSmokePrepared &&
           Thread::getCurrentTask()==TaskId::Application && m_physicsConfigured &&
           core::IApplicationManager::instance()->getTimer()->getTimeSinceSceneLoad()>3)
            prepareCityRetentionSmoke();
        if ( m_audioSmokeTest && m_audioEnabled && Thread::getCurrentTask() == TaskId::Physics )
            updateVehicleAudio();
        if( m_collisionSmokeTest && Thread::getCurrentTask() == TaskId::Physics )
            updateCollisionSmokeTest();
        if( m_smokeTest && Thread::getCurrentTask() == TaskId::Physics )
        {
            updateSmokeTest();
        }

        if( Thread::getCurrentTask() == TaskId::Render )
        {
            updateRenderCamera();
            if ( m_effectsSmokeTest && m_effectsEnabled ) updateVehicleEffects();
            if( m_smokeTest && m_smokePhase > 0 )
            {
                auto car = m_vehicleActor->getComponent<scene::CarController>();
                auto vehicle = car->getVehicleController();
                // Check the sustained turn and return to centre after the application
                // task has published the pose; input changes can precede it by one frame.
                for( u32 i = 0; m_smokePhase >= 2 && m_smokeTime > .5 && i < 2; ++i )
                {
                    const auto angle = vehicle->getWheelController( i )->getSteeringAngle();
                    // The assisted controller can be travelling toward the input
                    // or applying its speed limit. Check the authored lock rather
                    // than requiring an instantaneous, unfiltered input angle.
                    if( !std::isfinite( float( angle ) ) ||
                        std::abs( float( angle ) ) >
                            float( m_assets.vehicle.physics.wheels[i].maxSteerRad * 180.0 /
                                   3.14159265358979323846 ) +
                                .01f )
                    {
                        WP_LOG_ERROR( "Vehicle smoke: steering input lost its configured angle scale." );
                        m_smokeTestPassed = false;
                        core::IApplicationManager::instance()->setQuit( true );
                    }
                    const auto expectedAxle =
                        QuaternionF::eulerDegrees( 0, -float( angle ), 0 ) * Vector3F::unitX();
                    const auto renderedAxle =
                        m_wheelActors[i]->getLocalOrientation() * Vector3F::unitX();
                    // Spin about the axle cannot affect this comparison: it verifies
                    // that the visible steering frame matches the tyre contact frame.
                    // Physics can advance after the render wheel pose was
                    // published. Allow one tick of the configured steering slew.
                    const auto steeringTolerance =
                        .002f +
                        float(
                            90.0 *
                            std::clamp( core::IApplicationManager::instance()->getTimer()->getDeltaTime(
                                            TaskId::Physics ),
                                        0.0, 1.0 / 30.0 ) *
                            3.14159265358979323846 / 180.0 );
                    if( ( expectedAxle - renderedAxle ).length() > steeringTolerance )
                    {
                        WP_LOG_ERROR( "Vehicle smoke: rendered steering disagrees with wheel physics." );
                        m_smokeTestPassed = false;
                        core::IApplicationManager::instance()->setQuit( true );
                    }
                }
                auto app = core::IApplicationManager::instance();
                auto timer = app->getTimer();
                auto transform = m_vehicleActor->getTransform();
                auto expectedPose = m_vehicleActor->getWorldTransform();
                app->getGameManager()->getTransformState(
                    m_vehicleActor->getHandle()->getInstanceId(),
                    timer->getTime( TaskId::Render ) - scene::IGameManager::smoothMotionDelay,
                    timer->getDeltaTime( TaskId::Render ), expectedPose, transform->getTask() );
                if( !checkMeshTransforms( m_vehicleActor, expectedPose ) )
                {
                    WP_LOG_ERROR( "Vehicle smoke: rendered vehicle hierarchy does not match its pose." );
                    m_smokeTestPassed = false;
                    core::IApplicationManager::instance()->setQuit( true );
                }
            }

            updateDebugText();
            auto app = core::IApplicationManager::instance();
            // Loading/procedural generation can take longer than the entire
            // review interval. Warm up actual rendered frames, then measure.
            const auto sceneTime = app->getTimer()->getTimeSinceSceneLoad();
            if( m_reviewStart < 0 ) m_reviewStart = sceneTime;
            const auto profileNow = std::max( 0.0, sceneTime - m_reviewStart );
            ++m_reviewFrames;
            bool profileStartedThisFrame = false;
            if( profileNow > 5 && m_reviewFrames >= 30 && !m_benchmarkStarted )
            {
                m_benchmarkStarted = true;
                m_profileStart = profileNow;
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
                if( auto renderer = dynamic_cast<render::ClawRendererDX11 *>(
                        app->getGraphicsSystem()->getRendererPtr() ) )
                    wp_renderer_dx11_reset_statistics(
                        wp_renderer_get_dx11( renderer->getNativeRenderer() ) );
#endif
                if( m_sceneProfile )
                {
#if WP_GRAPHICS_SYSTEM_CLAW
                    if( auto graphics = dynamic_pointer_cast<render::ClawHammerSystem>( app->getGraphicsSystem() ) )
                        graphics->resetSceneProfile();
#endif
                    m_sceneProfile->beginMeasurement();
                    profileStartedThisFrame = true;
                }
            }
            if( m_benchmarkStarted )
            {
                ++m_profileFrames;
            }
            if( m_sceneProfile && m_benchmarkStarted && !profileStartedThisFrame && !m_captureAttempted )
            {
#if WP_GRAPHICS_SYSTEM_CLAW
                if( auto graphics = dynamic_pointer_cast<render::ClawHammerSystem>( app->getGraphicsSystem() ) )
                {
                    render::ClawHammerSystem::SceneProfileSnapshot snapshot;
                    if( graphics->readSceneProfile( snapshot ) ) m_sceneProfile->observeTelemetry( snapshot );
                }
#endif
                m_sceneProfile->frame();
            }
            if( ( !m_capturePath.empty() || m_benchmarkSeconds > 0 ) && !m_captureAttempted &&
                m_benchmarkStarted && m_profileFrames >= ( m_sceneProfile ? 31 : 30 ) &&
                profileNow - m_profileStart > ( m_benchmarkSeconds > 0 ? m_benchmarkSeconds : 3 ) )
            {
                // Stop ROI before logging, owner inventories, GPU readbacks or
                // formatting validation evidence can perturb the measurement.
                if( m_sceneProfile ) m_sceneProfile->endMeasurement();
                bool linearSceneValid = true;
                size_t treeMeshes = 0, treeImposters = 0;
                bool treeLODValid = m_assets.openCity || m_assets.useFoliageWorld || !m_assets.treeLODs.empty();
                for( const auto &group : m_assets.treeLODs )
                {
                    auto levels = group->getLevels();
                    treeLODValid &=
                        levels[0].renderers[0]->isLODVisible() != levels[1].renderers[0]->isLODVisible();
                    treeMeshes += levels[0].renderers[0]->isLODVisible() ? 1 : 0;
                    treeImposters += levels[1].renderers[0]->isLODVisible() ? 1 : 0;
                }
                WP_LOG( String( "Tree LOD patches: meshes=" ) + StringUtil::toString( treeMeshes ) +
                        " imposters=" + StringUtil::toString( treeImposters ) );
                bool vehicleLODValid = m_assets.vehicleLOD && m_assets.vehicleLOD->validate();
                if( vehicleLODValid )
                {
                    size_t visibleLevels = 0;
                    auto levels = m_assets.vehicleLOD->getLevels();
                    for( size_t i = 0; i < levels.size(); ++i )
                    {
                        const bool visible = levels[i].renderers.front()->isLODVisible();
                        for( const auto &renderer : levels[i].renderers )
                            vehicleLODValid &= renderer->isLODVisible() == visible;
                        if( visible )
                        {
                            ++visibleLevels;
                            WP_LOG( String( "Vehicle LOD level=" ) + StringUtil::toString( i ) );
                        }
                    }
                    vehicleLODValid &= visibleLevels == 1;
                }
                WP_LOG( String( "VehicleAdvanced mean frame milliseconds=" ) +
                        StringUtil::toString( m_profileFrames > 1
                                                  ? 1000 * ( profileNow - m_profileStart ) /
                                                        ( m_profileFrames - 1 )
                                                  : 0 ) +
                        " scene meshes=" + StringUtil::toString( m_assets.meshes.size() ) +
                        " textures=" + StringUtil::toString( m_assets.textures.size() ) );
                m_captureAttempted = true;
                bool nativeFrameValid = true;
                std::ostringstream evidence;
                evidence.imbue( std::locale::classic() );
                evidence << std::setprecision( 17 )
                         << "{\n  \"schema\": \"workphone.vehicle-review.v1\",\n"
                         << "  \"seed\": " << m_seed << ",\n  \"quality\": " << int( m_quality )
                         << ",\n  \"camera\": " << advanced::jsonString( m_captureView )
                         << ",\n  \"capture\": " << advanced::jsonString( m_capturePath )
                         << ",\n  \"replay\": " << advanced::jsonString( m_replayPath )
                         << ",\n  \"materialSourceExport\": " << advanced::jsonString( m_materialExportDirectory )
                         << ",\n  \"replayHz\": 120,\n  \"replayTicksConsumed\": " << m_replayTick
                         << ",\n  \"replayTicksAvailable\": " << m_replayControls.size()
                         << ",\n  \"requestedWidth\": " << m_reviewWidth
                         << ",\n  \"requestedHeight\": " << m_reviewHeight
                         << ",\n  \"forcedTreeLOD\": " << m_forcedLOD
                         << ",\n  \"forcedVehicleLOD\": " << m_forcedVehicleLOD
                         << ",\n  \"orbit\": " << ( m_orbitCamera ? "true" : "false" )
                         << ",\n  \"hud\": " << ( m_hudEnabled ? "true" : "false" )
                         << ",\n  \"audioEnabled\": " << ( m_audioEnabled ? "true" : "false" )
                         << ",\n  \"effectsEnabled\": " << ( m_effectsEnabled ? "true" : "false" )
                         << ",\n  \"measurementSeconds\": " << profileNow - m_profileStart
                         << ",\n  \"performanceAcceptance\": \"unverified\""
                         << ",\n  \"imagePipelineSettings\": null"
                         << ",\n  \"renderUpdatesMeasured\": " << m_profileFrames
                         << ",\n  \"sceneMeshes\": " << m_assets.meshes.size()
                         << ",\n  \"sceneTextures\": " << m_assets.textures.size();
                evidence << ",\n  \"sceneMode\": " << advanced::jsonString( m_assets.cityTerrainRoads?"terrain-city":m_assets.cityRoadShowcase?"road-authoring-showcase":m_assets.openCity ? "city" : "circuit" );
                bool cityValid = m_assets.openCity == m_openCity && m_assets.cityRoadShowcase==m_roadAuthoringShowcase && m_assets.cityTerrainRoads==m_cityTerrainRoads;
                if(m_assets.cityTerrainRoads)
                {
                    const auto check=checkTerrainCity(m_assets,app->getPhysicsManager()->getPhysicsScene());cityValid=cityValid && check.valid;
                    auto authoring=m_assets.roadAuthoring;const auto snapshot=m_assets.roadTerrain?m_assets.roadTerrain->getTerrainSnapshot():nullptr;
                    const auto &layout=m_assets.cityLayout;size_t parks=0;for(const auto &lot:layout.lots)parks+=lot.park?1:0;
                    evidence<<",\n  \"terrainCity\": {\"sourceId\":"<<advanced::jsonString(authoring?String(authoring->getSource().id.data(),authoring->getSource().id.size()):String())
                        <<",\"terrainBindingUUID\":"<<advanced::jsonString(authoring?authoring->getTerrainBindingUUID():String())
                        <<",\"bindingValid\":"<<(check.binding?"true":"false")<<",\"sourceReused\":"<<(m_assets.citySourceReused?"true":"false")
                        <<",\"sourceRevision\":"<<(authoring?authoring->getSourceRevision():0)<<",\"appliedSourceRevision\":"<<m_assets.cityAppliedSourceRevision
                        <<",\"activeRevision\":"<<(authoring?authoring->getActiveRevision():0)<<",\"terrainRevision\":"<<(snapshot?snapshot->revision:0)
                        <<",\"sourceBytes\":"<<(authoring?authoring->getSourceJSON().size():0)<<",\"roads\":"<<check.roads<<",\"roundabouts\":"<<check.junctions
                        <<",\"runtimeTriangles\":"<<check.triangles<<",\"terrainRendererReady\":"<<(check.renderer?"true":"false")
                        <<",\"routeSamples\":"<<check.routePoints<<",\"routeDriveable\":"<<(check.route?"true":"false")<<",\"routeMaximumHeightError\":"<<check.routeHeightError
                        <<",\"raceRouteAvailable\":"<<(m_assets.cityRaceRouteAvailable?"true":"false")<<",\"raceRouteError\":"<<advanced::jsonString(m_assets.cityRaceRouteError)
                        <<",\"islandCentresChecked\":"<<check.islandsChecked<<",\"islandsExcluded\":"<<(check.islands?"true":"false")
                        <<",\"nativeRayProbesPassed\":"<<check.rayProbes<<",\"flatFallbackAbsent\":"<<(check.flatFallbackAbsent?"true":"false")
                        <<",\"lots\":"<<layout.lots.size()<<",\"buildingLots\":"<<layout.lots.size()-parks<<",\"parkLots\":"<<parks
                        <<",\"buildingCollisionActors\":"<<check.buildingBodies<<",\"expectedBuildingCollisionActors\":"<<check.expectedBuildingBodies
                        <<",\"buildingSupportChecks\":"<<m_assets.cityBuildingSupportChecks<<",\"culledParcels\":"<<m_assets.cityCulledParcels
                        <<",\"buildingSupportValid\":"<<(check.support?"true":"false")<<",\"structuralValidation\":"<<(cityValid?"true":"false")<<"}";
                    if(!cityValid)WP_LOG_ERROR("Terrain city: source, fitted route, native collision or building support validation failed.");
                }
                else if(m_assets.cityRoadShowcase)
                {
                    auto authoring=m_assets.roadAuthoring;auto terrain=m_assets.roadTerrain;
                    const auto snapshot=terrain?terrain->getTerrainSnapshot():nullptr;
                    const auto activeTerrain=terrain?terrain->getActiveTerrainSnapshot():nullptr;
                    const bool terrainRenderReady=terrain && terrain->getTerrain() && activeTerrain && snapshot && activeTerrain->revision==snapshot->revision;
                    const auto physicsManager=app->getPhysicsManager();auto physics=physicsManager?physicsManager->getPhysicsScene():nullptr;
                    const size_t triangles=authoring?authoring->getRuntimeTriangleCount():0;
                    bool probesValid=bool(authoring && snapshot && physics);size_t probeCount=0;
                    bool islandDriveable=true;f32 islandHeight=0;bool islandValid=authoring && authoring->getRuntimeSurface(0,0,islandHeight,islandDriveable) && !islandDriveable;
                    const auto probe=[&](Vector3F p,bool expectedDriveable,bool terrainOnly)
                    {
                        f32 height=0;bool driveable=false;
                        bool valid=terrainOnly?render::sampleTerrainHeight(*snapshot,Transform3<real_Num>(),p,height):authoring->getRuntimeSurface(p.x,p.z,height,driveable);
                        if(!terrainOnly)valid=valid && driveable==expectedDriveable;
                        Vector3F hit,normal;SmartPtr<ISharedObject> object;
                        valid=valid && physics->intersects({p.x,height+5,p.z},{p.x,height-5,p.z},hit,normal,object) && object && std::isfinite(hit.y) && std::abs(hit.y-height)<.035f && normal.y>.5f;
                        if(valid)++probeCount;probesValid=probesValid && valid;
                    };
                    if(probesValid)
                    {
                        probe({11,0,0},true,false);probe({0,0,0},false,false);probe({-70,0,-70},false,true);
                        const auto &bake=authoring->getBake();
                        if(!bake.roads.empty() && !bake.roads.front().points.empty())probe(bake.roads.front().points[bake.roads.front().points.size()/2].position,true,false);
                        else probesValid=false;
                    }
                    bool flatFallbackAbsent=m_assets.ground && terrain && terrain->getActor()==m_assets.ground;
                    for(const auto &actor:m_assets.actors)if(actor && actor->getName()=="Circuit collision plane")flatFallbackAbsent=false;
                    const bool revisions=authoring && snapshot && authoring->getSourceRevision()==authoring->getActiveRevision() && authoring->getSource().terrainRevision==snapshot->revision;
                    bool circulationMatchesSource=authoring && authoring->getSource().roundabouts.size()==1 && m_assets.circuit.samples.size()>=3;
                    bool routeSurfaceHeightsMatch=circulationMatchesSource;
                    if(circulationMatchesSource)
                    {
                        const auto &junction=authoring->getSource().roundabouts.front();
                        for(size_t i=0;i<m_assets.circuit.samples.size();++i)
                        {
                            const auto p=m_assets.circuit.samples[i].position,q=m_assets.circuit.samples[(i+1)%m_assets.circuit.samples.size()].position;
                            const float crossY=(p-junction.center).crossProduct(q-junction.center).y;
                            circulationMatchesSource=circulationMatchesSource && std::isfinite(crossY) && (junction.clockwise?crossY<-.0001f:crossY>.0001f);
                            f32 height=0;bool driveable=false;
                            routeSurfaceHeightsMatch=routeSurfaceHeightsMatch && authoring->getRuntimeSurface(p.x,p.z,height,driveable) && driveable && std::isfinite(p.y) && std::abs(p.y-height)<=.001f;
                        }
                    }
                    cityValid=cityValid && revisions && terrainRenderReady && triangles>0 && authoring->getSource().roads.size()==4 && authoring->getSource().roundabouts.size()==1 &&
                        probesValid && probeCount==4 && islandValid && flatFallbackAbsent && circulationMatchesSource && routeSurfaceHeightsMatch;
                    evidence<<",\n  \"roadAuthoring\": {\"sourceRevision\":"<<(authoring?authoring->getSourceRevision():0)
                        <<",\"activeRevision\":"<<(authoring?authoring->getActiveRevision():0)<<",\"terrainRevision\":"<<(snapshot?snapshot->revision:0)
                        <<",\"roads\":"<<(authoring?authoring->getSource().roads.size():0)<<",\"roundabouts\":"<<(authoring?authoring->getSource().roundabouts.size():0)
                        <<",\"runtimeTriangles\":"<<triangles<<",\"terrainSamples\":"<<(snapshot?snapshot->heights.size():0)
                        <<",\"terrainActiveRevision\":"<<(activeTerrain?activeTerrain->revision:0)<<",\"terrainRendererReady\":"<<(terrainRenderReady?"true":"false")
                        <<",\"nativeRayProbesPassed\":"<<probeCount<<",\"nativeRayProbesValid\":"<<(probesValid?"true":"false")
                        <<",\"islandDriveable\":"<<(islandDriveable?"true":"false")<<",\"islandHeight\":"<<islandHeight
                        <<",\"flatFallbackAbsent\":"<<(flatFallbackAbsent?"true":"false")<<",\"foliageAvailability\":\"unavailable-in-fixture\""
                        <<",\"circulationMatchesSource\":"<<(circulationMatchesSource?"true":"false")<<",\"routeSurfaceHeightsMatch\":"<<(routeSurfaceHeightsMatch?"true":"false")
                        <<",\"structuralValidation\":"<<(cityValid?"true":"false")<<"}";
                    if(!cityValid)WP_LOG_ERROR("Road authoring showcase: runtime, revision, terrain or native collision validation failed.");
                }
                else if( m_assets.openCity )
                {
                    const auto city = procedural::OpenCityLayout::generate( m_seed, m_assets.cityBlocks, m_assets.cityRoute,
                                                                            m_assets.cityNaturalRoads );
                    size_t parkLots = 0, buildingLots = 0, buildingCollisions = 0, expectedBuildingCollisions = 0;
                    size_t cityLODGroups = 0, cityLODNear = 0, cityLODFar = 0;
                    bool footprintValid = true, routeValid = m_assets.circuit.samples.size() >= 3;
                    bool cityLODValid = true;
                    for( const auto &lot : city.lots )
                    {
                        if( lot.park ) ++parkLots; else ++buildingLots;
                        expectedBuildingCollisions += procedural::OpenCityLayout::buildingVolumes( lot ).size();
                        const float clearance = city.footprintRoadClearance( lot.x, lot.z, lot.width, lot.depth );
                        footprintValid = footprintValid && std::isfinite( clearance ) &&
                            clearance >= procedural::OpenCityLayout::roadSetback - .001f;
                    }
                    for( size_t i = 0; i < m_assets.circuit.samples.size(); ++i )
                    {
                        const auto p = m_assets.circuit.samples[i].position;
                        const auto q = m_assets.circuit.samples[( i + 1 ) % m_assets.circuit.samples.size()].position;
                        const float step = ( q - p ).length();
                        routeValid = routeValid && std::isfinite( p.x ) && std::isfinite( p.z ) &&
                            city.roadDistance( p.x, p.z ) < .01f && step > 0 && step <= 2.05f;
                    }
                    for( const auto &actor : m_assets.actors )
                    {
                        if( !actor ) continue;
                        const auto name = actor->getName();
                        if( name.find( "Trackside collision: City building mass" ) == 0 ) ++buildingCollisions;
                        if( name.find( "City LOD patch" ) != 0 ) continue;
                        ++cityLODGroups;
                        const auto group = actor->getComponent<scene::LODGroup>();
                        if( !group || !group->validate() ) { cityLODValid = false; continue; }
                        const auto levels = group->getLevels();
                        size_t visibleLevels = 0;
                        for( size_t level = 0; level < levels.size(); ++level )
                        {
                            if( levels[level].renderers.empty() ) { cityLODValid = false; continue; }
                            const bool visible = levels[level].renderers.front()->isLODVisible();
                            for( const auto &renderer : levels[level].renderers )
                                cityLODValid = cityLODValid && renderer && renderer->isLODVisible() == visible;
                            if( visible )
                            {
                                ++visibleLevels;
                                if( level == 0 && levels.size() > 1 ) ++cityLODNear; else ++cityLODFar;
                            }
                        }
                        cityLODValid = cityLODValid && visibleLevels == 1;
                    }
                    cityValid = cityValid && m_assets.cityBlocks == m_cityBlocks && m_assets.cityRoute == m_cityRoute &&
                        m_assets.cityNaturalRoads == m_cityNaturalRoads &&
                        routeValid && footprintValid && buildingLots > 0 && buildingCollisions == expectedBuildingCollisions &&
                        cityLODGroups > 0 && cityLODValid;
                    evidence << ",\n  \"city\": {\"blocks\":" << city.blocks << ",\"route\":" << m_assets.cityRoute
                             << ",\"layout\":" << advanced::jsonString( m_assets.cityNaturalRoads ? "lsystem" : "grid" )
                             << ",\"generatorRevision\":" << procedural::OpenCityLayout::generatorRevision
                             << ",\"roadNetworkNodes\":" << city.roads.nodes.size()
                             << ",\"roadNetworkEdges\":" << city.roads.edges.size()
                             << ",\"spacing\":" << city.spacing << ",\"extent\":" << city.extent
                             << ",\"lots\":" << city.lots.size() << ",\"buildingLots\":" << buildingLots
                             << ",\"parkLots\":" << parkLots << ",\"buildingCollisionActors\":" << buildingCollisions
                             << ",\"expectedBuildingCollisionActors\":" << expectedBuildingCollisions
                             << ",\"routeSamples\":" << m_assets.circuit.samples.size()
                             << ",\"closedRouteOnRoads\":" << ( routeValid ? "true" : "false" )
                             << ",\"footprintsClearRoads\":" << ( footprintValid ? "true" : "false" )
                             << ",\"lodGroups\":" << cityLODGroups << ",\"nearLodGroups\":" << cityLODNear
                             << ",\"farLodGroups\":" << cityLODFar << ",\"lodValid\":" << ( cityLODValid ? "true" : "false" )
                             << ",\"structuralValidation\":" << ( cityValid ? "true" : "false" ) << "}";
                    if( !cityValid ) WP_LOG_ERROR( "VehicleAdvanced: city route, clearance, collision or LOD validation failed." );
                }
                evidence << ",\n  \"device\": " << advanced::captureDeviceEvidence();
                void *reflectionView = nullptr;
                if( m_assets.reflectionTexture ) m_assets.reflectionTexture->getTextureFinal( &reflectionView );
                bool radianceValid = false;
                evidence << ",\n  \"reflectionRadiance\": " << advanced::captureRadianceEvidence( reflectionView, radianceValid );
                if( !radianceValid ) WP_LOG_ERROR( "VehicleAdvanced: HDR reflection readback did not confirm finite radiance above one." );
                evidence << ",\n  \"foliageWorldEnabled\": " << ( m_foliageWorldEnabled ? "true" : "false" );
                bool foliageValid = !m_foliageWorldEnabled;
                if( m_assets.foliageTerrain )
                {
                    const auto foliage = m_assets.foliageTerrain->getFoliageDiagnostics();
                    foliageValid = foliage.currentGenerationReady && foliage.failedPages == 0 && foliage.error.empty();
                    evidence << ",\n  \"foliage\": {\"ready\": " << ( foliage.currentGenerationReady ? "true" : "false" )
                             << ", \"residentPages\": " << foliage.residentPages
                             << ", \"pendingPages\": " << foliage.pendingPages
                             << ", \"failedPages\": " << foliage.failedPages
                             << ", \"residentInstances\": " << foliage.residentInstances
                             << ", \"testedInstances\": " << foliage.testedInstances
                             << ", \"culledInstances\": " << foliage.culledInstances
                             << ", \"lodInstances\": [" << foliage.lodInstances[0] << ',' << foliage.lodInstances[1] << ',' << foliage.lodInstances[2] << ']'
                             << ", \"actualDraws\": " << foliage.actualDraws
                             << ", \"geometryBytes\": " << foliage.geometryBytes
                             << ", \"uploadBytes\": " << foliage.uploadBytes
                             << ", \"error\": " << advanced::jsonString( foliage.error ) << "}";
                }
#if WP_GRAPHICS_SYSTEM_CLAW
                // Consume the completed render-task snapshot: querying the C system
                // directly from the application task would race frame publication.
                const auto graphicsProperties = app->getGraphicsSystem()->getProperties();
                u32 nativeUpdates = 0, nativeRenders = 0;
                s32 completedPasses = 0, skippedPasses = 0;
                nativeFrameValid = graphicsProperties &&
                    graphicsProperties->getPropertyValue( "native_update_serial", nativeUpdates ) &&
                    graphicsProperties->getPropertyValue( "native_render_serial", nativeRenders ) &&
                    graphicsProperties->getPropertyValue( "native_completed_passes", completedPasses ) &&
                    graphicsProperties->getPropertyValue( "native_skipped_passes", skippedPasses ) &&
                    nativeUpdates > 0 && nativeUpdates == nativeRenders && completedPasses > 0;
                std::ostringstream nativeReport;
                nativeReport << "VehicleAdvanced native frame: updates=" << nativeUpdates
                             << " renders=" << nativeRenders << " completedPasses=" << completedPasses
                             << " skippedPasses=" << skippedPasses
                             << " valid=" << ( nativeFrameValid ? "true" : "false" );
                WP_LOG( nativeReport.str() );
                evidence << ",\n  \"nativeUpdates\": " << nativeUpdates
                         << ",\n  \"nativeRenders\": " << nativeRenders
                         << ",\n  \"nativeCompletedPasses\": " << completedPasses
                         << ",\n  \"nativeSkippedPasses\": " << skippedPasses;
                if( !nativeFrameValid )
                    WP_LOG_ERROR( "VehicleAdvanced: expected one native update per rendered frame and an active target." );
#endif
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
                if( auto renderer = dynamic_cast<render::ClawRendererDX11 *>(
                        app->getGraphicsSystem()->getRendererPtr() ) )
                {
                    wp_render_statistics_dx11 stats{};
                    wp_renderer_dx11_get_statistics(
                        wp_renderer_get_dx11( renderer->getNativeRenderer() ), &stats );
                    const auto frames = double( std::max<uint64_t>( stats.frames, 1 ) );
                    std::ostringstream report;
                    report << "VehicleAdvanced benchmark: frames=" << stats.frames
                           << " gpuSamples=" << stats.gpu_samples << " meanMs=" << stats.interval_ms
                           << " p95Ms=" << stats.interval_p95_ms
                           << " cpuIncludingPresentMs=" << stats.cpu_frame_ms
                           << " presentMs=" << stats.present_ms << " gpuMs=" << stats.gpu_frame_ms
                           << " drawsPerFrame=" << stats.draws / frames
                           << " trianglesPerFrame=" << stats.triangles / frames
                           << " materialUploadsPerFrame=" << stats.material_uploads / frames
                           << " transformUploadsPerFrame=" << stats.transform_uploads / frames
                           << " stateBindingsPerFrame=" << stats.state_bindings / frames;
                    WP_LOG( report.str() );
                    const auto native = wp_renderer_get_dx11( renderer->getNativeRenderer() );
                    const auto hdrPasses = wp_renderer_dx11_get_hdr_completed_frames( native );
                    const auto pipeline = app->getGraphicsSystem()->getGraphicsPipeline();
                    const bool hdrRequested = pipeline && pipeline->getHdrSettings().m_enableAgxToneMapping;
                    const bool hdrActive = hdrRequested && hdrPasses > 0;
                    if( hdrRequested )
                    {
                        evidence << ",\n  \"linearScene\": " << advanced::captureLinearSceneEvidence(
                            wp_renderer_dx11_get_hdr_scene_view( native ), linearSceneValid );
                        linearSceneValid = linearSceneValid && hdrActive;
                        if( !linearSceneValid ) WP_LOG_ERROR( "VehicleAdvanced: native HDR scene resolve/readback validation failed." );
                    }
                    evidence << ",\n  \"lightingOutputPath\": \""
                             << ( hdrActive ? "dx11-native-hdr-filmic" : "dx11-fixed-reinhard" ) << "\""
                             << ",\n  \"hdrPostPipelineActive\": " << ( hdrActive ? "true" : "false" )
                             << ",\n  \"hdrCompletedPassesLifetime\": " << hdrPasses
                             << ",\n  \"hdrExposure\": 1.0,\n  \"autoExposureActive\": false"
                             << ",\n  \"dx11\": {\"frames\": " << stats.frames
                             << ", \"intervalSamples\": " << stats.interval_samples
                             << ", \"gpuSamples\": " << stats.gpu_samples
                             << ", \"meanIntervalMs\": " << stats.interval_ms
                             << ", \"p95IntervalMs\": " << stats.interval_p95_ms
                             << ", \"cpuIncludingPresentMs\": " << stats.cpu_frame_ms
                             << ", \"presentMs\": " << stats.present_ms
                             << ", \"gpuMs\": " << stats.gpu_frame_ms
                             << ", \"draws\": " << stats.draws
                             << ", \"triangles\": " << stats.triangles
                             << ", \"instancedDraws\": " << stats.instanced_draws
                             << ", \"instances\": " << stats.instances
                             << ", \"instanceUploadBytes\": " << stats.instance_upload_bytes
                             << ", \"geometryCreations\": " << stats.geometry_creations << "}";
                }
#endif
                bool sunlightValid = true;
#if WP_GRAPHICS_SYSTEM_CLAW
                if( auto scene = dynamic_pointer_cast<render::ClawScene>(
                        app->getGraphicsSystem()->getGraphicsScene() ) )
                {
                    const auto light = scene->getPrimaryDirectionalLight();
                    sunlightValid = light && light == m_assets.sun;
                    if( light )
                    {
                        const auto direction = light->getDerivedDirection();
                        const auto power = light->getPowerScale();
                        sunlightValid = sunlightValid && std::isfinite( direction.x ) &&
                            std::isfinite( direction.y ) && std::isfinite( direction.z ) &&
                            direction.lengthSquared() > .99f && std::isfinite( power ) && power > 0;
                        evidence << ",\n  \"primarySun\": {\"matchesProceduralSky\": "
                                 << ( sunlightValid ? "true" : "false" ) << ", \"power\": " << power
                                 << ", \"direction\": [" << direction.x << ", " << direction.y
                                 << ", " << direction.z << "]}";
                    }
                    if( !sunlightValid ) WP_LOG_ERROR( "VehicleAdvanced: primary sunlight does not match the generated sky." );
                }
#endif
                const bool reflectionValid = advanced::validateReflection( m_assets );
                if( !reflectionValid )
                    WP_LOG_ERROR(
                        "VehicleAdvanced: cubemap actor, texture or vehicle material binding is "
                        "invalid." );
                const bool windingValid = checkSceneWinding( m_assets );
                m_capturePassed = ( m_capturePath.empty() || advanced::captureFrame( m_capturePath ) ) &&
                                  treeLODValid && vehicleLODValid && reflectionValid && windingValid &&
                                  nativeFrameValid && foliageValid && radianceValid && sunlightValid && linearSceneValid && cityValid;
                evidence << ",\n  \"validationPassed\": " << ( m_capturePassed ? "true" : "false" )
                         << ",\n  \"visualAcceptance\": \"unreviewed\",\n"
                         << "  \"limits\": \"Control replay is physics-tick based; settings and source hashes are recorded by the capture runner. Timings do not certify image quality.\"\n}\n";
                if( m_sceneProfile )
                    m_sceneProfile->setValidationEvidence( evidence.str(), m_evidencePath, m_capturePassed, nativeFrameValid );
                else if( !m_evidencePath.empty() && !advanced::writeReviewEvidence( m_evidencePath, evidence.str() ) )
                {
                    WP_LOG_ERROR( "VehicleAdvanced: could not write evidence manifest." );
                    m_capturePassed = false;
                }
                if( !vehicleLODValid )
                    WP_LOG_ERROR( "Vehicle LOD visibility validation failed." );
                if( !treeLODValid )
                    WP_LOG_ERROR( "Tree LOD: expected exactly one visible level per patch." );
                WP_LOG( m_capturePassed ? "VehicleAdvanced review completed."
                                        : "VehicleAdvanced capture failed." );
                app->setQuit( true );
            }
        }
    }

    void SampleVehicleAdvanced::reset()
    {
        m_resetRequested = true;
    }

    void SampleVehicleAdvanced::setSmokeTest( bool enabled )
    {
        m_smokeTest = enabled;
    }

    bool SampleVehicleAdvanced::smokeTestPassed() const
    {
        return m_smokeTestPassed;
    }

    void SampleVehicleAdvanced::performReset()
    {
        if ( m_raceScene ) m_raceScene->performReset();
        m_lapStart = 0;
        m_nextCheckpoint = 1;
        m_lastTrackIndex = 0;
        if( m_cameraActor )
        {
            const bool terrainRoads=m_assets.roadAuthoring && m_assets.roadTerrain;
            const auto target=terrainRoads?m_vehicleActor->getPosition():getVehicleSpawnPosition();
            if(terrainRoads)m_generatedSpawnPosition=target;
            m_cameraActor->setPosition( terrainRoads?target+Vector3F(0,cameraHeight,cameraDistance):getInitialCameraPosition() );
            m_cameraActor->lookAt( target, Vector3<real_Num>::unitY() );
            m_cameraActor->updateTransform();
        }
    }

    void SampleVehicleAdvanced::updateControls()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto input = applicationManager ? applicationManager->getInputDeviceManager() : nullptr;
        auto car = m_vehicleActor ? m_vehicleActor->getComponent<scene::CarController>() : nullptr;
        auto vehicle = car ? car->getVehicleController() : nullptr;
        if( !input || !vehicle )
        {
            return;
        }

        float throttle = 0, brake = 0, steering = 0;

        if( m_smokeTest )
        {
            throttle = m_smokePhase == 1 || m_smokePhase == 2 ? 1.0f : 0.0f;
            brake = m_smokePhase == 3 ? 1.0f : 0.0f;
            steering = m_smokePhase == 2 ? .35f : 0.0f;
        }

        if( !m_capturePath.empty() || m_benchmarkSeconds > 0 )
        {
            throttle = 0;
            brake = 1;
            steering = 0;
        }

        if( !m_replayControls.empty() && m_physicsConfigured )
        {
            if( m_replayTick < m_replayControls.size() )
            {
                const auto &control = m_replayControls[m_replayTick++];
                throttle = control.throttle;
                brake = control.brake;
                steering = control.steering;
            }
            else
            {
                throttle = 0;
                brake = 1;
                steering = 0;
            }
        }

        // At speed, limit requested lateral acceleration rather than allowing a full
        // parking-speed steering angle to overturn the car. Keep full lock for hairpins.
        const auto speed =
            m_vehicleActor->getComponent<scene::Rigidbody>()->getLinearVelocity().length();
        const auto maxSteer = m_assets.vehicle.physics.wheels[0].maxSteerRad;
        const auto steerScale =
            std::min( 1.0, m_assets.vehicle.physics.wheelbaseM * 8.0 /
                               ( std::max( double( speed * speed ), 1.0 ) * maxSteer ) );
        // Keep the handling fixture on the track, clear of the physical barriers.
        steering *= float( steerScale );
        if( m_trackSmokeTest && m_physicsConfigured )
        {
            if(m_assets.circuit.samples.empty() || m_assets.circuit.length<=0 || (m_assets.cityTerrainRoads && !m_assets.cityRaceRouteAvailable))
            {WP_LOG_ERROR("VehicleAdvanced full circuit unavailable: "+m_assets.cityRaceRouteError);m_raceScene->setControls(0,1,0);applicationManager->setQuit(true);return;}
            const auto position = m_vehicleActor->getPosition();
            const auto &circuit = m_assets.circuit;
            const auto nearest = circuit.nearest( position );
            const auto lookahead = std::max( 8.f, speed * .75f );
            const auto target = circuit
                                    .samples[( nearest + size_t( lookahead * circuit.samples.size() /
                                                                 circuit.length ) ) %
                                             circuit.samples.size()]
                                    .position;
            const auto local = m_vehicleActor->getOrientation().inverse() * ( target - position );
            const auto angle = std::atan2( 2.f * float( m_assets.vehicle.physics.wheelbaseM ) * local.x,
                                           std::max( local.x * local.x + local.z * local.z, 1.f ) );
            steering = float( std::clamp( double( angle ) / maxSteer, -1.0, 1.0 ) );
            const auto wanted = std::clamp( 7.f / ( 1.f + std::abs( angle ) * 2.f ), 3.5f, 7.f );
            throttle = std::clamp( ( wanted - speed ) * .25f, 0.f, .4f );
            brake = std::clamp( ( speed - wanted ) * .12f, 0.f, .4f );
            auto flat = position - circuit.samples[nearest].position;
            flat.y = 0;
            const auto now = applicationManager->getTimer()->getTimeSinceSceneLoad();
            f32 groundY=0;bool driveable=false;if(m_assets.roadAuthoring)m_assets.roadAuthoring->getRuntimeSurface(position.x,position.z,groundY,driveable);
            if( position.y-groundY < .1f || position.y-groundY > 1.f || flat.length() > 12.f || now > 300 )
            {
                WP_LOG_ERROR( String( "VehicleAdvanced full circuit: FAIL position=" ) +
                              StringUtil::toString( position ) +
                              " offset=" + StringUtil::toString( flat.length() ) +
                              " index=" + StringUtil::toString( nearest ) +
                              " steer=" + StringUtil::toString( steering ) );
                applicationManager->setQuit( true );
            }
            else if( m_lap > 1 )
            {
                m_smokeTestPassed = true;
                WP_LOG( "VehicleAdvanced full circuit: PASS." );
                applicationManager->setQuit( true );
            }
        }
        if( m_collisionSmokeTest )
            m_raceScene->setControls( 0, 0, 0 );
        else if( m_smokeTest || m_trackSmokeTest || !m_capturePath.empty() || m_benchmarkSeconds > 0 )
            m_raceScene->setControls( throttle, brake, steering );
        else
            m_raceScene->usePlayerControls();
    }

    void SampleVehicleAdvanced::updateVehicleEffects()
    {
        if( m_effectsSmokeTest && m_effectsTestPhase >= 5 )
            return;
        auto app = core::IApplicationManager::instance();
        const auto dt = float( app->getTimer()->getDeltaTime() );
        if( !std::isfinite( dt ) || dt <= 0 )
            return;
        auto frame = m_raceScene->sampleVehicleEffects();
        if( m_effectsSmokeTest )
        {
            m_effectsTestGrounded =
                m_effectsTestGrounded || std::any_of( frame.grounded.begin(), frame.grounded.end(),
                                                      []( bool contact ) { return contact; } );
            const bool road = m_effectsTestPhase == 0;
            const bool dust = m_effectsTestPhase == 2 || m_effectsTestPhase == 3;
            frame.position = { 0, .45f, float( 4 - m_effectsTestTime * 4 ) };
            frame.velocity = road && m_effectsTestTime < 2.92 ? Vector3F( 0, 0, -10 )
                             : dust                           ? Vector3F( 0, 0, -20 )
                                                              : Vector3F::zero();
            frame.playing = m_effectsTestPhase != 3;
            for( size_t i = 0; i < 4; ++i )
            {
                frame.contact[i] = { i % 2 ? .95f : -.95f, .026f,
                                     float( 4 - m_effectsTestTime * 4 ) + ( i < 2 ? -1.5f : 1.5f ) };
                frame.normal[i] = Vector3F::unitY();
                frame.slip[i] = 8;
                frame.width[i] = .28f;
                frame.grounded[i] = road || dust;
                frame.onRoad[i] = !dust;
            }
        }
        m_raceScene->getVehicleVisualEffects().update( frame, dt );
        if( !m_effectsSmokeTest )
            return;
        m_effectsTestTime += std::min( dt, .05f );
        const float durations[] = { 3.f, 2.f, 1.f, 1.f, .5f };
        if( m_effectsTestTime < durations[m_effectsTestPhase] )
            return;
        const auto particleCount = m_raceScene->getVehicleVisualEffects().particles(),
                   decalCount = m_raceScene->getVehicleVisualEffects().decals();
        bool passed = m_raceScene->getVehicleVisualEffects().uploaded();
        if( m_effectsTestPhase == 0 )
        {
            passed = passed && particleCount > 0 && decalCount > 0 && m_effectsTestGrounded;
            m_effectsTestEmitted = m_raceScene->getVehicleVisualEffects().emitted();
            m_effectsTestDecals = decalCount;
            if( !m_effectsCapture.empty() )
                passed = advanced::captureFrame( m_effectsCapture ) && passed;
        }
        else if( m_effectsTestPhase == 1 )
        {
            passed = passed && particleCount == 0 &&
                     m_raceScene->getVehicleVisualEffects().emitted() == m_effectsTestEmitted &&
                     decalCount > 0;
        }
        else if( m_effectsTestPhase == 2 )
        {
            passed = passed && particleCount > 0 &&
                     m_raceScene->getVehicleVisualEffects().emitted() > m_effectsTestEmitted &&
                     decalCount == m_effectsTestDecals;
            m_effectsTestEmitted = m_raceScene->getVehicleVisualEffects().emitted();
            m_effectsTestParticles = particleCount;
            if( !m_effectsCapture.empty() )
                passed = advanced::captureFrame( m_effectsCapture + ".dust.bmp" ) && passed;
            frame.playing = false;
            m_raceScene->getVehicleVisualEffects().update( frame, dt );
        }
        else if( m_effectsTestPhase == 3 )
        {
            passed = passed && particleCount == m_effectsTestParticles &&
                     m_raceScene->getVehicleVisualEffects().emitted() == m_effectsTestEmitted &&
                     decalCount == m_effectsTestDecals;
            m_raceScene->getVehicleVisualEffects().reset();
            passed = passed && m_raceScene->getVehicleVisualEffects().decals() == 0 &&
                     m_raceScene->getVehicleVisualEffects().particles() == 0;
        }
        else
        {
            passed = passed && particleCount == 0 && decalCount == 0 &&
                     m_raceScene->getVehicleVisualEffects().emitted() == m_effectsTestEmitted;
            // These are the same lifecycle controls exposed to the Lua sample.
            m_raceScene->setEffectsEnabled(false);
            m_raceScene->initializePresentation();
            passed = passed && !m_raceScene->isEffectsAvailable() &&
                     m_raceScene->getParticleCount() == 0 && m_raceScene->getSkidDecalCount() == 0;
            m_raceScene->setEffectsEnabled(true);
            m_raceScene->initializePresentation();
            passed = passed && m_raceScene->isEffectsAvailable() &&
                     m_raceScene->getParticleCount() == 0 && m_raceScene->getSkidDecalCount() == 0;
        }
        WP_LOG( "Vehicle FX smoke phase " + StringUtil::toString( m_effectsTestPhase ) +
                ( passed ? ": PASS" : ": FAIL" ) +
                " particles=" + StringUtil::toString( particleCount ) +
                " decals=" + StringUtil::toString( decalCount ) );
        if( !passed || m_effectsTestPhase == 4 )
        {
            m_smokeTestPassed = passed;
            app->setQuit( true );
        }
        ++m_effectsTestPhase;
        m_effectsTestTime = 0;
    }

    void SampleVehicleAdvanced::updateVehicleAudio()
    {
        auto app = core::IApplicationManager::instance();
        const auto dt = app->getTimer()->getDeltaTime();
        if( !std::isfinite( dt ) || dt <= 0 )
            return;
        m_audioElapsed += std::min( dt, 1.0 / 30.0 );
        if( m_audioElapsed < 1.0 / 30.0 )
            return;
        const auto audioDt = float( m_audioElapsed );
        m_audioElapsed = 0;
        auto input = m_raceScene->sampleVehicleAudio();
        if( m_audioSmokeTest )
        {
            // Exercise the real platform voices without depending on driving-test tuning.
            input.rpm = m_audioSmokePhase == 0 ? input.idleRpm : input.redlineRpm;
            input.throttle = m_audioSmokePhase == 0 ? 0.f : 1.f;
            input.speed = m_audioSmokePhase == 0 ? 0.f : 40.f;
            input.slipSpeed = m_audioSmokePhase == 0 ? 0.f : 8.f;
            input.grounded = m_audioSmokePhase < 2;
            input.playing = m_audioSmokePhase < 3;
        }
        m_raceScene->getVehicleAudio().update( input, audioDt );
        if( !m_audioSmokeTest )
            return;
        m_audioSmokeTime += audioDt;
        if( m_audioSmokeTime < 2 )
            return;
        const auto& gains = m_raceScene->getVehicleAudio().gains();
        bool passed = m_raceScene->getVehicleAudio().isPlaying();
        if( m_audioSmokePhase == 0 )
            passed = passed && gains.engine[0] > .19f && gains.squeal < .001f;
        else if( m_audioSmokePhase == 1 )
            passed = passed && gains.engine[5] > .39f && gains.squeal > .27f && gains.rolling > .05f;
        else if( m_audioSmokePhase == 2 )
            passed = passed && gains.squeal < .001f && gains.rolling < .001f;
        else
        {
            for( const auto gain : gains.engine )
                passed = passed && gain < .001f;
            passed = passed && gains.squeal < .001f && gains.rolling < .001f;
        }
        WP_LOG( "Vehicle audio smoke phase " + StringUtil::toString( m_audioSmokePhase ) +
                ( passed ? ": PASS" : ": FAIL" ) + " idle=" + StringUtil::toString( gains.engine[0] ) +
                " redline=" + StringUtil::toString( gains.engine[5] ) +
                " rolling=" + StringUtil::toString( gains.rolling ) +
                " squeal=" + StringUtil::toString( gains.squeal ) );
        if( !passed || m_audioSmokePhase == 3 )
        {
            m_smokeTestPassed = passed;
            app->setQuit( true );
        }
        ++m_audioSmokePhase;
        m_audioSmokeTime = 0;
    }

    void SampleVehicleAdvanced::prepareCityRetentionSmoke()
    {
        auto app=core::IApplicationManager::instancePtr();
        try
        {
            auto authoring=m_assets.roadAuthoring;auto terrain=m_assets.roadTerrain;
            if(!authoring || !terrain || !m_raceScene)throw std::runtime_error("Native editable city anchors are missing.");
            auto edited=authoring->getSource();const auto sourceId=edited.id;const auto binding=authoring->getTerrainBindingUUID();
            SourceBudgetSet<SourceBudgetText> referenced;
            for(const auto &junction:edited.roundabouts)for(const auto &port:junction.ports)referenced.insert(port.approachId);
            auto road=std::find_if(edited.roads.begin(),edited.roads.end(),[&](const auto &candidate){return !referenced.count(candidate.id) && candidate.width>=.75f;});
            if(road==edited.roads.end())throw std::runtime_error("No eligible non-junction road exists for the required width edit.");
            const auto roadId=road->id;SourceBudgetVector<SourceBudgetText> knotIds;for(const auto &knot:road->knots)knotIds.push_back(knot.id);
            road->width-=.25f;const float width=road->width;SourceBudgetText bytes;String error;
            if(!procedural::serializeCityRoadSplineSource(edited,bytes,error) ||
               !authoring->setSourceJSON(String(bytes.data(),bytes.size()),authoring->getSourceRevision(),error))throw std::runtime_error(error.c_str());
            m_physicsConfigured=false;
            if(!m_raceScene->regenerate())throw std::runtime_error(m_raceScene->getGenerationError().c_str());
            m_assets=m_raceScene->getAssets();m_boxGround=m_assets.ground;m_chassisMeshActor=m_assets.body;m_wheelActors=m_assets.wheels;
            m_generatedSpawnPosition=m_vehicleActor->getPosition();
#if WP_GRAPHICS_SYSTEM_CLAW
            if(auto scene=dynamic_pointer_cast<render::ClawScene>(app->getGraphicsSystem()->getGraphicsScene()))
                if(!scene->setPrimaryDirectionalLight(m_assets.sun))throw std::runtime_error("Regenerated city sunlight could not be selected.");
#endif
            if(m_assets.vehicleLOD)m_assets.vehicleLOD->setForcedLOD(m_forcedVehicleLOD);
            for(auto &group:m_assets.treeLODs)group->setForcedLOD(m_forcedLOD);
            if(m_assets.foliageTerrain && !m_assets.foliageTerrain->setFoliageForcedLod(m_forcedLOD,error))throw std::runtime_error(error.c_str());
            m_raceScene->initializePresentation();
            if(m_assets.roadAuthoring!=authoring || m_assets.roadTerrain!=terrain || !m_assets.citySourceReused ||
               authoring->getSource().id!=sourceId || authoring->getTerrainBindingUUID()!=binding ||
               m_assets.cityAppliedSourceRevision!=authoring->getSourceRevision() || authoring->getActiveRevision()!=authoring->getSourceRevision())
                throw std::runtime_error("Regeneration replaced an anchor/source identity or failed to apply the retained source revision.");
            const auto &retained=authoring->getSource();const auto match=std::find_if(retained.roads.begin(),retained.roads.end(),[&](const auto &candidate){return candidate.id==roadId;});
            if(match==retained.roads.end() || std::abs(match->width-width)>.000001f || match->knots.size()!=knotIds.size())
                throw std::runtime_error("Regeneration discarded the road width edit or control-point identities.");
            for(size_t i=0;i<knotIds.size();++i)if(match->knots[i].id!=knotIds[i])throw std::runtime_error("Regeneration changed a retained control-point GUID.");
            if(!m_assets.cityBuildingSupportValid || m_assets.cityBuildingSupportChecks<m_assets.cityLayout.lots.size())
                throw std::runtime_error("Regenerated building support checks failed.");
            m_cityRetentionSmokeRoadId=roadId;m_cityRetentionSmokeWidth=width;m_cityRetentionSmokePrepared=true;
        }
        catch(const std::exception &error)
        {WP_LOG_ERROR(String("Terrain city edit/regeneration retention smoke: FAIL ")+error.what());m_smokeTestPassed=false;if(app)app->setQuit(true);}
    }

    void SampleVehicleAdvanced::updateCollisionSmokeTest()
    {
        auto app = core::IApplicationManager::instance();
        if( !m_physicsConfigured || app->getTimer()->getTimeSinceSceneLoad() <= 3 )
            return;
        if(m_assets.cityTerrainRoads && !m_cityRetentionSmokePrepared)return;
        auto physics = app->getPhysicsManager()->getPhysicsScene();
        auto chassis = m_vehicleActor->getComponent<scene::Rigidbody>()->getRigidDynamic();
        if( m_assets.openCity && m_collisionProbeStarted ) return;
        if( !m_collisionProbeStarted )
        {
            size_t count = 0;
            size_t cityBuildingMasses = 0;
            SmartPtr<scene::IGameActor> citySidewalk;
            auto fail = [&]( const String &message ) {
                WP_LOG_ERROR( String( m_assets.openCity ? "City collision smoke: FAIL " :
                                                       "Trackside collision smoke: FAIL " ) + message );
                m_smokeTestPassed = false;
                app->setQuit( true );
            };
            for( const auto &actor : m_assets.actors )
            {
                if( actor->getName().find( "Trackside collision: " ) != 0 )
                    continue;
                ++count;
                if( actor->getName() == "Trackside collision: City building mass" ) ++cityBuildingMasses;
                if( !citySidewalk && actor->getName() == "Trackside collision: City sidewalk" ) citySidewalk = actor;
                auto box = actor->getComponent<scene::CollisionBox>();
                auto body = actor->getComponent<scene::Rigidbody>();
                if( !box || !box->getShape() || !body || !body->getRigidStatic() ||
                    body->getRigidStatic()->getScene() != physics || !box->getShape()->isAttached() )
                {
                    fail( actor->getName() + " has no static box body" );
                    return;
                }
                const auto centre = actor->getPosition();
                const auto orientation = actor->getOrientation();
                const auto size = box->getExtents();
                const auto nativePose = body->getRigidStatic()->getTransform();
                if( (nativePose.getPosition() - centre).length() > .001f ||
                    (nativePose.getOrientation() * Vector3F::unitX() - orientation * Vector3F::unitX()).length() > .001f )
                {
                    fail( actor->getName() + " native pose does not match the generated box" );
                    return;
                }
                // Short rays cannot hit the ground below. Probe the thin rotated
                // rail side too, so an incorrect yaw cannot pass the top-face check.
                for( int axis = 0; axis < (actor->getName() == "Trackside collision: Guardrail beam" ? 2 : 1); ++axis )
                {
                    const auto normal = orientation * (axis ? Vector3F::unitX() : Vector3F::unitY());
                    const auto halfSize = (axis ? size.x : size.y) * .5f;
                    const auto face = centre + normal * halfSize;
                    Vector3F hitPosition, hitNormal;
                    SmartPtr<ISharedObject> object;
                    if( !physics->intersects( face + normal * .04f, face - normal * .04f,
                                             hitPosition, hitNormal, object ) ||
                        !object ||
                        // Adjacent rails can overlap on tight inside corners.
                        // Their registration and poses were checked above; a
                        // neighboring face can be the closest result here.
                        (object == body->getRigidStatic() && (hitPosition - face).length() > .015f) )
                    {
                        fail( actor->getName() + " face does not match its physics shape axis=" +
                              StringUtil::toString(axis) + " expected=" + StringUtil::toString(face) +
                              " actual=" + StringUtil::toString(hitPosition) +
                              " body=" + StringUtil::toString(body->getRigidStatic()->getTransform().getPosition()) );
                        return;
                    }
                }
            }
            Vector3F hitPosition, hitNormal;
            SmartPtr<ISharedObject> object;
            if(m_assets.cityTerrainRoads)
            {
                const auto check=checkTerrainCity(m_assets,physics);
                if(!check.valid){fail("terrain-city source, fitted route, static building support or native surface probes failed");return;}
                WP_LOG("Terrain city edit/regeneration retention smoke: PASS road="+String(m_cityRetentionSmokeRoadId.data(),m_cityRetentionSmokeRoadId.size())+
                       " width="+StringUtil::toString(m_cityRetentionSmokeWidth)+" source reused=true revision="+StringUtil::toString(m_assets.cityAppliedSourceRevision)+
                       " native surface rays="+StringUtil::toString(u32(check.rayProbes))+" building support checks="+StringUtil::toString(m_assets.cityBuildingSupportChecks));
                WP_LOG("Terrain city collision smoke: PASS native static boxes="+StringUtil::toString(u32(count))+" surface rays="+StringUtil::toString(u32(check.rayProbes))+" building masses="+StringUtil::toString(u32(check.buildingBodies)));
                m_smokeTestPassed=true;app->setQuit(true);return;
            }
            if( m_assets.openCity )
            {
                // Registration/pose/face checks above cover every authored static box.
                // These longer downward probes verify the two distinct drive/walk grades.
                const auto roadPoint = m_assets.cityNaturalRoads && !m_assets.circuit.samples.empty()
                    ? m_assets.circuit.samples[std::min<size_t>( 10, m_assets.circuit.samples.size() - 1 )].position
                    : Vector3F( 0, 0, -m_citySpacing * .25f );
                if( count == 0 || !std::isfinite( m_citySpacing ) || m_citySpacing <= 0 ||
                    !physics->intersects( roadPoint + Vector3F( 0, 2, 0 ),
                                          roadPoint - Vector3F( 0, 1, 0 ),
                                          hitPosition, hitNormal, object ) || !object ||
                    !std::isfinite( hitPosition.y ) || std::abs( hitPosition.y ) > .02f )
                {
                    fail( "road-centre collision grade does not match Y=0" );
                    return;
                }
                const float roadY = hitPosition.y;
                if( !citySidewalk )
                {
                    fail( "no shared-descriptor sidewalk collision was generated" );
                    return;
                }
                // Stay inside the retained walk but away from its centre lamp.
                const auto walkSize = citySidewalk->getComponent<scene::CollisionBox>()->getExtents();
                const auto walkPoint = citySidewalk->getPosition() + citySidewalk->getOrientation() *
                    Vector3F( walkSize.x > walkSize.z ? walkSize.x * .2f : 0, 0,
                              walkSize.z >= walkSize.x ? walkSize.z * .2f : 0 );
                const auto isWalkBody = [&]( const SmartPtr<ISharedObject> &hit ) {
                    if( !m_assets.cityNaturalRoads )
                        return hit == citySidewalk->getComponent<scene::Rigidbody>()->getRigidStatic();
                    // Equal-grade natural footways can overlap between close parallel
                    // streets. Every body's placement was checked independently above.
                    for( const auto &candidate : m_assets.actors )
                        if( candidate && candidate->getName() == "Trackside collision: City sidewalk" )
                            if( auto body = candidate->getComponent<scene::Rigidbody>() )
                                if( hit == body->getRigidStatic() ) return true;
                    return false;
                };
                object = nullptr;
                if( !physics->intersects( { walkPoint.x, 2, walkPoint.z },
                                          { walkPoint.x, -1, walkPoint.z },
                                          hitPosition, hitNormal, object ) || !object ||
                    !isWalkBody( object ) ||
                    !std::isfinite( hitPosition.y ) || std::abs( hitPosition.y - .15f ) > .015f )
                {
                    fail( "sidewalk collision grade does not match Y=0.15" );
                    return;
                }
                const float sidewalkY = hitPosition.y;
                const auto city = procedural::OpenCityLayout::generate( m_seed, m_assets.cityBlocks, m_assets.cityRoute,
                                                                        m_assets.cityNaturalRoads );
                size_t expectedMasses = 0;
                for( const auto &lot : city.lots ) expectedMasses += procedural::OpenCityLayout::buildingVolumes( lot ).size();
                if( cityBuildingMasses != expectedMasses || expectedMasses == 0 )
                {
                    fail( "native building mass count does not match the shared layout" );
                    return;
                }
                WP_LOG( "City collision smoke: PASS native static boxes=" + StringUtil::toString( count ) +
                        " roadY=" + StringUtil::toString( roadY ) + " sidewalkY=" + StringUtil::toString( sidewalkY ) +
                        " building masses=" + StringUtil::toString( cityBuildingMasses ) );
                m_collisionProbeStarted = true;
                m_smokeTestPassed = true;
                app->setQuit( true );
                return;
            }
            if( count == 0 || physics->intersects( { 0, .5f, 2 }, { 0, .5f, 4 },
                                                  hitPosition, hitNormal, object ) )
            {
                fail( "gantry opening is obstructed or no colliders were generated" );
                return;
            }
            WP_LOG( "Trackside collision smoke: verified static boxes=" + StringUtil::toString(count) );
            // Coast into the first garage to verify actual contact response.
            auto car = m_vehicleActor->getComponent<scene::CarController>();
            car->getVehicleController()->reset();
            const Transform3F spawn( { -28, .42f, -3 }, QuaternionF::identity() );
            chassis->clearForce();
            chassis->clearTorque();
            chassis->setTransform( spawn );
            chassis->setLinearVelocity( { 0, 0, -10 } );
            chassis->setAngularVelocity( Vector3F::zero() );
            m_vehicleActor->setPosition( spawn.getPosition() );
            m_vehicleActor->setOrientation( spawn.getOrientation() );
            m_vehicleActor->updateTransform();
            m_collisionProbeStarted = true;
            return;
        }
        m_collisionProbeTime += std::clamp(app->getTimer()->getDeltaTime(), 0.0, 1.0 / 30.0);
        if( m_collisionProbeTime < 3 )
            return;
        const auto position = chassis->getTransform().getPosition();
        const auto velocity = chassis->getLinearVelocity();
        m_smokeTestPassed = position.z < -4 && position.z > -8 && std::abs(velocity.z) < 2;
        WP_LOG( String("Trackside collision smoke: garage impact ") +
                (m_smokeTestPassed ? "PASS" : "FAIL") + " position=" + StringUtil::toString(position) +
                " velocity=" + StringUtil::toString(velocity) );
        app->setQuit( true );
    }

    void SampleVehicleAdvanced::updateWheelVisuals()
    {
        m_raceScene->updateWheelVisuals();
    }

    void SampleVehicleAdvanced::updateSmokeTest()
    {
        auto app = core::IApplicationManager::instance();
        // Application allows scene state and resources to settle before stepping physics.
        if( app->getTimer()->getTimeSinceSceneLoad() <= 3.0 )
            return;
        m_smokeTime += Math<f64>::clamp( app->getTimer()->getDeltaTime(), 0.0, 1.0 / 30.0 );
        auto body = m_vehicleActor->getComponent<scene::Rigidbody>()->getRigidDynamic();
        const auto position = body->getTransform().getPosition();
        const auto settling = m_smokePhase == 0 || m_smokePhase == 4;
        const auto phaseDuration = settling ? 3.0 : m_smokePhase == 2 ? 2.0 : 4.0;
        // Ignore the initial drop, then monitor every physics step instead of just phase endpoints.
        if( !settling || m_smokeTime > phaseDuration - 1.0 )
        {
            m_smokeMinHeight = Math<real_Num>::min( m_smokeMinHeight, position.Y() );
            m_smokeMaxHeight = Math<real_Num>::max( m_smokeMaxHeight, position.Y() );
            m_smokePeakVerticalSpeed = Math<real_Num>::max(
                m_smokePeakVerticalSpeed, Math<real_Num>::Abs( body->getLinearVelocity().Y() ) );
        }
        auto horizontalVelocity = body->getLinearVelocity();
        horizontalVelocity.Y() = 0.0f;
        const auto speed = horizontalVelocity.length();
        if( speed > 5 )
        {
            const auto localVelocity =
                body->getTransform().getOrientation().inverse() * horizontalVelocity;
            m_smokePeakBodySlip = std::max(
                m_smokePeakBodySlip,
                real_Num( std::atan2( std::abs( localVelocity.X() ),
                                      std::max( std::abs( localVelocity.Z() ), real_Num( .1 ) ) ) ) );
            m_smokePeakYawRate =
                std::max( m_smokePeakYawRate, std::abs( body->getAngularVelocity().Y() ) );
        }
        if( m_smokeTime < phaseDuration )
            return;
        m_smokeTime = 0.0;
        bool passed = true;
        const auto heightRange = m_smokeMaxHeight - m_smokeMinHeight;
        const auto stable =
            m_smokeMinHeight > 0.0f && heightRange < 0.15f && m_smokePeakVerticalSpeed < 0.8f;
        switch( m_smokePhase )
        {
        case 0:
            passed = position.Y() > 0.0f && position.Y() < vehicleSpawnHeight && speed < 2.0f;
            m_smokeStartPosition = position;
            break;
        case 1:
            passed = ( position - m_smokeStartPosition ).length() > 1.0f && speed > 20.0f;
            m_smokeStartOrientation = body->getTransform().getOrientation();
            break;
        case 2:
        {
            auto heading = body->getTransform().getOrientation() * Vector3<real_Num>::unitZ();
            auto startHeading = m_smokeStartOrientation * Vector3<real_Num>::unitZ();
            heading.Y() = startHeading.Y() = 0.0f;
            heading.normalise();
            startHeading.normalise();
            passed = ( heading - startHeading ).length() > 0.02f;
            m_smokeDriveSpeed = speed;
            break;
        }
        case 3:
            passed = speed < m_smokeDriveSpeed;
            performReset();
            passed =
                passed &&
                ( body->getTransform().getPosition() - (m_assets.roadAuthoring && m_assets.roadTerrain?m_generatedSpawnPosition:getVehicleSpawnPosition()) ).length() < 0.01f &&
                body->getLinearVelocity().length() < 0.01f &&
                body->getAngularVelocity().length() < 0.01f;
            break;
        case 4:
            passed = position.Y() > 0.0f && position.Y() < vehicleSpawnHeight && speed < 2.0f;
            break;
        }
        passed = passed && stable && m_smokePeakBodySlip < .35f && m_smokePeakYawRate < 1.5f;
        if( m_smokePhase == 4 )
        {
            m_smokeTestPassed = passed;
            if( passed )
                WP_LOG( "Vehicle smoke: chassis and wheel render matrices match their poses." );
        }
        WP_LOG( String( "Vehicle smoke phase " ) + StringUtil::toString( m_smokePhase ) +
                ( passed ? ": PASS" : ": FAIL" ) + " position=" + StringUtil::toString( position ) +
                " speed=" + StringUtil::toString( speed ) +
                " min height=" + StringUtil::toString( m_smokeMinHeight ) +
                " max height=" + StringUtil::toString( m_smokeMaxHeight ) +
                " orientation=" + StringUtil::toString( body->getTransform().getOrientation() ) +
                " angular velocity=" + StringUtil::toString( body->getAngularVelocity() ) +
                " peak body slip=" + StringUtil::toString( m_smokePeakBodySlip ) +
                " peak yaw rate=" + StringUtil::toString( m_smokePeakYawRate ) +
                " height range=" + StringUtil::toString( heightRange ) +
                " peak vertical speed=" + StringUtil::toString( m_smokePeakVerticalSpeed ) );
        if( auto car = m_vehicleActor->getComponent<scene::CarController>() )
            for( u32 i = 0; i < 4; ++i )
            {
                auto wheel = car->getVehicleController()->getWheelController( i );
                float load = 0, acceleration = 0;
                auto props = wheel->getProperties();
                props->getPropertyValue( "Normal Force", load );
                props->getPropertyValue( "Contact Acceleration", acceleration );
                WP_LOG( "Handling wheel " + StringUtil::toString( i ) +
                        " compression=" + StringUtil::toString( wheel->getCompression() ) +
                        " load=" + StringUtil::toString( load ) +
                        " acceleration=" + StringUtil::toString( acceleration ) );
            }
        m_smokeMinHeight = std::numeric_limits<real_Num>::max();
        m_smokeMaxHeight = std::numeric_limits<real_Num>::lowest();
        m_smokePeakVerticalSpeed = 0.0f;
        m_smokePeakBodySlip = 0.0f;
        m_smokePeakYawRate = 0.0f;
        if( !passed || m_smokePhase == 4 )
        {
            app->setQuit( true );
            return;
        }
        ++m_smokePhase;
    }

    void SampleVehicleAdvanced::updateDebugText()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager ? applicationManager->getTimer() : nullptr;
        if( !m_hudEnabled || !timer || !m_vehicleActor ||
            timer->getTime() < m_nextDebugUpdate )
        {
            return;
        }
        const auto now = timer->getTime();
        m_nextDebugUpdate = now + 0.1;
        if(m_assets.circuit.samples.empty() || (m_assets.cityTerrainRoads && !m_assets.cityRaceRouteAvailable))
        {
            if(auto graphics=applicationManager->getGraphicsSystem())if(auto debug=graphics->getDebug())
            {debug->drawText(debugTextId,Vector2F(.02f,.14f),"Race route unavailable",0);debug->drawText(debugTextId+1,Vector2F(.02f,.18f),m_assets.cityRaceRouteError,0);debug->drawText(debugTextId+2,Vector2F(.02f,.22f),"Free city driving remains available.",0);}
            return;
        }
        if( m_lapStart == 0 )
            m_lapStart = now;
        auto car = m_vehicleActor->getComponent<scene::CarController>();
        auto vehicle = car ? car->getVehicleController() : nullptr;
        auto rigidbody = m_vehicleActor->getComponent<scene::Rigidbody>();
        const auto velocity = rigidbody ? rigidbody->getLinearVelocity() : Vector3<real_Num>::zero();
        const auto position = m_vehicleActor->getPosition();
        auto index = m_assets.circuit.nearest( position );
        auto roadOffset = position - m_assets.circuit.samples[index].position;
        roadOffset.y = 0;
        const auto onRoad = roadOffset.length() < 6.85f;
        if( m_lastTrackIndex > m_assets.circuit.samples.size() * 3 / 4 &&
            index < m_assets.circuit.samples.size() / 4 && m_nextCheckpoint == 4 && onRoad )
        {
            m_lastLapTime = now - m_lapStart;
            if( m_bestLapTime == 0 || m_lastLapTime < m_bestLapTime )
                m_bestLapTime = m_lastLapTime;
            ++m_lap;
            m_lapStart = now;
            m_nextCheckpoint = 1;
        }
        auto quarter = index * 4 / m_assets.circuit.samples.size();
        if( quarter == m_nextCheckpoint && onRoad )
            ++m_nextCheckpoint;
        m_lastTrackIndex = index;
        std::ostringstream text;
        text << std::fixed << std::setprecision( 1 );
        s32 gear = 0;
        double rpm = 0;
        if( vehicle && vehicle->getDriveTrain() )
        {
            auto props = vehicle->getDriveTrain()->getProperties();
            props->getPropertyValue( "Gear", gear );
            props->getPropertyValue( "RPM", rpm );
        }
        text << "GRAND PRIX | " << velocity.length() * 3.6f << " km/h | Gear "
             << ( gear == 0   ? "N"
                  : gear == 1 ? "R"
                              : std::to_string( gear - 1 ) )
             << " | " << int( rpm ) << " rpm | Lap " << m_lap << '\n';
        text << "Lap time " << ( m_lapStart > 0 ? now - m_lapStart : 0 ) << " s | Last " << m_lastLapTime
             << " s | Best " << m_bestLapTime << " s\n";
        text << "Circuit " << m_assets.circuit.length << " m | Seed " << m_seed;
        if( ( position - m_assets.circuit.samples[index].position ).length() > 11 )
            text << " | OFF TRACK";
        if( auto graphics = applicationManager->getGraphicsSystem() )
        {
            if( auto debug = graphics->getDebug() )
            {
                std::istringstream lines( text.str() );
                std::string line;
                for( size_t i = 0; std::getline( lines, line ); ++i )
                {
                    debug->drawText( debugTextId + i,
                                     Vector2<real_Num>( 0.02f, 0.14f + static_cast<f32>( i ) * 0.04f ),
                                     String( line.c_str() ), 0 );
                }
            }
        }
        if( now >= m_nextDebugLog )
        {
            WP_LOG( String( text.str().c_str() ) );
            if( ( m_smokeTest || m_trackSmokeTest ) && vehicle )
            {
                auto w = vehicle->getWheelController( 0 );
                WP_LOG(
                    String( "Advanced physics: dt=" ) + StringUtil::toString( timer->getDeltaTime() ) +
                    " mass=" + StringUtil::toString( rigidbody->getRigidDynamic()->getMass() ) +
                    " inertia=" +
                    StringUtil::toString( rigidbody->getRigidDynamic()->getMassSpaceInertiaTensor() ) +
                    " angular=" + StringUtil::toString( rigidbody->getAngularVelocity() ) +
                    " controllerMass=" + StringUtil::toString( vehicle->getBody()->getMass() ) +
                    " root=" + StringUtil::toString( position ) +
                    " mount=" + StringUtil::toString( w->getLocalTransform().getPosition() ) +
                    " rate=" + StringUtil::toString( w->getSpringRate() ) +
                    " damping=" + StringUtil::toString( w->getDamping() ) +
                    " travel=" + StringUtil::toString( w->getSuspensionTravel() ) );
            }
            m_nextDebugLog = now + 2.0;
        }
    }

    bool SampleVehicleAdvanced::createSoundManager()
    {
        if( !m_audioEnabled )
            return Application::createSoundManager();
        auto app = core::IApplicationManager::instance();
        // Select the platform backend explicitly: the generic factory can resolve
        // ISoundManager to the base SoundManager, which has no playback device.
        auto factory = app->getFactoryManager()->getFactoryByName( "workphone::WPAudioManager" );
        if( !factory )
        {
            WP_LOG_WARNING( "Vehicle audio: WPAudioManager factory is unavailable." );
            return false;
        }
        auto manager = factory->make_ptr<ISoundManager>();
        manager->load( nullptr );
        app->setSoundManager( manager );
        WP_LOG( "Vehicle audio manager: " + factory->getObjectTypeName() +
                ( manager->isLoaded() ? " loaded" : " initialization failed" ) );
        return manager->isLoaded();
    }

    void SampleVehicleAdvanced::createPlugins()
    {
        Application::createPlugins();

#ifndef _WP_STATIC_LIB_
        auto factoryManager = core::IApplicationManager::instance()->getFactoryManager();
        if( m_audioEnabled && !factoryManager->hasFactoryByName( "workphone::WPAudioManager" ) )
        {
            auto job = factoryManager->make_ptr<LoadPluginJob>();
            job->setPluginPath( "WPAudio" );
            job->execute();
        }
        if( !factoryManager->hasFactoryByName( "workphone::CCarController" ) )
        {
            auto job = factoryManager->make_ptr<LoadPluginJob>();
            job->setPluginPath( "WPVehiclePhysics" );
            job->execute();
        }
        if( !factoryManager->hasFactoryByName( "IVehicleGenerator" ) )
        {
            auto plugin = workphone::make_ptr<procedural::WPProcedural>();
            core::IApplicationManager::instance()->addPlugin( plugin );
        }
#endif

#ifdef _WP_STATIC_LIB_
        try
        {
            WP_DEBUG_TRACE;

            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto corePlugin = workphone::make_ptr<WPCore>();
            applicationManager->addPlugin( corePlugin );
            applicationManager->addPlugin( workphone::make_ptr<WPVehiclePhysics>() );
            applicationManager->addPlugin( workphone::make_ptr<procedural::WPProcedural>() );

            auto databasePlugin = workphone::make_ptr<SQLitePlugin>();
            applicationManager->addPlugin( databasePlugin );

            auto inputPlugin = workphone::make_ptr<OISInput>();
            applicationManager->addPlugin( inputPlugin );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
            auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgreNext>();
            applicationManager->addPlugin( graphicsPlugin );
#    elif WP_GRAPHICS_SYSTEM_OGRE
            auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgre>();
            applicationManager->addPlugin( graphicsPlugin );
#    endif

#    if WP_BUILD_PHYSX
            auto physxPlugin = workphone::make_ptr<physics::FBPhysx>();
            applicationManager->addPlugin( physxPlugin );
#    elif WP_BUILD_ODE
#    endif

            ApplicationUtil::createFactories();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
#endif
    }

    void SampleVehicleAdvanced::createScene()
    {
        if( m_sceneProfile ) m_sceneProfile->checkpoint( "runtime_before_scene_generation" );
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto physicsManager = applicationManager->getPhysicsManager();
        WP_ASSERT( physicsManager );
        if( !physicsManager->getPhysicsScene() )
        {
            physicsManager->setPhysicsScene( physicsManager->addScene() );
        }
        WP_ASSERT( physicsManager->getPhysicsScene() );
        physicsManager->getPhysicsScene()->setGravity( Vector3<real_Num>( 0.0f, -9.81f, 0.0f ) );
        if( auto vehicleManager = applicationManager->getVehicleManager() )
        {
            vehicleManager->load( nullptr );
        }

        m_vehicleActor = sceneManager->createActor();
        m_vehicleActor->setName( "Procedural Grand Prix" );
        m_vehicleActor->setPosition( getVehicleSpawnPosition() );
        m_raceScene = m_vehicleActor->addComponent<scene::ProceduralRaceScene>();
        m_raceScene->setAudioEnabled( m_audioEnabled );
        m_raceScene->setEffectsEnabled( m_effectsEnabled );
        m_raceScene->setAutomaticPresentation( !m_audioSmokeTest && !m_effectsSmokeTest );
        m_raceScene->setSeed( m_seed );
        m_raceScene->setQuality( static_cast<s32>( m_quality ) );
        m_raceScene->setFoliageWorldEnabled( m_foliageWorldEnabled );
        m_raceScene->setMaterialExportDirectory( m_materialExportDirectory );
        if( m_openCity )
        {
            auto properties = m_raceScene->getProperties();
            properties->setProperty( "Open City", true );
            properties->setProperty( "City Blocks", m_cityBlocks );
            properties->setProperty( "City Route", m_cityRoute );
            properties->setProperty( "City Natural Roads", m_cityNaturalRoads );
            properties->setProperty( "Road Authoring Showcase", m_roadAuthoringShowcase );
            properties->setProperty( "City Terrain Roads", m_cityTerrainRoads );
            m_raceScene->setProperties( properties );
        }
        if( !m_raceScene->regenerate() )
            throw std::runtime_error( m_raceScene->getGenerationError() );
        m_assets = m_raceScene->getAssets();
        m_cityExtent = m_citySpacing = 0;
        m_cityOverviewExtent = 0;
        m_cityOverviewCentre = m_cityStreetCentre = Vector3F::zero();
        m_cityStreetForward = { 0, 0, -1 };
        m_cityHasJunction=false;m_cityJunctionCentre=m_cityBoulevardCentre={};m_generatedSpawnPosition=m_vehicleActor->getPosition();
        if(m_assets.cityTerrainRoads && m_assets.roadAuthoring && m_assets.roadTerrain)
        {
            const auto &layout=m_assets.cityLayout;const auto &bake=m_assets.roadAuthoring->getBake();const auto &source=m_assets.roadAuthoring->getSource();
            m_citySpacing=layout.spacing;float minX=std::numeric_limits<float>::max(),minZ=minX,minY=minX,maxX=-minX,maxZ=-minX,maxY=-minX;
            const auto include=[&](Vector3F p){minX=std::min(minX,p.x);minZ=std::min(minZ,p.z);minY=std::min(minY,p.y);maxX=std::max(maxX,p.x);maxZ=std::max(maxZ,p.z);maxY=std::max(maxY,p.y);};
            size_t longest=0;
            for(const auto &road:bake.roads)
            {
                for(const auto &point:road.points)include(point.position);
                if(road.points.size()>longest){longest=road.points.size();m_cityStreetCentre=road.points[road.points.size()/2].position;m_cityStreetForward=road.points[road.points.size()/2].tangent;}
            }
            for(const auto &lot:layout.lots){include({lot.x-lot.width*.5f,lot.groundHeight,lot.z-lot.depth*.5f});include({lot.x+lot.width*.5f,lot.groundHeight,lot.z+lot.depth*.5f});}
            if(minX<=maxX){m_cityExtent=std::max({std::abs(minX),std::abs(maxX),std::abs(minZ),std::abs(maxZ)});m_cityOverviewExtent=std::max(maxX-minX,maxZ-minZ)*.5f+20;m_cityOverviewCentre={(minX+maxX)*.5f,(minY+maxY)*.5f+8,(minZ+maxZ)*.5f};}
            float nearest=std::numeric_limits<float>::max();
            for(const auto &junction:source.roundabouts)if(!junction.ports.empty())
            {
                const float distance=(junction.center-m_cityOverviewCentre).lengthSquared();if(distance>=nearest)continue;nearest=distance;m_cityHasJunction=true;m_cityJunctionCentre=junction.center;
                const auto cameraPoint=[&](const auto &port){auto p=junction.center+port.outward*(junction.outerRadius+port.length*.65f);f32 height=0;bool driveable=false;if(m_assets.roadAuthoring->getRuntimeSurface(p.x,p.z,height,driveable))p.y=height;return p;};
                m_cityStreetCentre=cameraPoint(junction.ports.front());m_cityBoulevardCentre=cameraPoint(junction.ports[junction.ports.size()/2]);m_cityStreetForward=junction.ports.front().outward;
            }
        }
        else if( m_assets.cityRoadShowcase )
        {
            m_cityExtent=m_cityOverviewExtent=96;m_citySpacing=32;m_cityOverviewCentre={0,3,0};
        }
        else if( m_assets.openCity )
        {
            const auto city = procedural::OpenCityLayout::generate( m_seed, m_assets.cityBlocks, m_assets.cityRoute,
                                                                    m_assets.cityNaturalRoads );
            m_cityExtent = city.extent;
            m_citySpacing = city.spacing;
            m_cityOverviewExtent = city.extent;
            if( city.naturalRoads && !city.roads.nodes.empty() )
            {
                float minX = std::numeric_limits<float>::max(), minZ = minX;
                float maxX = -minX, maxZ = -minX;
                for( const auto &node : city.roads.nodes )
                {
                    minX = std::min( minX, node.x - 9.f ); maxX = std::max( maxX, node.x + 9.f );
                    minZ = std::min( minZ, node.z - 9.f ); maxZ = std::max( maxZ, node.z + 9.f );
                }
                for( const auto &lot : city.lots )
                {
                    minX = std::min( minX, lot.x - lot.width * .5f ); maxX = std::max( maxX, lot.x + lot.width * .5f );
                    minZ = std::min( minZ, lot.z - lot.depth * .5f ); maxZ = std::max( maxZ, lot.z + lot.depth * .5f );
                }
                m_cityOverviewCentre = { ( minX + maxX ) * .5f, 0, ( minZ + maxZ ) * .5f };
                m_cityOverviewExtent = std::max( city.spacing, std::max( maxX - minX, maxZ - minZ ) * .5f + 20.f );
                float best = std::numeric_limits<float>::max();
                const float scale = city.spacing * city.spacing;
                for( const auto &edge : city.roads.edges )
                {
                    if( edge.a >= city.roads.nodes.size() || edge.b >= city.roads.nodes.size() ) continue;
                    const auto &a = city.roads.nodes[edge.a], &b = city.roads.nodes[edge.b];
                    const Vector3F delta( b.x - a.x, 0, b.z - a.z );
                    const float lengthSquared = delta.lengthSquared();
                    if( lengthSquared < 1.f ) continue;
                    const Vector3F centre( ( a.x + b.x ) * .5f, 0, ( a.z + b.z ) * .5f );
                    // Prefer a central avenue with enough uninterrupted length for
                    // both opposing inspection views; never assume a +Z grid street.
                    const float score = ( centre - m_cityOverviewCentre ).lengthSquared() + scale * scale / lengthSquared;
                    if( score < best )
                    {
                        best = score; m_cityStreetCentre = centre;
                        m_cityStreetForward = delta / std::sqrt( lengthSquared );
                    }
                }
            }
        }
#if WP_GRAPHICS_SYSTEM_CLAW
        if( auto scene = dynamic_pointer_cast<render::ClawScene>(
                applicationManager->getGraphicsSystem()->getGraphicsScene() ) )
            if( !scene->setPrimaryDirectionalLight( m_assets.sun ) )
                throw std::runtime_error( "Cannot select the procedural sky's sunlight." );
#endif
        for( auto &group : m_assets.treeLODs ) group->setForcedLOD( m_forcedLOD );
        if( m_assets.foliageTerrain )
        {
            String error;
            if( !m_assets.foliageTerrain->setFoliageForcedLod( m_forcedLOD, error ) )
                throw std::runtime_error( error );
        }
        m_assets.vehicleLOD->setForcedLOD( m_forcedVehicleLOD );
        // The fixed high-altitude circuit review must retain surface contrast.
        if( m_captureView == "track" && !m_orbitCamera )
            applicationManager->getGraphicsSystem()->getGraphicsScene()->setFog(
                render::IGraphicsScene::FOG_NONE );
        if( auto window = applicationManager->getGraphicsSystem()->getDefaultWindow() )
            window->setSize( Vector2I( m_reviewWidth, m_reviewHeight ) );
        m_boxGround = m_assets.ground;
        m_chassisMeshActor = m_assets.body;
        m_wheelActors = m_assets.wheels;
        scene->addActor( m_vehicleActor );
        scene->registerAllUpdates( m_vehicleActor );

        m_cameraActor = sceneManager->createActor();
        WP_ASSERT( m_cameraActor );
        m_cameraActor->setName( "Vehicle Follow Camera" );
        m_cameraActor->setSmoothMotion( true );
        const bool terrainRoads=m_assets.roadAuthoring && m_assets.roadTerrain;
        const auto cameraTarget=terrainRoads?m_generatedSpawnPosition:getVehicleSpawnPosition();
        m_cameraActor->setPosition(terrainRoads?cameraTarget+Vector3F(0,cameraHeight,cameraDistance):getInitialCameraPosition());
        m_cameraActor->lookAt(cameraTarget,Vector3<real_Num>::unitY());

        m_cameraController = m_cameraActor->addComponent<scene::VehicleCameraController>();
        WP_ASSERT( m_cameraController );
        m_cameraController->setTarget( m_vehicleActor );
        m_cameraController->setDistance( cameraDistance );
        m_cameraController->setHeight( cameraHeight );

        scene->addActor( m_cameraActor );
        scene->registerAllUpdates( m_cameraActor );

        if( m_camera )
        {
            // The overhead view needs more depth precision to resolve the road's
            // millimetre surface offsets at a distance of nearly 500 metres.
            m_camera->setNearClipDistance( m_captureView == "track" && !m_orbitCamera ? 10.f : .5f );
            m_camera->setFarClipDistance( std::max( 1000.f, m_cityExtent * 6.f ) );
        }
        updateRenderCamera();

        if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
        {
            if( auto debug = m_hudEnabled ? graphicsSystem->getDebug() : nullptr )
            {
                debug->drawText(
                    0, Vector2F( 0.02f, 0.02f ),
                    "W/Up: throttle  S/Down: brake  A/D or Left/Right: steer  R: reset  Esc: quit", 0 );
                debug->drawText( 1, Vector2F( 0.02f, 0.06f ), "Mouse wheel: camera zoom", 0 );
            }
        }
        if( m_sceneProfile )
        {
            m_sceneProfile->setWorkload( describeProfileScene( m_assets ) );
            m_sceneProfile->checkpoint( "scene_generated" );
        }
    }

    void SampleVehicleAdvanced::updateRenderCamera()
    {
        if( m_cameraSceneNode && m_cameraActor )
        {
            if( auto transform = m_cameraActor->getTransform() )
            {
                m_cameraSceneNode->setTransform( transform->getWorldTransform() );
                if( ( !m_capturePath.empty() || m_benchmarkSeconds > 0 ) && ( m_captureView != "follow" || m_orbitCamera ) )
                {
                    if( m_orbitCamera )
                    {
                        const auto elapsed = core::IApplicationManager::instance()->getTimer()->getTimeSinceSceneLoad();
                        const auto angle = float( std::max( elapsed - 5.0, 0.0 ) * .15 );
                        const auto target = (m_assets.circuit.samples.empty()?m_cityOverviewCentre:m_assets.circuit.samples.front().position) + Vector3F( 0, 1, 0 );
                        const auto radius = 45.f + 35.f * ( 1 + std::sin( angle * .5f ) );
                        m_cameraActor->setPosition( target + Vector3F( std::sin(angle)*radius, 8, std::cos(angle)*radius ) );
                        m_cameraActor->lookAt( target, Vector3F::unitY() );
                    }
                    else if( m_captureView == "corner" && !m_assets.circuit.samples.empty() )
                    {
                        const auto &point =
                            m_assets.circuit.samples[m_assets.circuit.samples.size() / 4];
                        m_cameraActor->setPosition( point.position + point.right * 22.f +
                                                    Vector3F( 0, 10, 0 ) );
                        m_cameraActor->lookAt( point.position, Vector3F::unitY() );
                    }
                    else if( m_captureView == "track" )
                    {
                        if( m_assets.cityRoadShowcase )
                        {
                            m_cameraActor->setPosition({110,150,110});m_cameraActor->lookAt({0,3,0},Vector3F::unitY());
                        }
                        else if( m_openCity )
                        {
                            m_cameraActor->setPosition( m_cityOverviewCentre + Vector3F(
                                m_cityOverviewExtent * 1.2f, m_cityOverviewExtent * 1.9f, m_cityOverviewExtent * 1.35f ) );
                            m_cameraActor->lookAt( m_cityOverviewCentre, Vector3F::unitY() );
                        }
                        else
                        {
                            m_cameraActor->setPosition( Vector3F( 75, 480, -30 ) );
                            m_cameraActor->lookAt( Vector3F( 75, 0, -30 ), Vector3F::unitZ() );
                        }
                    }
                    else if( m_captureView == "street" || m_captureView == "boulevard" )
                    {
                        if(m_assets.cityTerrainRoads)
                        {
                            if(m_cityHasJunction)
                            {m_cameraActor->setPosition((m_captureView=="street"?m_cityStreetCentre:m_cityBoulevardCentre)+Vector3F(0,16,0));m_cameraActor->lookAt(m_cityJunctionCentre+Vector3F(0,2,0),Vector3F::unitY());}
                            else{m_cameraActor->setPosition(m_cityStreetCentre+Vector3F(0,4,0));m_cameraActor->lookAt(m_cityStreetCentre+m_cityStreetForward*40.f+Vector3F(0,5,0),Vector3F::unitY());}
                        }
                        else if(m_assets.cityRoadShowcase)
                        {
                            m_cameraActor->setPosition(m_captureView=="street"?Vector3F(24,22,28):Vector3F(-24,22,-28));
                            m_cameraActor->lookAt({0,3,0},Vector3F::unitY());
                        }
                        else
                        {
                        const float travel = m_captureView == "boulevard" ? -1.f : 1.f;
                        const Vector3F right( -m_cityStreetForward.z, 0, m_cityStreetForward.x );
                        m_cameraActor->setPosition( m_cityStreetCentre +
                            m_cityStreetForward * ( travel * m_citySpacing * .12f ) + Vector3F( 0, 3.2f, 0 ) );
                        m_cameraActor->lookAt( m_cityStreetCentre +
                            m_cityStreetForward * ( travel * m_citySpacing * .60f ) +
                            right * ( travel * m_citySpacing * .24f ) + Vector3F( 0, 8, 0 ), Vector3F::unitY() );
                        }
                    }
                    else
                    {
                        auto target = m_vehicleActor->getPosition();
                        m_cameraActor->setPosition( target + Vector3F( -5.5f, 2.2f, -6.5f ) );
                        m_cameraActor->lookAt( target, Vector3F::unitY() );
                    }
                    m_cameraActor->updateTransform();
                    m_cameraSceneNode->setTransform( m_cameraActor->getWorldTransform() );
                }
            }
        }
        if( m_camera && m_cameraActor && m_assets.vehicleLOD )
        {
            auto system = workphone::static_pointer_cast<scene::LODSystem>(
                m_assets.vehicleLOD->getComponentSystem() );
            if( system )
            {
                scene::LODSystem::View view;
                view.position = m_cameraActor->getWorldTransform().getPosition();
                view.verticalFovRadians = Math<f32>::DegToRad( m_camera->getFOVy() );
                view.nearClipDistance = m_camera->getNearClipDistance();
                view.lodBias = m_camera->getLodBias();
                system->setViewOverride( view );
            }
        }
    }

    SampleVehicleAdvanced::InputListener::InputListener() = default;

    SampleVehicleAdvanced::InputListener::~InputListener() = default;

    void SampleVehicleAdvanced::InputListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    Parameter SampleVehicleAdvanced::InputListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::inputEvent )
        {
            auto result = inputEvent( event );
            return Parameter( result );
        }

        return Parameter();
    }

    bool SampleVehicleAdvanced::InputListener::inputEvent( SmartPtr<IInputEvent> event )
    {
        if( !event )
        {
            return false;
        }

        if( auto owner = getOwner() )
        {
            switch( event->getEventType() )
            {
            case IInputEvent::EventType::Key:
            {
                if( auto keyboardState = event->getKeyboardState() )
                {
                    if( keyboardState->isPressedDown() )
                    {
                        if( keyboardState->getKeyCode() == static_cast<u32>( KeyCodes::KEY_KEY_R ) )
                        {
                            owner->reset();
                            return true;
                        }
                        if( keyboardState->getKeyCode() == static_cast<u32>( KeyCodes::KEY_ESCAPE ) )
                        {
                            core::IApplicationManager::instance()->setQuit( true );
                            return true;
                        }
                    }
                }
            }
            break;
            default:
                break;
            };
        }

        return false;
    }

    void SampleVehicleAdvanced::InputListener::setPriority( s32 priority )
    {
        m_priority = priority;
    }

    s32 SampleVehicleAdvanced::InputListener::getPriority() const
    {
        return m_priority;
    }

    SmartPtr<SampleVehicleAdvanced> SampleVehicleAdvanced::InputListener::getOwner() const
    {
        return m_owner.lock();
    }

    void SampleVehicleAdvanced::InputListener::setOwner( SmartPtr<SampleVehicleAdvanced> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone

int main( int argc, char **argv )
{
    using namespace workphone;

    for( int i = 1; i < argc; ++i )
    {
        if( std::string( argv[i] ) == "--validate-circuit" )
        {
            try
            {
                advanced::validateCircuit();
                std::cout << "100 seeded circuits: closure, spacing, spawn and determinism passed\n";
                return 0;
            }
            catch( const std::exception &e )
            {
                std::cerr << e.what() << '\n';
                return 1;
            }
        }
        if( std::string( argv[i] ) == "--help" )
        {
            std::cout
                << "SampleVehicleAdvanced [--seed N] [--quality low|medium|high|cinematic] [--plugins PATH]\n"
                   "  --smoke-test                 Settle, accelerate, turn, brake and reset checks\n"
                   "  --track-smoke-test            Drive one full lap through the physics controller\n"
                   "  --collision-smoke-test        Verify native boxes; circuit garage impact or city road/sidewalk grades\n"
                   "  --validate-circuit           Validate 100 generated closed circuits\n"
                   "  --capture PATH.bmp --view follow|car|track|corner|street|boulevard   Save a rendered view and "
                   "exit\n"
                   "  --no-hud --force-lod auto|0|1  Clean captures and tree LOD comparisons\n"
                   "  --force-vehicle-lod auto|0|1|2  Override the vehicle level (clamped for low)\n"
                   "  --benchmark SECONDS [--orbit] Measure after five seconds of warmup\n"
                   "  --scene-profile NEW.json      Profile the existing scene; requires --benchmark\n"
                   "  --scene-profile-mode memory|cpu  Heap checkpoints or CPU accounting (default memory)\n"
                   "  --evidence PATH.json          Write machine-readable review results\n"
                   "  --replay PATH.csv             Apply 120 Hz tick,throttle,brake,steering inputs\n"
                   "  --foliage-world               Use paged instanced broadleaf vegetation\n"
                   "  --city                        Generate the existing open-city scene instead of a circuit\n"
                   "  --city-blocks 6|8|10           Blocks per side (requires --city; default 8)\n"
                   "  --city-route 0|1|2             Street race route (requires --city; default 0)\n"
                   "  --city-layout grid|lsystem     Road network layout (requires --city; default grid)\n"
                   "  --road-authoring-showcase       Terrain-fitted editable roads and roundabout (implies --city)\n"
                   "  --city-terrain-roads            Editable generated city roads fitted to terrain (implies --city)\n"
                   "  --view street|boulevard       Opposite street-level city inspections; track is the overview\n"
                   "  --export-materials NEW_DIR    Bake asphalt maps and resource source descriptors\n"
                   "  --resolution WIDTHxHEIGHT     Set the review window resolution\n"
                   "  --no-audio                    Disable vehicle sounds\n"
                   "  --audio-smoke-test            Check idle, redline, skid and mute playback\n"
                   "  --no-effects                  Disable particles and skid decals\n"
                   "  --effects-smoke-test [--effects-capture PATH.bmp] Check effects and reset\n"
                   "  W/Up throttle; S/Down brake; A/D steer; R reset; Esc quit; wheel camera zoom\n";
            return 0;
        }
    }
    TypeManager *typeManager = nullptr;
    bool ownsTypeManager = false;
    SmartPtr<SampleVehicleAdvanced> app;
    bool smokeTest = false, trackSmokeTest = false, collisionSmokeTest = false;
    bool audioEnabled = true, audioSmokeTest = false;
    bool effectsEnabled = true, effectsSmokeTest = false;
    std::string effectsCapture;
    String pluginsConfig;
    std::string capturePath, captureView = "follow";
    std::string evidencePath, replayPath;
    std::vector<advanced::ReviewControl> replayControls;
    std::string materialExportDirectory;
    std::string sceneProfilePath, sceneProfileError;
    auto sceneProfileMode = advanced::ProductionSceneProfile::Mode::Memory;
    bool sceneProfileModeExplicit = false;
    std::unique_ptr<advanced::ProductionSceneProfile> sceneProfile;
    u32 seed = 7;
    auto quality = procedural::VehicleAppearanceQuality::High;
    bool hud = true, orbit = false;
    bool foliageWorld = false;
    bool openCity = false, cityOptionsExplicit = false;
    bool cityNaturalRoads = false;
    bool roadAuthoringShowcase = false;
    bool cityTerrainRoads = false;
    s32 cityBlocks = 8, cityRoute = 0;
    s32 forcedLOD = -1;
    s32 forcedVehicleLOD = -1;
    f64 benchmarkSeconds = 0;
    u32 width = 1280, height = 720;
    try
    {
        for( int i = 1; i < argc; ++i )
        {
            if( String( argv[i] ) == "--smoke-test" )
                smokeTest = true;
            else if( String( argv[i] ) == "--no-audio" )
                audioEnabled = false;
            else if( String( argv[i] ) == "--audio-smoke-test" )
                audioSmokeTest = true;
            else if( String( argv[i] ) == "--no-effects" )
                effectsEnabled = false;
            else if( String( argv[i] ) == "--effects-smoke-test" )
                effectsSmokeTest = true;
            else if( String( argv[i] ) == "--effects-capture" && i + 1 < argc )
                effectsCapture = argv[++i];
            else if( String( argv[i] ) == "--collision-smoke-test" )
                collisionSmokeTest = true;
            else if( String( argv[i] ) == "--no-hud" )
                hud = false;
            else if( String( argv[i] ) == "--orbit" )
                orbit = true;
            else if( String( argv[i] ) == "--foliage-world" )
                foliageWorld = true;
            else if( String( argv[i] ) == "--city" )
                openCity = true;
            else if( String( argv[i] ) == "--road-authoring-showcase" )
            { roadAuthoringShowcase = true; openCity = true; }
            else if( String( argv[i] ) == "--city-terrain-roads" )
            { cityTerrainRoads = true; openCity = true; }
            else if( String( argv[i] ) == "--city-blocks" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                if( value != "6" && value != "8" && value != "10" )
                    throw std::runtime_error( "City blocks must be 6, 8 or 10." );
                cityBlocks = std::stoi( value ); cityOptionsExplicit = true;
            }
            else if( String( argv[i] ) == "--city-route" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                if( value != "0" && value != "1" && value != "2" )
                    throw std::runtime_error( "City route must be 0, 1 or 2." );
                cityRoute = std::stoi( value ); cityOptionsExplicit = true;
            }
            else if( String( argv[i] ) == "--city-layout" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                if( value != "grid" && value != "lsystem" )
                    throw std::runtime_error( "City layout must be grid or lsystem." );
                cityNaturalRoads = value == "lsystem"; cityOptionsExplicit = true;
            }
            else if( String( argv[i] ) == "--force-lod" && i + 1 < argc )
            {
                const String value = argv[++i];
                if( value != "auto" && value != "0" && value != "1" )
                    throw std::runtime_error( "Forced LOD must be auto, 0, or 1." );
                forcedLOD = value == "auto" ? -1 : value == "0" ? 0 : 1;
            }
            else if( String( argv[i] ) == "--force-vehicle-lod" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                if( value != "auto" && value != "0" && value != "1" && value != "2" )
                    throw std::runtime_error( "Vehicle LOD must be auto, 0, 1 or 2." );
                forcedVehicleLOD = value == "auto" ? -1 : std::stoi( value );
            }
            else if( String( argv[i] ) == "--benchmark" && i + 1 < argc )
            {
                size_t consumed = 0;
                const std::string value = argv[++i];
                benchmarkSeconds = std::stod( value, &consumed );
                if( consumed != value.size() || !std::isfinite( benchmarkSeconds ) ||
                    benchmarkSeconds <= 0 || benchmarkSeconds > 3600 )
                    throw std::runtime_error(
                        "Benchmark duration must be between zero and 3600 seconds." );
            }
            else if( String( argv[i] ) == "--resolution" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                const auto separator = value.find( 'x' );
                if( separator == std::string::npos )
                    throw std::runtime_error( "Resolution must be WIDTHxHEIGHT." );
                size_t left = 0, right = 0;
                width = std::stoul( value.substr( 0, separator ), &left );
                height = std::stoul( value.substr( separator + 1 ), &right );
                if( left != separator || right != value.size() - separator - 1 || width < 320 ||
                    height < 180 || width > 7680 || height > 4320 )
                    throw std::runtime_error( "Resolution must be between 320x180 and 7680x4320." );
            }
            else if( String( argv[i] ) == "--track-smoke-test" )
                trackSmokeTest = true;
            else if( String( argv[i] ) == "--plugins" && i + 1 < argc )
                pluginsConfig = argv[++i];
            else if( String( argv[i] ) == "--capture" && i + 1 < argc )
                capturePath = argv[++i];
            else if( String( argv[i] ) == "--evidence" && i + 1 < argc )
                evidencePath = argv[++i];
            else if( String( argv[i] ) == "--scene-profile" && i + 1 < argc )
                sceneProfilePath = argv[++i];
            else if( String( argv[i] ) == "--scene-profile-mode" && i + 1 < argc )
            {
                const std::string mode = argv[++i];
                if( mode != "memory" && mode != "cpu" )
                    throw std::runtime_error( "Scene profile mode must be memory or cpu." );
                sceneProfileMode = mode == "memory" ? advanced::ProductionSceneProfile::Mode::Memory
                                                      : advanced::ProductionSceneProfile::Mode::Cpu;
                sceneProfileModeExplicit = true;
            }
            else if( String( argv[i] ) == "--replay" && i + 1 < argc )
                replayPath = argv[++i];
            else if( String( argv[i] ) == "--export-materials" && i + 1 < argc )
                materialExportDirectory = argv[++i];
            else if( String( argv[i] ) == "--view" && i + 1 < argc )
                captureView = argv[++i];
            else if( String( argv[i] ) == "--seed" && i + 1 < argc )
                seed = static_cast<u32>( std::stoul( argv[++i] ) );
            else if( String( argv[i] ) == "--quality" && i + 1 < argc )
            {
                const String value = argv[++i];
                if( value == "low" )
                    quality = procedural::VehicleAppearanceQuality::Preview;
                else if( value == "medium" )
                    quality = procedural::VehicleAppearanceQuality::Standard;
                else if( value == "high" )
                    quality = procedural::VehicleAppearanceQuality::High;
                else if( value == "cinematic" )
                    quality = procedural::VehicleAppearanceQuality::Cinematic;
                else
                    throw std::runtime_error( "Quality must be low, medium, high, or cinematic." );
            }
            else
                throw std::runtime_error( std::string( "Unknown option or missing value: " ) + argv[i] );
        }
        if( captureView != "follow" && captureView != "car" && captureView != "track" &&
            captureView != "corner" && captureView != "street" && captureView != "boulevard" )
            throw std::runtime_error( "View must be follow, car, track, corner, street, or boulevard." );
        if( ( cityOptionsExplicit || captureView == "street" || captureView == "boulevard" ) && !openCity )
            throw std::runtime_error( "City options and street-level views require --city." );
        if(roadAuthoringShowcase && foliageWorld)throw std::runtime_error("Road authoring showcase does not provide foliage-world generation.");
        if(roadAuthoringShowcase && cityTerrainRoads)throw std::runtime_error("Choose road-authoring-showcase or city-terrain-roads.");
        if( int(smokeTest) + int(trackSmokeTest) + int(collisionSmokeTest) + int(audioSmokeTest) + int(effectsSmokeTest) > 1 ||
            ( ( !capturePath.empty() || benchmarkSeconds > 0 ) && ( smokeTest || trackSmokeTest || collisionSmokeTest || audioSmokeTest || effectsSmokeTest ) ) )
            throw std::runtime_error( "Choose one smoke test or a capture per run." );
        if( audioSmokeTest && !audioEnabled )
            throw std::runtime_error( "--audio-smoke-test requires audio to be enabled." );
        if( (effectsSmokeTest && !effectsEnabled) || (!effectsCapture.empty() && !effectsSmokeTest) )
            throw std::runtime_error("Effects capture requires --effects-smoke-test with effects enabled.");
        if( orbit && benchmarkSeconds <= 0 ) throw std::runtime_error( "--orbit requires --benchmark." );
        if( ( !evidencePath.empty() || !replayPath.empty() ) && capturePath.empty() && benchmarkSeconds <= 0 )
            throw std::runtime_error( "--evidence and --replay require a capture or benchmark run." );
        if( advanced::reviewPathsAlias( evidencePath, capturePath ) || advanced::reviewPathsAlias( evidencePath, replayPath ) ||
            advanced::reviewPathsAlias( capturePath, replayPath ) )
            throw std::runtime_error( "Evidence output must differ from capture and replay paths." );
        if( sceneProfileModeExplicit && sceneProfilePath.empty() )
            throw std::runtime_error( "--scene-profile-mode requires --scene-profile." );
        if( !sceneProfilePath.empty() )
        {
            if( benchmarkSeconds <= 0 ) throw std::runtime_error( "--scene-profile requires --benchmark." );
#if !defined( _WIN32 ) || !WP_GRAPHICS_SYSTEM_CLAW
            throw std::runtime_error( "Production scene profiling requires Windows and Claw graphics." );
#endif
            const auto output = std::filesystem::absolute( sceneProfilePath );
            if( std::filesystem::exists( output ) )
                throw std::runtime_error( "Choose a new scene profile output; existing evidence is preserved." );
            if( !std::filesystem::is_directory( output.parent_path() ) )
                throw std::runtime_error( "Scene profile output directory must already exist." );
            for( const auto &otherOutput : { evidencePath, capturePath, effectsCapture } )
                if( !otherOutput.empty() && std::filesystem::exists( std::filesystem::path( otherOutput ) ) )
                    throw std::runtime_error( "Choose new profile capture/evidence outputs; existing files are preserved." );
            for( const auto &other : { evidencePath, capturePath, replayPath, effectsCapture,
                    materialExportDirectory, std::string( pluginsConfig.empty() ? "wp_plugins_samples.cfg" : pluginsConfig.c_str() ) } )
                if( advanced::reviewPathsAlias( sceneProfilePath, other ) )
                    throw std::runtime_error( "Scene profile output must differ from every input and other output." );
            sceneProfile = std::make_unique<advanced::ProductionSceneProfile>( output.string(), sceneProfileMode );
            sceneProfile->configure( seed, int( quality ), width, height, captureView, orbit,
                foliageWorld, audioEnabled, effectsEnabled, benchmarkSeconds, replayPath,
                pluginsConfig.empty() ? "wp_plugins_samples.cfg" : pluginsConfig.c_str(), forcedLOD, forcedVehicleLOD );
        }
        if( !replayPath.empty() ) replayControls = advanced::loadReviewReplay( replayPath );
    }
    catch( const std::exception &e )
    {
        std::cerr << e.what() << '\n';
        return 2;
    }
    int exitCode = 1;

    auto unloadApplication = [&app, &sceneProfile]() {
        if( app )
        {
            if( sceneProfile ) sceneProfile->endMeasurement();
            if( app->getLoadingState() != LoadingState::Unloaded )
            {
                app->unload( nullptr );
            }

            app = nullptr;
            if( sceneProfile ) sceneProfile->checkpoint( "application_unloaded" );
        }

        core::IApplicationManager::setInstance( nullptr );
    };

    auto unloadTypeManager = [&typeManager, &ownsTypeManager, &sceneProfile]() {
        if( ownsTypeManager && typeManager )
        {
            typeManager->unload();
            delete typeManager;
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
            ownsTypeManager = false;
            if( sceneProfile ) sceneProfile->checkpoint( "type_manager_destroyed" );
        }
    };

    try
    {
        typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            typeManager->load();
            TypeManager::setInstance( typeManager );
            ownsTypeManager = true;
        }

        app = workphone::make_ptr<SampleVehicleAdvanced>();
        app->setSceneProfile( sceneProfile.get() );
        app->setActiveThreads( activeThreadCount );
        app->setSmokeTest( smokeTest );
        app->setTrackSmokeTest( trackSmokeTest );
        app->setCollisionSmokeTest( collisionSmokeTest );
        app->setGenerationOptions( seed, quality );
        app->setAudioOptions( audioEnabled, audioSmokeTest );
        app->setEffectsOptions( effectsEnabled, effectsSmokeTest, effectsCapture );
        app->setCapture( capturePath, captureView );
        app->setReviewOptions( hud, forcedLOD, forcedVehicleLOD, benchmarkSeconds, orbit, width, height );
        app->setEvidenceOptions( evidencePath, replayPath, std::move( replayControls ) );
        app->setFoliageWorldEnabled( foliageWorld );
        app->setCityOptions( openCity, cityBlocks, cityRoute, cityNaturalRoads );
        app->setRoadAuthoringShowcase( roadAuthoringShowcase );
        app->setCityTerrainRoads( cityTerrainRoads );
        app->setMaterialExportDirectory( materialExportDirectory );
        if( !pluginsConfig.empty() )
            app->setPluginsConfigFilePath( pluginsConfig );
        app->load( nullptr );
        if( app->getLoadingState() != LoadingState::Loaded )
            throw std::runtime_error( "SampleVehicleAdvanced failed to load." );
        app->run();
        exitCode = ( ( smokeTest || trackSmokeTest || collisionSmokeTest || audioSmokeTest || effectsSmokeTest ) && !app->smokeTestPassed() ) ||
                           ( ( !capturePath.empty() || benchmarkSeconds > 0 ) && !app->capturePassed() )
                       ? 1
                       : 0;
        if( sceneProfile && !sceneProfile->invariantsPassed() )
        {
            sceneProfileError = "Production scene profile did not complete its rendering/telemetry invariants.";
            std::cerr << sceneProfileError << '\n';
            exitCode = 1;
        }

        unloadApplication();
        unloadTypeManager();
    }
    catch( Exception &e )
    {
        sceneProfileError = e.what();
        unloadApplication();
        unloadTypeManager();
        if( sceneProfile ) std::cerr << sceneProfileError << '\n';
        else MessageBoxUtil::show( e.what() );
    }
    catch( std::exception &e )
    {
        sceneProfileError = e.what();
        unloadApplication();
        unloadTypeManager();
        if( sceneProfile ) std::cerr << sceneProfileError << '\n';
        else MessageBoxUtil::show( e.what() );
    }
    catch( ... )
    {
        sceneProfileError = "Unknown error";
        unloadApplication();
        unloadTypeManager();
        if( sceneProfile ) std::cerr << sceneProfileError << '\n';
        else MessageBoxUtil::show( "Unknown error" );
    }

    if( sceneProfile )
    {
        try { sceneProfile->write( exitCode, sceneProfileError ); }
        catch( const std::exception &e ) { std::cerr << e.what() << '\n'; exitCode = 1; }
    }

    return exitCode;
}
