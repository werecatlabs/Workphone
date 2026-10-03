#ifndef ImGuiProfilerWindow_h__
#define ImGuiProfilerWindow_h__

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIProfilerWindow.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiProfilerWindow : public ImGuiElement<IUIProfilerWindow>
        {
        public:
            /** Constructor. */
            ImGuiProfilerWindow();

            /** Virtual destructor. */
            ~ImGuiProfilerWindow() override;

            /** @copydoc CImGuiElement<IUIProfilerWindow>::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc CImGuiElement<IUIProfilerWindow>::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc CImGuiElement<IUIProfilerWindow>::update */
            void update() override;

            SmartPtr<IUIProfileWindow> addProfile() override;

            void removeProfile( SmartPtr<IUIProfileWindow> profile ) override;

            Array<SmartPtr<IUIProfileWindow>> getProfiles() const override;

            void setProfiles( const Array<SmartPtr<IUIProfileWindow>> &profiles ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<SmartPtr<IUIProfileWindow>> m_profiles;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiProfilerWindow_h__
