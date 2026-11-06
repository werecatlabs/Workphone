#pragma once

#include "ClawRTTJobContracts.hpp"
#include <WorkphonePlatformWin32/workphone_graphics_ui_dx11.h>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/StateManager.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Interface/UI/IUILayoutWindow.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>

namespace claw_ui_rtt_contracts
{
using namespace workphone;
using namespace workphone::render;
using claw_rtt_contracts::require;

struct Services
{
    Services() : previousApplication( core::IApplicationManager::instance() ),
                 previousTask( Thread::getCurrentTask() )
    {
        application = make_ptr<core::ApplicationManager>();
        core::IApplicationManager::setInstance( application );
        application->setFactoryManager( make_ptr<FactoryManager>() );
        states = make_ptr<StateManager>();
        application->setStateManager( states );
        states->load( nullptr );
        require( states->isLoaded(), "UI RTT fixture StateManager must load" );
        Thread::setCurrentTask( TaskId::Render );
    }

    ~Services()
    {
        {
            auto ui = application->getRenderUI();
            if( ui )
                ui->unload( nullptr );
            application->setRenderUI( nullptr );
        }
        application->setGraphicsSystem( nullptr );
        states->unload( nullptr );
        application->setStateManager( nullptr );
        states = nullptr;
        application->setFactoryManager( nullptr );
        core::IApplicationManager::setInstance( previousApplication );
        Thread::setCurrentTask( previousTask );
    }

