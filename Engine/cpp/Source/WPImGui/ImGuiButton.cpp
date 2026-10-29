#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiButton.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/ProjectLocalization.hpp>
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#if defined( _WIN32 )
#    include <Windows.h>
#endif

namespace workphone::ui
{
    namespace
    {
        constexpr u32 shortcutShift = 0x10000;
        constexpr u32 observedEnabled = 1, observedVisible = 2, observedFocused = 4;
        constexpr u32 observedCtrl = 8, observedAlt = 16, observedShift = 32, observedSuper = 64;
        constexpr u32 observedKeyPress = 128;

        ImGuiButton::ShortcutObservation observation( u64 value )
        {
            ImGuiButton::ShortcutObservation result;
            result.frame = value ? static_cast<int>( ( value >> 32 ) - 1 ) : -1;
            result.enabled = ( value & observedEnabled ) != 0;
            result.itemVisible = ( value & observedVisible ) != 0;
            result.applicationFocused = ( value & observedFocused ) != 0;
            result.ctrl = ( value & observedCtrl ) != 0;
            result.alt = ( value & observedAlt ) != 0;
            result.shift = ( value & observedShift ) != 0;
            result.super = ( value & observedSuper ) != 0;
            result.keyPressed = ( value & observedKeyPress ) != 0;
            return result;
        }

        u32 parseShortcut( String value )
        {
            value.erase( std::remove_if( value.begin(), value.end(),
                []( unsigned char c ) { return std::isspace( c ) != 0; } ), value.end() );
            std::transform( value.begin(), value.end(), value.begin(),
                []( unsigned char c ) { return static_cast<char>( std::toupper( c ) ); } );
            if( value.empty() ) return 0;
            if( value == "F5" ) return ImGuiKey_F5;
            if( value == "F8" ) return ImGuiKey_F8;
            if( value == "F10" ) return ImGuiKey_F10;
            if( value == "F11" ) return ImGuiKey_F11;
            if( value == "SHIFT+F11" ) return ImGuiKey_F11 | shortcutShift;
            throw std::invalid_argument( "Unsupported button keyboardShortcut; use F5, F8, F10, F11 or Shift+F11." );
        }

        String shortcutName( u32 shortcut )
        {
            switch( shortcut )
            {
            case ImGuiKey_F5: return "F5";
            case ImGuiKey_F8: return "F8";
            case ImGuiKey_F10: return "F10";
            case ImGuiKey_F11: return "F11";
            case ImGuiKey_F11 | shortcutShift: return "Shift+F11";
            default: return "";
            }
        }

