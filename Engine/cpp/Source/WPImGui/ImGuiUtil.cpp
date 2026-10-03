#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiUtil.hpp>
#include <WPImGui/WPImGuiStyle.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace workphone::ui
{
    bool ImGuiUtil::ToggleButton( const char *str_id, bool *v, f32 size )
    {
        bool itemClicked = false;

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        float height = ImGui::GetFrameHeight() * size;
        float width = height * 1.55f;
        float radius = height * 0.50f;

        ImGui::InvisibleButton( str_id, ImVec2( width, height ) );
        if( ImGui::IsItemClicked() )
        {
            auto value = ( *v );
            *v = !value;
            itemClicked = true;
        }

        float t = *v ? 1.0f : 0.0f;

        ImGuiContext &g = *GImGui;
        float ANIM_SPEED = 0.08f;
        if( g.LastActiveId == g.CurrentWindow->GetID( str_id ) )  // && g.LastActiveIdTimer < ANIM_SPEED)
        {
            float t_anim = ImSaturate( g.LastActiveIdTimer / ANIM_SPEED );
            t = *v ? ( t_anim ) : ( 1.0f - t_anim );
        }

        ImU32 col_bg;
        if( ImGui::IsItemHovered() )
        {
            col_bg = ImGui::GetColorU32(
                ImLerp( ImVec4( 0.78f, 0.78f, 0.78f, 1.0f ), ImVec4( 0.06f, 0.5f, 0.9f, 1.00f ), t ) );
        }
        else
        {
            col_bg = ImGui::GetColorU32(
                ImLerp( ImVec4( 0.85f, 0.85f, 0.85f, 1.0f ), ImVec4( 0.06f, 0.5f, 0.9f, 1.00f ), t ) );
        }

        draw_list->AddRectFilled( p, ImVec2( p.x + width, p.y + height ), col_bg, height * 0.5f );
        draw_list->AddCircleFilled( ImVec2( p.x + radius + t * ( width - radius * 2.0f ), p.y + radius ),
                                    radius - 1.5f, IM_COL32( 255, 255, 255, 255 ) );

        return itemClicked;
    }

    void ImGuiUtil::SameLineSeparator( f32 width )
    {
        const auto &style = ImGui::GetStyle();
        const ImVec2 separatorSize(
            width <= 0.0f ? ( style.ItemSpacing.x * 2.0f ) + 1.0f : width,
            ImGui::GetFrameHeight() );

        ImGui::SameLine( 0.0f, 0.0f );
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const f32 x = canvasPos.x + std::floor( separatorSize.x * 0.5f );
        ImGui::GetWindowDrawList()->AddLine( ImVec2( x, canvasPos.y + 1.0f ),
                                             ImVec2( x, canvasPos.y + separatorSize.y - 2.0f ),
                                             ImGui::GetColorU32( ImGuiCol_Separator ) );
        ImGui::Dummy( separatorSize );
        ImGui::SameLine( 0.0f, 0.0f );
    }

    void ImGuiUtil::ItemTooltip( const char *text )
    {
        if( !text || !text[0] )
        {
            return;
        }

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4.0f, 4.0f ) );
        if( ImGui::IsItemHovered() && GImGui->HoveredIdTimer > WPImGuiStyle::ToolTipDelay )
        {
            ImGui::SetTooltip( "%s", text );
        }
        ImGui::PopStyleVar();
    }

    bool ImGuiUtil::Spinner( const char *id, f32 size, f32 thickness, f32 padding )
    {
        if( !id || !id[0] || ImGui::GetCurrentWindow()->SkipItems )
        {
            return false;
        }

        if( size < 0.0f )
        {
            const ImVec2 available = ImGui::GetContentRegionAvail();
            size = std::min( available.x, available.y );
        }
        if( size <= 0.0f )
        {
            size = ImGui::GetFrameHeight();
        }

        thickness = std::max( thickness, 1.0f );
        const f32 radius = std::max( ( size - thickness ) * 0.5f - padding, 1.0f );
        const bool pressed = ImGui::InvisibleButton( id, ImVec2( size, size ) );
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        const ImVec2 center( ( min.x + max.x ) * 0.5f, ( min.y + max.y ) * 0.5f );

        constexpr int segmentCount = 30;
        constexpr f32 pi = 3.14159265358979323846f;
        const f32 time = static_cast<f32>( ImGui::GetTime() );
        const f32 first = std::fabs( std::sin( time * 1.8f ) ) * ( segmentCount - 5.0f );
        const f32 arcMin = 2.0f * pi * first / segmentCount;
        const f32 arcMax = 2.0f * pi * ( segmentCount - 3.0f ) / segmentCount;

        ImDrawList *drawList = ImGui::GetWindowDrawList();
        for( int i = 0; i < segmentCount; ++i )
        {
            const f32 angle = arcMin + ( static_cast<f32>( i ) / segmentCount ) *
                                           ( arcMax - arcMin ) +
                              time * 8.0f;
            drawList->PathLineTo( ImVec2( center.x + std::cos( angle ) * radius,
                                          center.y + std::sin( angle ) * radius ) );
        }
        drawList->PathStroke( ImGui::GetColorU32( ImGuiCol_CheckMark ), false, thickness );
        return pressed;
    }
}  // namespace workphone::ui