    SmartPtr<core::IApplicationManager> previousApplication;
    SmartPtr<core::ApplicationManager> application;
    SmartPtr<StateManager> states;
    TaskId previousTask;
};

class Graphics : public claw_rtt_contracts::Graphics
{
public:
    void render() override { ++draws; }
};

inline void nativeRanges( wp_renderer_dx11 *renderer )
{
    std::unique_ptr<wp_render_texture_dx11, decltype( &wp_renderer_dx11_destroy_render_texture )>
        target( wp_renderer_dx11_create_render_texture( renderer, 32, 32 ),
                &wp_renderer_dx11_destroy_render_texture );
    require( target != nullptr, "native UI range fixture must allocate an output" );

    wp_graphics_rtt_desc desc{};
    desc.renderer_generation = wp_renderer_dx11_get_rtt_generation( renderer );
    desc.target_generation = wp_renderer_dx11_get_render_texture_generation( target.get() );
    desc.width = desc.height = 32;
    desc.viewport = { 0, 0, 32, 32 };
    desc.clear_colour = 1;
    desc.colour[3] = 1;
    const auto draw = []( void *user, const wp_graphics_rtt_desc * ) {
        auto *native = static_cast<wp_renderer_dx11 *>( user );
        wp_mat4f identity{};
        for( int i = 0; i < 4; ++i )
            identity.m[i][i] = 1;
        wp_renderer_dx11_set_world_matrix( native, &identity );
        wp_renderer_dx11_set_view_matrix( native, &identity );
        wp_renderer_dx11_set_projection_matrix( native, &identity );
        wp_renderer_dx11_set_cull_mode( native, WORKPHONE_CULL_MODE_NONE );
        wp_renderer_dx11_set_blend_mode( native, WORKPHONE_BLEND_MODE_NONE );
        wp_renderer_dx11_set_depth_test_enabled( native, 0 );

        const wp_vertex_ptc vertices[] = { {},
            { { -1, -1, .5f }, { 0, 0 }, 0xffffffffu },
            { { -1, 3, .5f }, { 0, 0 }, 0xffffffffu },
            { { 3, -1, .5f }, { 0, 0 }, 0xffffffffu } };
        const uint16_t indices16[] = { 3, 0, 1, 2, 0, 1, 2 };
        const uint32_t indices32[] = { 0, 1, 2 };
        wp_ui_draw_list_dx11 list16{ vertices, indices16, 4, 7, sizeof( uint16_t ), 0 };
        wp_ui_draw_list_dx11 list32{ vertices, indices32, 4, 3, sizeof( uint32_t ), 0 };
        const wp_viewport_i untouched{ 24, 0, 8, 32 };
        const bool submitted = wp_renderer_dx11_draw_ui_range( native, &list16, 1, 3, 1,
                                                               { 0, 0, 8, 32 }, nullptr ) &&
            !wp_renderer_dx11_draw_ui_range( native, &list16, 0, 3, 1, untouched, nullptr ) &&
            !wp_renderer_dx11_draw_ui_range( native, &list16, 6, 3, 1, untouched, nullptr ) &&
            wp_renderer_dx11_draw_ui_range( native, &list16, 4, 3, 1, { 8, 0, 8, 32 }, nullptr ) &&
            wp_renderer_dx11_draw_ui_range( native, &list32, 0, 3, 1, { 16, 0, 8, 32 }, nullptr );
        return submitted ? WP_RTT_OK : WP_RTT_BACKEND_FAILED;
    };
    wp_render_statistics_dx11 before{}, after{};
    wp_renderer_dx11_get_statistics_counters( renderer, &before );
    require( wp_renderer_dx11_execute_rtt_job( renderer, target.get(), &desc, nullptr, nullptr,
                                              draw, renderer ) == WP_RTT_OK,
             "native UI must accept valid ranges and reject out-of-bounds ranges and indices" );
    wp_renderer_dx11_get_statistics_counters( renderer, &after );
    require( after.draws == before.draws + 3 && after.triangles == before.triangles + 3,
             "native UI must submit exactly the three valid ranges" );
    const auto pixels = claw_rtt_contracts::pixels( target.get() );
    const auto red = [&]( size_t x ) { return pixels[( 16 * 32 + x ) * 4]; };
    require( red( 4 ) > 240 && red( 12 ) > 240 && red( 20 ) > 240 && red( 28 ) == 0,
             "native UI must preserve rebased indices and per-range scissor for both index widths" );
}

inline bool editorPreview( SmartPtr<IGraphicsWindow> window )
{
    Services services;
    auto renderer = make_ptr<claw_rtt_contracts::Renderer>();
    renderer->load( window );
    require( renderer->isLoaded(), "UI preview renderer must initialize" );

    constexpr unsigned width = 128, height = 64;
    auto target = make_ptr<ClawRenderTarget>();
    target->setSize( { width, height } );
    target->setActive( true );
    auto viewport = target->addViewport( 23, make_ptr<ClawCamera>() );
    require( viewport != nullptr, "UI preview viewport must exist" );
    viewport->setActive( true );
    viewport->setEnableSceneRender( true );
    viewport->setEnableUI( false );
    viewport->setClearEveryFrame( true, WORKPHONE_CLEAR_FLAG_ALL );
    viewport->setBackgroundColour( ColourF( .1f, .2f, .3f, 1 ) );
    auto texture = make_ptr<claw_rtt_contracts::Texture>();
    texture->output = target;
    auto textures = make_ptr<claw_rtt_contracts::Textures>();
    textures->values.push_back( texture );
    auto graphics = make_ptr<Graphics>();
    graphics->initialise( renderer, textures );
    wp_graphics_system_set_renderer( graphics->getNativeSystem(), renderer->getNativeRenderer() );
    services.application->setGraphicsSystem( graphics );

    auto ui = make_ptr<ui::ClawUIManager>();
    ui->load( nullptr );
    services.application->setRenderUI( ui );
    auto canvas = ui->addElement( ui::IUILayoutWindow::typeInfo() );
    auto panel = ui->addElement( ui::IUIImage::typeInfo() );
    auto text = dynamic_pointer_cast<ui::IUIText>( ui->addElement( ui::IUIText::typeInfo() ) );
    require( canvas && panel && text, "UI preview must create a native canvas, image and text" );
    canvas->load( nullptr );
    panel->load( nullptr );
    text->load( nullptr );
    canvas->addChild( panel );
    canvas->addChild( text );
    panel->setPosition( { .1f, .15f } );
    panel->setSize( { .3f, .65f } );
    panel->setColour( ColourF( 1, 0, 0, 1 ) );
    text->setPosition( { .55f, .2f } );
    text->setSize( { .4f, .6f } );
    text->setColour( ColourF( 1, 1, 1, 1 ) );
    text->setTextSize( 24 );
    text->setText( "UI" );

    auto *output = target->getNativeRenderTexture( renderer->getNativeRenderer() );
    const auto pixels = [&] {
        std::vector<unsigned char> value( width * height * 4 );
        require( wp_renderer_dx11_copy_render_texture_rgba8( output, value.data(), value.size() ),
                 "UI preview texture must support RGBA8 readback" );
        return value;
    };
    graphics->frame();
    const auto background = pixels();
    viewport->setEnableUI( true );
    graphics->frame();
    const auto visible = pixels();
    const auto panelPixel = ( height / 2 * width + width / 4 ) * 4;
    require( visible[panelPixel] > 240 && visible[panelPixel + 1] < 10 &&
                 visible[panelPixel + 2] < 10 && visible[panelPixel + 3] == 255,
             "Editor-style RTT must contain the native canvas image colour" );
    unsigned glyphPixels = 0;
    for( unsigned y = 0; y < height; ++y )
        for( unsigned x = width / 2; x < width; ++x )
        {
            const auto offset = ( y * width + x ) * 4;
            glyphPixels += visible[offset] > 200 && visible[offset + 1] > 200 &&
                           visible[offset + 2] > 200;
        }
    require( glyphPixels > 20, "Editor-style RTT must contain visible text glyph pixels" );
    require( renderer->getRenderTarget() == window,
             "UI preview must restore the window target after offscreen composition" );

    text->setVisible( false );
    graphics->frame();
    const auto withoutText = pixels();
    require( withoutText != visible && withoutText[panelPixel] > 240,
             "text visibility must change RTT pixels while retaining its canvas sibling" );
    canvas->setVisible( false );
    graphics->frame();
    require( pixels() == background, "a hidden native canvas must hide its offscreen UI children" );
    canvas->setVisible( true );
    viewport->setEnableUI( false );
    graphics->frame();
    require( pixels() == background, "disabling viewport UI must remove offscreen UI composition" );
    nativeRanges( wp_renderer_get_dx11( renderer->getNativeRenderer() ) );
    ui->clear();
    std::puts( "Editor-style RTT UI image/text pixels, canvas visibility and viewport UI gating passed." );
    return true;
}
} // namespace claw_ui_rtt_contracts
