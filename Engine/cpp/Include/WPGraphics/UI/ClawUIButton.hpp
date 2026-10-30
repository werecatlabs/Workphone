#ifndef _ClawUIBUTTON_H
#define _ClawUIBUTTON_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>
#include <memory>

struct wp_context;
struct wp_user_font;

namespace workphone
{
    namespace ui
    {

        // Use the Prototype<IUIButton> instantiation exported from Workphone.dll
        // to avoid LNK2005 (already defined) when linking against Workphone.lib.
#if defined( WP_PLATFORM_WIN32 ) && !defined( _WP_STATIC_LIB_ ) && !defined( WPCore_EXPORTS )
        extern template class core::Prototype<IUIButton>;
#endif

        class ClawUIButton : public ClawUIElement<IUIButton>
        {
        public:
            ClawUIButton();
            ~ClawUIButton() override;

            /** @copydoc IObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            void setPosition( const Vector2F &position ) override;

            void setTextSize( f32 textSize ) override;
            f32 getTextSize() const override;

            bool isSimpleButton() const;
            void setSimpleButton( bool simpleButton );

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc GuiElement::draw */
            void draw( struct wp_context *ctx ) override;

            void handleButtonClick();

        protected:
            String m_defaultMaterial;
            String m_hoverMaterial;
            f32 m_textSize = 12.0f;
            bool m_isSimpleButton = true;
            bool m_hasLocalStyle = false;
            ColourF m_normalColour = ColourF::White;
            ColourF m_hoverColour = ColourF::White;
            ColourF m_activeColour = ColourF::White;
            ColourF m_disabledColour = ColourF::White;
            std::unique_ptr<wp_user_font> m_drawFont;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
