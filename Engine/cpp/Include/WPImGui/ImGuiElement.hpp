#ifndef _WP_GUIElement_H
#define _WP_GUIElement_H

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Interface/UI/IUILayoutContainer.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/Script/IScriptInvoker.hpp>
#include <Workphone/Interface/Script/IScriptReceiver.hpp>
#include <Workphone/Interface/Script/IScriptData.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Core/ConcurrentFixedArrayGrowable.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/System/Prototype.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @brief Base class for immediate-mode GUI elements used by the ImGui integration.
         *
         * This class implements a generic UI element that can be used as a parent for
         * concrete ImGui-backed controls. It provides basic state (position, size,
         * visibility, enabled, focus, selection, children management, scripting hooks,
         * drag & drop support and state context/listener wiring).
         *
         * The class is a template so a derived type can be used as the template parameter
         * to enable shared_from_this style patterns provided by core::Prototype<T>.
         *
         * @tparam T Concrete derived type that inherits from ImGuiElement<T>.
         */
        template <class T>
        class ImGuiElement : public core::Prototype<T>
        {
        public:
            using element_type = T;

            /** @brief Default constructor. Initializes internal state and generated name. */
            ImGuiElement();

            /** @brief Virtual destructor. Releases resources by calling unload(). */
            ~ImGuiElement() override;

            /**
             * @brief Loads the element.
             * @param data Optional shared object used to configure the element.
             *
             * Implementations may override to initialize resources. Default implementation is empty.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the element and releases child resources.
             * @param data Optional data passed during unload.
             *
             * This will remove listeners, clear children and disconnect state context/listener.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Update the element each frame.
             *
             * Default implementation performs early-out checks (visibility, enabled and thread).
             * Derived classes should override to implement per-frame behavior and may call
             * the base implementation to preserve checks.
             */
            void update() override;

            /**
             * @brief Handle an input event targeted at this element.
             * @param event Smart pointer to the input event.
             * @return True if the event was consumed by this element, false otherwise.
             *
             * Default implementation does not consume events. Override to handle input.
             */
            virtual bool handleEvent( const SmartPtr<IInputEvent> &event );

            /** @brief Returns the element id (internal hash). */
            virtual hash_type getElementId() const;

            /** @brief Sets the element id (internal hash). */
            virtual void setElementId( hash_type itemId );

            /**
             * @brief Set a string identifier for the component.
             * @param componentId Identifier string for component lookups.
             */
            void setComponentID( const String &componentId );

            /**
             * @brief Get the string identifier for the component.
             * @return Component identifier string.
             */
            const String &getComponentID() const;

            /**
             * @brief Get the type name for this UI element.
             * @return Type string previously set with setType().
             */
            const String &getType() const;

            /** @brief Get the element label (used for display). */
            String getLabel() const override;

            /** @brief Set the element label (used for display). */
            void setLabel( const String &label ) override;

            /**
             * @brief Set the element's local position (relative to parent).
             * @param position 2D position in UI units.
             */
            virtual void setPosition( const Vector2F &position );

            /** @brief Get the element's local position. */
            Vector2F getPosition() const;

            /**
             * @brief Get the element's absolute position in screen/UI space.
             * @return Absolute position computed by adding parent's absolute position.
             */
            Vector2F getAbsolutePosition() const;

            /**
             * @brief Set the element size.
             * @param size 2D size (width, height) in UI units.
             */
            virtual void setSize( const Vector2F &size );

            /** @brief Get the element size. */
            Vector2F getSize() const;

            /** @brief Get the element scale factor. */
            f32 getScale() const;

            /** @brief Set the element scale factor. */
            void setScale( f32 scale );

            /**
             * @brief Set whether the element is enabled.
             * @param enabled If true the element is enabled.
             * @param cascade If true, apply the enabled state to all children.
             */
            void setEnabled( bool enabled, bool cascade = true );

            /** @brief Query whether the element is enabled. */
            bool isEnabled() const;

            /**
             * @brief Mark element as selected or not.
             * @param selected True when selected.
             *
             * Triggers onSelect/onDeselect callbacks when selection changes.
             */
            void setSelected( bool selected );

            /** @brief Returns true if the element is selected. */
            bool isSelected() const;

            /**
             * @brief Set element visibility.
             * @param visible If true the element will be rendered/considered visible.
             * @param cascade If true, propagate visibility to children.
             */
            virtual void setVisible( bool visible, bool cascade = true );

            /** @brief Set hovered state for the element (used by input handling). */
            void setHovered( bool hovered );

            /** @brief Returns true if the element is currently hovered. */
            bool isHovered() const;

            /** @brief Returns true if the element is visible. */
            bool isVisible() const;

            /**
             * @brief Set keyboard/gamepad focus for this element.
             * @param hasFocus True to give focus to the element.
             *
             * The default implementation does not manage focus propagation. Override to implement focus behaviour.
             */
            virtual void setFocus( bool hasFocus );

            /** @brief Returns true if the element currently has focus. */
            bool isInFocus() const;

            /**
             * @brief Mark the element as highlighted.
             * @param isHighlighted True to highlight.
             * @param cascade If true, propagate highlight state to children.
             *
             * Default implementation is a no-op. Override to change visual highlight.
             */
            void setHighlighted( bool isHighlighted, bool cascade = true );

            /** @brief Returns true if highlighted. */
            bool isHighlighted() const;

            /** @brief Get the parent UI element. */
            SmartPtr<IUIElement> getParent() const;

            /** @brief Set the parent UI element. */
            void setParent( SmartPtr<IUIElement> parent );

            /** @copydoc IUIElement::getNumChildren */
            u32 getNumChildren() const;

            /** @brief Get a thread-safe collection of children. */
            Array<SmartPtr<IUIElement>> getChildren() const;

            /**
             * @brief Add a child element to this element.
             * @param pGUIItem Child element to add. If it already has a parent it will be removed from it.
             */
            void addChild( SmartPtr<IUIElement> pGUIItem ) override;

            /**
             * @brief Remove a child element.
             * @param pGUIItem Child to remove.
             * @return True if the child was found and removed.
             */
            bool removeChild( SmartPtr<IUIElement> pGUIItem ) override;

            /** @brief Remove this element from its parent (if any). */
            void remove() override;

            /** @brief Remove all children from this element (children will be detached but not destroyed). */
            void removeAllChildren() override;

            /** @brief Destroy (and remove from UI) all children recursively. */
            void destroyAllChildren() override;

            /**
             * @brief Check if a descendant exists with the specified id.
             * @param id Name/id of the child to search for.
             * @return True if a child or descendant with the given id is found.
             */
            virtual bool hasChildById( const String &id ) const override;

            /**
             * @brief Find and return a child element with the given id.
             * @param id Name/id of the child to search for.
             * @return SmartPtr to the found child or nullptr if not found.
             */
            virtual SmartPtr<IUIElement> findChildById( const String &id ) const override;

            /** @brief Get this element's index among its siblings or -1 if no parent. */
            s32 getSiblingIndex() const;

            /** @brief Get or set the layout element associated with this element. */
            SmartPtr<IUIElement> getLayout() const;
            void setLayout( SmartPtr<IUIElement> layout );

            /** @brief Attach user data pointer to the element. */
            void setUserData( void *userData ) override;

            /** @brief Retrieve attached user data pointer. */
            void *getUserData() const override;

            /** @brief Add an animator to the element. Default implementation is empty. */
            void addAnimator( SmartPtr<IAnimator> &animator );

            /** @brief Remove an animator from the element. Default implementation returns false. */
            bool removeAnimator( SmartPtr<IAnimator> &animator );

            /** @brief Get the container this element belongs to. */
            SmartPtr<IUILayoutContainer> getContainer() const;

            /** @brief Set the container this element belongs to. */
            void setContainer( SmartPtr<IUILayoutContainer> container );

            /**
             * @copydoc IComponent::getOwner
             * @brief Returns the shared owner object for this UI element.
             */
            virtual SmartPtr<ISharedObject> getOwner() const;

            /**
             * @copydoc IComponent::setOwner
             * @brief Set the shared owner object for this UI element.
             */
            virtual void setOwner( SmartPtr<ISharedObject> owner );

            /**
             * @brief Internal native object access. Implementations may expose internal pointers.
             * @param ppObject Output pointer set to internal object or nullptr.
             */
            void _getObject( void **ppObject ) const;

            /**
             * @copydoc IComponent::getProperties
             * @brief Get serializable properties for this element (name, position and size are included).
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IComponent::setProperties
             * @brief Set persistent properties from a Properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @brief Return a list of child shared objects. Default returns empty list. */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @brief Query if this element acts as a drag source. */
            bool isDragDropSource() const;

            /** @brief Enable/disable drag source behaviour for this element. */
            void setDragDropSource( bool dragDropSource );

            /** @brief Access the drag source helper object. */
            SmartPtr<IUIDragSource> getDragSource() const;
            void setDragSource( SmartPtr<IUIDragSource> dragSource );

            /** @brief Access the drop target helper object. */
            SmartPtr<IUIDropTarget> getDropTarget() const;
            void setDropTarget( SmartPtr<IUIDropTarget> dropTarget );

            /** @brief Get and set render order (z-order). Lower values rendered first. */
            virtual u32 getOrder() const;

            /** @brief Set render order (z-order). Lower values rendered first. */
            virtual void setOrder( u32 order );

            /** @brief Query whether children should be rendered. */
            bool getRenderChildren() const;

            /** @brief Enable/disable rendering of children. */
            void setRenderChildren( bool renderChildren );

            /**
             * @brief Get the script invoker used to call script functions on this element.
             * @return Reference to the invoker smart pointer.
             */
            virtual SmartPtr<IScriptInvoker> &getInvoker();

            /** @brief Const overload of getInvoker(). */
            virtual const SmartPtr<IScriptInvoker> &getInvoker() const;

            /** @brief Set the script invoker for the element. */
            virtual void setInvoker( SmartPtr<IScriptInvoker> invoker );

            /**
             * @brief Get the script receiver used to receive script calls/events on this element.
             * @return Reference to the receiver smart pointer.
             */
            virtual SmartPtr<IScriptReceiver> &getReceiver();

            /** @brief Const overload of getReceiver(). */
            virtual const SmartPtr<IScriptReceiver> &getReceiver() const;

            /** @brief Set the script receiver for the element. */
            virtual void setReceiver( SmartPtr<IScriptReceiver> receiver );

            /**
             * @brief Internal: set script data associated with this element.
             * @param data Arbitrary script data object.
             */
            virtual void _setData( SmartPtr<IScriptData> data );

            /**
             * @brief Internal: get script data associated with this element.
             * @return Script data object or null.
             */
            virtual SmartPtr<IScriptData> _getData() const;

            /**
             * @brief Set a property value by hash and string.
             * @param hash Hash of the property name.
             * @param value Value string to assign.
             * @return Status code (0 by default).
             *
             * Override to implement property mapping from string values.
             */
            virtual s32 setProperty( hash32 hash, const String &value );

            /**
             * @brief Get a property value by hash as a string.
             * @param hash Hash of the property name.
             * @param value Output string to populate.
             * @return Status code (0 by default).
             */
            virtual s32 getProperty( hash32 hash, String &value ) const;

            /**
             * @brief Set a property using a typed Parameter.
             * @param hash Property hash.
             * @param param Typed value to set.
             * @return Status code.
             */
            virtual s32 setProperty( hash32 hash, const Parameter &param );

            /**
             * @brief Set a property using multiple Parameters.
             * @param hash Property hash.
             * @param params Parameter list.
             * @return Status code.
             */
            virtual s32 setProperty( hash32 hash, const Parameters &params );

            /**
             * @brief Set a property using a raw pointer value.
             * @param hash Property hash.
             * @param param Raw pointer parameter.
             * @return Status code.
             */
            virtual s32 setProperty( hash32 hash, void *param );

            /** @brief Get a property into a Parameter. */
            virtual s32 getProperty( hash32 hash, Parameter &param );

            /** @brief Get a property into a Parameters list. */
            virtual s32 getProperty( hash32 hash, Parameters &params ) const;

            /** @brief Get a property via a raw pointer. */
            virtual s32 getProperty( hash32 hash, void *param ) const;

            /**
             * @brief Get named or indexed child/shared object by hash.
             * @param hash Hash key for object lookup.
             * @param object Output object smart pointer.
             * @return Status code (0 by default).
             */
            virtual s32 getObject( u32 hash, SmartPtr<ISharedObject> &object ) const;

            /**
             * @brief Call a function by hash with specified parameters.
             * @param hash Function identifier hash.
             * @param params Input parameters.
             * @param results Output parameters returned by the call.
             * @return Status code.
             */
            virtual s32 callFunction( u32 hash, const Parameters &params, Parameters &results );

            /**
             * @brief Call a function by hash on a specific object.
             * @param hash Function identifier hash.
             * @param object Target object for call.
             * @param results Output parameters returned by the call.
             * @return Status code.
             */
            virtual s32 callFunction( u32 hash, SmartPtr<ISharedObject> object, Parameters &results );

            /** @brief Internal: called at the start of initialization. Override to perform setup steps. */
            virtual void _onInitialiseStart();

            /** @brief Internal: called at the end of initialization. Override to perform final setup steps. */
            virtual void _onInitialiseEnd();

            //
            // Events - overridable hooks for derived types to react to lifecycle changes
            //

            /** @brief Called when a child is added to this element. */
            virtual void onAddChild( IUIElement *child );
            /** @brief Called when a child is removed from this element. */
            virtual void onRemoveChild( IUIElement *child );
            /** @brief Called when this element's state has changed. */
            virtual void onChangedState();
            /** @brief Called when one of this element's children's state changes. */
            virtual void onChildChangedState( IUIElement *child );
            /** @brief Called when the enabled state toggles. */
            virtual void onToggleEnabled();
            /** @brief Called when the visibility toggles. */
            virtual void onToggleVisibility();
            /** @brief Called when the highlight toggles. */
            virtual void onToggleHighlight();
            /** @brief Called when element is activated; may be forwarded to children. */
            virtual void onActivate( SmartPtr<IUIElement> element );
            /** @brief Called when element is deactivated. */
            virtual void onDeactivate();
            /** @brief Called when the element is selected. */
            virtual void onSelect();
            /** @brief Called when the element is deselected. */
            virtual void onDeselect();
            /** @brief Called when the element gains input focus. */
            virtual void onGainFocus();
            /** @brief Called when the element loses input focus. */
            virtual void onLostFocus();

            /**
             * @brief Generic event handling for non-input events.
             * @param event Event object to handle.
             */
            virtual void handleEvent( const SmartPtr<IEvent> &event );

            /** @brief Mark the element as invalidated (needs redraw/rebuild). */
            void invalidate();

            /** @brief Get the state context attached to this element. */
            SmartPtr<IStateContext> getStateContext() const;

            /** @brief Set the state context attached to this element. */
            void setStateContext( SmartPtr<IStateContext> stateContext );

            /** @brief Get the state listener attached to this element. */
            SmartPtr<IStateListener> getStateListener() const;

            /** @brief Set the state listener attached to this element. */
            void setStateListener( SmartPtr<IStateListener> stateListener );

            /** @brief Query whether subsequent elements should be on the same line (ImGui layout hint). */
            bool getSameLine() const;

            /** @brief Set the same-line layout hint. */
            void setSameLine( bool sameLine );

            /** @brief Get the element colour used for rendering. Thread-safe. */
            ColourF getColour() const;

            /** @brief Set the element colour used for rendering. Thread-safe. */
            void setColour( const ColourF &colour );

            /** @brief Query whether this element should handle input events. */
            bool getHandleInputEvents() const;

            /** @brief Enable/disable handling of input events for this element. */
            void setHandleInputEvents( bool handleInputEvents );

            void sortZOrder();

            /** @brief Update the Z-order; override to implement reordering logic. */
            void updateZOrder();

            /** @brief Acquire the element mutex for manual locking. Prefer ScopedLock where possible. */
            void lock();

            /** @brief Release the element mutex. */
            void unlock();

            WP_CLASS_REGISTER_TEMPLATE_DECL( ImGuiElement, T );

        protected:
            /**
             * @brief Simple default state listener base used to attach to a state context.
             *
             * Only the owner pointer is provided by default. Derived listeners should override
             * handleStateMessage / handleStateChanged to respond to state updates.
             */
            class BaseStateListener : public IStateListener
            {
            public:
                BaseStateListener() = default;
                ~BaseStateListener() override = default;

                /** @brief Default handler for state messages. Returns false. */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override
                {
                    return false;
                }

                /** @brief Default handler for state changed notifications. Returns false. */
                bool handleStateChanged( SmartPtr<IState> &state ) override
                {
                    return false;
                }

                /** @brief Get the owning ImGuiElement associated with the listener. */
                SmartPtr<ImGuiElement> getOwner() const
                {
                    auto p = m_owner.load();
                    return p.lock();
                }

                /** @brief Set the owning ImGuiElement associated with the listener. */
                void setOwner( SmartPtr<ImGuiElement> owner )
                {
                    m_owner = owner;
                }

            protected:
                AtomicWeakPtr<ImGuiElement>
                    m_owner; /**< Weak reference to owner element to avoid cycles. */
            };

            /**
             * @brief Replace the children collection. Protected so derived classes can set children atomically.
             * @param p Shared pointer to new children collection (or nullptr to clear).
             */
            void setChildren( SharedPtr<ConcurrentArray<SmartPtr<IUIElement>>> p );

            /** @brief Generic event hook invoked with a string event type. */
            virtual void onEvent( const String &eventType );

            /** @brief Set the human readable type for this element (used for serialization/debug). */
            void setType( const String &type );

            WeakPtr<ISharedObject> m_owner; /**< Shared owner object for the element. */

            AtomicSmartPtr<IStateContext>
                m_stateContext; /**< Optional state context owned by this element. */
            AtomicSmartPtr<IStateListener>
                m_stateListener; /**< Optional state listener attached to stateContext. */

            SmartPtr<IUIDragSource> m_dragSource; /**< Optional drag source helper object. */
            SmartPtr<IUIDropTarget> m_dropTarget; /**< Optional drop target helper object. */

            SmartPtr<IUILayoutContainer> m_container; /**< UI container managing this element. */

            SmartPtr<IScriptInvoker>
                m_scriptInvoker; /**< Script invoker used to call script functions. */
            SmartPtr<IScriptReceiver>
                m_scriptReceiver; /**< Script receiver used to handle script calls. */

            /// The data used by the script system.
            SmartPtr<IScriptData> m_scriptData;

            /// The parent gui element.
            WeakPtr<IUIElement> m_parent;

            /// Used to store user properties.
            mutable SmartPtr<ISharedObject> m_userProperties;

            WeakPtr<IUIElement> m_layout; /**< Optional layout element used by this element. */

            /// The attached user data (raw pointer).
            void *m_userData;

            AtomicObject<ColourF> m_colour = ColourF::White; /**< Element colour used for rendering. */

            /// The position of the gui element (local to parent).
            AtomicObject<Vector2F> m_position;

            /// The size of the gui element.
            AtomicObject<Vector2F> m_size;

            atomic_f32 m_scale = 1.0f; /**< Scale factor applied to the element. */

            /// True when the element is enabled.
            atomic_bool m_isEnabled = true;

            /// True when the element is visible.
            atomic_bool m_isVisible = true;

            /// True when the element has input focus.
            atomic_bool m_hasFocus = false;

            /// True when the element is highlighted.
            atomic_bool m_isHighlighted = false;

            /// True when the element is selected.
            atomic_bool m_isSelected = false;

            atomic_bool m_isHovered = false; /**< True when mouse is hovering the element. */

            atomic_bool m_dragDropSource = false; /**< True if element should act as a drag source. */

            atomic_bool m_sameLine = false; /**< ImGui same-line layout hint. */

            atomic_bool m_renderChildren = true; /**< If false, children will not be rendered. */

            atomic_bool m_handleInputEvents = true; /**< If false, input events are ignored. */

            atomic_u32 m_order = 0; /**< Render/order index (z-order). */

            AtomicValue<hash_type> m_itemId = -1; /**< Internal hashed item id. */

            /// The id of the component (string form).
            AtomicObject<String> m_componentId;

            /// The type/name of the UI element.
            AtomicObject<String> m_type;

            /**< Label text used for display. */
            AtomicObject<String> m_label;

            /// Thread-safe container of children.
            ConcurrentFixedArrayGrowable<SmartPtr<IUIElement>, 1024> m_children;

            /// Static counter used to generate default names.
            static u32 m_nextGeneratedNameExt;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::ui, ImGuiElement, T, core::Prototype<T> );

        template <class T>
        u32 ImGuiElement<T>::m_nextGeneratedNameExt = 0;

        template <class T>
        ImGuiElement<T>::ImGuiElement() :
            m_parent( nullptr ),
            m_userData( nullptr ),
            m_isEnabled( true ),
            m_isVisible( true ),
            m_hasFocus( false ),
            m_isHighlighted( false ),
            m_isSelected( false )
        {
            ImGuiElement<T>::setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
            ImGuiElement<T>::setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
            ImGuiElement<T>::setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
            ImGuiElement<T>::setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

            auto name = String( "GUIElement" ) + StringUtil::toString( m_nextGeneratedNameExt++ );
            ImGuiElement<T>::setName( name );

            m_layout = nullptr;

            m_size = Vector2F::zero();

#if WP_TRACK_REFERENCES
            setObjectFlag( OBJECT_FLAG_TRACK_REFERENCES, true );
#endif
        }

        template <class T>
        ImGuiElement<T>::~ImGuiElement()
        {
        }

        template <class T>
        void ImGuiElement<T>::load( SmartPtr<ISharedObject> data )
        {
        }

        template <class T>
        void ImGuiElement<T>::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                this->removeObjectListeners();

                setContainer( nullptr );

                m_parent = nullptr;

                m_layout = nullptr;
                m_userProperties = nullptr;

                m_dragSource = nullptr;
                m_dropTarget = nullptr;

                m_container = nullptr;

                m_scriptInvoker = nullptr;
                m_scriptReceiver = nullptr;

                m_scriptData = nullptr;

                auto children = getChildren();
                for( auto &child : children )
                {
                    if( child )
                    {
                        child->unload( nullptr );
                    }
                }

                m_children.clear();

                auto stateManager = applicationManager->getStateManager();
                if( stateManager )
                {
                    if( auto stateContext = getStateContext() )
                    {
                        if( auto stateListener = getStateListener() )
                        {
                            stateContext->removeStateListener( stateListener );
                            setStateListener( nullptr );
                        }

                        stateManager->removeStateContext( stateContext );

                        setStateContext( nullptr );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void ImGuiElement<T>::update()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto timer = applicationManager->getTimer();

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            if( task != TaskId::Application )
                return;

            if( !m_isVisible )
                return;

            if( !m_isEnabled )
                return;

            //if( !m_listeners.empty() )
            //{
            //    Parameters params;
            //    params[0].setPtr( this );
            //    params[1].setU32( static_cast<u32>( task ) );
            //    params[2].setF64( t );
            //    params[3].setF64( dt );
            //
            //    // for (u32 i = 0; i < m_listeners.size(); ++i)
            //    //{
            //    //	m_listeners[i]->OnEvent(StringUtil::getHash("update"), params);
            //    // }
            //}

            // for(u32 i=0; i<m_children.size(); ++i)
            //{
            //	m_children[i]->update();
            // }
        }

        template <class T>
        bool ImGuiElement<T>::handleEvent( const SmartPtr<IInputEvent> &event )
        {
            // if(!m_isEnabled)
            //	return false;

            // for(u32 i=0; i<m_inputListeners.size(); ++i)
            //{
            //	IGUIItemInputListener* inputListener = m_inputListeners[i];
            //	if(inputListener->onEvent(event))
            //		return true;
            // }

            // for (u32 i = 0; i < m_children.size(); ++i)
            //{
            //	CUIElement* pGUIItem = (CUIElement*)m_children[i];
            //
            //	if (pGUIItem->getName() == ("CreateShip0"))
            //	{
            //		int i = 0;
            //		i = i;
            //	}
            //
            //	if (pGUIItem->onEvent(event))
            //	{
            //		return true;
            //	}
            //}

            return false;
        }

        template <class T>
        void ImGuiElement<T>::invalidate()
        {
        }

        template <class T>
        Vector2F ImGuiElement<T>::getAbsolutePosition() const
        {
            if( m_parent )
            {
                return m_parent->getAbsolutePosition() + m_position;
            }

            return m_position;
        }

        template <class T>
        void ImGuiElement<T>::setEnabled( bool enabled, bool cascade )
        {
            if( m_isEnabled != enabled )
            {
                m_isEnabled = enabled;

                if( cascade )
                {
                    auto children = getChildren();
                    for( auto &child : children )
                    {
                        child->setEnabled( m_isEnabled );
                    }
                }

                /*if(!m_isEnabled && m_hasFocus)
                setFocus( false );*/

                onToggleEnabled();
            }
        }

        template <class T>
        void ImGuiElement<T>::setSelected( bool selected )
        {
            bool wasSelected = m_isSelected;
            m_isSelected = selected;

            if( wasSelected && !m_isSelected )
            {
                onDeselect();
            }
            else if( !wasSelected && m_isSelected )
            {
                onSelect();
            }
        }

        template <class T>
        bool ImGuiElement<T>::isSelected() const
        {
            return m_isSelected;
        }

        template <class T>
        void ImGuiElement<T>::setVisible( bool visible, bool cascade )
        {
            if( m_isVisible != visible )
            {
                m_isVisible = visible;

                if( cascade )
                {
                    auto children = getChildren();

                    for( auto &child : children )
                    {
                        if( child )
                        {
                            child->setVisible( m_isVisible, cascade );
                        }
                    }
                }

                onToggleVisibility();
            }
        }

        template <class T>
        void ImGuiElement<T>::setHovered( bool hovered )
        {
            m_isHovered = hovered;
        }

        template <class T>
        bool ImGuiElement<T>::isHovered() const
        {
            return m_isHovered;
        }

        template <class T>
        void ImGuiElement<T>::setFocus( bool hasFocus )
        {
            // if (m_hasFocus && !hasFocus)
            //	onLostFocus();
            //
            // m_hasFocus = hasFocus;
            //
            // for (u32 i = 0; i < m_children.size(); ++i)
            //{
            //	((CUIElement*)m_children[i])->setFocus(m_hasFocus);
            // }
            //
            // if (m_hasFocus)
            //	onGainFocus();
        }

        template <class T>
        void ImGuiElement<T>::setHighlighted( bool isHighlighted, bool cascade )
        {
            // if (m_isHighlighted != isHighlighted)
            //{
            //	m_isHighlighted = isHighlighted;
            //
            //	if (cascade)
            //	{
            //		for (u32 i = 0; i < m_children.size(); ++i)
            //		{
            //			((CUIElement*)m_children[i])->setHighlighted(m_isHighlighted);
            //		}
            //	}
            //
            //	onToggleHighlight();
            //}
        }

        template <class T>
        u32 ImGuiElement<T>::getNumChildren() const
        {
            return (u32)m_children.size();
        }

        template <class T>
        Array<SmartPtr<IUIElement>> ImGuiElement<T>::getChildren() const
        {
            return Array<SmartPtr<IUIElement>>( m_children.begin(), m_children.end() );
        }

        template <class T>
        void ImGuiElement<T>::addChild( SmartPtr<IUIElement> pGUIItem )
        {
            ScopedLock lock( this );

            if( pGUIItem )
            {
                pGUIItem->remove();  // remove from old parent
                pGUIItem->setParent( this );

                m_children.push_back( pGUIItem );

                auto layout = getLayout();
                pGUIItem->setLayout( layout );

                onAddChild( pGUIItem.get() );
            }
        }

        template <class T>
        bool ImGuiElement<T>::removeChild( SmartPtr<IUIElement> element )
        {
            if( element )
            {
                m_children.erase( std::remove( m_children.begin(), m_children.end(), element ),
                                  m_children.end() );

                element->setParent( nullptr );
                onRemoveChild( element.get() );
                return true;
            }

            return false;
        }

        template <class T>
        void ImGuiElement<T>::remove()
        {
            if( m_parent )
            {
                m_parent->removeChild( this );
            }
        }

        template <class T>
        void ImGuiElement<T>::removeAllChildren()
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    child->setParent( nullptr );
                }
            }

            m_children.clear();
        }

        template <class T>
        void ImGuiElement<T>::destroyAllChildren()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto ui = applicationManager->getUIPtr();

            auto children = getChildren();

            for( auto &child : children )
            {
                if( child )
                {
                    child->destroyAllChildren();
                }
            }

            for( auto &child : children )
            {
                if( child )
                {
                    ui->removeElement( child );
                }
            }

            m_children.clear();
        }

        template <class T>
        bool ImGuiElement<T>::hasChildById( const String &id ) const
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    auto childElement = workphone::static_pointer_cast<ImGuiElement<T>>( child );
                    if( id == childElement->getName() )
                    {
                        return true;
                    }
                }
            }

            for( auto child : children )
            {
                if( child )
                {
                    auto childElement = workphone::static_pointer_cast<ImGuiElement<T>>( child );
                    if( childElement->hasChildById( id ) )
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        template <class T>
        SmartPtr<IUIElement> ImGuiElement<T>::findChildById( const String &id ) const
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    auto childElement = workphone::static_pointer_cast<ImGuiElement<T>>( child );
                    if( id == childElement->getName() )
                    {
                        return child;
                    }
                }
            }

            for( auto child : children )
            {
                if( child )
                {
                    auto childElement = workphone::static_pointer_cast<ImGuiElement<T>>( child );
                    if( childElement->hasChildById( id ) )
                    {
                        return child;
                    }
                }
            }

            return nullptr;
        }

        template <class T>
        s32 ImGuiElement<T>::getSiblingIndex() const
        {
            if( m_parent )
            {
                auto pThis = ImGuiElement<T>::template getSharedFromThis<IUIElement>();
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
        void ImGuiElement<T>::setUserData( void *pUserData )
        {
            m_userData = pUserData;
        }

        template <class T>
        void *ImGuiElement<T>::getUserData() const
        {
            return m_userData;
        }

        template <class T>
        void ImGuiElement<T>::addAnimator( SmartPtr<IAnimator> &animator )
        {
        }

        template <class T>
        bool ImGuiElement<T>::removeAnimator( SmartPtr<IAnimator> &animator )
        {
            return false;
        }

        template <class T>
        SmartPtr<IUILayoutContainer> ImGuiElement<T>::getContainer() const
        {
            return m_container;
        }

        template <class T>
        void ImGuiElement<T>::setContainer( SmartPtr<IUILayoutContainer> container )
        {
            m_container = container;
        }

        template <class T>
        void ImGuiElement<T>::_getObject( void **ppObject ) const
        {
            *ppObject = nullptr;
        }

        //
        // Callbacks
        //

        template <class T>
        void ImGuiElement<T>::_onInitialiseStart()
        {
        }

        template <class T>
        void ImGuiElement<T>::_onInitialiseEnd()
        {
            //if( !m_listeners.empty() )
            //{
            //    Parameters params;
            //    // params.set_used(2);
            //    params[0].setPtr( this );
            //
            //    // for (u32 i = 0; i < m_listeners.size(); ++i)
            //    //{
            //    //	m_listeners[i]->OnEvent(StringUtil::getHash("initialise"), params);
            //    // }
            //}
        }

        template <class T>
        void ImGuiElement<T>::onAddChild( IUIElement *child )
        {
            //for( u32 i = 0; i < m_listeners.size(); ++i )
            //    m_listeners[i]->handleAddChild( this, child );
        }

        template <class T>
        void ImGuiElement<T>::onRemoveChild( IUIElement *child )
        {
            //for( u32 i = 0; i < m_listeners.size(); ++i )
            //    m_listeners[i]->handleRemoveChild( this, child );
        }

        template <class T>
        void ImGuiElement<T>::onChangedState()
        {
            //for( u32 i = 0; i < m_listeners.size(); ++i )
            //    m_listeners[i]->handleChangedState( this );
        }

        template <class T>
        void ImGuiElement<T>::onChildChangedState( IUIElement *child )
        {
            //for( u32 i = 0; i < m_listeners.size(); ++i )
            //    m_listeners[i]->handleChildChangedState( this, child );
        }

        template <class T>
        void ImGuiElement<T>::onToggleEnabled()
        {
            //for( u32 i = 0; i < m_listeners.size(); ++i )
            //    m_listeners[i]->handleToggleEnabled( this );
            //
            // if (m_parent)
            //	m_parent->onChildChangedState(this);
        }

        template <class T>
        void ImGuiElement<T>::onToggleVisibility()
        {
            //for( u32 i = 0; i < m_listeners.size(); ++i )
            //    m_listeners[i]->handleToggleVisibility( this );
            //
            // if (m_parent)
            //	m_parent->onChildChangedState(this);

            if( isVisible() )
            {
                //if( !m_listeners.empty() )
                //{
                //    Parameters params;
                //    params.resize( 1 );
                //    params[0].setPtr( this );
                //
                //    // for (u32 i = 0; i < m_listeners.size(); ++i)
                //    //	m_listeners[i]->OnEvent(StringUtil::getHash("show"), params);
                // }
            }
            else
            {
                //if( !m_listeners.empty() )
                //{
                //    Parameters params;
                //    params.resize( 1 );
                //    params[0].setPtr( this );
                //
                //    // for (u32 i = 0; i < m_listeners.size(); ++i)
                //    //	m_listeners[i]->OnEvent(StringUtil::getHash("hide"), params);
                // }
            }
        }

        template <class T>
        void ImGuiElement<T>::onToggleHighlight()
        {
            //for( u32 i = 0; i < m_listeners.size(); ++i )
            //    m_listeners[i]->handleToggleHighlight( this );
            //
            // if (m_parent)
            //	m_parent->onChildChangedState(this);
        }

        template <class T>
        void ImGuiElement<T>::onActivate( SmartPtr<IUIElement> element )
        {
            // if (!m_listeners.empty())
            //{
            //	Parameters params;
            //	//params.set_used(1);
            //	params[0].setPtr(element.get());
            //
            //	for (u32 i = 0; i < m_listeners.size(); ++i)
            //		m_listeners[i]->OnEvent(StringUtil::getHash("activate"), params);
            //}
            //
            // for (u32 i = 0; i < m_children.size(); ++i)
            //	((CUIElement*)m_children[i])->onActivate(element);
            //
            // if (m_parent)
            //	((CUIElement*)m_parent)->onChildChangedState(this);
        }

        template <class T>
        void ImGuiElement<T>::onDeactivate()
        {
            // for (u32 i = 0; i < m_listeners.size(); ++i)
            //	m_listeners[i]->onDeactivate(this);
            //
            // for (u32 i = 0; i < m_children.size(); ++i)
            //	((CUIElement*)m_children[i])->onDeactivate();
            //
            // if (m_parent)
            //	((CUIElement*)m_parent)->onChildChangedState(this);
        }

        template <class T>
        void ImGuiElement<T>::onSelect()
        {
            // for (u32 i = 0; i < m_listeners.size(); ++i)
            //	m_listeners[i]->onSelect(this);
            //
            // for (u32 i = 0; i < m_children.size(); ++i)
            //	((CUIElement*)m_children[i])->onSelect();
            //
            // if (m_parent)
            //	((CUIElement*)m_parent)->onChildChangedState(this);
        }

        template <class T>
        void ImGuiElement<T>::onDeselect()
        {
            // for (u32 i = 0; i < m_listeners.size(); ++i)
            //	m_listeners[i]->onDeselect(this);
            //
            // for (u32 i = 0; i < m_children.size(); ++i)
            //	((CUIElement*)m_children[i])->onDeselect();
            //
            // if (m_parent)
            //	((CUIElement*)m_parent)->onChildChangedState(this);
        }

        template <class T>
        void ImGuiElement<T>::onGainFocus()
        {
            //for( u32 i = 0; i < m_listeners.size(); ++i )
            //    m_listeners[i]->handleGainFocus( this );
            //
            // if (m_parent)
            //	m_parent->onChildChangedState(this);
        }

        template <class T>
        void ImGuiElement<T>::onLostFocus()
        {
            //for( u32 i = 0; i < m_listeners.size(); ++i )
            //    m_listeners[i]->handleLostFocus( this );
            //
            // if (m_parent)
            //	m_parent->onChildChangedState(this);
        }

        template <class T>
        void ImGuiElement<T>::onEvent( const String &eventType )
        {
            // for (u32 i = 0; i < m_children.size(); ++i)
            //	((CUIElement*)m_children[i])->onEvent(eventType);
            //
            // for (u32 i = 0; i < m_listeners.size(); ++i)
            //	m_listeners[i]->onEvent(eventType);
        }

        template <class T>
        s32 ImGuiElement<T>::setProperty( hash32 hash, const Parameter &param )
        {
            // if(hash == StringUtil::VISIBLE_HASH)
            //{
            //	setVisible(param.getBool());
            // }

            return 0;
        }

        template <class T>
        s32 ImGuiElement<T>::getObject( u32 hash, SmartPtr<ISharedObject> &object ) const
        {
            if( hash == StringUtil::getHash( "userProperties" ) )
            {
                // if(!m_userProperties)
                //	m_userProperties = SmartPtr<ISharedObject>(new GUIElementProperties, true);

                object = m_userProperties;
            }
            else if( hash == StringUtil::getHash( "animations" ) )
            {
                // object = m_animatorContainer;
            }

            // for(u32 i=0; i<m_children.size(); ++i)
            //{
            //	GUIElement* pGUIItem = (GUIElement*)m_children[i];
            //	if(hash == pGUIItem->getHashId())
            //	{
            //		object = pGUIItem;
            //	}
            // }

            // SmartPtr<IGUIElement> pCompGUIItem;
            // for(u32 i=0; i<m_children.size(); ++i)
            //{
            //	GUIElement* pGUIItem = (GUIElement*)m_children[i];
            //	pGUIItem->getObject(hash, object);
            //	if(!object.isNull())
            //		return 0;
            // }

            return 0;
        }

        template <class T>
        String ImGuiElement<T>::getLabel() const
        {
            return m_label;
        }

        template <class T>
        void ImGuiElement<T>::setLabel( const String &label )
        {
            m_label = label;
        }

        template <class T>
        void ImGuiElement<T>::setPosition( const Vector2F &position )
        {
            m_position = position;
        }

        template <class T>
        SmartPtr<IScriptInvoker> &ImGuiElement<T>::getInvoker()
        {
            return m_scriptInvoker;
        }

        template <class T>
        const SmartPtr<IScriptInvoker> &ImGuiElement<T>::getInvoker() const
        {
            return m_scriptInvoker;
        }

        template <class T>
        void ImGuiElement<T>::setInvoker( SmartPtr<IScriptInvoker> invoker )
        {
            m_scriptInvoker = invoker;
        }

        template <class T>
        SmartPtr<IScriptReceiver> &ImGuiElement<T>::getReceiver()
        {
            return m_scriptReceiver;
        }

        template <class T>
        const SmartPtr<IScriptReceiver> &ImGuiElement<T>::getReceiver() const
        {
            return m_scriptReceiver;
        }

        template <class T>
        void ImGuiElement<T>::setReceiver( SmartPtr<IScriptReceiver> receiver )
        {
            m_scriptReceiver = receiver;
        }

        template <class T>
        void ImGuiElement<T>::_setData( SmartPtr<IScriptData> data )
        {
            m_scriptData = data;
        }

        template <class T>
        SmartPtr<IScriptData> ImGuiElement<T>::_getData() const
        {
            return m_scriptData;
        }

        template <class T>
        void ImGuiElement<T>::setComponentID( const String &componentId )
        {
            m_componentId = componentId;
        }

        template <class T>
        const String &ImGuiElement<T>::getComponentID() const
        {
            return m_componentId;
        }

        template <class T>
        void ImGuiElement<T>::setType( const String &type )
        {
            m_type = type;
        }

        template <class T>
        const String &ImGuiElement<T>::getType() const
        {
            return m_type;
        }

        template <class T>
        Vector2F ImGuiElement<T>::getPosition() const
        {
            return m_position;
        }

        template <class T>
        void ImGuiElement<T>::setSize( const Vector2F &size )
        {
            m_size = size;
        }

        template <class T>
        Vector2F ImGuiElement<T>::getSize() const
        {
            return m_size;
        }

        template <class T>
        f32 ImGuiElement<T>::getScale() const
        {
            return m_scale;
        }

        template <class T>
        void ImGuiElement<T>::setScale( f32 scale )
        {
            m_scale = scale;
        }

        template <class T>
        bool ImGuiElement<T>::isEnabled() const
        {
            return m_isEnabled;
        }

        template <class T>
        bool ImGuiElement<T>::isVisible() const
        {
            return m_isVisible;
        }

        template <class T>
        bool ImGuiElement<T>::isInFocus() const
        {
            return m_hasFocus;
        }

        template <class T>
        bool ImGuiElement<T>::isHighlighted() const
        {
            return m_isHighlighted;
        }

        template <class T>
        s32 ImGuiElement<T>::setProperty( hash32 hash, const String &value )
        {
            return 0;
        }

        template <class T>
        s32 ImGuiElement<T>::setProperty( hash32 hash, void *param )
        {
            return 0;
        }

        template <class T>
        SmartPtr<IUIElement> ImGuiElement<T>::getLayout() const
        {
            return m_layout.lock();
        }

        template <class T>
        void ImGuiElement<T>::setLayout( SmartPtr<IUIElement> layout )
        {
            m_layout = layout;
        }

        template <class T>
        void ImGuiElement<T>::handleEvent( const SmartPtr<IEvent> &event )
        {
        }

        template <class T>
        s32 ImGuiElement<T>::setProperty( hash32 hash, const Parameters &params )
        {
            return 0;
        }

        template <class T>
        s32 ImGuiElement<T>::getProperty( hash32 hash, Parameters &params ) const
        {
            return 0;
        }

        template <class T>
        s32 ImGuiElement<T>::getProperty( hash32 hash, Parameter &param )
        {
            return 0;
        }

        // template <class T>
        // s32 CUIElement<T>::getProperty(hash32 hash, const Parameter& param)
        // {
        //	return 0;
        // }

        template <class T>
        s32 ImGuiElement<T>::getProperty( hash32 hash, String &value ) const
        {
            return 0;
        }

        template <class T>
        s32 ImGuiElement<T>::getProperty( hash32 hash, void *param ) const
        {
            return 0;
        }

        template <class T>
        s32 ImGuiElement<T>::callFunction( u32 hash, SmartPtr<ISharedObject> object,
                                           Parameters &results )
        {
            return 0;
        }

        template <class T>
        s32 ImGuiElement<T>::callFunction( u32 hash, const Parameters &params, Parameters &results )
        {
            return 0;
        }

        template <class T>
        hash_type ImGuiElement<T>::getElementId() const
        {
            return m_itemId;
        }

        template <class T>
        void ImGuiElement<T>::setElementId( hash_type itemId )
        {
            m_itemId = itemId;
        }

        template <class T>
        SmartPtr<IUIElement> ImGuiElement<T>::getParent() const
        {
            return m_parent.lock();
        }

        template <class T>
        void ImGuiElement<T>::setParent( SmartPtr<IUIElement> parent )
        {
            m_parent = parent;
        }

        template <class T>
        SmartPtr<ISharedObject> ImGuiElement<T>::getOwner() const
        {
            return m_owner.lock();
        }

        template <class T>
        void ImGuiElement<T>::setOwner( SmartPtr<ISharedObject> owner )
        {
            m_owner = owner;
        }

        template <class T>
        SmartPtr<Properties> ImGuiElement<T>::getProperties() const
        {
            auto properties = core::Prototype<T>::getProperties();
            properties->setProperty( "name", this->getName() );
            properties->setProperty( "position", this->getPosition() );
            properties->setProperty( "size", this->getSize() );
            return properties;
        }

        template <class T>
        void ImGuiElement<T>::setProperties( SmartPtr<Properties> properties )
        {
            core::Prototype<T>::setProperties( properties );
        }

        template <class T>
        Array<SmartPtr<ISharedObject>> ImGuiElement<T>::getChildObjects() const
        {
            return {};
        }

        template <class T>
        bool ImGuiElement<T>::isDragDropSource() const
        {
            return m_dragDropSource;
        }

        template <class T>
        void ImGuiElement<T>::setDragDropSource( bool dragDropSource )
        {
            m_dragDropSource = dragDropSource;
        }

        template <class T>
        SmartPtr<IUIDragSource> ImGuiElement<T>::getDragSource() const
        {
            return m_dragSource;
        }

        template <class T>
        void ImGuiElement<T>::setDragSource( SmartPtr<IUIDragSource> dragSource )
        {
            m_dragSource = dragSource;
        }

        template <class T>
        SmartPtr<IUIDropTarget> ImGuiElement<T>::getDropTarget() const
        {
            return m_dropTarget;
        }

        template <class T>
        void ImGuiElement<T>::setDropTarget( SmartPtr<IUIDropTarget> dropTarget )
        {
            m_dropTarget = dropTarget;
        }

        template <class T>
        u32 ImGuiElement<T>::getOrder() const
        {
            return m_order;
        }

        template <class T>
        void ImGuiElement<T>::setOrder( u32 order )
        {
            m_order = order;
        }

        template <class T>
        bool ImGuiElement<T>::getRenderChildren() const
        {
            return m_renderChildren;
        }

        template <class T>
        void ImGuiElement<T>::setRenderChildren( bool renderChildren )
        {
            m_renderChildren = renderChildren;
        }

        template <class T>
        SmartPtr<IStateContext> ImGuiElement<T>::getStateContext() const
        {
            return m_stateContext;
        }

        template <class T>
        void ImGuiElement<T>::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        template <class T>
        SmartPtr<IStateListener> ImGuiElement<T>::getStateListener() const
        {
            return m_stateListener;
        }

        template <class T>
        void ImGuiElement<T>::setStateListener( SmartPtr<IStateListener> stateListener )
        {
            m_stateListener = stateListener;
        }

        template <class T>
        bool ImGuiElement<T>::getSameLine() const
        {
            return m_sameLine;
        }

        template <class T>
        void ImGuiElement<T>::setSameLine( bool sameLine )
        {
            m_sameLine = sameLine;
        }

        template <class T>
        ColourF ImGuiElement<T>::getColour() const
        {
            return m_colour;
        }

        template <class T>
        void ImGuiElement<T>::setColour( const ColourF &colour )
        {
            m_colour = colour;
        }

        template <class T>
        bool ImGuiElement<T>::getHandleInputEvents() const
        {
            return m_handleInputEvents;
        }

        template <class T>
        void ImGuiElement<T>::setHandleInputEvents( bool handleInputEvents )
        {
            m_handleInputEvents = handleInputEvents;
        }

        template <class T>
        void ImGuiElement<T>::sortZOrder()
        {
        }

        template <class T>
        void ImGuiElement<T>::updateZOrder()
        {
        }

        template <class T>
        void ImGuiElement<T>::setChildren( SharedPtr<ConcurrentArray<SmartPtr<IUIElement>>> p )
        {
            if( p )
            {
                m_children = *p;
            }
            else
            {
                m_children.clear();
            }
        }

        template <class T>
        void ImGuiElement<T>::lock()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto ui = applicationManager->getUI();
            ui->lock();
        }

        template <class T>
        void ImGuiElement<T>::unlock()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto ui = applicationManager->getUI();
            ui->unlock();
        }
    }  // end namespace ui
}  // namespace workphone

#endif
