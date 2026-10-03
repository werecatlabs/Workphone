#ifndef ImGuiTextEntry_h__
#define ImGuiTextEntry_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiTextEntry : public ImGuiElement<IUITextEntry>
        {
        public:
            ImGuiTextEntry();
            ~ImGuiTextEntry() override;

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

            WP_CLASS_REGISTER_DECL;

        protected:
            bool m_readOnly = false;
            bool m_secureEntry = false;
            bool m_multiline = false;
            InputType m_inputType = InputType::Text;
            FixedString<512> m_text;
            FixedString<128> m_placeholder;
            FixedString<128> m_textHint;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ImGuiTextEntry_h__
