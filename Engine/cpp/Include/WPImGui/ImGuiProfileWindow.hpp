#ifndef ImGuiProfileWindow_h__
#define ImGuiProfileWindow_h__

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIProfileWindow.hpp>
#include <chrono>

namespace workphone
{
    namespace ui
    {
        class ImGuiProfileWindow : public ImGuiElement<IUIProfileWindow>
        {
        public:
            /** Constructor. */
            ImGuiProfileWindow();

            /** Virtual destructor. */
            ~ImGuiProfileWindow() override;

            /** @copydoc CImGuiElement<IUIProfileWindow>::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc CImGuiElement<IUIProfileWindow>::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc CImGuiElement<IUIProfileWindow>::update */
            void update() override;

            SmartPtr<IProfile> getProfile() const override;

            void setProfile( SmartPtr<IProfile> profile ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_label;
            std::chrono::high_resolution_clock::time_point m_start;
            std::vector<float> m_data;

            SmartPtr<IProfile> m_profile;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiProfileWindow_h__
