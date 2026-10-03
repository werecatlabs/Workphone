#ifndef _ClawUIBUTTON_H
#define _ClawUIBUTTON_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

struct wp_context;

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

            String getLabel() const override;
            void setLabel( const String &label ) override;

            void setTextSize( f32 textSize ) override;
            f32 getTextSize() const override;

            bool isSimpleButton() const;
            void setSimpleButton( bool simpleButton );

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /** @copydoc GuiElement::draw */
            void draw( struct wp_context *ctx ) override;

            void handleButtonClick();

        protected:
            String m_defaultMaterial;
            String m_hoverMaterial;
            f32 m_textSize = 12.0f;
            bool m_isSimpleButton = true;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
