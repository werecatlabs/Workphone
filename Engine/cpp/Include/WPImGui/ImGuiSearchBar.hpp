#ifndef ImGuiSearchBar_h__
#define ImGuiSearchBar_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUISearchBar.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiSearchBar : public ImGuiElement<IUISearchBar>
        {
        public:
            ImGuiSearchBar();
            ~ImGuiSearchBar() override;

            void update() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiSearchBar_h__
