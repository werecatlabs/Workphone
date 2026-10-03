#ifndef __ImGuiText_h__
#define __ImGuiText_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/UI/UIText.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiText : public ImGuiElement<IUIText>
        {
        public:
            ImGuiText();
            ~ImGuiText() override;

            void update() override;

            /** @copydoc IUIText::setText */
            void setText( const String &text ) override;

            /** @copydoc IUIText::getText */
            String getText() const override;

            /** @copydoc IUIText::getText */
            const c8 *getTextPtr() const;

            /** @copydoc IUIText::setTextSize */
            void setTextSize( f32 textSize ) override;

            /** @copydoc IUIText::getTextSize */
            f32 getTextSize() const override;

            void setVerticalAlignment( u8 alignment ) override;
            u8 getVerticalAlignment() const override;

            void setHorizontalAlignment( u8 alignment ) override;

            u8 getHorizontalAlignment() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Renders text with proper alignment. */
            void renderTextWithAlignment( const String &text );

            /** Text size in points. */
            f32 m_textSize = 10.0f;

            /** Horizontal alignment using HorizontalAlignment enum values. */
            u8 m_horizontalAlignment = static_cast<u8>( HorizontalAlignment::LEFT );

            /** Vertical alignment using VerticalAlignment enum values. */
            u8 m_verticalAlignment = static_cast<u8>( VerticalAlignment::TOP );

            /** Text string. */
            FixedString<128> m_text;

            /** Long text string for texts exceeding the fixed buffer size. */
            String m_longText;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // CEGUIText_h__
