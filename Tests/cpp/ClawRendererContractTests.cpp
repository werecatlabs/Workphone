#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneRenderer.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WPGraphics/ClawShader.hpp>
#include <WPGraphics/ClawGraphicsPipeline.hpp>
#include <Workphone/Scene/Directors/GraphicsSettingsDirector.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/Input/InputEvent.hpp>
#include <Workphone/Input/MouseState.hpp>
#include <Workphone/Input/KeyboardState.hpp>
#include <Workphone/UI/UIWindow.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <WorkphoneCore/workphone.h>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <WorkphoneGraphics/workphone_graphics_shader.h>

#include <cstdio>
#include <memory>
#include <limits>

namespace
{
    constexpr wp_s32 kWidth = 256;
    constexpr wp_s32 kHeight = 64;
    int gShaderNative = 0;

    struct TypeManagerScope
    {
        TypeManagerScope() : manager( std::make_unique<workphone::TypeManager>() )
        {
            manager->load();
            workphone::TypeManager::setInstance( manager.get() );
        }

        ~TypeManagerScope()
        {
            workphone::TypeManager::setInstance( nullptr );
            manager->unload();
        }

        std::unique_ptr<workphone::TypeManager> manager;
    };

    wp_s32 compileShader( wp_shader *, void **candidate, void * )
    {
        *candidate = &gShaderNative;
        return 1;
    }

    bool check( bool condition, const char *message )
    {
        if( !condition )
        {
            std::fprintf( stderr, "FAIL: %s\n", message );
        }
        return condition;
    }

    bool framebufferContainsTextPixels( const wp_byte *pixels )
    {
        if( !pixels )
        {
            return false;
        }

        for( wp_s32 i = 0; i < kWidth * kHeight; ++i )
        {
            const auto *pixel = pixels + i * 4;
            if( pixel[0] != 0 || pixel[1] != 0 || pixel[2] != 0 )
            {
                return true;
            }
        }
        return false;
    }

    class InputSceneWindow : public workphone::ui::UIWindow
    {
    public:
        void invalidate() override {}
        workphone::Vector2F getPosition() const override { return { 100.0f, 50.0f }; }
        workphone::Vector2F getSize() const override { return { 256.0f, 64.0f }; }
    };

