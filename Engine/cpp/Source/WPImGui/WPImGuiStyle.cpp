#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/WPImGuiStyle.hpp>
#include <imgui.h>

#include <algorithm>

namespace workphone::ui
{
    namespace
    {
        constexpr ImVec4 rgb( int r, int g, int b, int a = 255 )
        {
            return ImVec4( r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f );
        }

        constexpr ImVec4 grays[] = { rgb( 91, 91, 91 ), rgb( 76, 76, 76 ),
                                     rgb( 68, 68, 68 ), rgb( 58, 58, 58 ),
                                     rgb( 48, 48, 48 ), rgb( 44, 44, 44 ),
                                     rgb( 35, 35, 35 ), rgb( 28, 28, 28 ),
                                     rgb( 22, 22, 22 ), rgb( 17, 17, 17 ) };

        constexpr ImVec4 accents[] = { rgb( 63, 255, 63 ), rgb( 50, 205, 50 ),
                                       rgb( 36, 147, 36 ) };
    }  // namespace

    ImVec4 WPImGuiStyle::gray( u32 level )
    {
        return grays[std::min<u32>( level, 9 )];
    }

    ImVec4 WPImGuiStyle::accent( u32 level )
    {
        return accents[std::min<u32>( level, 2 )];
    }

    ImVec4 WPImGuiStyle::axisX()
    {
        return rgb( 205, 65, 65 );
    }

    ImVec4 WPImGuiStyle::axisY()
    {
        return rgb( 50, 205, 50 );
    }

    ImVec4 WPImGuiStyle::axisZ()
    {
        return rgb( 65, 105, 225 );
    }

    ImVec4 WPImGuiStyle::axisW()
    {
        return rgb( 255, 140, 0 );
    }

    void WPImGuiStyle::apply( f32 dpiScale )
    {
        ImGuiStyle &style = ImGui::GetStyle();
        style = ImGuiStyle();

        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_Text] = rgb( 255, 255, 255 );
        colors[ImGuiCol_TextDisabled] = rgb( 130, 130, 130 );
        colors[ImGuiCol_TextSelectedBg] = gray( 0 );

        colors[ImGuiCol_TitleBg] = gray( 9 );
        colors[ImGuiCol_TitleBgActive] = gray( 8 );
        colors[ImGuiCol_TitleBgCollapsed] = gray( 8 );
        colors[ImGuiCol_WindowBg] = gray( 6 );
        colors[ImGuiCol_ChildBg] = gray( 6 );
        colors[ImGuiCol_PopupBg] = gray( 6 );
        colors[ImGuiCol_MenuBarBg] = gray( 6 );

        colors[ImGuiCol_Border] = gray( 2 );
        colors[ImGuiCol_BorderShadow] = gray( 6 );
        colors[ImGuiCol_FrameBg] = gray( 8 );
        colors[ImGuiCol_FrameBgHovered] = gray( 7 );
        colors[ImGuiCol_FrameBgActive] = gray( 5 );

        colors[ImGuiCol_Tab] = gray( 6 );
        colors[ImGuiCol_TabActive] = gray( 4 );
        colors[ImGuiCol_TabHovered] = gray( 3 );
        colors[ImGuiCol_TabUnfocused] = gray( 6 );
        colors[ImGuiCol_TabUnfocusedActive] = gray( 5 );

        colors[ImGuiCol_Header] = gray( 3 );
        colors[ImGuiCol_HeaderHovered] = gray( 2 );
        colors[ImGuiCol_HeaderActive] = gray( 1 );
        colors[ImGuiCol_Separator] = gray( 2 );
        colors[ImGuiCol_SeparatorHovered] = gray( 1 );
        colors[ImGuiCol_SeparatorActive] = gray( 0 );
        colors[ImGuiCol_NavHighlight] = gray( 1 );
        colors[ImGuiCol_DockingPreview] = gray( 1 );

        colors[ImGuiCol_ScrollbarBg] = gray( 6 );
        colors[ImGuiCol_ScrollbarGrab] = gray( 3 );
        colors[ImGuiCol_ScrollbarGrabHovered] = gray( 2 );
        colors[ImGuiCol_ScrollbarGrabActive] = gray( 1 );
        colors[ImGuiCol_SliderGrab] = gray( 2 );
        colors[ImGuiCol_SliderGrabActive] = gray( 1 );
        colors[ImGuiCol_ResizeGrip] = gray( 3 );
        colors[ImGuiCol_ResizeGripHovered] = gray( 2 );
        colors[ImGuiCol_ResizeGripActive] = gray( 2 );

        colors[ImGuiCol_Button] = gray( 3 );
        colors[ImGuiCol_ButtonHovered] = gray( 2 );
        colors[ImGuiCol_ButtonActive] = gray( 1 );
        colors[ImGuiCol_CheckMark] = accent( 1 );
        colors[ImGuiCol_PlotLines] = accent( 2 );
        colors[ImGuiCol_PlotLinesHovered] = accent( 1 );
        colors[ImGuiCol_PlotHistogram] = accent( 2 );
        colors[ImGuiCol_PlotHistogramHovered] = accent( 1 );
        colors[ImGuiCol_TableRowBg] = gray( 6 );
        colors[ImGuiCol_TableRowBgAlt] = gray( 5 );
        colors[ImGuiCol_DragDropTarget] = accent( 0 );

        style.FramePadding = ImVec2( 6.0f, 6.0f );
        style.WindowPadding = ImVec2( 6.0f, 6.0f );
        style.ChildBorderSize = 0.0f;
        style.TabBorderSize = 1.0f;
        style.GrabRounding = 0.0f;
        style.GrabMinSize = 8.0f;
        style.WindowRounding = 0.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameRounding = 3.0f;
        style.IndentSpacing = 12.0f;
        style.ItemSpacing = ImVec2( 4.0f, 6.0f );
        style.TabRounding = 6.0f;
        style.ScrollbarSize = 20.0f;
        style.ScrollbarRounding = 0.0f;
        style.CellPadding = ImVec2( 4.0f, 6.0f );

        style.ScaleAllSizes( std::max( dpiScale, 1.0f ) );
    }
}  // namespace workphone::ui
