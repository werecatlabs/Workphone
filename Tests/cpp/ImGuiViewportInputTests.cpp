#include <WPImGui/ImGuiApplication.hpp>
#include <WPImGui/ImGuiButton.hpp>
#include <WPImGui/ImGuiManager.hpp>
#include <WPImGui/ImGuiRenderWindow.hpp>
#include <WPImGui/ImGuiText.hpp>
#include <WPImGui/ImGuiWindow.hpp>
#include <WPImGui/WPImGuiStyle.hpp>
#include <Workphone/Core/Properties.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Input/InputEvent.hpp>
#include <Workphone/Input/MouseState.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <Workphone/Jobs/EventJob.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <WorkphoneCore/workphone.h>
#include <imgui.h>
#if defined WP_PLATFORM_WIN32
#    include <WPGraphics/ClawWin32ImGuiMessages.hpp>
#    include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND, UINT, WPARAM, LPARAM );
#endif
#include <cmath>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <cstring>

using namespace workphone;

namespace
{
    unsigned assertions = 0;

    void require( bool condition, const char *message )
    {
        ++assertions;
        if( !condition ) throw std::runtime_error( message );
    }

    bool near( float actual, float expected ) { return std::abs( actual - expected ) < .001f; }

#if defined WP_PLATFORM_WIN32
    struct NativeShortcutMessages
    {
        unsigned menuRequests = 0, consumedMenus = 0, unhandledMenus = 0, progress = 0;
    };

    LRESULT CALLBACK nativeShortcutWindowProc( HWND hwnd, UINT message, WPARAM command, LPARAM data )
    {
        if( message == WM_NCCREATE )
        {
            const auto create = reinterpret_cast<CREATESTRUCTW *>( data );
            SetWindowLongPtrW( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( create->lpCreateParams ) );
        }
        auto state = reinterpret_cast<NativeShortcutMessages *>( GetWindowLongPtrW( hwnd, GWLP_USERDATA ) );
        const bool active = ImGui::GetCurrentContext() && ImGui::GetIO().BackendPlatformUserData;
        const auto handled = active ? ImGui_ImplWin32_WndProcHandler( hwnd, message, command, data ) : 0;
        const bool consume = render::detail::consumeImGuiSystemMenuMessage( message, command, active );
        if( state && message == WM_SYSCOMMAND && ( command & 0xfff0 ) == SC_KEYMENU )
        {
            ++state->menuRequests;
            if( consume ) ++state->consumedMenus;
            else ++state->unhandledMenus;
            // Failure-only safety net: record the missing production handling
            // instead of letting a regression hang the suite in a modal menu.
            return 0;
        }
        if( handled || consume ) return 0;
        if( state && message == WM_APP + 91 ) { ++state->progress; return 0; }
        return DefWindowProcW( hwnd, message, command, data );
    }

