#ifndef ImGuiCollapsingHeader_h__
#define ImGuiCollapsingHeader_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUICollapsingHeader.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiCollapsingHeader : public ImGuiElement<IUICollapsingHeader>
        {
        public:
            ImGuiCollapsingHeader();
            ~ImGuiCollapsingHeader() override;

            void update() override;

            String getLabel() const override;
            void setLabel( const String &label ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            FixedString<128> m_label;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiCollapsingHeader_h__
