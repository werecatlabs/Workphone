#ifndef ImGuiLabelSliderPair_h__
#define ImGuiLabelSliderPair_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/Interface/UI/IUILabelSliderPair.hpp>
#include <WPImGui/ImGuiElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiLabelSliderPair : public ImGuiElement<IUILabelSliderPair>
        {
        public:
            ImGuiLabelSliderPair();
            ~ImGuiLabelSliderPair() override;

            void update() override;

            String getLabel() const override;

            void setLabel( const String &label ) override;

            f32 getValue() const override;

            void setValue( f32 value ) override;

            f32 getMinValue() const override;

            void setMinValue( f32 minValue ) override;

            f32 getMaxValue() const override;

            void setMaxValue( f32 maxValue ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_label;
            f32 m_value = 0.5f;
            f32 m_minValue = 0.0f;
            f32 m_maxValue = 1.0f;
            bool m_showValue = true;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiLabelSliderPair_h__
