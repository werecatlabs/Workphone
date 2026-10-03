#ifndef ImGuiInputManager_h__
#define ImGuiInputManager_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIInputManager.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiInputManager : public ImGuiElement<IUIInputManager>
        {
        public:
            ImGuiInputManager();
            ~ImGuiInputManager() override;

            void update() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            void CaptureInput();
            void ChangeKeyBinding( int &key );
            void ChangeJoyBinding( int &button );

            void ShowJoystickInformation( SmartPtr<IJoystick> joystick );

            std::map<u32, std::string> m_key_map;
            u32 *m_change_key = nullptr;
            bool m_changing_key = false;

            std::map<int, std::string> m_joy_map;
            int *m_change_joy = nullptr;
            bool m_changing_joy = false;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiInputManager_h__
