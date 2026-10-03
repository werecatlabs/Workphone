#ifndef WPImGuiStyle_h__
#define WPImGuiStyle_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>

struct ImVec4;

namespace workphone::ui
{
    /**
     * Editor-focused ImGui theme adapted from Esoterica's ImGui style.
     *
     * The palette deliberately keeps controls neutral until they are active, which makes
     * selection and state changes easier to scan in dense tools such as property grids.
     */
    class WPImGui_API WPImGuiStyle
    {
    public:
        static void apply( f32 dpiScale = 1.0f );

        static ImVec4 gray( u32 level );
        static ImVec4 accent( u32 level = 1 );

        static ImVec4 axisX();
        static ImVec4 axisY();
        static ImVec4 axisZ();
        static ImVec4 axisW();

        static constexpr f32 ToolTipDelay = 0.4f;
    };
}  // namespace workphone::ui

#endif  // WPImGuiStyle_h__
