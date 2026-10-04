#ifndef UIElement_h__
#define UIElement_h__

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/UI/IUILayoutContainer.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/Animation/IAnimator.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/State/States/UIElementStateData.hpp>
#include <Workphone/State/States/UITransformStateData.hpp>
#include <Workphone/System/Prototype.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <algorithm>

namespace workphone
{
    namespace render
    {
        class IMaterial;
    }

    namespace ui
    {
        /**
         * @class UIElement
         * @brief Base implementation for UI elements.
         *
         * UIElement provides common functionality used by concrete UI widgets and containers:
         * - Hierarchy management (parent/children)
         * - State-backed properties (position, size, flags, colour, z-order)
         * - Basic event hooks that derived classes can override
         *
         * The class is implemented as a CRTP template: `T` should be the concrete derived type.
         *
         * @tparam T The concrete derived UI element type (CRTP).
         */
        template <class T>
        class UIElement : public core::Prototype<T>
        {
        public:
            /**
             * @brief Construct a UIElement.
             *
             * Default constructor initializes internal fields. Many runtime-visible
             * properties (position, size, flags, etc.) are stored in the associated
             * IStateContext and accessed lazily via state objects.
             */
            UIElement();

            /**
             * @brief Construct a UIElement with a specific pool type ID.
             *
             * @param poolTypeId Identifier for the memory pool type.
             */
            UIElement( u32 poolTypeId );

            /**
             * @brief Virtual destructor.
             *
             * Cleans up references; state/children may be released by derived classes
             * during unload.
             */
            ~UIElement() override;

            /**
             * @brief Unload and release renderer and runtime resources owned by this element.
             *
             * This removes the element from its parent, clears child lists and resets
             * internal pointers (container, owner, properties, state context/listener,
             * drag/drop targets).
             *
             * @param data Optional data pointer passed by the caller (currently unused).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Perform per-frame update for this element.
             *
             * Base implementation is empty; derived types implement animation, layout
             * or other per-frame behaviour.
             */
            void update() override;

            /**
             * @brief Handle an input event targeted at this element.
             *
             * Override in derived classes to process pointer/key input.
             *
             * @param event Input event object.
             * @return true if the event was handled and should not propagate further.
             */
            virtual bool handleEvent( const SmartPtr<IInputEvent> &event );

            /**
             * @brief Get element identifier (hash).
             *
             * The id is stored in the element's UIElementState when a state context is present.
             *
             * @return Element hash id or 0 if unavailable.
             */
            virtual hash_type getElementId() const;

            /**
             * @brief Set element identifier (hash).
             *
             * Stores the id in the element's UIElementState via the state context.
             *
             * @param elementId Hash id to assign to this element.
             */
            virtual void setElementId( hash_type elementId );

            /**
             * @brief Get the textual label/name for this element.
             *
             * The label is typically used to locate elements by id or for debugging.
             *
             * @return Label string or empty string if none set.
             */
            String getLabel() const;

            /**
             * @brief Set a textual label for this element.
             *
             * @param label New label string.
             */
            void setLabel( const String &label );

            /**
             * @brief Set the element's local position.
             *
             * Coordinates are expressed in the UI's normalized/reference coordinate system.
             * Uses UITransformState when a state context is present.
             *
             * @param position New local position.
             */
            virtual void setPosition( const Vector2<real_Num> &position );

            /**
             * @brief Get the element's local position.
             *
             * @return Local position from UITransformState or zero vector if unavailable.
             */
            Vector2<real_Num> getPosition() const;

            /**
             * @brief Compute absolute position in the UI hierarchy.
             *
             * This returns the sum of this element's local position plus the absolute
             * position of its parent chain.
             *
             * @return Absolute position in parent coordinate space.
             */
            Vector2<real_Num> getAbsolutePosition() const;

            /**
             * @brief Set the element size.
             *
             * The size is stored in UITransformState when a state context is present.
             *
             * @param size New element size (width, height).
             */
            virtual void setSize( const Vector2<real_Num> &size );

            /**
             * @brief Get the element size.
             *
             * @return Size stored in UITransformState or zero vector if unavailable.
             */
            Vector2<real_Num> getSize() const;

            /**
             * @brief Get the element scale factor.
             *
             * @return Scale value from UITransformState or 1.0f if unavailable.
             */
            f32 getScale() const;

            /**
             * @brief Set the element scale factor.
             *
             * @param scale Scale multiplier (1.0 = default).
             */
            void setScale( f32 scale );

            /**
             * @brief Enable or disable the element.
             *
             * When disabled, elements typically do not process input or participate in
             * interaction. The enabled flag is stored in UIElementState.
             *
             * @param enabled New enabled state.
             * @param cascade If true, propagate the enabled state to all children.
             */
            void setEnabled( bool enabled, bool cascade = true );

