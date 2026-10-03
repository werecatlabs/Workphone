#ifndef ImGuiTabItem_h__
#define ImGuiTabItem_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUITabItem.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiTabItem : public ImGuiElement<IUITabItem>
        {
        public:
            ImGuiTabItem();
            ~ImGuiTabItem() override;

            void update() override;

            String getLabel() const override;

            void setLabel( const String &label ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_label;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiTabItem_h__
