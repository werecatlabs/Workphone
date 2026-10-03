#ifndef ImGuiMaskEditor_h__
#define ImGuiMaskEditor_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiMaskEditor : public ImGuiElement<IUIElement>
        {
        public:
            ImGuiMaskEditor();
            ~ImGuiMaskEditor() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            void showCollisionMaskEditor( u32 &mask );

            void showHexCollisionMaskEditor( u32 &mask );
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ImGuiMaskEditor_h__
