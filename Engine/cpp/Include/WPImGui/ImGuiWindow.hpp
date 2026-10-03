#ifndef ImGuiWindow_h__
#define ImGuiWindow_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiWindowT.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>

namespace workphone
{
    namespace core
    {
        extern template class WPCore_API Prototype<ui::IUIWindow>;
    }

    namespace ui
    {
        class ImGuiWindow : public ImGuiWindowT<IUIWindow>
        {
        public:
            ImGuiWindow();
            ~ImGuiWindow() override;

            void update() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ImGuiWindow_h__
