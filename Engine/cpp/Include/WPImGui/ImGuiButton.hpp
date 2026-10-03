#ifndef ImGuiButton_h__
#define ImGuiButton_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>

namespace workphone
{
    namespace core
    {
        extern template class WPCore_API Prototype<ui::IUIButton>;
    }

    namespace ui
    {
        class ImGuiButton : public ImGuiElement<IUIButton>
        {
        public:
            ImGuiButton();
            ~ImGuiButton() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void update() override;

            String getLabel() const override;
            void setLabel( const String &label ) override;

            void setTextSize( f32 textSize ) override;
            f32 getTextSize() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            AtomicObject<FixedString<128>> m_label;
            atomic_f32 m_textSize = 14.0f;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiButton_h__
