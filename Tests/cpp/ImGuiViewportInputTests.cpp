#include <WPImGui/ImGuiApplication.hpp>
#include <WPImGui/ImGuiRenderWindow.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Input/InputEvent.hpp>
#include <Workphone/Input/MouseState.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <WorkphoneCore/workphone.h>
#include <imgui.h>
#include <cmath>
#include <cstdio>
#include <memory>
#include <stdexcept>

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
        }

        ~Fixture()
        {
            ImGui::DestroyContext();
            application->setSceneRenderWindow( nullptr );
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
        displayedImageGeometry( fixture, true, false );
        displayedImageGeometry( fixture, false, true );
        std::printf( "ImGui viewport input: 2 cases, %u assertions passed\n", assertions );
        return 0;
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "FAIL: %s\n", error.what() );
        return 1;
    }
}