            /**
             * @brief Query whether the element is enabled.
             *
             * @return true if enabled; false otherwise.
             */
            bool isEnabled() const;

            /**
             * @brief Mark element selected/deselected.
             *
             * Triggers onSelect/onDeselect callbacks when selection state changes.
             *
             * @param selected New selection state.
             */
            void setSelected( bool selected );

            /**
             * @brief Returns whether the element is selected.
             *
             * @return true if selected.
             */
            bool isSelected() const;

            /**
             * @brief Set hovered state (pointer-over).
             *
             * @param hovered true when pointer is over this element.
             */
            void setHovered( bool hovered );

            /**
             * @brief Query pointer hover state.
             *
             * @return true when element is hovered.
             */
            bool isHovered() const;

            /**
             * @brief Set element visibility.
             *
             * Visibility is stored in UIElementState. When cascade is true, children
             * receive the same visibility state recursively.
             *
             * @param isVisible True to show, false to hide.
             * @param cascade If true, propagate visibility to children.
             */
            virtual void setVisible( bool isVisible, bool cascade = true );

            /**
             * @brief Returns true when the element is visible.
             *
             * @return Visibility flag value.
             */
            bool isVisible() const;

            /**
             * @brief Query whether this element is configured to receive input events.
             *
             * The flag is backed by UIElementState and controls whether handleEvent is
             * expected to be called for this element.
             *
             * @return true if input events should be handled by this element.
             */
            bool getHandleInputEvents() const;

            /**
             * @brief Enable or disable input event handling for this element.
             *
             * @param handleInputEvents true to enable, false to disable.
             */
            void setHandleInputEvents( bool handleInputEvents );

            /**
             * @brief Give or remove focus from this element.
             *
             * Focus is stored in UIElementState. Focus change triggers onGainFocus/onLostFocus.
             *
             * @param hasFocus true to give focus, false to remove it.
             */
            virtual void setFocus( bool hasFocus );

            /**
             * @brief Query whether the element or one of its descendants has focus.
             *
             * This method checks the element's focused flag and recursively queries children.
             *
             * @return true if this element or a descendant is focused.
             */
            bool isInFocus() const;

            /**
             * @brief Set element highlighted state.
             *
             * Highlighting is typically used for hover/selection visual feedback.
             *
             * @param isHighlighted New highlighted state.
             * @param cascade If true propagate to children.
             */
            void setHighlighted( bool isHighlighted, bool cascade = true );

            /**
             * @brief Query whether the element is highlighted.
             *
             * @return true when highlighted.
             */
            bool isHighlighted() const;

            /**
             * @brief Get the parent UI element or null if none.
             *
             * @return SmartPtr to parent element or nullptr.
             */
            SmartPtr<IUIElement> getParent() const;

            /**
             * @brief Set the parent UI element.
             *
             * This does not update the parent's child list; addChild/removeChild manage both sides.
             *
             * @param parent New parent element (may be nullptr).
             */
            void setParent( SmartPtr<IUIElement> parent );

            u32 getNumChildren() const;

            /**
             * @brief Get a snapshot array of children.
             *
             * Returns a thread-safe copy/snapshot of children.
             *
             * @return Array of child element smart pointers.
             */
            Array<SmartPtr<IUIElement>> getChildren() const;

            /**
             * @brief Access the internal thread-safe children container.
             *
             * Use with care; direct manipulation may break invariants. Prefer addChild/removeChild.
             *
             * @return Reference to internal ConcurrentArray of children.
             */
            ConcurrentArray<SmartPtr<IUIElement>> &getChildrenRef();

            /**
             * @brief Const access to the internal children container.
             *
             * @return Const reference to internal ConcurrentArray of children.
             */
            const ConcurrentArray<SmartPtr<IUIElement>> &getChildrenRef() const;

            /**
             * @brief Replace the children container from a snapshot.
             *
             * Existing children are replaced; callers should ensure parent pointers are consistent.
             *
             * @param children New children to set.
             */
            void setChildren( const Array<SmartPtr<IUIElement>> &children );

            /**
             * @brief Add a child element to this element.
             *
             * The child is detached from any previous parent, its parent is set to this,
             * and onAddChild is invoked.
             *
             * @param child Child element to add.
             */
            void addChild( SmartPtr<IUIElement> child ) override;

            /**
             * @brief Remove a specific child from this element.
             *
             * If the child is found it is unlinked (parent set to null) and onRemoveChild is invoked.
             *
             * @param child Child element to remove.
             * @return true if the child was found and removed; false otherwise.
             */
            bool removeChild( SmartPtr<IUIElement> child ) override;

            /**
             * @brief Remove this element from its current parent (if any).
             *
             * Equivalent to calling parent->removeChild(this) when parent exists.
             */
            void remove();

