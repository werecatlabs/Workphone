#ifndef _ClawUITEXT_H
#define _ClawUITEXT_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

struct wp_context;

namespace workphone
{
    namespace ui
    {
        /**
         * @brief Uses the Prototype<IUIText> instantiation exported from Workphone.dll.
         *
         * This avoids duplicate symbol errors when linking against Workphone.lib.
         */
#if defined( WP_PLATFORM_WIN32 ) && !defined( _WP_STATIC_LIB_ ) && !defined( WPCore_EXPORTS )
        extern template class core::Prototype<IUIText>;
#endif
        /**
         * @class ClawUIText
         * @brief UI text element implementation backed by a claw UI overlay element.
         *
         * This class stores the text state locally and forwards rendering-related
         * behavior to the underlying overlay text object.
         */
        class ClawUIText : public ClawUIElement<IUIText>
        {
        public:
            /**
             * @brief Creates a new text UI element.
             */
            ClawUIText();

            /**
             * @brief Destroys the text UI element.
             */
            ~ClawUIText() override;

            /**
             * @brief Loads the text element state from shared object data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the text element state from shared object data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the element position.
             */
            void setPosition( const Vector2F &position ) override;

            /**
             * @brief Updates the element size.
             */
            void setSize( const Vector2F &size ) override;

            /**
             * @brief Sets the displayed text.
             */
            void setText( const String &text ) override;

            /**
             * @brief Gets the currently displayed text.
             */
            String getText() const override;

            /**
             * @brief Sets the text size in pixels.
             */
            void setTextSize( f32 textSize ) override;

            /**
             * @brief Gets the current text size in pixels.
             */
            f32 getTextSize() const override;

            /**
             * @brief Sets the vertical text alignment.
             */
            void setVerticalAlignment( u8 alignment ) override;

            /**
             * @brief Gets the vertical text alignment.
             */
            u8 getVerticalAlignment() const override;

            /**
             * @brief Sets the horizontal text alignment.
             */
            void setHorizontalAlignment( u8 alignment ) override;

            /**
             * @brief Gets the horizontal text alignment.
             */
            u8 getHorizontalAlignment() const override;

            /**
             * @brief Returns the underlying COM-style object pointer.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Gets the underlying overlay text object.
             */
            SmartPtr<render::IOverlayElementText> getOverlayText() const;

            /**
             * @brief Sets the underlying overlay text object.
             */
            void setOverlayText( SmartPtr<render::IOverlayElementText> overlayText );

            /**
             * @brief Handles state changes propagated to the UI element.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @copydoc GuiElement::draw
             */
            void draw( struct wp_context *ctx ) override;

            /**
             * @brief Registers the class with the runtime type system.
             */
            WP_CLASS_REGISTER_DECL;

        private:
            FixedString<1024> m_text;            ///< Cached text content.
            f32 m_textSize = 12.0f;              ///< Cached text size in pixels.
            u8 m_verticalAlignment = 0;          ///< Cached vertical alignment value.
            u8 m_horizontalAlignment = 0;        ///< Cached horizontal alignment value.
        };
    }  // end namespace ui
}  // namespace workphone

#endif
