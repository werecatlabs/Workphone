#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawLight.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRendererSoftware.hpp>
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawSceneNode.hpp>
#include <WPGraphics/ClawDynamicMesh.hpp>
#include <WPGraphics/ClawBillboard.hpp>
#include <WPGraphics/ClawBillboardSet.hpp>
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawSky.hpp>
#include <WPGraphics/ClawSkybox.hpp>
#include <WPGraphics/ClawSkyboxCube.hpp>
#include <WPGraphics/Particle/CParticleSystem.hpp>
#include "WPGraphics/Jobs/CameraVisibilitySet.hpp"
#include <WPGraphics/Jobs/SceneNodeCullJob.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_object.h"
#include "workphone_graphics_scenenode.h"

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawScene, GraphicsScene );

        /**
         * @brief Wraps the C graphics scene's root node without owning it.
         *
         * The C API creates and destroys the root wp_scenenode together with the scene.
         * This derived ClawSceneNode simply references that existing native node.
         */
        class ClawRootSceneNode : public ClawSceneNode
        {
        public:
            explicit ClawRootSceneNode( ClawScene *scene, wp_scenenode *root );

            ~ClawRootSceneNode() override;

            WP_CLASS_REGISTER_DECL;
        };

        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawRootSceneNode, ClawSceneNode );

        ClawRootSceneNode::ClawRootSceneNode( ClawScene *scene, wp_scenenode *root ) : ClawSceneNode()
        {
            m_creator = scene;
            m_node = root;
            m_sceneOwnsNode = true;
        }

        ClawRootSceneNode::~ClawRootSceneNode()
        {
            // Leave the root node alive: it is released by wp_graphics_scene_destroy.
            m_node = nullptr;
        }

        ClawScene::ClawScene() : ClawScene( wp_graphics_scene_create(), true )
        {
            createStateContext();
        }

        ClawScene::ClawScene( wp_graphics_scene *scene, bool ownsScene ) :
            m_scene( scene ),
            m_ownsScene( ownsScene )
        {
            setType( "ClawScene" );
            createStateContext();
        }

        ClawScene::~ClawScene()
        {
            unbindNativeRenderObjects();
            if( m_scene && m_ownsScene )
            {
                wp_graphics_scene_destroy( m_scene );
            }
            m_scene = nullptr;
        }

        void ClawScene::load( SmartPtr<ISharedObject> data )
        {
            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );
            GraphicsScene::load( data );

            auto sceneFactoryManager = workphone::make_ptr<FactoryManager>();
            sceneFactoryManager->load( nullptr );
            setFactoryManager( sceneFactoryManager );

            FactoryUtil::addFactory<ClawCamera>( sceneFactoryManager );
            FactoryUtil::addFactory<ClawLight>( sceneFactoryManager );
            FactoryUtil::addFactory<ClawMesh>( sceneFactoryManager );
            FactoryUtil::addFactory<ClawSceneNode>( sceneFactoryManager );
            FactoryUtil::addFactory<ClawDynamicMesh>( sceneFactoryManager );
            FactoryUtil::addFactory<ClawBillboard>( sceneFactoryManager );
            FactoryUtil::addFactory<ClawBillboardSet>( sceneFactoryManager );
            FactoryUtil::addFactory<ClawTerrain>( sceneFactoryManager );
            FactoryUtil::addFactory<ClawSky>( sceneFactoryManager );
            FactoryUtil::addFactory<ClawSkybox>( sceneFactoryManager );
            FactoryUtil::addFactory<ClawSkyboxCube>( sceneFactoryManager );
            FactoryUtil::addFactory<CParticleSystem>( sceneFactoryManager );
            FactoryUtil::addFactory<SceneNodeCullJob>( sceneFactoryManager );

            sceneFactoryManager->setPoolSizeByType<ClawCamera>( 8 );
            sceneFactoryManager->setPoolSizeByType<ClawLight>( 32 );
            sceneFactoryManager->setPoolSizeByType<ClawMesh>( 128 );
            sceneFactoryManager->setPoolSizeByType<ClawSceneNode>( 1024 );
            sceneFactoryManager->setPoolSizeByType<ClawDynamicMesh>( 32 );
            sceneFactoryManager->setPoolSizeByType<ClawBillboardSet>( 32 );
            sceneFactoryManager->setPoolSizeByType<ClawTerrain>( 4 );
            sceneFactoryManager->setPoolSizeByType<CParticleSystem>( 32 );

            // Wrap the C API root node so the C++ scene graph has a hierarchy root.
            if( m_scene )
            {
                if( auto rootNative = wp_graphics_scene_get_root_node( m_scene ) )
                {
                    auto rootNode = workphone::make_ptr<ClawRootSceneNode>( this, rootNative );
                    rootNode->setCreator( this );
                    rootNode->setName( sceneNodePrefix + "Root" );
                    m_rootSceneNode = rootNode;
                    m_sceneNodes.push_back( rootNode );

                    auto applicationManager = core::IApplicationManager::instancePtr();
                    if( auto graphicsSystem =
                            applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr )
                    {
                        graphicsSystem->loadObject( rootNode );
                    }
                }
            }

            setLoadingState( LoadingState::Loaded );
        }

        void ClawScene::unload( SmartPtr<ISharedObject> data )
        {
            if( getLoadingState() == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );
            GraphicsScene::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }

        wp_graphics_scene *ClawScene::getNativeScene() const
        {
            return m_scene;
        }

        wp_graphics_scene *ClawScene::releaseNativeScene()
        {
            unbindNativeRenderObjects();
            auto scene = m_scene;
            m_scene = nullptr;
            m_ownsScene = false;
            return scene;
        }

        void ClawScene::clear()
        {
            unbindNativeRenderObjects();
            m_lights.clear();

            // Release C++ wrappers while their scene-owned C nodes are still valid.
            GraphicsScene::clear();
            if( m_scene )
            {
                wp_graphics_scene_clear( m_scene );
            }
        }

        SmartPtr<ISharedObject> ClawScene::addGraphicsObjectByTypeId( hash_type id )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "ClawScene::addGraphicsObjectByTypeId: application manager not found." );
                return nullptr;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( !graphicsSystem )
            {
                WP_LOG_ERROR( "ClawScene::addGraphicsObjectByTypeId: graphics system not found." );
                return nullptr;
            }

            auto factoryManager = getFactoryManagerPtr();
            if( !factoryManager )
            {
                WP_LOG_ERROR( "ClawScene::addGraphicsObjectByTypeId: factory manager not found." );
                return nullptr;
            }

            if( id == IGraphicsCamera::typeInfo() )
            {
                auto camera = factoryManager->make_ptr<ClawCamera>();
                camera->setCreator( this );
                graphicsSystem->loadObject( camera, true );
                m_cameras.push_back( camera );
                return camera;
            }
            if( id == IGraphicsLight::typeInfo() )
            {
                auto light = factoryManager->make_ptr<ClawLight>();
                light->setCreator( this );
                graphicsSystem->loadObject( light, true );
                m_graphicsObjects.push_back( light );
                m_lights.push_back( light );
                return light;
            }
            if( id == IGraphicsMesh::typeInfo() )
            {
                auto mesh = factoryManager->make_ptr<ClawMesh>();
                mesh->setCreator( this );
                if( m_scene )
                {
                    auto renderObject = wp_graphics_scene_create_object( m_scene );
                    if( renderObject )
                    {
                        mesh->bindNativeRenderObject( renderObject );
                    }
                    else
                    {
                        WP_LOG_ERROR( "ClawScene: failed to allocate native mesh render object." );
                    }
                }
                graphicsSystem->loadObject( mesh, true );
                m_graphicsObjects.push_back( mesh );
                return mesh;
            }
            if( id == IDynamicMesh::typeInfo() )
            {
                auto mesh = factoryManager->make_ptr<ClawDynamicMesh>();
                mesh->setCreator( this );
                graphicsSystem->loadObject( mesh, true );
                m_graphicsObjects.push_back( mesh );
                return mesh;
            }
            if( id == IBillboardSet::typeInfo() )
            {
                auto billboardSet = factoryManager->make_ptr<ClawBillboardSet>();
                billboardSet->setCreator( this );
                graphicsSystem->loadObject( billboardSet, true );
                m_graphicsObjects.push_back( billboardSet );
                return billboardSet;
            }
            if( id == IBillboard::typeInfo() )
            {
                auto billboard = factoryManager->make_ptr<ClawBillboard>();
                graphicsSystem->loadObject( billboard, true );
                return billboard;
            }
            if( id == IParticleSystem::typeInfo() )
            {
                auto particleSystem = factoryManager->make_ptr<CParticleSystem>();
                particleSystem->setCreator( this );
                graphicsSystem->loadObject( particleSystem, true );
                m_particleSystems.push_back( particleSystem );
                return particleSystem;
            }
            if( id == IGraphicsTerrain::typeInfo() )
            {
                auto terrain = factoryManager->make_ptr<ClawTerrain>();
                terrain->setSceneManager( this );
                graphicsSystem->loadObject( terrain, true );
                m_terrains.push_back( terrain );
                return terrain;
            }
            if( id == ISkybox::typeInfo() )
            {
                auto sky = factoryManager->make_ptr<ClawSkybox>();
                sky->setScene( this );
                graphicsSystem->loadObject( sky, true );
                m_skies.push_back( sky );
                return sky;
            }
            if( id == ISkyboxCube::typeInfo() )
            {
                auto sky = factoryManager->make_ptr<ClawSkyboxCube>();
                sky->setScene( this );
                graphicsSystem->loadObject( sky, true );
                m_skies.push_back( sky );
                return sky;
            }
            if( id == ISky::typeInfo() )
            {
                auto sky = factoryManager->make_ptr<ClawSky>();
                sky->setScene( this );
                graphicsSystem->loadObject( sky, true );
                m_skies.push_back( sky );
                return sky;
            }

            auto object = factoryManager->make_object<ISharedObject>( static_cast<u32>( id ) );
            if( !object )
            {
                WP_LOG_ERROR(
                    "ClawScene::addGraphicsObjectByTypeId: could not create graphics object." );
                return nullptr;
            }

            if( object->isDerived<IGraphicsObject>() )
            {
                auto graphicsObject = workphone::static_pointer_cast<IGraphicsObject>( object );
                graphicsObject->setCreator( this );
                graphicsSystem->loadObject( graphicsObject, true );

                if( auto light = dynamic_pointer_cast<IGraphicsLight>( graphicsObject ) )
                    m_lights.push_back( light );

                if( graphicsObject->isDerived<IParticleSystem>() )
                {
                    m_particleSystems.push_back( graphicsObject );
                }
                else
                {
                    m_graphicsObjects.push_back( graphicsObject );
                }

                return graphicsObject;
            }
            if( object->isDerived<ISky>() )
            {
                auto sky = workphone::static_pointer_cast<ISky>( object );
                graphicsSystem->loadObject( sky, true );
                m_skies.push_back( sky );
                return sky;
            }

            graphicsSystem->loadObject( object, true );
            return object;
        }

        bool ClawScene::removeGraphicsObject( SmartPtr<ISharedObject> object )
        {
            if( !GraphicsScene::removeGraphicsObject( object ) ) return false;
            if( auto light = dynamic_pointer_cast<IGraphicsLight>( object ) )
                m_lights.erase( std::remove( m_lights.begin(), m_lights.end(), light ), m_lights.end() );
            return true;
        }

        SmartPtr<IGraphicsSceneNode> ClawScene::addSceneNode( const String &name )
        {
            auto factoryManager = getFactoryManagerPtr();
            if( !factoryManager )
            {
                WP_LOG_ERROR( "ClawScene::addSceneNode: factory manager not found." );
                return nullptr;
            }

            auto sceneNode = factoryManager->make_ptr<ClawSceneNode>( this );
            if( !sceneNode )
            {
                WP_LOG_ERROR( "ClawScene::addSceneNode: could not create scene node." );
                return nullptr;
            }

            sceneNode->setCreator( this );
            sceneNode->setName( name );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( auto graphicsSystem =
                    applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr )
            {
                graphicsSystem->loadObject( sceneNode );
            }

            m_sceneNodes.push_back( sceneNode );

            return sceneNode;
        }

        SmartPtr<IGraphicsSceneNode> ClawScene::addSceneNode()
        {
            auto nodeName = sceneNodePrefix + StringUtil::toString( nodeCounter++ );
            return addSceneNode( nodeName );
        }

        void ClawScene::setAmbientLight( const ColourF &colour )
        {
            if( m_scene )
            {
                wp_graphics_scene_set_ambient_light( m_scene, colour.r, colour.g, colour.b );
            }

            GraphicsScene::setAmbientLight( colour );
        }

        ColourF ClawScene::getAmbientLight() const
        {
            if( m_scene )
            {
                wp_f32 r = 0.0f;
                wp_f32 g = 0.0f;
                wp_f32 b = 0.0f;
                wp_graphics_scene_get_ambient_light( m_scene, &r, &g, &b );
                return ColourF( r, g, b );
            }

            return GraphicsScene::getAmbientLight();
        }

        ColourF ClawScene::getUpperHemisphere() const
        {
            if( m_scene )
            {
                wp_colour4f colour = wp_graphics_scene_get_upper_hemisphere( m_scene );
                return ColourF( colour.r, colour.g, colour.b, colour.a );
            }

            return GraphicsScene::getUpperHemisphere();
        }

        void ClawScene::setUpperHemisphere( const ColourF &upperHemisphere )
        {
            if( m_scene )
            {
                wp_colour4f colour = { upperHemisphere.r, upperHemisphere.g, upperHemisphere.b,
                                       upperHemisphere.a };
                wp_graphics_scene_set_upper_hemisphere( m_scene, colour );
            }

            GraphicsScene::setUpperHemisphere( upperHemisphere );
        }

        ColourF ClawScene::getLowerHemisphere() const
        {
            if( m_scene )
            {
                wp_colour4f colour = wp_graphics_scene_get_lower_hemisphere( m_scene );
                return ColourF( colour.r, colour.g, colour.b, colour.a );
            }

            return GraphicsScene::getLowerHemisphere();
        }

        void ClawScene::setLowerHemisphere( const ColourF &lowerHemisphere )
        {
            if( m_scene )
            {
                wp_colour4f colour = { lowerHemisphere.r, lowerHemisphere.g, lowerHemisphere.b,
                                       lowerHemisphere.a };
                wp_graphics_scene_set_lower_hemisphere( m_scene, colour );
            }

            GraphicsScene::setLowerHemisphere( lowerHemisphere );
        }

        Vector3<real_Num> ClawScene::getHemisphereDir() const
        {
            if( m_scene )
            {
                wp_vec3f dir = wp_graphics_scene_get_hemisphere_dir( m_scene );
                return Vector3<real_Num>( dir.x, dir.y, dir.z );
            }

            return GraphicsScene::getHemisphereDir();
        }

        void ClawScene::setHemisphereDir( const Vector3<real_Num> &hemisphereDir )
        {
            if( m_scene )
            {
                wp_vec3f dir = { ( hemisphereDir.X() ), ( hemisphereDir.Y() ), ( hemisphereDir.Z() ) };
                wp_graphics_scene_set_hemisphere_dir( m_scene, dir );
            }

            GraphicsScene::setHemisphereDir( hemisphereDir );
        }

        f32 ClawScene::getEnvmapScale() const
        {
            if( m_scene )
            {
                return wp_graphics_scene_get_envmap_scale( m_scene );
            }

            return GraphicsScene::getEnvmapScale();
        }

        void ClawScene::setEnvmapScale( f32 envmapScale )
        {
            if( m_scene )
            {
                wp_graphics_scene_set_envmap_scale( m_scene, envmapScale );
            }

            GraphicsScene::setEnvmapScale( envmapScale );
        }

        void ClawScene::setFog( u32 fogMode, const ColourF &colour, f32 expDensity, f32 linearStart,
                                f32 linearEnd )
        {
            if( m_scene )
            {
                wp_colour4f fogColour = { colour.r, colour.g, colour.b, colour.a };
                wp_graphics_scene_set_fog( m_scene, static_cast<wp_fog_mode>( fogMode ), fogColour,
                                           expDensity, linearStart, linearEnd );
            }

            GraphicsScene::setFog( fogMode, colour, expDensity, linearStart, linearEnd );
        }

        void ClawScene::setSkyBox( bool enable, SmartPtr<IMaterial> material, f32 distance,
                                   bool /*drawFirst*/ )
        {
            if( m_scene )
            {
                wp_graphics_scene_set_skybox( m_scene, enable ? 1 : 0, distance );
            }

            GraphicsScene::setSkyBox( enable, material, distance );
        }

        void ClawScene::setSkyBox( bool enable, SmartPtr<ITexture> texture, f32 distance,
                                   bool /*drawFirst*/ )
        {
            if( m_scene )
            {
                wp_graphics_scene_set_skybox( m_scene, enable ? 1 : 0, distance );
            }

            GraphicsScene::setSkyBox( enable, texture, distance );
        }

        bool ClawScene::getEnableSkybox() const
        {
            if( m_scene )
            {
                return wp_graphics_scene_get_skybox_enabled( m_scene ) != 0;
            }

            return GraphicsScene::getEnableSkybox();
        }

        bool ClawScene::getEnableShadows() const
        {
            if( m_scene )
            {
                return wp_graphics_scene_get_shadows_enabled( m_scene ) != 0;
            }

            return GraphicsScene::getEnableShadows();
        }

        void ClawScene::setEnableShadows( bool enableShadows, bool depthShadows )
        {
            if( m_scene )
            {
                wp_graphics_scene_set_shadows( m_scene, enableShadows ? 1 : 0, depthShadows ? 1 : 0 );
            }

            GraphicsScene::setEnableShadows( enableShadows, depthShadows );
        }

        void ClawScene::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                wp_graphics_scene_get_native( m_scene, ppObject );
                if( !*ppObject )
                {
                    *ppObject = m_scene;
                }
            }
        }

        void ClawScene::render( void *rendererPtr )
        {
            if( !m_scene || !rendererPtr )
            {
                return;
            }

            auto rawRenderer = static_cast<IRenderer *>( rendererPtr );
            auto dx11Renderer = dynamic_cast<ClawRendererDX11 *>( rawRenderer );
            wp_renderer *nativeRenderer = nullptr;
            if( dx11Renderer )
            {
                nativeRenderer = dx11Renderer->getNativeRenderer();
            }
            else if( auto softwareRenderer = dynamic_cast<ClawRendererSoftware *>( rawRenderer ) )
            {
                nativeRenderer = softwareRenderer->getNativeRenderer();
            }

            if( nativeRenderer )
            {
                auto camera = dynamic_pointer_cast<ClawCamera>( rawRenderer->getCamera() );
                wp_graphics_scene_set_active_camera( m_scene,
                                                     camera ? camera->getNativeCamera() : nullptr );

                // Skies and terrains are C++ renderer objects and are not members of the native C
                // scene's object list. Submit them explicitly before the native mesh scene pass.
                if( dx11Renderer )
                {
                    const auto objects = m_lights.snapshot();
                    // The forward shader supports one directional light. Configure it once
                    // for both terrain and meshes, rather than using the renderer's preview sun.
                    Vector3F lightDirection( 0.0f, -1.0f, 0.0f );
                    ColourF lightColour = ColourF::White;
                    f32 lightIntensity = 0.0f;
                    for( const auto &object : objects )
                    {
                        if( auto light = dynamic_pointer_cast<ClawLight>( object ) )
                        {
                            if( light->isVisible() && light->getType() == LightTypes::LT_DIRECTIONAL )
                            {
                                const auto direction = light->getDerivedDirection();
                                lightDirection = Vector3F( direction.X(), direction.Y(), direction.Z() );
                                lightColour = light->getDiffuseColour();
                                lightIntensity = light->getPowerScale();
                                break;
                            }
                        }
                    }
                    dx11Renderer->setSceneLighting( getAmbientLight(), lightDirection, lightColour,
                                                    lightIntensity );
                    for( auto &sky : m_skies.snapshot() )
                    {
                        dx11Renderer->renderSky( sky );
                    }

                    for( auto &terrainObject : m_terrains.snapshot() )
                    {
                        if( auto terrain = dynamic_pointer_cast<ClawTerrain>( terrainObject ) )
                        {
                            dx11Renderer->renderTerrain( terrain );
                        }
                    }
                    // Use the material-aware C++ draw path for native mesh objects while
                    // retaining the native scene's visibility filtering and queue ordering.
                    wp_graphics_scene_render_with_submit(
                        m_scene, nativeRenderer,
                        []( wp_graphics_object *object, wp_renderer *, void *data ) -> wp_s32 {
                            auto renderer = static_cast<ClawRendererDX11 *>( data );
                            auto mesh = static_cast<ClawMesh *>( wp_graphics_object_get_submit_data( object ) );
                            if( !mesh ) return 0;
                            wp_mat4f world;
                            wp_scenenode_get_world_matrix( wp_graphics_object_get_owner( object ),
                                                           &world );
                            renderer->renderMesh( mesh, Matrix4F( world.m[0] ) );
                            return 1;
                        },
                        dx11Renderer );
                }
                else
                {
                    wp_graphics_scene_render( m_scene, nativeRenderer );
                }
            }
        }

        void ClawScene::unbindNativeRenderObjects()
        {
            for( auto &object : m_graphicsObjects.snapshot() )
            {
                if( auto mesh = dynamic_pointer_cast<ClawMesh>( object ) )
                {
                    mesh->bindNativeRenderObject( nullptr );
                }
            }
        }

        SmartPtr<SceneNodeCullJob> ClawScene::dispatchCullingJob( SmartPtr<IGraphicsCamera> camera )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "ClawScene::dispatchCullingJob: application manager not found." );
                return nullptr;
            }

            auto jobQueue = applicationManager->getJobQueuePtr();
            if( !jobQueue )
            {
                WP_LOG_ERROR( "ClawScene::dispatchCullingJob: job queue not found." );
                return nullptr;
            }

            auto factoryManager = applicationManager->getFactoryManager();
            if( !factoryManager )
            {
                WP_LOG_ERROR( "ClawScene::dispatchCullingJob: factory manager not found." );
                return nullptr;
            }

            // Build the cull job, configure it with this scene's root node as
            // the traversal root and the supplied camera's frustum as the cull
            // volume, and submit it to the job system. The smart pointer is
            // cached on the scene so that the subsequent render() pass (and any
            // other lazy consumer that runs later on the queue) can read the
            // per-node cull flag via SceneNodeCullJob::isCulled.
            auto cullJob = factoryManager->make_ptr<SceneNodeCullJob>();
            if( !cullJob )
            {
                WP_LOG_ERROR( "ClawScene::dispatchCullingJob: failed to create cull job." );
                return nullptr;
            }

            cullJob->setScene( this );
            cullJob->setFrustum( camera );

            // Bind the camera so the job also populates a per-camera
            // visibility set. setCamera() ensures the set exists and is
            // cached as the active set inside the job, so render() and other
            // lazy consumers can read it via getActiveVisibilitySet().
            cullJob->setCamera( camera );

            jobQueue->addJob( cullJob );

            m_cullJob = cullJob;
            return cullJob;
        }

        SmartPtr<CameraVisibilitySet> ClawScene::getVisibilitySet() const
        {
            // The per-camera visibility set is owned by the cull job. When
            // no cull job has been dispatched (yet) or no camera was bound to
            // the job, simply forward an empty pointer so callers can use a
            // single uniform code path.
            return m_cullJob ? m_cullJob->getActiveVisibilitySet() : nullptr;
        }

        bool ClawScene::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            if( GraphicsScene::handleStateMessage( message ) )
            {
                return true;
            }

            return false;
        }

        bool ClawScene::handleStateChanged( SmartPtr<IState> &state )
        {
            if( GraphicsScene::handleStateChanged( state ) )
            {
                return true;
            }

            if( state->getOwnerPtr() == this )
            {
                try
                {
                    if( auto stateData = state->getData() )
                    {
                        if( stateData->isDerived<GraphicsSceneState>() )
                        {
                            ScopedLock lock( this );

                            auto sceneManagerState = SafeReadPtr<GraphicsSceneState>( stateData );

                            return true;
                        }
                        else if( stateData->isDerived<AmbientLightStateData>() )
                        {
                            ScopedLock lock( this );

                            auto ambientLightState = SafeReadPtr<AmbientLightStateData>( stateData );
                            auto &colour = ambientLightState->ambientColour;
                            wp_graphics_scene_set_ambient_light( m_scene, colour.r, colour.g, colour.b );
                            return true;
                        }
                    }
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }

            return false;
        }

        void ClawScene::createStateContext()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto stateContext = stateManager->addStateContext();
            WP_ASSERT( stateContext );

            stateContext->setOwner( this );
            setStateContext( stateContext );
            stateContext->setTaskId( TaskId::Render );

            auto sceneNodeContext = stateManager->addStateContext();
            WP_ASSERT( sceneNodeContext );

            sceneNodeContext->setOwner( this );
            setSceneNodeContext( sceneNodeContext );
            sceneNodeContext->setTaskId( TaskId::Render );

            auto stateListener = factoryManager->make_ptr<StateListener>();
            stateListener->setOwner( this );
            setStateListener( stateListener );

            Array<u32> graphicsObjectContextIds;
            graphicsObjectContextIds.reserve( 12 );

            graphicsObjectContextIds.push_back( ClawCamera::typeInfo() );
            graphicsObjectContextIds.push_back( ClawMesh::typeInfo() );
            graphicsObjectContextIds.push_back( ClawLight::typeInfo() );
            // graphicsObjectContextIds.push_back( ClawParticleSystem::typeInfo() );

            for( auto &id : graphicsObjectContextIds )
            {
                auto graphicsObjectContext = stateManager->addStateContext();
                WP_ASSERT( graphicsObjectContext );

                graphicsObjectContext->setOwner( this );
                setGraphicsObjectContext( id, graphicsObjectContext );
                graphicsObjectContext->setTaskId( TaskId::Render );

                graphicsObjectContext->addStateListener( stateListener );
            }

            stateContext->addStateListener( stateListener );
            sceneNodeContext->addStateListener( stateListener );

            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto graphicsSceneState = factoryManager->make_ptr<GraphicsSceneState>();
            state->setData( graphicsSceneState );

            auto ambientLightState = factoryManager->make_ptr<State>();
            ambientLightState->setId( getId() );
            ambientLightState->setOwner( this );
            stateContext->addState( ambientLightState );

            auto ambientLightData = factoryManager->make_ptr<AmbientLightStateData>();
            ambientLightState->setData( ambientLightData );
        }

    }  // namespace render
}  // namespace workphone
