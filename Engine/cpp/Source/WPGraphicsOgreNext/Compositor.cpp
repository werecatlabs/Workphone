#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Compositor.hpp>
#include <WPGraphicsOgreNext/CompositorManager.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCameraOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CWindowOgreNext.hpp>
#include <WPGraphicsOgreNext/CompositorPassUI.hpp>
#include <WPGraphicsOgreNext/CompositorPassUiDef.hpp>
#include <WPGraphicsOgreNext/Terra/Terra.h>
#include <WPGraphicsOgreNext/Terra/TerraShadowMapper.h>
#include <WPGraphicsOgreNext/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <OgreTextureGpuManager.h>
#include <OgreRoot.h>
#include <OgreWindow.h>
#include <Compositor/OgreCompositorNode.h>
#include <Compositor/OgreCompositorNodeDef.h>
#include <Compositor/OgreCompositorWorkspaceDef.h>
#include <Compositor/Pass/OgreCompositorPassDef.h>
#include <Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h>
#include <Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h>
#include <Compositor/OgreCompositorShadowNodeDef.h>
#include <Compositor/Pass/PassClear/OgreCompositorPassClearDef.h>
#include <OgrePixelFormatGpu.h>
#include <OgreDepthBuffer.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgrePass.h>
#include <OgreGpuProgramParams.h>
#include <Terra/Hlms/OgreHlmsTerra.h>
#include <Terra/TerraWorkspaceListener.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, Compositor, SharedGraphicsObject<ISharedObject> );
    WP_CLASS_REGISTER_DERIVED( workphone::render, Compositor::EventListener, IEventListener );

    u32 Compositor::m_idExt = 0;

    Compositor::Compositor()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        auto name = "Compositor" + StringUtil::toString( m_idExt++ );
        auto id = StringUtil::getHash( name );

        setName( name );
        setId( id );
    }

    Compositor::Compositor( CompositorManager *mgr ) : m_owner( mgr )
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        auto name = "Compositor" + StringUtil::toString( m_idExt++ );
        auto id = StringUtil::getHash( name );

        setName( name );
        setId( id );

        setupStateContext();
    }

    Compositor::~Compositor()
    {
        SharedGraphicsObject<ISharedObject>::unload( nullptr );
    }

    void Compositor::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( !isLoaded() )
            {
                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instancePtr();
                auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                auto factoryManager = applicationManager->getFactoryManagerPtr();

                ScopedLock lock( graphicsSystem );

                m_eventListener = factoryManager->make_ptr<EventListener>();
                m_eventListener->setOwner( this );
                applicationManager->addObjectListener( m_eventListener );

                setLoadingState( LoadingState::Loaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Compositor::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( data );
            load( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Compositor::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instancePtr();
                auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

                ScopedLock lock( graphicsSystem );

                auto root = Ogre::Root::getSingletonPtr();
                Ogre::CompositorManager2 *pCompositorManager = nullptr;
                if( root )
                    pCompositorManager = root->getCompositorManager2();

                if( m_eventListener )
                {
                    applicationManager->removeObjectListener( m_eventListener );
                    m_eventListener = nullptr;
                }

                if( m_scenePassDef )
                {
                    m_scenePassDef->mShadowNode = Ogre::IdString();
                    m_scenePassDef = nullptr;
                }

                if( mTerraWorkspaceListener )
                {
                    if( m_compositorWorkspace )
                    {
                        m_compositorWorkspace->removeListener( mTerraWorkspaceListener );
                    }

                    delete mTerraWorkspaceListener;
                    mTerraWorkspaceListener = nullptr;
                }

                if( m_compositorWorkspace )
                {
                    m_compositorWorkspace->setEnabled( false );
                    if( pCompositorManager )
                        pCompositorManager->removeWorkspace( m_compositorWorkspace );
                    m_compositorWorkspace = nullptr;
                }

                destroyCompositeMaterial();

                setTargetTexture( nullptr );
                setTextureGpu( nullptr );

                setSceneManager( nullptr );
                setCamera( nullptr );
                setWindow( nullptr );

                SharedGraphicsObject<ISharedObject>::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Compositor::setEnabled( bool enabled, bool updateSetup )
    {
        if( !isLoaded() || !updateSetup )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData =
                        stateContext->invalidateStateDataById<CompositorStateData>( getId() ) )
                {
                    stateData->enabled = enabled;
                }
            }
        }
        else
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData =
                        stateContext->invalidateStateDataById<CompositorStateData>( getId(), false ) )
                {
                    if( isLoaded() )
                    {
                        if( stateData->enabled != enabled )
                        {
                            stateData->enabled = enabled;
                            setupWorkspace( enabled );
                        }
                    }
                }
            }
        }
    }

    void Compositor::stopCompositor()
    {
        WP_ASSERT( Thread::getCurrentTask() == TaskId::Render );

        if( mTerraWorkspaceListener )
        {
            if( m_compositorWorkspace )
            {
                m_compositorWorkspace->removeListener( mTerraWorkspaceListener );
            }

            delete mTerraWorkspaceListener;
            mTerraWorkspaceListener = nullptr;
        }
    }

    bool Compositor::setupWorkspace( bool enabled )
    {
        try
        {
            WP_LOG( "Compositor::setupEnabled: " + StringUtil::toString( enabled ) );
            WP_ASSERT( Thread::getCurrentTask() == TaskId::Render );

            auto applicationManager = core::IApplicationManager::instancePtr();

            auto graphicsSystem = (CGraphicsSystemOgreNext *)applicationManager->getGraphicsSystemPtr();
            if( !graphicsSystem )
            {
                WP_LOG_ERROR( "Compositor::setupWorkspace: graphics system is null" );
                return false;
            }

            auto compositorManager = graphicsSystem->getCompositorManagerPtr();
            if( !compositorManager )
            {
                WP_LOG_ERROR( "Compositor::setupWorkspace: compositor manager is null" );
                return false;
            }

            ScopedLock lock( this, true );

            auto root = Ogre::Root::getSingletonPtr();
            if( !root )
            {
                WP_LOG_ERROR( "Compositor::setupWorkspace: Ogre root is null" );
                return false;
            }

            auto ogreCompositorManager = root->getCompositorManager2();
            if( !ogreCompositorManager )
            {
                WP_LOG_ERROR( "Compositor::setupWorkspace: Ogre compositor manager is null" );
                return false;
            }

            auto pSceneManager =
                workphone::dynamic_pointer_cast<CGraphicsSceneOgreNext>( getSceneManager() );
            auto ogrenextWindow = workphone::dynamic_pointer_cast<CWindowOgreNext>( getWindow() );

            if( enabled )
            {
                m_compositorWorkspaceDef = setupCompositor();
                if( !m_compositorWorkspaceDef )
                {
                    WP_LOG_ERROR( "Compositor::setupWorkspace: failed to create workspace definition" );
                    return false;
                }

                auto workspaceName = getWorkspaceName();

                auto pCamera = (CCameraOgreNext *)getCameraPtr();
                if( !pCamera )
                {
                    WP_LOG_ERROR( "Compositor::setupWorkspace: camera is null" );
                    return false;
                }

                Ogre::Camera* pOgreCamera = nullptr;
                pCamera->_getObject( reinterpret_cast<void **>( &pOgreCamera ) );

                if( !pOgreCamera )
                {
                    WP_LOG_ERROR( "Compositor::setupWorkspace: Ogre camera is null" );
                    return false;
                }

                auto viewport = pCamera->getViewport();

                auto renderScene = true;
                auto showOverlays = true;
                auto showUI = true;
                auto bgColour = ColourF::Blue;
                auto mask = std::numeric_limits<uint32_t>::max();
                auto shadowsEnabled = false;
                auto clearEveryFrame = true;
                auto workspacePosition = -1;

                if( viewport )
                {
                    renderScene = viewport->getEnableSceneRender();
                    showOverlays = viewport->getOverlaysEnabled();
                    showUI = viewport->getEnableUI();
                    bgColour = viewport->getBackgroundColour();
                    mask = viewport->getVisibilityMask();
                    shadowsEnabled = viewport->getShadowsEnabled();
                    clearEveryFrame = viewport->getClearEveryFrame();
                    workspacePosition = viewport->getZOrder();
                }

                if( m_scenePassDef )
                {
                    const auto shadowNodeName = Ogre::IdString( "MainShadowNode" );

                    //if( shadowsEnabled &&
                    //    ogreCompositorManager->hasShadowNodeDefinition( shadowNodeName ) )
                    //{
                    //    m_scenePassDef->mShadowNode = shadowNodeName;
                    //}
                    //else
                    //{
                    //    if( shadowsEnabled )
                    //    {
                    //        WP_LOG_WARNING(
                    //            "Compositor::setupWorkspace: shadow node 'MainShadowNode' not found, "
                    //            "shadows disabled" );
                    //    }

                        m_scenePassDef->mShadowNode = Ogre::IdString();
                    //}

                    m_scenePassDef->mIncludeOverlays = showOverlays;

                    // Scene-disabled viewports are used for UI-only cameras. Keep their scene pass
                    // empty instead of rendering the scene redundantly behind the application UI.
                    m_scenePassDef->mVisibilityMask = renderScene ? mask : 0u;

                    m_scenePassDef->setAllLoadActions( clearEveryFrame ? Ogre::LoadAction::Clear
                                                                       : Ogre::LoadAction::Load );
                    // A later camera may preserve colour from an earlier camera, but it must
                    // always start with a fresh depth/stencil buffer.
                    m_scenePassDef->mLoadActionDepth = Ogre::LoadAction::Clear;
                    m_scenePassDef->mLoadActionStencil = Ogre::LoadAction::Clear;
                }

                if( !m_compositorWorkspace )
                {
                    if( !pSceneManager )
                    {
                        WP_LOG_ERROR( "Compositor::setupWorkspace: scene manager is null" );
                        return false;
                    }

                    if( !ogrenextWindow )
                    {
                        WP_LOG_ERROR( "Compositor::setupWorkspace: window is null" );
                        return false;
                    }

                    auto sceneManager = pSceneManager->getSceneManager();
                    auto renderWindow = ogrenextWindow->getWindow();

                    if( !sceneManager || !renderWindow )
                    {
                        WP_LOG_ERROR(
                            "Compositor::setupWorkspace: underlying scene manager or render window is "
                            "null" );
                        return false;
                    }

                    Ogre::Camera *ogreCamera = pCamera->getGraphicsObjectByType<Ogre::Camera>();
                    if( !ogreCamera )
                    {
                        WP_LOG_ERROR( "Compositor::setupWorkspace: Ogre camera is null" );
                        return false;
                    }

                    // Determine render target: prefer target texture, fall back to window.
                    SmartPtr<ITexture> targetTexture = pCamera->getTargetTexture();
                    Ogre::TextureGpu *texture = nullptr;

                    if( targetTexture && targetTexture->isLoaded() )
                    {
                        targetTexture->_getObject( (void **)(Ogre::TextureGpu **)&texture );
                        if( !texture )
                        {
                            WP_LOG_ERROR( "Compositor::setupWorkspace: TextureGpu is null" );
                            return false;
                        }

                        setTargetTexture( targetTexture );
                        setTextureGpu( texture );
                    }
                    else
                    {
                        // Window camera path — render directly to the window surface.
                        texture = renderWindow->getTexture();
                        if( !texture )
                        {
                            WP_LOG_ERROR( "Compositor::setupWorkspace: render window texture is null" );
                            return false;
                        }

                        setTargetTexture( nullptr );
                        setTextureGpu( texture );
                    }

                    // Re-validate ogreCompositorManager before use: it may have been
                    // destroyed between the earlier null check and this point (TOCTOU race).
                    ogreCompositorManager = root->getCompositorManager2();
                    if( !ogreCompositorManager )
                    {
                        WP_LOG_ERROR(
                            "Compositor::setupWorkspace: Ogre compositor manager became null before "
                            "addWorkspace" );
                        setTargetTexture( nullptr );
                        setTextureGpu( nullptr );
                        return false;
                    }

                    auto workspaceNameStr =
                        Ogre::String( workspaceName.c_str(), workspaceName.length() );
                    // Build external channels:
                    //  [0] = render target (window surface or target texture)
                    //  [1] = Terra shadow map, or a reusable 1x1 dummy when Terra
                    //        is not yet initialised.
                    auto compositeLayers = pCamera->getCompositeLayers();
                    Ogre::CompositorChannelVec externalChannels( 6u );
                    externalChannels[0] = texture;

                    if( auto *terra = graphicsSystem->getTerra() )
                    {
                        const auto *shadowMapper = terra->getShadowMapper();
                        shadowMapper->fillUavDataForCompositorChannel( &externalChannels[1] );
                    }
                    else
                    {
                        // Terra not yet initialised — supply a minimal dummy texture.
                        // createOrRetrieveTexture reuses an existing one if already created.
                        auto *textureManager = root->getRenderSystem()->getTextureGpuManager();
                        Ogre::TextureGpu *nullTex = textureManager->createOrRetrieveTexture(
                            "DummyNull", Ogre::GpuPageOutStrategy::Discard,
                            Ogre::TextureFlags::ManualTexture, Ogre::TextureTypes::Type2D );
                        nullTex->setResolution( 1u, 1u );
                        nullTex->setPixelFormat( Ogre::PFG_R10G10B10A2_UNORM );
                        nullTex->scheduleTransitionTo( Ogre::GpuResidency::Resident );
                        externalChannels[1] = nullTex;
                    }

                    auto *textureManager = root->getRenderSystem()->getTextureGpuManager();
                    Ogre::TextureGpu *nullCompositeTex = textureManager->createOrRetrieveTexture(
                        "WorkphoneCompositeNull", Ogre::GpuPageOutStrategy::Discard,
                        Ogre::TextureFlags::ManualTexture, Ogre::TextureTypes::Type2D );
                    nullCompositeTex->setResolution( 1u, 1u );
                    nullCompositeTex->setPixelFormat( Ogre::PFG_RGBA8_UNORM );
                    nullCompositeTex->scheduleTransitionTo( Ogre::GpuResidency::Resident );

                    for( size_t i = 0u; i < 4u; ++i )
                    {
                        Ogre::TextureGpu *layerTexture = nullCompositeTex;
                        if( i < compositeLayers.size() )
                        {
                            const auto &layer = compositeLayers[i];
                            if( layer.enabled && layer.texture == targetTexture )
                            {
                                WP_LOG_WARNING( "A camera cannot sample its own output render "
                                                "target as a composite layer; the layer was skipped." );
                            }
                            else if( layer.enabled && layer.texture && layer.texture->isLoaded() )
                            {
                                layer.texture->_getObject(
                                    reinterpret_cast<void **>( &layerTexture ) );
                                if( !layerTexture )
                                    layerTexture = nullCompositeTex;
                            }
                        }

                        externalChannels[2u + i] = layerTexture;
                    }

                    m_compositorWorkspace = ogreCompositorManager->addWorkspace(
                        sceneManager, externalChannels, ogreCamera, workspaceNameStr, true,
                        workspacePosition );

                    if( !m_compositorWorkspace )
                    {
                        WP_LOG_ERROR( "Compositor::setupWorkspace: addWorkspace returned null" );
                        setTargetTexture( nullptr );
                        setTextureGpu( nullptr );
                        return false;
                    }

                    auto &nodes = m_compositorWorkspace->getNodeSequence();
                    for( auto node : nodes )
                    {
                        auto &passes = node->_getPasses();
                        for( auto pass : passes )
                        {
                            if( auto uiPass = dynamic_cast<CompositorPassUI *>( pass ) )
                            {
                                auto uiDef = uiPass->mUiDefinition;
                                uiDef->mRenderUI = !renderScene && showUI;
                                uiDef->mRenderSceneUI = renderScene && showUI;
                            }
                        }
                    }

                    return true;
                }
                // Note: the else branch (setEnabled(true)) is intentionally omitted.
                // setupCompositor() always nullifies m_compositorWorkspace before returning,
                // so !m_compositorWorkspace is always true here.
            }
            else
            {
                setTextureGpu( nullptr );

                if( m_compositorWorkspace )
                {
                    ogreCompositorManager->removeWorkspace( m_compositorWorkspace );
                    m_compositorWorkspace = nullptr;
                }

                return true;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

    bool Compositor::isEnabled() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<CompositorStateData>( getId() ) )
            {
                return state->enabled;
            }
        }

        return false;
    }

    Ogre::CompositorWorkspace *Compositor::getCompositorWorkspace() const
    {
        return m_compositorWorkspace;
    }

    void Compositor::setCompositorWorkspace( Ogre::CompositorWorkspace *compositorWorkspace )
    {
        m_compositorWorkspace = compositorWorkspace;
    }

    SmartPtr<IGraphicsScene> Compositor::getSceneManager() const
    {
        auto p = m_sceneManager.load();
        return p.lock();
    }

    void Compositor::setSceneManager( SmartPtr<IGraphicsScene> sceneManager )
    {
        m_sceneManager = sceneManager;
    }

    SmartPtr<IGraphicsWindow> Compositor::getWindow() const
    {
        auto p = m_window.load();
        return p.lock();
    }

    void Compositor::setWindow( SmartPtr<IGraphicsWindow> window )
    {
        m_window = window;
    }

    SmartPtr<IGraphicsCamera> Compositor::getCamera() const
    {
        auto p = m_camera.load();
        return p.lock();
    }

    void Compositor::setCamera( SmartPtr<IGraphicsCamera> camera )
    {
        m_camera = camera;
    }

    String Compositor::getWorkspaceName() const
    {
        return m_workspaceName;
    }

    void Compositor::setWorkspaceName( const String &workspaceName )
    {
        m_workspaceName = workspaceName;
    }

    Ogre::CompositorWorkspace *Compositor::setupTestCompositor()
    {
        WP_ASSERT( Thread::getCurrentTask() == TaskId::Render );

        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem =
            static_cast<CGraphicsSystemOgreNext *>( applicationManager->getGraphicsSystemPtr() );
        auto pCompositorManager =
            workphone::static_pointer_cast<CompositorManager>( graphicsSystem->getCompositorManager() );

        auto pCamera = getCamera();

        auto pSceneManager =
            workphone::dynamic_pointer_cast<CGraphicsSceneOgreNext>( getSceneManager() );
        auto ogreCamera = workphone::dynamic_pointer_cast<CCameraOgreNext>( pCamera );
        auto ogreWindow = workphone::dynamic_pointer_cast<CWindowOgreNext>( getWindow() );

        auto sceneManager = pSceneManager->getSceneManager();
        auto renderWindow = ogreWindow->getWindow();
        auto camera = ogreCamera->getGraphicsObjectByType<Ogre::Camera>();

        using namespace Ogre;

        // Setup a basic compositor with a blue clear colour
        auto root = Root::getSingletonPtr();
        auto compositorManager = root->getCompositorManager2();

        auto workspaceName = getWorkspaceName();
        const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
        //compositorManager->createBasicWorkspaceDef( workspaceName, backgroundColour, IdString("MainShadowNode") );
        return compositorManager->addWorkspace( sceneManager, renderWindow->getTexture(), camera,
                                                "PbsMaterialsWorkspace", true );
    }

    SmartPtr<Properties> Compositor::getProperties() const
    {
        auto properties = SharedGraphicsObject<ISharedObject>::getProperties();

        auto enabled = isEnabled();
        properties->setProperty( "isEnabled", enabled );

        return properties;
    }

    void Compositor::setProperties( SmartPtr<Properties> properties )
    {
        bool enabled = false;
        properties->getPropertyValue( "isEnabled", enabled );

        if( isEnabled() != enabled )
        {
            setEnabled( enabled );
        }
    }

    Array<SmartPtr<ISharedObject>> Compositor::getChildObjects() const
    {
        auto objects = SharedGraphicsObject<ISharedObject>::getChildObjects();
        //objects.push_back( getCamera() );
        objects.emplace_back( getWindow() );
        objects.emplace_back( getSceneManager() );
        return objects;
    }

    Ogre::CompositorWorkspaceDef *Compositor::setupCompositor()
    {
        using namespace Ogre;

        auto root = Root::getSingletonPtr();
        if( !root )
        {
            WP_LOG_ERROR( "Compositor::setupCompositor: Ogre root is null" );
            return nullptr;
        }

        auto *pOgreCompositorManager = root->getCompositorManager2();

        if( m_compositorWorkspace )
        {
            // Retrieve the Terra shadow texture before the workspace is destroyed so
            // we can clean it up if it was a UAV-backed null-format texture created
            // by the shadow mapper. Dummy textures (non-null format) are reused via
            // createOrRetrieveTexture and must not be destroyed here.
            const auto &externalRTs = m_compositorWorkspace->getExternalRenderTargets();
            TextureGpu *terraShadowTex = externalRTs.size() > 1u ? externalRTs[1] : nullptr;

            setTextureGpu( nullptr );
            m_compositorWorkspace->setEnabled( false );
            pOgreCompositorManager->removeWorkspace( m_compositorWorkspace );
            m_compositorWorkspace = nullptr;

            if( terraShadowTex && terraShadowTex->getPixelFormat() == PFG_NULL )
            {
                TextureGpuManager *textureManager =
                    root->getRenderSystem()->getTextureGpuManager();
                textureManager->destroyTexture( terraShadowTex );
            }
        }

        if( m_compositorWorkspaceDef && m_ownsWorkspaceDefinition )
        {
            try
            {
                auto workspaceName = getWorkspaceName();
                Ogre::String str( workspaceName.c_str(), workspaceName.length() );

                if( pOgreCompositorManager->hasWorkspaceDefinition( str ) )
                {
                    pOgreCompositorManager->removeWorkspaceDefinition( str );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            m_compositorWorkspaceDef = nullptr;
        }

        m_ownsWorkspaceDefinition = false;
        destroyCompositeMaterial();

        // Reset pass def pointers — they belong to the definition we just removed
        m_scenePassDef = nullptr;
        m_sceneUiPassDef = nullptr;
        m_applicationUiPassDef = nullptr;
        m_targetDef = nullptr;

        if( auto camera = getCamera() )
        {
            const auto settings = camera->getPostProcessSettings();
            if( settings.enabled && !settings.workspace.empty() )
            {
                const Ogre::IdString workspaceId( settings.workspace.c_str() );
                if( pOgreCompositorManager->hasWorkspaceDefinition( workspaceId ) )
                {
                    setWorkspaceName( settings.workspace );
                    m_compositorWorkspaceDef =
                        pOgreCompositorManager->getWorkspaceDefinition( workspaceId );
                    return m_compositorWorkspaceDef;
                }

                WP_LOG_WARNING( "Camera post-processing workspace not found: " +
                                settings.workspace + ". Falling back to the default workspace." );
            }
        }

        auto workspaceName = StringUtil::getUUID();
        setWorkspaceName( workspaceName );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = (CGraphicsSystemOgreNext *)applicationManager->getGraphicsSystemPtr();
        auto compositorManager = graphicsSystem->getCompositorManager();

        auto renderScene = true;
        auto showOverlays = true;
        auto showUI = true;
        auto bgColour = ColourF::Blue;
        auto mask = std::numeric_limits<uint32_t>::max();
        auto shadowsEnabled = false;
        auto clearEveryFrame = true;

        auto pCamera = (CCameraOgreNext *)getCameraPtr();
        if( pCamera )
        {
            if( auto viewport = pCamera->getViewport() )
            {
                renderScene = viewport->getEnableSceneRender();
                showOverlays = viewport->getOverlaysEnabled();
                showUI = viewport->getEnableUI();
                bgColour = viewport->getBackgroundColour();
                mask = viewport->getVisibilityMask();
                shadowsEnabled = viewport->getShadowsEnabled();
                clearEveryFrame = viewport->getClearEveryFrame();
            }
        }

        auto pSceneManager =
            workphone::dynamic_pointer_cast<CGraphicsSceneOgreNext>( getSceneManager() );
        auto ogreWindow = workphone::dynamic_pointer_cast<CWindowOgreNext>( getWindow() );

        Ogre::Camera *camera = nullptr;

        if( pCamera )
        {
            camera = pCamera->getGraphicsObjectByType<Ogre::Camera>();
        }

        const auto backgroundColour = OgreUtil::convertToOgre( bgColour );

        auto shadowNodeName = IdString( "MainShadowNode" );

        const auto nodeDefinitionName = String( "AutoGen " ) + workspaceName + "/Node";
        const Ogre::IdString nodeDefinitionId( nodeDefinitionName.c_str() );

        if( pOgreCompositorManager->hasNodeDefinition( nodeDefinitionId ) )
        {
            pOgreCompositorManager->removeNodeDefinition( nodeDefinitionId );
        }

        auto nodeDef = pOgreCompositorManager->addNodeDefinition( nodeDefinitionName.c_str() );

        // Input texture
        const auto targetName = String( "WindowRT" );
        nodeDef->addTextureSourceName( targetName.c_str(), 0, TextureDefinitionBase::TEXTURE_INPUT );

        // Terra shadow map input (channel 1). When Terra is inactive a dummy
        // PFG_R10G10B10A2_UNORM texture is supplied instead.
        nodeDef->addTextureSourceName( "TerraShadowMap", 1, TextureDefinitionBase::TEXTURE_INPUT );

        const auto compositeLayers = pCamera ? pCamera->getCompositeLayers()
                                             : Array<IGraphicsCamera::CompositeLayer>();
        const bool hasCompositeLayers = std::any_of(
            compositeLayers.begin(), compositeLayers.end(), []( const auto &layer ) {
                return layer.enabled && layer.texture;
            } );

        for( size_t i = 0u; i < 4u; ++i )
        {
            const auto layerName = "CompositeLayer" + Ogre::StringConverter::toString( i );
            nodeDef->addTextureSourceName( layerName, 2u + i,
                                           TextureDefinitionBase::TEXTURE_INPUT );
        }

        nodeDef->setNumTargetPass( 1 );
        {
            CompositorTargetDef *targetDef = nodeDef->addTargetPass( targetName.c_str() );
            // Count passes: scene, optional composite, scene_gui and app_gui.
            size_t numPasses = 1u + ( hasCompositeLayers ? 1u : 0u );
#if WP_USE_COLLIBRI
            if( renderScene )
                ++numPasses;
#endif
            if( showUI )
                ++numPasses;
            targetDef->setNumPasses( numPasses );
            m_targetDef = targetDef;

            {
                {
                    auto passScene =
                        static_cast<CompositorPassSceneDef *>( targetDef->addPass( PASS_SCENE ) );

                    if( shadowsEnabled &&
                        pOgreCompositorManager->hasShadowNodeDefinition( shadowNodeName ) )
                    {
                        passScene->mShadowNode = shadowNodeName;
                    }

                    passScene->setAllClearColours( backgroundColour );
                    passScene->setAllLoadActions( clearEveryFrame ? LoadAction::Clear
                                                                  : LoadAction::Load );
                    passScene->mLoadActionDepth = LoadAction::Clear;
                    passScene->mLoadActionStencil = LoadAction::Clear;
                    passScene->mStoreActionDepth = StoreAction::DontCare;
                    passScene->mStoreActionStencil = StoreAction::DontCare;
                    passScene->mIncludeOverlays = showOverlays;

                    passScene->mVisibilityMask = renderScene ? mask : 0u;

                    m_scenePassDef = passScene;
                }

                if( hasCompositeLayers )
                {
                    auto &materialManager = Ogre::MaterialManager::getSingleton();
                    auto baseMaterial = materialManager.getByName(
                        "Workphone/Composite4",
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                    if( baseMaterial )
                    {
                        baseMaterial->load();
                        m_compositeMaterialName = "Workphone/Composite4/" + workspaceName;
                        auto material = baseMaterial->clone( m_compositeMaterialName.c_str() );
                        auto *pass = material->getTechnique( 0 )->getPass( 0 );
                        auto params = pass->getFragmentProgramParameters();

                        Ogre::Vector4 opacity( 0.0f );
                        Ogre::Vector4 blendMode( 0.0f );
                        Ogre::Vector4 enabledLayers( 0.0f );
                        for( size_t i = 0u; i < 4u && i < compositeLayers.size(); ++i )
                        {
                            const auto &layer = compositeLayers[i];
                            opacity[i] = std::clamp( layer.opacity, 0.0f, 1.0f );
                            blendMode[i] = static_cast<Ogre::Real>( layer.blendMode );
                            enabledLayers[i] =
                                layer.enabled && layer.texture &&
                                        layer.texture != pCamera->getTargetTexture()
                                    ? 1.0f
                                    : 0.0f;
                        }

                        const auto settings = pCamera->getPostProcessSettings();
                        const Ogre::Vector4 colourSettings = settings.enabled
                            ? Ogre::Vector4( settings.exposure, std::max( settings.gamma, 0.01f ),
                                             settings.contrast, settings.saturation )
                            : Ogre::Vector4( 0.0f, 1.0f, 1.0f, 1.0f );
                        const Ogre::Vector4 effectSettings = settings.enabled
                            ? Ogre::Vector4( settings.bloom ? settings.bloomIntensity : 0.0f,
                                             settings.bloomThreshold, settings.vignette,
                                             settings.fxaa ? 1.0f : 0.0f )
                            : Ogre::Vector4::ZERO;

                        params->setNamedConstant( "layerOpacity", opacity );
                        params->setNamedConstant( "layerBlendMode", blendMode );
                        params->setNamedConstant( "layerEnabled", enabledLayers );
                        params->setNamedConstant( "colourSettings", colourSettings );
                        params->setNamedConstant( "effectSettings", effectSettings );

                        auto passQuad = static_cast<CompositorPassQuadDef *>(
                            targetDef->addPass( PASS_QUAD ) );
                        passQuad->mMaterialName = m_compositeMaterialName.c_str();
                        for( size_t i = 0u; i < 4u; ++i )
                        {
                            passQuad->addQuadTextureSource(
                                i, "CompositeLayer" + Ogre::StringConverter::toString( i ) );
                        }
                    }
                    else
                    {
                        WP_LOG_ERROR( "Built-in composite material Workphone/Composite4 was not "
                                      "found. Composite layers will be skipped." );
                    }
                }

                if( renderScene )
                {
                    auto passProvider = pOgreCompositorManager->getCompositorPassProvider();
                    if( passProvider )
                    {
                        auto uiNodeName = IdString( "scene_gui" );
                        m_sceneUiPassDef = targetDef->addPass( PASS_CUSTOM, uiNodeName );
                    }
                }

                if( showUI )
                {
                    auto passProvider = pOgreCompositorManager->getCompositorPassProvider();
                    if( passProvider )
                    {
                        auto uiNodeName = IdString( "app_gui" );
                        m_applicationUiPassDef = targetDef->addPass( PASS_CUSTOM, uiNodeName );
                    }
                }
            }
        }

        Ogre::String str( workspaceName.c_str(), workspaceName.length() );
        auto workDef = pOgreCompositorManager->addWorkspaceDefinition( str );
        m_ownsWorkspaceDefinition = true;

        auto nodeDefName = nodeDef->getName();
        workDef->connectExternal( 0, nodeDefName, 0 );
        workDef->connectExternal( 1, nodeDefName, 1 );  // Terra shadow map
        for( size_t i = 0u; i < 4u; ++i )
        {
            const auto channel = static_cast<Ogre::uint32>( 2u + i );
            workDef->connectExternal( channel, nodeDefName, channel );
        }

        return workDef;
    }

    void Compositor::destroyCompositeMaterial()
    {
        if( !m_compositeMaterialName.empty() )
        {
            auto *materialManager = Ogre::MaterialManager::getSingletonPtr();
            if( materialManager )
            {
                materialManager->remove( m_compositeMaterialName.c_str() );
            }
            m_compositeMaterialName.clear();
        }
    }

    Parameter Compositor::handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto task = Thread::getCurrentTask();
        if( task == TaskId::Render )
        {
            auto targetTexture = getTargetTexture();
            if( eventValue == IEvent::renderTargetTextureLoaded )
            {
                if( object == targetTexture )
                {
                    auto enabled = isEnabled();
                    setupWorkspace( enabled );
                }
            }
            else if( eventValue == IEvent::renderTargetTextureUnloaded )
            {
                if( object == targetTexture )
                {
                    setupWorkspace( false );
                }
            }
        }

        return {};
    }

    void Compositor::setTextureGpu( Ogre::TextureGpu *textureGpu )
    {
        m_textureGpu = textureGpu;
    }

    Ogre::TextureGpu *Compositor::getTextureGpu() const
    {
        return m_textureGpu;
    }

    SmartPtr<ITexture> Compositor::getTargetTexture() const
    {
        auto p = m_targetTexture.load();
        return p.lock();
    }

    void Compositor::setTargetTexture( SmartPtr<ITexture> texture )
    {
        m_targetTexture = texture;
    }

    void Compositor::makeDirty()
    {
    }

    bool Compositor::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( isLoaded() )
        {
            if( message->getSender() == this )
            {
            }
        }

        return false;
    }

    bool Compositor::handleStateChanged( SmartPtr<IState> &state )
    {
        if( isLoaded() )
        {
            if( state->getOwnerPtr() == this )
            {
                if( auto data = state->getDataPtr() )
                {
                    if( data->isDerived<CompositorStateData>() )
                    {
                        auto stateData = SafeReadPtr<CompositorStateData>( data );
                        if( setupWorkspace( stateData->enabled ) )
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    CompositorManager *Compositor::getOwner() const
    {
        return m_owner;
    }

    void Compositor::setOwner( CompositorManager *owner )
    {
        m_owner = owner;
    }

    void Compositor::setupStateContext()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManagerPtr();
        WP_ASSERT( stateManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = graphicsSystem->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto stateContext = m_owner->getStateContextPtr();
        WP_ASSERT( stateContext );
        setStateContext( stateContext );

        auto state = factoryManager->make_ptr<State>();
        state->setId( getId() );
        state->setOwner( this );
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<CompositorStateData>();
        state->setData( stateData );
    }

    void Compositor::EventListener::setOwner( SmartPtr<Compositor> owner )
    {
        m_owner = owner;
    }

    SmartPtr<Compositor> Compositor::EventListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    Parameter Compositor::EventListener::handleEvent( EventType eventType, hash_type eventValue,
                                                      const Array<Parameter> &arguments,
                                                      SmartPtr<ISharedObject> sender,
                                                      SmartPtr<ISharedObject> object,
                                                      SmartPtr<IEvent> event )
    {
        if( auto owner = getOwnerPtr() )
        {
            return owner->handleEvent( eventType, eventValue, arguments, sender, object, event );
        }

        return {};
    }

    void Compositor::EventListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    Compositor::EventListener::EventListener() = default;

    Compositor::EventListener::~EventListener() = default;

}  // namespace workphone::render
