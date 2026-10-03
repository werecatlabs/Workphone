#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderTargetOgreNext.hpp>
//#include <WPGraphicsOgreNext/Wrapper/CRenderTexture.hpp>
#include <WPGraphicsOgreNext/Wrapper/CViewportOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone::render
{

    template <class T>
    auto CRenderTargetOgreNext<T>::addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera,
                                                s32 ZOrder /*= 0*/, f32 left /*= 0.0f*/,
                                                f32 top /*= 0.0f */, f32 width /*= 1.0f*/,
                                                f32 height /*= 1.0f*/ ) -> SmartPtr<IViewport>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto viewport = factoryManager->make_ptr<CViewportOgreNext>();
            viewport->setRenderTarget( this );
            viewport->setCamera( camera );
            viewport->setViewportId( id );

            if( auto stateContext = this->getStateContext() )
            {
                if( auto data = stateContext->template getStateData<RenderTargetStateData>() )
                {
                    data->viewports.push_back( viewport );
                }
            }

            graphicsSystem->loadObject( viewport );
            return viewport;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    template <class T>
    void CRenderTargetOgreNext<T>::getStatistics( f32 &lastFPS, f32 &avgFPS, f32 &bestFPS,
                                                  f32 &worstFPS ) const
    {
    }

    template <class T>
    auto CRenderTargetOgreNext<T>::getRenderTargetStats() const -> IRenderTarget::RenderTargetStats
    {
        return {};
    }

    template <class T>
    void CRenderTargetOgreNext<T>::_getObject( void **ppObject ) const
    {
    }

    template <class T>
    void CRenderTargetOgreNext<T>::copyContentsToMemory( void *buffer, u32 size,
                                                         FrameBuffer bufferId /*= WP_AUTO */ )
    {
    }

    template <class T>
    void CRenderTargetOgreNext<T>::swapBuffers()
    {
        // m_renderTarget->swapBuffers();
    }

    template <class T>
    void CRenderTargetOgreNext<T>::setupStateObject()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateContext = stateManager->addStateContext();

            // auto sceneNodeStateListener = factoryManager->make_ptr<WindowStateListener>();
            // sceneNodeStateListener->setOwner( this );
            // m_stateListener = sceneNodeStateListener;
            // stateContext->addStateListener( m_stateListener );

            stateContext->setOwner( this );
            T::setStateContext( stateContext );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    template <class T>
    void CRenderTargetOgreNext<T>::destroyedStateObject()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        if( stateManager )
        {
            if( auto stateContext = T::getStateContext() )
            {
                stateContext->setOwner( nullptr );

                if( auto stateListener = T::getStateListener() )
                {
                    stateContext->removeStateListener( stateListener );
                    stateListener->unload( nullptr );
                    T::setStateListener( nullptr );
                }

                if( stateManager )
                {
                    stateManager->removeStateContext( stateContext );
                }

                T::setStateContext( nullptr );
            }
        }
    }

    template class CRenderTargetOgreNext<RenderTexture>;
    template class CRenderTargetOgreNext<GraphicsWindow>;

}  // namespace workphone::render
