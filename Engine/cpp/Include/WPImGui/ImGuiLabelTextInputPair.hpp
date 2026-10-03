#ifndef ImGuiLabelTextInputPair_h__
#define ImGuiLabelTextInputPair_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/Interface/UI/IUILabelTextInputPair.hpp>
#include <WPImGui/ImGuiElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiLabelTextInputPair : public ImGuiElement<IUILabelTextInputPair>
        {
        public:
            ImGuiLabelTextInputPair();
            ~ImGuiLabelTextInputPair() override;

            String getLabel() const override;
            void setLabel( const String &label ) override;

            String getValue() const override;
            void setValue( const String &value ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_label;
            String m_value;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiLabelTextInputPair_h__