            /**
             * @brief Remove all children (unlink only).
             *
             * Children are unlinked (parent set to nullptr) but not destroyed or removed from UI
             * manager.
             */
            void removeAllChildren();

            /**
             * @brief Destroy all children and request UI manager remove them.
             *
             * For each child this calls UIManager::removeElement(child) and clears the local list.
             */
            void destroyAllChildren();

            /**
             * @brief Check if a descendant with the given label/id exists.
             *
             * Performs a recursive search.
             *
             * @param id Label or id to search for.
             * @return true if a descendant with the id exists.
             */
            virtual bool hasChildById( const String &id ) const;

            /**
             * @brief Find and return the first child or descendant with the given label/id.
             *
             * Performs a depth-first recursive search and returns the first match.
             *
             * @param id Label or id to search for.
             * @return SmartPtr to the found element or nullptr if not found.
             */
            virtual SmartPtr<IUIElement> findChildById( const String &id ) const;

            /**
             * @brief Get the zero-based index of this element in its parent's child list.
             *
             * @return Zero-based index, or -1 if no parent or not found in parent.
             */
            s32 getSiblingIndex() const;

            /**
             * @brief Get the layout owner associated with this element.
             *
             * Layout owner may be used by layout systems to group/position elements.
             *
             * @return SmartPtr to layout owner or nullptr.
             */
            SmartPtr<IUIElement> getLayout() const;

            /**
             * @brief Set layout owner.
             *
             * @param layout Layout owner element.
             */
            void setLayout( SmartPtr<IUIElement> layout );

            /** @copydoc IComponent::setUserData */
            void setUserData( void *userData ) override;

            /**
             * @brief Search object listeners by name across this element and its children.
             *
             * The method first checks the static object listeners on T (derived type),
             * then recursively searches child elements.
             *
             * @param id Listener name.
             * @return Found listener or nullptr.
             */
            SmartPtr<IEventListener> findObjectListener( const String &id ) const;

            /**
             * @brief Add an animator to the element.
             *
             * Base class is a no-op; derived elements can keep and update animators.
             *
             * @param animator Animator to add.
             */
            void addAnimator( SmartPtr<IAnimator> &animator );

            /**
             * @brief Remove an animator from the element.
             *
             * Base implementation returns false. Derived classes that manage animators
             * should override and return true when removal succeeds.
             *
             * @param animator Animator to remove.
             * @return true if removed; false otherwise.
             */
            bool removeAnimator( SmartPtr<IAnimator> &animator );

            /**
             * @brief Get the UI container this element belongs to.
             *
             * @return SmartPtr to the container or nullptr.
             */
            SmartPtr<IUILayoutContainer> getContainer() const;

            /**
             * @brief Set the UI container this element belongs to.
             *
             * @param container Container pointer (may be nullptr).
             */
            void setContainer( SmartPtr<IUILayoutContainer> container );

            /** @copydoc IComponent::getOwner */
            virtual SmartPtr<ISharedObject> getOwner() const;

            /** @copydoc IComponent::setOwner */
            virtual void setOwner( SmartPtr<ISharedObject> owner );

            /**
             * @brief Get native/platform object pointer for this element.
             *
             * Default implementation sets *ppObject to nullptr. Derived renderer-specific
             * implementations may return native handles.
             *
             * @param ppObject Output pointer to set to the native object.
             */
            void _getObject( void **ppObject ) const;

            /** @copydoc IComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Return related child objects for serialization/inspection.
             *
             * @return Array of ISharedObject smart pointers representing children.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Query whether the element is configured as a drag/drop source.
             *
             * @return true when drag source flag is set.
             */
            bool isDragDropSource() const;

            /**
             * @brief Enable or disable drag/drop source behavior.
             *
             * @param dragDropSource true to enable drag source behaviour.
             */
            void setDragDropSource( bool dragDropSource );

            /**
             * @brief Get the drag source helper object (if assigned).
             *
             * @return SmartPtr to IUIDragSource or nullptr.
             */
            SmartPtr<IUIDragSource> getDragSource() const;

            /**
             * @brief Set the drag source helper object.
             *
             * @param dragSource Drag source object to assign.
             */
            void setDragSource( SmartPtr<IUIDragSource> dragSource );

            /**
             * @brief Get the drop target helper object (if assigned).
             *
             * @return SmartPtr to IUIDropTarget or nullptr.
             */
            SmartPtr<IUIDropTarget> getDropTarget() const;

            /**
             * @brief Set the drop target helper object.
             *
             * @param dropTarget Drop target object to assign.
             */
            void setDropTarget( SmartPtr<IUIDropTarget> dropTarget );

