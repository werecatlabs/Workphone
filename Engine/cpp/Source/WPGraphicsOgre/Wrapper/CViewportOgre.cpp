#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CViewportOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CCameraOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreViewport.h>
#include <Ogre.h>
#include <OgreRTShaderSystem.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CViewportOgre, Viewport );

        u32 CViewportOgre::m_zOrderExt = 0;

        CViewportOgre::CViewportOgre()
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();

                auto stateManager = applicationManager->getStateManager();
                auto threadPool = applicationManager->getThreadPool();
                auto factoryManager = applicationManager->getFactoryManager();

                auto stateContext = stateManager->addStateContext();

                auto stateTask = graphicsSystem->getStateTask();
                stateContext->setOwner( this );
                stateContext->setTaskId( stateTask );

                auto viewportStateListener = factoryManager->make_ptr<ViewportStateListener>();
                viewportStateListener->setOwner( this );
                setStateListener( viewportStateListener );

                auto state = factoryManager->make_ptr<State>();
                stateContext->addState( state );
                stateContext->addStateListener( viewportStateListener );

                auto stateData = factoryManager->make_ptr<ViewportStateData>();
                state->setData( stateData );

                setStateContext( stateContext );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        CViewportOgre::~CViewportOgre()
        {
            unload( nullptr );
        }

        void CViewportOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                WP_ASSERT( Thread::getTaskFlag( Thread::Render_Flag ) );

                setLoadingState( LoadingState::Loading );

                auto camera = getCamera();

                Ogre::Camera *ogreCamera = nullptr;
                if( camera )
                {
                    camera->_getObject( (void **)&ogreCamera );
                }

                auto ZOrder = getZOrder();
                auto left = 0.0f;
                auto top = 0.0f;
                auto width = 1.0f;
                auto height = 1.0f;

                auto renderTarget = getRenderTarget();
                if( renderTarget )
                {
                    if( renderTarget->isDerived<IGraphicsWindow>() )
                    {
                        Ogre::RenderTarget *rt = nullptr;
                        renderTarget->_getObject( (void **)&rt );

                        if( rt )
                        {
                            //if( ZOrder == -1 )
                            {
                                ZOrder = renderTarget->getNumViewports() - 1;
                                if( ZOrder < 0 )
                                {
                                    ZOrder = 0;
                                }

                                ZOrder = m_zOrderExt++;
                            }

                            //WP_ASSERT( rt->getViewportByZOrder( ZOrder ) != nullptr );
                            auto vp = rt->addViewport( ogreCamera, ZOrder, left, top, width, height );
                            m_viewport = vp;
                        }
                    }
                }
                else
                {
                    auto window = getWindow();
                    Ogre::RenderWindow *ogreWindow = nullptr;
                    if( window )
                    {
                        window->_getObject( (void **)&ogreWindow );
                    }

                    if( ogreWindow )
                    {
                        //if (ZOrder == -1)
                        {
                            ZOrder = renderTarget->getNumViewports() - 1;
                            if( ZOrder < 0 )
                            {
                                ZOrder = 0;
                            }

                            ZOrder = m_zOrderExt++;
                        }

                        auto vp =
                            ogreWindow->addViewport( ogreCamera, ZOrder, left, top, width, height );
                        m_viewport = vp;
                    }
                }

#ifdef OGRE_BUILD_COMPONENT_RTSHADERSYSTEM
                if( auto vp = getViewport() )
                {
                    auto schemeName = Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME;
                    vp->setMaterialScheme( schemeName );
                }