    void nativeSystemMenuShortcuts()
    {
        struct NativeWindow
        {
            NativeWindow()
            {
                previous = ImGui::GetCurrentContext();
                context = ImGui::CreateContext();
                ImGui::SetCurrentContext( context );
                auto &io = ImGui::GetIO();
                io.IniFilename = nullptr;
                io.DisplaySize = ImVec2( 400, 300 );
                io.DeltaTime = 1.0f / 60;
                unsigned char *pixels; int width, height;
                io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );
                WNDCLASSW windowClass{};
                windowClass.lpfnWndProc = nativeShortcutWindowProc;
                windowClass.hInstance = GetModuleHandleW( nullptr );
                windowClass.lpszClassName = L"Workphone.ImGuiShortcut.Tests";
                atom = RegisterClassW( &windowClass );
                if( atom ) hwnd = CreateWindowExW( 0, windowClass.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                    0, 0, 400, 300, nullptr, nullptr, windowClass.hInstance, &messages );
                if( hwnd ) backend = ImGui_ImplWin32_Init( hwnd );
            }
            ~NativeWindow()
            {
                ImGui::SetCurrentContext( context );
                if( hwnd && IsWindow( hwnd ) ) DestroyWindow( hwnd );
                if( backend ) ImGui_ImplWin32_Shutdown();
                if( atom ) UnregisterClassW( L"Workphone.ImGuiShortcut.Tests", GetModuleHandleW( nullptr ) );
                ImGui::DestroyContext( context );
                ImGui::SetCurrentContext( previous );
            }
            NativeShortcutMessages messages;
            ImGuiContext *previous = nullptr, *context = nullptr;
            HWND hwnd = nullptr;
            ATOM atom = 0;
            bool backend = false;
        } window;
        require( window.hwnd && window.backend, "native system-key fixture must initialize a hidden owned HWND and real ImGui backend" );
        require( !IsWindowVisible( window.hwnd ), "native message regression must not activate or show a desktop window" );
        const auto keyData = []( UINT key, bool released ) {
            return static_cast<LPARAM>( 1u | ( MapVirtualKeyW( key, MAPVK_VK_TO_VSC ) << 16 ) |
                                        ( released ? 0xc0000000u : 0u ) );
        };
        // DefWindowProc handles the actual F10 transition and synchronously
        // generates SC_KEYMENU; do not substitute a direct policy-only test.
        SendMessageW( window.hwnd, WM_SYSKEYDOWN, VK_F10, keyData( VK_F10, false ) );
        ImGui::NewFrame();
        require( ImGui::IsKeyPressed( ImGuiKey_F10, false ), "native F10 system-key down must reach the ImGui input queue" );
        ImGui::EndFrame();
        SendMessageW( window.hwnd, WM_SYSKEYUP, VK_F10, keyData( VK_F10, true ) );
        require( window.messages.menuRequests > 0 && window.messages.consumedMenus == window.messages.menuRequests &&
                     window.messages.unhandledMenus == 0,
                 "Win32-generated F10 menu activation must be consumed by the production ImGui policy" );
        require( PostMessageW( window.hwnd, WM_KEYDOWN, VK_F8, keyData( VK_F8, false ) ) &&
                     PostMessageW( window.hwnd, WM_APP + 91, 0, 0 ), "native follow-up messages must be queued" );
        MSG message{};
        for( unsigned count = 0; count < 128 && PeekMessageW( &message, window.hwnd, 0, 0, PM_REMOVE ); ++count )
        {
            TranslateMessage( &message ); DispatchMessageW( &message );
        }
        require( window.messages.progress == 1, "F10 must leave the native pump able to dispatch subsequent queued work" );
        ImGui::NewFrame();
        require( !ImGui::IsKeyDown( ImGuiKey_F10 ) && ImGui::IsKeyPressed( ImGuiKey_F8, false ),
                 "F10 release and subsequent F8 input must survive the native message pump" );
        ImGui::EndFrame();
        const auto before = window.messages.unhandledMenus;
        ImGui::SetCurrentContext( nullptr );
        SendMessageW( window.hwnd, WM_SYSCOMMAND, SC_KEYMENU, 0 );
        ImGui::SetCurrentContext( window.context );
        require( window.messages.unhandledMenus == before + 1,
                 "a window without an active ImGui platform backend must preserve native menu handling" );
        require( !render::detail::consumeImGuiSystemMenuMessage( WM_SYSCOMMAND, SC_CLOSE, true ),
                 "the ImGui policy must not consume Alt+F4's native SC_CLOSE command" );
        SendMessageW( window.hwnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
        require( !IsWindow( window.hwnd ), "the native close command must still close the owned test window" );
    }
#endif

    class Texture : public render::Texture
    {
    public:
        Vector2I getSize() const override { return size; }
        void setSize( const Vector2I &value ) override { size = value; }
        void _getObject( void **output ) const override { *output = handle; }
        Vector2I size;
        void *handle = nullptr;
    };

    struct Fixture
    {
        Fixture() : types( std::make_unique<TypeManager>() ), previousTask( Thread::getCurrentTask() )
        {
            types->load();
            TypeManager::setInstance( types.get() );
            application = make_ptr<core::ApplicationManager>();
            core::IApplicationManager::setInstance( application );
            application->setFactoryManager( make_ptr<FactoryManager>() );
            application->getFactoryManager()->load( nullptr );
            Thread::setCurrentTask( TaskId::Render );
            ImGui::CreateContext();
            auto &io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.DisplaySize = ImVec2( 2048, 1536 );
            io.DisplayFramebufferScale = ImVec2( 1, 1 );
            io.DeltaTime = 1.0f / 60;
            unsigned char *pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );
            // Child attachment and event dispatch use the application's UI
            // manager lock, just as widgets created by the Editor do.
            auto uiManager = make_ptr<ui::ImGuiManager>();
            application->setUI( uiManager );
            uiManager->load( nullptr );
            require( uiManager->isLoaded(), "native UI fixture must load the real ImGui manager" );
        }

