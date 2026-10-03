#ifndef ImGuiAbout_h__
#define ImGuiAbout_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIAbout.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiAbout : public ImGuiElement<IUIAbout>
        {
        public:
            ImGuiAbout();
            ~ImGuiAbout() override;

            void update() override;

            WP_CLASS_REGISTER_DECL;

        protected:
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ImGuiAbout_h__
