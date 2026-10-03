#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiAbout.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiAbout, ImGuiElement<IUIAbout> );

    ImGuiAbout::ImGuiAbout() = default;

    ImGuiAbout::~ImGuiAbout() = default;

    void ImGuiAbout::update()
    {
        // Title with larger font
        ImGui::PushFont( ImGui::GetIO().Fonts->Fonts[0] );  // Use default font for now
        ImGui::SetCursorPosX( ( ImGui::GetWindowWidth() - ImGui::CalcTextSize( "Work Phone" ).x ) *
                              0.5f );
        ImGui::TextColored( ImVec4( 0.2f, 0.6f, 1.0f, 1.0f ), "Work Phone" );
        ImGui::PopFont();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Version info
        ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.7f, 1.0f ), "Version" );
        ImGui::Text( "1.0.0" );

        ImGui::Spacing();

        // Copyright
        ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.7f, 1.0f ), "Copyright" );
        ImGui::Text( "© 2023 Your Company. All rights reserved." );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Description
        ImGui::TextWrapped(
            "This application is designed for games, providing essential tools and features for game "
            "development and management." );

        ImGui::Spacing();

        // Website link
        ImGui::TextColored( ImVec4( 0.2f, 0.6f, 1.0f, 1.0f ), "WorkPhoneGames" );
        if( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Click to visit our website" );
            if( ImGui::IsItemClicked() )
            {
                // TODO: Add website URL opening functionality
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Additional info
        ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.7f, 1.0f ), "System Information" );
        ImGui::Text( "Build Date: %s", __DATE__ );
        ImGui::Text( "Build Time: %s", __TIME__ );
    }
}  // namespace workphone::ui
