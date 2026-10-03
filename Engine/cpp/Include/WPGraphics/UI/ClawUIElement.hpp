#ifndef _WP_GUIElement_H
#define _WP_GUIElement_H

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/IRenderer.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/State/Messages/StateMessageObject.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/States/UIElementStateData.hpp>
#include <Workphone/State/States/UITransformStateData.hpp>
#include <Workphone/System/Prototype.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone
{
    namespace ui
    {
        /** Non-template rendering interface shared by every WPGraphics UI element. */
        class IWorkphoneWidget
        {
        public:
            virtual ~IWorkphoneWidget() = default;
            virtual void draw( struct wp_context *ctx ) = 0;
            virtual const String &getWorkphoneComponentId() const = 0;
        };

        /** A base gui element.
         */
        template <class T>
        class ClawUIElement : public core::Prototype<T>, public IWorkphoneWidget
        {
        public:
            /** Default constructor. */
            ClawUIElement();

            /** Virtual destructor. */
            ~ClawUIElement() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            /** Used to initialise the gui element with default values. */
            virtual void initialise( SmartPtr<IUIElement> &parent );

            /** Used to initialise the gui element. */
            virtual void initialise( SmartPtr<IUIElement> &parent, const Properties &properties );

            /** Called the update the gui item. */
            void update() override;

            /**
             * @brief Draw this element using the WorkphoneCore immediate-mode context.
             *
             * Called each render frame while the element is visible and enabled.
             *
             * @param ctx The active WorkphoneCore Nuklear-style context.
             */
            void draw( struct wp_context *ctx ) override;

            const String &getWorkphoneComponentId() const override;

            /** Called when an input event has occurred. */
            virtual bool handleEvent( const SmartPtr<IInputEvent> &event );

            String getLabel() const override;
            void setLabel( const String &label ) override;

            virtual hash_type getElementId() const;

            virtual void setElementId( hash_type id );

            /** Gets the hash id of the gui element. */
            u32 getHashId() const;

            /** Sets the component id. */
            void setComponentID( const String &componentId );

            /** Gets the component id. */
            const String &getComponentID() const;

            /** Gets the type of the gui item. */
            const String &getType() const;

            /** Sets the position of the gui item. */
            virtual void setPosition( const Vector2F &position );

            /** Returns the position of the gui item. */
            Vector2F getPosition() const;

            /** Gets the absolute position of the gui item. */
            Vector2F getAbsolutePosition() const;

            /** Sets the size of the gui item. */
            virtual void setSize( const Vector2F &size );

            /** Gets the size of the gui item. */
            Vector2F getSize() const;

            f32 getScale() const;

            void setScale( f32 scale );

            /** Sets whether this gui item is enabled. */
            void setEnabled( bool enabled, bool cascade = true );

            /** Sets whether this gui item is enabled. */
            bool isEnabled() const;

            /** Sets whether this gui item is selected. */
            void setSelected( bool selected );

            /** Checks if this gui item is selected. */
            bool isSelected() const;

            void setHovered( bool hovered );

            bool isHovered() const;

            /** Sets whether the gui item is visible or not. */
            virtual void setVisible( bool isVisible, bool cascade = true );

            /** Returns a boolean value indicating whether or not the ui item is visible. */
            bool isVisible() const;

            /** Sets whether this gui item is in focus. */
            virtual void setFocus( bool hasFocus );

            /** Checks if the this gui item is in focus. */
            bool isInFocus() const;

            /** Sets this gui item as being highlighted. */
            void setHighlighted( bool isHighlighted, bool cascade = true );

            /** Check if this gui item is highlighted. */
            bool isHighlighted() const;

            SmartPtr<IUIElement> getParent() const;

            void setParent( SmartPtr<IUIElement> parent );

            /** Gets the children of the gui item. */
            Array<SmartPtr<IUIElement>> getChildren() const;

            u32 getNumChildren() const override;

            void setChildren( ConcurrentArray<SmartPtr<IUIElement>> children );

            virtual ConcurrentArray<SmartPtr<IUIElement>> &getChildrenRef();
            virtual const ConcurrentArray<SmartPtr<IUIElement>> &getChildrenRef() const;

            /** Adds a child to this gui item. */
            void addChild( SmartPtr<IUIElement> child ) override;

            /** Removes a child of the gui item. */
            bool removeChild( SmartPtr<IUIElement> child ) override;

            /** Removes this element. */
            void remove();

            /** Removes a the children of this gui item. */
            void removeAllChildren();

            void destroyAllChildren();

            /** Check if the check item has a child. */
            virtual bool hasChildById( const String &id ) const;

            /** Find a child by id. */
            virtual SmartPtr<IUIElement> findChildById( const String &id ) const;

            /** Finds a component by id. */
            SmartPtr<IUIElement> findChildByComponentId( const String &componentId ) const;

            s32 getSiblingIndex() const;

            SmartPtr<IUIElement> getLayout() const;
            void setLayout( SmartPtr<IUIElement> layout );

            /** Sets the attached user data.*/
            void setUserData( void *userData ) override;

            /** Gets the user data attached. */
            void *getUserData() const override;

            /** Finds a gui item listener. */
            SmartPtr<IEventListener> findObjectListener( const String &id ) const;

            /** Adds an animator to the container. */
            void addAnimator( SmartPtr<IAnimator> &animator );

            /** Removes an animator from the container. */
            bool removeAnimator( SmartPtr<IAnimator> &animator );

            SmartPtr<IUILayoutContainer> getContainer() const;
            void setContainer( SmartPtr<IUILayoutContainer> container );

            /** @copydoc IComponent::getOwner */
            virtual SmartPtr<ISharedObject> getOwner() const;

            /** @copydoc IComponent::setOwner */
            virtual void setOwner( SmartPtr<ISharedObject> owner );

            void _getObject( void **ppObject ) const;

            /** @copydoc IComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            bool isDragDropSource() const;

            void setDragDropSource( bool dragDropSource );

            SmartPtr<IUIDragSource> getDragSource() const;

            void setDragSource( SmartPtr<IUIDragSource> dragSource );

            SmartPtr<IUIDropTarget> getDropTarget() const;

            void setDropTarget( SmartPtr<IUIDropTarget> dropTarget );

            virtual u32 getOrder() const;

            virtual void setOrder( u32 order );

            virtual void handleEvent( const SmartPtr<IEvent> &event );

            bool getSameLine() const;

            void setSameLine( bool sameLine );

            ColourF getColour() const;

            void setColour( const ColourF &colour );

            void invalidate();

            SmartPtr<IStateContext> getStateContext() const;

            void setStateContext( SmartPtr<IStateContext> stateContext );

            SmartPtr<IStateListener> getStateListener() const;

            void setStateListener( SmartPtr<IStateListener> stateListener );

            void sortZOrder();

            void updateZOrder();

            bool getRenderChildren() const;

            void setRenderChildren( bool renderChildren );

            void onActivate( SmartPtr<IUIElement> element ) override;
            void onDeactivate() override;
            void onSelect() override;
            void onDeselect() override;
            void onGainFocus() override;
            void onLostFocus() override;

            virtual bool handleStateChanged( SmartPtr<IState> &state );

            bool getHandleInputEvents() const;

            void setHandleInputEvents( bool handleInputEvents );

            bool isThreadSafe() const;

            void addMessage( SmartPtr<IStateMessage> message );

        protected:
            class ElementStateListener : public IStateListener
            {
            public:
                ElementStateListener() = default;
                ~ElementStateListener() override = default;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                bool handleStateChanged( SmartPtr<IState> &state ) override;

                SmartPtr<ClawUIElement<T>> getOwner() const;

                void setOwner( SmartPtr<ClawUIElement<T>> owner );

            protected:
                // The element owns its callback; the callback must not retain the element.
                AtomicWeakPtr<ClawUIElement<T>> m_owner;
            };

            void setType( const String &type );

            virtual void createStateContext();

            /** Push this element's absolute rectangle into the active free-form layout. */
            bool beginWorkphoneWidget( struct wp_context *ctx ) const;

            /** Draw visible child elements through the non-template Workphone interface. */
            void drawWorkphoneChildren( struct wp_context *ctx );

            /** Calculate this element's pixel-space rectangle for WorkphoneCore. */
            struct wp_rect getWorkphoneBounds() const;

            AtomicSmartPtr<IStateContext> m_stateContext;
            AtomicSmartPtr<IStateListener> m_stateListener;

            AtomicSmartPtr<ISharedObject> m_owner;

            SmartPtr<IUIDragSource> m_dragSource;
            SmartPtr<IUIDropTarget> m_dropTarget;

            SmartPtr<IUILayoutContainer> m_container;

            /// The parent gui element.
            SmartPtr<IUIElement> m_parent;

            SmartPtr<IUIElement> m_layout;

            /// Used to store user properties.
            mutable SmartPtr<ISharedObject> m_userProperties;

            /// The attached user data.
            void *m_userData;

            Vector2F m_position = Vector2F::zero();
            Vector2F m_size = Vector2F( 1.0f, 1.0f );
            ColourF m_colour = ColourF::White;

            f32 m_scale = 1.0f;

            u32 m_order = 0;

            /// The hashed id of the element.
            hash_type m_hashId;

            hash_type m_elementId = -1;

            bool m_visible = true;

            /// Used to know if the element is enabled.
            bool m_isEnabled = true;

            /// To know if the element has focus.
            bool m_hasFocus = false;

            /// To know if the element is highlighted.
            bool m_isHighlighted = false;

            /// To know if the element is selected.
            bool m_isSelected = false;

            bool m_isHovered = false;

            bool m_dragDropSource = false;

            bool m_sameLine = false;

            bool m_handleInputEvents = true;

            bool m_renderChildren = true;

            /// The id of the component.
            String m_componentId;

            String m_label;

            /// The type of gui element.
            String m_type;

            /// The children of the gui element.
            ConcurrentArray<SmartPtr<IUIElement>> m_children;

            /// The number of the next name extension.
            static u32 m_nextGeneratedNameExt;
        };

        template <class T>
        u32 ClawUIElement<T>::m_nextGeneratedNameExt = 0;

        template <class T>
        ClawUIElement<T>::ClawUIElement() :
            m_parent( nullptr ),
            m_userData( nullptr ),
            m_isEnabled( true ),
            m_hasFocus( false ),
            m_isHighlighted( false ),
            m_isSelected( false )
        {
            auto name = String( "GUIElement" ) + StringUtil::toString( m_nextGeneratedNameExt++ );
            this->setName( name );

            m_layout = nullptr;
            createStateContext();
        }

        template <class T>
        ClawUIElement<T>::~ClawUIElement()
        {
            unload( nullptr );
        }

        template <class T>
        void ClawUIElement<T>::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setContainer( nullptr );

                removeAllChildren();

                m_parent = nullptr;
                m_layout = nullptr;

                auto stateContext = getStateContext();
                setStateContext( nullptr );
                setStateListener( nullptr );
                if( stateContext )
                {
                    stateContext->setOwner( nullptr );
                    auto applicationManager = core::IApplicationManager::instance();
                    auto stateManager = applicationManager ? applicationManager->getStateManager() : nullptr;
                    if( stateManager )
                    {
                        stateManager->removeStateContext( stateContext );
                    }
                    else
                    {
                        stateContext->unload( nullptr );
                    }
                }
                this->setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void ClawUIElement<T>::update()
        {
            if( !isLoaded() || !isVisible() || !isEnabled() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
            {
                return;
            }

            auto renderUI = applicationManager->getRenderUI();
            auto ui = workphone::static_pointer_cast<ClawUIManager>( renderUI );
            if( !ui )
            {
                return;
            }

            auto *ctx = ui->getContext();
            if( !ctx )
            {
                return;
            }

            draw( ctx );
        }

        template <class T>
        void ClawUIElement<T>::draw( struct wp_context *ctx )
        {
            if( !ctx || !isVisible() || !m_isEnabled || !m_renderChildren )
            {
                return;
            }

            drawWorkphoneChildren( ctx );
        }

        template <class T>
        const String &ClawUIElement<T>::getWorkphoneComponentId() const
        {
            return m_componentId;
        }

        template <class T>
        bool ClawUIElement<T>::beginWorkphoneWidget( struct wp_context *ctx ) const
        {
            if( !ctx || !isVisible() || !isEnabled() )
            {
                return false;
            }

            wp_layout_space_push( ctx, getWorkphoneBounds() );
            return true;
        }

        template <class T>
        void ClawUIElement<T>::drawWorkphoneChildren( struct wp_context *ctx )
        {
            if( !ctx || !m_renderChildren )
            {
                return;
            }

            auto &children = getChildrenRef();
            for( auto &child : children )
            {
                if( auto guiChild = dynamic_cast<IWorkphoneWidget *>( child.get() ) )
                {
                    guiChild->draw( ctx );
                }
            }
        }

        template <class T>
        struct wp_rect ClawUIElement<T>::getWorkphoneBounds() const
        {
            auto position = getAbsolutePosition();
            auto size = getSize() * getScale();

            auto viewportSize = Vector2F( 1.0f, 1.0f );
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto uiWindow = applicationManager->getSceneRenderWindow() )
                {
                    viewportSize = uiWindow->getSize();
                }
                else if( auto mainWindow = applicationManager->getWindow() )
                {
                    auto windowSize = mainWindow->getSize();
                    viewportSize =
                        Vector2F( static_cast<f32>( windowSize.x ), static_cast<f32>( windowSize.y ) );
                }
                if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
                {
                    if( auto renderer = graphicsSystem->getRenderer() )
                    {
                        if( auto viewport = renderer->getViewport() )
                        {
                            const auto size = viewport->getActualSize();
                            if( size.X() > 0.0f && size.Y() > 0.0f )
                            {
                                viewportSize = size;
                            }
                        }
                    }
                }
            }

            position *= viewportSize;
            size *= viewportSize;

            struct wp_rect bounds;
            bounds.x = position.X();
            bounds.y = position.Y();
            bounds.w = MathF::max( size.X(), 1.0f );
            bounds.h = MathF::max( size.Y(), 1.0f );
            return bounds;
        }

        template <class T>
        SmartPtr<ISharedObject> ClawUIElement<T>::getOwner() const
        {
            return m_owner;
        }

        template <class T>
        void ClawUIElement<T>::setOwner( SmartPtr<ISharedObject> owner )
        {
            m_owner = owner;
        }

        template <class T>
        bool ClawUIElement<T>::getSameLine() const
        {
            return m_sameLine;
        }

        template <class T>
        void ClawUIElement<T>::setSameLine( bool sameLine )
        {
            m_sameLine = sameLine;
        }

        template <class T>
        void ClawUIElement<T>::addChild( SmartPtr<IUIElement> child )
        {
            if( !child )
            {
                return;
            }

            child->remove();
            child->setParent( this );

            auto &children = getChildrenRef();
            if( std::find( children.begin(), children.end(), child ) == children.end() )
            {
                children.push_back( child );
            }

            if( !this->template isDerived<IUILayoutWindow>() )
            {
                child->setLayout( getLayout() );
            }

            if( auto stateContext = child->getStateContext() )
            {
                stateContext->setDirty( true );
            }
            updateZOrder();
        }

        template <class T>
        bool ClawUIElement<T>::removeChild( SmartPtr<IUIElement> child )
        {
            if( child )
            {
                auto &children = getChildrenRef();

                auto it = std::find( children.begin(), children.end(), child );
                if( it != children.end() )
                {
                    child->setParent( nullptr );
                    children.erase( it );
                    return true;
                }
            }

            return false;
        }

        template <class T>
        void ClawUIElement<T>::remove()
        {
            if( m_parent )
            {
                m_parent->removeChild( this );
            }
        }

        template <class T>
        void ClawUIElement<T>::removeAllChildren()
        {
            auto &children = getChildrenRef();
            for( auto &child : children )
            {
                if( child )
                {
                    child->setParent( nullptr );
                }
            }

            children.clear();
        }

        template <class T>
        void ClawUIElement<T>::destroyAllChildren()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto ui = applicationManager->getUI();

            auto &children = getChildrenRef();
            for( auto &child : children )
            {
                if( child )
                {
                    child->setParent( nullptr );

                    ui->removeElement( child );
                }
            }

            children.clear();
        }

        template <class T>
        bool ClawUIElement<T>::hasChildById( const String &id ) const
        {
            auto &children = getChildrenRef();
            for( auto &child : children )
            {
                if( child )
                {
                    if( id == child->getName() )
                    {
                        return true;
                    }
                }
            }

            for( auto &child : children )
            {
                if( child )
                {
                    if( child->hasChildById( id ) )
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        template <class T>
        SmartPtr<IUIElement> ClawUIElement<T>::findChildById( const String &id ) const
        {
            auto &children = getChildrenRef();
            for( auto &child : children )
            {
                if( child )
                {
                    if( id == child->getName() )
                    {
                        return child;
                    }
                }
            }

            for( auto &child : children )
            {
                if( child )
                {
                    if( auto found = child->findChildById( id ) )
                    {
                        return found;
                    }
                }
            }

            return nullptr;
        }

        template <class T>
        SmartPtr<IUIElement> ClawUIElement<T>::findChildByComponentId( const String &componentId ) const
        {
            auto &children = getChildrenRef();
            for( auto &child : children )
            {
                if( auto childElement = dynamic_cast<IWorkphoneWidget *>( child.get() );
                    childElement && componentId == childElement->getWorkphoneComponentId() )
                {
                    return child;
                }
            }

            SmartPtr<IUIElement> pCompGUIItem;
            for( auto &child : children )
            {
                if( auto childElement = dynamic_cast<ClawUIElement<T> *>( child.get() ) )
                {
                    pCompGUIItem = childElement->findChildByComponentId( componentId );
                }
                if( pCompGUIItem )
                {
                    return pCompGUIItem;
                }
            }

            return nullptr;
        }

        template <class T>
        s32 ClawUIElement<T>::getSiblingIndex() const
        {
            if( m_parent )
            {
                auto pThis = ClawUIElement<T>::template getSharedFromThis<IUIElement>();
                auto children = m_parent->getChildren();

                for( u32 i = 0; i < children.size(); ++i )
                {
                    if( children.at( i ) == pThis )
                    {
                        return i;
                    }
                }
            }

            return -1;
        }

        template <class T>
        void ClawUIElement<T>::setUserData( void *pUserData )
        {
            m_userData = pUserData;
        }

        template <class T>
        void *ClawUIElement<T>::getUserData() const
        {
            return m_userData;
        }

        template <class T>
        SmartPtr<IEventListener> ClawUIElement<T>::findObjectListener( const String &id ) const
        {
            auto listeners = ClawUIElement<T>::getObjectListeners();
            for( auto &listener : listeners )
            {
                if( listener->getName() == id )
                {
                    return listener;
                }
            }

            auto &children = getChildrenRef();
            for( auto &child : children )
            {
                auto listener = child->findObjectListener( id );
                if( listener )
                {
                    return listener;
                }
            }

            return nullptr;
        }

        template <class T>
        void ClawUIElement<T>::addAnimator( SmartPtr<IAnimator> &animator )
        {
        }

        template <class T>
        bool ClawUIElement<T>::removeAnimator( SmartPtr<IAnimator> &animator )
        {
            return false;
        }

        template <class T>
        SmartPtr<IUILayoutContainer> ClawUIElement<T>::getContainer() const
        {
            return m_container;
        }

        template <class T>
        void ClawUIElement<T>::setContainer( SmartPtr<IUILayoutContainer> container )
        {
            m_container = container;
        }

        template <class T>
        void ClawUIElement<T>::_getObject( void **ppObject ) const
        {
            *ppObject = nullptr;
        }

        template <class T>
        bool ClawUIElement<T>::handleEvent( const SmartPtr<IInputEvent> &event )
        {
            auto enabled = isEnabled();
            if( !enabled )
            {
                return false;
            }

            // for( u32 i = 0; i < m_inputListeners.size(); ++i )
            //{
            //     auto inputListener = m_inputListeners[i];
            //     if( inputListener->onEvent( event ) )
            //         return true;
            // }

            auto &children = getChildrenRef();
            for( auto &child : children )
            {
                if( child->handleEvent( event ) )
                {
                    return true;
                }
            }

            return false;
        }

        template <class T>
        Vector2F ClawUIElement<T>::getAbsolutePosition() const
        {
            if( m_parent )
            {
                return m_parent->getAbsolutePosition() + getPosition();
            }

            return getPosition();
        }

        template <class T>
        void ClawUIElement<T>::setEnabled( bool enabled, bool cascade )
        {
            if( m_isEnabled != enabled )
            {
                m_isEnabled = enabled;

                if( cascade )
                {
                    auto &children = getChildrenRef();
                    for( u32 i = 0; i < children.size(); ++i )
                    {
                        children[i]->setEnabled( m_isEnabled );
                    }
                }

                if( !m_isEnabled && m_hasFocus )
                {
                    setFocus( false );
                }
            }
        }

        template <class T>
        void ClawUIElement<T>::setSelected( bool selected )
        {
            bool wasSelected = m_isSelected;
            m_isSelected = selected;

            if( wasSelected && !m_isSelected )
            {
            }
            else if( !wasSelected && m_isSelected )
            {
            }
        }

        template <class T>
        bool ClawUIElement<T>::isSelected() const
        {
            return m_isSelected;
        }

        template <class T>
        void ClawUIElement<T>::setHovered( bool hovered )
        {
            m_isHovered = hovered;
        }

        template <class T>
        bool ClawUIElement<T>::isHovered() const
        {
            return m_isHovered;
        }

        template <class T>
        void ClawUIElement<T>::setVisible( bool visible, bool cascade )
        {
            m_visible = visible;

            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::visibleFlag ), visible );
                }
            }

            if( cascade )
            {
                auto &children = getChildrenRef();
                for( auto &child : children )
                {
                    child->setVisible( visible, cascade );
                }
            }

            // if( !visible && element->hasFocus() )
            //{
            //     setFocus( false );
            // }
        }

        template <class T>
        void ClawUIElement<T>::setFocus( bool hasFocus )
        {
            if( m_hasFocus && !hasFocus )
            {
            }

            m_hasFocus = hasFocus;

            auto &children = getChildrenRef();
            for( auto &child : children )
            {
                child->setFocus( m_hasFocus );
            }

            if( m_hasFocus )
            {
            }
        }

        template <class T>
        bool ClawUIElement<T>::isThreadSafe() const
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto renderTask = graphicsSystem->getRenderTask();

            auto task = Thread::getCurrentTask();

            const auto &loadingState = T::getLoadingState();

            return loadingState == LoadingState::Loaded && task == renderTask;
        }

        template <class T>
        void ClawUIElement<T>::addMessage( SmartPtr<IStateMessage> message )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            if( auto stateContext = getStateContext() )
            {
                const auto stateTask = graphicsSystem->getStateTask();
                stateContext->addMessage( stateTask, message );
            }
        }

        template <class T>
        ConcurrentArray<SmartPtr<IUIElement>> &ClawUIElement<T>::getChildrenRef()
        {
            return m_children;
        }

        template <class T>
        const ConcurrentArray<SmartPtr<IUIElement>> &ClawUIElement<T>::getChildrenRef() const
        {
            return m_children;
        }

        template <class T>
        Array<SmartPtr<IUIElement>> ClawUIElement<T>::getChildren() const
        {
            return m_children.snapshot();
        }

        template <class T>
        u32 ClawUIElement<T>::getNumChildren() const
        {
            return static_cast<u32>( m_children.size() );
        }

        template <class T>
        void ClawUIElement<T>::setChildren( ConcurrentArray<SmartPtr<IUIElement>> children )
        {
            m_children = children;
        }

        template <class T>
        void ClawUIElement<T>::setPosition( const Vector2F &position )
        {
            m_position = position;

            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UITransformStateData>() )
                {
                    state->position = position;
                }
            }
        }

        template <class T>
        SmartPtr<Properties> ClawUIElement<T>::getProperties() const
        {
            auto properties = workphone::make_ptr<Properties>();
            properties->setProperty( "size", getSize() );
            properties->setProperty( "position", getPosition() );
            properties->setProperty( "enabled", m_isEnabled );
            properties->setProperty( "visible", isVisible() );
            return properties;
        }

        template <class T>
        void ClawUIElement<T>::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                return;
            }

            auto enabled = isEnabled();
            auto visible = isVisible();
            // properties->getPropertyValue( "size", m_size );
            // properties->getPropertyValue( "position", m_position );
            properties->getPropertyValue( "enabled", enabled );
            properties->getPropertyValue( "visible", visible );

            setEnabled( enabled );
            setVisible( visible );
        }

        template <class T>
        Array<SmartPtr<ISharedObject>> ClawUIElement<T>::getChildObjects() const
        {
            auto objects = Array<SmartPtr<ISharedObject>>();
            objects.reserve( 32 );

            // if( auto stateContext = m_stateContext.load() )
            //{
            //     objects.push_back( stateContext );
            // }

            // if( auto stateListener = m_stateListener.load() )
            //{
            //     objects.push_back( stateListener );
            // }

            // if( auto owner = m_owner.load() )
            //{
            //     objects.push_back( owner );
            // }

            if( m_dragSource )
            {
                objects.push_back( m_dragSource );
            }

            if( m_dropTarget )
            {
                objects.push_back( m_dropTarget );
            }

            // if( auto element = getOverlayElement() )
            //{
            //     objects.push_back( element );
            // }

            // if( m_container )
            //{
            //     objects.push_back( m_container );
            // }

            // if( m_parent )
            //{
            //     objects.push_back( m_parent );
            // }

            // if( m_layout )
            //{
            //     objects.push_back( m_layout );
            // }

            return objects;
        }

        template <class T>
        bool ClawUIElement<T>::isDragDropSource() const
        {
            return m_dragDropSource;
        }

        template <class T>
        void ClawUIElement<T>::setDragDropSource( bool dragDropSource )
        {
            m_dragDropSource = dragDropSource;
        }

        template <class T>
        SmartPtr<IUIDragSource> ClawUIElement<T>::getDragSource() const
        {
            return m_dragSource;
        }

        template <class T>
        void ClawUIElement<T>::setDragSource( SmartPtr<IUIDragSource> dragSource )
        {
            m_dragSource = dragSource;
        }

        template <class T>
        SmartPtr<IUIDropTarget> ClawUIElement<T>::getDropTarget() const
        {
            return m_dropTarget;
        }

        template <class T>
        void ClawUIElement<T>::setDropTarget( SmartPtr<IUIDropTarget> dropTarget )
        {
            m_dropTarget = dropTarget;
        }

        template <class T>
        u32 ClawUIElement<T>::getOrder() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UITransformStateData>() )
                {
                    return state->zorder;
                }
            }

            return m_order;
        }

        template <class T>
        void ClawUIElement<T>::setOrder( u32 order )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UITransformStateData>() )
                {
                    state->zorder = order;
                }
            }

            m_order = order;
        }

        template <class T>
        void ClawUIElement<T>::initialise( SmartPtr<IUIElement> &parent )
        {
        }

        template <class T>
        void ClawUIElement<T>::initialise( SmartPtr<IUIElement> &parent, const Properties &properties )
        {
        }

        template <class T>
        hash_type ClawUIElement<T>::getElementId() const
        {
            return m_elementId;
        }

        template <class T>
        void ClawUIElement<T>::setElementId( hash_type elementId )
        {
            m_elementId = elementId;
        }

        template <class T>
        void ClawUIElement<T>::setComponentID( const String &componentId )
        {
            m_componentId = componentId;
        }

        template <class T>
        const String &ClawUIElement<T>::getComponentID() const
        {
            return m_componentId;
        }

        template <class T>
        void ClawUIElement<T>::setType( const String &type )
        {
            m_type = type;
        }

        template <class T>
        const String &ClawUIElement<T>::getType() const
        {
            return m_type;
        }

        template <class T>
        String ClawUIElement<T>::getLabel() const
        {
            return m_label;
        }

        template <class T>
        void ClawUIElement<T>::setLabel( const String &label )
        {
            m_label = label;
        }

        template <class T>
        Vector2F ClawUIElement<T>::getPosition() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UITransformStateData>() )
                {
                    return state->position;
                }
            }

            return m_position;
        }

        template <class T>
        void ClawUIElement<T>::setSize( const Vector2F &size )
        {
            m_size = size;

            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UITransformStateData>() )
                {
                    state->size = size;
                }
            }
        }

        template <class T>
        Vector2F ClawUIElement<T>::getSize() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UITransformStateData>() )
                {
                    return state->size;
                }
            }

            return m_size;
        }

        template <class T>
        f32 ClawUIElement<T>::getScale() const
        {
            return m_scale;
        }

        template <class T>
        void ClawUIElement<T>::setScale( f32 scale )
        {
            m_scale = scale;
        }

        template <class T>
        bool ClawUIElement<T>::isEnabled() const
        {
            return m_isEnabled;
        }

        template <class T>
        bool ClawUIElement<T>::isVisible() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue( state->flags,
                                                  static_cast<u16>( IUIElement::visibleFlag ) );
                }
            }

            return m_visible;
        }

        template <class T>
        bool ClawUIElement<T>::isInFocus() const
        {
            return m_hasFocus;
        }

        template <class T>
        bool ClawUIElement<T>::isHighlighted() const
        {
            return m_isHighlighted;
        }

        template <class T>
        SmartPtr<IUIElement> ClawUIElement<T>::getParent() const
        {
            return m_parent;
        }

        template <class T>
        void ClawUIElement<T>::setParent( SmartPtr<IUIElement> parent )
        {
            m_parent = parent;
        }

        template <class T>
        u32 ClawUIElement<T>::getHashId() const
        {
            return static_cast<u32>( m_hashId );
        }

        template <class T>
        SmartPtr<IUIElement> ClawUIElement<T>::getLayout() const
        {
            return m_layout;
        }

        template <class T>
        void ClawUIElement<T>::setLayout( SmartPtr<IUIElement> layout )
        {
            m_layout = layout;
        }

        template <class T>
        void ClawUIElement<T>::handleEvent( const SmartPtr<IEvent> &event )
        {
        }

        template <class T>
        void ClawUIElement<T>::createStateContext()
        {
            if( getStateContext() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
            {
                return;
            }

            auto stateManager = applicationManager->getStateManager();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();
            if( !stateManager || !graphicsSystem || !factoryManager )
            {
                return;
            }

            auto stateContext = stateManager->addStateContext();

            auto listener = factoryManager->make_ptr<ElementStateListener>();
            listener->setOwner( this );
            stateContext->addStateListener( listener );

            auto state = factoryManager->make_ptr<State>();
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<UIElementStateData>();
            stateData->colour = m_colour;
            stateData->flags = BitUtil::setFlagValue(
                stateData->flags, static_cast<u16>( IUIElement::visibleFlag ), m_visible );
            state->setData( stateData );

            auto transformState = factoryManager->make_ptr<State>();
            stateContext->addState( transformState );

            auto transformStateData = factoryManager->make_ptr<UITransformStateData>();
            transformStateData->position = m_position;
            transformStateData->size = m_size;
            transformState->setData( transformStateData );

            stateContext->setOwner( this );

            setStateContext( stateContext );
            setStateListener( listener );

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );
        }

        template <class T>
        ColourF ClawUIElement<T>::getColour() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UIElementStateData>() )
                {
                    return state->colour;
                }
            }

            return m_colour;
        }

        template <class T>
        void ClawUIElement<T>::setColour( const ColourF &colour )
        {
            m_colour = colour;

            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UIElementStateData>() )
                {
                    state->colour = colour;
                }
            }
        }

        template <class T>
        void ClawUIElement<T>::invalidate()
        {
        }

        template <class T>
        SmartPtr<IStateContext> ClawUIElement<T>::getStateContext() const
        {
            return m_stateContext;
        }

        template <class T>
        void ClawUIElement<T>::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        template <class T>
        SmartPtr<IStateListener> ClawUIElement<T>::getStateListener() const
        {
            return m_stateListener;
        }

        template <class T>
        void ClawUIElement<T>::setStateListener( SmartPtr<IStateListener> stateListener )
        {
            m_stateListener = stateListener;
        }

        template <class T>
        void ClawUIElement<T>::sortZOrder()
        {
            auto &children = getChildrenRef();

            std::sort( children.begin(), children.end(),
                       []( auto a, auto b ) { return a->getOrder() < b->getOrder(); } );

            for( auto child : children )
            {
                child->sortZOrder();
            }
        }

        template <class T>
        void ClawUIElement<T>::updateZOrder()
        {
            sortZOrder();

            auto &children = getChildrenRef();
            for( auto &child : children )
            {
                child->updateZOrder();
            }
        }

        template <class T>
        bool ClawUIElement<T>::getRenderChildren() const
        {
            return m_renderChildren;
        }

        template <class T>
        void ClawUIElement<T>::setRenderChildren( bool renderChildren )
        {
            m_renderChildren = renderChildren;
        }

        template <class T>
        void ClawUIElement<T>::onActivate( SmartPtr<IUIElement> element )
        {
            setSelected( true );
        }

        template <class T>
        void ClawUIElement<T>::onDeactivate()
        {
            setSelected( false );
        }

        template <class T>
        void ClawUIElement<T>::onSelect()
        {
            setSelected( true );
        }

        template <class T>
        void ClawUIElement<T>::onDeselect()
        {
            setSelected( false );
        }

        template <class T>
        void ClawUIElement<T>::onGainFocus()
        {
            m_hasFocus = true;
        }

        template <class T>
        void ClawUIElement<T>::onLostFocus()
        {
            m_hasFocus = false;
        }

        template <class T>
        bool ClawUIElement<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        template <class T>
        bool ClawUIElement<T>::getHandleInputEvents() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue(
                        state->flags, static_cast<u16>( IUIElement::handleInputEventsFlag ) );
                }
            }

            return m_handleInputEvents;
        }

        template <class T>
        void ClawUIElement<T>::setHandleInputEvents( bool handleInputEvents )
        {
            m_handleInputEvents = handleInputEvents;
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateByType<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::handleInputEventsFlag ),
                        handleInputEvents );
                }
            }
        }

        template <class T>
        void ClawUIElement<T>::setHighlighted( bool isHighlighted, bool cascade )
        {
            if( m_isHighlighted != isHighlighted )
            {
                m_isHighlighted = isHighlighted;

                if( cascade )
                {
                    auto &children = getChildrenRef();
                    for( auto &child : children )
                    {
                        child->setHighlighted( m_isHighlighted );
                    }
                }
            }
        }

        template <class T>
        bool ClawUIElement<T>::ElementStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwner() )
            {
                if( message->isExactly<StateMessageObject>() )
                {
                    auto objectMessage = workphone::static_pointer_cast<StateMessageObject>( message );
                    auto messageType = objectMessage->getType();
                    auto object = objectMessage->getObject();

                    if( messageType == IUIElement::STATE_MESSAGE_ADD_CHILD )
                    {
                        owner->addChild( object );
                    }
                    else if( messageType == IUIElement::STATE_MESSAGE_REMOVE_CHILD )
                    {
                        owner->removeChild( object );
                    }
                }
            }

            return false;
        }

        template <class T>
        bool ClawUIElement<T>::ElementStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            auto owner = getOwner();
            if( owner )
            {
                owner->handleStateChanged( state );
                state->setDirty( false );
            }
            return false;
        }

        template <class T>
        SmartPtr<ClawUIElement<T>> ClawUIElement<T>::ElementStateListener::getOwner() const
        {
            return m_owner.load().lock();
        }

        template <class T>
        void ClawUIElement<T>::ElementStateListener::setOwner( SmartPtr<ClawUIElement<T>> owner )
        {
            m_owner = owner;
        }
    }  // end namespace ui
}  // namespace workphone

#endif
