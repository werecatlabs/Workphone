#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiText.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>
#include "imgui_internal.h"

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiText, ImGuiElement<IUIText> );

    ImGuiText::ImGuiText() = default;

    ImGuiText::~ImGuiText()
    {
        unload( nullptr );
    }

    void ImGuiText::update()
    {
        if( !m_longText.empty() )
        {
            // Apply text size if different from default
            if( m_textSize != 10.0f )
            {
                auto window = ImGui::GetCurrentWindow();
                auto originalFontSize = window->FontWindowScale;

                // Calculate scale factor based on text size
                auto fontScale = m_textSize / ImGui::GetFontSize();
                ImGui::SetWindowFontScale( fontScale );

                renderTextWithAlignment( m_longText );

                // Restore original font scale
                ImGui::SetWindowFontScale( originalFontSize );
            }
            else
            {
                renderTextWithAlignment( m_longText );
            }
        }
        else if( !m_text.empty() )
        {
            // Apply text size if different from default
            if( m_textSize != 10.0f )
            {
                auto window = ImGui::GetCurrentWindow();
                auto originalFontSize = window->FontWindowScale;

                // Calculate scale factor based on text size
                auto fontScale = m_textSize / ImGui::GetFontSize();
                ImGui::SetWindowFontScale( fontScale );

                renderTextWithAlignment( m_text );

                // Restore original font scale
                ImGui::SetWindowFontScale( originalFontSize );
            }
            else
            {
                renderTextWithAlignment( m_text );
            }
        }
    }

    void ImGuiText::setText( const String &text )
    {
        if( text.length() > m_text.capacity() )
        {
            m_longText = text;
        }
        else
        {
            m_text = text;
        }
    }

    void ImGuiText::setTextSize( f32 textSize )
    {
        m_textSize = textSize;
    }

    f32 ImGuiText::getTextSize() const
    {
        return m_textSize;
    }

    void ImGuiText::setVerticalAlignment( u8 alignment )
    {
        m_verticalAlignment = alignment;
    }

    u8 ImGuiText::getVerticalAlignment() const
    {
        return m_verticalAlignment;
    }

    void ImGuiText::setHorizontalAlignment( u8 alignment )
    {
        m_horizontalAlignment = alignment;
    }

    u8 ImGuiText::getHorizontalAlignment() const
    {
        return m_horizontalAlignment;
    }

    String ImGuiText::getText() const
    {
        if( !m_longText.empty() )
        {
            return m_longText;
        }

        return m_text;
    }

    const c8 *ImGuiText::getTextPtr() const
    {
        return m_text.c_str();
    }

    void ImGuiText::renderTextWithAlignment( const String &text )
    {
        if( m_horizontalAlignment == static_cast<u8>( HorizontalAlignment::LEFT ) &&
            m_verticalAlignment == static_cast<u8>( VerticalAlignment::TOP ) )
        {
            // Default alignment - no special handling needed
            ImGui::Text( "%s", text.c_str() );
            return;
        }

        // Get available space and text size for alignment calculations
        auto availableSize = ImGui::GetContentRegionAvail();
        auto textSize = ImGui::CalcTextSize( text.c_str() );

        // Calculate horizontal offset
        f32 horizontalOffset = 0.0f;
        switch( static_cast<HorizontalAlignment>( m_horizontalAlignment ) )
        {
        case HorizontalAlignment::CENTER:
            horizontalOffset = ( availableSize.x - textSize.x ) * 0.5f;
            break;
        case HorizontalAlignment::RIGHT:
            horizontalOffset = availableSize.x - textSize.x;
            break;
        case HorizontalAlignment::LEFT:
        default:
            horizontalOffset = 0.0f;
            break;
        }

        // Calculate vertical offset
        f32 verticalOffset = 0.0f;
        switch( static_cast<VerticalAlignment>( m_verticalAlignment ) )
        {
        case VerticalAlignment::CENTER:
            verticalOffset = ( availableSize.y - textSize.y ) * 0.5f;
            break;
        case VerticalAlignment::BOTTOM:
            verticalOffset = availableSize.y - textSize.y;
            break;
        case VerticalAlignment::TOP:
        default:
            verticalOffset = 0.0f;
            break;
        }

        // Ensure offsets are not negative
        horizontalOffset = MathF::max( 0.0f, horizontalOffset );
        verticalOffset = MathF::max( 0.0f, verticalOffset );

        // Apply cursor positioning for alignment
        if( horizontalOffset > 0.0f )
        {
            ImGui::SetCursorPosX( ImGui::GetCursorPosX() + horizontalOffset );
        }

        if( verticalOffset > 0.0f )
        {
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + verticalOffset );
        }

        // Render the text
        ImGui::Text( "%s", text.c_str() );
    }
}  // namespace workphone::ui
