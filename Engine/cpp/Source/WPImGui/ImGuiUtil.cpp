#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiUtil.hpp>
#include <WPImGui/WPImGuiStyle.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace workphone::ui
{
    bool ImGuiUtil::SliderFloatWithInput( const char *label, f32 *value, f32 minimum,
                                         f32 maximum, const char *format )
    {
        const auto previous = *value;
        const auto &style = ImGui::GetStyle();
        const auto labelEnd = ImGui::FindRenderedTextEnd( label );
        const auto labelWidth = ImGui::CalcTextSize( label, labelEnd ).x;
        const auto available = ImGui::GetContentRegionAvail().x;
        const auto requestedWidth = ImGui::CalcItemWidth();
        const bool labelAbove = labelWidth > 0.0f &&
                                available < labelWidth + ImGui::GetFontSize() * 18.0f;
        const auto labelSpace = labelWidth > 0.0f && !labelAbove
                                    ? labelWidth + style.ItemInnerSpacing.x : 0.0f;
        const auto width = std::max( 1.0f, std::min( requestedWidth, available - labelSpace ) );
        const auto spacing = std::min( style.ItemInnerSpacing.x, width * 0.1f );
        const auto inputWidth = std::min( ImGui::GetFontSize() * 8.0f, ( width - spacing ) * 0.65f );

        ImGui::BeginGroup();
        ImGui::PushID( label );
        if( labelAbove )
        {
            ImGui::TextUnformatted( label, labelEnd );
        }
        ImGui::SetNextItemWidth( width - inputWidth - spacing );
        const bool sliderChanged = ImGui::SliderFloat(
            "##slider", value, minimum, maximum, format,
            ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat );
        ImGui::SameLine( 0.0f, spacing );
        ImGui::SetNextItemWidth( inputWidth );

        // Keep the draft's parsed value in window storage. ImGui owns the active
        // text buffer, but can replace it when focus moves to an earlier widget.
        auto storage = ImGui::GetStateStorage();
        const auto stateId = ImGui::GetID( "##editState" );
        const auto draftId = ImGui::GetID( "##draft" );
        const auto frameId = ImGui::GetID( "##editFrame" );
        const auto inputId = ImGui::GetID( "##value" );
        constexpr int activeFlag = 1;
        constexpr int validFlag = 2;
        constexpr int dirtyFlag = 4;
        auto state = storage->GetInt( frameId, -1 ) == ImGui::GetFrameCount() - 1
                         ? storage->GetInt( stateId ) : 0;
        const bool wasActive = ( state & activeFlag ) != 0;
        const bool hadFocus = ImGui::GetActiveID() == inputId;
        char buffer[64];
        std::snprintf( buffer, sizeof( buffer ), "%.9g", static_cast<double>( *value ) );
        bool edited = false;
        const bool submitted = ImGui::InputText(
            "##value", buffer, sizeof( buffer ), ImGuiInputTextFlags_CharsScientific |
                ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_CallbackEdit,
            []( ImGuiInputTextCallbackData *data ) {
                *static_cast<bool *>( data->UserData ) = true;
                return 0;
            }, &edited );
        if( edited ) state |= dirtyFlag;
        const bool active = ImGui::IsItemActive();
        if( active || hadFocus || submitted )
        {
            char *end = nullptr;
            errno = 0;
            const auto draft = std::strtof( buffer, &end );
            const bool valid = end != buffer && *end == '\0' && errno != ERANGE &&
                               std::isfinite( draft );
            state = ( state & dirtyFlag ) | ( valid ? validFlag : 0 );
            if( valid ) storage->SetFloat( draftId, draft );
        }
        const bool cancelled = ( wasActive || hadFocus ) && ImGui::IsKeyPressed( ImGuiKey_Escape );
        if( !sliderChanged && !cancelled && ( submitted || ( wasActive && !active ) ) &&
            ( state & ( validFlag | dirtyFlag ) ) == ( validFlag | dirtyFlag ) )
        {
            *value = std::max( minimum, std::min( maximum, storage->GetFloat( draftId ) ) );
        }
        storage->SetInt( stateId, active ? ( state | activeFlag ) : 0 );
        storage->SetInt( frameId, ImGui::GetFrameCount() );
        ItemTooltip( "Type an exact value. Enter or Tab applies; Escape cancels." );
        if( labelWidth > 0.0f && !labelAbove )
        {
            ImGui::SameLine( 0.0f, style.ItemInnerSpacing.x );
            ImGui::TextUnformatted( label, labelEnd );
        }
        ImGui::PopID();
        ImGui::EndGroup();
        return *value != previous;
    }

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