        ~Fixture()
        {
            if( auto ui = application->getUI() ) ui->unload( nullptr );
            application->setUI( nullptr );
            ImGui::DestroyContext();
            application->setSceneRenderWindow( nullptr );
            if( auto factories = application->getFactoryManager() ) factories->unload( nullptr );
            application->setFactoryManager( nullptr );
            core::IApplicationManager::setInstance( nullptr );
            application = nullptr;
            Thread::setCurrentTask( previousTask );
            TypeManager::setInstance( nullptr );
            types->unload();
        }

        std::unique_ptr<TypeManager> types;
        SmartPtr<core::ApplicationManager> application;
        TaskId previousTask;
    };

    void mouse( ui::ClawUIManager &manager, Vector2F position, IMouseState::Event type )
    {
        auto event = make_ptr<InputEvent>();
        event->setEventType( IInputEvent::EventType::Mouse );
        auto state = make_ptr<MouseState>();
        state->setAbsolutePosition( position );
        state->setEventType( type );
        event->setMouseState( state );
        manager.OnEvent( event );
    }

    void textReplacement()
    {
        auto widget=make_ptr<ui::ImGuiText>();
        const String diagnostic(256,'A');
        const String recovered="Shader generation 2 active.";
        const auto render=[&](const String &expected) {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(0,0),ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(1900,300),ImGuiCond_Always);
            ImGui::Begin("Material shader status",nullptr,ImGuiWindowFlags_NoSavedSettings);
            const auto vertices=ImGui::GetWindowDrawList()->VtxBuffer.Size;
            widget->update();
            if(expected.empty())
                require(ImGui::GetWindowDrawList()->VtxBuffer.Size==vertices,"cleared text submits no stale glyphs");
            else
                require(near(ImGui::GetItemRectSize().x,ImGui::CalcTextSize(expected.c_str()).x),
                    "actual text rendering uses the current replacement rather than retired diagnostics");
            ImGui::End();ImGui::EndFrame();
        };
        for(const auto &value:{diagnostic,recovered,String(127,'B'),String(128,'C'),String()})
        {
            widget->setText(value);
            require(widget->getText()==value,"text getters follow long, short, boundary and empty replacements");
            require(std::strcmp(widget->getTextPtr(),value.c_str())==0,"borrowed text pointer matches the displayed representation");
            render(value);
        }
    }

    void buttonKeyboardAccelerators( Fixture &fixture )
    {
        class Selections : public IEventListener
        {
        public:
            Parameter handleEvent( EventType type, hash_type value, const Array<Parameter> &,
                SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject>, SmartPtr<IEvent> ) override
            {
                if( type == EventType::UI && value == IEvent::handleSelection )
                {
                    ++count;
                    renderOrigin = renderOrigin && Thread::getCurrentTask() == TaskId::Render;
                    correctSender = correctSender && sender.get() == expected;
                }
                return {};
            }
            ISharedObject *expected = nullptr;
            unsigned count = 0;
            bool renderOrigin = true, correctSender = true;
        };
        auto factories = fixture.application->getFactoryManager();
        FactoryUtil::addFactory<EventJob>( factories );
        auto parent = make_ptr<ui::ImGuiWindow>();
        auto button = make_ptr<ui::ImGuiButton>();
        auto selections = make_ptr<Selections>();
        selections->expected = button.get();
        button->setLabel( "Step Over" );
        button->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        button->addObjectListener( selections );
        parent->addChild( button );
        auto &io = ImGui::GetIO();
        const bool trickle = io.ConfigInputTrickleEventQueue;
        io.ConfigInputTrickleEventQueue = false;
        ImVec2 mouseTarget;
        bool clipButton = false;
        const auto render = [&] {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos( ImVec2( 20, 20 ), ImGuiCond_Always );
            ImGui::SetNextWindowSize( ImVec2( 500, 250 ), ImGuiCond_Always );
            ImGui::SetNextWindowFocus();
            ImGui::Begin( "Debugger button input", nullptr, ImGuiWindowFlags_NoSavedSettings );
            if( clipButton ) ImGui::SetCursorScreenPos( ImVec2( 1500, 1500 ) );
            button->update();
            const auto minimum = ImGui::GetItemRectMin(), maximum = ImGui::GetItemRectMax();
            mouseTarget = ImVec2( ( minimum.x + maximum.x ) * .5f, ( minimum.y + maximum.y ) * .5f );
            ImGui::End(); ImGui::EndFrame();
        };
        const auto shortcut = [&]( const String &value ) {
            auto properties = make_ptr<Properties>();
            properties->setProperty( "keyboardShortcut", value );
            button->setProperties( properties );
            String published;
            require( button->getProperties()->getPropertyValue( "keyboardShortcut", published ) && published == value,
                     "button shortcuts must round-trip through the shared properties API" );
        };
        require( button->getKeyboardShortcutSnapshot().lastRendered.frame == -1 &&
                     button->getKeyboardShortcutSnapshot().activationCount == 0,
                 "unrendered button diagnostics must not imply a native frame or activation" );
        shortcut( "F10" ); render();
        auto diagnostic = button->getKeyboardShortcutSnapshot();
        require( diagnostic.keyboardShortcut == "F10" && diagnostic.lastRendered.frame == ImGui::GetFrameCount() &&
                     diagnostic.lastRendered.enabled && diagnostic.lastRendered.itemVisible &&
                     diagnostic.lastRendered.applicationFocused && !diagnostic.lastRendered.keyPressed && diagnostic.keyPressCount == 0,
                 "native diagnostics must describe the actual rendered, configured and focused button" );
        auto partial = make_ptr<Properties>(); partial->setProperty( "unrelated", true );
        button->setProperties( partial );
        io.AddKeyEvent( ImGuiKey_F10, true ); render();
        require( selections->count == 1, "F10 must activate the actual native button event path" );
        diagnostic = button->getKeyboardShortcutSnapshot();
        require( diagnostic.lastRendered.keyPressed && diagnostic.lastKeyPress.frame == ImGui::GetFrameCount() &&
                     diagnostic.keyPressCount == 1 && diagnostic.activationCount == selections->count &&
                     !diagnostic.lastKeyPress.ctrl && !diagnostic.lastKeyPress.alt &&
                     !diagnostic.lastKeyPress.shift && !diagnostic.lastKeyPress.super,
                 "key-edge diagnostics must retain the actual modifiers and activation evidence" );
        for( int frame = 0; frame < 45; ++frame ) render();
        require( ImGui::IsKeyDown( ImGuiKey_F10 ) && 45 * io.DeltaTime > io.KeyRepeatDelay && selections->count == 1,
                 "holding a debugger shortcut beyond native repeat delay must not trigger repeated steps" );
        diagnostic = button->getKeyboardShortcutSnapshot();
        require( !diagnostic.lastRendered.keyPressed && diagnostic.keyPressCount == 1 &&
                     diagnostic.lastKeyPress.frame < diagnostic.lastRendered.frame,
                 "diagnostics must retain a brief input edge across later frames without counting repeats" );
        io.AddKeyEvent( ImGuiKey_F10, false ); render();
        button->setEnabled( false, false );
        io.AddKeyEvent( ImGuiKey_F10, true ); render();
        diagnostic = button->getKeyboardShortcutSnapshot();
        require( !diagnostic.lastKeyPress.enabled && diagnostic.lastKeyPress.keyPressed && diagnostic.activationCount == 1,
                 "disabled input must remain observable without generating an activation" );
        button->setEnabled( true, false ); render();
        require( selections->count == 1, "enabling a button while its key is held must not activate it" );
        io.AddKeyEvent( ImGuiKey_F10, false ); render();
        io.AddKeyEvent( ImGuiKey_F10, true ); render();
        require( selections->count == 2, "releasing and pressing F10 must permit the next step" );
        io.AddKeyEvent( ImGuiKey_F10, false ); render();
        io.AddKeyEvent( ImGuiKey_ModCtrl, true ); io.AddKeyEvent( ImGuiKey_F10, true ); render();
        require( selections->count == 2, "Ctrl+F10 must not be treated as the unmodified F10 shortcut" );
        require( button->getKeyboardShortcutSnapshot().lastKeyPress.ctrl &&
                     button->getKeyboardShortcutSnapshot().activationCount == 2,
                 "rejected modifier combinations must be observable without changing dispatch" );
        io.AddKeyEvent( ImGuiKey_F10, false ); io.AddKeyEvent( ImGuiKey_ModCtrl, false ); render();
        shortcut( "Shift+F11" );
        io.AddKeyEvent( ImGuiKey_F11, true ); render();
        require( selections->count == 2, "plain F11 must not activate Shift+F11" );
        io.AddKeyEvent( ImGuiKey_F11, false ); render();
        io.AddKeyEvent( ImGuiKey_ModShift, true ); io.AddKeyEvent( ImGuiKey_F11, true ); render();
        require( selections->count == 3, "Shift+F11 must reach the same native selection listener" );
        require( button->getKeyboardShortcutSnapshot().lastKeyPress.shift &&
                     button->getKeyboardShortcutSnapshot().keyboardShortcut == "Shift+F11",
                 "shifted shortcut diagnostics must reflect the configured modifier and native edge" );
        io.AddKeyEvent( ImGuiKey_ModShift, false ); render();
        shortcut( "F11" ); render();
        require( selections->count == 3, "a modifier or shortcut configuration change must not retrigger a held key" );
        io.AddKeyEvent( ImGuiKey_F11, false ); render();
        io.AddKeyEvent( ImGuiKey_F11, true ); render();
        require( selections->count == 4, "plain F11 must activate after release" );
        io.AddKeyEvent( ImGuiKey_F11, false ); render();
        for( const auto key : { ImGuiKey_F5, ImGuiKey_F8 } )
        {
            const auto before = selections->count;
            shortcut( key == ImGuiKey_F5 ? "F5" : "F8" );
            io.AddKeyEvent( key, true ); render();
            require( selections->count == before + 1, "F5/F8 must dispatch through the rendered native button" );
            io.AddKeyEvent( key, false ); render();
        }
        shortcut( "F10" );
        const auto beforeHidden = selections->count;
        const auto frameBeforeHidden = button->getKeyboardShortcutSnapshot().lastRendered.frame;
        button->setVisible( false, false );
        io.AddKeyEvent( ImGuiKey_F10, true ); render();
        require( button->getKeyboardShortcutSnapshot().lastRendered.frame == frameBeforeHidden,
                 "hidden widgets must not claim they were rendered in a later native frame" );
        button->setVisible( true, false ); render();
        require( selections->count == beforeHidden, "hidden buttons and their held keys must not activate" );
        io.AddKeyEvent( ImGuiKey_F10, false ); render();
        const auto pressesBeforeFocusLoss = button->getKeyboardShortcutSnapshot().keyPressCount;
        io.AddKeyEvent( ImGuiKey_F10, true ); io.AddFocusEvent( false ); render();
        require( selections->count == beforeHidden, "focus loss must cancel queued shortcut input" );
        require( !button->getKeyboardShortcutSnapshot().lastRendered.keyPressed &&
                     button->getKeyboardShortcutSnapshot().keyPressCount == pressesBeforeFocusLoss,
                 "focus-cleared input must not be reported as a surviving shortcut edge" );
        io.AddFocusEvent( true ); render();
        io.AddKeyEvent( ImGuiKey_F10, true ); render();
        require( selections->count == beforeHidden + 1, "a fresh key press after focus returns must work" );
        io.AddKeyEvent( ImGuiKey_F10, false ); render();
        clipButton = true;
        const auto beforeClipped = selections->count;
        io.AddKeyEvent( ImGuiKey_F10, true ); render();
        diagnostic = button->getKeyboardShortcutSnapshot();
        require( !diagnostic.lastRendered.itemVisible && diagnostic.lastRendered.keyPressed &&
                     diagnostic.lastRendered.frame == ImGui::GetFrameCount() && selections->count == beforeClipped + 1,
                 "a clipped item must report clipping while preserving its existing keyboard activation path" );
        io.AddKeyEvent( ImGuiKey_F10, false ); clipButton = false; render();
        bool invalid = false;
        try { shortcut( "Alt+F4" ); } catch( const std::invalid_argument & ) { invalid = true; }
        require( invalid, "unsupported shortcut properties must fail explicitly" );
        String retained;
        require( button->getProperties()->getPropertyValue( "keyboardShortcut", retained ) && retained == "F10",
                 "invalid shortcut properties must preserve the previous valid binding" );
        shortcut( "" );
        const auto beforeMouse = selections->count;
        io.AddMousePosEvent( mouseTarget.x, mouseTarget.y ); io.AddMouseButtonEvent( 0, true ); render();
        io.AddMouseButtonEvent( 0, false ); render();
        require( selections->count == beforeMouse + 1, "keyboard configuration must preserve ordinary pointer activation" );
        require( button->getKeyboardShortcutSnapshot().activationCount == selections->count,
                 "native activation diagnostics must match both keyboard and ordinary pointer dispatch" );
        require( selections->renderOrigin && selections->correctSender,
                 "accelerators must preserve the real render-origin event and button sender" );
        io.ConfigInputTrickleEventQueue = trickle;
        button->removeObjectListener( selections );
        parent->removeChild( button );
    }

    void projectThemeSelection()
    {
        auto settings = make_ptr<Properties>();
        ImGui::StyleColorsDark();
        auto &style = ImGui::GetStyle();
        style.FramePadding = ImVec2( 9, 7 );
        style.WindowBorderSize = 3;
        const auto originalColour = style.Colors[ImGuiCol_WindowBg];
        ui::WPImGuiStyle::applyProjectTheme( settings.get() );
        require( near( style.Colors[ImGuiCol_WindowBg].x, originalColour.x ) &&
                     near( style.Colors[ImGuiCol_WindowBg].w, originalColour.w ),
                 "a legacy profile must preserve the initialized ImGui palette" );
        require( near( style.FramePadding.x, 9 ) && near( style.FramePadding.y, 7 ) &&
                     near( style.WindowBorderSize, 3 ),
                 "a legacy profile must preserve host layout metrics" );

        settings->setProperty( "editor.theme", 3 );
        ui::WPImGuiStyle::applyProjectTheme( settings.get() );
        require( near( style.WindowBorderSize, 2 ) && near( style.FrameBorderSize, 1 ) &&
                     near( style.Colors[ImGuiCol_WindowBg].x, 0 ),
                 "an explicit high contrast selection must change the real ImGui style" );

        for( const auto theme : { 0, 2, 1 } )
        {
            settings->setProperty( "editor.theme", 3 );
            ui::WPImGuiStyle::applyProjectTheme( settings.get() );
            settings->setProperty( "editor.theme", theme );
            ui::WPImGuiStyle::applyProjectTheme( settings.get() );
            require( near( style.WindowBorderSize, 1 ) && near( style.FrameBorderSize, 0 ) &&
                         near( style.FramePadding.x, 6 ) && near( style.FramePadding.y, 6 ),
                     "explicit theme transitions must restore editor layout metrics" );
        }
        require( near( style.Colors[ImGuiCol_WindowBg].x, ui::WPImGuiStyle::gray( 6 ).x ),
                 "the dark theme must restore the editor palette" );
        style.FramePadding.x = 11;
        ui::WPImGuiStyle::applyProjectTheme( settings.get() );
        require( near( style.FramePadding.x, 11 ),
                 "an unchanged theme must not reset style customizations every frame" );

        auto partial = make_ptr<Properties>();
        partial->setProperty( "graphics.hdr", true );
        ui::WPImGuiStyle::applyProjectTheme( partial.get() );
        require( near( style.FramePadding.x, 11 ),
                 "a partial graphics profile must not replace ImGui styling" );
        ui::WPImGuiStyle::applyProjectTheme( settings.get() );
        require( near( style.FramePadding.x, 6 ),
                 "an explicit selection after a legacy profile must apply again" );
    }

    void displayedImageGeometry( Fixture &fixture, bool textureReady, bool fractional )
    {
        auto window = make_ptr<ui::ImGuiRenderWindow>();
        auto texture = make_ptr<Texture>();
        if( textureReady ) texture->handle = reinterpret_cast<void *>( size_t( 17 ) );
        window->setRenderTexture( texture );
        fixture.application->setSceneRenderWindow( window );
        ui::ImGuiApplication editor;
        ui::ClawUIManager nativeUi;

        // The production Win32 backend supplies main-client coordinates and enables
        // docking without platform viewports. DPI/RTT scaling is handled by the
        // texture-size/display-size ratio, not by adding a desktop-window offset.
        require( !( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable ),
                 "this input contract uses main-client ImGui coordinates" );
        ImGui::GetStyle().WindowPadding = ImVec2( 13, 17 );
        ImGui::NewFrame();
        ImGui::SetNextWindowPos( ImVec2( 271, 113 ), ImGuiCond_Always );
        ImGui::SetNextWindowSize( ImVec2( 871, 653 ), ImGuiCond_Always );
        ImGui::Begin( "Nonzero dock panel", nullptr, ImGuiWindowFlags_NoSavedSettings );
        const auto panel = ImGui::GetWindowPos();
        if( fractional )
        {
            const auto cursor = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos( ImVec2( cursor.x + .25f, cursor.y + .5f ) );
        }
        const auto image = ImGui::GetCursorScreenPos();
        const auto displayed = ImGui::GetContentRegionAvail();
        require( image.x > panel.x && image.y > panel.y + ImGui::GetFontSize(),
                 "fixture must include both horizontal padding and a title bar" );
        editor.draw( window );
        const auto published = window->getPosition();
        const auto publishedSize = window->getSize();
        require( near( published.X(), image.x ) && near( published.Y(), image.y ),
                 "published viewport origin must equal the actual texture image origin" );
        require( near( publishedSize.X(), displayed.x ) && near( publishedSize.Y(), displayed.y ),
                 "published display bounds must retain fractional image dimensions" );
        require( texture->size.X() == int( displayed.x ) && texture->size.Y() == int( displayed.y ),
                 "texture allocation must retain integer pixel dimensions" );
        require( near( ImGui::GetItemRectMin().x, image.x ) && near( ImGui::GetItemRectMin().y, image.y ),
                 "the reserved image item must start at the published input origin" );
        ImGui::End();
        ImGui::EndFrame();

        // Exercise the actual native UI translation with an RTT that differs from
        // its displayed size, including when the initial texture handle is absent.
        texture->size = Vector2I( 1280, 720 );
        auto *context = nativeUi.getContext();
        require( context != nullptr, "native UI input context must exist" );
        mouse( nativeUi, published, IMouseState::Event::Moved );
        require( context->input.mouse.pos.x == 0 && context->input.mouse.pos.y == 0,
                 "a client pointer at the image origin must map to RTT origin" );
        mouse( nativeUi, published + publishedSize * .5f, IMouseState::Event::Moved );
        require( std::abs( context->input.mouse.pos.x - 640 ) <= 1 &&
                     std::abs( context->input.mouse.pos.y - 360 ) <= 1,
                 "a client pointer at displayed image center must map to RTT center" );
        mouse( nativeUi, Vector2F( published.X(), published.Y() - 1 ), IMouseState::Event::LeftPressed );
        require( !context->input.mouse.buttons[WORKPHONE_BUTTON_LEFT].down,
                 "the title/padding strip outside the image must not start a click" );
        mouse( nativeUi, published + publishedSize * .25f, IMouseState::Event::LeftPressed );
        require( context->input.mouse.buttons[WORKPHONE_BUTTON_LEFT].down,
                 "an image click must reach native UI after client-to-RTT mapping" );
        mouse( nativeUi, published - Vector2F( 1, 1 ), IMouseState::Event::LeftReleased );
        require( !context->input.mouse.buttons[WORKPHONE_BUTTON_LEFT].down,
                 "releasing outside the image must clear native button state" );
        fixture.application->setSceneRenderWindow( nullptr );
    }
}

int main()
{
    try
    {
        Fixture fixture;
        projectThemeSelection();
        textReplacement();
        buttonKeyboardAccelerators( fixture );
#if defined WP_PLATFORM_WIN32
        nativeSystemMenuShortcuts();
#endif
        displayedImageGeometry( fixture, true, false );
        displayedImageGeometry( fixture, false, true );
        std::printf( "ImGui theme/text/button/native-message/viewport input: %u assertions passed\n", assertions );
        return 0;
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "FAIL: %s\n", error.what() );
        return 1;
    }
}
