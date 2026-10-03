#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/ImguiManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Compositor.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsMeshOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CResourceGroupManager.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CWindowOgreNext.hpp>
#include <WPGraphicsOgreNext/ResourceLoadingListener.hpp>
#include <WPGraphicsOgreNext/WPGraphicsOgreNextTypes.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCameraOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CWindowOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDebugOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CFontManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#include <WPGraphicsOgreNext/UIRenderer.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreRoot.h>
#include <OgreException.h>
#include <OgreConfigFile.h>
#include <OgreCamera.h>
#include <OgreItem.h>
#include <OgreHlmsUnlit.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsManager.h>
#include <OgreArchiveManager.h>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreTextureGpuManager.h>
#include <OgreWindowEventUtilities.h>
#include <OgreWindow.h>
#include <OgreFileSystemLayer.h>
#include <OgreHlmsDiskCache.h>
#include <OgreGpuProgramManager.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreLogManager.h>
#include <OgreParticle.h>
#include <OgreParticleEmitter.h>
#include <OgreParticleEmitterFactory.h>
#include <OgreParticleSystemManager.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <OgreAbiUtils.h>

#ifdef OGRE_STATIC_LIB
#    if WP_BUILD_RENDERER_GL3PLUS
#        include <OgreGL3PlusPlugin.h>
#    endif
#    if WP_BUILD_RENDERER_GLES2
#        include "OgreGLES2Plugin.h"
#    endif
#    if WP_BUILD_RENDERER_DX11
#        include <OgreD3D11Plugin.h>
#    endif
#    if WP_BUILD_RENDERER_VULKAN
#        include <OgreVulkanPlugin.h>
#    endif
#    if WP_BUILD_RENDERER_METAL
#        include <OgreMetalPlugin.h>
#    endif
#endif

#if WP_OGRE_USE_PARTICLE_UNIVERSE
#    include <ParticleUniversePlugin.h>
#endif

#include <OgreHlmsBufferManager.h>
#include <OgreMemoryAllocatorConfig.h>
#include <OgreOverlay.h>
#include <OgreOverlayContainer.h>
#include <OgreTextAreaOverlayElement.h>
#include <OgreFrameStats.h>
#include <Terra/Terra.h>
#include <Terra/TerraShadowMapper.h>
#include <Terra/Hlms/OgreHlmsTerra.h>
#include <OgreWindowEventUtilities.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
#    include <xcb/xcb.h>
#    include <X11/Xlib.h>
extern void GLXProc( Ogre::Window *win, const XEvent &event );
extern void XcbProc( xcb_connection_t *xcbConnection, xcb_generic_event_t *event );

typedef Ogre::map<xcb_window_t, Ogre::Window *>::type XcbWindowMap;
static XcbWindowMap gXcbWindowToOgre;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
//#    include "OSX/macUtils.h"
#    include <WPGraphicsOgreNext/Window/Apple/macOS/macUtil.hpp>
#endif

namespace
{
    /**
     * Minimal point emitter for Ogre Next's legacy billboard particle renderer.
     *
     * Ogre Next still contains ParticleSystem itself, but the stock Point emitter
     * lives in the optional ParticleFX plugin, which is not part of this renderer
     * build. Code-created particle systems therefore need this small built-in
     * implementation.
     */
    class PointParticleEmitter final : public Ogre::ParticleEmitter
    {
    public:
        explicit PointParticleEmitter( Ogre::ParticleSystem *particleSystem ) :
            Ogre::ParticleEmitter( particleSystem )
        {
            mType = "Point";

            if( createParamDictionary( "WorkphonePointEmitter" ) )
            {
                addBaseParameters();
            }
        }

        unsigned short _getEmissionCount( Ogre::Real timeElapsed ) override
        {
            return genConstantEmissionCount( timeElapsed );
        }

        void _initParticle( Ogre::Particle *particle ) override
        {
            Ogre::ParticleEmitter::_initParticle( particle );

            particle->mPosition = mPosition;
            genEmissionColour( particle->mColour );
            genEmissionDirection( particle->mPosition, particle->mDirection );
            genEmissionVelocity( particle->mDirection );
            particle->mTimeToLive = particle->mTotalTimeToLive = genEmissionTTL();
        }
    };

    class PointParticleEmitterFactory final : public Ogre::ParticleEmitterFactory
    {
    public:
        Ogre::String getName() const override
        {
            return "Point";
        }

        Ogre::ParticleEmitter *createEmitter( Ogre::ParticleSystem *particleSystem ) override
        {
            auto emitter = OGRE_NEW PointParticleEmitter( particleSystem );
            mEmitters.push_back( emitter );
            return emitter;
        }
    };
}  // namespace

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsSystemOgreNext, GraphicsSystem );
    WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsSystemOgreNext::GraphicsSystemEventListener,
                               IEventListener );

    CGraphicsSystemOgreNext::CGraphicsSystemOgreNext()
    {
        static const String graphicsSystemName = "CGraphicsSystemOgreNext";
        setName( graphicsSystemName );

        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        setupStateObject();
    }

    CGraphicsSystemOgreNext::~CGraphicsSystemOgreNext()
    {
        m_loadQueue.clear();
        m_unloadQueue.clear();
    }

    void CGraphicsSystemOgreNext::setupStateObject()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManagerPtr();
        WP_ASSERT( stateManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto stateContext = stateManager->addStateContext();
        stateContext->setOwner( this );
        setStateContext( stateContext );
        stateContext->setTaskId( TaskId::Render );

        auto stateListener = factoryManager->make_ptr<StateListener>();
        stateListener->setOwner( this );
        setStateListener( stateListener );
        stateContext->addStateListener( stateListener );
    }

    void CGraphicsSystemOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            ScopedLock lock( this, true );

            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto factoryManager = WPGraphicsOgreNext::getFactoryManager();
            setFactoryManager( factoryManager );

            m_eventListener = workphone::make_ptr<GraphicsSystemEventListener>();
            m_eventListener->setOwner( this );
            applicationManager->addObjectListener( m_eventListener );

            auto mediaPath = applicationManager->getMediaPath();

            static const String ogreNextFolder = "/OgreNext";
            const String ogreNextMediaFolder = mediaPath + ogreNextFolder;

            m_logListener = new CustomLogListener();

            applicationManager->setRenderMediaPath( ogreNextMediaFolder );

#if defined WP_PLATFORM_WIN32
            auto logPath = String( "Ogre.log" );
#elif defined WP_PLATFORM_APPLE
            auto macLogPath = macGetLogFilePath();
            auto logPath = macLogPath + String( "/Ogre.log" );
#elif defined WP_PLATFORM_IOS
            auto logPath = String( "Ogre.log" );
#else
            auto logPath = String( "Ogre.log" );
#endif

#ifdef WP_PLATFORM_WIN32
            Ogre::String resourcePath = "";
            Ogre::String pluginsPath;
            // only use plugins.cfg if not static
#    ifndef OGRE_STATIC_LIB
            pluginsPath = m_resourcePath + "plugins.cfg";
#    endif

            Ogre::String ogreConfigFileName = "ogre.cfg";
            Ogre::String configFilePath = m_resourcePath + "/" + ogreConfigFileName;

#    if 1
            Ogre::AbiCookie libCookie = Ogre::generateAbiCookie();
            m_root = new Ogre::Root( &libCookie, pluginsPath, configFilePath, logPath.c_str() );
#    else
            m_root = new Ogre::Root( pluginsPath, configFilePath, logPath );
#    endif

#elif defined( WP_PLATFORM_APPLE )
            Ogre::String resourcePath = "";
            Ogre::String pluginsPath;

            Ogre::String ogreConfigFileName = "ogre.cfg";
            Ogre::String configFilePath = macGetConfigFilePath() + "/" + ogreConfigFileName;

            Ogre::AbiCookie libCookie = Ogre::generateAbiCookie();
            m_root = new Ogre::Root( &libCookie, pluginsPath, configFilePath, logPath );
#elif defined( WP_PLATFORM_IOS )
            Ogre::String resourcePath = "";
            Ogre::String pluginsPath;

            Ogre::String ogreConfigFileName = "ogre.cfg";
            Ogre::String configFilePath = macGetConfigPath() + "/" + ogreConfigFileName;

            m_root = new Ogre::Root( pluginsPath, configFilePath, logPath );
#else
            Ogre::String resourcePath = "";
            Ogre::String pluginsPath;

            Ogre::String ogreConfigFileName = "ogre.cfg";
            Ogre::String configFilePath = ogreConfigFileName;

            Ogre::AbiCookie libCookie = Ogre::generateAbiCookie();
            m_root = new Ogre::Root( &libCookie, pluginsPath, configFilePath, logPath );
#endif

            auto logManager = Ogre::LogManager::getSingletonPtr();
            WP_ASSERT( logManager );

            auto defaultLog = logManager->getDefaultLog();
            WP_ASSERT( defaultLog );

            defaultLog->addListener( m_logListener );

            WP_ASSERT( applicationManager->isValid() );

            m_overlayMgr = workphone::make_ptr<COverlayManagerOgreNext>();
            loadObject( m_overlayMgr.load() );

            WP_ASSERT( applicationManager->isValid() );

            m_resGrpMgr = workphone::make_ptr<CResourceGroupManager>();
            WP_ASSERT( applicationManager->isValid() );

            auto materialManager = workphone::make_ptr<CMaterialManagerOgreNext>();
            WP_ASSERT( applicationManager->isValid() );
            loadObject( materialManager );
            m_materialManager = materialManager;

            auto textureManager = workphone::make_ptr<CTextureManagerOgreNext>();
            WP_ASSERT( applicationManager->isValid() );
            loadObject( textureManager );
            m_textureManager = textureManager;

            auto meshManager = workphone::make_ptr<MeshManager>();
            m_meshManager = meshManager;
            loadObject( meshManager );

            m_fontManager = workphone::make_ptr<CFontManagerOgreNext>();
            loadObject( m_fontManager.load() );

            WP_ASSERT( applicationManager->isValid() );

            auto compositorManager = workphone::make_ptr<CompositorManager>();

            m_compositorManager = compositorManager;

            // m_resourceGroupHelper = new ResourceGroupHelper;

            WP_ASSERT( applicationManager->isValid() );

            m_resourceLoadingListener = new ResourceLoadingListener;

            //auto meshConverter = workphone::make_ptr<MeshConverter>();
            //setMeshConverter( meshConverter );

            auto ogreResGrpMgr = Ogre::ResourceGroupManager::getSingletonPtr();
            WP_ASSERT( ogreResGrpMgr );

            ogreResGrpMgr->setLoadingListener( m_resourceLoadingListener );

            WP_ASSERT( applicationManager->isValid() );

            m_frameListener = new AppFrameListener( this );
            m_root->addFrameListener( m_frameListener );

            WP_ASSERT( applicationManager->isValid() );