    bool testUiInput()
    {
        using namespace workphone;
        auto application = make_ptr<core::ApplicationManager>();
        core::IApplicationManager::setInstance( application );
        application->setFactoryManager( make_ptr<FactoryManager>() );
        const auto previousTask = Thread::getCurrentTask();
        bool ok = true;
        {
            ui::ClawUIManager manager;
            auto *ctx = manager.getContext();
            auto mouse = make_ptr<MouseState>();
            auto event = make_ptr<InputEvent>();
            event->setEventType( IInputEvent::EventType::Mouse );
            event->setMouseState( mouse );
            mouse->setAbsolutePosition( { 188.0f, 80.0f } );
            mouse->setRelativePosition( { 0.188f, 0.1f } );
            mouse->setEventType( IMouseState::Event::Moved );

            Thread::setCurrentTask( TaskId::Primary );
            manager.OnEvent( event );
            ok &= check( ctx->input.mouse.pos.x == 0.0f,
                         "non-render task input must not mutate the UI context" );
            Thread::setCurrentTask( TaskId::Render );
            manager.OnEvent( event );
            ok &= check( ctx->input.mouse.pos.x == 188.0f && ctx->input.mouse.pos.y == 80.0f,
                         "standalone input must use absolute pixel coordinates" );

            application->setSceneRenderWindow( make_ptr<InputSceneWindow>() );
            manager.OnEvent( event );
            ok &= check( ctx->input.mouse.pos.x == 88.0f && ctx->input.mouse.pos.y == 30.0f,
                         "editor input must be relative to the displayed scene image" );

            const auto drawButton = [&]() {
                wp_input_end( ctx );
                const wp_rect bounds = { 0.0f, 0.0f, 256.0f, 64.0f };
                bool clicked = false;
                if( wp_begin( ctx, reinterpret_cast<const wp_c8 *>( "Input contract" ), bounds,
                              WORKPHONE_WINDOW_NO_SCROLLBAR | WORKPHONE_WINDOW_BACKGROUND ) )
                {
                    wp_layout_space_begin( ctx, WORKPHONE_STATIC, 64.0f, 1 );
                    const wp_rect buttonBounds = { 64.0f, 8.0f, 128.0f, 40.0f };
                    wp_layout_space_push( ctx, buttonBounds );
                    clicked = wp_button_label( ctx, "Click" ) != 0;
                    wp_layout_space_end( ctx );
                }
                wp_end( ctx );
                wp_clear( ctx );
                wp_input_begin( ctx );
                return clicked;
            };
            drawButton();
            mouse->setEventType( IMouseState::Event::LeftPressed );
            manager.OnEvent( event );
            int clicks = drawButton() ? 1 : 0;
            mouse->setEventType( IMouseState::Event::LeftReleased );
            manager.OnEvent( event );
            clicks += drawButton() ? 1 : 0;
            ok &= check( clicks == 1, "a scene-local press/release must activate a UI button once" );

            mouse->setEventType( IMouseState::Event::LeftPressed );
            manager.OnEvent( event );
            mouse->setAbsolutePosition( { 400.0f, 200.0f } );
            mouse->setEventType( IMouseState::Event::LeftReleased );
            manager.OnEvent( event );
            ok &= check( !ctx->input.mouse.buttons[WORKPHONE_BUTTON_LEFT].down,
                         "releasing outside the scene must clear held buttons" );
            wp_input_begin( ctx );
            mouse->setEventType( IMouseState::Event::LeftPressed );
            manager.OnEvent( event );
            ok &= check( !ctx->input.mouse.buttons[WORKPHONE_BUTTON_LEFT].down,
                         "a click outside the scene must not press a game UI button" );

            auto keyboard = make_ptr<KeyboardState>();
            event->setEventType( IInputEvent::EventType::Key );
            event->setKeyboardState( keyboard );
            keyboard->setPressedDown( true );
            keyboard->setChar( 'a' );
            manager.OnEvent( event );
            Thread::setCurrentTask( TaskId::Primary );
            manager.OnEvent( event );
            Thread::setCurrentTask( TaskId::Render );
            ok &= check( ctx->input.keyboard.text_len == 1 && ctx->input.keyboard.text[0] == 'a',
                         "keyboard text must arrive once despite multiple task dispatches" );
            keyboard->setKeyCode( static_cast<u32>( KeyCodes::KEY_BACK ) );
            keyboard->setChar( 8 );
            manager.OnEvent( event );
            ok &= check( ctx->input.keyboard.keys[WORKPHONE_KEY_BACKSPACE].down &&
                             ctx->input.keyboard.text_len == 1,
                         "backspace must be an editing key, not a text character" );
        }
        Thread::setCurrentTask( previousTask );
        application->setSceneRenderWindow( nullptr );
        application->setFactoryManager( nullptr );
        core::IApplicationManager::setInstance( nullptr );
        return ok;
    }
}  // namespace

