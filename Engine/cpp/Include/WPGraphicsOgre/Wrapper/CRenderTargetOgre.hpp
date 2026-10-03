#ifndef _CRenderTargetOgre_H
#define _CRenderTargetOgre_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <WPGraphicsOgre/Wrapper/CViewportOgre.hpp>
#include <OgreRenderTarget.h>
#include <OgreViewport.h>

namespace workphone
{
    namespace render
    {

        /** Implements IRenderTarget interface for Ogre. */
        template <class T>
        class CRenderTargetOgre : public T
        {
        public:
            /** Constructor. */
            CRenderTargetOgre();

            /** Destructor. */
            virtual ~CRenderTargetOgre() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::preUpdate */
            void preUpdate() override;

            /** @copydoc ISharedObject::update */
            void update() override;

            /** @copydoc ISharedObject::postUpdate */
            void postUpdate() override;

            /** @copydoc IRenderTarget::swapBuffers */
            void swapBuffers() override;

            /** @copydoc IRenderTarget::setPriority */
            void setPriority( u8 priority ) override;

            /** @copydoc IRenderTarget::getPriority */
            u8 getPriority() const override;

            /** @copydoc IRenderTarget::isActive */
            bool isActive() const override;

            /** @copydoc IRenderTarget::setActive */
            void setActive( bool state ) override;

            /** @copydoc IRenderTarget::setAutoUpdated */
            void setAutoUpdated( bool autoupdate ) override;

            /** @copydoc IRenderTarget::isAutoUpdated */
            bool isAutoUpdated() const override;

            /** @copydoc IRenderTarget::getMetrics */
            void getMetrics( u32 &width, u32 &height, u32 &colourDepth );

            /** @copydoc IRenderTarget::addViewport */
            SmartPtr<IViewport> addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera, s32 ZOrder = -1,
                                             f32 left = 0.0f, f32 top = 0.0f, f32 width = 1.0f,
                                             f32 height = 1.0f ) override;

            /** @copydoc IRenderTarget::getPriority */
            u32 getNumViewports() const override;

            /** @copydoc IRenderTarget::getPriority */
            SmartPtr<IViewport> getViewport( u32 index ) override;

            /** @copydoc IRenderTarget::getPriority */
            SmartPtr<IViewport> getViewportById( hash_type id ) override;

            /** @copydoc IRenderTarget::getViewportByZOrder */
            SmartPtr<IViewport> getViewportByZOrder( s32 zorder ) const override;

            /** @copydoc IRenderTarget::hasViewportWithZOrder */
            bool hasViewportWithZOrder( s32 zorder ) const override;

            /** @copydoc IRenderTarget::getPriority */
            Array<SmartPtr<IViewport>> getViewports() const override;

            /** @copydoc IRenderTarget::getPriority */
            void removeViewport( SmartPtr<IViewport> vp ) override;

            /** @copydoc IRenderTarget::getPriority */
            void removeAllViewports() override;

            /** @copydoc IRenderTarget::getPriority */
            void getStatistics( f32 &lastFPS, f32 &avgFPS, f32 &bestFPS, f32 &worstFPS ) const;

            /** @copydoc IRenderTarget::getPriority */
            IRenderTarget::RenderTargetStats getRenderTargetStats() const override;

            /** @copydoc IRenderTarget::getPriority */
            void _getObject( void **ppObject ) const override;

            /** @copydoc IRenderTarget::getPriority */
            void copyContentsToMemory( void *buffer, u32 size,
                                       FrameBuffer bufferId = FrameBuffer::Auto ) override;

            /** @copydoc IRenderTarget::getSize */
            Vector2I getSize() const override;

            /** @copydoc IRenderTarget::setSize */
            void setSize( const Vector2I &size ) override;

            /** @copydoc IRenderTarget::getColourDepth */
            u32 getColourDepth() const override;

            /** @copydoc IRenderTarget::setColourDepth */
            void setColourDepth( u32 colourDepth ) override;

            /** @copydoc IRenderTarget::getSwapBuffers */
            bool getSwapBuffers() const;

            /** @copydoc IRenderTarget::setSwapBuffers */
            void setSwapBuffers( bool swapBuffers );

            /** Gets the render target object. */
            Ogre::RenderTarget *getRenderTarget() const;

            /** Sets the render target object. */
            void setRenderTarget( Ogre::RenderTarget *renderTarget );

            /** Gets the state object. */
            SmartPtr<IStateContext> getStateContext() const;

            /** Sets the state object. */
            void setStateContext( SmartPtr<IStateContext> stateContext );

            /** Gets the state listener. */
            SmartPtr<IStateListener> getStateListener() const;

