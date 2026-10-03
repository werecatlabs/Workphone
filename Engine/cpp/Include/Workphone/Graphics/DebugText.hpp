#ifndef __Render_DebugText_h__
#define __Render_DebugText_h__

#include <Workphone/Interface/Graphics/IDebugText.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Renders a single overlay text element used for debug information.
         *
         * This class is a concrete implementation of `IDebugText`. It owns a text
         * string and a reference to an overlay text element used by the renderer.
         *
         * Typical usage:
         * - Construct the object.
         * - Call `load` with an appropriate shared-data object when attaching to
         *   a graphics system.
         * - Use `setText` to update the displayed string and `getText` to read it.
         */
        class WPCore_API DebugText : public IDebugText
        {
        public:
            /**
             * @brief Construct a DebugText instance.
             *
             * Initializes internal state. The overlay element is initially null;
             * call `setTextElement` or `load` to attach a renderable element.
             */
            DebugText();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived cleanup runs and that resources referenced by the
             * overlay element are released when the object is destroyed.
             */
            ~DebugText() override;

            /**
             * @brief Load resources or state required by this object.
             *
             * @param data Shared object containing initialization data or dependencies.
             *             The actual accepted type depends on the renderer integration.
             *
             * This method is intended to be called when the graphics subsystem is
             * available and the object should acquire renderer-specific handles.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload or release renderer resources held by this object.
             *
             * @param data Shared object that may be required to properly release resources.
             *
             * After unload the object should be in a state where it can be safely
             * re-loaded or destroyed.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Get the debug text string.
             *
             * @return The currently stored text.
             */
            String getText() const override;

            /**
             * @brief Set the debug text string.
             *
             * @param text New text to display. Implementation should update the
             *             overlay element if one is attached.
             */
            void setText( const String &text ) override;

            /**
             * @brief Get the overlay element used to render the text.
             *
             * @return Smart pointer to the overlay text element or null if none set.
             */
            SmartPtr<IOverlayElementText> getTextElement() const override;

            /**
             * @brief Set the overlay element used to render the text.
             *
             * @param textElement Smart pointer to an overlay element that will be
             *                    used for rendering the stored text. Ownership is
             *                    shared via SmartPtr.
             */
            void setTextElement( SmartPtr<IOverlayElementText> textElement ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief The text displayed by this debug object.
             *
             * Stored separately so the value can be updated before the overlay
             * element is attached or while switching renderers.
             */
            FixedString<WP_MAX_PATH> m_text;

            /**
             * @brief Overlay element used for rendering the debug text.
             *
             * This is a renderer-specific object (wrapped by `IOverlayElementText`).
             * It can be null when the DebugText is not attached to a renderer.
             */
            SmartPtr<IOverlayElementText> m_textElement;
        };
    }  // namespace render
}  // namespace workphone

#endif  // __Render_DebugText_h__