        bool applicationFocused()
        {
#if defined( _WIN32 )
            // Real Win32 backends may receive input from shared device polling.
            // Do not activate the Editor while another process owns focus.
            if( ImGui::GetIO().BackendPlatformUserData )
            {
                DWORD owner = 0;
                const auto foreground = GetForegroundWindow();
                if( foreground ) GetWindowThreadProcessId( foreground, &owner );
                return foreground && owner == GetCurrentProcessId();
            }
#endif
            // Headless contexts use explicit ImGui input events; a focus-loss
            // event clears the key state before the frame is evaluated.
            return !ImGui::GetIO().AppFocusLost;
        }
    }

    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiButton, ImGuiElement<IUIButton> );

    ImGuiButton::ImGuiButton() = default;

    ImGuiButton::~ImGuiButton() = default;

    void ImGuiButton::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiButton::unload( SmartPtr<ISharedObject> data )
    {
        ImGuiElement<IUIButton>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void ImGuiButton::update()
    {
        if( !isVisible() ) return;
        const auto sourceLabel = getLabel();
        auto label = localizeProjectText( sourceLabel );
        if( sourceLabel.size() >= 4 && sourceLabel[0] == '@' && sourceLabel[1] == '{' && sourceLabel.back() == '}' )
            label += "###" + sourceLabel;
        if( StringUtil::isNullOrEmpty( label ) )
        {
            label = "Untitled";
        }

        auto enabled = isEnabled();
        ImGui::BeginDisabled( !enabled );
        auto pressed = ImGui::Button( label.c_str() );
        ImGui::EndDisabled();

        const u32 shortcut = m_keyboardShortcut;
        const auto &io = ImGui::GetIO();
        const bool focused = shortcut && applicationFocused();
        const bool keyPressed = shortcut && ImGui::IsKeyPressed( static_cast<ImGuiKey>( shortcut & 0xffff ), false );
        const auto frame = ImGui::GetFrameCount();
        const u64 observed = ( static_cast<u64>( static_cast<u32>( frame ) + 1 ) << 32 ) |
            ( enabled ? observedEnabled : 0 ) | ( ImGui::IsItemVisible() ? observedVisible : 0 ) |
            ( focused ? observedFocused : 0 ) | ( io.KeyCtrl ? observedCtrl : 0 ) |
            ( io.KeyAlt ? observedAlt : 0 ) | ( io.KeyShift ? observedShift : 0 ) |
            ( io.KeySuper ? observedSuper : 0 ) | ( keyPressed ? observedKeyPress : 0 );
        m_shortcutObservation = observed;
        if( keyPressed && observation( m_shortcutKeyPressObservation ).frame != frame )
        {
            m_shortcutKeyPressObservation = observed;
            ++m_shortcutKeyPressCount;
        }
        if( enabled && shortcut && !io.KeyCtrl && !io.KeyAlt && !io.KeySuper &&
            io.KeyShift == ( ( shortcut & shortcutShift ) != 0 ) && focused &&
            m_shortcutFrame != frame && keyPressed )
        {
            pressed = true;
            m_shortcutFrame = frame;
        }

        if( pressed && enabled )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();

            if( auto parent = getParent() )
            {
                ++m_activationCount;
                if( parent->isDerived<IUIToolbar>() )
                {
                    auto toolbar = workphone::static_pointer_cast<IUIToolbar>( parent );
                    if( toolbar )
                    {
                        auto args = Array<Parameter>();
                        applicationManager->triggerEvent( EventType::UI, IEvent::handleSelection,
                                                          args, toolbar, this, nullptr, false,
                                                          Thread::Application_Flag );
                    }
                }
                else
                {
                    auto args = Array<Parameter>();
                    applicationManager->triggerEvent( EventType::UI, IEvent::handleSelection, args,
                                                      this, nullptr, nullptr, false,
                                                      Thread::Application_Flag );
                }
            }
        }
    }

    String ImGuiButton::getLabel() const
    {
        auto str = m_label.load();
        return str.c_str();
    }

    void ImGuiButton::setLabel( const String &label )
    {
        m_label = label.c_str();
    }

    void ImGuiButton::setTextSize( f32 textSize )
    {
        m_textSize = textSize;
    }

    f32 ImGuiButton::getTextSize() const
    {
        return m_textSize;
    }

    SmartPtr<Properties> ImGuiButton::getProperties() const
    {
        auto properties = ImGuiElement<IUIButton>::getProperties();
        properties->setProperty( "keyboardShortcut", shortcutName( m_keyboardShortcut ) );
        return properties;
    }

    void ImGuiButton::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties ) return;
        String value;
        const bool hasShortcut = properties->getPropertyValue( "keyboardShortcut", value );
        const auto parsed = hasShortcut ? parseShortcut( value ) : static_cast<u32>( m_keyboardShortcut );
        ImGuiElement<IUIButton>::setProperties( properties );
        if( hasShortcut ) m_keyboardShortcut = parsed;
    }

    ImGuiButton::KeyboardShortcutSnapshot ImGuiButton::getKeyboardShortcutSnapshot() const
    {
        KeyboardShortcutSnapshot result;
        result.keyboardShortcut = shortcutName( m_keyboardShortcut );
        result.lastRendered = observation( m_shortcutObservation );
        result.lastKeyPress = observation( m_shortcutKeyPressObservation );
        result.keyPressCount = m_shortcutKeyPressCount;
        result.activationCount = m_activationCount;
        return result;
    }
}  // namespace workphone::ui
