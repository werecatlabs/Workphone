#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiOutputConsole.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    void ImGuiOutputConsole::Clear()
    {
        m_log.clear();
    }

    void ImGuiOutputConsole::Show()
    {
        ImGui::Begin( "Output Console" );

        //ImGui::BeginChild( "ScrollingRegion", ImVec2( 0, -ImGui::GetItemsLineHeightWithSpacing() ),
        //                   false, ImGuiWindowFlags_HorizontalScrollbar );

        for( const std::string &message : m_log )
        {
            ImGui::Text( "%s", message.c_str() );
        }

        ImGui::EndChild();

        if( ImGui::Button( "Clear" ) )
        {
            Clear();
        }

        ImGui::End();
    }

    void ImGuiOutputConsole::Log( const std::string &message )
    {
        m_log.push_back( message );
    }

    ImGuiOutputConsole::ImGuiOutputConsole() = default;

}  // namespace workphone::ui
