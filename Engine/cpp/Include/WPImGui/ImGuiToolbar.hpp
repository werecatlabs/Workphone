#ifndef ImGuiToolbar_h__
#define ImGuiToolbar_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIToolbar.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiToolbar : public ImGuiElement<IUIToolbar>
        {
        public:
            ImGuiToolbar();
            ~ImGuiToolbar() override;

            void load(SmartPtr<ISharedObject> data) override;

            void unload( SmartPtr<ISharedObject> data ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiToolbar_h__
