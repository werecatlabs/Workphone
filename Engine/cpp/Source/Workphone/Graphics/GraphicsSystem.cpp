#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>
#include <Workphone/Interface/Graphics/IDebugLine.hpp>
#include <Workphone/Interface/Graphics/IDebugCircle.hpp>
#include <Workphone/Interface/Graphics/IDebugText.hpp>
#include <Workphone/Interface/Graphics/IDecalCursor.hpp>
#include <Workphone/Interface/Graphics/IGraphicsDeferredShading.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <Workphone/Interface/Graphics/IFontManager.hpp>
#include <Workphone/Interface/Graphics/IFont.hpp>
#include <Workphone/Interface/Graphics/IInstanceManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsLight.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IMaterialManager.hpp>
#include <Workphone/Interface/Graphics/IMeshConverter.hpp>
#include <Workphone/Interface/Graphics/IOverlay.hpp>
#include <Workphone/Interface/Graphics/IOverlayElement.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementContainer.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementText.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementVector.hpp>
#include <Workphone/Interface/Graphics/IOverlayManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/ISprite.hpp>
#include <Workphone/Interface/Graphics/IRenderer.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Graphics/IBillboard.hpp>
#include <Workphone/Interface/Graphics/IBillboardSet.hpp>
#include <Workphone/Interface/Graphics/IDynamicMesh.hpp>
#include <Workphone/Interface/Graphics/IInstancedObject.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IParticleEmitter.hpp>
#include <Workphone/Interface/Graphics/IParticleAffector.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/Interface/Graphics/ISky.hpp>
#include <Workphone/Interface/Graphics/ISkybox.hpp>
#include <Workphone/Interface/Graphics/ISkyboxCube.hpp>
#include <Workphone/Interface/Graphics/ISkySphere.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSkeleton.hpp>
#include <Workphone/Interface/Graphics/IGraphicsBone.hpp>
#include <Workphone/Interface/Graphics/IAnimationController.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/Graphics/IRenderTexture.hpp>
#include <Workphone/Interface/Graphics/IShader.hpp>
#include <Workphone/Interface/Graphics/IComputeShader.hpp>
#include <Workphone/Interface/Graphics/IVideo.hpp>
#include <Workphone/Interface/Graphics/IVideoTexture.hpp>
#include <Workphone/Interface/Graphics/IVideoManager.hpp>
#include <Workphone/Interface/System/IResourceGroupManager.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Graphics/GraphicsSettings.hpp>
#include <Workphone/Mesh/MeshConverter.hpp>
namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsSystem,
                               SharedGraphicsObject<IGraphicsSystem> );
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsSystem::StateListener, IStateListener );

    GraphicsSystem::GraphicsSystem()
    {
        static const auto graphicsSystemStr = String( "GraphicsSystem" );
        setName( graphicsSystemStr );

        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    GraphicsSystem::~GraphicsSystem()
    {
        WP_ASSERT( m_scenes.empty() );
        WP_ASSERT( m_loadQueue.empty() );
        WP_ASSERT( m_unloadQueue.empty() );
    }

    void GraphicsSystem::load( SmartPtr<ISharedObject> data )
    {
        if( !m_meshConverter )
        {
            m_meshConverter = workphone::make_ptr<MeshConverter>();
        }
    }

    void GraphicsSystem::unload( SmartPtr<ISharedObject> data )
    {
        for( auto window : m_windows )
        {
            window->unload( nullptr );
        }

        for( auto scene : m_scenes )
        {
            scene->unload( nullptr );
        }

        if( auto scene = getGraphicsScene() )
        {
            if( scene->isLoaded() )
            {
                scene->unload( data );
            }

            setGraphicsScene( nullptr );
        }

        m_debug = nullptr;
        m_overlayMgr = nullptr;
        m_resourceGroupManager = nullptr;
        m_materialManager = nullptr;
        m_textureManager = nullptr;
        m_instanceManager = nullptr;
        m_meshConverter = nullptr;
        m_meshManager = nullptr;
        m_fontManager = nullptr;
        m_factoryManager = nullptr;
        m_resGrpMgr = nullptr;

        m_windows.clear();
        m_scenes.clear();
        m_deferredShadingSystems.clear();
        m_graphicsObjects.clear();
        m_loadQueue.clear();
        m_unloadQueue.clear();
    }
    void GraphicsSystem::update()
    {
        updateLoadQueue();
        updateUnloadQueue();

        for( auto &scene : m_scenes )
        {
            scene->update();
        }
    }

    void GraphicsSystem::lock()
    {
        m_mutex.lock();
    }

    bool GraphicsSystem::try_lock()
    {
        return m_mutex.try_lock();
    }

    void GraphicsSystem::unlock()
    {
        m_mutex.unlock();
    }

    Array<SmartPtr<IGraphicsWindow>> GraphicsSystem::getWindows() const
    {
        return m_windows.snapshot();
    }

    Array<SmartPtr<IGraphicsScene>> GraphicsSystem::getSceneManagers() const
    {
        return m_scenes.snapshot();
    }

    SmartPtr<IGraphicsScene> GraphicsSystem::getGraphicsScene( const String &name ) const
    {
        for( auto &scene : m_scenes )
        {
            if( scene->getName() == name )
            {
                return scene;
            }
        }

        return nullptr;
    }

    IGraphicsScene *GraphicsSystem::getGraphicsScenePtr() const
    {
        return m_sceneManager.get();
    }

    SmartPtr<IGraphicsScene> GraphicsSystem::getGraphicsScene() const
    {
        return m_sceneManager;
    }

    void GraphicsSystem::setGraphicsScene( SmartPtr<IGraphicsScene> smgr )
    {
        m_sceneManager = smgr;
    }

    SmartPtr<IGraphicsScene> GraphicsSystem::getGraphicsSceneById( hash_type id ) const
    {
        for( auto &scene : m_scenes )
        {
            auto handle = scene->getHandle();
            if( handle->getId() == id )
            {
                return scene;
            }
        }

        return nullptr;
    }

    void GraphicsSystem::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

    IFactoryManager *GraphicsSystem::getFactoryManagerPtr() const
    {
        return m_factoryManager.get();
    }

    SmartPtr<IFactoryManager> GraphicsSystem::getFactoryManager() const
    {
        return m_factoryManager;
    }

    void GraphicsSystem::updateUnloadQueue()
    {
        if( !m_unloadQueue.empty() )
        {
            SmartPtr<ISharedObject> obj;
            while( m_unloadQueue.try_pop( obj ) )
            {
                obj->unload( nullptr );
            }
        }
    }

    void GraphicsSystem::updateLoadQueue()
    {
        if( !m_loadQueue.empty() )
        {
            SmartPtr<ISharedObject> obj;
            while( m_loadQueue.try_pop( obj ) )
            {
                obj->load( nullptr );
            }
        }
    }

    s32 GraphicsSystem::getLoadPriority( SmartPtr<ISharedObject> obj )
    {
        // ====================================================================
        // Tier 1: Core Infrastructure (must load first)
        // ====================================================================

        // Graphics window is the foundation - highest priority
        if( obj->isDerived<IGraphicsWindow>() )
        {
            return 100000;
        }

        // Resource management infrastructure
        if( obj->isDerived<IResourceGroupManager>() )
        {
            return 95000;
        }

        // Material manager for material resource handling
        if( obj->isDerived<IMaterialManager>() )
        {
            return 94000;
        }

        // Texture manager for texture resource handling
        if( obj->isDerived<ITextureManager>() )
        {
            return 93000;
        }

        // Font manager for font resources
        if( obj->isDerived<IFontManager>() )
        {
            return 92000;
        }

        // Overlay manager for UI overlay management
        if( obj->isDerived<IOverlayManager>() )
        {
            return 91000;
        }

        // ====================================================================
        // Tier 2: Scene Structure
        // ====================================================================

        // Graphics scene container
        if( obj->isDerived<IGraphicsScene>() )
        {
            return 90000;
        }

        // Scene nodes for scene hierarchy
        if( obj->isDerived<IGraphicsSceneNode>() )
        {
            return 85000;
        }

        // ====================================================================
        // Tier 3: Viewing & Rendering
        // ====================================================================

        // Camera for viewing the scene
        if( obj->isDerived<IGraphicsCamera>() )
        {
            return 80000;
        }

        // Viewport for rendering output
        if( obj->isDerived<IViewport>() )
        {
            return 79000;
        }

        // Renderer for rendering operations
        if( obj->isDerived<IRenderer>() )
        {
            return 78500;
        }

        // Render targets for off-screen rendering
        if( obj->isDerived<IRenderTarget>() )
        {
            return 78000;
        }

        // Render textures for render-to-texture
        if( obj->isDerived<IRenderTexture>() )
        {
            return 77500;
        }

        // ====================================================================
        // Tier 4: Materials & Shaders
        // ====================================================================

        // Materials for surface appearance
        if( obj->isDerived<IMaterial>() )
        {
            return 75000;
        }

        // Textures for material surfaces
        if( obj->isDerived<ITexture>() )
        {
            return 74000;
        }

        // Shaders for GPU programs
        if( obj->isDerived<IShader>() )
        {
            return 73000;
        }

        // Compute shaders for GPU compute
        if( obj->isDerived<IComputeShader>() )
        {
            return 72500;
        }

        // ====================================================================
        // Tier 5: Lighting
        // ====================================================================

        // Lights for scene illumination
        if( obj->isDerived<IGraphicsLight>() )
        {
            return 70000;
        }

        // ====================================================================
        // Tier 6: Geometry & Objects
        // ====================================================================

        // Meshes for 3D geometry
        if( obj->isDerived<IGraphicsMesh>() )
        {
            return 65000;
        }

        // Dynamic meshes for animated/modified geometry
        if( obj->isDerived<IDynamicMesh>() )
        {
            return 64000;
        }

        // Billboards for point sprites
        if( obj->isDerived<IBillboard>() )
        {
            return 63000;
        }

        // Billboard sets for grouped billboards
        if( obj->isDerived<IBillboardSet>() )
        {
            return 62500;
        }

        // Sprites for 2D graphics
        if( obj->isDerived<ISprite>() )
        {
            return 62000;
        }

        // Instanced objects for GPU instancing
        if( obj->isDerived<IInstancedObject>() )
        {
            return 61500;
        }

        // Instance manager for instancing support
        if( obj->isDerived<IInstanceManager>() )
        {
            return 61000;
        }

        // ====================================================================
        // Tier 7: Particle Systems
        // ====================================================================

        // Particle systems for effects
        if( obj->isDerived<IParticleSystem>() )
        {
            return 60000;
        }

        // Particle emitters
        if( obj->isDerived<IParticleEmitter>() )
        {
            return 59000;
        }

        // Particle affectors
        if( obj->isDerived<IParticleAffector>() )
        {
            return 58500;
        }

        // ====================================================================
        // Tier 8: Environment & Terrain
        // ====================================================================

        // Terrain for landscape rendering
        if( obj->isDerived<IGraphicsTerrain>() )
        {
            return 55000;
        }

        // Sky elements for skybox/skydome
        if( obj->isDerived<ISky>() )
        {
            return 54000;
        }

        // Skybox for cube sky rendering
        if( obj->isDerived<ISkybox>() )
        {
            return 53500;
        }

        // SkyboxCube for cube map sky
        if( obj->isDerived<ISkyboxCube>() )
        {
            return 53000;
        }

        // SkySphere for spherical sky
        if( obj->isDerived<ISkySphere>() )
        {
            return 52500;
        }

        // ====================================================================
        // Tier 9: Animation & Skeletons
        // ====================================================================

        // Graphics skeletons for skeletal animation
        if( obj->isDerived<IGraphicsSkeleton>() )
        {
            return 50000;
        }

        // Graphics bones for skeleton hierarchy
        if( obj->isDerived<IGraphicsBone>() )
        {
            return 49000;
        }

        // Animation controllers
        if( obj->isDerived<IAnimationController>() )
        {
            return 48000;
        }

        // ====================================================================
        // Tier 10: Debug & Visualization
        // ====================================================================

        // Debug visualization
        if( obj->isDerived<IDebug>() )
        {
            return 45000;
        }

        // Debug lines
        if( obj->isDerived<IDebugLine>() )
        {
            return 44000;
        }

        // Debug circles
        if( obj->isDerived<IDebugCircle>() )
        {
            return 43500;
        }

        // Debug text
        if( obj->isDerived<IDebugText>() )
        {
            return 43000;
        }

        // Decal cursor for projected decals
        if( obj->isDerived<IDecalCursor>() )
        {
            return 42000;
        }

        // ====================================================================
        // Tier 11: Video & Media
        // ====================================================================

        // Video streams
        if( obj->isDerived<IVideo>() )
        {
            return 40000;
        }

        // Video textures
        if( obj->isDerived<IVideoTexture>() )
        {
            return 39000;
        }

        // Video manager
        if( obj->isDerived<IVideoManager>() )
        {
            return 38000;
        }

        // ====================================================================
        // Tier 12: Fonts & Text
        // ====================================================================

        // Font resources
        if( obj->isDerived<IFont>() )
        {
            return 35000;
        }

        // ====================================================================
        // Tier 13: Overlay & UI Elements
        // ====================================================================

        // Overlay containers
        if( obj->isDerived<IOverlay>() )
        {
            return 30000;
        }

        // Overlay elements
        if( obj->isDerived<IOverlayElement>() )
        {
            return 25000;
        }

        // Overlay element containers
        if( obj->isDerived<IOverlayElementContainer>() )
        {
            return 24000;
        }

        // Overlay text elements
        if( obj->isDerived<IOverlayElementText>() )
        {
            return 23000;
        }

        // Overlay vector elements
        if( obj->isDerived<IOverlayElementVector>() )
        {
            return 22000;
        }

        // UI Manager
        if( obj->isDerived<ui::IUIManager>() )
        {
            return 20000;
        }

        // ====================================================================
        // Default: Unknown types
        // ====================================================================

        return 0;
    }

    SmartPtr<IBuildDirector> GraphicsSystem::createConfiguration()
    {
        auto factoryManager = getFactoryManagerPtr();
        if( !factoryManager )
        {
            return nullptr;
        }

        return factoryManager->make_ptr<GraphicsSettings>();
    }

    bool GraphicsSystem::configure( SmartPtr<IBuildDirector> config )
    {
        // Base implementation - derived classes should override with backend-specific configuration
        return config != nullptr;
    }

    void GraphicsSystem::messagePump()
    {
        // Base implementation - derived classes should override with platform-specific message handling
    }

    IDebug *GraphicsSystem::getDebugPtr() const
    {
        return m_debug.get();
    }

    SmartPtr<IDebug> GraphicsSystem::getDebug() const
    {
        return m_debug;
    }

    void GraphicsSystem::setDebug( SmartPtr<IDebug> debug )
    {
        m_debug = debug;
    }

    SmartPtr<IOverlayManager> GraphicsSystem::getOverlayManager() const
    {
        return m_overlayMgr;
    }

    SmartPtr<IResourceGroupManager> GraphicsSystem::getResourceGroupManager() const
    {
        return m_resourceGroupManager;
    }

    IMaterialManager *GraphicsSystem::getMaterialManagerPtr() const
    {
        return m_materialManager.get();
    }

    SmartPtr<IMaterialManager> GraphicsSystem::getMaterialManager() const
    {
        return m_materialManager;
    }

    ITextureManager *GraphicsSystem::getTextureManagerPtr() const
    {
        return m_textureManager.get();
    }

    SmartPtr<ITextureManager> GraphicsSystem::getTextureManager() const
    {
        return m_textureManager;
    }

    SmartPtr<IInstanceManager> GraphicsSystem::getInstanceManager() const
    {
        return m_instanceManager;
    }

    IRenderer *GraphicsSystem::getRendererPtr() const
    {
        return m_renderer.get();
    }

    SmartPtr<IRenderer> GraphicsSystem::getRenderer() const
    {
        return m_renderer;
    }

    SmartPtr<IFontManager> GraphicsSystem::getFontManager() const
    {
        return m_fontManager;
    }

    SmartPtr<IGraphicsWindow> GraphicsSystem::createRenderWindow(
        const String &name, u32 width, u32 height, bool fullScreen,
        const SmartPtr<Properties> &properties )
    {
        auto factoryManager = getFactoryManagerPtr();
        if( !factoryManager )
        {
            return nullptr;
        }

        auto window = factoryManager->make_object<IGraphicsWindow>();
        if( !window )
        {
            return nullptr;
        }

        window->setName( name );
        window->setSize( Vector2I( static_cast<s32>( width ), static_cast<s32>( height ) ) );
        window->setFullscreen( fullScreen );

        window->load( nullptr );

        m_windows.push_back( window );

        if( !m_defaultWindow )
        {
            m_defaultWindow = window;
        }

        return window;
    }

    void GraphicsSystem::destroyRenderWindow( SmartPtr<IGraphicsWindow> window )
    {
        if( !window )
        {
            return;
        }

        auto it = std::find( m_windows.begin(), m_windows.end(), window );
        if( it != m_windows.end() )
        {
            ( *it )->unload( nullptr );
            m_windows.erase( it );
        }

        if( m_defaultWindow == window )
        {
            m_defaultWindow = nullptr;
        }
    }

    SmartPtr<IGraphicsWindow> GraphicsSystem::getDefaultWindow() const
    {
        return m_defaultWindow;
    }

    void GraphicsSystem::setDefaultWindow( SmartPtr<IGraphicsWindow> defaultWindow )
    {
        m_defaultWindow = defaultWindow;
    }

    void GraphicsSystem::removeDeferredShadingSystem( SmartPtr<IViewport> vp )
    {
        if( !vp )
        {
            return;
        }

        auto it = std::find_if( m_deferredShadingSystems.begin(), m_deferredShadingSystems.end(),
                                [&]( const SmartPtr<IGraphicsDeferredShading> &system ) {
                                    if( !system )
                                    {
                                        return false;
                                    }

                                    // Since there's no getViewport, we'll need derived classes to
                                    // override this method. For now, just return false.
                                    return false;
                                } );

        if( it != m_deferredShadingSystems.end() )
        {
            ( *it )->unload( nullptr );
            m_deferredShadingSystems.erase( it );
        }
    }

    Array<SmartPtr<IGraphicsDeferredShading>> GraphicsSystem::getDeferredShadingSystems() const
    {
        return m_deferredShadingSystems;
    }

    void GraphicsSystem::loadObject( SmartPtr<ISharedObject> graphicsObject,
                                     bool forceQueue /*= false */ )
    {
        if( !graphicsObject )
        {
            return;
        }

        if( forceQueue )
        {
            m_loadQueue.push( graphicsObject );
        }
        else
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto taskManager = applicationManager->getTaskManagerPtr();
            auto currentTask = Thread::getCurrentTask();
            auto renderTask = getRenderTask();

            if( currentTask == renderTask )
            {
                ScopedLock lock( this );
                graphicsObject->load( nullptr );
            }
            else if( currentTask == TaskId::Primary )
            {
                ScopedLock lock( this );
                graphicsObject->load( nullptr );
            }
            else
            {
                m_loadQueue.push( graphicsObject );
            }
        }
    }

    void GraphicsSystem::reloadObject( SmartPtr<ISharedObject> graphicsObject, bool forceQueue )
    {
        if( !graphicsObject )
        {
            return;
        }

        if( forceQueue )
        {
            m_reloadQueue.push( graphicsObject );
        }
        else
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto taskManager = applicationManager->getTaskManagerPtr();
            auto currentTask = Thread::getCurrentTask();
            auto renderTask = getRenderTask();
            if( currentTask == renderTask )
            {
                graphicsObject->reload( nullptr );
            }
            else
            {
                m_reloadQueue.push( graphicsObject );
            }
        }
    }

    void GraphicsSystem::unloadObject( SmartPtr<ISharedObject> graphicsObject,
                                       bool forceQueue /*= false */ )
    {
        if( !graphicsObject )
        {
            return;
        }

        if( forceQueue )
        {
            m_unloadQueue.push( graphicsObject );
        }
        else
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto taskManager = applicationManager->getTaskManagerPtr();
            auto threadPool = applicationManager->getThreadPool();

            if( threadPool && threadPool->getNumThreads() > 0 )
            {
                auto currentTask = Thread::getCurrentTask();
                auto renderTask = getRenderTask();

                if( currentTask == renderTask )
                {
                    graphicsObject->unload( nullptr );
                }
                else
                {
                    m_unloadQueue.push( graphicsObject );
                }
            }
            else
            {
                graphicsObject->unload( nullptr );
            }
        }
    }

    void GraphicsSystem::clearObjectQueues()
    {
        m_loadQueue.clear();
        m_unloadQueue.clear();
    }

    void GraphicsSystem::setupRenderer( SmartPtr<IGraphicsScene> sceneManager,
                                        SmartPtr<IGraphicsWindow> window,
                                        SmartPtr<IGraphicsCamera> camera, String workspaceName,
                                        bool enabled )
    {
        // Base implementation - derived classes should override with backend-specific renderer setup
        // This method sets up compositor workspaces, render targets, and other renderer-specific state
    }

    TaskId GraphicsSystem::getStateTask() const
    {
        return TaskId::Primary;
    }

    TaskId GraphicsSystem::getRenderTask() const
    {
        return TaskId::Render;
    }

    SmartPtr<IMeshConverter> GraphicsSystem::getMeshConverter() const
    {
        return m_meshConverter;
    }

    void GraphicsSystem::setMeshConverter( SmartPtr<IMeshConverter> meshConverter )
    {
        m_meshConverter = meshConverter;
    }

    SmartPtr<IGraphicsDeferredShading> GraphicsSystem::addDeferredShadingSystem( SmartPtr<IViewport> vp )
    {
        if( !vp )
        {
            return nullptr;
        }

        auto factoryManager = getFactoryManagerPtr();
        if( !factoryManager )
        {
            return nullptr;
        }

        auto deferredShadingSystem = factoryManager->make_object<IGraphicsDeferredShading>();
        if( !deferredShadingSystem )
        {
            return nullptr;
        }

        // deferredShadingSystem->setViewport( vp );
        deferredShadingSystem->load( nullptr );

        m_deferredShadingSystems.push_back( deferredShadingSystem );

        return deferredShadingSystem;
    }

    SmartPtr<IGraphicsScene> GraphicsSystem::addGraphicsScene( const String &type, const String &name )
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( type ) );
        WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = getFactoryManagerPtr();

        auto scene = factoryManager->make_object<IGraphicsScene>();
        scene->setType( type );
        scene->setName( name );

        auto defaultSceneManager = getGraphicsScene();
        if( !defaultSceneManager )
        {
            setGraphicsScene( scene );
        }

        m_scenes.push_back( scene );
        scene->load( nullptr );

        return scene;
    }

    void GraphicsSystem::removeGraphicsScene( SmartPtr<IGraphicsScene> scene )
    {
        if( scene )
        {
            scene->unload( nullptr );
            m_scenes.erase( std::remove( m_scenes.begin(), m_scenes.end(), scene ), m_scenes.end() );
        }
    }

    void GraphicsSystem::removeAllGraphicsScenes()
    {
        auto scenes = m_scenes.snapshot();
        m_scenes.clear();
        m_sceneManager = nullptr;

        for( auto &scene : scenes )
        {
            if( scene )
            {
                scene->unload( nullptr );
            }
        }
    }

    void GraphicsSystem::clearGraphicScenes()
    {
        for( auto &scene : m_scenes )
        {
            if( scene )
            {
                scene->clear();
            }
        }
    }

    SmartPtr<IGraphicsWindow> GraphicsSystem::getRenderWindow(
        const String &name /*= StringUtil::EmptyString */ ) const
    {
        auto windows = m_windows.snapshot();
        for( auto window : windows )
        {
            if( window->getName() == name )
            {
                return window;
            }
        }

        return nullptr;
    }

    void GraphicsSystem::render()
    {
    }

    void GraphicsSystem::StateListener::setOwner( SmartPtr<GraphicsSystem> owner )
    {
        m_owner = owner;
    }

    SmartPtr<GraphicsSystem> GraphicsSystem::StateListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    GraphicsSystem *GraphicsSystem::StateListener::getOwnerPtr() const
    {
        return m_owner.get();
    }

    bool GraphicsSystem::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

        auto graphicsScenes = graphicsSystem->getSceneManagers();
        for( auto &graphicsScene : graphicsScenes )
        {
            if( graphicsScene )
            {
                if( graphicsScene->handleStateChanged( state ) )
                {
                    return true;
                }
            }
        }

        auto windows = graphicsSystem->getWindows();
        for( auto &window : windows )
        {
            if( window )
            {
                if( window->handleStateChanged( state ) )
                {
                    return true;
                }
            }
        }

        auto textureManager = graphicsSystem->getTextureManager();
        if( textureManager )
        {
            textureManager->handleStateChanged( state );
        }

        return false;
    }

    bool GraphicsSystem::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

        auto graphicsScenes = graphicsSystem->getSceneManagers();
        for( auto graphicsScene : graphicsScenes )
        {
            if( graphicsScene )
            {
                if( graphicsScene->handleStateMessage( message ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    void GraphicsSystem::StateListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    GraphicsSystem::StateListener::~StateListener() = default;

    GraphicsSystem::StateListener::StateListener() = default;

}  // namespace workphone::render
