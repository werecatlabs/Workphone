#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiAssetBrowserItems.hpp>
#include <WPImGui/ImGuiRuntimeFont.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/Interface/UI/IUITreeNode.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/UI/IconsFontAwesome6.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cfloat>

namespace workphone::ui
{
    namespace
    {
        constexpr ImGuiSelectableFlags itemFlags = ImGuiSelectableFlags_AllowDoubleClick |
            static_cast<ImGuiSelectableFlags>( ImGuiSelectableFlags_NoPadWithHalfSpacing );

        ImU32 iconColor( const String &icon, bool folder )
        {
            const auto text = ImGui::GetStyleColorVec4( ImGuiCol_Text );
            ImVec4 tint = text;
            if( folder )
                tint = ImVec4( 0.97f, 0.66f, 0.24f, 1.0f );
            else if( icon == ICON_FA_FILE_IMAGE || icon == ICON_FA_PALETTE )
                tint = ImVec4( 0.38f, 0.72f, 0.44f, 1.0f );
            else if( icon == ICON_FA_CUBE || icon == ICON_FA_CUBES )
                tint = ImVec4( 0.40f, 0.66f, 0.98f, 1.0f );
            else if( icon == ICON_FA_FILE_AUDIO || icon == ICON_FA_FILE_VIDEO )
                tint = ImVec4( 0.71f, 0.50f, 0.92f, 1.0f );
            else if( icon == ICON_FA_FILE_CODE )
                tint = ImVec4( 0.35f, 0.73f, 0.76f, 1.0f );
            return ImGui::GetColorU32( ImLerp( text, tint, 0.58f ) );
        }

        bool drawPreview( const IUITreeNode &node, const ImRect &bounds )
        {
            const auto image = workphone::dynamic_pointer_cast<IUIImage>( node.getNodeData() );
            const auto texture = image ? image->getTexture() : nullptr;
            if( !texture || !texture->isLoaded() )
                return false;

            const auto size = texture->getSize();
            if( size.x <= 0 || size.y <= 0 )
                return false;

            void *view = nullptr;
            texture->getTextureFinal( &view );
            if( !view )
                return false;

            const auto scale = std::min( bounds.GetWidth() / static_cast<float>( size.x ),
                                         bounds.GetHeight() / static_cast<float>( size.y ) );
            const auto center = bounds.GetCenter();
            const ImVec2 halfSize( size.x * scale * 0.5f, size.y * scale * 0.5f );
            retainImGuiDx11FrameTexture( view );
            retainImGuiFrameResource( texture );
            ImGui::GetWindowDrawList()->AddImageRounded(
                view, ImVec2( center.x - halfSize.x, center.y - halfSize.y ),
                ImVec2( center.x + halfSize.x, center.y + halfSize.y ),
                ImVec2( 0.0f, 0.0f ), ImVec2( 1.0f, 1.0f ), IM_COL32_WHITE, 3.0f );
            return true;
        }

        void drawIcon( const IUITreeNode &node, const ImRect &bounds, float extent )
        {
            if( bounds.GetWidth() <= 0.0f || bounds.GetHeight() <= 0.0f || drawPreview( node, bounds ) )
                return;

            const bool folder = node.getElementId() == 1;
            const auto icon = node.getLabel();
            const auto *label = icon.empty() ? ( folder ? ICON_FA_FOLDER : ICON_FA_FILE ) : icon.c_str();
            const auto color = iconColor( icon, folder );
            const auto *font = ImGui::GetFont();
            const auto center = bounds.GetCenter();
            auto *drawList = ImGui::GetWindowDrawList();

            unsigned int codepoint = 0;
            ImTextCharFromUtf8( &codepoint, label, nullptr );
            const auto *glyph = font->FindGlyphNoFallback( static_cast<ImWchar>( codepoint ) );
            if( glyph && glyph->Visible )
            {
                const auto glyphWidth = glyph->X1 - glyph->X0;
                const auto glyphHeight = glyph->Y1 - glyph->Y0;
                const auto scale = extent / std::max( glyphWidth, glyphHeight );
                drawList->AddText(
                    font, font->FontSize * scale,
                    ImVec2( center.x - ( glyph->X0 + glyph->X1 ) * scale * 0.5f,
                            center.y - ( glyph->Y0 + glyph->Y1 ) * scale * 0.5f ),
                    color, label );
            }
            else
            {
                // Keep the distinction readable in contexts without the merged icon font.
                const auto *fallback = folder ? "+" : "-";
                const auto size = font->CalcTextSizeA( extent, FLT_MAX, 0.0f, fallback );
                drawList->AddText( font, extent,
                                   ImVec2( center.x - size.x * 0.5f, center.y - size.y * 0.5f ),
                                   color, fallback );
            }
        }

