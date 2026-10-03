#ifndef __LayoutTransform_h__
#define __LayoutTransform_h__

#include <Workphone/Scene/Components/Component.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class LayoutTransform
         * @brief UI component for managing layout and transform properties.
         *
         * This component handles the position, size, anchor, alignment, and z-ordering of UI elements.
         * It provides methods to manipulate and query layout-related properties, and supports automatic
         * order calculation and cascading updates. Typically used as part of a UI system to control
         * how elements are arranged and rendered.
         */
        class WPCore_API LayoutTransform : public Component
        {
        public:
            /** @name Static String Identifiers */
            ///@{
            ///< Identifier for horizontal alignment property.
            static const String horizontalStr;

            ///< Identifier for vertical alignment property.
            static const String verticalStr;

            ///< Identifier for z-order property.
            static const String orderStr;

            ///< Identifier for auto-calculate order property.
            static const String autoCalculateOrderStr;
            ///@}

            /** @brief Flag value for auto-calculate order. */
            static const u8 autoCalculateOrderFlag;

            /** @brief Default constructor. */
            LayoutTransform();

            /** @brief Destructor. */
            ~LayoutTransform() override;

            /**
             * @brief Loads the component with the given data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the component and releases resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the transform of the UI component.
             *
             * Recalculates the position, size, and other transform-related properties.
             */
            void updateTransform() override;

            /**
             * @brief Gets the minimum position of the UI element relative to its parent.
             * @return Minimum position as a 2D vector.
             */
            Vector2<real_Num> getMin() const;

            /**
             * @brief Gets the absolute minimum position of the UI element in screen space.
             * @return Absolute minimum position as a 2D vector.
             */
            Vector2<real_Num> getAbsoluteMin() const;

            /**
             * @brief Gets the maximum position of the UI element relative to its parent.
             * @return Maximum position as a 2D vector.
             */
            Vector2<real_Num> getMax() const;

            /**
             * @brief Gets the absolute maximum position of the UI element in screen space.
             * @return Absolute maximum position as a 2D vector.
             */
            Vector2<real_Num> getAbsoluteMax() const;

            /**
             * @brief Gets the local position of the UI element.
             * @return Position as a 2D vector.
             */
            Vector2<real_Num> getPosition() const;

            /**
             * @brief Sets the local position of the UI element.
             * @param position New position as a 2D vector.
             */
            void setPosition( const Vector2<real_Num> &position );

            /**
             * @brief Gets the size of the UI element.
             * @return Size as a 2D vector.
             */
            Vector2<real_Num> getSize() const;

            /**
             * @brief Sets the size of the UI element.
             * @param size New size as a 2D vector.
             */
            void setSize( const Vector2<real_Num> &size );

            /**
             * @brief Gets the anchor point of the UI element.
             * @return Anchor as a 2D vector (normalized, typically [0,1]).
             */
            Vector2<real_Num> getAnchor() const;

            /**
             * @brief Sets the anchor point of the UI element.
             * @param anchor New anchor as a 2D vector.
             */
            void setAnchor( const Vector2<real_Num> &anchor );

            /**
             * @brief Gets the minimum anchor value.
             * @return Minimum anchor as a 2D vector.
             */
            Vector2<real_Num> getAnchorMin() const;

            /**
             * @brief Sets the minimum anchor value.
             * @param anchorMin New minimum anchor as a 2D vector.
             */
            void setAnchorMin( const Vector2<real_Num> &anchorMin );

            /**
             * @brief Gets the maximum anchor value.
             * @return Maximum anchor as a 2D vector.
             */
            Vector2<real_Num> getAnchorMax() const;

            /**
             * @brief Sets the maximum anchor value.
             * @param anchorMax New maximum anchor as a 2D vector.
             */
            void setAnchorMax( const Vector2<real_Num> &anchorMax );

            /**
             * @brief Updates the anchor values based on the current alignment settings.
             */
            void updateAnchorFromAlignment();

            /**
             * @brief Updates the z-order of the UI element.
             *
             * Recalculates the rendering order of the element and its children if necessary.
             */
            void updateOrder() override;

            /**
             * @brief Gets the z-order of the UI component.
             * @return The z-order value.
             */
            u32 getZOrder() const;

            /**
             * @brief Sets the z-order of the UI component.
             * @param zOrder The new z-order value.
             * @param cascade If true, applies the change to child elements as well.
             */
            void setZOrder( u32 zOrder, bool cascade = false );

            /**
             * @brief Checks if the UI component is set to automatically calculate its order.
             * @return True if auto-calculate order is enabled, false otherwise.
             */
            bool getAutoCalculateOrder() const;

            /**
             * @brief Sets whether the UI component should automatically calculate its order.
             * @param autoCalculateOrder True to enable, false to disable.
             * @param cascade If true, applies the change to child elements as well.
             */
            void setAutoCalculateOrder( bool autoCalculateOrder, bool cascade = true );

            /**
             * @brief Gets the properties of the component.
             * @return Smart pointer to the properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the component.
             * @param properties Smart pointer to the properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Sets the horizontal alignment of the UI element.
             * @param gha The horizontal alignment value.
             */
            void setHorizontalAlignment( HorizontalAlignment gha );

            /**
             * @brief Gets the horizontal alignment of the UI element.
             * @return The horizontal alignment value.
             */
            HorizontalAlignment getHorizontalAlignment() const;

            /**
             * @brief Sets the vertical alignment of the UI element.
             * @param gva The vertical alignment value.
             */
            void setVerticalAlignment( VerticalAlignment gva );

            /**
             * @brief Gets the vertical alignment of the UI element.
             * @return The vertical alignment value.
             */
            VerticalAlignment getVerticalAlignment() const;

            /**
             * @brief Gets the absolute position of the UI element in screen space.
             * @return Absolute position as a 2D vector.
             */
            Vector2<real_Num> getAbsolutePosition() const;

            /**
             * @brief Sets the absolute position of the UI element in screen space.
             * @param absolutePosition New absolute position as a 2D vector.
             */
            void setAbsolutePosition( const Vector2<real_Num> &absolutePosition );

            /**
             * @brief Gets the absolute size of the UI element in screen space.
             * @return Absolute size as a 2D vector.
             */
            Vector2<real_Num> getAbsoluteSize() const;

            /**
             * @brief Sets the absolute size of the UI element in screen space.
             * @param absoluteSize New absolute size as a 2D vector.
             */
            void setAbsoluteSize( const Vector2<real_Num> &absoluteSize );

            /**
             * @brief Gets the relative position of the UI element within its parent.
             * @return Relative position as a 2D vector.
             */
            Vector2<real_Num> getRelativePosition() const;

            /**
             * @brief Gets the relative size of the UI element within its parent.
             * @return Relative size as a 2D vector.
             */
            Vector2<real_Num> getRelativeSize() const;

            /**
             * @brief Gets a raw pointer to the layout UI component.
             * @return Pointer to the UIComponent representing the layout.
             */
            UIComponent *getLayoutPtr() const;

            /**
             * @brief Gets a smart pointer to the layout UI component.
             * @return Smart pointer to the UIComponent representing the layout.
             */
            SmartPtr<UIComponent> getLayout() const;

            /**
             * @brief Sets the layout UI component.
             * @param layout Smart pointer to the UIComponent to set as layout.
             */
            void setLayout( SmartPtr<UIComponent> layout );

            /**
             * @brief Gets a smart pointer to the associated UI component.
             * @return Smart pointer to the UIComponent.
             */
            SmartPtr<UIComponent> getUIComponent() const;

            /**
             * @brief Sets the associated UI component.
             * @param uiComponent Smart pointer to the UIComponent to associate.
             */
            void setUIComponent( SmartPtr<UIComponent> uiComponent );

            /** @brief Registers the class for reflection or serialization. */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Updates the layout transforms for this component and its children.
             */
            void updateLayoutTransforms();

            /**
             * @brief Handles component-specific events.
             * @param state The current state.
             * @param eventType The type of event.
             * @return The result of the event handling.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Handles generic events for the component.
             * @param eventType The type of event.
             * @param eventValue The event value.
             * @param arguments Array of event parameters.
             * @param sender The sender object.
             * @param object The target object.
             * @param event The event instance.
             * @return The result parameter.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Sets up the canvas for the UIComponent.
             *
             * This method is responsible for initializing the canvas for the UIComponent.
             * It should be implemented by derived classes if additional setup is needed.
             */
            virtual void setupCanvas();

            /** @brief The z-order of the UI component. */
            u32 m_zOrder = 0;

            /** @brief Smart pointer to the associated UI component. */
            mutable AtomicSmartPtr<UIComponent> m_uiComponent;

            /**
             * @brief Smart pointer to the UIComponent object representing the layout canvas.
             */
            SmartPtr<UIComponent> m_layout;

            /**
             * @brief Array of pointers to child layout transforms.
             */
            Array<LayoutTransform *> m_layoutTransforms;

            /** @brief Flags for the UI component (bitmask). */
            u8 m_uiComponentFlags = 0;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // __LayoutTransform_h__