#if WP_OGRE_USE_PARTICLE_UNIVERSE
            m_particlePlugin = new ParticleUniverse::ParticleUniversePlugin;
#endif

            m_uiRenderer = workphone::make_ptr<UIRenderer>();
            //m_uiRenderer->load( nullptr );
            loadObject( m_uiRenderer );

            // m_cellSceneManagerFactory = new CellSceneManagerFactory;
            // m_root->addSceneManagerFactory(m_cellSceneManagerFactory);

            // BasicSceneManagerFactory* m_basicSceneManagerFactory = new BasicSceneManagerFactory;
            // m_root->addSceneManagerFactory(m_basicSceneManagerFactory);

            WP_ASSERT( applicationManager->isValid() );

            auto debug = workphone::make_ptr<CDebugOgreNext>();
            setDebug( debug );

            auto meshConverter = workphone::make_ptr<MeshConverter>();
            meshConverter->load( nullptr );
            setMeshConverter( meshConverter );

            WP_ASSERT( applicationManager->isValid() );

            m_imguiManager = ImguiManagerOgreNext::getSingletonPtr();
            m_imguiManager->load( nullptr );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CGraphicsSystemOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            WP_ASSERT( isValid() );

            if( isLoaded() )
            {
                std::fprintf( stderr, "TRACE Ogre graphics unload start\n" );
                ScopedLock lock( this );

                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();

                while( !m_loadQueue.empty() )
                {
                    SmartPtr<ISharedObject> obj;
                    if( m_loadQueue.try_pop( obj ) )
                    {
                        obj = nullptr;
                    }
                }

                while( !m_unloadQueue.empty() )
                {
                    SmartPtr<ISharedObject> obj;
                    if( m_unloadQueue.try_pop( obj ) )
                    {
                        obj->unload( nullptr );
                    }
                }

                m_loadQueue.clear();
                m_unloadQueue.clear();
                std::fprintf( stderr, "TRACE Ogre graphics queues cleared\n" );

                if( auto debug = getDebug() )
                {
                    debug->unload( nullptr );
                    setDebug( nullptr );
                }
                std::fprintf( stderr, "TRACE Ogre debug unloaded\n" );

                if( auto overlayManager = getOverlayManager() )
                {
                    overlayManager->unload( nullptr );
                    m_overlayMgr = nullptr;
                }
                std::fprintf( stderr, "TRACE Ogre overlay manager unloaded\n" );

                if( auto renderUI = applicationManager->getRenderUI() )
                {
                    renderUI->unload( nullptr );
                    applicationManager->setRenderUI( nullptr );
                }
                std::fprintf( stderr, "TRACE Ogre render UI unloaded\n" );

                if( m_uiRenderer )
                {
                    m_uiRenderer->unload( nullptr );
                    m_uiRenderer = nullptr;
                }
                std::fprintf( stderr, "TRACE Ogre UI renderer unloaded\n" );

                if( auto fontManager = getFontManager() )
                {
                    fontManager->unload( nullptr );
                    setFontManager( nullptr );
                }
                std::fprintf( stderr, "TRACE Ogre font manager unloaded\n" );

                if( m_overlaySystem )
                {
                    OGRE_DELETE m_overlaySystem;
                    m_overlaySystem = nullptr;
                }
                std::fprintf( stderr, "TRACE Ogre overlay system unloaded\n" );

                // Scenes must be unloaded before the compositor manager, because
                // Terra/ShadowMapper objects hold compositor workspaces that need to
                // be deregistered (via removeWorkspace) while the compositor manager
                // is still alive. Unloading the compositor manager first clears all
                // workspaces, causing subsequent removeWorkspace calls from Terra
                // destructors to throw ERR_ITEM_NOT_FOUND and crash.
                for( auto scene : m_scenes )
                {
                    if( scene )
                    {
                        std::fprintf( stderr, "TRACE Ogre scene unload start %p\n", scene.get() );
                        scene->unload( nullptr );
                        std::fprintf( stderr, "TRACE Ogre scene unload end %p\n", scene.get() );
                    }
                }

                m_scenes.clear();

                if( auto compositorManager = getCompositorManager() )
                {
                    compositorManager->unload( nullptr );
                    setCompositorManager( nullptr );
                }

                for( auto window : m_windows )
                {
                    window->unload( nullptr );
                }

                m_windows.clear();
                m_defaultWindow = nullptr;

                if( auto materialManager = getMaterialManager() )
                {
                    materialManager->unload( nullptr );
                    m_materialManager = nullptr;
                }

                if( auto textureManager = getTextureManager() )
                {
                    textureManager->unload( nullptr );
                    m_textureManager = nullptr;
                }

                if( m_meshManager )
                {
                    m_meshManager->unload( nullptr );
                    m_meshManager = nullptr;
                }

                if( auto resourceGroupManager = getResourceGroupManager() )
                {
                    resourceGroupManager->unload( nullptr );
                    m_resGrpMgr = nullptr;
                }

                Ogre::HighLevelGpuProgramManager *gpuProgramManager =
                    Ogre::HighLevelGpuProgramManager::getSingletonPtr();
                if( gpuProgramManager )
                {
                    gpuProgramManager->removeAll();
                }

                if( m_root && m_frameListener )
                {
                    m_root->removeFrameListener( m_frameListener );
                }

                WP_SAFE_DELETE( m_frameListener );

                if( auto ogreResourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr() )
                {
                    ogreResourceGroupManager->setLoadingListener( nullptr );
                }
                WP_SAFE_DELETE( m_resourceLoadingListener );

                if( m_logListener )
                {
                    if( auto logManager = Ogre::LogManager::getSingletonPtr() )
                    {
                        if( auto defaultLog = logManager->getDefaultLog() )
                        {
                            defaultLog->removeListener( m_logListener );
                        }
                    }

                    delete m_logListener;
                    m_logListener = nullptr;
                }

                // auto renderSystem = m_root->getRenderSystem();
                // auto textureGpuManager = renderSystem->getTextureGpuManager();
                // if (textureGpuManager)
                //{
                //	textureGpuManager->shutdown();
                // }

                auto ogreResourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
                if( ogreResourceGroupManager )
                {
                    std::fprintf( stderr, "TRACE Ogre resource group shutdown start\n" );
                    ogreResourceGroupManager->shutdownAll();
                    std::fprintf( stderr, "TRACE Ogre resource group shutdown end\n" );
                }

                // m_root->removeSceneManagerFactory(m_cellSceneManagerFactory);

                //                    if( m_root )
                //                    {
                //#if WP_BUILD_RENDERER_GL3PLUS
                //                        if( mGL3PlusPlugin )
                //                        {
                //                            m_root->uninstallPlugin( mGL3PlusPlugin );
                //                            mGL3PlusPlugin = nullptr;
                //                        }
                //#endif
                //
                //#if WP_BUILD_RENDERER_GLES2
                //                        if( mGLES2Plugin )
                //                        {
                //                            m_root->uninstallPlugin( mGLES2Plugin );
                //                            mGLES2Plugin = nullptr;
                //                        }
                //#endif
                //
                //#if WP_BUILD_RENDERER_DX11
                //                        if( mD3D11PlusPlugin )
                //                        {
                //                            m_root->uninstallPlugin( mD3D11PlusPlugin );
                //                            mD3D11PlusPlugin = nullptr;
                //                        }
                //#endif
                //
                //#if WP_BUILD_RENDERER_METAL
                //                        if( mMetalPlugin )
                //                        {
                //                            m_root->uninstallPlugin( mMetalPlugin );
                //                            mMetalPlugin = nullptr;
                //                        }
                //#endif
                //                    }
                //
                //                    if( m_particlePlugin )
                //                    {
                //                        // m_particlePlugin->uninstall();
                //                        // m_particlePlugin->shutdown();
                //                        //WP_SAFE_DELETE( m_particlePlugin );
                //                    }

                // WP_SAFE_DELETE(m_cellSceneManagerFactory);

#if WP_GRAPHICS_SYSTEM_OGRENEXT
                if( m_imguiManager )
                {
                    m_imguiManager->shutdown();
                    delete m_imguiManager;
                    m_imguiManager = nullptr;
                }
#endif
                std::fprintf( stderr, "TRACE Ogre imgui stopped\n" );

                // Remove all GPU programs before the render system is destroyed.
                // Root::~Root() calls mHlmsManager->_changeRenderSystem(nullptr) internally
                // (after OGRE_DELETE mHlmsCompute) which releases GpuProgramPtr references held
                // in HLMS PSO caches. Calling _changeRenderSystem(nullptr) here — before
                // WP_SAFE_DELETE(m_root) — would null out HlmsManager::mRenderSystem prematurely,
                // causing HlmsComputeJob destructors (triggered by OGRE_DELETE mHlmsCompute inside
                // Root::~Root()) to crash when they call mRenderSystem->_hlmsSamplerblockDestroyed()
                // on the now-null pointer. Root handles the correct ordering internally.
                // D3D11HLSLProgram objects register themselves with D3D11DeviceResourceManager
                // on construction and only deregister in their destructor. unloadAll() alone is
                // not sufficient because it does not destroy the objects. removeAll() drops the
                // resource manager's references so the objects are actually destroyed, satisfying
                // the mResources.empty() assert in D3D11DeviceResourceManager::~D3D11DeviceResourceManager.
                if( Ogre::HighLevelGpuProgramManager::getSingletonPtr() )
                {
                    Ogre::HighLevelGpuProgramManager::getSingleton().unloadAll();
                    Ogre::HighLevelGpuProgramManager::getSingleton().removeAll();
                }
                if( Ogre::GpuProgramManager::getSingletonPtr() )
                {
                    Ogre::GpuProgramManager::getSingleton().unloadAll();
                    Ogre::GpuProgramManager::getSingleton().removeAll();
                }
                std::fprintf( stderr, "TRACE Ogre programs removed\n" );

#if WP_OGRE_USE_PARTICLE_UNIVERSE
                WP_SAFE_DELETE( m_particlePlugin );
#endif

                // Destroy Ogre::Root first so that Root::~Root() can call shutdown() ->
                // shutdownPlugins() and then unloadPlugins() with the full scene still intact.
                // This ensures CompositorManager2 (and its Rectangle2D objects whose VertexData
                // holds a raw mMgr pointer) is torn down BEFORE the render system plugin and its
                // HardwareBufferManager are destroyed. Uninstalling the plugin before deleting
                // Root would leave mMgr as a dangling pointer, causing a crash in
                // VertexData::~VertexData when it calls mMgr->destroyVertexBufferBinding().
                if( auto renderSystem = m_root ? m_root->getRenderSystem() : nullptr )
                {
                    renderSystem->_clearStateAndFlushCommandBuffer();
                    renderSystem->flushCommands();
                }

                std::fprintf( stderr, "TRACE Ogre root delete start\n" );
                WP_SAFE_DELETE( m_root );
                std::fprintf( stderr, "TRACE Ogre root delete end\n" );

                // After Root is gone the plugins have already been shut down and uninstalled by
                // Root::shutdown() -> shutdownPlugins() / Root::~Root() -> unloadPlugins().
                // All that remains is to free the plugin objects themselves.
                OGRE_DELETE m_pointEmitterFactory;
                m_pointEmitterFactory = nullptr;

#ifdef OGRE_STATIC_LIB
#    if WP_BUILD_RENDERER_GL3PLUS
                OGRE_DELETE mGL3PlusPlugin;
                mGL3PlusPlugin = nullptr;
#    endif

#    if WP_BUILD_RENDERER_GLES2
                OGRE_DELETE mGLES2Plugin;
                mGLES2Plugin = nullptr;
#    endif

#    if WP_BUILD_RENDERER_DX11
                OGRE_DELETE mD3D11PlusPlugin;
                mD3D11PlusPlugin = nullptr;
#    endif

#    if WP_BUILD_RENDERER_VULKAN
                OGRE_DELETE mVulkanPlugin;
                mVulkanPlugin = nullptr;
#    endif

#    if WP_BUILD_RENDERER_METAL
                OGRE_DELETE mMetalPlugin;
                mMetalPlugin = nullptr;
#    endif
#endif

                if( m_eventListener )
                {
                    applicationManager->removeObjectListener( m_eventListener );
                    m_eventListener = nullptr;
                }

                std::fprintf( stderr, "TRACE Ogre graphics base unload start\n" );
                GraphicsSystem::unload( data );
                std::fprintf( stderr, "TRACE Ogre graphics base unload end\n" );

                if( auto stateManager = applicationManager->getStateManager() )
                {
                    if( auto stateContext = getStateContext() )
                    {
                        if( auto stateListener = getStateListener() )
                        {
                            stateContext->removeStateListener( stateListener );
                            setStateListener( nullptr );
                        }

                        stateManager->removeStateContext( stateContext );
                        std::fprintf( stderr, "TRACE Ogre graphics context removed\n" );
                        setStateContext( nullptr );
                        std::fprintf( stderr, "TRACE Ogre graphics context cleared\n" );
                    }
                }

                setLoadingState( LoadingState::Unloaded );
                std::fprintf( stderr, "TRACE Ogre graphics unload end\n" );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> CGraphicsSystemOgreNext::getProperties() const
    {
        auto properties = GraphicsSystem::getProperties();
        return properties;
    }

    void CGraphicsSystemOgreNext::setProperties( SmartPtr<Properties> properties )
    {
        GraphicsSystem::setProperties( properties );
    }

    Array<SmartPtr<ISharedObject>> CGraphicsSystemOgreNext::getChildObjects() const
    {
        ScopedLock lock( this );

        Array<SmartPtr<ISharedObject>> objects;
        objects.reserve( 12 );

        objects.push_back( getCompositorManager() );
        objects.push_back( getOverlayManager() );
        objects.push_back( getResourceGroupManager() );
        objects.push_back( getMaterialManager() );
        objects.push_back( getTextureManager() );

        for( auto scene : m_scenes )
        {
            objects.push_back( scene );
        }

        return objects;
    }

    bool CGraphicsSystemOgreNext::isUpdating() const
    {
        return m_isUpdating;
    }

    void CGraphicsSystemOgreNext::setUpdating( bool updating )
    {
        m_isUpdating = updating;
    }

    bool CGraphicsSystemOgreNext::chooseRenderSystem()
    {
        try
        {
            ScopedLock lock( this );

            Ogre::RenderSystem *selectedRenderSystem = nullptr;

            // supported render system plugin names in Ogre
            const auto rsDX11Name = String( "Direct3D11 Rendering Subsystem" );
            const auto rsGL3Name = String( "OpenGL 3+ Rendering Subsystem" );
            const auto rsDX9Name = String( "Direct3D9 Rendering Subsystem" );
            const auto rsGLName = String( "OpenGL Rendering Subsystem" );

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            auto rsDX11 = m_root->getRenderSystemByName( rsDX11Name.c_str() );
            auto rsGL3 = m_root->getRenderSystemByName( rsGL3Name.c_str() );
            auto rsDX9 = m_root->getRenderSystemByName( rsDX9Name.c_str() );
            auto rsGL = m_root->getRenderSystemByName( rsGLName.c_str() );

            if( rsDX11 )
            {
                selectedRenderSystem = rsDX11;
            }
            else if( rsGL3 )
            {
                selectedRenderSystem = rsGL3;
            }
            else if( rsDX9 )
            {
                selectedRenderSystem = rsDX9;
            }
            else if( rsGL )
            {
                selectedRenderSystem = rsGL;
            }
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            auto renderers = m_root->getAvailableRenderers();
            for( auto renderer : renderers )
            {
                auto name = renderer->getName();
                static const auto preferred = String( "Metal" );
                if( name.find( preferred ) != String::npos )
                {
                    selectedRenderSystem = renderer;
                    break;
                }
            }

            if( !selectedRenderSystem )
            {
                if( !renderers.empty() )
                {
                    selectedRenderSystem = renderers[0];
                }
            }
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            auto renderers = m_root->getAvailableRenderers();
            for( auto renderer : renderers )
            {
                auto name = renderer->getName();
                static const auto preferred = String( "Metal" );
                if( name.find( preferred ) != String::npos )
                {
                    selectedRenderSystem = renderer;
                    break;
                }
            }

            if( !selectedRenderSystem )
            {
                if( !renderers.empty() )
                {
                    selectedRenderSystem = renderers[0];
                }
            }
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            auto renderers = m_root->getAvailableRenderers();
            for( auto renderer : renderers )
            {
                auto name = renderer->getName();
                static const auto preferred = String( "GL" );
                if( name.find( preferred ) != String::npos )
                {
                    selectedRenderSystem = renderer;
                    break;
                }
            }

            if( !selectedRenderSystem )
            {
                if( !renderers.empty() )
                {
                    selectedRenderSystem = renderers[0];
                }
            }
#else
#    pragma error "Unsupported platform!"
#endif

            if( selectedRenderSystem )
            {
                m_root->setRenderSystem( selectedRenderSystem );
            }
            else
            {
                // no supported render system found
                const String msg( "No supported render system found. Program will now exit." );
                MessageBoxUtil::show( msg.c_str() );
                return false;
            }

            // Pick first rendering device
            auto &configOptions = selectedRenderSystem->getConfigOptions();
            auto deviceIterator = configOptions.find( "Rendering Device" );
            if( deviceIterator != configOptions.end() )
            {
                const auto &deviceNames = deviceIterator->second.possibleValues;
                if( !deviceNames.empty() )
                {
                    const Ogre::String &deviceName = deviceNames.at( 0 );
                    selectedRenderSystem->setConfigOption( deviceIterator->first, deviceName );
                }
                else
                {
                    const String msg( "No render devices found. Exiting program." );
                    MessageBoxUtil::show( msg.c_str() );
                    return false;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return true;
    }

    void CGraphicsSystemOgreNext::setRenderSystemDefaults( Ogre::RenderSystem *renderSystem )
    {
        ScopedLock lock( this );

        if( !renderSystem )
        {
            MessageBoxUtil::show( "No render system" );
            return;
        }

        // int width = 800;//GetSystemMetrics(SM_CXSCREEN);
        // int height = 800;//GetSystemMetrics(SM_CYSCREEN);

        // choose suitable resolution
        Vector2I resolution( 1280, 720 );  // hack
        renderSystem->setConfigOption(
            "Video Mode", Ogre::StringConverter::toString( resolution.X() ) + Ogre::String( " x " ) +
                              Ogre::StringConverter::toString( resolution.Y() ) + " @ 32-bit colour" );

        renderSystem->setConfigOption( Ogre::String( "Full Screen" ), Ogre::String( "No" ) );

        // set anti alias defaults
        // renderSystem->setConfigOption(String("Anti aliasing"), String("None"));*/
    }

    bool CGraphicsSystemOgreNext::configure( SmartPtr<IBuildDirector> data )
    {
        try
        {
            ScopedLock lock( this );

            auto config = workphone::dynamic_pointer_cast<GraphicsSettings>( data );

            auto applicationManager = core::IApplicationManager::instance();

            WP_ASSERT( m_root );

            bool showDialog = false;
            bool applyDefaults = true;
            bool createWindow = true;

            if( config )
            {
                showDialog = config->getShowDialog();
                applyDefaults = true;
                createWindow = config->getCreateWindow();
            }

            installPlugins( m_root );

            // Show the configuration dialog and initialise the system
            // You can skip this and use root.restoreConfig() to load configuration
            // settings if you were sure there are valid ones saved in ogre.cfg
            /*
            if (m_root->restoreConfig())
            {
                //validate config options
                auto renderSystem = m_root->getRenderSystem();
                if (renderSystem)
                {
                    String errStr = renderSystem->validateConfigOptions();
                    if (errStr.size() > 0)
                    {
                        setRenderSystemDefaults(renderSystem);

                        //show config dialog
                        if (!m_root->showConfigDialog())
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    //set defaults
                    chooseRenderSystem();
                    auto renderSystem = m_root->getRenderSystem();
                    setRenderSystemDefaults(renderSystem);

                    //show config dialog
                    if (!m_root->showConfigDialog())
                    {
                        return false;
                    }
                }
            }
            else
                */
            {
                // set defaults
                chooseRenderSystem();
                auto renderSystem = m_root->getRenderSystem();
                setRenderSystemDefaults( renderSystem );

                // show config dialog
                if( showDialog && !m_root->showConfigDialog() )
                {
                    return false;
                }
            }

#if 0
	            auto ogreWindow = m_root->initialise( createWindow );
	            if( ogreWindow )
	            {
	                auto window = fb::make_ptr<CWindowOgreNext>();
	
	                auto windowHandle = window->getHandle();
	                windowHandle->setName( "DefaultWindow" );
	
	                window->initialise( ogreWindow );
	
	                setDefaultWindow( window );
	                m_windows.push_back( window );
	            }
#else
            m_root->initialise( false );

            if( createWindow )
            {
                auto window = workphone::make_ptr<CWindowOgreNext>();

                window->setName( "DefaultWindow" );

                // window->initialise( ogreWindow );
                window->load( nullptr );

                setDefaultWindow( window );
                m_windows.emplace_back( window );
            }
#endif

            m_overlaySystem = OGRE_NEW Ogre::v1::OverlaySystem();

            auto debug = getDebug();
            if( debug )
            {
                debug->load( nullptr );
            }

            if( config && config->getCreateRenderUI() )
            {
                if( auto renderUI = applicationManager->getRenderUI() )
                {
                    loadObject( renderUI );
                }
            }

            return true;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

    SmartPtr<IGraphicsWindow> CGraphicsSystemOgreNext::initialise(
        bool autoCreateWindow, const String &windowTitle, const String &customCapabilitiesConfig )
    {
        ScopedLock lock( this );

        WP_ASSERT( m_root );

        auto window = workphone::make_ptr<CWindowOgreNext>();

        window->setName( "DefaultWindow" );

        auto renderSystem = m_root->getRenderSystem();
        WP_ASSERT( renderSystem );

        auto ogreWindow = m_root->initialise( autoCreateWindow );
        window->setupWindow( ogreWindow );

        return window;
    }

    void CGraphicsSystemOgreNext::installPlugins( Ogre::Root *root )
    {
        ScopedLock lock( this );

#ifdef OGRE_STATIC_LIB
#    ifdef OGRE_BUILD_RENDERSYSTEM_METAL
#        if WP_BUILD_RENDERER_METAL
        if( !mMetalPlugin )
        {
            mMetalPlugin = OGRE_NEW Ogre::MetalPlugin();
        }

        root->installPlugin( mMetalPlugin, nullptr );
#        endif
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_GL3PLUS
#        if WP_BUILD_RENDERER_GL3PLUS
        if( !mGL3PlusPlugin )
        {
            mGL3PlusPlugin = OGRE_NEW Ogre::GL3PlusPlugin();
        }

        root->installPlugin( mGL3PlusPlugin, nullptr );
#        endif
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_GLES2
        if( !mGLES2Plugin )
            mGLES2Plugin = OGRE_NEW Ogre::GLES2Plugin();
        root->installPlugin( mGLES2Plugin );
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_D3D11
#        if WP_BUILD_RENDERER_DX11
        if( !mD3D11PlusPlugin )
        {
            mD3D11PlusPlugin = OGRE_NEW Ogre::D3D11Plugin();
        }

        root->installPlugin( mD3D11PlusPlugin, nullptr );
#        endif
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_VULKAN
#        if WP_BUILD_RENDERER_VULKAN
        if( !mVulkanPlugin )
        {
            mVulkanPlugin = OGRE_NEW Ogre::VulkanPlugin();
        }

        root->installPlugin( mVulkanPlugin, nullptr );
#        endif
#    endif

#endif

        // ParticleSystem is part of OgreMain, while concrete emitter types normally
        // come from the optional ParticleFX plugin. Register the point emitter used
        // by code-created engine particle systems when that plugin is unavailable.
        if( !m_pointEmitterFactory )
        {
            m_pointEmitterFactory = OGRE_NEW PointParticleEmitterFactory();
            Ogre::ParticleSystemManager::getSingleton().addEmitterFactory( m_pointEmitterFactory );
        }
    }

    SmartPtr<IFontManager> CGraphicsSystemOgreNext::getFontManager() const
    {
        return m_fontManager;
    }

    void CGraphicsSystemOgreNext::setFontManager( SmartPtr<IFontManager> fontManager )
    {
        m_fontManager = fontManager;
    }

    SmartPtr<CompositorManager> CGraphicsSystemOgreNext::getCompositorManager() const
    {
        return m_compositorManager;
    }

    void CGraphicsSystemOgreNext::setCompositorManager( SmartPtr<CompositorManager> compositorManager )
    {
        m_compositorManager = compositorManager;
    }

    SmartPtr<IResourceGroupManager> CGraphicsSystemOgreNext::getResourceGroupManager() const
    {
        return m_resGrpMgr;
    }

    SmartPtr<IGraphicsWindow> CGraphicsSystemOgreNext::createRenderWindow(
        const String &name, u32 width, u32 height, bool fullScreen,
        const SmartPtr<Properties> &properties )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = WPGraphicsOgreNext::getFactoryManager();

        auto window = factoryManager->make_ptr<CWindowOgreNext>();

        window->setName( name );
        window->setTitle( name );

        window->setColourDepth( 32 );

        auto windowSize = Vector2I( static_cast<s32>( width ), static_cast<s32>( height ) );
        window->setSize( windowSize );

        window->setFullscreen( fullScreen );

        m_windows.emplace_back( window );

        String handle;

        if( properties )
        {
            static const String windowHandleStr = "WindowHandle";
            if( properties->getPropertyValue( windowHandleStr, handle ) )
            {
                window->setWindowHandleAsString( handle );
            }
        }

        auto defaultWindow = getDefaultWindow();
        if( defaultWindow == nullptr )
        {
            setDefaultWindow( window );
        }

        window->load( nullptr );
        // loadObject( window );

        return window;
    }

    SmartPtr<IGraphicsWindow> CGraphicsSystemOgreNext::getRenderWindow( const String &name ) const
    {
        for( auto window : m_windows )
        {
            if( window->getNamePtr() == name )
            {
                return window;
            }
        }

        return nullptr;
    }

    s32 CGraphicsSystemOgreNext::getLoadPriority( SmartPtr<ISharedObject> obj )
    {
        if( obj )
        {
            if( obj->isDerived<IGraphicsWindow>() )
            {
                return 100000;
            }
            if( obj->isDerived<IResourceGroupManager>() )
            {
                return 95000;
            }
            if( obj->isDerived<IResourceManager>() )
            {
                return 94500;
            }
            if( obj->isDerived<UIRenderer>() )
            {
                return 94300;
            }
            if( obj->isDerived<ui::IUIManager>() )
            {
                return 94250;
            }
            if( obj->isDerived<CompositorManager>() )
            {
                return 94000;
            }
            if( obj->isDerived<Compositor>() )
            {
                return 93000;
            }
            if( obj->isDerived<IGraphicsScene>() )
            {
                return 90000;
            }
            if( obj->isDerived<IGraphicsSceneNode>() )
            {
                return 85000;
            }
            if( obj->isDerived<IGraphicsCamera>() )
            {
                return 80000;
            }
            if( obj->isDerived<IMaterial>() )
            {
                return 78000;
            }
            if( obj->isDerived<CompositorManager>() )
            {
                return 75000;
            }
            if( obj->isDerived<IGraphicsMesh>() )
            {
                return 70000;
            }
            if( obj->isDerived<IGraphicsLight>() )
            {
                return 65000;
            }
            if( obj->isDerived<IOverlayElement>() )
            {
                return 1000;
            }
            if( obj->isDerived<ui::IUIManager>() )
            {
                return 1000;
            }
        }

        return 0;
    }

    void CGraphicsSystemOgreNext::update()
    {
        try
        {
            ScopedLoadLock loadLock( this );

            TryLockGuard lock( this );
            if( lock.locked() && loadLock.isLoaded() )
            {
                setUpdating( true );

                if( auto compositorManager = getCompositorManagerPtr() )
                {
                    if( !compositorManager->isLoaded() )
                    {
                        compositorManager->load( nullptr );
                    }
                }

                WP_ASSERT( getResourceGroupManager() );
                WP_ASSERT( getResourceGroupManager() && getResourceGroupManager()->isLoaded() );

                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                const auto timer = applicationManager->getTimerPtr();
                WP_ASSERT( timer );

                auto dt = timer->getDeltaTime();
                WP_ASSERT( dt > 0.0 );
                WP_ASSERT( MathD::isFinite( dt ) );

                for( auto &window : m_windows )
                {
                    if( window )
                    {
                        window->update();
                    }
                }

                for( auto &scene : m_scenes )
                {
                    if( scene )
                    {
                        scene->update();
                    }
                }

                if( auto overlayManager = getOverlayManagerPtr() )
                {
                    overlayManager->update();
                }

                if( auto ui = applicationManager->getRenderUIPtr() )
                {
                    ui->update();
                }

                if( !m_loadQueue.empty() )
                {
                    auto loadArray = Array<SmartPtr<ISharedObject>>();
                    loadArray.reserve( 256 );

                    SmartPtr<ISharedObject> obj;
                    while( m_loadQueue.try_pop( obj ) )
                    {
                        loadArray.push_back( obj );
                    }

                    std::sort( loadArray.begin(), loadArray.end(),
                               []( const SmartPtr<ISharedObject> &a,
                                   const SmartPtr<ISharedObject> &b ) -> bool {
                                   auto applicationManager = core::IApplicationManager::instancePtr();
                                   auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

                                   auto aPriority = graphicsSystem->getLoadPriority( a );
                                   auto bPriority = graphicsSystem->getLoadPriority( b );

                                   return aPriority > bPriority;
                               } );

                    for( auto loadObj : loadArray )
                    {
                        if( loadObj )
                        {
                            auto loadingState = loadObj->getLoadingState();
                            if( loadingState != LoadingState::Loaded &&
                                loadingState != LoadingState::Unloaded )
                            {
                                loadObj->load( nullptr );
                            }
                        }
                    }
                }

                if( !m_reloadQueue.empty() )
                {
                    SmartPtr<ISharedObject> obj;
                    while( m_reloadQueue.try_pop( obj ) )
                    {
                        obj->reload( nullptr );
                    }
                }

                if( !m_unloadQueue.empty() )
                {
                    auto unloadArray = Array<SmartPtr<ISharedObject>>();
                    unloadArray.reserve( 256 );

                    SmartPtr<ISharedObject> obj;
                    while( m_unloadQueue.try_pop( obj ) )
                    {
                        unloadArray.push_back( obj );
                    }

                    for( auto loadObj : unloadArray )
                    {
                        loadObj->unload( nullptr );
                    }
                }

                WP_ASSERT( getResourceGroupManager() );
                WP_ASSERT( getResourceGroupManager() && getResourceGroupManager()->isLoaded() );

                auto fDT = static_cast<f32>( dt );
                m_root->renderOneFrame( fDT );

                if( auto debug = getDebugPtr() )
                {
                    debug->preUpdate();
                    debug->update();
                    debug->postUpdate();
                }

                for( auto &scene : m_scenes )
                {
                    if( scene )
                    {
                        scene->postUpdate();
                    }
                }

                if( auto textureManager = getTextureManagerPtr() )
                {
                    textureManager->update();
                }

                setUpdating( false );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    IMaterialManager *CGraphicsSystemOgreNext::getMaterialManagerPtr() const
    {
        return m_materialManager.get();
    }

    SmartPtr<IMaterialManager> CGraphicsSystemOgreNext::getMaterialManager() const
    {
        return m_materialManager;
    }

    SmartPtr<IInstanceManager> CGraphicsSystemOgreNext::getInstanceManager() const
    {
        return m_instanceManager;
    }

    SmartPtr<IGraphicsWindow> CGraphicsSystemOgreNext::getDefaultWindow() const
    {
        return m_defaultWindow;
    }

    void CGraphicsSystemOgreNext::setDefaultWindow( SmartPtr<IGraphicsWindow> defaultWindow )
    {
        m_defaultWindow = defaultWindow;
    }

    void CGraphicsSystemOgreNext::messagePump()
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        // Windows Message Loop (NULL means check all HWNDs belonging to this context)
        MSG msg;
        while( PeekMessage( &msg, nullptr, 0U, 0U, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
#    ifndef OGRE_CONFIG_UNIX_NO_X11
        // GLX Message Pump

        xcb_connection_t *xcbConnection = 0;

        if( !_msWindows.empty() )
            _msWindows.front()->getCustomAttribute( "xcb_connection_t", &xcbConnection );

        if( !xcbConnection )
        {
            // Uses the older Xlib
            auto win = _msWindows.begin();
            auto end = _msWindows.end();

            Display *xDisplay = 0;  // same for all windows

            for( ; win != end; win++ )
            {
                XID xid;
                XEvent event;

                if( !xDisplay )
                    ( *win )->getCustomAttribute( "XDISPLAY", &xDisplay );

                ( *win )->getCustomAttribute( "WINDOW", &xid );

                while( XCheckWindowEvent( xDisplay, xid,
                                          StructureNotifyMask | VisibilityChangeMask | FocusChangeMask,
                                          &event ) )
                {
                    // GLXProc( *win, event );
                }

                // The ClientMessage event does not appear under any Event Mask
                while( XCheckTypedWindowEvent( xDisplay, xid, ClientMessage, &event ) )
                {
                    // GLXProc( *win, event );
                }
            }
        }
        else
        {
            // Uses the newer xcb
            xcb_generic_event_t *nextEvent = 0;

            nextEvent = xcb_poll_for_event( xcbConnection );

            while( nextEvent )
            {
                // XcbProc( xcbConnection, nextEvent );
                free( nextEvent );
                nextEvent = xcb_poll_for_event( xcbConnection );
            }
        }
#    endif
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE && !defined __OBJC__ && !defined __LP64__
        // OSX Message Pump
        EventRef event = NULL;
        EventTargetRef targetWindow;
        targetWindow = GetEventDispatcherTarget();

        // If we are unable to get the target then we no longer care about events.
        if( !targetWindow )
            return;

        // Grab the next event, process it if it is a window event
        while( ReceiveNextEvent( 0, NULL, kEventDurationNoWait, true, &event ) == noErr )
        {
            // Dispatch the event
            SendEventToEventTarget( event, targetWindow );
            ReleaseEvent( event );
        }
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        mac_dispatchOneEvent();
#endif
    }

    void CGraphicsSystemOgreNext::setupRenderer( SmartPtr<IGraphicsScene> sceneManager,
                                                 SmartPtr<IGraphicsWindow> window,
                                                 SmartPtr<IGraphicsCamera> camera, String workspaceName,
                                                 bool enabled )
    {
        //auto compositorManager = getCompositorManager();
        //WP_ASSERT( compositorManager );

        //compositorManager->setSceneManager( sceneManager );
        //compositorManager->setWindow( window );
        //compositorManager->setCamera( camera );
        ////compositorManager->setWorkspaceName( workspaceName );
        ////compositorManager->setEnabled( enabled );
        //loadObject( compositorManager );
    }

    Ogre::Terra *CGraphicsSystemOgreNext::getTerra() const
    {
        return m_terra;
    }

    void CGraphicsSystemOgreNext::setTerra( Ogre::Terra *terra )
    {
        m_terra = terra;
    }

    IGraphicsSystem::RenderApi CGraphicsSystemOgreNext::getRenderApi() const
    {
        return m_renderApi;
    }

    void CGraphicsSystemOgreNext::setRenderApi( RenderApi renderApi )
    {
        m_renderApi = renderApi;
    }

    SmartPtr<IMeshConverter> CGraphicsSystemOgreNext::getMeshConverter() const
    {
        return m_meshConverter;
    }

    void CGraphicsSystemOgreNext::loadObject( SmartPtr<ISharedObject> graphicsObject, bool forceQueue )
    {
        auto loadingState = graphicsObject->getLoadingState();
        if( loadingState == LoadingState::LoadingQueued )
        {
            return;
        }

        if( loadingState == LoadingState::Loading )
        {
            return;
        }

        if( loadingState == LoadingState::Loaded )
        {
            return;
        }

        graphicsObject->setLoadingState( LoadingState::LoadingQueued );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( applicationManager->isRunning() )
        {
            if( auto taskManager = applicationManager->getTaskManagerPtr() )
            {
                auto renderTask = taskManager->getTaskPtr( TaskId::Render );

                if( forceQueue || ( renderTask && renderTask->isExecuting() ) )
                {
                    const auto &graphicsObjectLoadingState = graphicsObject->getLoadingState();
                    if( !( graphicsObjectLoadingState == LoadingState::Loading ||
                           graphicsObjectLoadingState == LoadingState::Loaded ) )
                    {
                        m_loadQueue.push( graphicsObject );
                    }
                }
                else
                {
                    auto stateTask = getStateTask();
                    auto task = Thread::getCurrentTask();

                    if( task != stateTask )
                    {
                        const auto &graphicsObjectLoadingState = graphicsObject->getLoadingState();
                        if( !( graphicsObjectLoadingState == LoadingState::Loading ||
                               graphicsObjectLoadingState == LoadingState::Loaded ) )
                        {
                            m_loadQueue.push( graphicsObject );
                        }
                    }
                    else
                    {
                        ScopedLock lock( this );

                        if( !graphicsObject->isLoaded() )
                        {
                            graphicsObject->load( nullptr );
                        }
                    }
                }
            }
            else
            {
                ScopedLock lock( this );

                if( !graphicsObject->isLoaded() )
                {
                    graphicsObject->load( nullptr );
                }
            }
        }
    }

    void CGraphicsSystemOgreNext::unloadObject( SmartPtr<ISharedObject> graphicsObject, bool forceQueue )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto threadPool = applicationManager->getThreadPoolPtr();

        if( threadPool && threadPool->getNumThreads() > 0 )
        {
            if( !forceQueue )
            {
                if( auto taskManager = applicationManager->getTaskManagerPtr() )
                {
                    auto renderTask = taskManager->getTaskPtr( TaskId::Render );
                    if( renderTask && !renderTask->isExecuting() )
                    {
                        ScopedLock lock( this );
                        graphicsObject->unload( nullptr );
                        return;
                    }
                }
            }

            if( graphicsObject )
            {
                const auto loadingState = graphicsObject->getLoadingState();
                if( loadingState == LoadingState::Loaded )
                {
                    if( applicationManager->getQuit() )
                    {
                        ScopedLock lock( this );
                        graphicsObject->unload( nullptr );
                    }
                    else
                    {
                        auto stateTask = getStateTask();
                        if( forceQueue || stateTask != TaskId::Render ||
                            Thread::getTaskFlag( Thread::Render_Flag ) )
                        {
                            const auto &loadingState = graphicsObject->getLoadingState();
                            if( loadingState != LoadingState::Unloaded )
                            {
                                m_unloadQueue.push( graphicsObject );
                            }
                        }
                        else
                        {
                            ScopedLock lock( this );
                            graphicsObject->unload( nullptr );
                        }
                    }
                }
                else
                {
                    ScopedLock lock( this );
                    graphicsObject->unload( nullptr );
                }
            }
        }
        else
        {
            if( graphicsObject )
            {
                ScopedLock lock( this );
                graphicsObject->unload( nullptr );
            }
        }
    }

    Ogre::v1::OverlaySystem *CGraphicsSystemOgreNext::getOverlaySystem() const
    {
        return m_overlaySystem;
    }

    void CGraphicsSystemOgreNext::setOverlaySystem( Ogre::v1::OverlaySystem *overlaySystem )
    {
        m_overlaySystem = overlaySystem;
    }

    void CGraphicsSystemOgreNext::setMeshConverter( SmartPtr<IMeshConverter> meshConverter )
    {
        m_meshConverter = meshConverter;
    }

    bool CGraphicsSystemOgreNext::isValid() const
    {
        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded )
        {
            auto root = Ogre::Root::getSingletonPtr();
            auto hlmsManager = root->getHlmsManager();

            Ogre::HlmsUnlit *hlmsUnlit = (Ogre::HlmsUnlit *)hlmsManager->getHlms( Ogre::HLMS_UNLIT );
            Ogre::HlmsPbs *hlmsPbs = (Ogre::HlmsPbs *)hlmsManager->getHlms( Ogre::HLMS_PBS );

            if( !hlmsUnlit || !hlmsPbs )
            {
                return false;
            }

            return root != nullptr;
        }

        return true;
    }

    TaskId CGraphicsSystemOgreNext::getStateTask() const
    {
        return TaskId::Render;
    }

    TaskId CGraphicsSystemOgreNext::getRenderTask() const
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( applicationManager->isLoading() )
        {
            return TaskId::Primary;
        }

        // auto threadPool = applicationManager->getThreadPool();

        // if (isUpdating())
        //{
        //	return Thread::getCurrentTask();
        // }

        auto taskManager = applicationManager->getTaskManager();
        if( taskManager )
        {
            auto task = taskManager->getTask( TaskId::Render );
            if( task )
            {
                if( task->isExecuting() )
                {
                    return TaskId::Render;
                }
                if( task->isPrimary() )
                {
                    return TaskId::Primary;
                }
            }
        }

        return applicationManager->hasTasks() ? TaskId::Render : TaskId::Primary;
    }

    Array<SmartPtr<IGraphicsWindow>> CGraphicsSystemOgreNext::getWindows() const
    {
        return m_windows.snapshot();
    }

    bool CGraphicsSystemOgreNext::AppFrameListener::frameStarted( const Ogre::FrameEvent &evt )
    {
        //if( m_graphicsSystem->m_uiRenderer )
        //{
        //    m_graphicsSystem->m_uiRenderer->render();
        //}

        return true;
    }

    bool CGraphicsSystemOgreNext::AppFrameListener::frameRenderingQueued( const Ogre::FrameEvent &evt )
    {
        return true;
    }

    bool CGraphicsSystemOgreNext::AppFrameListener::frameEnded( const Ogre::FrameEvent &evt )
    {
        //if( m_graphicsSystem->m_uiRenderer )
        //{
        //    m_graphicsSystem->m_uiRenderer->render();
        //}

        return true;
    }

    CGraphicsSystemOgreNext::AppFrameListener::~AppFrameListener() = default;

    CGraphicsSystemOgreNext::AppFrameListener::AppFrameListener(
        CGraphicsSystemOgreNext *graphicsSystem ) :
        m_graphicsSystem( graphicsSystem ),
        m_time( 0.0f )
    {
    }

    void CGraphicsSystemOgreNext::createDebugTextOverlay()
    {
        // Ogre::v1::OverlayManager& overlayManager = Ogre::v1::OverlayManager::getSingleton();
        // Ogre::v1::Overlay* overlay = overlayManager.create("DebugText");

        // Ogre::v1::OverlayContainer* panel = static_cast<Ogre::v1::OverlayContainer*>(
        //	overlayManager.createOverlayElement("Panel", "DebugPanel"));
        // mDebugText = static_cast<Ogre::v1::TextAreaOverlayElement*>(
        //	overlayManager.createOverlayElement("TextArea", "DebugText"));
        // mDebugText->setFontName("DebugFont");
        // mDebugText->setCharHeight(0.025f);

        // mDebugTextShadow = static_cast<Ogre::v1::TextAreaOverlayElement*>(
        //	overlayManager.createOverlayElement("TextArea", "0DebugTextShadow"));
        // mDebugTextShadow->setFontName("DebugFont");
        // mDebugTextShadow->setCharHeight(0.025f);
        // mDebugTextShadow->setColour(Ogre::ColourValue::Black);
        // mDebugTextShadow->setPosition(0.002f, 0.002f);

        // panel->addChild(mDebugTextShadow);
        // panel->addChild(mDebugText);
        // overlay->add2D(panel);
        // overlay->show();

        // auto overlayManager = Ogre::v1::OverlayManager::getSingletonPtr();
        // auto overlay = overlayManager->create("Test");

        // auto panel = (Ogre::v1::OverlayContainer*)overlayManager->createOverlayElement("Panel",
        // "Panel");
        ////panel->setVisible(true);
        ////panel->setMaterialName("DebugCube");
        // overlay->add2D(panel);

        // auto text =
        // (Ogre::v1::TextAreaOverlayElement*)overlayManager->createOverlayElement("TextArea",
        // "TextArea"); text->setMetricsMode(Ogre::v1::GuiMetricsMode::GMM_PIXELS);
        // text->setHorizontalAlignment(Ogre::v1::GuiHorizontalAlignment::GHA_CENTER);
        // text->setVerticalAlignment(Ogre::v1::GuiVerticalAlignment::GVA_CENTER);
        // text->setFontName("DebugFont");
        // text->setCharHeight(18);
        // text->setCaption("Test");
        // text->setSpaceWidth(9);
        // text->setColour(Ogre::ColourValue::White);
        // panel->addChild(text);

        ////auto textName = String("test");
        ////auto mElement =
        /// Ogre::v1::OverlayManager::getSingleton().createOverlayElementFromTemplate("SdkTrays/Label",
        ///"BorderPanel", textName); /auto mTextArea =
        ///(Ogre::v1::TextAreaOverlayElement*)((Ogre::v1::OverlayContainer*)mElement)->getChild(textName
        ///+ "/LabelCaption"); /mTextArea->setCaption("Test2"); /panel->addChild(mTextArea);

        // overlay->show();
    }

    void CGraphicsSystemOgreNext::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        const Ogre::FrameStats *frameStats = m_root->getFrameStats();

        Ogre::String finalText;
        finalText.reserve( 128 );
        finalText = "Frame time:\t";
        finalText += Ogre::StringConverter::toString( timeSinceLast * 1000.0f );
        finalText += " ms\n";
        finalText += "Frame FPS:\t";
        finalText += Ogre::StringConverter::toString( 1.0f / timeSinceLast );
        finalText += "\nAvg time:\t";
        finalText += Ogre::StringConverter::toString( frameStats->getAvgTime() );
        finalText += " ms\n";
        finalText += "Avg FPS:\t";
        finalText += Ogre::StringConverter::toString( 1000.0f / frameStats->getAvgTime() );
        finalText += "\n\nPress F1 to toggle help";

        outText.swap( finalText );

        if( m_debugText )
        {
            m_debugText->setCaption( finalText.c_str() );
        }

        if( m_debugTextShadow )
        {
            m_debugTextShadow->setCaption( finalText.c_str() );
        }
    }

    bool CGraphicsSystemOgreNext::WindowEventListener::windowClosing( Ogre::RenderWindow *rw )
    {
        auto applicationManager = core::IApplicationManager::instance();
        applicationManager->setRunning( false );
        return false;
    }

    CGraphicsSystemOgreNext::WindowEventListener::~WindowEventListener() = default;

    CGraphicsSystemOgreNext::WindowEventListener::WindowEventListener() = default;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

    LRESULT CALLBACK CGraphicsSystemOgreNext::_WndProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                                        LPARAM lParam )
    {
        using namespace Ogre;

        auto applicationManager = core::IApplicationManager::instance();
        auto ui = applicationManager->getUI();

        auto data = workphone::make_ptr<WindowMessageData>();
        data->setWindowHandle( hWnd );
        data->setMessage( uMsg );
        data->setWParam( wParam );
        data->setLParam( lParam );

        if( ui )
        {
            ui->messagePump( data );
        }

        if( uMsg == WM_CREATE )
        {
            // Store pointer to Win32Window in user data area
            SetWindowLongPtr( hWnd, GWLP_USERDATA,
                              (LONG_PTR)( ( (LPCREATESTRUCT)lParam )->lpCreateParams ) );
            return 0;
        }

        // look up window instance
        // note: it is possible to get a WM_SIZE before WM_CREATE
        auto win = (Ogre::Window *)GetWindowLongPtr( hWnd, GWLP_USERDATA );
        if( !win )
        {
            return DefWindowProc( hWnd, uMsg, wParam, lParam );
        }

        // LogManager* log = LogManager::getSingletonPtr();
        // Iterator of all listeners registered to this Window
        auto listeners = WindowEventUtilities::_msListeners;

        multimap<Ogre::Window *, Ogre::WindowEventListener *>::type::iterator index;

        auto start = listeners.lower_bound( win );
        auto end = listeners.upper_bound( win );

        switch( uMsg )
        {
        case WM_ACTIVATE:
        {
            bool active = ( LOWORD( wParam ) != WA_INACTIVE );
            win->setFocused( active );

            for( ; start != end; ++start )
            {
                ( start->second )->windowFocusChange( win );
            }
        }
        break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint( hWnd, &ps );
            win->_setVisible( !IsRectEmpty( &ps.rcPaint ) );
            EndPaint( hWnd, &ps );
        }
        break;
        case WM_SYSKEYDOWN:
        {
            switch( wParam )
            {
            case VK_CONTROL:
            case VK_SHIFT:
            case VK_MENU:  // ALT
                // return zero to bypass defProc and signal we processed the message
                return 0;
            }
        }
        break;
        case WM_SYSKEYUP:
        {
            switch( wParam )
            {
            case VK_CONTROL:
            case VK_SHIFT:
            case VK_MENU:  // ALT
            case VK_F10:
                // return zero to bypass defProc and signal we processed the message
                return 0;
            }
        }
        break;
        case WM_SYSCHAR:
        {
            // return zero to bypass defProc and signal we processed the message, unless it's an
            // ALT-space
            if( wParam != VK_SPACE )
            {
                return 0;
            }
        }
        break;
        case WM_ENTERSIZEMOVE:
            // log->logMessage("WM_ENTERSIZEMOVE");
            break;
        case WM_EXITSIZEMOVE:
            // log->logMessage("WM_EXITSIZEMOVE");
            break;
        case WM_MOVE:
        {
            // log->logMessage("WM_MOVE");
            win->windowMovedOrResized();

            for( auto listener : listeners )
            {
                ( listener.second )->windowMoved( win );
            }
        }
        break;
        case WM_DISPLAYCHANGE:
        {
            win->windowMovedOrResized();

            for( auto listener : listeners )
            {
                ( listener.second )->windowResized( win );
            }
        }
        break;
        case WM_SIZE:
        {
            // log->logMessage("WM_SIZE");
            win->windowMovedOrResized();

            for( auto listener : listeners )
            {
                ( listener.second )->windowResized( win );
            }
        }
        break;
        case WM_GETMINMAXINFO:
        {
            // Prevent the window from going smaller than some minimu size
            ( (MINMAXINFO *)lParam )->ptMinTrackSize.x = 100;
            ( (MINMAXINFO *)lParam )->ptMinTrackSize.y = 100;
        }
        break;
        case WM_CLOSE:
        {
            // log->logMessage("WM_CLOSE");
            bool close = true;

            for( auto listener : listeners )
            {
                if( !( listener.second )->windowClosing( win ) )
                {
                    close = false;
                }
            }

            if( !close )
            {
                return 0;
            }

            for( index = listeners.lower_bound( win ); index != end; ++index )
            {
                ( index->second )->windowClosed( win );
            }

            win->destroy();
            return 0;
        }
        }

        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    void CGraphicsSystemOgreNext::render()
    {
        if( auto imgui = ImguiManagerOgreNext::getSingletonPtr() )
        {
            imgui->newFrame();

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            if( auto ui = applicationManager->getUI() )
            {
                if( auto application = ui->getApplication() )
                {
                    application->update();
                }
            }

            imgui->render();
        }

        if( m_uiRenderer )
        {
            m_uiRenderer->beginFrame();
            m_uiRenderer->render();
            m_uiRenderer->endFrame();
        }
    }

    void CGraphicsSystemOgreNext::setUIRenderer( SmartPtr<UIRenderer> uiRenderer )
    {
        m_uiRenderer = uiRenderer;
    }

    SmartPtr<UIRenderer> CGraphicsSystemOgreNext::getUIRenderer() const
    {
        return m_uiRenderer;
    }

#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
    //
    void GLXProc( Ogre::Window *win, const XEvent &event )
    {
        // An iterator for the window listeners
        Ogre::WindowEventUtilities::WindowEventListeners::iterator index,
            start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win ),
            end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );

        switch( event.type )
        {
        case ClientMessage:
        {
            ::Atom atom;
            win->getCustomAttribute( "ATOM", &atom );
            if( event.xclient.format == 32 && event.xclient.data.l[0] == (long)atom )
            {  // Window closed by window manager
                // Send message first, to allow app chance to unregister things that need done before
                // window is shutdown
                bool close = true;
                for( index = start; index != end; ++index )
                {
                    if( !( index->second )->windowClosing( win ) )
                        close = false;
                }
                if( !close )
                    return;

                for( index = start; index != end; ++index )
                    ( index->second )->windowClosed( win );
                win->destroy();
            }
            break;
        }
        case DestroyNotify:
        {
            if( !win->isClosed() )
            {
                // Window closed without window manager warning.
                for( index = start; index != end; ++index )
                    ( index->second )->windowClosed( win );
                win->destroy();
            }
            break;
        }
        case ConfigureNotify:
        {
            // This could be slightly more efficient if windowMovedOrResized took arguments:
            Ogre::uint32 oldWidth, oldHeight;
            Ogre::int32 oldLeft, oldTop;
            win->getMetrics( oldWidth, oldHeight, oldLeft, oldTop );
            win->windowMovedOrResized();

            Ogre::uint32 newWidth, newHeight;
            Ogre::int32 newLeft, newTop;
            win->getMetrics( newWidth, newHeight, newLeft, newTop );

            if( newLeft != oldLeft || newTop != oldTop )
            {
                for( index = start; index != end; ++index )
                    ( index->second )->windowMoved( win );
            }

            if( newWidth != oldWidth || newHeight != oldHeight )
            {
                for( index = start; index != end; ++index )
                    ( index->second )->windowResized( win );
            }
            break;
        }
        case FocusIn:   // Gained keyboard focus
        case FocusOut:  // Lost keyboard focus
            win->setFocused( event.type == FocusIn );
            for( index = start; index != end; ++index )
                ( index->second )->windowFocusChange( win );
            break;
        case MapNotify:  // Restored
            win->setFocused( true );
            for( index = start; index != end; ++index )
                ( index->second )->windowFocusChange( win );
            break;
        case UnmapNotify:  // Minimised
            win->setFocused( false );
            win->_setVisible( false );
            for( index = start; index != end; ++index )
                ( index->second )->windowFocusChange( win );
            break;
        case VisibilityNotify:
            switch( event.xvisibility.state )
            {
            case VisibilityUnobscured:
                win->setFocused( true );
                win->_setVisible( true );
                break;
            case VisibilityPartiallyObscured:
                win->setFocused( true );
                win->_setVisible( true );
                break;
            case VisibilityFullyObscured:
                win->setFocused( false );
                win->_setVisible( false );
                break;
            }
            for( index = start; index != end; ++index )
                ( index->second )->windowFocusChange( win );
            break;
        default:
            break;
        }  // End switch event.type
    }
    //
    void XcbProc( xcb_connection_t *xcbConnection, xcb_generic_event_t *e )
    {
        XcbWindowMap::const_iterator itWindow;
        Ogre::WindowEventUtilities::WindowEventListeners::iterator index, start, end;

        const Ogre::uint8 responseType = e->response_type & ~0x80;
        switch( responseType )
        {
        case XCB_CLIENT_MESSAGE:
        {
            xcb_client_message_event_t *event = reinterpret_cast<xcb_client_message_event_t *>( e );
            itWindow = gXcbWindowToOgre.find( event->window );
            if( itWindow != gXcbWindowToOgre.end() )
            {
                Ogre::Window *win = itWindow->second;
                if( event->format == 32u )
                {
                    xcb_atom_t wmProtocols;
                    xcb_atom_t wmDeleteWindow;
                    win->getCustomAttribute( "mWmProtocols", &wmProtocols );
                    win->getCustomAttribute( "mWmDeleteWindow", &wmDeleteWindow );

                    if( event->type == wmProtocols && event->data.data32[0] == wmDeleteWindow )
                    {
                        start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
                        end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );

                        // Window closed by window manager
                        // Send message first, to allow app chance to unregister things that need
                        // done before window is shutdown
                        bool close = true;
                        for( index = start; index != end; ++index )
                        {
                            if( !( index->second )->windowClosing( win ) )
                                close = false;
                        }
                        if( !close )
                            return;

                        for( index = start; index != end; ++index )
                            ( index->second )->windowClosed( win );
                        win->destroy();
                    }
                }
            }
        }
        break;
        case XCB_FOCUS_IN:   // Gained keyboard focus
        case XCB_FOCUS_OUT:  // Lost keyboard focus
        {
            xcb_focus_in_event_t *event = reinterpret_cast<xcb_focus_in_event_t *>( e );
            itWindow = gXcbWindowToOgre.find( event->event );
            if( itWindow != gXcbWindowToOgre.end() )
            {
                Ogre::Window *win = itWindow->second;
                win->setFocused( responseType == XCB_FOCUS_IN );

                start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
                end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );
                for( index = start; index != end; ++index )
                    ( index->second )->windowFocusChange( win );
            }
        }
        break;
        case XCB_MAP_NOTIFY:  // Restored
        {
            xcb_map_notify_event_t *event = reinterpret_cast<xcb_map_notify_event_t *>( e );
            itWindow = gXcbWindowToOgre.find( event->window );
            if( itWindow != gXcbWindowToOgre.end() )
            {
                Ogre::Window *win = itWindow->second;
                win->setFocused( true );

                start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
                end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );
                for( index = start; index != end; ++index )
                    ( index->second )->windowFocusChange( win );
            }
        }
        break;
        case XCB_UNMAP_NOTIFY:  // Minimized
        {
            xcb_unmap_notify_event_t *event = reinterpret_cast<xcb_unmap_notify_event_t *>( e );
            itWindow = gXcbWindowToOgre.find( event->window );
            if( itWindow != gXcbWindowToOgre.end() )
            {
                Ogre::Window *win = itWindow->second;
                win->setFocused( false );
                win->_setVisible( false );

                start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
                end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );
                for( index = start; index != end; ++index )
                    ( index->second )->windowFocusChange( win );
            }
        }
        break;
        case XCB_VISIBILITY_NOTIFY:
        {
            xcb_visibility_notify_event_t *event =
                reinterpret_cast<xcb_visibility_notify_event_t *>( e );
            itWindow = gXcbWindowToOgre.find( event->window );
            if( itWindow != gXcbWindowToOgre.end() )
            {
                Ogre::Window *win = itWindow->second;
                xcb_visibility_t visibility = static_cast<xcb_visibility_t>( event->state );
                switch( visibility )
                {
                case XCB_VISIBILITY_UNOBSCURED:
                case XCB_VISIBILITY_PARTIALLY_OBSCURED:
                    win->setFocused( true );
                    win->_setVisible( true );
                    break;
                case XCB_VISIBILITY_FULLY_OBSCURED:
                    win->setFocused( false );
                    win->_setVisible( false );
                    break;
                }

                start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
                end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );
                for( index = start; index != end; ++index )
                    ( index->second )->windowFocusChange( win );
            }
        }
        break;
        case XCB_CONFIGURE_NOTIFY:
        {
            xcb_configure_notify_event_t *event = reinterpret_cast<xcb_configure_notify_event_t *>( e );

            itWindow = gXcbWindowToOgre.find( event->window );
            if( itWindow != gXcbWindowToOgre.end() )
            {
                Ogre::Window *win = itWindow->second;

                // This could be slightly more efficient if windowMovedOrResized took arguments:
                Ogre::uint32 oldWidth, oldHeight;
                Ogre::int32 oldLeft, oldTop;
                win->getMetrics( oldWidth, oldHeight, oldLeft, oldTop );
                win->windowMovedOrResized();

                Ogre::uint32 newWidth, newHeight;
                Ogre::int32 newLeft, newTop;
                win->getMetrics( newWidth, newHeight, newLeft, newTop );

                start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
                end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );

                if( newLeft != oldLeft || newTop != oldTop )
                {
                    for( index = start; index != end; ++index )
                        ( index->second )->windowMoved( win );
                }

                if( newWidth != oldWidth || newHeight != oldHeight )
                {
                    for( index = start; index != end; ++index )
                        ( index->second )->windowResized( win );
                }
            }
        }
        break;
        case XCB_DESTROY_NOTIFY:
        {
            xcb_visibility_notify_event_t *event =
                reinterpret_cast<xcb_visibility_notify_event_t *>( e );
            itWindow = gXcbWindowToOgre.find( event->window );
            if( itWindow != gXcbWindowToOgre.end() )
            {
                Ogre::Window *win = itWindow->second;
                if( !win->isClosed() )
                {
                    start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
                    end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );
                    for( index = start; index != end; ++index )
                        ( index->second )->windowClosed( win );
                    win->destroy();
                }
            }
        }
        break;
        }
    }
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE && !defined __OBJC__ && !defined __LP64__
    //
    namespace Ogre
    {
        OSStatus WindowEventUtilities::_CarbonWindowHandler( EventHandlerCallRef nextHandler,
                                                             EventRef event, void *wnd )
        {
            OSStatus status = noErr;

            // Only events from our window should make it here
            // This ensures that our user data is our WindowRef
            Window *curWindow = (Window *)wnd;
            if( !curWindow )
                return eventNotHandledErr;

            // Iterator of all listeners registered to this Window
            WindowEventListeners::iterator index, start = _msListeners.lower_bound( curWindow ),
                                                  end = _msListeners.upper_bound( curWindow );

            // We only get called if a window event happens
            UInt32 eventKind = GetEventKind( event );

            switch( eventKind )
            {
            case kEventWindowActivated:
                curWindow->setFocused( true );
                for( ; start != end; ++start )
                    ( start->second )->windowFocusChange( curWindow );
                break;
            case kEventWindowDeactivated:
                curWindow->setFocused( false );

                for( ; start != end; ++start )
                    ( start->second )->windowFocusChange( curWindow );

                break;
            case kEventWindowShown:
            case kEventWindowExpanded:
                curWindow->setFocused( true );
                curWindow->setVisible( true );
                for( ; start != end; ++start )
                    ( start->second )->windowFocusChange( curWindow );
                break;
            case kEventWindowHidden:
            case kEventWindowCollapsed:
                curWindow->setFocused( false );
                curWindow->setVisible( false );
                for( ; start != end; ++start )
                    ( start->second )->windowFocusChange( curWindow );
                break;
            case kEventWindowDragCompleted:
                curWindow->windowMovedOrResized();
                for( ; start != end; ++start )
                    ( start->second )->windowMoved( curWindow );
                break;
            case kEventWindowBoundsChanged:
                curWindow->windowMovedOrResized();
                for( ; start != end; ++start )
                    ( start->second )->windowResized( curWindow );
                break;
            case kEventWindowClose:
            {
                bool close = true;
                for( ; start != end; ++start )
                {
                    if( !( start->second )->windowClosing( curWindow ) )
                        close = false;
                }
                if( close )
                    // This will cause event handling to continue on to the standard handler, which
                    // calls DisposeWindow(), which leads to the 'kEventWindowClosed' event
                    status = eventNotHandledErr;
                break;
            }
            case kEventWindowClosed:
                curWindow->destroy();
                for( ; start != end; ++start )
                    ( start->second )->windowClosed( curWindow );
                break;
            default:
                status = eventNotHandledErr;
                break;
            }

            return status;
        }
    }  // namespace Ogre
