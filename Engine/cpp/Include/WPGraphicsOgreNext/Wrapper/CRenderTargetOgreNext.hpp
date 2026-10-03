#ifndef _CRenderTargetOgreNext_H
#define _CRenderTargetOgreNext_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone
{
    namespace render
    {

        template <class T>
        class CRenderTargetOgreNext : public T
        {
        public:
            CRenderTargetOgreNext();
            ~CRenderTargetOgreNext() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void swapBuffers() override;

            SmartPtr<IViewport> addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera,
                                             s32 ZOrder = 0, f32 left = 0.0f, f32 top = 0.0f,
                                             f32 width = 1.0f, f32 height = 1.0f ) override;

            void getStatistics( f32 &lastFPS, f32 &avgFPS, f32 &bestFPS, f32 &worstFPS ) const;

            IRenderTarget::RenderTargetStats getRenderTargetStats() const override;

            void _getObject( void **ppObject ) const override;

            void copyContentsToMemory( void *buffer, u32 size,
                                       FrameBuffer bufferId = FrameBuffer::Auto ) override;

            bool handleStateMessage( const SmartPtr<IStateMessage> &message );
            bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_TEMPLATE_DECL( CRenderTargetOgreNext, T );

        protected:
            /** Sets up the state object. */
            virtual void setupStateObject();

            virtual void destroyedStateObject();
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::render, CRenderTargetOgreNext, T, T );

        template <class T>
        CRenderTargetOgreNext<T>::CRenderTargetOgreNext()
        {
        }

        template <class T>
        CRenderTargetOgreNext<T>::~CRenderTargetOgreNext()
        {
        }

        template <class T>
        void CRenderTargetOgreNext<T>::load( SmartPtr<ISharedObject> data )
        {
        }

        template <class T>
        void CRenderTargetOgreNext<T>::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                auto viewports = T::getViewports();
                for( auto vp : viewports )
                {
                    vp->unload( nullptr );
                }

                viewports.clear();

                destroyedStateObject();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        bool CRenderTargetOgreNext<T>::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        template <class T>
        bool CRenderTargetOgreNext<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            //auto stateData = state->getData();
            //if( stateData->isDerived<RenderTargetStateData>() )
            //{
            //    auto renderTargetStateData =
            //        workphone::static_pointer_cast<RenderTargetStateData>( stateData );

            //    renderTargetStateData->size = size;
            //    renderTargetStateData->actualSize = size;
            //}

            return false;
        }

    }  // end namespace render
}  // namespace workphone

#endif