            /**
             * @brief Get the rendering order (z-order).
             *
             * Lower values render behind higher values. The value is read from UITransformState.
             *
             * @return z-order value.
             */
            virtual u32 getOrder() const;

            /**
             * @brief Set the rendering order (z-order).
             *
             * @param order New z-order value.
             */
            virtual void setOrder( u32 order );

            /**
             * @brief Query whether children should be rendered.
             *
             * Controlled by a flag in UIElementState.
             *
             * @return true if children should be rendered.
             */
            bool getRenderChildren() const;

            /**
             * @brief Control whether children should be rendered.
             *
             * @param renderChildren true to render children.
             */
            void setRenderChildren( bool renderChildren );

            //
            // Events - callbacks that derived types can override
            //

            /**
             * @brief Called after a child has been added.
             *
             * Override to react to children being attached.
             *
             * @param child Raw pointer to added child.
             */
            virtual void onAddChild( IUIElement *child );

            /**
             * @brief Called after a child has been removed.
             *
             * Override to react to children being detached.
             *
             * @param child Raw pointer to removed child.
             */
            virtual void onRemoveChild( IUIElement *child );

            /**
             * @brief Called when this element's state data has changed.
             *
             * Override to react to state changes (e.g. repaint).
             */
            virtual void onChangedState();

            /**
             * @brief Called when a child element's state has changed.
             *
             * @param child Child element that changed.
             */
            virtual void onChildChangedState( IUIElement *child );

            /**
             * @brief Called when enabled state toggles.
             *
             * Override to update visuals/logic when enabled/disabled.
             */
            virtual void onToggleEnabled();

            /**
             * @brief Called when visibility toggles.
             *
             * Override to update visuals/logic when shown/hidden.
             */
            virtual void onToggleVisibility();

            /**
             * @brief Called when highlight toggles.
             *
             * Override to update highlight visuals.
             */
            virtual void onToggleHighlight();

            /**
             * @brief Called to activate an element.
             *
             * Activation may be used by selection/activation systems.
             *
             * @param element Element requesting activation (may be this or a child).
             */
            virtual void onActivate( SmartPtr<IUIElement> element );

            /**
             * @brief Called to deactivate element.
             *
             * Override to handle deactivation cleanup.
             */
            virtual void onDeactivate();

            /**
             * @brief Called when element is selected.
             *
             * Override to react to selection.
             */
            virtual void onSelect();

            /**
             * @brief Called when element is deselected.
             *
             * Override to react to deselection.
             */
            virtual void onDeselect();

            /**
             * @brief Called when element or a descendant gained focus.
             *
             * Override to react to focus gain.
             */
            virtual void onGainFocus();

            /**
             * @brief Called when element or a descendant lost focus.
             *
             * Override to react to focus loss.
             */
            virtual void onLostFocus();

            /**
             * @brief Generic event handler for internal (non-input) events.
             *
             * Base implementation is a no-op.
             *
             * @param event Event object.
             */
            virtual void handleEvent( const SmartPtr<IEvent> &event );

            /**
             * @brief Whether this element should be laid out on the same line as previous sibling.
             *
             * Used by simple layout engines to support layout.
             *
             * @return true when same-line layout is desired.
             */
            bool getSameLine() const;

            /**
             * @brief Set whether this element should be placed on the same line as previous sibling.
             *
             * @param sameLine true to place on same line.
             */
            void setSameLine( bool sameLine );

            /**
             * @brief Get the element colour.
             *
             * Colour is stored in UIElementState and can be used by renderers.
             *
             * @return ColourF value.
             */
            ColourF getColour() const;

            /**
             * @brief Set the element colour.
             *
             * @param colour New colour to assign.
             */
            void setColour( const ColourF &colour );

            /**
             * @brief Get current state context associated with this element.
             *
             * The state context provides access to UIElementState and UITransformState objects.
             *
             * @return State context or nullptr.
             */
            SmartPtr<IStateContext> getStateContext() const;

            /**
             * @brief Set the state context for this element.
             *
             * @param stateContext State context to associate with this element.
             */
            void setStateContext( SmartPtr<IStateContext> stateContext );

            /**
             * @brief Get the element's state listener.
             *
             * @return IStateListener pointer or nullptr.
             */
            SmartPtr<IStateListener> getStateListener() const;

            /**
             * @brief Set the element's state listener.
             *
             * @param stateListener Listener to receive state change notifications.
             */
            void setStateListener( SmartPtr<IStateListener> stateListener );

            /**
             * @brief Sort children by z-order (recursive).
             *
             * This reorders the internal children container so elements with lower getOrder()
             * render behind higher ordered elements.
             */
            void sortZOrder();

            /**
             * @brief Update z-order for this element and children (recursive).
             *
             * Intended as a hook for renderers to recalculate world z-order or transforms.
             */
            void updateZOrder();

