#ifndef ImGuiVector2_h__
#define ImGuiVector2_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIVector2.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiVector2 : public ImGuiElement<IUIVector2>
        {
        public:
            ImGuiVector2();
            ~ImGuiVector2() override;

            Vector2<real_Num> getValue() const override;
            void setValue( const Vector2<real_Num> &value ) override;

            /** @copydoc IUIVector3::getLabel */
            String getLabel() const override;

            /** @copydoc IUIVector3::setLabel */
            void setLabel( const String &label ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Vector2<real_Num> m_value;
            String m_label;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiVector2_h__