#endif

                //if( auto vp = getViewport() )
                {
                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->setDirty( true );

                        if( auto state = stateContext->getStateByType<ViewportStateData>() )
                        {
                            //state->setZOrder( m_viewport->getZOrder() );
                        }
                    }
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CViewportOgre::reload( SmartPtr<ISharedObject> data )
        {
            unload( data );
            load( data );
        }

        void CViewportOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );

                removeViewportFromRT();

                m_viewport = nullptr;
                m_renderTarget = nullptr;

                m_window = nullptr;

                Viewport::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CViewportOgre::update()
        {
            Ogre::RenderTarget *rt = nullptr;

            if( auto renderTarget = getRenderTarget() )
            {
                renderTarget->_getObject( (void **)&rt );
            }

            if( auto vp = getViewport() )
            {
                WP_ASSERT( MathF::isFinite( vp->getWidth() ) );
                WP_ASSERT( MathF::isFinite( vp->getHeight() ) );
                WP_ASSERT( MathF::isFinite( vp->getTop() ) );
                WP_ASSERT( MathF::isFinite( vp->getLeft() ) );

                rt->fireViewportPreUpdate( vp );

                const auto actualWidth = vp->getActualWidth();
                const auto actualHeight = vp->getActualHeight();

                if( actualWidth > 0 && actualHeight > 0 )
                {
                    if( vp->isAutoUpdated() )
                    {
                        auto mask = static_cast<u32>( 0 );

                        mask = BitUtil::setFlagValue( mask, IGraphicsObject::UiFlag, getEnableUI() );
                        mask = BitUtil::setFlagValue( mask, IGraphicsObject::SceneFlag,
                                                      getEnableSceneRender() );

                        auto skyEnabled = getSkiesEnabled();
                        vp->setSkiesEnabled( skyEnabled );
                        vp->setVisibilityMask( mask );
                        //rt->_updateViewport( vp );

                        Ogre::Camera *camera = nullptr;

                        if( auto pCamera = getCamera() )
                        {
                            pCamera->_getObject( (void **)&camera );
                        }

                        //if( camera )
                        //{
                        //    camera->_renderScene( m_viewport );
                        //}

                        vp->setBackgroundColour( Ogre::ColourValue( 0.0f, 0.0f, 0.5f, 1.0f ) );
                        vp->update();
                    }
                }

                rt->fireViewportPostUpdate( vp );
            }
        }

        void CViewportOgre::_getObject( void **ppObject ) const
        {
            *ppObject = m_viewport.load();
        }

        bool CViewportOgre::ViewportStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();

                ScopedLock lock( graphicsSystem );

                if( auto owner = getOwner() )
                {
                    if( owner->isLoaded() )
                    {
                        Ogre::Viewport *vp = nullptr;
                        owner->_getObject( (void **)&vp );

                        //if (vp)
                        //{
                        //    vp->setCamera( nullptr );
                        //}

                        auto stateData = state->getData();
                        if( stateData->isDerived<ViewportStateData>() )
                        {
                            auto viewportState =
                                workphone::static_pointer_cast<ViewportStateData>( stateData );
                            if( viewportState )
                            {
                                auto dirty = false;

                                auto zorder = viewportState->zorder;
                                auto backgroundColour = viewportState->backgroundColour;
                                auto c = Ogre::ColourValue( backgroundColour.r, backgroundColour.g,
                                                            backgroundColour.b, backgroundColour.a );

                                Ogre::Camera *ogreCamera = nullptr;

                                auto pCamera = viewportState->camera;
                                if( pCamera )
                                {
                                    if( !pCamera->isLoaded() )
                                    {
                                        pCamera->load( nullptr );
                                    }

                                    if( pCamera->isLoaded() )
                                    {
                                        pCamera->_getObject( (void **)&ogreCamera );
                                    }
                                    else
                                    {
                                        dirty = true;
                                    }
                                }

                                auto vpZOrder = -1;

                                if( vp )
                                {
                                    vpZOrder = vp->getZOrder();
                                }

                                /*
                                if( BitUtil::getFlagValue( viewportState->flags,
                                                           render::IViewport::activeFlag ) )
                                {
                                    if( zorder != vpZOrder )
                                    {
                                        if( auto renderTarget = owner->getRenderTarget() )
                                        {
                                            Ogre::RenderTarget *rt = nullptr;
                                            renderTarget->_getObject( (void **)&rt );

                                            auto left = 0.0f;
                                            auto top = 0.0f;
                                            auto width = 1.0f;
                                            auto height = 1.0f;

                                            if( rt )
                                            {
                                                WP_ASSERT( rt->hasViewportWithZOrder( zorder ) ==
                                                           false );

                                                if( vpZOrder != -1 )
                                                {
                                                    if( vp )
                                                    {
                                                        vp->setCamera( nullptr );
                                                    }

                                                    auto rttVP = rt->getViewportByZOrder( zorder );
                                                    WP_ASSERT( rttVP == vp );
                                                    rttVP->setCamera( nullptr );

                                                    rt->removeViewport( vpZOrder );
                                                    vp = nullptr;
                                                    owner->setViewport( nullptr );
                                                }

                                                WP_ASSERT( rt->hasViewportWithZOrder( zorder ) ==
                                                           false );

                                                if( rt->hasViewportWithZOrder( zorder ) == true )
                                                {
                                                    auto rttVP = rt->getViewportByZOrder( zorder );
                                                    rttVP->setCamera( nullptr );

                                                    rt->removeViewport( zorder );
                                                    owner->setViewport( nullptr );
                                                }

                                                if( rt->hasViewportWithZOrder( zorder ) == false )
                                                {
                                                    vp = rt->addViewport( ogreCamera, zorder, left, top,
                                                                          width, height );
                                                    owner->setViewport( vp );
                                                }
                                            }
                                        }
                                    }

                                    if( vp )
                                    {
                                        vp->setBackgroundColour( c );

                                        auto overlaysEnabled = BitUtil::getFlagValue(
                                            viewportState->flags,
                                            render::IViewport::overlaysEnabledFlag );
                                        if( overlaysEnabled != vp->getOverlaysEnabled() )
                                        {
                                            vp->setOverlaysEnabled( overlaysEnabled );
                                        }
                                    }
                                }
                                */

                                /*
                                else
                                {
                                    if( vp )
                                    {
                                        vp->setCamera( nullptr );
                                    }

                                    if( vpZOrder != -1 )
                                    {
                                        if( auto renderTarget = owner->getRenderTarget() )
                                        {
                                            Ogre::RenderTarget *rt = nullptr;
                                            renderTarget->_getObject( (void **)&rt );

                                            if( rt )
                                            {
                                                auto rttVP = rt->getViewportByZOrder( vpZOrder );
                                                rttVP->setCamera( nullptr );

                                                rt->removeViewport( vpZOrder );
                                                owner->setViewport( nullptr );
                                            }
                                        }
                                    }
                                }
                                */

                                if( vp )
                                {
                                    vp->setCamera( ogreCamera );
                                }

                                viewportState->flags = BitUtil::setFlagValue(
                                    viewportState->flags, render::IViewport::activeFlag, dirty );
                            }
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                auto error = String( e.what() );
                WP_LOG_ERROR( error );
            }

            return false;
        }

        CViewportOgre::ViewportStateListener::ViewportStateListener() = default;

        CViewportOgre::ViewportStateListener::~ViewportStateListener() = default;

        bool CViewportOgre::ViewportStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        CViewportOgre *CViewportOgre::ViewportStateListener::getOwner() const
        {
            return m_owner;
        }

        void CViewportOgre::ViewportStateListener::setOwner( CViewportOgre *owner )
        {
            m_owner = owner;
        }

        Ogre::Viewport *CViewportOgre::getViewport() const
        {
            return m_viewport.load();
        }

        void CViewportOgre::setViewport( Ogre::Viewport *viewport )
        {
            m_viewport = viewport;
        }

        void CViewportOgre::removeViewportFromRT()
        {
            if( auto renderTarget = getRenderTarget() )
            {
                Ogre::RenderTarget *rt = nullptr;
                renderTarget->_getObject( (void **)&rt );

                if( auto vp = getViewport() )
                {
                    auto zorder = vp->getZOrder();

                    if( rt )
                    {
                        rt->removeViewport( zorder );
                    }
                }
            }
        }

    }  // end namespace render
}  // namespace workphone
