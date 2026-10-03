#ifndef ImGuiColourPicker_h__
#define ImGuiColourPicker_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIColourPicker.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiColourPicker : public ImGuiElement<IUIColourPicker>
        {
        public:
            ImGuiColourPicker();
            ~ImGuiColourPicker() override;

            void update() override;

            String getLabel() const override;

            void setLabel( const String &label ) override;

            void setGradient( const ColourF &startColour, const ColourF &endColour ) override;

            Pair<ColourF, ColourF> getGradient() const override;

            void setColourFormat( ColourFormat format ) override;

            ColourFormat getColourFormat() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            ColourFormat m_colourFormat = ColourFormat::RGB;
            Pair<ColourF, ColourF> m_gradient;
            FixedString<128> m_label;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiColourPicker_h__
