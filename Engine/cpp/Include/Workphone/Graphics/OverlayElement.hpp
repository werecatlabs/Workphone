#ifndef _OverlayElement_H
#define _OverlayElement_H

#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Graphics/IOverlay.hpp>
#include <Workphone/Interface/Graphics/IOverlayElement.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/State/States/OverlayElementState.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class OverlayElement
         * @brief Base template for concrete overlay element implementations.
         *
         * OverlayElement<T> implements the common behaviour required by UI elements
         * such as panels, text labels, and images. It uses a state-context to
         * store runtime properties (captions, material, position, size, colour,
         * visibility, alignment, and z-order). Concrete backends provide the
         * rendering-specific behaviour by supplying T.
         *
         * @tparam T Concrete type implementing renderer-specific behaviour and
         *           satisfying the SharedGraphicsObject contract.
         */
        template <class T>
        class OverlayElement : public SharedGraphicsObject<T>
        {
        public:
            /**
             * @brief Nested state listener class for handling state changes.
             *
             * ElementStateListener monitors and responds to state messages and
             * state changes, forwarding them to the owning overlay element.
             */
            class ElementStateListener : public IStateListener
            {
            public:
                /// @brief Construct a listener with no owner assigned.
                ElementStateListener();

                /// @brief Destroy the listener.
                ~ElementStateListener() override;

                /** @copydoc IStateListener::unload */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle an incoming state message.
                 * @param message State message to inspect.
                 * @return true if the message was handled; false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle a state-change notification.
                 * @param state State object that changed.
                 * @return true if the state change was handled; false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the overlay element that owns this listener.
                 * @return Owner element, or nullptr if the owner has expired.
                 */
                SmartPtr<IOverlayElement> getOwner() const;

                /**
                 * @brief Set the overlay element that owns this listener.
                 * @param owner Owner element to track weakly.
                 */
                void setOwner( SmartPtr<IOverlayElement> owner );

            protected:
                /// Owning overlay element, held weakly to avoid a reference cycle.
                AtomicWeakPtr<IOverlayElement> m_owner;
            };

            /// @brief Construct a new OverlayElement instance.
            OverlayElement();

            /// @brief Destroy the OverlayElement and release resources.
            ~OverlayElement() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Apply a material to this element.
             * @param material Material to use when rendering this element.
             */
            void setMaterial( SmartPtr<IMaterial> material ) override;

            /**
             * @brief Retrieve the current material applied to this element.
             * @return Current material, or nullptr when no material is assigned.
             */
            SmartPtr<IMaterial> getMaterial() const override;

            /**
             * @brief Set the caption or text displayed by this element.
             * @param text The caption string to display.
             */
            void setCaption( const String &text ) override;

            /**
             * @brief Get the current caption text.
             * @return Current caption, or an empty string if none is set.
             */
            String getCaption() const override;

            /**
             * @brief Set whether this element is visible.
             * @param visible True to show the element, false to hide it.
             */
            void setVisible( bool visible ) override;

            /**
             * @brief Query visibility state of the element.
             * @return True if visible, false otherwise.
             */
            bool isVisible() const override;

            /**
             * @brief Get the element's 2D position.
             * @return Position expressed in the current metrics mode.
             */
            Vector2<real_Num> getPosition() const override;

            /**
             * @brief Set the element's 2D position.
             * @param position Position expressed in the current metrics mode.
             */
            void setPosition( const Vector2<real_Num> &position ) override;

            /**
             * @brief Get the element's size (width, height).
             * @return Size expressed in the current metrics mode.
             */
            Vector2<real_Num> getSize() const override;

            /**
             * @brief Set the element's size (width, height).
             * @param size Size expressed in the current metrics mode.
             */
            void setSize( const Vector2<real_Num> &size ) override;

            /**
             * @brief Get the element's z-order relative to sibling elements.
             * @return Current z-order value.
             */
            u32 getZOrder() const override;

            /**
             * @brief Set the element's z-order relative to siblings.
             * @param zOrder New z-order value.
             */
            void setZOrder( u32 zOrder ) override;

            /**
             * @brief Set the color tint applied to this element.
             * @param colour Colour to apply as a tint.
             */
            void setColour( const ColourF &colour ) override;

            /**
             * @brief Get the current colour tint.
             * @return Current colour tint, or white when no state is available.
             */
            ColourF getColour() const override;

            /**
             * @brief Set the metrics mode (pixels, relative, aspect-adjusted).
             * @param metricsMode Metrics mode enumerator value.
             */
            void setMetricsMode( u8 metricsMode ) override;

            /**
             * @brief Get the current metrics mode in use.
             * @return Current IOverlayElement::GuiMetricsMode value.
             */
            u8 getMetricsMode() const override;

            /**
             * @brief Set the horizontal alignment of the element.
             * @param gha Horizontal alignment enumerator value.
             */
            void setHorizontalAlignment( u8 gha ) override;

            /**
             * @brief Get the current horizontal alignment.
             * @return Current IOverlayElement::GuiHorizontalAlignment value.
             */
            u8 getHorizontalAlignment() const override;

            /**
             * @brief Set the vertical alignment of the element.
             * @param gva Vertical alignment enumerator value.
             */
            void setVerticalAlignment( u8 gva ) override;

            /**
             * @brief Get the current vertical alignment.
             * @return Current IOverlayElement::GuiVerticalAlignment value.
             */
            u8 getVerticalAlignment() const override;

            /**
             * @brief Determine whether this element can contain children.
             * @return true if this element is a container; false otherwise.
             */
            bool isContainer() const override;

            /**
             * @brief Retrieve the underlying backend graphics object pointer.
             * @param ppObject Output pointer that receives a backend-specific pointer.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get the overlay that owns this element.
             * @return Owning overlay, or nullptr when detached.
             */
            SmartPtr<IOverlay> getOverlay() const override;

            /**
             * @brief Set the parent overlay for this element.
             * @param overlay Overlay that will own this element.
             */
            void setOverlay( SmartPtr<IOverlay> overlay ) override;

            /**
             * @brief Get the parent element in the hierarchy.
             * @return Parent element, or nullptr if this is a root element.
             */
            SmartPtr<IOverlayElement> getParent() const override;

            /**
             * @brief Set the parent element in the hierarchy.
             * @param parent New parent element.
             */
            void setParent( SmartPtr<IOverlayElement> parent ) override;

            /**
             * @brief Add a child element to this element.
             * @param element Smart pointer to the child to add.
             */
            void addChild( SmartPtr<IOverlayElement> element ) override;

            /**
             * @brief Remove a child element from this element.
             * @param element Smart pointer to the child to remove.
             */
            void removeChild( SmartPtr<IOverlayElement> element ) override;

            /**
             * @brief Retrieve all child elements of this element.
             * @return Snapshot of the current child elements.
             */
            Array<SmartPtr<IOverlayElement>> getChildren() const override;

            /**
             * @brief Validate this element and its descendants.
             *
             * The default validation checks that the element has a parent and
             * that all children recursively report valid.
             *
             * @return true if the element and all children are valid; false otherwise.
             */
            bool isValid() const override;

            /**
             * @brief Export this element's properties into a Properties object.
             * @return Properties snapshot for serialization or inspection.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply a Properties collection to this element.
             * @param properties Properties to read values from.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get dependency child objects for resource tracking.
             * @return Objects referenced by this element for dependency tracking.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            WP_CLASS_REGISTER_TEMPLATE_DECL( OverlayElement, T );

        protected:
            /**
             * @brief Creates and initializes the state context for this element.
             *
             * Override this method to customize state context initialization.
             */
            virtual void createStateContext();

            /// Material name stored for serialization and property round-tripping.
            String m_materialName;

            /// Owning overlay, held weakly to avoid a reference cycle.
            AtomicWeakPtr<IOverlay> m_overlay;

            /// Parent element, held weakly to avoid a reference cycle.
            AtomicWeakPtr<IOverlayElement> m_parent;

            /// Child elements owned by this element.
            Array<SmartPtr<IOverlayElement>> m_children;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, OverlayElement, T, T );

        template <class T>
        OverlayElement<T>::OverlayElement() = default;

        template <class T>
        OverlayElement<T>::~OverlayElement() = default;

        template <class T>
        void OverlayElement<T>::load( SmartPtr<ISharedObject> data )
        {
        }

        template <class T>
        void OverlayElement<T>::unload( SmartPtr<ISharedObject> data )
        {
            if( auto overlay = getOverlay() )
            {
                overlay->removeElement( this );
                setOverlay( nullptr );
            }

            m_materialName.clear();
            m_parent = nullptr;
            m_children.clear();

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto stateManager = applicationManager->getStateManagerPtr();
            if( stateManager )
            {
                if( auto stateContext = this->getStateContext() )
                {
                    stateManager->removeStateContext( stateContext );
                    this->setStateContext( nullptr );
                }
            }
        }

        template <class T>
        void OverlayElement<T>::setMaterial( SmartPtr<IMaterial> material )
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<OverlayElementState>() )
                {
                    state->setMaterial( material );
                }
            }
        }

        template <class T>
        SmartPtr<IMaterial> OverlayElement<T>::getMaterial() const
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<OverlayElementState>() )
                {
                    return state->getMaterial();
                }
            }

            return nullptr;
        }

        template <class T>
        void OverlayElement<T>::setCaption( const String &text )
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<OverlayElementState>() )
                {
                    state->setCaption( text );
                }
            }
        }

        template <class T>
        String OverlayElement<T>::getCaption() const
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<OverlayElementState>() )
                {
                    return state->getCaption();
                }
            }

            return {};
        }

        template <class T>
        void OverlayElement<T>::setVisible( bool visible )
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<OverlayElementState>() )
                {
                    return state->setVisible( visible );
                }
            }
        }

        template <class T>
        bool OverlayElement<T>::isVisible() const
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<OverlayElementState>() )
                {
                    return state->isVisible();
                }
            }

            return false;
        }

        template <class T>
        Vector2<real_Num> OverlayElement<T>::getPosition() const
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<OverlayElementState>() )
                {
                    return state->getPosition();
                }
            }

            return Vector2<real_Num>::zero();
        }

        template <class T>
        void OverlayElement<T>::setPosition( const Vector2<real_Num> &position )
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<OverlayElementState>() )
                {
                    state->setPosition( position );
                }
            }
        }

        template <class T>
        Vector2<real_Num> OverlayElement<T>::getSize() const
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<OverlayElementState>() )
                {
                    return state->getSize();
                }
            }

            return Vector2<real_Num>::zero();
        }

        template <class T>
        void OverlayElement<T>::setSize( const Vector2<real_Num> &size )
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<OverlayElementState>() )
                {
                    state->setSize( size );
                }
            }
        }

        template <class T>
        u32 OverlayElement<T>::getZOrder() const
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<OverlayElementState>() )
                {
                    return state->getZOrder();
                }
            }

            return 0;
        }

        template <class T>
        void OverlayElement<T>::setZOrder( u32 zOrder )
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<OverlayElementState>() )
                {
                    state->setZOrder( zOrder );
                }
            }
        }

        template <class T>
        void OverlayElement<T>::setColour( const ColourF &colour )
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<OverlayElementState>() )
                {
                    state->setColour( colour );
                }
            }
        }

        template <class T>
        ColourF OverlayElement<T>::getColour() const
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<OverlayElementState>() )
                {
                    return state->getColour();
                }
            }

            return ColourF::White;
        }

        template <class T>
        void OverlayElement<T>::setMetricsMode( u8 metricsMode )
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<OverlayElementState>() )
                {
                    return state->setMetricsMode( metricsMode );
                }
            }
        }

        template <class T>
        u8 OverlayElement<T>::getMetricsMode() const
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<OverlayElementState>() )
                {
                    return state->getMetricsMode();
                }
            }

            return 0;
        }

        template <class T>
        void OverlayElement<T>::setHorizontalAlignment( u8 gha )
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<OverlayElementState>() )
                {
                    return state->setHorizontalAlignment( gha );
                }
            }
        }

        template <class T>
        u8 OverlayElement<T>::getHorizontalAlignment() const
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<OverlayElementState>() )
                {
                    return state->getHorizontalAlignment();
                }
            }

            return 0;
        }

        template <class T>
        void OverlayElement<T>::setVerticalAlignment( u8 gva )
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<OverlayElementState>() )
                {
                    return state->setVerticalAlignment( gva );
                }
            }
        }

        template <class T>
        u8 OverlayElement<T>::getVerticalAlignment() const
        {
            if( auto stateContext = SharedGraphicsObject<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<OverlayElementState>() )
                {
                    return state->getVerticalAlignment();
                }
            }

            return 0;
        }

        template <class T>
        bool OverlayElement<T>::isContainer() const
        {
            return false;
        }

        template <class T>
        void OverlayElement<T>::_getObject( void **ppObject ) const
        {
            *ppObject = nullptr;
        }

        template <class T>
        SmartPtr<IOverlay> OverlayElement<T>::getOverlay() const
        {
            auto p = m_overlay.load();
            return p.lock();
        }

        template <class T>
        void OverlayElement<T>::setOverlay( SmartPtr<IOverlay> overlay )
        {
            m_overlay = overlay;
        }

        template <class T>
        SmartPtr<IOverlayElement> OverlayElement<T>::getParent() const
        {
            auto p = m_parent.load();
            return p.lock();
        }

        template <class T>
        void OverlayElement<T>::setParent( SmartPtr<IOverlayElement> parent )
        {
            m_parent = parent;
        }

        template <class T>
        void OverlayElement<T>::addChild( SmartPtr<IOverlayElement> element )
        {
            if( element )
            {
                m_children.push_back( element );
            }
        }

        template <class T>
        void OverlayElement<T>::removeChild( SmartPtr<IOverlayElement> element )
        {
            if( element )
            {
                auto it = std::find( m_children.begin(), m_children.end(), element );
                if( it != m_children.end() )
                {
                    m_children.erase( it );
                }
            }
        }

        template <class T>
        Array<SmartPtr<IOverlayElement>> OverlayElement<T>::getChildren() const
        {
            return Array<SmartPtr<IOverlayElement>>( m_children.begin(), m_children.end() );
        }

        template <class T>
        bool OverlayElement<T>::isValid() const
        {
            if( auto parent = getParent() )
            {
                auto children = getChildren();
                for( auto child : children )
                {
                    if( !child->isValid() )
                    {
                        return false;
                    }
                }

                return true;
            }

            return false;
        }

        template <class T>
        SmartPtr<Properties> OverlayElement<T>::getProperties() const
        {
            auto properties = workphone::make_ptr<Properties>();
            properties->setProperty( IOverlayElement::materialNameStr, m_materialName );
            const auto colour = getColour();
            properties->setProperty( IOverlayElement::colourStr, colour );
            auto position = getPosition();
            properties->setProperty( IOverlayElement::positionStr, position );
            auto size = getSize();
            properties->setProperty( IOverlayElement::sizeStr, size );
            const auto zorder = getZOrder();
            properties->setProperty( IOverlayElement::zorderStr, zorder );
            const auto visible = isVisible();
            properties->setProperty( IOverlayElement::visibleStr, visible );
            return properties;
        }

        template <class T>
        void OverlayElement<T>::setProperties( SmartPtr<Properties> properties )
        {
            properties->getPropertyValue( IOverlayElement::materialNameStr, m_materialName );
        }

        template <class T>
        Array<SmartPtr<ISharedObject>> OverlayElement<T>::getChildObjects() const
        {
            Array<SmartPtr<ISharedObject>> objects;
            objects.reserve( 2 );

            objects.push_back( getOverlay() );
            objects.push_back( getParent() );

            return objects;
        }

        template <class T>
        void OverlayElement<T>::createStateContext()
        {
        }

        template <class T>
        OverlayElement<T>::ElementStateListener::ElementStateListener() = default;

        template <class T>
        OverlayElement<T>::ElementStateListener::~ElementStateListener() = default;

        template <class T>
        void OverlayElement<T>::ElementStateListener::unload( SmartPtr<ISharedObject> data )
        {
            m_owner = nullptr;
        }

        template <class T>
        bool OverlayElement<T>::ElementStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        template <class T>
        bool OverlayElement<T>::ElementStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        template <class T>
        SmartPtr<IOverlayElement> OverlayElement<T>::ElementStateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        template <class T>
        void OverlayElement<T>::ElementStateListener::setOwner( SmartPtr<IOverlayElement> owner )
        {
            m_owner = owner;
        }

    }  // end namespace render
}  // namespace workphone

#endif
