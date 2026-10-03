#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiLabelDropdownPair.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiLabelDropdownPair,
                               ImGuiElement<IUILabelDropdownPair> );

    ImGuiLabelDropdownPair::ImGuiLabelDropdownPair() = default;

    ImGuiLabelDropdownPair::~ImGuiLabelDropdownPair() = default;

    void ImGuiLabelDropdownPair::update()
    {
        if( m_options.empty() )
        {
            return;
        }

        auto selectedOption = m_options[m_selectedOption];
        if( ImGui::BeginCombo( m_label.c_str(), selectedOption.c_str() ) )
        {
            for( u32 i = 0; i < m_options.size(); ++i )
            {
                const bool isSelected = ( i == m_selectedOption );
                if( ImGui::Selectable( m_options[i].c_str(), isSelected ) )
                {
                    m_selectedOption = i;
                }

                if( isSelected )
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }

    void ImGuiLabelDropdownPair::setLabel( const String &label )
    {
        m_label = label;
    }

    String ImGuiLabelDropdownPair::getLabel() const
    {
        return m_label;
    }

    Array<String> ImGuiLabelDropdownPair::getOptions() const
    {
        return m_options;
    }

    void ImGuiLabelDropdownPair::setOptions( const Array<String> &options )
    {
        m_options = options;
    }

    u32 ImGuiLabelDropdownPair::getSelectedOption() const
    {
        return m_selectedOption;
    }

    void ImGuiLabelDropdownPair::setSelectedOption( u32 selectedOption )
    {
        m_selectedOption = selectedOption;
    }

    void ImGuiLabelDropdownPair::addOption( const String &option )
    {
        m_options.push_back( option );
    }

    void ImGuiLabelDropdownPair::removeOption( const String &option )
    {
        auto it = std::find( m_options.begin(), m_options.end(), option );
        if( it != m_options.end() )
        {
            m_options.erase( it );
        }
    }
}  // namespace workphone::ui
