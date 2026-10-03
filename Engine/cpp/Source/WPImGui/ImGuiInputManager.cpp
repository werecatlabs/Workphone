#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiInputManager.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>
#include <map>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiInputManager, ImGuiElement<IUIInputManager> );

    ImGuiInputManager::ImGuiInputManager() = default;

    ImGuiInputManager::~ImGuiInputManager() = default;

    void ImGuiInputManager::CaptureInput()
    {
        //m_keyboard->capture();
        //m_joystick->capture();
        //if( m_changing_key )
        //{
        //    for( int i = 0; i < 256; i++ )
        //    {
        //        if( m_keyboard->isKeyDown( static_cast<OIS::KeyCode>( i ) ) )
        //        {
        //            *m_change_key = static_cast<OIS::KeyCode>( i );
        //            m_changing_key = false;
        //            break;
        //        }
        //    }
        //}
    }

    void ImGuiInputManager::ChangeKeyBinding( int &key )
    {
    }

    void ImGuiInputManager::ChangeJoyBinding( int &button )
    {
    }

    void ImGuiInputManager::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto inputDeviceManager = applicationManager->getInputDeviceManager();

        //ImGui::Begin( "Input Manager" );

        if( ImGui::BeginTabBar( "Tabs" ) )
        {
            // Display current input bindings
            ImGui::Text( "Current Input Bindings:" );
            ImGui::Separator();
            //for( const auto &binding : inputBindings )
            //{
            //    ImGui::Text( "%s : %s", ImGui::GetKeyName( binding.key ), binding.action );
            //}

            // Display a button to add a new input binding
            if( ImGui::Button( "Add Input Binding" ) )
            {
                // Open a popup window to select a key and action
                ImGui::OpenPopup( "Add Input Binding Popup" );
            }

            // Popup for adding a new input binding
            if( ImGui::BeginPopup( "Add Input Binding Popup" ) )
            {
                static int selectedKey = 0;
                static char actionBuffer[64] = "";

                // Display a combo box to select a key
                ImGui::Text( "Select Key:" );
                ImGui::Combo( "##keyCombo", &selectedKey, ImGui::GetKeyName( -1 ),
                              IM_ARRAYSIZE( ImGui::GetKeyName( -1 ) ) );

                // Input field for entering action name
                ImGui::InputText( "Action", actionBuffer, IM_ARRAYSIZE( actionBuffer ) );

                // Button to confirm adding the input binding
                if( ImGui::Button( "Add" ) )
                {
                    if( actionBuffer[0] != '\0' )
                    {
                        //inputBindings.push_back({selectedKey, actionBuffer});
                    }
                    // Close the popup window
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            if( ImGui::BeginTabItem( "Bindings" ) )
            {
                // Key bindings
                ImGui::Text( "Key Bindings" );
                for( auto &[key, name] : m_key_map )
                {
                    ImGui::PushID( &key );
                    ImGui::Text( "%s:", name.c_str() );
                    ImGui::SameLine();
                    if( ImGui::Button( "Change" ) )
                    {
                        //    ChangeKeyBinding( key );
                    }

                    ImGui::PopID();
                }

                // Joystick bindings
                ImGui::Text( "Joystick Bindings" );
                for( auto &[button, name] : m_joy_map )
                {
                    ImGui::PushID( &button );
                    ImGui::Text( "%s:", name.c_str() );
                    ImGui::SameLine();
                    if( ImGui::Button( "Change" ) )
                    {
                        //ChangeJoyBinding( button );
                    }
                    ImGui::PopID();
                }

                ImGui::EndTabItem();
            }

            if( ImGui::BeginTabItem( "JoyStick" ) )
            {
                auto joysticks = inputDeviceManager->getJoysticks();
                for( auto joystick : joysticks )
                {
                    ShowJoystickInformation( joystick );
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        //ImGui::End();
    }

    void ImGuiInputManager::ShowJoystickInformation( SmartPtr<IJoystick> joystick )
    {
        //ImGui::Begin( "Joystick Information" );

        auto name = joystick->getName();
        auto numAxes = joystick->getNumAxes();
        auto numButtons = joystick->getNumButtons();

        ImGui::Text( "Name: %s", name.c_str() );
        ImGui::Text( "Axes: %d", numAxes );
        ImGui::Text( "Buttons: %d", numButtons );

        ImGui::Text( "Axis Values:" );
        for( int i = 0; i < numAxes; ++i )
        {
            auto axisValue = joystick->getAxis( i );
            ImGui::Text( "  %d: %.2f", i, axisValue );
        }

        ImGui::Text( "Button States:" );
        for( int i = 0; i < numButtons; ++i )
        {
            auto buttonState = joystick->isButtonDown( i );
            ImGui::Text( "  %d: %s", i, buttonState ? "Pressed" : "Released" );
        }

        //ImGui::End();
    }
}  // namespace workphone::ui