int main()
{
    TypeManagerScope typeManagerScope;
    workphone::ui::ClawUIWorkphoneContext uiContext;
    workphone::ui::ClawUIWorkphoneRenderer uiRenderer;
    wp_renderer *renderer = nullptr;
    bool ok = true;

    ok &= testUiInput();

    {
        using namespace workphone;
        using namespace workphone::render;
        ClawGraphicsPipeline pipeline;
        auto properties = pipeline.getProperties();
        ok &= check( properties->getPropertyObject( "Applies to" ).isReadOnly(),
                     "pipeline backend information must be read-only" );
        properties->setProperty( "Quality", 0 );
        pipeline.setProperties( properties );
        ok &= check( pipeline.getQualityLevel() == QualityLevel::Low &&
                         !pipeline.isTAAEnabled() && !pipeline.isBloomEnabled() &&
                         !pipeline.isExposureEnabled(),
                     "changing the quality preset must not reapply stale inspector toggles" );

        auto changes = make_ptr<Properties>();
        changes->setProperty( "Bloom", true );
        changes->setProperty( "Bloom intensity", 2.0f );
        changes->setProperty( "TAA feedback", 5.0f );
        changes->setProperty( "AO radius", -10.0f );
        pipeline.setProperties( changes );
        ok &= check( pipeline.isBloomEnabled() && pipeline.getHdrSettings().m_bloomIntensity == 2.0f &&
                         pipeline.getTaaSettings().m_feedback == 0.99f &&
                         pipeline.getTaoaSettings().m_radius == 0.01f && !pipeline.isTAAEnabled(),
                     "partial pipeline edits must reach setters, clamp ranges and preserve omitted options" );

        changes = make_ptr<Properties>();
        changes->setProperty( "Bloom intensity", std::numeric_limits<f32>::infinity() );
        pipeline.setProperties( changes );
        ok &= check( pipeline.getHdrSettings().m_bloomIntensity == 2.0f,
                     "non-finite pipeline values must not reach the renderer" );
        auto roundTrip = pipeline.getProperties();
        pipeline.setProperties( roundTrip );
        ok &= check( pipeline.isBloomEnabled() && !pipeline.isExposureEnabled(),
                     "an unchanged inspector snapshot must preserve custom effect overrides" );
        pipeline.setProperties( nullptr );

        scene::GraphicsSettingsDirector gameOptions;
        auto gameChanges = make_ptr<Properties>();
        gameChanges->setProperty( "Render API", 0 );
        gameChanges->setProperty( "Bloom", false );
        gameOptions.setProperties( gameChanges );
        ok &= check( gameOptions.getRenderApi() == IGraphicsSystem::RenderApi::Software &&
                         pipeline.isBloomEnabled(),
                     "game renderer and effect settings must not mutate editor pipeline options" );
        auto serialized = DataUtil::toString( gameOptions.getProperties().get(), true );
        auto restored = make_ptr<Properties>();
        DataUtil::parse( serialized, restored.get() );
        ClawGraphicsPipeline runtimePipeline;
        runtimePipeline.setProperties( restored );
        runtimePipeline.setProperties( restored );
        ok &= check( !runtimePipeline.isBloomEnabled() && pipeline.isBloomEnabled(),
                     "saved game options must restore custom effects independently of the editor" );

        auto lowPreset = make_ptr<Properties>();
        lowPreset->setProperty( "Quality", 0 );
        gameOptions.setProperties( lowPreset );
        auto customBloom = make_ptr<Properties>();
        customBloom->setProperty( "Bloom", true );
        gameOptions.setProperties( customBloom );
        serialized = DataUtil::toString( gameOptions.getProperties().get(), true );
        restored = make_ptr<Properties>();
        DataUtil::parse( serialized, restored.get() );
        runtimePipeline.setProperties( restored );
        runtimePipeline.setProperties( restored );
        ok &= check( runtimePipeline.getQualityLevel() == QualityLevel::Low &&
                         runtimePipeline.isBloomEnabled() && !runtimePipeline.isExposureEnabled(),
                     "restoring saved settings must apply a new preset before its custom effect overrides" );
    }

    {
        workphone::render::ClawShader shader;
        void *native = nullptr;
        workphone::Array<workphone::u8> binary = { 3u, 2u, 1u, 0u };
        shader.setLanguage( workphone::render::IShader::Language::HLSL );
        shader.setSource( "void main(){}" );
        ok &= check( !shader.compile() &&
                         shader.getStatus() == workphone::render::IShader::Status::Failed &&
                         !shader.getDiagnostics().empty(),
                     "Claw must expose a missing backend compiler as a diagnosed failure" );
        wp_shader_set_compile_func( shader.getNativeShader(), compileShader, nullptr );
        ok &= check( shader.compile() && shader.isCompiled(),
                     "Claw must compile through the canonical C89 callback" );
        shader.getNativeHandle( &native );
        ok &= check( native == &gShaderNative,
                     "Claw native handle must expose the compiled backend object" );
        shader.setDefine( "QUALITY", "ULTRA" );
        ok &= check( shader.getStatus() == workphone::render::IShader::Status::SourceReady &&
                         wp_shader_is_dirty( shader.getNativeShader() ) &&
                         wp_shader_get_define_count( shader.getNativeShader() ) == 1,
                     "define changes must invalidate canonical C89 shader state" );
        ok &= check( shader.setSpecializationConstant( "LIGHT_COUNT", 8u ) &&
                         wp_shader_get_specialization_count( shader.getNativeShader() ) == 1,
                     "specialization constants must be canonical C89 compile inputs" );
        ok &= check( shader.setBinary( binary, workphone::render::IShader::Language::SPIRV ) &&
                         shader.getSource().empty() && shader.getBinary() == binary &&
                         shader.isCompiled(),
                     "binary-only shaders must remain synchronized across C++ and C89" );
    }

    ok &= check( uiContext.isValid(), "UI context should initialise its default font" );
    if( !ok )
    {
        return 1;
    }

    auto *context = uiContext.getContext();
    uiRenderer.setNullTexture( uiContext.getNullTexture() );

    wp_input_begin( context );
    wp_input_end( context );

    const wp_rect bounds = { 0.0f, 0.0f, static_cast<wp_f32>( kWidth ),
                             static_cast<wp_f32>( kHeight ) };
    const wp_color transparent = { 0, 0, 0, 0 };
    context->style.window.fixed_background = wp_style_item_color( transparent );
    context->style.window.background = transparent;
    if( wp_begin( context, reinterpret_cast<const wp_c8 *>( "Renderer contract" ), bounds,
                  WORKPHONE_WINDOW_NO_SCROLLBAR | WORKPHONE_WINDOW_BACKGROUND ) )
    {
        wp_layout_space_begin( context, WORKPHONE_STATIC, 48.0f, 1 );
        const wp_rect labelBounds = { 8.0f, 8.0f, 232.0f, 32.0f };
        wp_layout_space_push( context, labelBounds );
        const wp_color white = { 255, 255, 255, 255 };
        wp_label_colored( context, reinterpret_cast<const wp_c8 *>( "Renderer text contract" ),
                          WORKPHONE_TEXT_ALIGN_LEFT | WORKPHONE_TEXT_ALIGN_MIDDLE, white );
        wp_layout_space_end( context );
    }
    wp_end( context );

    ok &= check( uiRenderer.convert( context ), "UI commands should convert to draw geometry" );
    ok &= check( uiRenderer.getCommandCount() > 0, "text should produce a draw command" );
    ok &= check( uiRenderer.getVertexCount() > 0, "text should produce vertices" );
    ok &= check( uiRenderer.getIndexCount() > 0, "text should produce indices" );

    renderer = wp_renderer_create_software( kWidth, kHeight, WORKPHONE_PIXEL_FORMAT_BGRA8 );
    ok &= check( renderer != nullptr, "software renderer should be available for headless tests" );
    if( renderer )
    {
        wp_renderer_begin_frame( renderer );
        wp_renderer_set_clear_color( renderer, 0.0f, 0.0f, 0.0f, 1.0f );
        wp_renderer_clear( renderer, WORKPHONE_CLEAR_FLAG_ALL );
        ok &= check( uiRenderer.submit( context, renderer, uiContext.getFontPixels(),
                                        uiContext.getFontWidth(), uiContext.getFontHeight(),
                                        uiContext.getFontTexture() ),
                     "converted text should be submitted to the renderer" );
        wp_renderer_end_frame( renderer );

        const auto *pixels =
            static_cast<const wp_byte *>( wp_renderer_get_framebuffer( renderer ) );
        ok &= check( framebufferContainsTextPixels( pixels ),
                     "text submission should change framebuffer pixels" );

        // A scene target/viewport can be smaller than the renderer's main window.
        // UI coordinates and clipping must remain local to that active viewport.
        wp_clear( context );
        wp_input_begin( context );
        wp_input_end( context );
        const wp_viewport_i insetViewport = { 64, 16, 128, 32 };
        wp_renderer_set_viewport( renderer, insetViewport );
        const wp_rect insetBounds = { 0.0f, 0.0f, 128.0f, 32.0f };
        if( wp_begin( context, reinterpret_cast<const wp_c8 *>( "Inset viewport" ), insetBounds,
                      WORKPHONE_WINDOW_NO_SCROLLBAR | WORKPHONE_WINDOW_BACKGROUND ) )
        {
            const wp_rect rectangle = { 16.0f, 8.0f, 32.0f, 16.0f };
            const wp_color white = { 255, 255, 255, 255 };
            wp_fill_rect( wp_window_get_canvas( context ), rectangle, 0.0f, white );
        }
        wp_end( context );
        ok &= check( uiRenderer.convert( context ), "inset UI should convert to geometry" );
        wp_renderer_begin_frame( renderer );
        wp_renderer_clear( renderer, WORKPHONE_CLEAR_FLAG_ALL );
        ok &= check( uiRenderer.submit( context, renderer, uiContext.getFontPixels(),
                                        uiContext.getFontWidth(), uiContext.getFontHeight(),
                                        uiContext.getFontTexture() ),
                     "inset UI should submit to the active viewport" );
        wp_renderer_end_frame( renderer );
        pixels = static_cast<const wp_byte *>( wp_renderer_get_framebuffer( renderer ) );
        const auto *inside = pixels + ( 30 * kWidth + 88 ) * 4;
        const auto *outside = pixels + ( 30 * kWidth + 70 ) * 4;
        ok &= check( inside[0] > 200 && inside[1] > 200 && inside[2] > 200,
                     "UI must retain its pixel size and viewport offset" );
        ok &= check( outside[0] == 0 && outside[1] == 0 && outside[2] == 0,
                     "UI must not draw outside its viewport-local bounds" );
        wp_renderer_destroy( renderer );
    }

    if( ok )
    {
        std::puts( "Claw renderer and UI input contract tests passed." );
        return 0;
    }
    return 1;
}
