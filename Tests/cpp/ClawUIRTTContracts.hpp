#pragma once

#include "ClawRTTJobContracts.hpp"
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
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
        Thread::setCurrentTask( TaskId::Render );
    }

    ~Services()
    {
        application->setRenderUI( nullptr );
        application->setGraphicsSystem( nullptr );
        application->setFactoryManager( nullptr );
        core::IApplicationManager::setInstance( previousApplication );
        Thread::setCurrentTask( previousTask );
    }

    SmartPtr<core::IApplicationManager> previousApplication;
    SmartPtr<core::ApplicationManager> application;
    TaskId previousTask;
};

class Graphics : public claw_rtt_contracts::Graphics
{
public:
    void render() override { ++draws; }
};

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
    ui->clear();
    std::puts( "Editor-style RTT UI image/text pixels, canvas visibility and viewport UI gating passed." );
    return true;
}
} // namespace claw_ui_rtt_contracts
