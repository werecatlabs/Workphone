#include "WPGraphics/WPClawHammerPCH.hpp"
#include <stdexcept>
#include "workphone_graphics_cull_job.h"
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawLight.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawFoliageBatch.hpp>
#include <WPGraphics/ClawFoliageWorld.hpp>
#include <chrono>
#include <map>
#include <set>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRendererSoftware.hpp>
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawSceneNode.hpp>
#include <WPGraphics/ClawDynamicMesh.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawDepthReadback.hpp>
#include <Workphone/Graphics/HierarchicalDepthOcclusion.hpp>
#include <Workphone/Graphics/Material.hpp>
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
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include "workphone_graphics_object.h"
#include "workphone_graphics_scenenode.h"

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawScene, GraphicsScene );

        struct ClawScene::FoliageWorldState
        {
            struct Attached
            {
                SmartPtr<ClawTerrain> terrain;
                std::shared_ptr<ClawFoliageWorld> world;
            };

            std::map<ClawTerrain *, Attached> terrains;
            Array<std::shared_ptr<ClawFoliageWorld>> standalone;
            Array<ClawFoliageInteraction> interactions;
            std::chrono::steady_clock::time_point epoch = std::chrono::steady_clock::now();
            u64 frame = 0;
            f32 seconds = 0;
            String error;
        };

        struct ClawScene::OcclusionHistory
        {
            ClawDepthReadback readback;
            ClawDepthReadback::Surface surface{};
            HierarchicalDepthOcclusion hierarchy;
            HierarchicalDepthOcclusion::Matrix matrix{};
            std::shared_ptr<NativeRenderJobs> capturedJobs, renderedJobs;

            struct MaterialSnapshot
            {
                SmartPtr<ClawMesh> mesh;
                s32 section;
                SmartPtr<Material> material;
                MaterialRenderState::Ptr state;
            };

            std::vector<MaterialSnapshot> materials;
            SmartPtr<IGraphicsCamera> camera;
            u64 capturedFrame = 0;
            u32 culled = 0;
        };

        namespace
        {
            bool projectOcclusionEnabled()
            {
                auto app = core::IApplicationManager::instancePtr();
                auto settings = app ? app->getPlayerSettings() : nullptr;
                return settings && settings->getPropertyAsBool( "performance.occlusionCulling", false );
            }

            HierarchicalDepthOcclusion::Matrix occlusionMatrix( const wp_mat4f &matrix )
            {
                HierarchicalDepthOcclusion::Matrix result;
                std::copy( &matrix.m[0][0], &matrix.m[0][0] + 16, result.begin() );
                return result;
            }
        }  // namespace

        bool ClawScene::addFoliageWorld( std::shared_ptr<ClawFoliageWorld> world, String &error )
        {
            if( !world )
            {
                error = "A foliage world is required.";
                return false;
            }
            if( !m_foliageWorlds )
            {
                m_foliageWorlds = std::make_unique<FoliageWorldState>();
            }
            auto &worlds = m_foliageWorlds->standalone;
            if( worlds.size() >= 64 || std::find( worlds.begin(), worlds.end(), world ) != worlds.end() )
            {
                error = "Foliage world already attached or scene world limit exceeded.";
                return false;
            }
            worlds.push_back( std::move( world ) );
            error.clear();
            return true;
        }

        bool ClawScene::removeFoliageWorld( const std::shared_ptr<ClawFoliageWorld> &world )
        {
            if( !m_foliageWorlds )
            {
                return false;
            }
            auto &worlds = m_foliageWorlds->standalone;
            auto found = std::find( worlds.begin(), worlds.end(), world );
            if( found == worlds.end() )
            {
                return false;
            }
            worlds.erase( found );
            return true;
        }

        void ClawScene::clearFoliageWorlds()
        {
            m_foliageWorlds.reset();
        }

        bool ClawScene::setFoliageInteractions( const Array<ClawFoliageInteraction> &values,
                                                String &error )
        {
            if( values.size() > 16 )
            {
                error = "Foliage supports at most sixteen interaction spheres.";
                return false;
            }
            for( const auto &v : values )
            {
                if( !std::isfinite( v.position.x ) || !std::isfinite( v.position.y ) ||
                    !std::isfinite( v.position.z ) || !std::isfinite( v.radius ) || v.radius <= 0 ||
                    !std::isfinite( v.strength ) || v.strength < 0 || v.strength > 1 )
                {
                    error = "Invalid foliage interaction.";
                    return false;
                }
            }
            if( !m_foliageWorlds )
            {
                m_foliageWorlds = std::make_unique<FoliageWorldState>();
            }
            m_foliageWorlds->interactions = values;
            error.clear();
            return true;
        }

        String ClawScene::getFoliageError() const
        {
            return m_foliageWorlds ? m_foliageWorlds->error : String{};
        }

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
        }

        ClawScene::ClawScene( wp_graphics_scene *scene, bool ownsScene ) :
            m_scene( scene ),
            m_ownsScene( ownsScene )
        {
            setType( "ClawScene" );
            createStateContext();
            setEnableShadows( true );
        }

        ClawScene::~ClawScene()
        {
            m_occlusionHistory.reset();
            discardRenderJobs();
            m_materialRecordWorkspace.reset();
            clearFoliageBatches();
            clearFoliageWorlds();
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
            discardRenderJobs();
            m_materialRecordWorkspace.reset();
            clearFoliageBatches();
            clearFoliageWorlds();
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
            discardRenderJobs();
            m_materialRecordWorkspace.reset();
            clearFoliageBatches();
            clearFoliageWorlds();
            unbindNativeRenderObjects();
            auto scene = m_scene;
            m_scene = nullptr;
            m_ownsScene = false;
            return scene;
        }

        void ClawScene::clear()
        {
            discardRenderJobs();
            m_materialRecordWorkspace.reset();
            clearFoliageBatches();
            clearFoliageWorlds();
            unbindNativeRenderObjects();
            {
                std::lock_guard<std::mutex> lock( m_primaryLightMutex );
                m_primaryDirectionalLight = nullptr;
                m_lights.clear();
            }

            // Release C++ wrappers while their scene-owned C nodes are still valid.
            GraphicsScene::clear();
            // Native clear destroys every non-root node. Detach cameras/lights
            // and invalidate the wrappers before releasing that native storage.
            auto root = getRootSceneNode();
            for( auto &node : m_sceneNodes.snapshot() )
            {
                if( node && node != root )
                {
                    node->unload( nullptr );
                }
            }
            m_sceneNodes.clear();
            m_registeredSceneNodes.clear();
            if( root )
            {
                root->removeChildren();
            }
            for( auto &camera : m_cameras.snapshot() )
            {
                if( camera )
                {
                    camera->detachFromParent( camera->getOwner() );
                }
            }
            m_cameras.clear();
            setDefaultCamera( nullptr );
            if( m_scene )
            {
                wp_graphics_scene_clear( m_scene );
            }
        }

        bool ClawScene::replaceFoliageBatch( std::shared_ptr<const ClawFoliageBatch> previous,
                                             std::shared_ptr<const ClawFoliageBatch> replacement,
                                             String &error )
        {
            error.clear();
            try
            {
                std::lock_guard<std::mutex> lock( m_foliageMutex );
                const auto retained = std::atomic_load( &m_foliageBatches );
                auto candidate =
                    std::make_shared<FoliageBatches>( retained ? *retained : FoliageBatches{} );
                if( previous )
                {
                    const auto found = std::find( candidate->begin(), candidate->end(), previous );
                    if( found == candidate->end() )
                    {
                        error = "Foliage batch was removed or replaced before this update.";
                        return false;
                    }
                    candidate->erase( found );
                }
                if( replacement )
                {
                    if( std::find( candidate->begin(), candidate->end(), replacement ) !=
                        candidate->end() )
                    {
                        error = "Foliage batch is already attached.";
                        return false;
                    }
                    candidate->push_back( replacement );
                }
                u64 count = 0;
                for( const auto &batch : *candidate )
                {
                    count += batch->instanceCount();
                }
                if( candidate->size() > 1024 || count > 1048576 )
                {
                    error = "Scene foliage exceeds 1024 batches or 1048576 instances.";
                    return false;
                }
                std::shared_ptr<const FoliageBatches> published = candidate;
                std::atomic_store( &m_foliageBatches, published );
                return true;
            }
            catch( const std::exception &exception )
            {
                error = exception.what();
                return false;
            }
        }

        std::shared_ptr<const ClawScene::FoliageBatches> ClawScene::getFoliageBatches() const
        {
            return std::atomic_load( &m_foliageBatches );
        }

        void ClawScene::clearFoliageBatches()
        {
            std::shared_ptr<const FoliageBatches> retired;
            {
                std::lock_guard<std::mutex> lock( m_foliageMutex );
                retired =
                    std::atomic_exchange( &m_foliageBatches, std::shared_ptr<const FoliageBatches>{} );
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

            // update() holds this same recursive lock through native capture and rendering.
            // Protect creation too: loadObject's queue lock is acquired only after the
            // native object array has already been grown and the mesh has been bound.
            ScopedLock graphicsLock( this );

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
                // Native lights contain CPU scene data only. Initialize before returning:
                // configuring an unloaded GraphicsLight otherwise silently loses settings.
                light->load( nullptr );
                m_graphicsObjects.push_back( light );
                {
                    std::lock_guard<std::mutex> lock( m_primaryLightMutex );
                    m_lights.push_back( light );
                }
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
                if( auto nativeLight = dynamic_pointer_cast<ClawLight>( graphicsObject ) )
                {
                    nativeLight->load( nullptr );
                }
                else
                {
                    graphicsSystem->loadObject( graphicsObject, true );
                }

                if( auto light = dynamic_pointer_cast<IGraphicsLight>( graphicsObject ) )
                {
                    std::lock_guard<std::mutex> lock( m_primaryLightMutex );
                    m_lights.push_back( light );
                }

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

        bool ClawScene::setPrimaryDirectionalLight( SmartPtr<IGraphicsLight> light )
        {
            std::lock_guard<std::mutex> lock( m_primaryLightMutex );
            if( !light )
            {
                m_primaryDirectionalLight = nullptr;
                return true;
            }
            const auto lights = m_lights.snapshot();
            if( !dynamic_pointer_cast<ClawLight>( light ) ||
                light->getType() != LightTypes::LT_DIRECTIONAL ||
                std::find( lights.begin(), lights.end(), light ) == lights.end() )
            {
                return false;
            }
            m_primaryDirectionalLight = light;
            return true;
        }

        SmartPtr<IGraphicsLight> ClawScene::getPrimaryDirectionalLight() const
        {
            std::lock_guard<std::mutex> lock( m_primaryLightMutex );
            const auto lights = m_lights.snapshot();
            const auto preferred = m_primaryDirectionalLight.lock();
            const auto visibleDirectional = []( const SmartPtr<IGraphicsLight> &light ) {
                return light && dynamic_pointer_cast<ClawLight>( light ) && light->isVisible() &&
                       light->getType() == LightTypes::LT_DIRECTIONAL;
            };
            if( visibleDirectional( preferred ) &&
                std::find( lights.begin(), lights.end(), preferred ) != lights.end() )
            {
                return preferred;
            }
            for( const auto &light : lights )
            {
                if( visibleDirectional( light ) )
                {
                    return light;
                }
            }
            return nullptr;
        }

        bool ClawScene::removeGraphicsObject( SmartPtr<ISharedObject> object )
        {
            if( !GraphicsScene::removeGraphicsObject( object ) )
            {
                return false;
            }
            if( auto light = dynamic_pointer_cast<IGraphicsLight>( object ) )
            {
                std::lock_guard<std::mutex> lock( m_primaryLightMutex );
                if( m_primaryDirectionalLight.lock() == light )
                {
                    m_primaryDirectionalLight = nullptr;
                }
                m_lights.erase( std::remove( m_lights.begin(), m_lights.end(), light ), m_lights.end() );
            }
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

        struct ClawScene::NativeRenderJobs
        {
            wp_graphics_cull_result *main = nullptr;
            wp_graphics_cull_result *shadow = nullptr;
            SmartPtr<IGraphicsCamera> camera;
            PreparationTiming timing;
            bool profiled = false;
            std::unique_ptr<ClawMaterialDrawRecords> materials;

            bool reusedMaterialStorage = false;
            u64 materialStorageAllocations = 0;
            ClawDrawSortStrategy sortStrategy = ClawDrawSortStrategy::QueueOrder;
            double sortBucketSize = 10.0;

            void captureMaterials( ClawRendererDX11 &renderer,
                                   std::unique_ptr<ClawMaterialDrawRecords> &workspace )
            {
                if( materials )
                {
                    return;
                }
                reusedMaterialStorage = workspace != nullptr;
                const auto previousAllocations = workspace ? workspace->getStorageAllocationCount() : 0;
                auto publication =
                    workspace ? std::move( workspace ) : std::make_unique<ClawMaterialDrawRecords>();
                // The shadow view includes offscreen casters; the main view can
                // also include objects that do not cast. Capture their union.
                for( auto view : { main, shadow } )
                {
                    for( s32 index = 0; index < wp_graphics_cull_result_get_visible_count( view );
                         ++index )
                    {
                        if( auto object = wp_graphics_cull_result_get_object( view, index ) )
                        {
                            publication->capture( static_cast<ClawMesh *>(
                                wp_graphics_object_get_submit_data( object ) ) );
                        }
                    }
                }
                renderer.prepareMaterialRecords( *publication );
                if( !publication->sortView( main, sortStrategy, sortBucketSize ) )
                {
                    throw std::runtime_error( "Could not order captured material draws." );
                }
                materialStorageAllocations =
                    publication->getStorageAllocationCount() - previousAllocations;
                materials = std::move( publication );
            }

            ~NativeRenderJobs()
            {
                wp_graphics_cull_result_destroy( main );
                wp_graphics_cull_result_destroy( shadow );
            }
        };

        u32 ClawScene::getOcclusionCulledObjects() const
        {
            return m_occlusionHistory ? m_occlusionHistory->culled : 0;
        }

        void ClawScene::captureOcclusionDepth( wp_renderer *renderer, SmartPtr<IGraphicsCamera> camera )
        {
            if( !projectOcclusionEnabled() )
            {
                m_occlusionHistory.reset();
                return;
            }
            if( !m_occlusionHistory || !m_occlusionHistory->renderedJobs )
            {
                return;
            }
            auto &history = *m_occlusionHistory;
            const auto jobs = history.renderedJobs;
            history.hierarchy.clear();
            history.capturedJobs.reset();
            history.materials.clear();
            if( !camera || jobs->camera != camera ||
                wp_graphics_cull_result_validate(
                    jobs->main, m_scene, wp_graphics_cull_result_get_scene_generation( jobs->main ) ) !=
                    WP_GRAPHICS_CULL_OK )
            {
                return;
            }
            try
            {
                // Terrain/foliage/particle callbacks can deform geometry without
                // native candidate revisions. Until those producers publish depth
                // revisions, this frame conservatively supplies no occlusion.
                const auto foliage = getFoliageBatches();
                if( ( m_foliageWorlds &&
                      ( !m_foliageWorlds->terrains.empty() || !m_foliageWorlds->standalone.empty() ) ) ||
                    ( foliage && !foliage->empty() ) )
                {
                    return;
                }
                size_t nativeMeshes = 0;
                for( const auto &object : m_graphicsObjects.snapshot() )
                {
                    if( !object || !object->isVisible() )
                    {
                        continue;
                    }
                    if( dynamic_cast<ClawTerrain *>( object.get() ) ||
                        dynamic_cast<CParticleSystem *>( object.get() ) ||
                        dynamic_cast<ClawBillboard *>( object.get() ) ||
                        dynamic_cast<ClawBillboardSet *>( object.get() ) )
                    {
                        return;
                    }
                    auto mesh = dynamic_pointer_cast<ClawMesh>( object );
                    if( !mesh || !mesh->getNativeRenderObject() )
                    {
                        continue;
                    }
                    ++nativeMeshes;
                    const auto native = mesh->getNativeMesh();
                    const auto sections =
                        native ? std::max<s32>( 1, wp_graphics_mesh_get_submesh_count( native ) ) : 1;
                    for( s32 section = 0; section < sections; ++section )
                    {
                        auto source = mesh->getMaterial( section );
                        if( !source )
                        {
                            source = mesh->getMaterial();
                        }
                        auto material = dynamic_pointer_cast<Material>( source );
                        if( source && !material )
                        {
                            return;
                        }
                        if( !material )
                        {
                            history.materials.push_back( { mesh, section, nullptr, {} } );
                            continue;
                        }
                        auto state = material->getRenderState();
                        if( !state || state->getFlag( IMaterial::cutoutFlag ) ||
                            state->getFlag( IMaterial::transparentFlag ) || state->blendMode != 0 ||
                            ( dynamic_cast<ClawMaterial *>( material.get() ) &&
                              static_cast<ClawMaterial *>( material.get() )->hasRuntimeProgram() ) )
                        {
                            return;
                        }
                        history.materials.push_back( { mesh, section, material, state } );
                    }
                }
                if( nativeMeshes !=
                    static_cast<size_t>( wp_graphics_cull_result_get_candidate_count( jobs->main ) ) )
                {
                    return;
                }
                wp_mat4f view, projection, matrix;
                if( !wp_graphics_cull_result_get_camera_matrices( jobs->main, &view, &projection ) )
                {
                    return;
                }
                wp_mat4f_mul( &matrix, &projection, &view );
                std::vector<float> depth;
                unsigned width = 0, height = 0;
                if( !history.readback.read( renderer, depth, width, height ) )
                {
                    return;
                }
                history.matrix = occlusionMatrix( matrix );
                if( !history.hierarchy.publish( depth.data(), width, height, history.matrix ) )
                {
                    return;
                }
                history.surface = history.readback.surface( renderer );
                history.camera = camera;
                history.capturedJobs = jobs;
                history.capturedFrame = m_occlusionFrame;
            }
            catch( const std::bad_alloc & )
            {
                history.hierarchy.clear();
                history.capturedJobs.reset();
                history.materials.clear();
            }
        }

        bool ClawScene::prepareRenderJobs( SmartPtr<IGraphicsCamera> camera )
        {
            return prepareRenderJobs( camera, false );
        }

        bool ClawScene::setDrawSortStrategy( ClawDrawSortStrategy strategy, double bucketSize )
        {
            if( !validDrawSort( strategy, bucketSize ) )
            {
                return false;
            }
            if( strategy == m_drawSortStrategy && bucketSize == m_drawSortBucketSize )
            {
                return true;
            }
            discardRenderJobs();
            m_drawSortStrategy = strategy;
            m_drawSortBucketSize = bucketSize;
            return true;
        }

        static wp_u64 cullProfileClock( void * )
        {
            return static_cast<wp_u64>( std::chrono::duration_cast<std::chrono::nanoseconds>(
                                            std::chrono::steady_clock::now().time_since_epoch() )
                                            .count() );
        }

        bool ClawScene::prepareRenderJobs( SmartPtr<IGraphicsCamera> camera, bool profile,
                                           ClawRendererDX11 *materialRenderer )
        {
            using Clock = std::chrono::steady_clock;
            const auto start = profile ? Clock::now() : Clock::time_point{};
            discardRenderJobs();
            if( !m_scene )
            {
                return false;
            }
            auto view = dynamic_pointer_cast<ClawCamera>( camera );
            wp_graphics_scene_set_active_camera( m_scene, view ? view->getNativeCamera() : nullptr );
            auto jobs = std::make_shared<NativeRenderJobs>();
            jobs->camera = camera;
            jobs->sortStrategy = m_drawSortStrategy;
            jobs->sortBucketSize = m_drawSortBucketSize;
            const auto mainStart = profile ? Clock::now() : Clock::time_point{};
            jobs->main =
                wp_graphics_scene_prepare_cull_profiled( m_scene, nullptr, WP_GRAPHICS_CULL_PASS_MAIN,
                                                         profile ? cullProfileClock : nullptr, nullptr );
            const auto mainEnd = profile ? Clock::now() : Clock::time_point{};
            jobs->shadow =
                wp_graphics_cull_result_create_view( jobs->main, nullptr, WP_GRAPHICS_CULL_PASS_SHADOW );
            const auto shadowEnd = profile ? Clock::now() : Clock::time_point{};
            if( !jobs->main || !jobs->shadow )
            {
                return false;
            }
            try
            {
                if( materialRenderer )
                {
                    jobs->captureMaterials( *materialRenderer, m_materialRecordWorkspace );
                }
            }
            catch( const std::bad_alloc & )
            {
                WP_LOG_WARNING( "Scene material record CPU budget denied; deferring publication." );
                return false;
            }
            const auto materialEnd = profile ? Clock::now() : Clock::time_point{};
            if( profile )
            {
                jobs->timing.total_ns = static_cast<u64>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>( materialEnd - start )
                        .count() );
                jobs->timing.main_prepare_ns = static_cast<u64>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>( mainEnd - mainStart )
                        .count() );
                jobs->timing.shadow_derive_ns = static_cast<u64>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>( shadowEnd - mainEnd )
                        .count() );
                jobs->timing.material_prepare_ns =
                    materialRenderer
                        ? static_cast<u64>( std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                materialEnd - shadowEnd )
                                                .count() )
                        : 0;
                jobs->profiled = true;
            }
            m_nativeRenderJobs = jobs;
            return true;
        }

        bool ClawScene::getPreparedTiming( PreparationTiming &out ) const
        {
            out = {};
            if( !m_nativeRenderJobs || !m_nativeRenderJobs->profiled )
            {
                return false;
            }
            out = m_nativeRenderJobs->timing;
            return true;
        }

        bool ClawScene::getPreparedPhaseTiming( wp_graphics_cull_timing_stats *out ) const
        {
            return wp_graphics_cull_result_get_timing_stats(
                       m_nativeRenderJobs ? m_nativeRenderJobs->main : nullptr, out ) != 0;
        }

        bool ClawScene::validateRenderJobs() const
        {
            return m_scene && m_nativeRenderJobs &&
                   wp_graphics_cull_result_validate(
                       m_nativeRenderJobs->main, m_scene,
                       static_cast<wp_u32>( wp_graphics_scene_get_generation( m_scene ) ) ) ==
                       WP_GRAPHICS_CULL_OK &&
                   wp_graphics_cull_result_validate(
                       m_nativeRenderJobs->shadow, m_scene,
                       static_cast<wp_u32>( wp_graphics_scene_get_generation( m_scene ) ) ) ==
                       WP_GRAPHICS_CULL_OK;
        }

        u32 ClawScene::getPreparedTransformDispatchCount() const
        {
            return m_nativeRenderJobs ? wp_graphics_cull_result_get_transform_dispatch_count(
                                            m_nativeRenderJobs->main ) +
                                            wp_graphics_cull_result_get_transform_dispatch_count(
                                                m_nativeRenderJobs->shadow )
                                      : 0;
        }

        bool ClawScene::getPreparedJobStatistics( wp_transform_job_stats *transforms,
                                                  wp_graphics_cull_bounds_stats *bounds,
                                                  wp_u32 *candidates, wp_u32 *visible,
                                                  wp_graphics_cull_sort_stats *sort ) const
        {
            auto main = m_nativeRenderJobs ? m_nativeRenderJobs->main : nullptr;
            wp_graphics_cull_result_get_transform_stats( main, transforms );
            wp_graphics_cull_result_get_bounds_stats( main, bounds );
            wp_graphics_cull_result_get_sort_stats( main, sort );
            if( candidates )
            {
                *candidates = static_cast<wp_u32>( wp_graphics_cull_result_get_candidate_count( main ) );
            }
            if( visible )
            {
                *visible = static_cast<wp_u32>( wp_graphics_cull_result_get_visible_count( main ) );
            }
            return main != nullptr;
        }

        bool ClawScene::getPreparedStorageStatistics( wp_graphics_cull_storage_stats *out ) const
        {
            auto main = m_nativeRenderJobs ? m_nativeRenderJobs->main : nullptr;
            wp_graphics_cull_result_get_storage_stats( main, out );
            return main != nullptr;
        }

        void ClawScene::discardRenderJobs()
        {
            // Reuse only after every consuming borrow has finished. Nested or
            // retained jobs retire normally instead of recycling live storage.
            if( m_nativeRenderJobs && m_nativeRenderJobs.use_count() == 1 &&
                m_nativeRenderJobs->materials )
            {
                m_nativeRenderJobs->materials->clear();
                m_materialRecordWorkspace = std::move( m_nativeRenderJobs->materials );
            }
            m_nativeRenderJobs.reset();
        }

        bool ClawScene::getPreparedMaterialStatistics( MaterialPreparationStatistics &out ) const
        {
            out = {};
            if( !m_nativeRenderJobs || !m_nativeRenderJobs->materials )
            {
                return false;
            }
            const auto &records = *m_nativeRenderJobs->materials;
            out.records = records.getRecordCount();
            out.bindings = records.getBindingCount();
            out.meshes = records.getMeshCount();
            out.preparations = records.getPreparationCount();
            out.dense_capacity_bytes = records.getDenseCapacityBytes();
            out.storage_allocations = m_nativeRenderJobs->materialStorageAllocations;
            out.retained_storage_bytes = records.getRetainedStorageBytes();
            out.peak_storage_bytes = records.getPeakStorageBytes();
            out.reused_storage = m_nativeRenderJobs->reusedMaterialStorage;
            return true;
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

                const bool temporaryJobs = !m_nativeRenderJobs;
                if( temporaryJobs &&
                    !prepareRenderJobs( rawRenderer->getCamera(), false, dx11Renderer ) )
                {
                    throw std::runtime_error( "Native scene job preparation failed." );
                }
                auto jobs = m_nativeRenderJobs;

                // Direct render callers release their snapshot even on exceptions.
                struct JobCleanup
                {
                    ClawScene *scene;
                    bool temporary;
                    std::shared_ptr<NativeRenderJobs> &borrowed;

                    ~JobCleanup()
                    {
                        if( temporary )
                        {
                            const bool current = scene->m_nativeRenderJobs == borrowed;
                            borrowed.reset();
                            if( current )
                            {
                                scene->discardRenderJobs();
                            }
                        }
                    }
                } cleanup{ this, temporaryJobs, jobs };

                if( !validateRenderJobs() )
                {
                    throw std::runtime_error( "Stale native scene jobs." );
                }

                ++m_occlusionFrame;
                if( projectOcclusionEnabled() )
                {
                    if( !m_occlusionHistory )
                    {
                        m_occlusionHistory = std::make_unique<OcclusionHistory>();
                    }
                    auto &history = *m_occlusionHistory;
                    history.culled = 0;
                    bool valid = history.capturedJobs && history.camera == rawRenderer->getCamera() &&
                                 history.capturedFrame + 1 == m_occlusionFrame &&
                                 history.surface == history.readback.surface( nativeRenderer ) &&
                                 wp_graphics_cull_result_validate(
                                     history.capturedJobs->main, m_scene,
                                     wp_graphics_cull_result_get_scene_generation(
                                         history.capturedJobs->main ) ) == WP_GRAPHICS_CULL_OK;
                    for( const auto &material : history.materials )
                    {
                        auto current = material.mesh->getMaterial( material.section );
                        if( !current )
                        {
                            current = material.mesh->getMaterial();
                        }
                        if( current != material.material ||
                            ( material.material &&
                              material.material->getRenderState() != material.state ) )
                        {
                            valid = false;
                        }
                    }
                    if( valid )
                    {
                        wp_mat4f view, projection, matrix;
                        if( wp_graphics_cull_result_get_camera_matrices( jobs->main, &view,
                                                                         &projection ) )
                        {
                            wp_mat4f_mul( &matrix, &projection, &view );
                            const auto current = occlusionMatrix( matrix );
                            if( current == history.matrix )
                            {
                                history.culled =
                                    static_cast<u32>( wp_graphics_cull_result_apply_occlusion(
                                        jobs->main,
                                        []( const wp_aabb3f *bounds, const wp_mat4f *world,
                                            void *context ) {
                                            const auto &previous =
                                                *static_cast<const OcclusionHistory *>( context );
                                            const float minimum[] = { bounds->min.x, bounds->min.y,
                                                                      bounds->min.z };
                                            const float maximum[] = { bounds->max.x, bounds->max.y,
                                                                      bounds->max.z };
                                            return previous.hierarchy.occluded(
                                                       minimum, maximum, occlusionMatrix( *world ),
                                                       previous.matrix )
                                                       ? 1
                                                       : 0;
                                        },
                                        &history ) );
                            }
                        }
                    }
                    history.renderedJobs = jobs;
                }
                else
                {
                    m_occlusionHistory.reset();
                }

                // Skies and terrains are C++ renderer objects and are not members of the native C
                // scene's object list. Submit them explicitly before the native mesh scene pass.
                if( dx11Renderer )
                {
                    // Normally prepared with culling. A caller that explicitly
                    // disabled capture can still switch to DX11 before drawing.
                    try
                    {
                        jobs->captureMaterials( *dx11Renderer, m_materialRecordWorkspace );
                    }
                    catch( const std::bad_alloc & )
                    {
                        WP_LOG_WARNING( "Scene material record CPU budget denied; deferring draw." );
                        return;
                    }
                    const auto foliage = getFoliageBatches();
                    const auto foliageMask =
                        m_scene->visibility_mask &
                        ( camera ? wp_camera_get_visibility_mask( camera->getNativeCamera() )
                                 : ~u32( 0 ) ) &
                        ( rawRenderer->getViewport() ? rawRenderer->getViewport()->getVisibilityMask()
                                                     : ~u32( 0 ) );
                    // The forward shader supports one directional light. Configure it once
                    // for both terrain and meshes, rather than using the renderer's preview sun.
                    Vector3F lightDirection( 0.0f, -1.0f, 0.0f );
                    ColourF lightColour = ColourF::White;
                    f32 lightIntensity = 0.0f;
                    bool lightCastsShadows = false;
                    if( auto light = getPrimaryDirectionalLight() )
                    {
                        const auto direction = light->getDerivedDirection();
                        lightDirection = Vector3F( direction.X(), direction.Y(), direction.Z() );
                        lightColour = light->getDiffuseColour();
                        lightIntensity = light->getPowerScale();
                        lightCastsShadows = light->getCastShadows();
                    }
                    dx11Renderer->setSceneLighting( getAmbientLight(), lightDirection, lightColour,
                                                    lightIntensity );
                    dx11Renderer->setSceneFog( static_cast<u32>( m_scene->fog_mode ),
                                               ColourF( m_scene->fog_colour.r, m_scene->fog_colour.g,
                                                        m_scene->fog_colour.b, 1 ),
                                               m_scene->fog_density, m_scene->fog_start,
                                               m_scene->fog_end );
                    dx11Renderer->disableShadows();
                    const auto viewport = rawRenderer->getViewport();
                    SmartPtr<IGraphicsPipeline> pipeline;
                    if( auto manager = core::IApplicationManager::instancePtr() )
                    {
                        if( auto graphics = manager->getGraphicsSystem() )
                        {
                            pipeline = graphics->getGraphicsPipeline();
                        }
                    }
                    const bool shadows = ( !pipeline || pipeline->isCSMEnabled() ) &&
                                         getEnableShadows() && lightCastsShadows &&
                                         lightIntensity > 0.0f &&
                                         ( !viewport || viewport->getShadowsEnabled() );
                    const auto terrainObjects = m_terrains.snapshot();
                    Array<SmartPtr<IGraphicsCamera>> terrainCameras;
                    for( const auto &sceneCamera : m_cameras.snapshot() )
                    {
                        if( auto terrainCamera = dynamic_pointer_cast<IGraphicsCamera>( sceneCamera ) )
                        {
                            terrainCameras.push_back( terrainCamera );
                        }
                    }
                    const bool shadowPass =
                        shadows && dx11Renderer->beginShadowMap(
                                       pipeline ? pipeline->getCsmSettings().m_shadowMapSize : 2048 );
                    // Publish once before both passes: a worker finishing during
                    // shadows cannot switch terrain revision for the colour pass.
                    for( const auto &object : terrainObjects )
                    {
                        if( auto terrain = dynamic_pointer_cast<ClawTerrain>( object ) )
                        {
                            dx11Renderer->prepareTerrain( terrain, terrainCameras );
                        }
                    }
                    if( !m_foliageWorlds )
                    {
                        m_foliageWorlds = std::make_unique<FoliageWorldState>();
                    }
                    auto &foliageState = *m_foliageWorlds;
                    Array<std::shared_ptr<ClawFoliageWorld>> foliageWorlds = foliageState.standalone;
                    std::map<ClawFoliageWorld *, SmartPtr<ClawTerrain>> foliageOwners;
                    std::map<ClawFoliageWorld *, String> foliageErrors;
                    std::map<ClawFoliageWorld *, ClawFoliageDiagnostics> foliageShadowStats;
                    std::set<ClawTerrain *> liveTerrains;
                    String foliageError, foliageFrameError;
                    const auto observeFoliage = [&]( bool success ) {
                        if( !success && foliageFrameError.empty() )
                        {
                            foliageFrameError = foliageError;
                        }
                        return success;
                    };
                    for( const auto &object : terrainObjects )
                    {
                        if( auto terrain = dynamic_pointer_cast<ClawTerrain>( object ) )
                        {
                            const auto snapshot = terrain->getFoliageSnapshot();
                            if( !snapshot )
                            {
                                continue;
                            }
                            liveTerrains.insert( terrain.get() );
                            auto &attached = foliageState.terrains[terrain.get()];
                            attached.terrain = terrain;
                            if( !attached.world )
                            {
                                attached.world = std::make_shared<ClawFoliageWorld>();
                            }
                            const auto placement = terrain->getWorldTransform();
                            // A larger world scale can make a new source's local
                            // paging range admissible. Retry after source changes
                            // when the previous source rejected the new scale.
                            attached.world->setPlacement(
                                Matrix4F( placement.getTransformationMatrix().ptr() ), foliageError );
                            if( attached.world->source().get() != snapshot.get() )
                            {
                                if( !observeFoliage(
                                        attached.world->setSource( snapshot, {}, foliageError ) ) )
                                {
                                    continue;
                                }
                            }
                            if( !observeFoliage( attached.world->setPlacement(
                                    Matrix4F( placement.getTransformationMatrix().ptr() ),
                                    foliageError ) ) )
                            {
                                continue;
                            }
                            if( !observeFoliage( attached.world->setPublicationGate(
                                    [terrain, placement]( FoliageSnapshot source,
                                                          const std::function<void()> &commit,
                                                          String &error ) mutable {
                                        return terrain->publishFoliageSnapshot(
                                            std::move( source ), placement, commit, error );
                                    },
                                    foliageError ) ) )
                            {
                                continue;
                            }
                            attached.world->setVisibilityMask( terrain->isVisible() ? ~u32( 0 ) : 0 );
                            foliageOwners[attached.world.get()] = terrain;
                            foliageWorlds.push_back( attached.world );
                        }
                    }
                    for( auto i = foliageState.terrains.begin(); i != foliageState.terrains.end(); )
                    {
                        if( !liveTerrains.count( i->first ) )
                        {
                            i = foliageState.terrains.erase( i );
                        }
                        else
                        {
                            ++i;
                        }
                    }
                    const auto makeFoliageView = [&]( SmartPtr<IGraphicsCamera> value ) {
                        ClawFoliageView view;
                        view.key = u64( reinterpret_cast<uintptr_t>( value.get() ) );
                        view.visibilityMask = foliageMask;
                        if( value )
                        {
                            const Matrix4F projection( value->getProjectionMatrix().ptr() ),
                                cameraView( value->getViewMatrix().ptr() );
                            const auto pose = cameraView.inverse();
                            view.position = { pose[0][3], pose[1][3], pose[2][3] };
                            view.worldToClip = projection * cameraView;
                            view.orthographic = std::abs( projection[3][3] ) > .5f;
                            view.projectionScale = std::max( .0001f, std::abs( projection[1][1] ) );
                        }
                        return view;
                    };
                    const auto colourFoliageView = makeFoliageView( rawRenderer->getCamera() );
                    auto shadowFoliageView = colourFoliageView;
                    shadowFoliageView.key ^= u64( 1 ) << 63;
                    shadowFoliageView.shadow = true;
                    shadowFoliageView.worldToClip = dx11Renderer->foliageViewProjection( true );
                    Array<ClawFoliageView> foliageViews{ colourFoliageView };
                    for( const auto &other : terrainCameras )
                    {
                        if( other != rawRenderer->getCamera() && foliageViews.size() < 15 )
                        {
                            foliageViews.push_back( makeFoliageView( other ) );
                        }
                    }
                    if( shadowPass )
                    {
                        foliageViews.push_back( shadowFoliageView );
                    }
                    const auto foliageFrame = dx11Renderer->terrainFrameToken();
                    if( !foliageFrame || foliageFrame != foliageState.frame )
                    {
                        foliageState.frame = foliageFrame;
                        foliageState.seconds =
                            std::chrono::duration<f32>( std::chrono::steady_clock::now() -
                                                        foliageState.epoch )
                                .count();
                    }
                    for( const auto &world : foliageWorlds )
                    {
                        observeFoliage(
                            world->setInteractions( foliageState.interactions, foliageError ) );
                        if( !observeFoliage( world->prepare( *dx11Renderer, foliageViews,
                                                             foliageState.seconds, foliageError ) ) )
                        {
                            foliageErrors[world.get()] = foliageError;
                        }
                    }
                    if( shadowPass )
                    {
                        // Camera culling cannot exclude casters: offscreen objects can shadow visible receivers.
                        const auto mask =
                            m_scene->visibility_mask &
                            ( camera ? wp_camera_get_visibility_mask( camera->getNativeCamera() )
                                     : ~u32( 0 ) ) &
                            ( viewport ? viewport->getVisibilityMask() : ~u32( 0 ) );
                        for( s32 i = 0; i < wp_graphics_cull_result_get_visible_count( jobs->shadow );
                             ++i )
                        {
                            if( wp_graphics_cull_result_validate_object( jobs->shadow, m_scene, i ) !=
                                WP_GRAPHICS_CULL_OK )
                            {
                                throw std::runtime_error( "Stale shadow draw snapshot." );
                            }
                            auto object = wp_graphics_cull_result_get_object( jobs->shadow, i );
                            if( !( wp_graphics_object_get_visibility_flags( object ) & mask ) )
                            {
                                continue;
                            }
                            auto mesh =
                                static_cast<ClawMesh *>( wp_graphics_object_get_submit_data( object ) );
                            if( !mesh || !mesh->getCastShadows() )
                            {
                                continue;
                            }
                            wp_mat4f world;
                            wp_graphics_cull_result_get_world_matrix( jobs->shadow, i, &world );
                            const auto binding = jobs->materials->find( mesh );
                            if( !binding ||
                                !dx11Renderer->renderMeshRecords( mesh, Matrix4F( world.m[0] ),
                                                                  *jobs->materials, *binding ) )
                            {
                                throw std::runtime_error( "Stale shadow material binding." );
                            }
                        }
                        for( auto &object : terrainObjects )
                        {
                            if( auto terrain = dynamic_pointer_cast<ClawTerrain>( object ) )
                            {
                                dx11Renderer->renderTerrain( terrain, false );
                            }
                        }
                        if( foliage )
                        {
                            for( const auto &batch : *foliage )
                            {
                                if( batch->castsShadows() && ( batch->visibilityMask() & foliageMask ) )
                                {
                                    batch->render( *dx11Renderer );
                                }
                            }
                        }
                        for( const auto &world : foliageWorlds )
                        {
                            auto view = shadowFoliageView;
                            const auto owner = foliageOwners.find( world.get() );
                            if( owner != foliageOwners.end() )
                            {
                                view.forcedLod = owner->second->getFoliageForcedLod();
                            }
                            if( !observeFoliage( world->render( *dx11Renderer, view, foliageError ) ) )
                            {
                                foliageErrors[world.get()] = foliageError;
                            }
                            foliageShadowStats[world.get()] = world->diagnostics();
                        }
                        dx11Renderer->endShadowMap();
                    }
                    for( auto &sky : m_skies.snapshot() )
                    {
                        dx11Renderer->renderSky( sky );
                    }

                    for( auto &terrainObject : terrainObjects )
                    {
                        if( auto terrain = dynamic_pointer_cast<ClawTerrain>( terrainObject ) )
                        {
                            dx11Renderer->renderTerrain( terrain, false );
                        }
                    }
                    // Render one retained population snapshot in both passes.
                    if( foliage )
                    {
                        for( const auto &batch : *foliage )
                        {
                            if( batch->visibilityMask() & foliageMask )
                            {
                                batch->render( *dx11Renderer );
                            }
                        }
                    }
                    for( const auto &world : foliageWorlds )
                    {
                        auto view = colourFoliageView;
                        const auto owner = foliageOwners.find( world.get() );
                        if( owner != foliageOwners.end() )
                        {
                            view.forcedLod = owner->second->getFoliageForcedLod();
                        }
                        if( !observeFoliage( world->render( *dx11Renderer, view, foliageError ) ) )
                        {
                            foliageErrors[world.get()] = foliageError;
                        }
                        if( owner != foliageOwners.end() )
                        {
                            const auto stats = world->diagnostics();
                            const auto shadow = foliageShadowStats.find( world.get() );
                            Terrain::FoliageDiagnostics published;
                            published.currentGenerationReady = stats.currentGenerationReady;
                            published.sourceRevision = stats.sourceRevision;
                            published.viewKey = stats.viewKey;
                            published.residentPages = stats.residentPages;
                            published.pendingPages = stats.pendingPages;
                            published.failedPages = stats.failedPages;
                            published.residentInstances = stats.residentInstances;
                            published.testedInstances = stats.testedInstances;
                            published.culledInstances = stats.culledInstances;
                            published.lodInstances = stats.lodInstances;
                            published.groups = stats.groups;
                            published.predictedDraws = stats.predictedDraws;
                            published.actualDraws = stats.actualDraws;
                            published.submittedInstances = stats.submittedInstances;
                            published.geometryBytes = stats.geometryBytes;
                            published.uploadBytes = stats.uploadBytes;
                            if( shadow != foliageShadowStats.end() )
                            {
                                published.groups += shadow->second.groups;
                                published.predictedDraws += shadow->second.predictedDraws;
                                published.actualDraws += shadow->second.actualDraws;
                                published.submittedInstances += shadow->second.submittedInstances;
                                published.uploadBytes += shadow->second.uploadBytes;
                            }
                            published.error = foliageErrors[world.get()];
                            owner->second->setFoliageDiagnostics( published );
                        }
                    }
                    if( foliageFrameError != foliageState.error )
                    {
                        foliageState.error = foliageFrameError;
                        if( !foliageFrameError.empty() )
                        {
                            WP_LOG_ERROR( "ClawScene foliage unavailable: " + foliageFrameError );
                        }
                    }

                    // Use the material-aware C++ draw path for native mesh objects while
                    // retaining the native scene's visibility filtering and queue ordering.
                    struct MeshSubmission
                    {
                        ClawRendererDX11 *renderer;
                        const ClawMaterialDrawRecords *materials;
                        bool failed = false;
                        wp_mat4f view, projection;
                    } meshSubmission{};

                    meshSubmission.renderer = dx11Renderer;
                    meshSubmission.materials = jobs->materials.get();
                    wp_mat4f_identity( &meshSubmission.view );
                    wp_mat4f_identity( &meshSubmission.projection );
                    wp_graphics_cull_result_get_camera_matrices( jobs->main, &meshSubmission.view,
                                                                 &meshSubmission.projection );
                    const auto submission = wp_graphics_scene_render_cull_result(
                        m_scene, nativeRenderer, jobs->main,
                        []( wp_graphics_object *object, wp_renderer *, const wp_mat4f *world,
                            void *data ) -> wp_s32 {
                            auto &submission = *static_cast<MeshSubmission *>( data );
                            auto renderer = submission.renderer;
                            auto mesh =
                                static_cast<ClawMesh *>( wp_graphics_object_get_submit_data( object ) );
                            if( !mesh )
                            {
                                return 0;
                            }
                            const auto binding = submission.materials->find( mesh );
                            if( !binding || !renderer->renderMeshRecords(
                                                mesh, Matrix4F( world->m[0] ), *submission.materials,
                                                *binding, &submission.view, &submission.projection ) )
                            {
                                submission.failed = true;
                            }
                            return 1;
                        },
                        &meshSubmission );
                    if( submission != WP_GRAPHICS_CULL_OK || meshSubmission.failed )
                    {
                        throw std::runtime_error( "Stale colour draw snapshot." );
                    }

                    // Particle simulation is updated by GraphicsScene::update, once per
                    // simulation cycle. Each camera consumes its own immutable draw snapshot.
                    for( auto &object : m_particleSystems.snapshot() )
                    {
                        auto particles = dynamic_pointer_cast<CParticleSystem>( object );
                        if( !particles || !particles->isVisible() || !particles->isLoaded() )
                        {
                            continue;
                        }

                        auto viewport = rawRenderer->getViewport();
                        if( viewport &&
                            !( particles->getVisibilityFlags() & viewport->getVisibilityMask() ) )
                        {
                            continue;
                        }

                        auto owner = particles->getOwner();
                        Matrix4F world =
                            owner
                                ? Matrix4F( owner->getWorldTransform().getTransformationMatrix().ptr() )
                                : Matrix4F::identity();
                        if( auto clawOwner = dynamic_pointer_cast<ClawSceneNode>( owner ) )
                        {
                            wp_mat4f nativeWorld{};
                            wp_scenenode_get_world_matrix( clawOwner->getNativeNode(), &nativeWorld );
                            world = Matrix4F( nativeWorld.m[0] );
                        }

                        auto manager = core::IApplicationManager::instancePtr();
                        auto graphics = manager->getGraphicsSystem();
                        auto materials = graphics->getMaterialManager();
                        auto paticlesMaterialName = particles->getMaterialName();

                        SmartPtr<IMaterial> material;

                        if( !StringUtil::isNullOrEmpty( paticlesMaterialName ) )
                        {
                            material = dynamic_pointer_cast<IMaterial>(
                                materials->getByName( particles->getMaterialName() ) );
                        }

                        const auto scale = particles->getScale();
                        dx11Renderer->renderParticles( particles->getRenderSnapshot(), world,
                                                       Vector3F( scale.X(), scale.Y(), scale.Z() ),
                                                       material );
                    }
                }
                else
                {
                    if( wp_graphics_scene_render_cull_result( m_scene, nativeRenderer, jobs->main,
                                                              nullptr, nullptr ) != WP_GRAPHICS_CULL_OK )
                    {
                        throw std::runtime_error( "Stale software draw snapshot." );
                    }
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
            else
            {
                ScopedLock lock( this );

                if( GraphicsScene::handleStateChanged( state ) )
                {
                    return true;
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