        void drawName( const IUITreeNode &node, const ImRect &bounds, bool centered )
        {
            if( bounds.GetWidth() <= 0.0f )
                return;

            const auto path = node.getName();
            const auto basename = Path::getFileName( path );
            const auto &name = basename.empty() ? path : basename;
            const auto *begin = name.c_str();
            const auto *end = begin + name.size();
            const auto textSize = ImGui::CalcTextSize( begin, end );
            auto position = bounds.Min;
            position.y += std::max( 0.0f, ( bounds.GetHeight() - textSize.y ) * 0.5f );
            if( centered )
                position.x += std::max( 0.0f, ( bounds.GetWidth() - textSize.x ) * 0.5f );
            ImGui::RenderTextEllipsis( ImGui::GetWindowDrawList(), position, bounds.Max, bounds.Max.x,
                                       bounds.Max.x, begin, end, &textSize );
        }
    }

    void drawAssetBrowserTile( const IUITreeNode &node, bool selected, const ImVec2 &size )
    {
        const auto origin = ImGui::GetCursorScreenPos();
        const ImVec2 end( origin.x + size.x, origin.y + size.y );
        auto *drawList = ImGui::GetWindowDrawList();
        constexpr float rounding = 4.0f;
        if( ImGui::IsRectVisible( origin, end ) )
            drawList->AddRectFilled( origin, end, ImGui::GetColorU32( ImGuiCol_FrameBg, 0.46f ), rounding );

        ImGui::Selectable( "##asset", selected, itemFlags, size );
        if( !ImGui::IsItemVisible() )
            return;

        const ImRect bounds( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        drawList->AddRect( bounds.Min, bounds.Max,
                           ImGui::GetColorU32( selected ? ImGuiCol_HeaderActive : ImGuiCol_Border ),
                           rounding );

        const auto labelTop = bounds.Max.y - ImGui::GetTextLineHeight() - 6.0f;
        const ImRect preview( ImVec2( bounds.Min.x + 8.0f, bounds.Min.y + 7.0f ),
                              ImVec2( bounds.Max.x - 8.0f, labelTop - 5.0f ) );
        drawIcon( node, preview, std::min( preview.GetWidth(), preview.GetHeight() ) * 0.65f );
        drawName( node, ImRect( ImVec2( bounds.Min.x + 5.0f, labelTop ),
                               ImVec2( bounds.Max.x - 5.0f, bounds.Max.y - 4.0f ) ), true );
    }

    void drawAssetBrowserRow( const IUITreeNode &node, bool selected )
    {
        const auto height = ImGui::GetFrameHeight();
        ImGui::Selectable( "##asset", selected, itemFlags, ImVec2( 0.0f, height ) );
        if( !ImGui::IsItemVisible() )
            return;

        const ImRect bounds( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        const auto iconSize = ImGui::GetTextLineHeight();
        const auto iconTop = bounds.Min.y + ( height - iconSize ) * 0.5f;
        const ImRect icon( ImVec2( bounds.Min.x + 5.0f, iconTop ),
                           ImVec2( bounds.Min.x + 5.0f + iconSize, iconTop + iconSize ) );
        drawIcon( node, icon, iconSize * 0.86f );
        drawName( node, ImRect( ImVec2( icon.Max.x + 6.0f, bounds.Min.y ),
                               ImVec2( bounds.Max.x - 5.0f, bounds.Max.y ) ), false );
    }
}  // namespace workphone::ui
