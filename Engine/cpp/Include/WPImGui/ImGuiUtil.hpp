#ifndef ImGuiUtil_h__
#define ImGuiUtil_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUITreeNode.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiUtil
        {
        public:
            static bool ToggleButton( const char *str_id, bool *v, f32 size = 1.0f );

            /** Draw a compact vertical separator between two same-line controls. */
            static void SameLineSeparator( f32 width = -1.0f );

            /** Show a compact tooltip after the hovered item has rested for a short delay. */
            static void ItemTooltip( const char *text );

            /** Draw an animated, clickable activity spinner. */
            static bool Spinner( const char *id, f32 size = 0.0f, f32 thickness = 2.0f,
                                 f32 padding = 1.0f );
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ImGuiUtil_h__