            /**
             * @brief Handle notifications that a state object changed.
             *
             * Derived classes should inspect the provided state and update internal data.
             *
             * @param state Changed state object.
             * @return true if state was handled successfully.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state );

            /**
             * @brief Setup or update the render material used by this element.
             *
             * Renderers may supply material instances which can be cached/used by the element.
             * Base implementation is a no-op.
             *
             * @param material Material to use.
             */
            virtual void setupMaterial( SmartPtr<render::IMaterial> material );

            WP_CLASS_REGISTER_TEMPLATE_DECL( UIElement, T );

        protected:
            /** Weak pointer to parent element; use getParent() to obtain a strong pointer. */
            AtomicWeakPtr<IUIElement> m_parent;

            /** Weak pointer to the layout owner element (if any). */
            AtomicWeakPtr<IUIElement> m_layout;

            /** Container that owns or groups this element (renderer-specific). */
            AtomicSmartPtr<IUILayoutContainer> m_container;

            /** Generic owner pointer (component owning this element). */
            AtomicSmartPtr<ISharedObject> m_owner;

            /** Miscellaneous properties associated with this element. */
            AtomicSmartPtr<Properties> m_properties;

            /** State context that stores UIElementState/UITransformState for this element. */
            AtomicSmartPtr<IStateContext> m_stateContext;

            /** Optional listener that receives state change notifications for this element. */
            AtomicSmartPtr<IStateListener> m_stateListener;

            /** Optional drag source helper assigned to this element. */
            AtomicSmartPtr<IUIDragSource> m_dragSource;

            /** Optional drop target helper assigned to this element. */
            AtomicSmartPtr<IUIDropTarget> m_dropTarget;

            /** User data pointer for arbitrary consumer state. */
            void *m_userData = nullptr;

            /** Thread-safe container holding child elements. Use addChild/removeChild to maintain
             * invariants. */
            ConcurrentArray<SmartPtr<IUIElement>> m_children;

            static u32 m_idExt;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::ui, UIElement, T, T );

        template <class T>
        u32 UIElement<T>::m_idExt = 0;

        template <class T>
        UIElement<T>::UIElement() : core::Prototype<T>( UIElement<T>::typeInfo() )
        {
            this->setId( m_idExt++ );
        }

        template <class T>
        UIElement<T>::UIElement( u32 poolTypeId ) : core::Prototype<T>( poolTypeId )
        {
            this->setId( m_idExt++ );
        }

        template <class T>
        UIElement<T>::~UIElement() = default;

        template <class T>
        void UIElement<T>::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                // Remove from parent
                if( auto parent = getParent() )
                {
                    parent->removeChild( this );
                }

                // Clear children
                m_children.clear();

                // Clear references
                m_parent = nullptr;
                m_layout = nullptr;
                m_container = nullptr;
                m_owner = nullptr;
                m_properties = nullptr;
                m_stateContext = nullptr;
                m_stateListener = nullptr;
                m_dragSource = nullptr;
                m_dropTarget = nullptr;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void UIElement<T>::update()
        {
            // Base implementation - can be overridden by derived classes
        }

        template <class T>
        bool UIElement<T>::handleEvent( const SmartPtr<IInputEvent> &event )
        {
            // Base implementation returns false - derived classes should override
            return false;
        }

