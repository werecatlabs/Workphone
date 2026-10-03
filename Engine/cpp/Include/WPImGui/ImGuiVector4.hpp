#ifndef ImGuiVector4_h__
#define ImGuiVector4_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIVector4.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiVector4 : public ImGuiElement<IUIVector4>
        {
        public:
            ImGuiVector4();
            ~ImGuiVector4() override;

            Vector4<real_Num> getValue() const override;
            void setValue( const Vector4<real_Num> &value ) override;

            /** @copydoc IUIVector3::getLabel */
            String getLabel() const override;

            /** @copydoc IUIVector3::setLabel */
            void setLabel( const String &label ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Vector4<real_Num> m_value;
            String m_label;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiVector4_h__
