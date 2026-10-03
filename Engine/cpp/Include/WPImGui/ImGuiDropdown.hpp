#ifndef ImGuiDropdown_h__
#define ImGuiDropdown_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIDropdown.hpp>

namespace workphone
{
    namespace core
    {
        extern template class WPCore_API Prototype<ui::IUIDropdown>;
    }

    namespace ui
    {
        class ImGuiDropdown : public ImGuiElement<IUIDropdown>
        {
        public:
            ImGuiDropdown();
            ~ImGuiDropdown() override;

            Array<String> getOptions() const override;
            void setOptions( const Array<String> &options ) override;
            void addOption( const String &option ) override;

            u32 getSelectedOption() const override;
            void setSelectedOption( u32 selectedOption ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<FixedString<128>> m_options;
            u32 m_selectedOption = 0;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiDropdown_h__
