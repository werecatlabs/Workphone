#ifndef UIText_h__
#define UIText_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @brief Concrete UI text element.
         *
         * Implements the IUIText interface and provides basic management for
         * a text element displayed in the UI. Handles text content, text size,
         * horizontal and vertical alignment, and integrates with the component
         * lifecycle (properties, unload, child objects).
         *
         * @note This class is a UI element and participates in the engine's
         * component and property systems. See IUIText and UIElement for related
         * behaviour and lifecycle contracts.
         */
        class WPCore_API UIText : public UIElement<IUIText>
        {
        public:
            /**
             * @brief Construct a new UIText instance.
             *
             * Initializes default text element state. Construction does not
             * necessarily create rendering resources — those are created by the
             * UI backend when the element is loaded/attached.
             */
            UIText();

            /**
             * @brief Destroy the UIText instance.
             *
             * Releases any owned resources. Use the component lifecycle methods
             * (unload) to explicitly release runtime resources before destruction
             * when required by the engine.
             */
            ~UIText() override;

            /**
             * @brief Set the text content for this UI element.
             *
             * Updates the displayed text. Calling this will typically mark the
             * element as needing a layout and redraw by the UI system.
             *
             * @param text The new text content to display.
             */
            void setText( const String &text ) override;

            /**
             * @brief Get the current text content.
             *
             * @return String The text currently stored for this element.
             */
            String getText() const override;

            /**
             * @brief Set the text size used to render this element.
             *
             * The unit and interpretation of @p textSize is determined by the
             * UI renderer (e.g., points, pixels, or scaled units). Changing the
             * text size may affect layout and should trigger a relayout.
             *
             * @param textSize The new text size.
             */
            void setTextSize( f32 textSize ) override;

            /**
             * @brief Get the text size currently used for rendering.
             *
             * @return f32 The configured text size.
             */
            f32 getTextSize() const override;

            /**
             * @brief Set the vertical alignment for the text within its bounds.
             *
             * The @p alignment parameter is an alignment value defined by the
             * UI system / IUIText interface (for example top, centre, bottom).
             *
             * @param alignment Alignment value (see IUIText for constants).
             */
            void setVerticalAlignment( u8 alignment ) override;

            /**
             * @brief Get the vertical alignment currently used.
             *
             * @return u8 The vertical alignment value.
             */
            u8 getVerticalAlignment() const override;

            /**
             * @brief Set the horizontal alignment for the text within its bounds.
             *
             * The @p alignment parameter is an alignment value defined by the
             * UI system / IUIText interface (for example left, centre, right).
             *
             * @param alignment Alignment value (see IUIText for constants).
             */
            void setHorizontalAlignment( u8 alignment ) override;

            /**
             * @brief Get the horizontal alignment currently used.
             *
             * @return u8 The horizontal alignment value.
             */
            u8 getHorizontalAlignment() const override;

            /**
             * @brief Unload the component and release runtime resources.
             *
             * Called when the component should release any runtime or renderer
             * specific resources (textures, font handles, GPU resources, etc).
             * Implementations should be safe to call multiple times.
             *
             * @param data Optional data that may be used by the unload routine.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Retrieve the component properties as a Properties object.
             *
             * Used by serialization, editors, and runtime property inspection.
             * The returned Properties object contains the serializable state of
             * this UIText instance (text, size, alignment, etc).
             *
             * @return SmartPtr<Properties> A new or existing properties object
             * containing this component's state.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply properties from a Properties object to this component.
             *
             * Used during deserialization or when applying settings from an
             * editor. Implementations should validate property values and
             * update internal state as needed.
             *
             * @param properties Properties container with values to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get this element's child objects.
             *
             * Returns any child objects owned or exposed by this component.
             * For a simple text element this is typically an empty array, but
             * implementations may return auxiliary objects (e.g., text effects).
             *
             * @return Array<SmartPtr<ISharedObject>> Array of child shared objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Called when the element's state has changed.
             *
             * Override to react to state changes in the UIElement base (for
             * example enabled/disabled/visibility changes). Implementations
             * should update internal state and mark the element for redraw or
             * layout as appropriate.
             */
            void onChangedState() override;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIText_h__
