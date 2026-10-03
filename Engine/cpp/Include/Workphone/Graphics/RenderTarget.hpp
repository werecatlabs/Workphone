#ifndef __WP_RenderTarget_h__
#define __WP_RenderTarget_h__

#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/State/States/RenderTargetStateData.hpp>
#include <Workphone/Core/BitUtil.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Base template implementation for a render target.
         *
         * This class provides a default, state-driven implementation of the
         * IRenderTarget interface. It's intended to be subclassed by
         * renderer-specific types (for example, a GPU/window back-end).
         * The template parameter `T` represents the underlying native object
         * type (if any) used by a concrete implementation.
         */
        template <typename T>
        class RenderTarget : public SharedGraphicsObject<T>
        {
        public:
            /**
             * @brief Construct a new RenderTarget object.
             *
             * The default constructor performs minimal initialization; most
             * state is managed via the associated state object.
             */
            RenderTarget();

            /**
             * @brief Destroy the RenderTarget object.
             */
            ~RenderTarget() override;

            /**
             * @brief Present or swap the target's front/back buffers.
             *
             * Concrete back-ends should override this to perform the actual
             * buffer swap. The default implementation is a no-op.
             */
            void swapBuffers() override;

            /**
             * @brief Set the rendering priority for this target.
             * @param priority Priority value (higher means rendered later).
             */
            void setPriority( u8 priority ) override;

            /**
             * @brief Get the rendering priority for this target.
             * @return u8 Current priority value.
             */
            u8 getPriority() const override;

            /**
             * @brief Check whether this render target is currently active.
             * @return true If active and should be processed by the renderer.
             */
            bool isActive() const override;

            /**
             * @brief Enable or disable this render target.
             * @param state True to mark active, false to disable.
             */
            void setActive( bool state ) override;

            /**
             * @brief Enable or disable automatic updates of this target.
             * @param autoupdate If true the target is auto-updated each frame.
             */
            void setAutoUpdated( bool autoupdate ) override;

            /**
             * @brief Query whether the target is automatically updated.
             * @return true If the target is auto-updated every frame.
             */
            bool isAutoUpdated() const override;

            /**
             * @brief Copy the contents of the target's framebuffer into memory.
             * @param buffer Destination memory buffer to receive pixel data.
             * @param size Size (in bytes) of the destination buffer.
             * @param bufferId Which framebuffer to read from; default is Auto.
             *
             * Implementations should ensure they do not write beyond
             * `size` bytes and should perform any necessary pixel format
             * conversions.
             */
            void copyContentsToMemory( void *buffer, u32 size,
                                       FrameBuffer bufferId = FrameBuffer::Auto ) override;

            /**
             * @brief Get the logical size of the render target in pixels.
             * @return Vector2I Width and height of the target.
             */
            Vector2I getSize() const override;

            /**
             * @brief Set the logical size of the render target.
             * @param size New width/height in pixels.
             */
            void setSize( const Vector2I &size ) override;

            /**
             * @brief Get the colour bit depth used by the target.
             * @return u32 Colour depth in bits.
             */
            u32 getColourDepth() const override;

            /**
             * @brief Set the colour bit depth for the target.
             * @param colourDepth Colour depth in bits.
             */
            void setColourDepth( u32 colourDepth ) override;

            /**
             * @brief Create and attach a new viewport to this target.
             * @param id Unique identifier for the viewport.
             * @param camera Camera used by the viewport.
             * @param ZOrder Z-order for rendering (lower values render first).
             * @param left Normalized left coordinate (0.0 - 1.0).
             * @param top Normalized top coordinate (0.0 - 1.0).
             * @param width Normalized width (0.0 - 1.0).
             * @param height Normalized height (0.0 - 1.0).
             * @return SmartPtr<IViewport> Newly created viewport or null on failure.
             */
            SmartPtr<IViewport> addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera,
                                             s32 ZOrder = -1, f32 left = 0.0f, f32 top = 0.0f,
                                             f32 width = 1.0f, f32 height = 1.0f ) override;

            /**
             * @brief Get the number of viewports attached to this target.
             * @return u32 Number of viewports.
             */
            u32 getNumViewports() const override;

            /**
             * @brief Retrieve a viewport by numeric index.
             * @param index Zero-based index of the viewport.
             * @return SmartPtr<IViewport> Viewport or null if out of range.
             */
            SmartPtr<IViewport> getViewport( u32 index ) override;

            /**
             * @brief Retrieve a viewport by its unique id.
             * @param id Identifier previously supplied to addViewport.
             * @return SmartPtr<IViewport> Matching viewport or null.
             */
            SmartPtr<IViewport> getViewportById( hash_type id ) override;

            /**
             * @brief Find a viewport by its Z-order.
             * @param zorder Z-order value to search for.
             * @return SmartPtr<IViewport> Matching viewport or null.
             */
            SmartPtr<IViewport> getViewportByZOrder( s32 zorder ) const override;

            /**
             * @brief Check whether a viewport exists with the given Z-order.
             * @param zorder Z-order to query.
             * @return true If a viewport with the given Z-order exists.
             */
            bool hasViewportWithZOrder( s32 zorder ) const override;

            /**
             * @brief Get a copy of the list of viewports attached to this target.
             * @return Array<SmartPtr<IViewport>> Copy of the viewports array.
             */
            Array<SmartPtr<IViewport>> getViewports() const override;

            /**
             * @brief Remove the specified viewport from this target.
             * @param vp Viewport to remove; no-op when null or not found.
             */
            void removeViewport( SmartPtr<IViewport> vp ) override;

            /**
             * @brief Remove all viewports from this render target.
             */
            void removeAllViewports() override;

            /**
             * @brief Retrieve runtime statistics for this render target.
             * @return IRenderTarget::RenderTargetStats Stats structure (may be empty).
             */
            IRenderTarget::RenderTargetStats getRenderTargetStats() const override;

            /**
             * @brief Internal: obtain the underlying native object pointer.
             * @param ppObject Pointer to receive the native object address.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Handle a state message forwarded from the state system.
             * @param message Message being delivered.
             * @return true if the message was handled, false to ignore.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Callback invoked when an associated state object changes.
             * @param state The state object that changed.
             * @return true if the change was handled.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_TEMPLATE_DECL( RenderTarget, T );

        protected:
            /**
             * @brief Create and register the object's state data.
             *
             * Subclasses should override to populate any renderer-specific
             * state fields required by their implementation.
             */
            virtual void setupStateObject();

            /**
             * @brief Extension ID counter used by derived classes when
             *        registering additional objects. Shared across template
             *        instantiations of this class.
             */
            static u32 m_idExt;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::render, RenderTarget, T,
                                            SharedGraphicsObject<T> );

        template <typename T>
        u32 RenderTarget<T>::m_idExt = 0;

        template <typename T>
        RenderTarget<T>::RenderTarget() = default;

        template <typename T>
        RenderTarget<T>::~RenderTarget() = default;

        template <typename T>
        void RenderTarget<T>::_getObject( void **ppObject ) const
        {
        }

        template <typename T>
        IRenderTarget::RenderTargetStats RenderTarget<T>::getRenderTargetStats() const
        {
            return {};
        }

        template <typename T>
        void RenderTarget<T>::removeAllViewports()
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data = stateContext->template invalidateStateDataById<RenderTargetStateData>(
                        this->getId() ) )
                {
                    data->viewports.clear();
                }
            }
        }

        template <typename T>
        void RenderTarget<T>::removeViewport( SmartPtr<IViewport> vp )
        {
            if( !vp )
            {
                return;
            }

            vp->unload( nullptr );

            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data = stateContext->template invalidateStateDataById<RenderTargetStateData>(
                        this->getId() ) )
                {
                    auto &viewports = data->viewports;
                    for( auto it = viewports.begin(); it != viewports.end(); ++it )
                    {
                        if( *it == vp )
                        {
                            data->viewports.erase( it );
                            break;
                        }
                    }
                }
            }
        }

        template <typename T>
        Array<SmartPtr<IViewport>> RenderTarget<T>::getViewports() const
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto stateManager = applicationManager->getStateManager();

            if( stateManager )
            {
                if( auto stateContext = RenderTarget<T>::getStateContext() )
                {
                    if( auto data = stateContext->template getStateDataById<RenderTargetStateData>(
                            this->getId() ) )
                    {
                        return data->viewports;
                    }
                }
            }

            return {};
        }

        template <typename T>
        bool RenderTarget<T>::hasViewportWithZOrder( s32 zorder ) const
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    for( const auto &viewport : data->viewports )
                    {
                        if( viewport && viewport->getZOrder() == zorder )
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        template <typename T>
        SmartPtr<IViewport> RenderTarget<T>::getViewportByZOrder( s32 zorder ) const
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    for( const auto &viewport : data->viewports )
                    {
                        if( viewport && viewport->getZOrder() == zorder )
                        {
                            return viewport;
                        }
                    }
                }
            }

            return nullptr;
        }

        template <typename T>
        SmartPtr<IViewport> RenderTarget<T>::getViewportById( hash_type id )
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    for( const auto &viewport : data->viewports )
                    {
                        if( viewport && viewport->getViewportId() == id )
                        {
                            return viewport;
                        }
                    }
                }
            }

            return nullptr;
        }

        template <typename T>
        SmartPtr<IViewport> RenderTarget<T>::getViewport( u32 index )
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    const auto &viewports = data->viewports;
                    if( index < viewports.size() )
                    {
                        return viewports[index];
                    }
                }
            }

            return nullptr;
        }

        template <typename T>
        u32 RenderTarget<T>::getNumViewports() const
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    return (u32)data->viewports.size();
                }
            }

            return 0;
        }

        template <typename T>
        SmartPtr<IViewport> RenderTarget<T>::addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera,
                                                          s32 ZOrder /*= -1*/, f32 left /*= 0.0f*/,
                                                          f32 top /*= 0.0f*/, f32 width /*= 1.0f*/,
                                                          f32 height /*= 1.0f */ )
        {
            return nullptr;  // Default implementation returns null
        }

        template <typename T>
        void RenderTarget<T>::setColourDepth( u32 colourDepth )
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data = stateContext->template invalidateStateDataById<RenderTargetStateData>(
                        this->getId() ) )
                {
                    data->colourDepth = colourDepth;
                }
            }
        }

        template <typename T>
        u32 RenderTarget<T>::getColourDepth() const
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    return data->colourDepth;
                }
            }

            return 0;
        }

        template <typename T>
        void RenderTarget<T>::setSize( const Vector2I &size )
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data = stateContext->template invalidateStateDataById<RenderTargetStateData>(
                        this->getId() ) )
                {
                    data->size = size;
                }
            }
        }

        template <typename T>
        Vector2I RenderTarget<T>::getSize() const
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    return data->size;
                }
            }

            return {};
        }

        template <typename T>
        void RenderTarget<T>::copyContentsToMemory( void *buffer, u32 size,
                                                    FrameBuffer bufferId /*= FrameBuffer::Auto */ )
        {
        }

        template <typename T>
        bool RenderTarget<T>::isAutoUpdated() const
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    return BitUtil::getFlagValue( data->flags, IRenderTarget::autoupdatedFlag );
                }
            }

            return false;
        }

        template <typename T>
        void RenderTarget<T>::setAutoUpdated( bool autoupdate )
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    data->flags =
                        BitUtil::setFlagValue( data->flags, IRenderTarget::autoupdatedFlag, autoupdate );
                }
            }
        }

        template <typename T>
        void RenderTarget<T>::setActive( bool state )
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data = stateContext->template invalidateStateDataById<RenderTargetStateData>(
                        this->getId() ) )
                {
                    data->flags = BitUtil::setFlagValue( data->flags, IRenderTarget::activeFlag, state );
                }
            }
        }

        template <typename T>
        bool RenderTarget<T>::isActive() const
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    return BitUtil::getFlagValue( data->flags, IRenderTarget::activeFlag );
                }
            }

            return false;
        }

        template <typename T>
        u8 RenderTarget<T>::getPriority() const
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data =
                        stateContext->template getStateDataById<RenderTargetStateData>( this->getId() ) )
                {
                    return (u8)data->priority;
                }
            }

            return 0;
        }

        template <typename T>
        void RenderTarget<T>::setPriority( u8 priority )
        {
            if( auto stateContext = RenderTarget<T>::getStateContext() )
            {
                if( auto data = stateContext->template invalidateStateDataById<RenderTargetStateData>(
                        this->getId() ) )
                {
                    data->priority = priority;
                }
            }
        }

        template <typename T>
        void RenderTarget<T>::swapBuffers()
        {
        }

        template <typename T>
        bool RenderTarget<T>::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        template <typename T>
        bool RenderTarget<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            auto viewports = getViewports();
            for( auto &viewport : viewports )
            {
                if( viewport )
                {
                    if( viewport->handleStateChanged( state ) )
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        template <typename T>
        void RenderTarget<T>::setupStateObject()
        {
        }

    }  // namespace render
}  // namespace workphone

#endif  // RenderTarget_h__
