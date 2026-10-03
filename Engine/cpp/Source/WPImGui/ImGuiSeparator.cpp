#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiSeparator.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiSeparator, ImGuiElement<IUISeparator> );

        ImGuiSeparator::ImGuiSeparator()
        {
        }

        ImGuiSeparator::~ImGuiSeparator()
        {
        }

        void ImGuiSeparator::update()
        {
            auto parent = getParent();
            if( parent && parent->isDerived<IUIToolbar>() )
            {
                // Vertical separator
                ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddLine(
                    ImVec2( cursorPos.x + 4, cursorPos.y ),                            // Start
                    ImVec2( cursorPos.x + 4, cursorPos.y + ImGui::GetFrameHeight() ),  // End
                    IM_COL32( 128, 128, 128, 255 ) );                                  // Color (gray)
                ImGui::Dummy( ImVec2( 8, 0 ) );  // Add spacing for the separator
                ImGui::SameLine();
            }
            else
            {
                ImGui::Separator();
            }
        }

        void ImGuiSeparator::setHorizontal( bool horizontal )
        {
            // Stub implementation
        }

        bool ImGuiSeparator::isHorizontal() const
        {
            return true;  // Default to horizontal
        }

        void ImGuiSeparator::setThickness( f32 thickness )
        {
            // Stub implementation
        }

        f32 ImGuiSeparator::getThickness() const
        {
            return 1.0f;  // Default thickness
        }

        void ImGuiSeparator::setMargin( f32 margin )
        {
            // Stub implementation
        }

        f32 ImGuiSeparator::getMargin() const
        {
            return 4.0f;  // Default margin
        }

        void ImGuiSeparator::setPosition( const Vector2<real_Num> &position )
        {
            // Stub implementation
        }

        void ImGuiSeparator::setSize( const Vector2<real_Num> &size )
        {
            // Stub implementation
        }
    }  // namespace ui
}  // namespace workphone
