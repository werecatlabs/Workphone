#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCameraOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CViewportOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/GraphicsObjectListenerOgreNext.hpp>
#include <WPGraphicsOgreNext/CompositorManager.hpp>
#include <WPGraphicsOgreNext/Compositor.hpp>
#include <WPGraphicsOgreNext/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreCamera.h>
#include <OgreSceneManager.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CCameraOgreNext,
                               CGraphicsObjectOgreNext<GraphicsCamera> );

    CCameraOgreNext::CCameraOgreNext()
    {
        setupStateObject();
    }

    CCameraOgreNext::CCameraOgreNext( CGraphicsSceneOgreNext *creator )
    {
        m_creator = creator;

        setupStateObject();
    }

    CCameraOgreNext::~CCameraOgreNext()
    {
        destroyCompositor();
        destroyStateContext();
    }

    void CCameraOgreNext::setupStateObject()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManagerPtr();
        WP_ASSERT( stateManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = graphicsSystem->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto scene = getCreator();
        if( !scene )
        {
            // Default constructor path: m_creator is not set yet.
            // State context will be set up when the creator is assigned and load() is called.
            return;
        }

        auto stateContext = scene->getGraphicsObjectContext( getTypeInfo() );
        WP_ASSERT( stateContext );
        setStateContext( stateContext );

        auto state = factoryManager->make_ptr<State>();
        state->setId( getId() );
        state->setOwner( this );
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<GraphicsObjectData>();
        state->setData( stateData );

        auto frustumState = factoryManager->make_ptr<State>();
        frustumState->setId( getId() );
        frustumState->setOwner( this );
        stateContext->addState( frustumState );

        auto frustumStateData = factoryManager->make_ptr<FrustumStateData>();
        frustumState->setData( frustumStateData );

        auto cameraState = factoryManager->make_ptr<State>();
        cameraState->setId( getId() );
        cameraState->setOwner( this );
        stateContext->addState( cameraState );

        auto cameraStateData = factoryManager->make_ptr<CameraStateData>();
        cameraState->setData( cameraStateData );
    }

    void CCameraOgreNext::createCompositor()
    {
        if( m_compositor )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = (CGraphicsSystemOgreNext *)applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto compositorManager = graphicsSystem->getCompositorManager();
        WP_ASSERT( compositorManager );

        auto window = graphicsSystem->getDefaultWindow();

        auto compositor = compositorManager->addCompositor();
        compositor->setName( getName() + "_Compositor" );
        compositor->setWorkspaceName( "Camera_" + StringUtil::getUUID() );
        compositor->setWindow( window );
        compositor->setSceneManager( getCreator() );
        compositor->setCamera( this );
        setCompositor( compositor );

        graphicsSystem->loadObject( compositor );
    }

    void CCameraOgreNext::destroyCompositor()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = (CGraphicsSystemOgreNext *)applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto compositorManager = graphicsSystem->getCompositorManager();

        ScopedLock lock( this, true );

        if( m_compositor )
        {
            if( compositorManager )
            {
                compositorManager->removeCompositor( m_compositor );
            }

            m_compositor = nullptr;
        }
    }

    void CCameraOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            WP_ASSERT( isLoaded() == false );

            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            ScopedLock lock( this );

            Ogre::SceneManager *ogreSmgr = nullptr;
            if( auto creator = getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &ogreSmgr ) );
            }

            auto handle = getHandle();
            WP_ASSERT( handle );

            auto name = getName();

            if( ogreSmgr )
            {
                auto camera = ogreSmgr->createCamera( name.c_str() );
                setGraphicsObject( camera );
            }

            // auto parentNode = m_camera->getParentSceneNode();
            // if (parentNode)
            //{
            //	parentNode->detachObject(m_camera);
            // }

            // some defaults
            if( auto camera = getGraphicsObjectByType<Ogre::Camera>() )
            {
                camera->setPosition( Ogre::Vector3( 0.0f, 0.0f, 0.0f ) );
                camera->lookAt( Ogre::Vector3::ZERO );
                camera->setNearClipDistance( 0.02f );
                camera->setFarClipDistance( 10000.0f );
                camera->setAutoAspectRatio( true );
            }

            createCompositor();

            if( auto stateContext = getStateContext() )
            {
                stateContext->setDirty( true );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CCameraOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                //WP_ASSERT( !getOwner() );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto viewport = getViewport();

                ScopedLock lock( this );

                destroyCompositor();

                SmartPtr<CGraphicsSceneOgreNext> smgr = getCreator();
                if( smgr )
                {
                    auto ogreSmgr = smgr->getSceneManager();
                    if( ogreSmgr )
                    {
                        if( auto camera = getGraphicsObjectByType<Ogre::Camera>() )
                        {
                            ogreSmgr->destroyCamera( camera );
                            setGraphicsObject( nullptr );
                        }
                    }
                }

                if( viewport )
                {
                    viewport->setCamera( nullptr );
                }

                setTargetTexture( nullptr );

                if( viewport )
                {
                    viewport->unload( nullptr );
                }

                CGraphicsObjectOgreNext<GraphicsCamera>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto CCameraOgreNext::clone( const String &name ) const -> SmartPtr<IGraphicsObject>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto camera = factoryManager->make_ptr<CCameraOgreNext>();
        if( !name.empty() )
            camera->setName( name );
        else
            camera->setName( getName() );
        return camera;
    }

    void CCameraOgreNext::setProperties( SmartPtr<Properties> properties )
    {
        CGraphicsObjectOgreNext<GraphicsCamera>::setProperties( properties );

        bool visible = isVisible();
        properties->getPropertyValue( IGraphicsObject::visiblePropertyStr, visible );

        setVisible( visible );
    }

    auto CCameraOgreNext::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = CGraphicsObjectOgreNext<GraphicsCamera>::getChildObjects();

        objects.emplace_back( getCompositor() );
        objects.emplace_back( getViewport() );

        return objects;
    }

    auto CCameraOgreNext::getCompositor() const -> SmartPtr<Compositor>
    {
        return m_compositor;
    }

    void CCameraOgreNext::setCompositor( SmartPtr<Compositor> compositor )
    {
        m_compositor = compositor;
    }

    bool CCameraOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( message->getSenderPtr() == this )
        {
            if( message->isExactly<StateMessageVisible>() )
            {
                auto visibleMessage = workphone::static_pointer_cast<StateMessageVisible>( message );
                WP_ASSERT( visibleMessage );

                auto visible = visibleMessage->isVisible();
                this->setVisible( visible );

                return true;
            }
        }

        return false;
    }

    bool CCameraOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( isLoaded() )
        {
            auto result = CGraphicsObjectOgreNext<GraphicsCamera>::handleStateChanged( state );

            if( state->getOwnerPtr() == this )
            {
                if( auto stateData = state->getData() )
                {
                    if( stateData->isDerived<GraphicsObjectData>() )
                    {
                        auto graphicsObjectData = SafeReadPtr<GraphicsObjectData>( stateData );
                        auto visible = BitUtil::getFlagValue( graphicsObjectData->flags,
                                                              IGraphicsObject::visibleFlag );

                        auto applicationManager = core::IApplicationManager::instancePtr();
                        auto graphicsSystem =
                            (CGraphicsSystemOgreNext *)applicationManager->getGraphicsSystemPtr();
                        if( auto compositorManager = graphicsSystem->getCompositorManager() )
                        {
                            if( !compositorManager->isLoaded() )
                            {
                                compositorManager->load( nullptr );
                            }
                        }

                        if( auto compositor = this->getCompositor() )
                        {
                            if( !compositor->isLoaded() )
                            {
                                compositor->load( nullptr );
                            }

                            // Let compositor state and dependency events coalesce the workspace
                            // rebuild on the render task.
                            compositor->setEnabled( visible );
                        }

                        result = true;
                    }
                    else if( stateData->isDerived<FrustumStateData>() )
                    {
                        auto frustumStateData = SafeReadPtr<FrustumStateData>( stateData );

                        if( auto camera = this->getGraphicsObjectByType<Ogre::Camera>() )
                        {
                            camera->setNearClipDistance( frustumStateData->nearClipDistance );
                            camera->setFarClipDistance( frustumStateData->farClipDistance );
                            camera->setFOVy( Ogre::Radian( frustumStateData->fovy ) );

                            auto ratio = frustumStateData->aspectRatio;

                            if( ratio > MathF::epsilon() && ratio < static_cast<f32>( 1e8 ) )
                            {
                                camera->setAspectRatio( ratio );
                            }
                        }

                        result = true;
                    }
                    else if( stateData->isDerived<CameraStateData>() )
                    {
                        auto cameraStateData = SafeReadPtr<CameraStateData>( stateData );

                        auto applicationManager = core::IApplicationManager::instancePtr();
                        auto graphicsSystem =
                            (CGraphicsSystemOgreNext *)applicationManager->getGraphicsSystemPtr();
                        if( auto compositorManager = graphicsSystem->getCompositorManager() )
                        {
                            if( !compositorManager->isLoaded() )
                            {
                                compositorManager->load( nullptr );
                            }
                        }

                        if( auto camera = this->getGraphicsObjectByType<Ogre::Camera>() )
                        {
                            auto &w = cameraStateData->windowDimensions;
                            camera->setWindow( w.x, w.y, w.z, w.w );
                            camera->setLodBias( cameraStateData->lodBias );

                            auto autoAspectRatio = BitUtil::getFlagValue(
                                cameraStateData->flags, IGraphicsCamera::CameraFlagAutoAspectRatio );
                            camera->setAutoAspectRatio( autoAspectRatio );
                        }

                        if( auto compositor = this->getCompositor() )
                        {
                            if( !compositor->isLoaded() )
                            {
                                compositor->load( nullptr );
                            }

                            compositor->setEnabled( this->isVisible() );
                        }

                        result = true;
                    }
                }

                if( auto camera = this->getGraphicsObjectByType<Ogre::Camera>() )
                {
                    if( auto stateContext = getStateContextPtr() )
                    {
                        if( auto state = stateContext->invalidateStateDataById<FrustumStateData>(
                                getId(), false ) )
                        {
                            state->viewMatrix = OgreUtil::convert( camera->getViewMatrix() );
                            state->projectionMatrix = OgreUtil::convert( camera->getProjectionMatrix() );
                        }
                    }
                }
            }

            return result;
        }

        return false;
    }

    auto CCameraOgreNext::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = CGraphicsObjectOgreNext<GraphicsCamera>::getProperties();

        auto visible = isVisible();
        properties->setProperty( IGraphicsObject::visiblePropertyStr, visible );
        return properties;
    }
}  // namespace workphone::render
