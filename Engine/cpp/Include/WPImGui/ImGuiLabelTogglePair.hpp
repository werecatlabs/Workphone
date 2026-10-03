#ifndef ImGuiLabelCheckboxPair_h__
#define ImGuiLabelCheckboxPair_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/Interface/UI/IUILabelTogglePair.hpp>
#include <WPImGui/ImGuiElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiLabelTogglePair : public ImGuiElement<IUILabelTogglePair>
        {
        public:
            ImGuiLabelTogglePair();
            ~ImGuiLabelTogglePair() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            String getLabel() const override;
            void setLabel( const String &label ) override;

            bool getValue() const override;
            void setValue( bool value ) override;

            bool getShowLabel() const override;

            void setShowLabel( bool showLabel ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            FixedString<1024> m_label;
            bool m_value = true;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiLabelCheckboxPair_h__
