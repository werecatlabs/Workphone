#ifndef __UIButtonCore_h__
#define __UIButtonCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/UI/UIButton.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class UIButtonOgreNext
         * @brief OgreNext implementation of a UI button element.
         *
         * This class provides a concrete implementation of a UI button using the OgreNext
         * rendering engine and the Core UI library. It handles button rendering, state
         * management, and user interactions within the OgreNext graphics framework.
         */
        class UIButtonCore : public UIElementCore<UIButton>
        {
        public:
            UIButtonCore();
            ~UIButtonCore() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Per-frame update � submits the button draw command and dispatches click events.
             *
             * Applies the current style to the WorkphoneCore context, emits a `wp_button_label`
             * call sized to this element's bounds, and fires `IEvent::CLICK_HASH` to all
             * registered object listeners if the button was clicked this frame.
             */
            void update() override;

            void setTextSize( f32 textSize ) override;
            f32 getTextSize() const override;

            // ---------------------------------------------------------------
            // Reference canvas
            // ---------------------------------------------------------------

            f32 getReferenceWidth() const;
            void setReferenceWidth( f32 width );

            f32 getReferenceHeight() const;
            void setReferenceHeight( f32 height );

            // ---------------------------------------------------------------
            // Text colours
            // ---------------------------------------------------------------

            ColourF getTextNormalColour() const;
            void setTextNormalColour( const ColourF &colour );

            ColourF getTextHoverColour() const;
            void setTextHoverColour( const ColourF &colour );

            ColourF getTextActiveColour() const;
            void setTextActiveColour( const ColourF &colour );

            ColourF getTextBackgroundColour() const;
            void setTextBackgroundColour( const ColourF &colour );

            // ---------------------------------------------------------------
            // Background colours
            // ---------------------------------------------------------------

            ColourF getNormalColour() const;
            void setNormalColour( const ColourF &colour );

            ColourF getHoverColour() const;
            void setHoverColour( const ColourF &colour );

            ColourF getActiveColour() const;
            void setActiveColour( const ColourF &colour );

            // ---------------------------------------------------------------
            // Border
            // ---------------------------------------------------------------

            ColourF getBorderColour() const;
            void setBorderColour( const ColourF &colour );

            f32 getBorderWidth() const;
            void setBorderWidth( f32 width );

            // ---------------------------------------------------------------
            // Shape
            // ---------------------------------------------------------------

            f32 getRounding() const;
            void setRounding( f32 rounding );

            // ---------------------------------------------------------------
            // Padding
            // ---------------------------------------------------------------

            Vector2F getPadding() const;
            void setPadding( const Vector2F &padding );

            // ---------------------------------------------------------------
            // Properties round-trip
            // ---------------------------------------------------------------

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            //bool handleEvent( const SmartPtr<IInputEvent> &event );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Push the current style members into the WorkphoneCore context. */
            void applyStyle( struct wp_context *ctx ) const;

            /// Underlying Core button widget (non-owning sentinel pointer).
            struct wp_button *m_button = nullptr;

            // ---------------------------------------------------------------
            // Configuration members
            // ---------------------------------------------------------------

            f32 m_referenceWidth = 1920.0f;
            f32 m_referenceHeight = 1080.0f;

            // Text colours
            ColourF m_textNormalColour = ColourF( 1.0f, 1.0f, 1.0f, 1.0f );
            ColourF m_textHoverColour = ColourF( 1.0f, 1.0f, 1.0f, 1.0f );
            ColourF m_textActiveColour = ColourF( 1.0f, 1.0f, 1.0f, 1.0f );
            ColourF m_textBackgroundColour = ColourF( 0.0f, 0.0f, 0.0f, 0.0f );

            // Background colours (solid fill per-state)
            ColourF m_normalColour = ColourF( 0.26f, 0.26f, 0.26f, 1.0f );
            ColourF m_hoverColour = ColourF( 0.40f, 0.40f, 0.40f, 1.0f );
            ColourF m_activeColour = ColourF( 0.18f, 0.18f, 0.18f, 1.0f );

            // Border
            ColourF m_borderColour = ColourF( 0.50f, 0.50f, 0.50f, 1.0f );
            f32 m_borderWidth = 1.0f;

            // Shape
            f32 m_rounding = 4.0f;

            // Padding
            Vector2F m_padding = Vector2F( 4.0f, 4.0f );

            // Label text size used by editor/runtime state for this Core backend.
            f32 m_textSize = 1.0f;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // __UIButtonCore_h__