            /** Sets the state listener. */
            void setStateListener( SmartPtr<IStateListener> stateListener );

            /** @copydoc IRenderTarget::getTexture */
            SmartPtr<ITexture> getTexture() const;

            /** @copydoc IRenderTarget::setTexture */
            void setTexture( SmartPtr<ITexture> texture );

            WP_CLASS_REGISTER_TEMPLATE_DECL( CRenderTargetOgre, T );

        protected:
            /** Sets up the state object. */
            virtual void setupStateObject();

            /** Destroys the state object. */
            virtual void destroyedStateObject();

            WeakPtr<ITexture> m_texture;

            AtomicSmartPtr<IStateContext> m_stateContext;
            AtomicSmartPtr<IStateListener> m_stateListener;

            Ogre::RenderTarget *m_renderTarget = nullptr;

            Array<SmartPtr<IViewport>> m_viewports;

            Vector2I m_size;
            atomic_u32 m_colourDepth = 0;
            atomic_bool m_swapBuffers = true;
            atomic_bool m_isActive = true;

            static s32 m_ext;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, CRenderTargetOgre, T, T );

        template <class T>
        s32 CRenderTargetOgre<T>::m_ext = 0;

        template <class T>
        CRenderTargetOgre<T>::CRenderTargetOgre()
        {
        }

        template <class T>
        CRenderTargetOgre<T>::~CRenderTargetOgre()
        {
            unload( nullptr );
            destroyedStateObject();
        }

        template <class T>
        void CRenderTargetOgre<T>::load( SmartPtr<ISharedObject> data )
        {
        }

        template <class T>
        void CRenderTargetOgre<T>::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                for( auto vp : m_viewports )
                {
                    vp->unload( nullptr );
                }

                m_viewports.clear();

                if( m_renderTarget )
                {
                    m_renderTarget->removeAllViewports();
                    m_renderTarget = nullptr;
                }

                m_texture = nullptr;
                m_renderTarget = nullptr;

