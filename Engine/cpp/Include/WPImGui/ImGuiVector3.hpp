#ifndef ImGuiVector3_h__
#define ImGuiVector3_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIVector3.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiVector3 : public ImGuiElement<IUIVector3>
        {
        public:
            ImGuiVector3();
            ~ImGuiVector3() override;

            /** @copydoc IUIVector3::getValue */
            Vector3<real_Num> getValue() const override;

            /** @copydoc IUIVector3::setValue */
            void setValue( const Vector3<real_Num> &value ) override;

            /** @copydoc IUIVector3::getLabel */
            String getLabel() const override;

            /** @copydoc IUIVector3::setLabel */
            void setLabel( const String &label ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Vector3<real_Num> m_value;
            String m_label;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiVector3_h__
