#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/CompositorManager.hpp>
#include <WPGraphicsOgreNext/Compositor.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCameraOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CWindowOgreNext.hpp>
#include <WPGraphicsOgreNext/Terra/Terra.h>
#include <WPGraphicsOgreNext/Terra/TerraShadowMapper.h>
#include <WPGraphicsOgreNext/CompositorPassProvider.hpp>
#include <Workphone/WorkphoneInterface.hpp>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <OgreTextureGpuManager.h>
#include <OgreWindow.h>
#include <Ogre.h>
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
#include <Terra/Hlms/OgreHlmsTerra.h>
#include <Terra/TerraWorkspaceListener.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CompositorManager, ISharedObject );
    WP_CLASS_REGISTER_DERIVED( workphone::render, CompositorManager::StateListener, IStateListener );

    CompositorManager::CompositorManager()
    {
        static const auto compositorManagerStr = String( "CompositorManager" );
        setName( compositorManagerStr );

        setupStateContext();
    }

    CompositorManager::~CompositorManager() = default;

    void CompositorManager::setupRenderer( SmartPtr<IGraphicsScene> pISceneManager,
                                           SmartPtr<IGraphicsWindow> pIGraphicsWindow,
                                           SmartPtr<IGraphicsCamera> pIGraphicsCamera,
                                           String workspaceName, bool enabled )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = (CGraphicsSystemOgreNext *)( applicationManager->getGraphicsSystemPtr() );
        auto pCompositorManager =
            workphone::static_pointer_cast<CompositorManager>( graphicsSystem->getCompositorManager() );

        auto pSceneManager = workphone::dynamic_pointer_cast<CGraphicsSceneOgreNext>( pISceneManager );
        auto ogreCamera = workphone::dynamic_pointer_cast<CCameraOgreNext>( pIGraphicsCamera );
        auto ogreWindow = workphone::dynamic_pointer_cast<CWindowOgreNext>( pIGraphicsWindow );

        auto sceneManager = pSceneManager->getSceneManager();
        auto renderWindow = ogreWindow->getWindow();
        auto camera = ogreCamera->getGraphicsObjectByType<Ogre::Camera>();

        using namespace Ogre;

        // Setup a basic compositor with a blue clear colour
        auto root = Root::getSingletonPtr();
        auto compositorManager = root->getCompositorManager2();

        const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
        compositorManager->createBasicWorkspaceDef( workspaceName.c_str(), backgroundColour,
                                                    IdString() );
        compositorManager->addWorkspace( sceneManager, renderWindow->getTexture(), camera,
                                         workspaceName.c_str(), true );
    }

    void CompositorManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto root = Ogre::Root::getSingletonPtr();
            auto compoProvider = OGRE_NEW CompositorPassProvider();
            auto compositorManager = root->getCompositorManager2();
            compositorManager->setCompositorPassProvider( compoProvider );
            m_passProvider = compoProvider;

            createMainShadowNode();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CompositorManager::reload( SmartPtr<ISharedObject> data )
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

    void CompositorManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                // Tear down state context and listener before releasing resources.
                if( auto stateContext = getStateContext() )
                {
                    if( auto stateListener = getStateListener() )
                    {
                        stateContext->removeStateListener( stateListener );
                        stateListener->unload( nullptr );
                        setStateListener( nullptr );
                    }

                    auto applicationManager = core::IApplicationManager::instancePtr();
                    if( auto stateManager =
                            applicationManager ? applicationManager->getStateManagerPtr() : nullptr )
                    {
                        stateManager->removeStateContext( stateContext );
                    }

                    setStateContext( nullptr );
                }

                for( auto &compositor : m_compositors )
                {
                    if( compositor )
                        compositor->unload( nullptr );
                }

                m_compositors.clear();

                // Clear member references so downstream objects can be released.
                m_sceneManager = nullptr;
                m_window = nullptr;
                m_camera = nullptr;

                auto root = Ogre::Root::getSingletonPtr();
                if( root )
                {
                    auto compositorManager = root->getCompositorManager2();
                    if( compositorManager )
                    {
                        // Clear the pass provider on the compositor manager before freeing the
                        // object so the compositor manager never holds a dangling raw pointer.
                        compositorManager->setCompositorPassProvider( nullptr );

                        compositorManager->removeAllWorkspaces();
                    }
                }

                // Safe to delete after the compositor manager no longer references it.
                if( m_passProvider )
                {
                    OGRE_DELETE m_passProvider;
                    m_passProvider = nullptr;
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CompositorManager::createMainShadowNode()
    {
        using namespace Ogre;

        auto root = Root::getSingletonPtr();
        if( !root )
            return;

        auto *pCompositorManager = root->getCompositorManager2();
        if( !pCompositorManager )
            return;

        const IdString shadowNodeName( "MainShadowNode" );
        if( pCompositorManager->hasShadowNodeDefinition( shadowNodeName ) )
            return;

        // PSSM splits for the directional light.
        const size_t numPssmSplits = 2u;
        const size_t numShadowMaps = numPssmSplits;
        const String atlasName = "MainShadowAtlas";

        CompositorShadowNodeDef *shadowNodeDef =
            pCompositorManager->addShadowNodeDefinition( "MainShadowNode" );
        if( !shadowNodeDef )
            return;

        shadowNodeDef->setNumLocalTextureDefinitions( 1u );

        TextureDefinitionBase::TextureDefinition *texDef =
            shadowNodeDef->addTextureDefinition( atlasName );
        if( !texDef )
            return;

        texDef->width = 2048u;
        texDef->height = 2048u;
        texDef->format = PFG_D32_FLOAT;
        texDef->depthBufferId = DepthBuffer::NO_POOL_EXPLICIT_RTV;
        texDef->depthBufferFormat = PFG_D32_FLOAT;
        texDef->preferDepthTexture = false;
        texDef->fsaa = "1";

        const Ogre::IdString atlasId( atlasName.c_str() );
        RenderTargetViewDef *rtv = shadowNodeDef->addRenderTextureView( atlasId );
        if( !rtv )
            return;

        Ogre::String ogreAtlasName = Ogre::String( atlasName.c_str(), atlasName.length() );
        rtv->setForTextureDefinition( ogreAtlasName, texDef );

        shadowNodeDef->setNumShadowTextureDefinitions( numShadowMaps );

        // Bind each PSSM split to a region of the atlas. The shadow node validator
        // applies matching viewport/scissor rectangles to the caster passes below.
        for( size_t split = 0; split < numPssmSplits; ++split )
        {
            const Real splitWidth = 1.0f / static_cast<Real>( numPssmSplits );
            const Ogre::Vector2 uvOffset( splitWidth * static_cast<Real>( split ), 0.0f );
            const Ogre::Vector2 uvLength( splitWidth, 1.0f );

            ShadowTextureDefinition *shadowTexDef = shadowNodeDef->addShadowTextureDefinition(
                0u, split, atlasName, uvOffset, uvLength, 0u );
            if( !shadowTexDef )
                return;

            shadowTexDef->shadowMapTechnique = SHADOWMAP_PSSM;
            shadowTexDef->numSplits = static_cast<uint32>( numPssmSplits );
            shadowTexDef->numStableSplits = 0u;
            shadowTexDef->pssmLambda = 0.95f;
            shadowTexDef->splitPadding = 1.0f;
            shadowTexDef->splitBlend = 0.125f;
            shadowTexDef->splitFade = 0.313f;
        }

        // Clear the atlas once, then render one caster pass per PSSM split.
        shadowNodeDef->setNumTargetPass( numShadowMaps + 1u );
        CompositorTargetDef *clearTargetDef = shadowNodeDef->addTargetPass( atlasName );
        if( !clearTargetDef )
            return;

        clearTargetDef->setNumPasses( 1u );
        auto *passClear = static_cast<CompositorPassClearDef *>( clearTargetDef->addPass( PASS_CLEAR ) );
        if( !passClear )
            return;

        for( size_t i = 0; i < numShadowMaps; ++i )
        {
            CompositorTargetDef *targetDef = shadowNodeDef->addTargetPass( atlasName );
            if( !targetDef )
                return;

            targetDef->setShadowMapSupportedLightTypes( 1u << Light::LT_DIRECTIONAL );
            targetDef->setNumPasses( 1u );

            auto *passScene = static_cast<CompositorPassSceneDef *>( targetDef->addPass( PASS_SCENE ) );
            if( !passScene )
                return;

            passScene->mShadowMapIdx = static_cast<uint32>( i );
            passScene->mIncludeOverlays = false;
        }
    }

    SmartPtr<Properties> CompositorManager::getProperties() const
    {
        return nullptr;
    }

    void CompositorManager::setProperties( SmartPtr<Properties> properties )
    {
    }

    Array<SmartPtr<ISharedObject>> CompositorManager::getChildObjects() const
    {
        Array<SmartPtr<ISharedObject>> objects;

        for( auto &compositor : m_compositors )
        {
            objects.push_back( compositor );
        }

        objects.push_back( m_sceneManager );
        objects.push_back( m_window );
        objects.push_back( m_camera );

        return objects;
    }

    void CompositorManager::removeCompositor( SmartPtr<Compositor> compositor )
    {
        if( compositor )
        {
            compositor->unload( nullptr );

            m_compositors.erase( std::remove( m_compositors.begin(), m_compositors.end(), compositor ),
                                 m_compositors.end() );
        }
    }

    auto CompositorManager::getSceneManager() const -> SmartPtr<IGraphicsScene>
    {
        return m_sceneManager;
    }

    void CompositorManager::setSceneManager( SmartPtr<IGraphicsScene> sceneManager )
    {
        m_sceneManager = sceneManager;
    }

    auto CompositorManager::getWindow() const -> SmartPtr<IGraphicsWindow>
    {
        return m_window;
    }

    void CompositorManager::setWindow( SmartPtr<IGraphicsWindow> window )
    {
        m_window = window;
    }

    auto CompositorManager::getCamera() const -> SmartPtr<IGraphicsCamera>
    {
        return m_camera;
    }

    void CompositorManager::setCamera( SmartPtr<IGraphicsCamera> camera )
    {
        m_camera = camera;
    }

    SmartPtr<Compositor> CompositorManager::addCompositor()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto compositor = factoryManager->make_ptr<Compositor>( this );
        m_compositors.push_back( compositor );

        graphicsSystem->loadObject( compositor );
        return compositor;
    }

    void CompositorManager::setupStateContext()
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

    void CompositorManager::StateListener::setOwner( SmartPtr<CompositorManager> owner )
    {
        m_owner = owner;
    }

    SmartPtr<CompositorManager> CompositorManager::StateListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    CompositorManager *CompositorManager::StateListener::getOwnerPtr() const
    {
        return m_owner.get();
    }

    bool CompositorManager::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwnerPtr() )
        {
            for( auto &compositor : owner->m_compositors )
            {
                if( compositor )
                {
                    if( compositor->handleStateChanged( state ) )
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool CompositorManager::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwnerPtr() )
        {
            for( auto &compositor : owner->m_compositors )
            {
                if( compositor )
                {
                    if( compositor->handleStateMessage( message ) )
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void CompositorManager::StateListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    CompositorManager::StateListener::~StateListener() = default;

    CompositorManager::StateListener::StateListener() = default;

}  // namespace workphone::render
