#ifndef ImGuiLabelDropdownPair_h__
#define ImGuiLabelDropdownPair_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/Interface/UI/IUILabelDropdownPair.hpp>
#include <WPImGui/ImGuiElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiLabelDropdownPair : public ImGuiElement<IUILabelDropdownPair>
        {
        public:
            ImGuiLabelDropdownPair();
            ~ImGuiLabelDropdownPair() override;

            void update() override;

            String getLabel() const override;
            void setLabel( const String &label ) override;

            Array<String> getOptions() const override;
            void setOptions( const Array<String> &options ) override;

            void addOption( const String &option ) override;
            void removeOption( const String &option ) override;

            u32 getSelectedOption() const override;
            void setSelectedOption( u32 selectedOption ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_label;
            Array<String> m_options;
            u32 m_selectedOption = 0;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiLabelDropdownPair_h__
