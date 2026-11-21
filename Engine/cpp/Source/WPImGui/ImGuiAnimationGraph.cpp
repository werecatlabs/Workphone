#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiAnimationGraph.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiAnimationGraph, ImGuiElement<IUIAnimationGraph> );

    void ImGuiAnimationGraph::update()
    {
#if WP_BUILD_IMGUI
        if( isVisible() )
        {
            draw();
        }
#endif
    }
}  // namespace workphone::ui
