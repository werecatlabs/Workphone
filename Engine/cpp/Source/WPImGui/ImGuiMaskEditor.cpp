#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiMaskEditor.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiMaskEditor, ImGuiElement<IUIElement> );

    ImGuiMaskEditor::~ImGuiMaskEditor() = default;

    ImGuiMaskEditor::ImGuiMaskEditor() = default;

    void ImGuiMaskEditor::showCollisionMaskEditor( u32 &mask )
    {
        ImGui::Text( "Collision Mask" );

        ImGui::Columns( 8, nullptr, false );  // 8 columns for compact display
        for( s32 i = 0; i < 32; ++i )
        {
            bool bitSet = ( mask & ( 1 << i ) ) != 0;
            if( ImGui::Checkbox( ( "##bit" + std::to_string( i ) ).c_str(), &bitSet ) )
            {
                if( bitSet )
                    mask |= ( 1 << i );  // Set bit
                else
                    mask &= ~( 1 << i );  // Clear bit
            }
            ImGui::SameLine();
            ImGui::Text( " %d ", i );
            if( ( i + 1 ) % 8 == 0 )  // New row every 8 checkboxes
                ImGui::NextColumn();
        }
        ImGui::Columns( 1 );
    }

    void ImGuiMaskEditor::showHexCollisionMaskEditor( u32 &mask )
    {
        ImGui::Text( "Collision Mask: 0x%08X", mask );
        ImGui::SameLine();
        ImGui::InputScalar( "##hexmask", ImGuiDataType_U32, &mask, nullptr, nullptr, "%08X",
                            ImGuiInputTextFlags_CharsHexadecimal );
    }
}  // namespace workphone::ui
