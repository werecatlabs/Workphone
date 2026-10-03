#ifndef _ClawUITextEntry_H
#define _ClawUITextEntry_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

struct wp_context;

namespace workphone
{
    namespace ui
    {
        class ClawUITextEntry : public ClawUIElement<IUITextEntry>
        {
        public:
            ClawUITextEntry();
            ~ClawUITextEntry() override;

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            void update() override;

            void setText( const String &text ) override;
            String getText() const override;

            void setTextSize( f32 textSize ) override;
            f32 getTextSize() const override;

            void setVerticalAlignment( u8 alignment ) override;

            u8 getVerticalAlignment() const override;

            void setHorizontalAlignment( u8 alignment ) override;

            u8 getHorizontalAlignment() const override;

            void setPlaceholder( const String &placeholder ) override;

            String getPlaceholder() const override;

            void setReadOnly( bool readOnly ) override;

            bool isReadOnly() const override;

            void setSecureEntry( bool secureEntry ) override;

            bool isSecureEntry() const override;

            void setMultiline( bool multiline ) override;

            bool isMultiline() const override;

            void setInputType( InputType inputType, const String &textHint = "" ) override;

            InputType getInputType() const override;

            String getTextHint() const override;

            void setPosition( const Vector2<real_Num> &position ) override;

            void setSize( const Vector2<real_Num> &size ) override;

            /** @copydoc GuiElement::draw */
            void draw( struct wp_context *ctx ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            String m_text;
            String m_placeholder;
            String m_textHint;

            String m_defaultMaterial;
            String m_hoverMaterial;

            f32 m_textSize = 12.0f;
            u8 m_verticalAlignment = 0;
            u8 m_horizontalAlignment = 0;
            InputType m_inputType = InputType::Text;
            bool m_readOnly = false;
            bool m_secureEntry = false;
            bool m_multiline = false;
            u32 m_nextCursorFlash;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
