#ifndef ImGuiToggleButton_h__
#define ImGuiToggleButton_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIToggle.hpp>

namespace workphone
{
    namespace core
    {
        extern template class WPCore_API Prototype<ui::IUIToggle>;
    }

    namespace ui
    {
        class ImGuiToggleButton : public ImGuiElement<IUIToggle>
        {
        public:
            ImGuiToggleButton();
            ~ImGuiToggleButton() override;

            void update() override;

            void setToggled( bool toggled ) override;

            bool isToggled() const override;

            ToggleType getToggleType() const override;

            void setToggleType( ToggleType toggleType ) override;

            ToggleState getToggleState() const override;

            void setToggleState( ToggleState toggleState ) override;

            bool getShowLabel() const override;

            void setShowLabel( bool showLabel ) override;

            String getLabel() const override;

            void setLabel( const String &label ) override;

            void setTextSize( f32 textSize ) override;

            f32 getTextSize() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_label;
            f32 m_textSize = 1.0f;
            bool m_showLabel = true;
            bool m_isToggled = false;
            ToggleType m_toggleType = ToggleType::ToggleButton;
            ToggleState m_toggleState = ToggleState::Off;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ImGuiToggleButton_h__