        template <class T>
        hash_type UIElement<T>::getElementId() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    return state->elementId;
                }
            }

            return 0;
        }

        template <class T>
        void UIElement<T>::setElementId( hash_type elementId )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->elementId = elementId;
                }
            }
        }

        template <class T>
        String UIElement<T>::getLabel() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    return state->label;
                }
            }

            return {};
        }

        template <class T>
        void UIElement<T>::setLabel( const String &label )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->label = label;
                }
            }
        }

        template <class T>
        void UIElement<T>::setPosition( const Vector2<real_Num> &position )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UITransformStateData>() )
                {
                    state->position = position;
                }
            }
        }

        template <class T>
        Vector2<real_Num> UIElement<T>::getPosition() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UITransformStateData>() )
                {
                    return state->position;
                }
            }

            return {};
        }

        template <class T>
        Vector2<real_Num> UIElement<T>::getAbsolutePosition() const
        {
            auto absolutePos = getPosition();

            if( auto parent = getParent() )
            {
                absolutePos += parent->getAbsolutePosition();
            }

            return absolutePos;
        }

        template <class T>
        void UIElement<T>::setSize( const Vector2<real_Num> &size )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UITransformStateData>() )
                {
                    state->size = size;
                }
            }
        }

        template <class T>
        Vector2<real_Num> UIElement<T>::getSize() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UITransformStateData>() )
                {
                    return state->size;
                }
            }

            return {};
        }

        template <class T>
        f32 UIElement<T>::getScale() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UITransformStateData>() )
                {
                    return state->scale;
                }
            }

            return 1.0f;
        }

        template <class T>
        void UIElement<T>::setScale( f32 scale )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UITransformStateData>() )
                {
                    state->scale = scale;
                }
            }
        }

        template <class T>
        void UIElement<T>::setEnabled( bool enabled, bool cascade )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::enabledFlag ), enabled );
                }
            }

            onToggleEnabled();

            if( cascade )
            {
                auto children = m_children.snapshot();
                for( auto &child : children )
                {
                    if( child )
                    {
                        child->setEnabled( enabled, true );
                    }
                }
            }
        }

        template <class T>
        bool UIElement<T>::isEnabled() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue( state->flags,
                                                  static_cast<u16>( IUIElement::enabledFlag ) );
                }
            }

            return false;
        }

        template <class T>
        void UIElement<T>::setSelected( bool selected )
        {
            if( isSelected() != selected )
            {
                if( auto stateContext = getStateContext() )
                {
                    if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                    {
                        state->flags = BitUtil::setFlagValue(
                            state->flags, static_cast<u16>( IUIElement::selectedFlag ), selected );
                    }
                }

                if( selected )
                {
                    onSelect();
                }
                else
                {
                    onDeselect();
                }
            }
        }

        template <class T>
        bool UIElement<T>::isSelected() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue( state->flags,
                                                  static_cast<u16>( IUIElement::selectedFlag ) );
                }
            }

            return false;
        }

        template <class T>
        void UIElement<T>::setHovered( bool hovered )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::hoveredFlag ), hovered );
                }
            }
        }

        template <class T>
        bool UIElement<T>::isHovered() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue( state->flags,
                                                  static_cast<u16>( IUIElement::hoveredFlag ) );
                }
            }

            return false;
        }

        template <class T>
        bool UIElement<T>::getHandleInputEvents() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue(
                        state->flags, static_cast<u16>( IUIElement::handleInputEventsFlag ) );
                }
            }

            return false;
        }

        template <class T>
        void UIElement<T>::setHandleInputEvents( bool handleInputEvents )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::handleInputEventsFlag ),
                        handleInputEvents );
                }
            }
        }

        template <class T>
        void UIElement<T>::setVisible( bool visible, bool cascade )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::visibleFlag ), visible );
                }
            }

            onToggleVisibility();

            if( cascade )
            {
                auto children = m_children.snapshot();
                for( auto &child : children )
                {
                    if( child )
                    {
                        child->setVisible( visible, true );
                    }
                }
            }
        }

        template <class T>
        bool UIElement<T>::isVisible() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue( state->flags,
                                                  static_cast<u16>( IUIElement::visibleFlag ) );
                }
            }

            return false;
        }

        template <class T>
        void UIElement<T>::setFocus( bool hasFocus )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::focusedFlag ), hasFocus );
                }
            }

            if( hasFocus )
            {
                onGainFocus();
            }
            else
            {
                onLostFocus();
            }
        }

        template <class T>
        bool UIElement<T>::isInFocus() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    if( BitUtil::getFlagValue( state->flags,
                                               static_cast<u16>( IUIElement::focusedFlag ) ) )
                    {
                        return true;
                    }
                }
            }

            // Check if any child has focus
            auto children = m_children.snapshot();
            for( auto &child : children )
            {
                if( child && child->isInFocus() )
                {
                    return true;
                }
            }

            return false;
        }

        template <class T>
        void UIElement<T>::setHighlighted( bool highlighted, bool cascade )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::highlightedFlag ), highlighted );
                }
            }

            onToggleHighlight();

            if( cascade )
            {
                auto children = m_children.snapshot();
                for( auto &child : children )
                {
                    if( child )
                    {
                        child->setHighlighted( highlighted, true );
                    }
                }
            }
        }

        template <class T>
        bool UIElement<T>::isHighlighted() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue( state->flags,
                                                  static_cast<u16>( IUIElement::highlightedFlag ) );
                }
            }

            return false;
        }

        template <class T>
        SmartPtr<IUIElement> UIElement<T>::getParent() const
        {
            auto p = m_parent.load();
            return p.lock();
        }

        template <class T>
        void UIElement<T>::setParent( SmartPtr<IUIElement> parent )
        {
            m_parent = parent;
        }

        template <class T>
        u32 UIElement<T>::getNumChildren() const
        {
            return (u32)m_children.size();
        }

        template <class T>
        Array<SmartPtr<IUIElement>> UIElement<T>::getChildren() const
        {
            return m_children.snapshot();
        }

        template <class T>
        ConcurrentArray<SmartPtr<IUIElement>> &UIElement<T>::getChildrenRef()
        {
            return m_children;
        }

        template <class T>
        const ConcurrentArray<SmartPtr<IUIElement>> &UIElement<T>::getChildrenRef() const
        {
            return m_children;
        }

        template <class T>
        void UIElement<T>::setChildren( const Array<SmartPtr<IUIElement>> &children )
        {
            m_children = { children.begin(), children.end() };
        }

        template <class T>
        void UIElement<T>::addChild( SmartPtr<IUIElement> child )
        {
            if( child )
            {
                // Remove from previous parent
                auto parent = child->getParent();
                if( parent != this )
                {
                    if( parent )
                    {
                        parent->removeChild( child );
                    }

                    // Set this as the new parent
                    child->setParent( this );

                    // Add to children list
                    m_children.push_back( child );

                    // Notify
                    onAddChild( child.get() );
                }
            }
        }

        template <class T>
        bool UIElement<T>::removeChild( SmartPtr<IUIElement> child )
        {
            if( child )
            {
                auto it = std::find( m_children.begin(), m_children.end(), child );

                if( it != m_children.end() )
                {
                    child->setParent( nullptr );
                    m_children.erase( it );
                    onRemoveChild( child.get() );
                    return true;
                }
            }

            return false;
        }

        template <class T>
        void UIElement<T>::remove()
        {
            if( auto parent = getParent() )
            {
                parent->removeChild( this );
            }
        }

        template <class T>
        void UIElement<T>::removeAllChildren()
        {
            auto children = m_children.snapshot();
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
        void UIElement<T>::destroyAllChildren()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto ui = applicationManager->getUI();

            for( auto child : m_children )
            {
                if( child )
                {
                    child->setParent( nullptr );

                    ui->removeElement( child );
                }
            }

            m_children.clear();
        }

        template <class T>
        bool UIElement<T>::hasChildById( const String &id ) const
        {
            return findChildById( id ) != nullptr;
        }

        template <class T>
        SmartPtr<IUIElement> UIElement<T>::findChildById( const String &id ) const
        {
            auto children = m_children.snapshot();
            for( auto &child : children )
            {
                if( child )
                {
                    // Check direct child
                    if( child->getLabel() == id )
                    {
                        return child;
                    }

                    // Check descendants recursively
                    if( auto found = child->findChildById( id ) )
                    {
                        return found;
                    }
                }
            }

            return nullptr;
        }

        template <class T>
        s32 UIElement<T>::getSiblingIndex() const
        {
            if( auto parent = getParent() )
            {
                auto children = parent->getChildren();
                for( size_t i = 0; i < children.size(); ++i )
                {
                    if( children[i].get() == this )
                    {
                        return static_cast<s32>( i );
                    }
                }
            }

            return -1;
        }

        template <class T>
        SmartPtr<IUIElement> UIElement<T>::getLayout() const
        {
            auto p = m_layout.load();
            return p.lock();
        }

        template <class T>
        void UIElement<T>::setLayout( SmartPtr<IUIElement> layout )
        {
            m_layout = layout;
        }

        template <class T>
        void UIElement<T>::setUserData( void *userData )
        {
            m_userData = userData;
        }

        template <class T>
        SmartPtr<IEventListener> UIElement<T>::findObjectListener( const String &id ) const
        {
            auto listeners = T::getObjectListeners();
            for( auto &listener : listeners )
            {
                if( listener->getName() == id )
                {
                    return listener;
                }
            }

            for( auto &child : m_children )
            {
                if( child )
                {
                    auto listener = child->findObjectListener( id );
                    if( listener )
                    {
                        return listener;
                    }
                }
            }

            return nullptr;
        }

        template <class T>
        void UIElement<T>::addAnimator( SmartPtr<IAnimator> &animator )
        {
            // Base implementation is no-op
        }

        template <class T>
        bool UIElement<T>::removeAnimator( SmartPtr<IAnimator> &animator )
        {
            // Base implementation returns false
            return false;
        }

        template <class T>
        SmartPtr<IUILayoutContainer> UIElement<T>::getContainer() const
        {
            return m_container;
        }

        template <class T>
        void UIElement<T>::setContainer( SmartPtr<IUILayoutContainer> container )
        {
            m_container = container;
        }

        template <class T>
        SmartPtr<ISharedObject> UIElement<T>::getOwner() const
        {
            return m_owner;
        }

        template <class T>
        void UIElement<T>::setOwner( SmartPtr<ISharedObject> owner )
        {
            m_owner = owner;
        }

        template <class T>
        void UIElement<T>::_getObject( void **ppObject ) const
        {
            *ppObject = nullptr;
        }

        template <class T>
        SmartPtr<Properties> UIElement<T>::getProperties() const
        {
            return m_properties;
        }

        template <class T>
        void UIElement<T>::setProperties( SmartPtr<Properties> properties )
        {
            m_properties = properties;
        }

        template <class T>
        Array<SmartPtr<ISharedObject>> UIElement<T>::getChildObjects() const
        {
            Array<SmartPtr<ISharedObject>> childObjects;

            auto children = m_children.snapshot();
            for( auto &child : children )
            {
                if( child )
                {
                    childObjects.push_back( child );
                }
            }

            return childObjects;
        }

        template <class T>
        bool UIElement<T>::isDragDropSource() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue( state->flags,
                                                  static_cast<u16>( IUIElement::dragDropSourceFlag ) );
                }
            }

            return false;
        }

        template <class T>
        void UIElement<T>::setDragDropSource( bool dragDropSource )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::dragDropSourceFlag ),
                        dragDropSource );
                }
            }
        }

        template <class T>
        SmartPtr<IUIDragSource> UIElement<T>::getDragSource() const
        {
            return m_dragSource;
        }

        template <class T>
        void UIElement<T>::setDragSource( SmartPtr<IUIDragSource> dragSource )
        {
            m_dragSource = dragSource;
        }

        template <class T>
        SmartPtr<IUIDropTarget> UIElement<T>::getDropTarget() const
        {
            return m_dropTarget;
        }

        template <class T>
        void UIElement<T>::setDropTarget( SmartPtr<IUIDropTarget> dropTarget )
        {
            m_dropTarget = dropTarget;
        }

        template <class T>
        u32 UIElement<T>::getOrder() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UITransformStateData>() )
                {
                    return state->zorder;
                }
            }

            return 0;
        }

        template <class T>
        void UIElement<T>::setOrder( u32 order )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UITransformStateData>() )
                {
                    state->zorder = order;
                }
            }
        }

        template <class T>
        bool UIElement<T>::getRenderChildren() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue( state->flags,
                                                  static_cast<u16>( IUIElement::renderChildrenFlag ) );
                }
            }

            return false;
        }

        template <class T>
        void UIElement<T>::setRenderChildren( bool renderChildren )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::renderChildrenFlag ),
                        renderChildren );
                }
            }
        }

        template <class T>
        void UIElement<T>::onAddChild( IUIElement *child )
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onRemoveChild( IUIElement *child )
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onChangedState()
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onChildChangedState( IUIElement *child )
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onToggleEnabled()
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onToggleVisibility()
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onToggleHighlight()
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onActivate( SmartPtr<IUIElement> element )
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onDeactivate()
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onSelect()
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onDeselect()
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onGainFocus()
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::onLostFocus()
        {
            // Base implementation - can be overridden
        }

        template <class T>
        void UIElement<T>::handleEvent( const SmartPtr<IEvent> &event )
        {
            // Base implementation - can be overridden
        }

        template <class T>
        bool UIElement<T>::getSameLine() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    return BitUtil::getFlagValue( state->flags,
                                                  static_cast<u16>( IUIElement::sameLineFlag ) );
                }
            }

            return false;
        }

        template <class T>
        void UIElement<T>::setSameLine( bool sameLine )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, static_cast<u16>( IUIElement::sameLineFlag ), sameLine );
                }
            }
        }

        template <class T>
        ColourF UIElement<T>::getColour() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template getStateData<UIElementStateData>() )
                {
                    return state->colour;
                }
            }

            return {};
        }

        template <class T>
        void UIElement<T>::setColour( const ColourF &colour )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<UIElementStateData>() )
                {
                    state->colour = colour;
                }
            }
        }

        template <class T>
        SmartPtr<IStateContext> UIElement<T>::getStateContext() const
        {
            return m_stateContext;
        }

        template <class T>
        void UIElement<T>::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        template <class T>
        SmartPtr<IStateListener> UIElement<T>::getStateListener() const
        {
            return m_stateListener;
        }

        template <class T>
        void UIElement<T>::setStateListener( SmartPtr<IStateListener> stateListener )
        {
            m_stateListener = stateListener;
        }

        template <class T>
        void UIElement<T>::sortZOrder()
        {
            std::sort( m_children.begin(), m_children.end(),
                       []( auto a, auto b ) { return a->getOrder() < b->getOrder(); } );

            // Recursively sort children
            for( auto child : m_children )
            {
                if( child )
                {
                    child->sortZOrder();
                }
            }
        }

        template <class T>
        void UIElement<T>::updateZOrder()
        {
            for( auto child : m_children )
            {
                if( child )
                {
                    child->updateZOrder();
                }
            }
        }

        template <class T>
        bool UIElement<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            // Base implementation returns true
            return true;
        }

        template <class T>
        void UIElement<T>::setupMaterial( SmartPtr<render::IMaterial> material )
        {
            // Base implementation - can be overridden
        }
    }  // namespace ui
}  // namespace workphone

#endif  // UIElement_h__