                destroyedStateObject();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void CRenderTargetOgre<T>::preUpdate()
        {
            try
            {
                if( m_renderTarget )
                {
                    auto size = m_renderTarget->getNumViewports();
                    WP_ASSERT( size <= m_viewports.size() );

                    for( size_t i = 0; i < size; ++i )
                    {
                        auto vp = m_renderTarget->getViewport( (u16)i );
                        WP_ASSERT( vp );

                        auto actualWidth = vp->getActualWidth();
                        auto actualHeight = vp->getActualHeight();

                        if( actualWidth <= 0 || actualHeight <= 0 )
                        {
                            vp->_updateDimensions();
                        }
                    }

                    for( size_t i = 0; i < size; ++i )
                    {
                        auto vp = m_renderTarget->getViewport( (u16)i );
                        WP_ASSERT( vp );

                        auto actualWidth = vp->getActualWidth();
                        auto actualHeight = vp->getActualHeight();

                        if( actualWidth > 0 && actualHeight > 0 )
                        {
                            vp->clear();
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void CRenderTargetOgre<T>::update()
        {
            try
            {
                if( m_renderTarget )
                {
                    m_renderTarget->_beginUpdate();

                    for( auto vp : m_viewports )
                    {
                        vp->update();
                    }

                    m_renderTarget->_endUpdate();
                }
                else
                {
                    WP_LOG_ERROR( "CRenderTarget<T>::update renderTarget null" );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void CRenderTargetOgre<T>::postUpdate()
        {
            try
            {
                if( m_renderTarget )
                {
                    auto swapBuffers = getSwapBuffers();
                    m_renderTarget->swapBuffers();
                }
                else
                {
                    WP_LOG_ERROR( "CRenderTarget<T>::postUpdate renderTarget null" );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void CRenderTargetOgre<T>::getMetrics( u32 &width, u32 &height, u32 &colourDepth )
        {
            if( m_renderTarget )
            {
                m_renderTarget->getMetrics( width, height );
            }
        }

        template <class T>
        u32 CRenderTargetOgre<T>::getColourDepth() const
        {
            return 0;
        }

        template <class T>
        SmartPtr<IViewport> CRenderTargetOgre<T>::addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera,
                                                               s32 ZOrder /*= 0*/, f32 left /*= 0.0f*/,
                                                               f32 top /*= 0.0f */, f32 width /*= 1.0f*/,
                                                               f32 height /*= 1.0f*/ )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();

                if( ZOrder == -1 )
                {
                    ZOrder = m_ext++;
                }

                auto viewport = factoryManager->make_ptr<CViewportOgre>();

                auto handle = viewport->getHandle();
                if( handle )
                {
                    handle->setId( id );
                }

                viewport->setRenderTarget( this );
                viewport->setZOrder( ZOrder );
                viewport->setPosition( Vector2F( left, top ) );
                viewport->setSize( Vector2F( width, height ) );

                viewport->setCamera( camera );
                m_viewports.push_back( viewport );

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
        u32 CRenderTargetOgre<T>::getNumViewports() const
        {
            return (u32)m_viewports.size();
        }

        template <class T>
        SmartPtr<IViewport> CRenderTargetOgre<T>::getViewport( u32 index )
        {
            WP_ASSERT( index < (u32)m_viewports.size() );

            if( index < (u32)m_viewports.size() )
            {
                return m_viewports[index];
            }

            return nullptr;
        }

        template <class T>
        SmartPtr<IViewport> CRenderTargetOgre<T>::getViewportById( hash_type id )
        {
            for( auto vp : m_viewports )
            {
                if( auto handle = vp->getHandle() )
                {
                    if( handle->getId() == id )
                    {
                        return vp;
                    }
                }
            }

            return nullptr;
        }

        template <class T>
        SmartPtr<IViewport> CRenderTargetOgre<T>::getViewportByZOrder( s32 zorder ) const
        {
            for( auto vp : m_viewports )
            {
                if( vp )
                {
                    if( vp->getZOrder() == zorder )
                    {
                        return vp;
                    }
                }
            }

            return nullptr;
        }

        template <class T>
        bool CRenderTargetOgre<T>::hasViewportWithZOrder( s32 zorder ) const
        {
            for( auto vp : m_viewports )
            {
                if( vp )
                {
                    if( vp->getZOrder() == zorder )
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        template <class T>
        void CRenderTargetOgre<T>::removeViewport( SmartPtr<IViewport> vp )
        {
            try
            {
                auto zorder = vp->getZOrder();

                if( m_renderTarget )
                {
                    m_renderTarget->removeViewport( zorder );
                }
                else
                {
                    WP_LOG_ERROR( "CRenderTarget<T>::removeViewport renderTarget null" );
                }

                vp->unload( nullptr );

                auto it = std::find( m_viewports.begin(), m_viewports.end(), vp );
                if( it != m_viewports.end() )
                {
                    m_viewports.erase( it );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void CRenderTargetOgre<T>::removeAllViewports()
        {
            try
            {
                if( m_renderTarget )
                {
                    m_renderTarget->removeAllViewports();
                }
                else
                {
                    WP_LOG_ERROR( "CRenderTarget<T>::removeAllViewports renderTarget null" );
                }

                for( auto vp : m_viewports )
                {
                    vp->unload( nullptr );
                }

                m_viewports.clear();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void CRenderTargetOgre<T>::getStatistics( f32 &lastFPS, f32 &avgFPS, f32 &bestFPS,
                                                  f32 &worstFPS ) const
        {
            if( m_renderTarget )
            {
                auto frameStats = m_renderTarget->getStatistics();

                lastFPS = frameStats.lastFPS;
                avgFPS = frameStats.avgFPS;
                bestFPS = frameStats.bestFPS;
                worstFPS = frameStats.worstFPS;
            }
            else
            {
                WP_LOG_ERROR( "CRenderTarget<T>::getStatistics renderTarget null" );
            }
        }

        template <class T>
        IRenderTarget::RenderTargetStats CRenderTargetOgre<T>::getRenderTargetStats() const
        {
            auto stats = IRenderTarget::RenderTargetStats();

            if( m_renderTarget )
            {
                auto frameStats = m_renderTarget->getStatistics();

                stats.lastFPS = frameStats.lastFPS;
                stats.avgFPS = frameStats.avgFPS;
                stats.bestFPS = frameStats.bestFPS;
                stats.worstFPS = frameStats.worstFPS;
            }
            else
            {
                WP_LOG_ERROR( "CRenderTarget<T>::getRenderTargetStats renderTarget null" );
            }

            return stats;
        }

        template <class T>
        void CRenderTargetOgre<T>::_getObject( void **ppObject ) const
        {
            *ppObject = m_renderTarget;
        }

        template <class T>
        void CRenderTargetOgre<T>::setPriority( u8 priority )
        {
            m_renderTarget->setPriority( priority );
        }

        template <class T>
        u8 CRenderTargetOgre<T>::getPriority() const
        {
            if( m_renderTarget )
            {
                return m_renderTarget->getPriority();
            }

            return 0;
        }

        template <class T>
        bool CRenderTargetOgre<T>::isActive() const
        {
            if( m_renderTarget )
            {
                return m_renderTarget->isActive();
            }

            return false;
        }

        template <class T>
        void CRenderTargetOgre<T>::setActive( bool state )
        {
            if( m_renderTarget )
            {
                m_renderTarget->setActive( state );
            }
        }

        template <class T>
        void CRenderTargetOgre<T>::setAutoUpdated( bool autoupdate )
        {
            if( m_renderTarget )
            {
                m_renderTarget->setAutoUpdated( autoupdate );
            }
        }

        template <class T>
        bool CRenderTargetOgre<T>::isAutoUpdated() const
        {
            if( m_renderTarget )
            {
                return m_renderTarget->isAutoUpdated();
            }

            return false;
        }

        template <class T>
        void CRenderTargetOgre<T>::copyContentsToMemory( void *buffer, u32 size,
                                                         FrameBuffer bufferId /*= WP_AUTO */ )
        {
            // m_renderTarget->copyContentsToMemory( buffer, size, bufferId );
        }

        template <class T>
        void CRenderTargetOgre<T>::swapBuffers()
        {
            if( m_renderTarget )
            {
                m_renderTarget->swapBuffers();
            }
        }

        template <class T>
        Array<SmartPtr<IViewport>> CRenderTargetOgre<T>::getViewports() const
        {
            return m_viewports;
        }

        template <class T>
        Vector2I CRenderTargetOgre<T>::getSize() const
        {
            if( auto texture = getTexture() )
            {
                return texture->getSize();
            }

            return m_size;
        }

        template <class T>
        void CRenderTargetOgre<T>::setSize( const Vector2I &size )
        {
            if( auto texture = getTexture() )
            {
                texture->setSize( size );
            }
            else
            {
                auto targetSize = getSize();
                if( targetSize != size )
                {
                    auto applicationManager = core::IApplicationManager::instance();
                    WP_ASSERT( applicationManager );

                    m_size = size;
                }
            }
        }

        template <class T>
        void CRenderTargetOgre<T>::setColourDepth( u32 colourDepth )
        {
        }

        template <class T>
        bool CRenderTargetOgre<T>::getSwapBuffers() const
        {
            return m_swapBuffers;
        }

        template <class T>
        void CRenderTargetOgre<T>::setSwapBuffers( bool swapBuffers )
        {
            m_swapBuffers = swapBuffers;
        }

        template <class T>
        void CRenderTargetOgre<T>::setRenderTarget( Ogre::RenderTarget *renderTarget )
        {
            if( m_renderTarget != renderTarget )
            {
                m_renderTarget = renderTarget;

                for( auto vp : m_viewports )
                {
                    vp->setRenderTarget( this );
                    vp->load( nullptr );
                }
            }
        }

        template <class T>
        Ogre::RenderTarget *CRenderTargetOgre<T>::getRenderTarget() const
        {
            return m_renderTarget;
        }

        template <class T>
        SmartPtr<IStateContext> CRenderTargetOgre<T>::getStateContext() const
        {
            return m_stateContext;
        }

        template <class T>
        void CRenderTargetOgre<T>::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        template <class T>
        SmartPtr<IStateListener> CRenderTargetOgre<T>::getStateListener() const
        {
            return m_stateListener;
        }

        template <class T>
        void CRenderTargetOgre<T>::setStateListener( SmartPtr<IStateListener> stateListener )
        {
            m_stateListener = stateListener;
        }

        template <class T>
        void CRenderTargetOgre<T>::setupStateObject()
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto stateManager = applicationManager->getStateManager();
                WP_ASSERT( stateManager );

                auto stateContext = stateManager->addStateContext();

                stateContext->setOwner( this );
                setStateContext( stateContext );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void CRenderTargetOgre<T>::destroyedStateObject()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            if( auto stateManager = applicationManager->getStateManager() )
            {
                if( auto stateContext = getStateContext() )
                {
                    stateContext->setOwner( nullptr );

                    if( auto stateListener = getStateListener() )
                    {
                        stateContext->removeStateListener( stateListener );
                    }

                    stateManager->removeStateContext( stateContext );

                    stateContext->unload( nullptr );
                    setStateContext( nullptr );
                }
            }

            if( auto stateListener = getStateListener() )
            {
                stateListener->unload( nullptr );
                setStateListener( nullptr );
            }
        }

        template <class T>
        SmartPtr<ITexture> CRenderTargetOgre<T>::getTexture() const
        {
            return m_texture.lock();
        }

        template <class T>
        void CRenderTargetOgre<T>::setTexture( SmartPtr<ITexture> texture )
        {
            m_texture = texture;
        }

    }  // end namespace render
}  // namespace workphone

#endif
