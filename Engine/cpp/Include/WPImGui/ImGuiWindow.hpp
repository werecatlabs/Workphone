#ifndef ImGuiWindow_h__
#define ImGuiWindow_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiWindowT.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>

namespace workphone
{
    namespace core
    {
        extern template class WPCore_API Prototype<ui::IUIWindow>;
    }

    namespace ui
    {
        class ImGuiWindow : public ImGuiWindowT<IUIWindow>
        {
        public:
            ImGuiWindow();
            ~ImGuiWindow() override;

            void load( SmartPtr<ISharedObject> data ) override;

            void update() override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Apply first-use floating placement before Begin; saved layouts take precedence.
             * Returns true when this window opts out of the ordinary default dock.
             */
            bool applyInitialPlacement() const;

            WP_CLASS_REGISTER_DECL;

        private:
            atomic_bool m_initiallyFloating = false;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ImGuiWindow_h__
