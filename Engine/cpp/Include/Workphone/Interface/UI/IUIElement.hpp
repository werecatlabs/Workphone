#ifndef __IUIElement_h__
#define __IUIElement_h__

#include <Workphone/Interface/IPrototype.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIElement
         * @brief Abstract interface representing a UI element.
         *
         * IUIElement defines the common behaviour and properties required by all
         * user-interface elements in the engine (position, size, visibility,
         * parent/child relationships, input handling, state/context, etc.).
         *
         * Implementations are responsible for rendering, input routing and
         * maintaining children. The interface is kept lightweight and engine-
         * agnostic: platform or renderer-specific objects may be attached and
         * retrieved via the `_getObject` method.
         *
         * Thread-safety:
         * - Read-only operations may be called from multiple threads depending
         *   on concrete implementation. Mutating operations (add/remove children,
         *   set properties) generally must be performed from the UI/main thread
         *   unless the concrete implementation documents otherwise.
         *
         * @note Methods that accept a `cascade` parameter will, when `true`,
         * propagate the change to child elements.
         *
         * @see IUIContainer, IUIElement (implementations)
         */
        class WPCore_API IUIElement : public core::IPrototype
        {
        public:
            /// @brief State message hash for adding a child to the element.
            static const hash_type STATE_MESSAGE_ADD_CHILD;

            /// @brief State message hash for removing a child from the element.
            static const hash_type STATE_MESSAGE_REMOVE_CHILD;

            /// @brief Flag indicating element should be laid out on the same line as previous.
            static const u32 sameLineFlag;

            /// @brief Flag indicating children should be rendered by this element.
            static const u32 renderChildrenFlag;

            /// @brief Flag indicating the element is enabled (can interact).
            static const u32 enabledFlag;

            /// @brief Flag indicating the element is visible.
            static const u32 visibleFlag;

            /// @brief Flag indicating the element is selected.
            static const u32 selectedFlag;

            /// @brief Flag indicating the element is currently hovered by the cursor.
            static const u32 hoveredFlag;

            /// @brief Flag indicating the element or one of its children has input focus.
            static const u32 focusedFlag;

            /// @brief Flag indicating the element is highlighted (visual emphasis).
            static const u32 highlightedFlag;

            /// @brief Flag indicating this element can be a drag source.
            static const u32 dragDropSourceFlag;

            /// @brief Flag indicating the element should handle input events.
            static const u32 handleInputEventsFlag;

            /// @brief Internal flag used for element visibility state.
            static const u32 elementVisibleFlag;

            /** @brief Property key string constants */
            static const String widgetPosStr;
            static const String widgetSizeStr;
            static const String sizeStr;
            static const String positionStr;
            static const String enabledStr;
            static const String visibleStr;
            static const String colourStr;
            static const String sameLineStr;
            static const String orderStr;
            static const String makeDirtyStr;

            IUIElement();
            IUIElement( u32 poolTypeId );

            /**
             * @brief Virtual destructor.
             *
             * Ensure derived destructors are called correctly when deleting via
             * a pointer to IUIElement.
             */
            ~IUIElement() override;

            /**
             * @brief Handle an input event.
             *
             * Concrete UI elements should process the provided input event and
             * perform actions as appropriate (state changes, activation,
             * selection, etc.).
             *
             * @param event Smart pointer to an input event instance.
             * @return true if the event was handled and should not be propagated
             *         further; false otherwise.
             */
            virtual bool handleEvent( const SmartPtr<IInputEvent> &event ) = 0;

            /**
             * @brief Get the element's hashed identifier.
             * @return A hash_type representing the element ID.
             */
            virtual hash_type getElementId() const = 0;

            /**
             * @brief Set the element's hashed identifier.
             * @param elementId Hash value to use as this element's identifier.
             */
            virtual void setElementId( hash_type elementId ) = 0;

            /**
             * @brief Get a human readable label for the element.
             * @return The element label string.
             */
            virtual String getLabel() const = 0;

            /**
             * @brief Set a human readable label for the element.
             * @param label New label string.
             */
            virtual void setLabel( const String &label ) = 0;

            /**
             * @brief Set the element's local position.
             *
             * Position is normally in the UI's 2D coordinate space (pixels or
             * normalized units depending on the implementation).
             *
             * @param position New local 2D position.
             */
            virtual void setPosition( const Vector2<real_Num> &position ) = 0;

            /**
             * @brief Get the element's local position.
             * @return Current local 2D position.
             */
            virtual Vector2<real_Num> getPosition() const = 0;

            /**
             * @brief Get the element's absolute position in screen / root coordinates.
             * @return Absolute 2D position of the element.
             */
            virtual Vector2<real_Num> getAbsolutePosition() const = 0;

            /**
             * @brief Set the element's size.
             * @param size New 2D size for the element.
             */
            virtual void setSize( const Vector2<real_Num> &size ) = 0;

            /**
             * @brief Get the element's size.
             * @return Current 2D size of the element.
             */
            virtual Vector2<real_Num> getSize() const = 0;

            /**
             * @brief Get the element's uniform scale.
             * @return Scaling factor (1.0 = no scale).
             */
            virtual f32 getScale() const = 0;

            /**
             * @brief Set the element's uniform scale.
             * @param scale Uniform scaling factor.
             */
            virtual void setScale( f32 scale ) = 0;

            /**
             * @brief Give or remove input focus from this element.
             *
             * When an element receives focus it typically becomes the recipient
             * of keyboard events and may change visual state.
             *
             * @param hasFocus true to give focus, false to remove.
             */
            virtual void setFocus( bool hasFocus ) = 0;

            /**
             * @brief Query whether the element or any of its children currently has focus.
             * @return true if this element or one of its descendants has focus.
             */
            virtual bool isInFocus() const = 0;

            /**
             * @brief Set the element's highlighted (emphasised) visual state.
             *
             * @param isHighlighted true to highlight the element, false to remove highlight.
             * @param cascade If true the highlight change will propagate to children.
             */
            virtual void setHighlighted( bool isHighlighted, bool cascade = true ) = 0;

            /**
             * @brief Query whether the element is highlighted.
             * @return true if element is highlighted.
             */
            virtual bool isHighlighted() const = 0;

            /**
             * @brief Set the element's visibility.
             *
             * If `cascade` is true the visibility change will be propagated to child elements.
             *
             * @param visible New visibility state.
             * @param cascade Whether to apply the change to children (default: true).
             */
            virtual void setVisible( bool visible, bool cascade = true ) = 0;

            /**
             * @brief Test whether the element is visible.
             * @return true if the element is visible.
             */
            virtual bool isVisible() const = 0;

            /**
             * @brief Enable or disable the element.
             *
             * Disabled elements typically do not respond to input events.
             *
             * @param enabled New enabled state.
             * @param cascade If true apply the change to children as well (default: true).
             */
            virtual void setEnabled( bool enabled, bool cascade = true ) = 0;

            /**
             * @brief Query whether the element is enabled.
             * @return true if enabled.
             */
            virtual bool isEnabled() const = 0;

            /**
             * @brief Mark the element as hovered.
             * @param hovered true if the cursor is over the element.
             */
            virtual void setHovered( bool hovered ) = 0;

            /**
             * @brief Query whether the element is currently hovered.
             * @return true if hovered by the cursor.
             */
            virtual bool isHovered() const = 0;

            /**
             * @brief Get this element's parent in the UI tree.
             * @return Smart pointer to the parent element, or null if root.
             */
            virtual SmartPtr<IUIElement> getParent() const = 0;

            /**
             * @brief Set the parent of this element.
             * @param parent Smart pointer to the new parent element (may be null).
             */
            virtual void setParent( SmartPtr<IUIElement> parent ) = 0;

            virtual u32 getNumChildren() const = 0;

            /**
             * @brief Retrieve the children container.
             * @return Array containing children.
             */
            virtual Array<SmartPtr<IUIElement>> getChildren() const = 0;

            /**
             * @brief Add a child element.
             * @param child Smart pointer to the child to add.
             *
             * Implementations should update parent/child relationships and
             * send corresponding state messages (STATE_MESSAGE_ADD_CHILD).
             */
            virtual void addChild( SmartPtr<IUIElement> child ) = 0;

            /**
             * @brief Remove a child element.
             * @param child Smart pointer to the child to remove.
             * @return true if the child was found and removed, false otherwise.
             */
            virtual bool removeChild( SmartPtr<IUIElement> child ) = 0;

            /**
             * @brief Remove this element from its parent.
             *
             * After calling `remove()` the element should no longer be part of
             * the parent's child list. The lifetime semantics depend on the
             * implementation (may delete the element or leave ownership to caller).
             */
            virtual void remove() = 0;

            /**
             * @brief Remove all children from this element without destroying them.
             *
             * The children are detached from this element; ownership/lifetime is
             * implementation-defined.
             */
            virtual void removeAllChildren() = 0;

            /**
             * @brief Destroy (delete) all child elements.
             *
             * This function will attempt to destroy and free resources for all children.
             */
            virtual void destroyAllChildren() = 0;

            /**
             * @brief Check whether a child with the given identifier exists.
             * @param id Identifier (string) of the child.
             * @return true if a child with the id exists; false otherwise.
             */
            virtual bool hasChildById( const String &id ) const = 0;

            /**
             * @brief Find a direct child by string identifier.
             * @param id Identifier of the child to find.
             * @return Smart pointer to the child if found, otherwise null.
             */
            virtual SmartPtr<IUIElement> findChildById( const String &id ) const = 0;

            /**
             * @brief Get the index of this element among its siblings.
             * @return Zero-based index, or -1 if the parent is null or not found.
             */
            virtual s32 getSiblingIndex() const = 0;

            /**
             * @brief Get the zero-based index among siblings of the same type T.
             * @tparam T Type to match when counting siblings.
             * @return The index among siblings of type T, or -1 if parent is null or not found.
             */
            template <class T>
            s32 getSiblingIndexByType() const;

            /**
             * @brief Count siblings of the same type T.
             * @tparam T Type to match when counting siblings.
             * @return Count of siblings of type T, or -1 if parent is null.
             */
            template <class T>
            s32 getSiblingCountByType() const;

            /**
             * @brief Get the layout element this belongs to (if any).
             * @return Smart pointer to the layout element.
             */
            virtual SmartPtr<IUIElement> getLayout() const = 0;

            /**
             * @brief Set the layout element that this element belongs to.
             * @param layout Smart pointer to the layout element.
             */
            virtual void setLayout( SmartPtr<IUIElement> layout ) = 0;

            /**
             * @brief Get the container element (logical grouping / container).
             * @return Smart pointer to the container, or null if none.
             */
            virtual SmartPtr<IUILayoutContainer> getContainer() const = 0;

            /**
             * @brief Set the container for this element.
             * @param container Smart pointer to the container.
             */
            virtual void setContainer( SmartPtr<IUILayoutContainer> container ) = 0;

            /**
             * @brief Get the owner of this element.
             * @return Smart pointer to an arbitrary shared object that owns or
             *         is associated with the element.
             */
            virtual SmartPtr<ISharedObject> getOwner() const = 0;

            /**
             * @brief Set the owner associated with this element.
             * @param owner Smart pointer to the owner object.
             */
            virtual void setOwner( SmartPtr<ISharedObject> owner ) = 0;

            /**
             * @brief Get the drag source.
             * @return Smart pointer representing the drag source.
             */
            virtual SmartPtr<IUIDragSource> getDragSource() const = 0;

            /**
             * @brief Set the drag source handler for this element.
             * @param dragSource Smart pointer to the drag source interface.
             */
            virtual void setDragSource( SmartPtr<IUIDragSource> dragSource ) = 0;

            /**
             * @brief Get the drop target.
             * @return Smart pointer representing the drop target.
             */
            virtual SmartPtr<IUIDropTarget> getDropTarget() const = 0;

            /**
             * @brief Set the drop target handler for this element.
             * @param dropTarget Smart pointer to the drop target interface.
             */
            virtual void setDropTarget( SmartPtr<IUIDropTarget> dropTarget ) = 0;

            /**
             * @brief Get the element's order (z-order / layout ordering).
             * @return Order as an unsigned integer; interpretation is implementation-defined.
             */
            virtual u32 getOrder() const = 0;

            /**
             * @brief Set the element's order (z-order / layout ordering).
             * @param order New order value.
             */
            virtual void setOrder( u32 order ) = 0;

            /**
             * @brief Query whether this element should be positioned on the same line
             * as the previous element (layout hint).
             * @return true if same-line layout is requested.
             */
            virtual bool getSameLine() const = 0;

            /**
             * @brief Set the same-line layout hint for the element.
             * @param sameLine true to request same-line layout.
             */
            virtual void setSameLine( bool sameLine ) = 0;

            /**
             * @brief Get the element's color (used for rendering).
             * @return ColourF representing the element colour and alpha.
             */
            virtual ColourF getColour() const = 0;

            /**
             * @brief Set the element's colour.
             * @param colour ColourF containing RGBA values to apply.
             */
            virtual void setColour( const ColourF &colour ) = 0;

            /**
             * @brief Query whether this element processes input events.
             * @return true if the element will handle input events.
             */
            virtual bool getHandleInputEvents() const = 0;

            /**
             * @brief Toggle whether this element handles input events.
             * @param handleInputEvents true to enable event handling.
             */
            virtual void setHandleInputEvents( bool handleInputEvents ) = 0;

            /**
             * @brief Sort children by z-order recursively.
             *
             * Implementations should re-order the child list according to their
             * order/z-index so that rendering and input hit-testing occurs in the
             * correct sequence.
             */
            virtual void sortZOrder() = 0;

            /**
             * @brief Recompute/update this element's Z-order.
             *
             * This may propagate to parent/layout to maintain correct ordering.
             */
            virtual void updateZOrder() = 0;

            /**
             * @brief Mark the element as dirty / invalid so it will be redrawn.
             *
             * Implementations should schedule a redraw or otherwise mark render
             * resources as needing update.
             */
            virtual void invalidate() = 0;

            /**
             * @brief Get the state/context object used by this UI element.
             * @return Smart pointer to the element's IStateContext.
             */
            virtual SmartPtr<IStateContext> getStateContext() const = 0;

            /**
             * @brief Set a new state/context object for this element.
             * @param stateContext Smart pointer to the IStateContext to use.
             */
            virtual void setStateContext( SmartPtr<IStateContext> stateContext ) = 0;

            /**
             * @brief Get the state listener associated with this element.
             * @return Smart pointer to the IStateListener instance.
             */
            virtual SmartPtr<IStateListener> getStateListener() const = 0;

            /**
             * @brief Set the state listener for this element.
             * @param stateListener Smart pointer to the IStateListener to attach.
             */
            virtual void setStateListener( SmartPtr<IStateListener> stateListener ) = 0;

            /**
             * @brief Query whether this element renders its children.
             * @return true if children are rendered by this element.
             */
            virtual bool getRenderChildren() const = 0;

            /**
             * @brief Enable or disable rendering of this element's children.
             * @param renderChildren true to render children.
             */
            virtual void setRenderChildren( bool renderChildren ) = 0;

            /**
             * @brief Callback invoked to activate an element.
             * @param element Smart pointer to the activated element (may be this).
             */
            virtual void onActivate( SmartPtr<IUIElement> element ) = 0;

            /** @brief Callback invoked to deactivate the element. */
            virtual void onDeactivate() = 0;

            /** @brief Callback invoked when the element becomes selected. */
            virtual void onSelect() = 0;

            /** @brief Callback invoked when the element is deselected. */
            virtual void onDeselect() = 0;

            /** @brief Callback invoked when the element gains focus. */
            virtual void onGainFocus() = 0;

            /** @brief Callback invoked when the element loses focus. */
            virtual void onLostFocus() = 0;

            /**
             * @brief Retrieve a pointer to the underlying renderer/engine object.
             *
             * Concrete implementations may expose engine-specific handle or
             * object (for example a scene node, widget handle, etc.) through
             * this API.
             *
             * @param ppObject Out parameter set to the implementation object pointer.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            /**
             * @brief Collect direct children of type T.
             * @tparam T Desired child type.
             * @return Array of smart pointers to children of type T.
             *
             * This helper iterates over direct children and returns those whose
             * dynamic type derives from T.
             */
            template <class T>
            Array<SmartPtr<T>> getChildrenByType() const;

            WP_CLASS_REGISTER_DECL;
        };

        template <class T>
        Array<SmartPtr<T>> IUIElement::getChildrenByType() const
        {
            auto children = getChildren();

            Array<SmartPtr<T>> childrenByType;
            childrenByType.reserve( children.size() );

            for( auto &child : children )
            {
                if( child )
                {
                    if( child->isDerived<T>() )
                    {
                        childrenByType.push_back( child );
                    }
                }
            }

            return childrenByType;
        }

        template <class T>
        s32 IUIElement::getSiblingIndexByType() const
        {
            auto pThis = getSharedFromThis<IUIElement>();
            auto siblingParent = getParent();
            if( siblingParent )
            {
                auto siblingCount = 0;
                auto children = siblingParent->getChildren();
                for( u32 i = 0; i < children.size(); ++i )
                {
                    auto child = children.at( i );
                    if( child )
                    {
                        if( child->isDerived<T>() )
                        {
                            if( child == pThis )
                            {
                                return siblingCount;
                            }

                            ++siblingCount;
                        }
                    }
                }
            }

            return -1;
        }

        template <class T>
        s32 IUIElement::getSiblingCountByType() const
        {
            auto pThis = getSharedFromThis<IUIElement>();
            auto siblingParent = getParent();
            if( siblingParent )
            {
                auto children = siblingParent->getChildren();

                auto siblingCount = 0;
                for( u32 i = 0; i < children.size(); ++i )
                {
                    auto child = children.at( i );
                    if( child )
                    {
                        if( child->isDerived<T>() )
                        {
                            ++siblingCount;
                        }
                    }
                }

                return siblingCount;
            }

            return -1;
        }

    }  // end namespace ui
}  // namespace workphone

#endif  // IUIElement_h__