#endif

    void CGraphicsSystemOgreNext::GraphicsSystemEventListener::setOwner(
        SmartPtr<CGraphicsSystemOgreNext> owner )
    {
        m_owner = owner;
    }

    SmartPtr<CGraphicsSystemOgreNext> CGraphicsSystemOgreNext::GraphicsSystemEventListener::getOwner()
        const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    Parameter CGraphicsSystemOgreNext::GraphicsSystemEventListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto task = Thread::getCurrentTask();

        if( task == TaskId::Primary )
        {
            if( eventValue == IEvent::loadingStateChanged )
            {
                if( arguments[1].getS32() == static_cast<s32>( LoadingState::Loaded ) )
                {
                    if( sender )
                    {
                        if( sender->isDerived<CGraphicsSceneOgreNext>() )
                        {
                            auto graphicsScene = (CGraphicsSceneOgreNext *)sender;

                            if( auto imgui = ImguiManagerOgreNext::getSingletonPtr() )
                            {
                                Ogre::SceneManager *ogreSceneMgr = nullptr;
                                graphicsScene->_getObject( (void **)&ogreSceneMgr );

                                if( ogreSceneMgr )
                                {
                                    if( !imgui->isInitialised() )
                                    {
                                        imgui->init( ogreSceneMgr );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if( Thread::getTaskFlag( Thread::Render_Flag ) )
        {
            if( eventValue == IEvent::loadingStateChanged )
            {
                if( arguments[1].getS32() == static_cast<s32>( LoadingState::Loaded ) )
                {
                    if( sender )
                    {
                        if( sender->isDerived<CGraphicsSceneOgreNext>() )
                        {
                            auto graphicsScene = (CGraphicsSceneOgreNext *)sender;

                            //if( auto imgui = ImguiManagerOgreNext::getSingletonPtr() )
                            //{
                            //    Ogre::SceneManager *ogreSceneMgr = nullptr;
                            //    graphicsScene->_getObject( (void **)&ogreSceneMgr );

                            //    if( ogreSceneMgr )
                            //    {
                            //        if( !imgui->isInitialised() )
                            //        {
                            //            imgui->init( ogreSceneMgr );
                            //        }
                            //    }
                            //}

                            auto applicationManager = core::IApplicationManager::instancePtr();
                            auto ui = applicationManager->getRenderUI();
                            if( ui )
                            {
                                if( ui->isLoaded() )
                                {
                                    if( ui->isDerived<ui::UIManagerCore>() )
                                    {
                                        auto renderUI =
                                            workphone::static_ptr_cast<ui::UIManagerCore>( ui );
                                        if( renderUI )
                                        {
                                            //renderUI->setGraphicsScene( sceneManager );
                                        }
                                    }
                                }
                            }
                        }
                        else if( sender->isDerived<CompositorManager>() )
                        {
                        }
                        else if( sender->isDerived<IMaterial>() )
                        {
                            auto material = workphone::static_pointer_cast<IMaterial>( sender );

                            auto applicationManager = core::IApplicationManager::instancePtr();
                            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

                            auto graphicsScenes = graphicsSystem->getSceneManagers();
                            for( auto graphicsScene : graphicsScenes )
                            {
                                auto graphicsObjects = graphicsScene->getGraphicsObjects();
                                for( auto graphicsObject : graphicsObjects )
                                {
                                    if( graphicsObject )
                                    {
                                        if( graphicsObject->isDerived<CGraphicsMeshOgreNext>() )
                                        {
                                            auto graphicsMesh =
                                                workphone::static_ptr_cast<CGraphicsMeshOgreNext>(
                                                    graphicsObject );
                                            graphicsMesh->materialLoaded( material );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if( eventValue == IEvent::meshesImported )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

                auto graphicsScenes = graphicsSystem->getSceneManagers();
                for( auto graphicsScene : graphicsScenes )
                {
                    auto graphicsObjects = graphicsScene->getGraphicsObjects();
                    for( auto graphicsObject : graphicsObjects )
                    {
                        if( graphicsObject->isDerived<IGraphicsMesh>() )
                        {
                            auto graphicsMesh =
                                workphone::static_ptr_cast<IGraphicsMesh>( graphicsObject );
                            auto meshName = graphicsMesh->getMeshName();
                            auto meshFileName = Path::getFileName( meshName );

                            auto hashMeshIt = std::find_if(
                                arguments.begin(), arguments.end(), [meshFileName]( Parameter p ) {
                                    auto resource = static_cast<IResource *>( p.object );
                                    if( resource )
                                    {
                                        auto resourceFilePath = resource->getFilePath();
                                        auto resourceFileName = Path::getFileName( resourceFilePath );
                                        return resourceFileName == meshFileName;
                                    }

                                    return false;
                                } );

                            if( hashMeshIt != arguments.end() )
                            {
                                graphicsMesh->reload( nullptr );
                            }
                        }
                    }
                }
            }
        }

        return {};
    }

    CGraphicsSystemOgreNext::GraphicsSystemEventListener::GraphicsSystemEventListener() = default;

    CGraphicsSystemOgreNext::GraphicsSystemEventListener::~GraphicsSystemEventListener() = default;

    void CGraphicsSystemOgreNext::CustomLogListener::messageLogged( const Ogre::String &message,
                                                                    Ogre::LogMessageLevel lml,
                                                                    bool maskDebug,
                                                                    const Ogre::String &logName,
                                                                    bool &skipMessage )
    {
        if( lml == Ogre::LogMessageLevel::LML_CRITICAL )
        {
            WP_LOG_ERROR( message.c_str() );
        }
        else if( lml == Ogre::LogMessageLevel::LML_TRIVIAL )
        {
            WP_LOG( message.c_str() );
        }
        else if( lml == Ogre::LogMessageLevel::LML_NORMAL )
        {
            WP_LOG( message.c_str() );
        }
        else
        {
            WP_LOG( message.c_str() );
        }
    }

    CGraphicsSystemOgreNext::CustomLogListener::~CustomLogListener()
    {
    }

    CGraphicsSystemOgreNext::CustomLogListener::CustomLogListener()
    {
    }

    void CGraphicsSystemOgreNext::RenderSystemListener::setOwner(
        SmartPtr<CGraphicsSystemOgreNext> owner )
    {
        m_owner = owner;
    }

    SmartPtr<CGraphicsSystemOgreNext> CGraphicsSystemOgreNext::RenderSystemListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void CGraphicsSystemOgreNext::RenderSystemListener::eventOccurred(
        const String &eventName, const NameValuePairList *parameters /*= nullptr */ )
    {
    }

    CGraphicsSystemOgreNext::RenderSystemListener::RenderSystemListener() = default;

    CGraphicsSystemOgreNext::RenderSystemListener::~RenderSystemListener() = default;
}  // namespace workphone::render
