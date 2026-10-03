#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiDropdown.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiDropdown, ImGuiElement<IUIDropdown> );

    ImGuiDropdown::ImGuiDropdown() = default;

    ImGuiDropdown::~ImGuiDropdown() = default;

    Array<String> ImGuiDropdown::getOptions() const
    {
        Array<String> options;
        for( const auto &option : m_options )
        {
            options.push_back( option );
        }

        return options;
    }

    void ImGuiDropdown::setOptions( const Array<String> &options )
    {
        m_options.clear();
        for (const auto &option : options)
        {
            m_options.push_back(option);
        }   
    }

    void ImGuiDropdown::addOption( const String &option )
    {
        m_options.push_back( option );
    }

    u32 ImGuiDropdown::getSelectedOption() const
    {
        return m_selectedOption;
    }

    void ImGuiDropdown::setSelectedOption( u32 selectedOption )
    {
        m_selectedOption = selectedOption;
    }
}  // namespace workphone::ui
