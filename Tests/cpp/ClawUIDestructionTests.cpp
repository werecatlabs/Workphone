#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WPGraphics/UI/ClawUILayout.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUIDropdown.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>
#include <Workphone/Interface/UI/IUICheckbox.hpp>
#include <Workphone/Interface/UI/IUIToggle.hpp>
#include <Workphone/Interface/UI/IUISlider.hpp>
#include <Workphone/Interface/UI/IUILayoutWindow.hpp>
#include <Workphone/Interface/UI/IUILayoutContainer.hpp>
#include <Workphone/Scene/Components/UI/Text.hpp>
#include <Workphone/Scene/Components/UI/Button.hpp>
#include <Workphone/Scene/Components/UI/Slider.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Systems/UI/LayoutTransformSystem.hpp>
#include <Workphone/Scene/UiUtil.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Core/Properties.hpp>
#include "UILayoutTraversalContracts.hpp"
#include "InputListenerDispatchContracts.hpp"
#include "SceneDropdownLifetimeContracts.hpp"
#include <Workphone/Scene/CameraManager.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <WorkphoneCore/workphone.h>

using namespace workphone;

namespace
{
    unsigned assertions = 0;

    struct Fixture
    {
        Fixture() : types( std::make_unique<TypeManager>() ),
                    previousTask( Thread::getCurrentTask() )
        {
            types->load();
            TypeManager::setInstance( types.get() );
            application = make_ptr<core::ApplicationManager>();
            core::IApplicationManager::setInstance( application );
            application->setFactoryManager( make_ptr<FactoryManager>() );
            application->setGraphicsSystem( make_ptr<render::ClawHammerSystem>() );
            application->setGameManager( make_ptr<scene::GameManager>() );
            Thread::setCurrentTask( TaskId::Render );
        }

        ~Fixture()
        {
            application->setRenderUI( nullptr );
            application->setGameManager( nullptr );
            application->setGraphicsSystem( nullptr );
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

    bool check( bool condition, const char *message )
    {
        ++assertions;
        if( !condition )
            std::fprintf( stderr, "FAIL: %s\n", message );
        return condition;
    }

    bool testStartupFont( Fixture &fixture, const String &project, bool proportional,
                          bool rememberedProject = false )
    {
        const auto previousProject = fixture.application->getProjectPath();
        const auto previousSettings = fixture.application->getEditorSettings();
        auto preferences = make_ptr<Properties>();
        if( rememberedProject ) preferences->setProperty( "Project Path", project + "/project.fbproject" );
        fixture.application->setEditorSettings( preferences );
        fixture.application->setProjectPath( rememberedProject ? String() : project );
        bool ok = true;
        {
            ui::ClawUIWorkphoneContext context;
            ok &= check( context.isValid() && context.getFontPixels() && context.getFontWidth() > 0 &&
                         context.getFontHeight() > 0, "startup font must produce a usable RGBA atlas" );
            const auto *ctx = context.getContext();
            const auto *font = ctx ? ctx->style.font : nullptr;
            ok &= check( font && font->width && font->height == 13.0f,
                         "startup font must preserve the default widget height" );
            if( font && font->width )
            {
                const auto wide = font->width( font->userdata, 32.0f,
                    reinterpret_cast<const wp_c8 *>( "WWW" ), 3 );
                const auto narrow = font->width( font->userdata, 32.0f,
                    reinterpret_cast<const wp_c8 *>( "iii" ), 3 );
                ok &= check( wide > 0 && narrow > 0 &&
                             ( proportional ? wide > narrow * 1.5f : wide == narrow ),
                             proportional ? "project TTF must replace the embedded monospace face" :
                                            "missing or invalid TTF must retain the embedded face" );
            }
            ok &= check( context.getFontTexture().ptr == context.getFontPixels() &&
                         context.getNullTexture().texture.ptr == context.getFontPixels(),
                         "font and null texture handles must reference the owned atlas pixels" );
        }
        fixture.application->setProjectPath( previousProject );
        fixture.application->setEditorSettings( previousSettings );
        return ok;
    }

    class CollectionProbe : public ui::ClawUIElement<ui::IUIElement>
    {
    public:
        u32 getOrder() const override
        {
            ++orderReads;
            if( onOrder ) onOrder();
            return order;
        }

        void draw( wp_context * ) override
        {
            ++draws;
            if( observedDraws ) ++*observedDraws;
            if( onDraw ) onDraw();
        }

        u32 order = 0;
        mutable unsigned orderReads = 0;
        unsigned draws = 0;
        std::shared_ptr<unsigned> observedDraws;
        std::function<void()> onOrder;
        std::function<void()> onDraw;
    };

    bool testSortingExcludesOtherThreads()
    {
        auto manager = make_ptr<ui::ClawUIManager>();
        auto layout = dynamic_pointer_cast<ui::ClawUILayout>( manager->addElement( ui::IUILayoutWindow::typeInfo() ) );
        if( !layout ) return check( false, "sort regression needs a native factory-created layout" );
        auto first = make_ptr<CollectionProbe>();
        auto second = make_ptr<CollectionProbe>();
        first->order = 2;
        second->order = 1;
        layout->addChild( first );
        layout->addChild( second );

        std::promise<void> compared, release;
        auto comparisonReached = compared.get_future();
        auto continueComparison = release.get_future().share();
        std::atomic<bool> observed{ false };
        first->onOrder = [&] {
            if( !observed.exchange( true ) )
            {
                compared.set_value();
                continueComparison.wait();
            }
        };
        std::exception_ptr sortFailure;
        std::thread sorter( [&] {
            try { layout->sortZOrder(); }
            catch( ... ) { sortFailure = std::current_exception(); }
        } );

        const auto reached = comparisonReached.wait_for( std::chrono::seconds( 5 ) ) ==
                             std::future_status::ready;
        bool acquired = false;
        if( reached )
        {
            acquired = layout->getChildrenRef().try_lock();
            if( acquired ) layout->getChildrenRef().unlock();
        }
        release.set_value();
        sorter.join();
        first->onOrder = {};
        bool ok = check( reached, "native child sorting must reach the controlled order capture" );
        ok &= check( !sortFailure, "native child sorting must finish without throwing" );
        ok &= check( reached && !acquired,
                     "sorting must retain exclusive child inventory ownership across comparisons" );
        const auto children = layout->getChildren();
        ok &= check( children.size() == 2 && children[0] == second && children[1] == first,
                     "the protected sort must preserve and order both native children" );
        layout->removeAllChildren();
        manager->clear();
        return ok;
    }

    bool testSortCapturesEachOrderOnce()
    {
        auto manager = make_ptr<ui::ClawUIManager>();
        auto layout = dynamic_pointer_cast<ui::ClawUILayout>( manager->addElement( ui::IUILayoutWindow::typeInfo() ) );
        if( !layout ) return check( false, "order snapshot regression needs a native factory-created layout" );
        Array<SmartPtr<CollectionProbe>> probes;
        for( u32 order : { 3u, 1u, 1u, 0u } )
        {
            auto probe = make_ptr<CollectionProbe>();
            probe->order = order;
            layout->addChild( probe );
            probes.push_back( probe );
        }
        const auto before = layout->getChildren();
        for( auto probe : probes ) probe->orderReads = 0;
        layout->sortZOrder();
        bool ok = true;
        for( auto probe : probes )
        {
            ok &= check( probe->orderReads == 1,
                         "one native sort must capture each live order only once before comparing keys" );
        }
        const auto sorted = layout->getChildren();
        ok &= check( sorted.size() == 4 && sorted[0] == probes[3] && sorted[3] == probes[0],
                     "captured keys must order native collection membership" );
        Array<SmartPtr<ui::IUIElement>> tied;
        for( auto child : before )
            if( child == probes[1] || child == probes[2] ) tied.push_back( child );
        ok &= check( sorted.size() == 4 && tied.size() == 2 && sorted[1] == tied[0] && sorted[2] == tied[1],
                     "equal captured draw orders must retain their existing relative membership order" );
        layout->removeAllChildren();
        manager->clear();
        return ok;
    }

    bool testDrawPinsItsFrameMembers( Fixture &fixture )
    {
        auto manager = make_ptr<ui::ClawUIManager>();
        fixture.application->setRenderUI( manager );
        auto layout = dynamic_pointer_cast<ui::ClawUILayout>( manager->addElement( ui::IUILayoutWindow::typeInfo() ) );
        if( !layout )
        {
            fixture.application->setRenderUI( nullptr );
            return check( false, "draw regression needs a native factory-created layout" );
        }
        layout->load( nullptr );
        auto first = make_ptr<CollectionProbe>();
        auto second = make_ptr<CollectionProbe>();
        auto replacement = make_ptr<CollectionProbe>();
        first->order = 0;
        second->order = 1;
        replacement->order = 2;
        auto removedDraws = std::make_shared<unsigned>( 0 );
        second->observedDraws = removedDraws;
        WeakPtr<ui::IUIElement> removed( second );
        layout->addChild( first );
        layout->addChild( second );
        bool changed = false, retainedDuringFrame = false;
        first->onDraw = [&] {
            if( !changed )
            {
                changed = true;
                layout->removeChild( second );
                second = nullptr;
                retainedDuringFrame = !removed.expired();
                layout->addChild( replacement );
            }
        };

        auto ctx = manager->getContext();
        bool ok = check( ctx != nullptr, "headless native draw needs its real UI context" );
        if( ctx )
        {
            wp_input_end( ctx );
            const auto frame = [&] {
                if( wp_begin( ctx, reinterpret_cast<const wp_c8 *>( "CollectionSnapshot" ),
                              wp_make_rect( 0, 0, 320, 200 ), WORKPHONE_WINDOW_NO_SCROLLBAR ) )
                {
                    wp_layout_space_begin( ctx, WORKPHONE_STATIC, 200, 32 );
                    layout->draw( ctx );
                    wp_layout_space_end( ctx );
                }
                wp_end( ctx );
                wp_clear( ctx );
            };
            frame();
            ok &= check( changed && first->draws == 1,
                         "first native child must mutate membership from its draw callback" );
            ok &= check( retainedDuringFrame && *removedDraws == 1 && replacement->draws == 0,
                         "a native frame must draw its pinned members before newly added replacements" );
            ok &= check( removed.expired(), "the frame snapshot must release removed widgets after drawing" );
            frame();
            ok &= check( first->draws == 2 && *removedDraws == 1 && replacement->draws == 1,
                         "the following frame must use the replacement membership" );
        }
        first->onDraw = {};
        layout->removeAllChildren();
        layout->unload( nullptr );
        manager->clear();
        fixture.application->setRenderUI( nullptr );
        return ok;
    }

    bool testRemoveEachElement()
    {
        ui::ClawUIManager manager;
        const hash64 types[] = { ui::IUIButton::typeInfo(), ui::IUIImage::typeInfo(),
                                 ui::IUIText::typeInfo(), ui::IUITextEntry::typeInfo(),
                                 ui::IUICheckbox::typeInfo(), ui::IUIToggle::typeInfo(),
                                 ui::IUILayoutWindow::typeInfo(), ui::IUILayoutContainer::typeInfo() };
        bool ok = true;
        for( const auto type : types )
        {
            auto element = manager.addElement( type );
            ok &= check( element != nullptr, "each UI element must be created before removal" );
            if( !element )
                continue;
            WeakPtr<ui::IUIElement> weak( element );
            manager.removeElement( element );
            ok &= check( element->getLoadingState() == LoadingState::Unloaded,
                         "removing an element must unload it" );
            ok &= check( manager.getElements().empty(), "removal must release manager ownership" );
            manager.removeElement( element );
            element = nullptr;
            ok &= check( weak.expired(), "removed UI elements must actually be destroyed" );
        }
        return ok;
    }

    bool testSceneTextAppearance()
    {
        ui::ClawUIManager manager;
        auto element = manager.addElement( ui::IUIText::typeInfo() );
        auto textElement = dynamic_pointer_cast<ui::IUIText>( element );
        if( !textElement )
            return check( false, "text element must be created" );
        auto component = make_ptr<scene::Text>();
        component->setEnabled( true );
        component->setTextObject( textElement );
        component->setElement( element );
        const ColourF colour( 0.8f, 0.9f, 1.0f, 1.0f );
        component->setColour( colour );
        component->updateElementState();
        bool ok = check( element->getColour() == colour,
                         "text state updates must preserve the public colour setting" );
        const String telemetry( 2048, 'T' );
        component->setText( telemetry );
        ok &= check( component->getText() == telemetry && textElement->getText() == telemetry,
                     "HUD text longer than 128 characters must reach the render element intact" );
        auto properties = component->getProperties();
        String serializedText;
        properties->getPropertyValue( scene::Text::textPropertyStr, serializedText );
        ok &= check( serializedText == telemetry,
                     "long text must serialize without truncation" );
        component->setText( "short" );
        component->setProperties( properties );
        ok &= check( component->getText() == telemetry && textElement->getText() == telemetry,
                     "serialized long text must restore to the component and renderer" );
        ColourF serializedColour;
        properties->getPropertyValue( scene::Text::colourPropertyStr, serializedColour );
        ok &= check( serializedColour == colour,
                     "text properties must serialize the same colour that is rendered" );
        component->setHorizontalAlignment( static_cast<u8>( HorizontalAlignment::CENTER ) );
        component->setVerticalAlignment( static_cast<u8>( VerticalAlignment::CENTER ) );
        ok &= check( textElement->getHorizontalAlignment() == static_cast<u8>( HorizontalAlignment::CENTER ) &&
                     textElement->getVerticalAlignment() == static_cast<u8>( VerticalAlignment::CENTER ),
                     "alignment setters must immediately update the render element" );
        class VisibilityActor : public scene::GameActor
        {
        public:
            bool isEnabledInScene() const override { return enabledInScene; }
            bool isVisible() const override { return true; }
            bool enabledInScene = true;
        };
        auto actor = make_ptr<VisibilityActor>();
        component->setActor( actor );
        const auto enabledFlags = scene::IGameActor::ActorFlagEnabled |
                                  scene::IGameActor::ActorFlagEnabledInScene;
        actor->enabledInScene = false;
        component->updateFlags( scene::IGameActor::ActorFlagEnabled, enabledFlags );
        ok &= check( !element->isVisible() && !element->isEnabled(),
                     "hiding a UI parent must hide and disable its text" );
        actor->enabledInScene = true;
        component->updateFlags( enabledFlags, scene::IGameActor::ActorFlagEnabled );
        ok &= check( element->isVisible() && element->isEnabled(),
                     "showing a UI parent must restore its text" );
        component->setActor( nullptr );
        component->setTextObject( nullptr );
        component->setElement( nullptr );
        component = nullptr;
        manager.clear();
        return ok;
    }

    bool testEditorCameraDuringPlay( Fixture &fixture )
    {
        class CountingActor : public scene::GameActor
        {
        public:
            void update() override { ++updates; }
            u32 updates = 0;
        };
        auto actor = make_ptr<CountingActor>();
        scene::CameraManager manager;
        manager.load( nullptr );
        manager.setEditorCamera( actor );
        manager.setState( scene::ICameraManager::State::Play );
        Thread::setCurrentTask( TaskId::Application );
        fixture.application->setEditorCamera( true );
        manager.update();
        bool ok = check( actor->updates == 1,
                         "the selected editor camera must process transitions during Play" );
        fixture.application->setEditorCamera( false );
        manager.update();
        ok &= check( actor->updates == 1,
                     "an unselected editor camera must not consume game input" );
        manager.unload( nullptr );
        actor = nullptr;
        Thread::setCurrentTask( TaskId::Render );
        return ok;
    }

    bool testSceneButtonAppearance()
    {
        ui::ClawUIManager manager;
        auto element = manager.addElement( ui::IUIButton::typeInfo() );
        auto buttonElement = dynamic_pointer_cast<ui::IUIButton>( element );
        if( !buttonElement ) return check( false, "scene button needs a native renderer element" );
        class VisibilityActor : public scene::GameActor
        {
        public:
            bool isEnabledInScene() const override { return enabledInScene; }
            bool isVisible() const override { return visible; }
            bool enabledInScene = true, visible = true;
        };
        auto actor = make_ptr<VisibilityActor>();
        auto component = make_ptr<scene::Button>();
        component->setActor( actor );
        component->setUseNativeUI( true );
        component->setElement( element );
        component->setEnabled( true );
        const ColourF normal( 0.9f, 0.65f, 0.1f, 1.0f );
        const ColourF hover( 1.0f, 0.8f, 0.25f, 1.0f );
        const ColourF active( 0.65f, 0.4f, 0.02f, 1.0f );
        const ColourF disabled( 0.12f, 0.13f, 0.14f, 0.7f );
        const ColourF text( 0.02f, 0.025f, 0.03f, 1.0f );
        component->setNormalColour( normal );
        component->setHighlightedColour( hover );
        component->setPressedColour( active );
        component->setDisabledColour( disabled );
        component->setTextSize( 28 );
        component->setTextStr( "DRIVE" );
        component->setColour( text );
        const auto properties = buttonElement->getProperties();
        ColourF actualNormal, actualHover, actualActive, actualDisabled;
        properties->getPropertyValue( ui::IUIButton::normalColourStr, actualNormal );
        properties->getPropertyValue( ui::IUIButton::hoverColourStr, actualHover );
        properties->getPropertyValue( ui::IUIButton::activeColourStr, actualActive );
        properties->getPropertyValue( "disabledColour", actualDisabled );
        bool ok = check( actualNormal == normal && actualHover == hover && actualActive == active &&
                         actualDisabled == disabled,
                         "scene colour setters must immediately reach native button state" );
        ok &= check( buttonElement->getTextSize() == 28 && buttonElement->getLabel() == "DRIVE" &&
                     buttonElement->getColour() == text,
                     "scene button size, label, and dark label colour must reach the renderer" );
        component->setEnabled( false );
        ok &= check( element->isVisible() && !element->isEnabled(),
                     "a disabled button remains visible but cannot accept input" );
        actor->enabledInScene = false;
        component->setEnabled( true );
        ok &= check( !element->isVisible() && !element->isEnabled(),
                     "a disabled parent actor must still hide a button" );
        actor->enabledInScene = true;
        component->setEnabled( true );
        ok &= check( element->isVisible() && element->isEnabled(),
                     "showing the parent restores an enabled button" );
        component->setActor( nullptr );
        component->setElement( nullptr );
        component = nullptr;
        manager.clear();
        return ok;
    }

    bool testButtonDrawStyleIsolation()
    {
        ui::ClawUIManager manager;
        auto first = manager.addElement( ui::IUIButton::typeInfo() );
        auto second = manager.addElement( ui::IUIButton::typeInfo() );
        auto primary = dynamic_pointer_cast<ui::IUIButton>( first );
        auto secondary = dynamic_pointer_cast<ui::IUIButton>( second );
        auto primaryWidget = dynamic_cast<ui::IWorkphoneWidget *>( first.get() );
        auto secondaryWidget = dynamic_cast<ui::IWorkphoneWidget *>( second.get() );
        auto ctx = manager.getContext();
        if( !primary || !secondary || !primaryWidget || !secondaryWidget || !ctx )
            return check( false, "button draw contracts need real native widgets" );
        const ColourF amber( 0.9f, 0.65f, 0.1f, 1.0f ), hover( 1.0f, 0.8f, 0.3f, 1.0f );
        const ColourF pressed( 0.7f, 0.4f, 0.05f, 1.0f ), disabled( 0.12f, 0.13f, 0.14f, 0.7f );
        const ColourF gray( 0.2f, 0.22f, 0.24f, 1.0f ), dark( 0.02f, 0.03f, 0.04f, 1.0f );
        const auto appearance = [&]( ColourF normal ) {
            auto properties = make_ptr<Properties>();
            properties->setProperty( ui::IUIButton::normalColourStr, normal );
            properties->setProperty( ui::IUIButton::hoverColourStr, hover );
            properties->setProperty( ui::IUIButton::activeColourStr, pressed );
            properties->setProperty( "disabledColour", disabled );
            return properties;
        };
        primary->setProperties( appearance( amber ) );
        secondary->setProperties( appearance( gray ) );
        primary->setLabel( "DRIVE" );
        secondary->setLabel( "BACK" );
        primary->setTextSize( 28 ); secondary->setTextSize( 18 );
        primary->setColour( dark ); secondary->setColour( ColourF::White );
        primary->setPosition( Vector2F( 20, 20 ) ); secondary->setPosition( Vector2F( 20, 110 ) );
        primary->setSize( Vector2F( 240, 70 ) ); secondary->setSize( Vector2F( 240, 60 ) );
        primary->setHandleInputEvents( true ); secondary->setHandleInputEvents( true );
        const auto sameColour = []( wp_color a, wp_color b ) {
            return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
        };
        struct Snapshot { bool firstFill = false, secondFill = false, text = false;
            float firstHeight = 0, secondHeight = 0; wp_color firstText{}; wp_flags firstState = 0; };
        const auto frame = [&]( int mouseX, int mouseY, bool down, ColourF expected ) {
            Snapshot result;
            wp_input_begin( ctx );
            wp_input_motion( ctx, mouseX, mouseY );
            wp_input_button( ctx, WORKPHONE_BUTTON_LEFT, mouseX, mouseY, down );
            wp_input_end( ctx );
            if( wp_begin( ctx, reinterpret_cast<const wp_c8 *>( "Button appearance" ),
                          wp_make_rect( 0, 0, 500, 300 ),
                          WORKPHONE_WINDOW_NO_SCROLLBAR | WORKPHONE_WINDOW_BACKGROUND ) )
            {
                wp_layout_space_begin( ctx, WORKPHONE_STATIC, 300, 2 );
                primaryWidget->draw( ctx );
                result.firstState = ctx->last_widget_state;
                secondaryWidget->draw( ctx );
                wp_layout_space_end( ctx );
            }
            wp_end( ctx );
            const wp_command *command;
            wp_foreach( command, ctx )
            {
                if( command->type == WORKPHONE_COMMAND_RECT_FILLED )
                {
                    auto rect = reinterpret_cast<const wp_command_rect_filled *>( command );
                    result.firstFill |= sameColour( rect->color,
                        ui::ClawUIWorkphoneContext::toWorkphoneColor( expected ) );
                    result.secondFill |= sameColour( rect->color,
                        ui::ClawUIWorkphoneContext::toWorkphoneColor( gray ) );
                }
                if( command->type == WORKPHONE_COMMAND_TEXT )
                {
                    auto text = reinterpret_cast<const wp_command_text *>( command );
                    if( text->length == 5 && std::strncmp( text->string, "DRIVE", 5 ) == 0 )
                    {
                        result.text = true; result.firstHeight = text->height;
                        result.firstText = text->foreground;
                        // The saved font descriptor must survive the second widget draw.
                        result.firstFill &= text->font && text->font->height == text->height;
                    }
                    if( text->length == 4 && std::strncmp( text->string, "BACK", 4 ) == 0 )
                        result.secondHeight = text->height;
                }
            }
            wp_clear( ctx );
            return result;
        };
        const auto sharedStyle = ctx->style.button;
        const auto sharedFont = ctx->style.font;
        auto normal = frame( 400, 240, false, amber );
        bool ok = check( normal.firstFill && normal.secondFill && normal.text &&
                         normal.firstHeight == 28 && normal.secondHeight == 18 &&
                         sameColour( normal.firstText, ui::ClawUIWorkphoneContext::toWorkphoneColor( dark ) ),
                         "two buttons must draw independent backgrounds, font sizes, and label colours" );
        ok &= check( std::memcmp( &sharedStyle, &ctx->style.button, sizeof sharedStyle ) == 0 &&
                     ctx->style.font == sharedFont,
                     "button drawing must leave shared style and font unchanged" );
        auto hovered = frame( 100, 50, false, hover );
        ok &= check( hovered.firstFill && hovered.secondFill,
                     "hover changes only the hovered button background" );
        auto active = frame( 100, 50, true, pressed );
        ok &= check( active.firstFill, "held pointer draws the authored pressed colour" );
        frame( 100, 50, false, hover );
        primary->setEnabled( false );
        auto inactive = frame( 100, 50, true, disabled );
        ok &= check( inactive.firstFill && inactive.text && inactive.firstText.a < normal.firstText.a &&
                     inactive.firstText.r > normal.firstText.r &&
                     !( inactive.firstState & ( WORKPHONE_WIDGET_STATE_ACTIVED | WORKPHONE_WIDGET_STATE_ACTIVE ) ),
                     "disabled controls draw their disabled colour and subdued text without activation" );
        primary->setEnabled( true ); primary->setVisible( false );
        auto hidden = frame( 400, 240, false, amber );
        ok &= check( !hidden.text && hidden.secondFill,
                     "hidden buttons emit no text and do not affect visible neighbours" );
        manager.clear();
        return ok;
    }

    bool testReadonlySlider( Direction direction )
    {
        ui::ClawUIManager manager;
        auto element = manager.addElement( ui::IUISlider::typeInfo() );
        auto slider = dynamic_pointer_cast<ui::IUISlider>( element );
        auto widget = dynamic_cast<ui::IWorkphoneWidget *>( element.get() );
        auto ctx = manager.getContext();
        if( !slider || !widget || !ctx ) return check( false, "readonly slider needs a real native widget" );
        const auto vertical = direction == Direction::Vertical;
        slider->setDirection( direction );
        slider->setMinValue( -100 );
        slider->setMaxValue( 100 );
        slider->setPosition( { 20, 20 } );
        slider->setSize( vertical ? Vector2F( 44, 240 ) : Vector2F( 300, 44 ) );
        slider->setHandleInputEvents( false );
        const auto cursorColour = wp_rgb( 193, 37, 173 );
        ctx->style.slider.cursor_normal = ctx->style.slider.cursor_hover = ctx->style.slider.cursor_active =
            wp_style_item_color( cursorColour );
        ctx->style.slider.color_factor = 1;
        const auto sharedStyle = ctx->style.slider;
        const auto sharedFont = ctx->style.font;
        struct Snapshot { float x = 0, y = 0; bool cursor = false, restored = true; };
        const auto frame = [&]( int x, int y, bool down, bool inheritedReadonly = false ) {
            Snapshot result;
            wp_input_begin( ctx );
            wp_input_motion( ctx, x, y );
            wp_input_button( ctx, WORKPHONE_BUTTON_LEFT, x, y, down );
            wp_input_end( ctx );
            if( wp_begin( ctx, reinterpret_cast<const wp_c8 *>( "Readonly axis slider" ),
                          wp_make_rect( 0, 0, 500, 300 ),
                          WORKPHONE_WINDOW_NO_SCROLLBAR | WORKPHONE_WINDOW_BACKGROUND ) )
            {
                wp_layout_space_begin( ctx, WORKPHONE_STATIC, 280, 1 );
                auto panel = ctx->current->layout;
                const auto originalFlags = panel->flags;
                if( inheritedReadonly ) panel->flags |= WORKPHONE_WINDOW_ROM;
                const auto incomingFlags = panel->flags;
                widget->draw( ctx );
                result.restored = panel->flags == incomingFlags && ctx->style.font == sharedFont &&
                                 std::memcmp( &sharedStyle, &ctx->style.slider, sizeof sharedStyle ) == 0;
                panel->flags = originalFlags;
                wp_layout_space_end( ctx );
            }
            wp_end( ctx );
            const wp_command *command;
            wp_foreach( command, ctx )
            {
                wp_color colour{};
                float x = 0, y = 0;
                if( vertical && command->type == WORKPHONE_COMMAND_RECT_FILLED )
                {
                    const auto cursor = reinterpret_cast<const wp_command_rect_filled *>( command );
                    colour = cursor->color; x = cursor->x + cursor->w * 0.5f; y = cursor->y + cursor->h * 0.5f;
                }
                else if( !vertical && command->type == WORKPHONE_COMMAND_CIRCLE_FILLED )
                {
                    const auto cursor = reinterpret_cast<const wp_command_circle_filled *>( command );
                    colour = cursor->color; x = cursor->x + cursor->w * 0.5f; y = cursor->y + cursor->h * 0.5f;
                }
                if( colour.r == cursorColour.r && colour.g == cursorColour.g && colour.b == cursorColour.b && colour.a == 255 )
                {
                    result.cursor = true; result.x = x; result.y = y;
                }
            }
            wp_clear( ctx );
            return result;
        };
        bool ok = true;
        float previousPosition = -1;
        for( const auto value : { -100.0f, -50.0f, 0.0f, 50.0f, 100.0f } )
        {
            slider->setValue( value );
            const auto visible = frame( 450, 290, false );
            const auto position = vertical ? visible.y : visible.x;
            ok &= check( visible.cursor && visible.restored && slider->isEnabled() && slider->getValue() == value &&
                         position > previousPosition,
                         "readonly axis sliders must visibly track negative, centered and positive programmatic values" );
            previousPosition = position;
        }
        slider->setValue( -150 );
        ok &= check( slider->getValue() == -100, "axis slider must clamp updates below -100 percent" );
        slider->setValue( 150 );
        ok &= check( slider->getValue() == 100, "axis slider must clamp updates above +100 percent" );
        slider->setValue( 0 );
        auto center = frame( 450, 290, false );
        const auto x = static_cast<int>( center.x ), y = static_cast<int>( center.y );
        frame( x, y, true );
        auto dragged = frame( x + ( vertical ? 0 : 80 ), y + ( vertical ? 60 : 0 ), true );
        frame( x, y, false );
        ok &= check( dragged.restored && slider->getValue() == 0 && !slider->isDragging(),
                     "readonly axis slider must ignore pointer dragging without leaking shared flags" );
        slider->setHandleInputEvents( true );
        frame( x, y, true );
        auto interactive = frame( x + ( vertical ? 0 : 80 ), y + ( vertical ? 60 : 0 ), true );
        frame( x, y, false );
        ok &= check( interactive.restored && slider->getValue() > 20,
                     "reenabling input must restore ordinary native slider interaction" );
        slider->setValue( 0 );
        frame( x, y, true, true );
        const auto inherited = frame( x + ( vertical ? 0 : 80 ), y + ( vertical ? 60 : 0 ), true, true );
        frame( x, y, false );
        ok &= check( inherited.restored && slider->getValue() == 0,
                     "an interactive slider must preserve and obey inherited readonly window flags" );
        if( vertical )
        {
            slider->setHandleInputEvents( false );
            slider->setPosition( { 900, 900 } );
            const auto clipped = frame( x, y, true );
            ok &= check( !clipped.cursor && clipped.restored && !slider->isDragging(),
                         "a clipped vertical readonly slider must restore flags on its early return" );
        }
        manager.clear();
        return ok;
    }

    bool testSceneReadonlySlider()
    {
        class Actor : public scene::GameActor
        {
        public:
            bool isEnabledInScene() const override { return true; }
            bool isVisible() const override { return true; }
        };
        class Slider : public scene::Slider
        {
        public:
            using scene::Slider::updateElementState;
        };
        ui::ClawUIManager manager;
        auto element = manager.addElement( ui::IUISlider::typeInfo() );
        auto native = dynamic_pointer_cast<ui::IUISlider>( element );
        if( !native ) return check( false, "scene readonly slider needs a native renderer element" );
        auto actor = make_ptr<Actor>();
        auto slider = make_ptr<Slider>();
        slider->setActor( actor );
        slider->setElement( element );
        slider->setEnabled( true );
        slider->setMinValue( -100 );
        slider->setMaxValue( 100 );
        slider->setCascadeInput( false );
        slider->setValue( -75 );
        bool ok = check( !native->getHandleInputEvents() && native->isEnabled() && native->getValue() == -75 &&
                         native->getMinValue() == -100 && native->getMaxValue() == 100,
                         "scene cascadeInput=false must publish a readonly signed slider while keeping it enabled" );
        slider->setValue( 75 );
        slider->setDirection( Direction::Vertical );
        ok &= check( native->getValue() == 75 && native->getDirection() == Direction::Vertical && !native->getHandleInputEvents(),
                     "scene programmatic axis updates and direction must reach the readonly native slider" );
        slider->setCascadeInput( true );
        slider->updateElementState();
        ok &= check( native->getHandleInputEvents(), "scene slider must restore native interaction when cascade input is enabled" );
        slider->setElement( nullptr );
        slider->setActor( nullptr );
        element->setOwner( nullptr );
        slider = nullptr;
        manager.clear();
        return ok;
    }

    bool testDropdownTextStyleIsolation()
    {
        ui::ClawUIManager manager;
        auto first = manager.addElement( ui::IUIDropdown::typeInfo() );
        auto second = manager.addElement( ui::IUIDropdown::typeInfo() );
        auto primary = dynamic_pointer_cast<ui::IUIDropdown>( first );
        auto secondary = dynamic_pointer_cast<ui::IUIDropdown>( second );
        auto primaryWidget = dynamic_cast<ui::IWorkphoneWidget *>( first.get() );
        auto secondaryWidget = dynamic_cast<ui::IWorkphoneWidget *>( second.get() );
        auto ctx = manager.getContext();
        if( !primary || !secondary || !primaryWidget || !secondaryWidget || !ctx )
            return check( false, "dropdown font contracts need real native widgets" );
        primary->setOptions( { "High", "Ultra" } );
        secondary->setOptions( { "Low", "Medium" } );
        primary->setSelectedOption( 0 );
        secondary->setSelectedOption( 0 );
        auto properties = make_ptr<Properties>();
        properties->setProperty( "textSize", 20 );
        primary->setProperties( properties );
        properties->setProperty( "textSize", 16 );
        secondary->setProperties( properties );
        primary->setPosition( { 20, 20 } );
        secondary->setPosition( { 20, 110 } );
        primary->setSize( { 300, 44 } );
        secondary->setSize( { 300, 44 } );
        // Match the real game root: its window background is transparent, while
        // an expanded dropdown must cover controls drawn beneath its popup.
        const auto transparent = wp_rgba( 0, 0, 0, 0 );
        const auto popupColour = wp_rgba( 31, 37, 43, 255 );
        ctx->style.window.fixed_background = wp_style_item_color( transparent );
        ctx->style.window.background = transparent;
        ctx->style.combo.normal = wp_style_item_color( wp_rgba( 31, 37, 43, 160 ) );
        const auto sharedFont = ctx->style.font;
        const auto sharedStyle = ctx->style.combo;
        const auto fixedBackground = ctx->style.window.fixed_background;
        const auto background = ctx->style.window.background;
        const auto backgroundRestored = [&] {
            return std::memcmp( &fixedBackground, &ctx->style.window.fixed_background, sizeof fixedBackground ) == 0 &&
                   std::memcmp( &background, &ctx->style.window.background, sizeof background ) == 0;
        };
        bool restored = true;
        struct Snapshot { float first = 0, second = 0, option = 0; bool persistent = true;
            unsigned secondaryCommand = 0, opaquePopupCommand = 0; };
        const auto frame = [&]( int x, int y, bool down ) {
            Snapshot result;
            wp_input_begin( ctx );
            wp_input_motion( ctx, x, y );
            wp_input_button( ctx, WORKPHONE_BUTTON_LEFT, x, y, down );
            wp_input_end( ctx );
            if( wp_begin( ctx, reinterpret_cast<const wp_c8 *>( "Dropdown fonts" ),
                          wp_make_rect( 0, 0, 500, 340 ),
                          WORKPHONE_WINDOW_NO_SCROLLBAR | WORKPHONE_WINDOW_BACKGROUND ) )
            {
                wp_layout_space_begin( ctx, WORKPHONE_STATIC, 320, 2 );
                primaryWidget->draw( ctx );
                restored &= ctx->style.font == sharedFont && backgroundRestored();
                secondaryWidget->draw( ctx );
                restored &= ctx->style.font == sharedFont && backgroundRestored();
                wp_layout_space_end( ctx );
            }
            wp_end( ctx );
            const wp_command *command;
            unsigned commandIndex = 0;
            wp_foreach( command, ctx )
            {
                ++commandIndex;
                if( command->type == WORKPHONE_COMMAND_RECT_FILLED )
                {
                    auto rect = reinterpret_cast<const wp_command_rect_filled *>( command );
                    // Combo panels are dynamic: Core emits opaque row fills
                    // rather than one fixed body rectangle. This point lies in
                    // the lower dropdown header covered by the open popup.
                    if( rect->w >= 280 && rect->h >= 24 && rect->x <= 60 &&
                        rect->x + rect->w >= 60 && rect->y <= 140 && rect->y + rect->h >= 140 &&
                        rect->color.r == popupColour.r && rect->color.g == popupColour.g &&
                        rect->color.b == popupColour.b && rect->color.a == 255 )
                        result.opaquePopupCommand = commandIndex;
                }
                if( command->type != WORKPHONE_COMMAND_TEXT ) continue;
                auto text = reinterpret_cast<const wp_command_text *>( command );
                if( text->length == 4 && std::strncmp( text->string, "High", 4 ) == 0 )
                    result.first = text->height;
                if( text->length == 3 && std::strncmp( text->string, "Low", 3 ) == 0 )
                {
                    result.second = text->height;
                    result.secondaryCommand = commandIndex;
                }
                if( text->length == 5 && std::strncmp( text->string, "Ultra", 5 ) == 0 )
                    result.option = text->height;
                result.persistent &= text->font && text->font->height == text->height;
            }
            wp_clear( ctx );
            return result;
        };
        auto closed = frame( 450, 300, false );
        bool ok = check( closed.first == 20 && closed.second == 16 && closed.persistent,
                         "dropdown textSize properties must draw independent persistent font sizes" );
        frame( 60, 42, true );
        auto opened = frame( 60, 42, false );
        ok &= check( opened.option == 20 && opened.persistent,
                     "opened dropdown options must retain the same authored font as their header" );
        ok &= check( !closed.opaquePopupCommand && opened.opaquePopupCommand > opened.secondaryCommand &&
                     opened.secondaryCommand > 0,
                     "expanded dropdown must emit an opaque popup panel after the controls underneath it" );
        ok &= check( restored && ctx->style.font == sharedFont &&
                     backgroundRestored() &&
                     std::memcmp( &sharedStyle, &ctx->style.combo, sizeof sharedStyle ) == 0 &&
                     primary->getProperties()->getPropertyAsInt( "textSize" ) == 20 &&
                     secondary->getProperties()->getPropertyAsInt( "textSize" ) == 16,
                     "dropdown rendering must restore both window backgrounds, shared style and each font property" );
        manager.clear();
        return ok;
    }

    struct FontConversionProbe
    {
        Array<WeakPtr<ui::IUIElement>> retired;
        wp_user_font original{};
        unsigned queries = 0;
        bool aliveDuringConversion = true;
    };
    FontConversionProbe *activeFontConversionProbe = nullptr;

    void queryRetainedFont( wp_handle, float height, wp_user_font_glyph *glyph,
                            wp_rune codepoint, wp_rune next )
    {
        auto &probe = *activeFontConversionProbe;
        ++probe.queries;
        for( const auto &owner : probe.retired )
            probe.aliveDuringConversion &= !owner.expired();
        // The original atlas belongs to the UI context throughout this test.
        probe.original.query( probe.original.userdata, height, glyph, codepoint, next );
    }

    bool testDropdownFrameFontLifetime( Fixture &fixture, bool lateOwners = false )
    {
        class Window : public render::GraphicsWindow
        {
        public:
            Vector2I getSize() const override { return { 500, 300 }; }
        };
        auto previousWindow = fixture.application->getWindow();
        fixture.application->setWindow( make_ptr<Window>() );
        ui::ClawUIManager manager;
        auto root = dynamic_pointer_cast<ui::IUILayoutWindow>( manager.addElement( ui::IUILayoutWindow::typeInfo() ) );
        auto retire = make_ptr<CollectionProbe>();
        auto ctx = manager.getContext();
        if( !root || !ctx || !ctx->style.font || !ctx->style.font->query )
        {
            fixture.application->setWindow( previousWindow );
            return check( false, "font lifetime test needs a real canvas and baked atlas" );
        }
        root->load( nullptr );
        root->setSize( { 1, 1 } );
        retire->order = 3;
        FontConversionProbe probe;
        Array<SmartPtr<ui::IUIElement>> elements;
        const auto createFontOwners = [&] {
            auto dropdown = dynamic_pointer_cast<ui::IUIDropdown>( manager.addElement( ui::IUIDropdown::typeInfo() ) );
            auto text = dynamic_pointer_cast<ui::IUIText>( manager.addElement( ui::IUIText::typeInfo() ) );
            auto button = dynamic_pointer_cast<ui::IUIButton>( manager.addElement( ui::IUIButton::typeInfo() ) );
            if( !dropdown || !text || !button ) return false;
            dropdown->setOptions( { "HIGH", "ULTRA" } );
            dropdown->setSelectedOption( 0 );
            text->setText( "TEXT" );
            text->setTextSize( 20 );
            button->setLabel( "BUTTON" );
            button->setTextSize( 20 );
            elements = { dropdown, text, button };
            for( size_t i = 0; i < elements.size(); ++i )
            {
                auto &element = elements[i];
                element->setPosition( { 0.04f, ( 20.0f + 60.0f * static_cast<float>( i ) ) / 300.0f } );
                element->setSize( { 0.6f, 44.0f / 300.0f } );
                element->setOrder( static_cast<u32>( i ) );
                root->addChild( element );
                probe.retired.emplace_back( element );
            }
            return true;
        };
        bool created = lateOwners || createFontOwners();
        root->addChild( retire );
        const auto previousFont = ctx->style.font;
        probe.original = *previousFont;
        auto tracingFont = probe.original;
        tracingFont.query = queryRetainedFont;
        ctx->style.font = &tracingFont;
        activeFontConversionProbe = &probe;
        retire->onDraw = [&] {
            if( lateOwners )
            {
                // These widgets were absent from both the manager's inventory
                // and the layout's child snapshot at the beginning of this draw.
                created = createFontOwners();
                for( auto &element : elements )
                    dynamic_cast<ui::IWorkphoneWidget *>( element.get() )->draw( ctx );
            }
            for( const auto &element : elements ) manager.removeElement( element );
            elements.clear();
        };
        manager.render();
        ctx->style.font = previousFont;
        activeFontConversionProbe = nullptr;
        bool ok = check( created && retire->draws == 1 && probe.retired.size() == 3 && probe.queries >= 14,
                         lateOwners ? "late-created dropdown, text and button must retire before real CPU font conversion"
                                    : "later draw must retire dropdown, text and button before real CPU font conversion" );
        ok &= check( probe.aliveDuringConversion,
                     lateOwners ? "actual font emitters must retain late-created owners until conversion finishes"
                                : "frame inventory must retain retired font owners until conversion finishes" );
        bool released = true;
        for( const auto &owner : probe.retired ) released &= owner.expired();
        ok &= check( released, "frame ownership must release all retired fonts once conversion finishes" );
        retire->onDraw = {};
        manager.clear();
        fixture.application->setWindow( previousWindow );
        return ok;
    }

    bool testChildRemoval()
    {
        ui::ClawUIManager manager;
        auto parent = manager.addElement( ui::IUILayoutWindow::typeInfo() );
        auto child = manager.addElement( ui::IUIButton::typeInfo() );
        if( !parent || !child )
            return check( false, "parent and child must be created" );
        parent->addChild( child );
        WeakPtr<ui::IUIElement> weakChild( child );
        manager.removeElement( child );
        bool ok = check( parent->getChildren().empty() && !child->getParent() && !child->getLayout(),
                         "removal must detach both sides of a child relationship" );
        child = nullptr;
        ok &= check( weakChild.expired(), "a removed child must die while its parent remains alive" );
        WeakPtr<ui::IUIElement> weakParent( parent );
        manager.removeElement( parent );
        parent = nullptr;
        ok &= check( weakParent.expired(), "layout roots must not retain themselves after unload" );
        return ok;
    }

    bool testBulkRemoval()
    {
        ui::ClawUIManager manager;
        auto first = manager.addElement( ui::IUIButton::typeInfo() );
        auto second = manager.addElement( ui::IUIText::typeInfo() );
        auto survivor = manager.addElement( ui::IUIImage::typeInfo() );
        if( !first || !second || !survivor )
            return check( false, "bulk removal elements must be created" );
        WeakPtr<ui::IUIElement> weakFirst( first ), weakSecond( second ), weakSurvivor( survivor );
        manager.removeElements( { first, nullptr, second, first } );
        first = nullptr;
        second = nullptr;
        bool ok = check( weakFirst.expired() && weakSecond.expired(),
                         "bulk removal must destroy removed elements, including duplicate entries" );
        ok &= check( manager.getElements().size() == 1 && !weakSurvivor.expired(),
                     "bulk removal must preserve unrelated elements" );
        manager.clear();
        ok &= check( !weakSurvivor.expired() && survivor->getLoadingState() == LoadingState::Unloaded,
                     "clear must unload but preserve elements still owned by the caller" );
        survivor = nullptr;
        ok &= check( weakSurvivor.expired(), "cleared elements must die after the caller releases them" );
        return ok;
    }

    class TrackingInputManager : public InputDeviceManager
    {
    public:
        void addListener( SmartPtr<IEventListener> listener ) override
        {
            InputDeviceManager::addListener( listener );
            registered = listener;
            ++listenerCount;
        }
        void removeListener( SmartPtr<IEventListener> listener ) override
        {
            InputDeviceManager::removeListener( listener );
            --listenerCount;
        }
        WeakPtr<IEventListener> registered;
        int listenerCount = 0;
    };

    bool testInputListenerCleanup( Fixture &fixture )
    {
        auto input = make_ptr<TrackingInputManager>();
        fixture.application->setInputDeviceManager( input );
        auto manager = make_ptr<ui::ClawUIManager>();
        bool ok = check( input->listenerCount == 1 && !input->registered.expired(),
                         "UI manager must register one input listener" );
        manager->load( nullptr );
        ok &= check( input->listenerCount == 1, "load must not duplicate the input listener" );
        manager->unload( nullptr );
        manager->unload( nullptr );
        ok &= check( input->listenerCount == 0 && input->registered.expired(),
                     "repeated unload must unregister and destroy the input listener once" );
        manager->load( nullptr );
        ok &= check( input->listenerCount == 1 && !input->registered.expired(),
                     "loading after unload must register a new input listener" );
        manager = nullptr;
        ok &= check( input->listenerCount == 0 && input->registered.expired(),
                     "manager destruction must not leave a dangling input callback" );
        fixture.application->setInputDeviceManager( nullptr );
        return ok;
    }

    bool testUiUtilButtonOwnInput( Fixture &fixture )
    {
        // Use the real UiUtil factory and layout-state allocation while keeping
        // this test independent of scene files, graphics devices, and FSM ticks.
        class VisibleActor : public scene::GameActor
        {
        public:
            bool isEnabledInScene() const override { return true; }
            bool isVisible() const override { return true; }
        };
        class DetachedGameManager : public scene::GameManager
        {
        public:
            SmartPtr<scene::IGameActor> createActor() override { return make_ptr<VisibleActor>(); }
            void destroyActor( SmartPtr<scene::IGameActor> actor, bool = true ) override
            {
                for( auto component : actor->getComponents() ) actor->removeComponentInstance( component );
            }
        };
        class ClickCounter : public IEventListener
        {
        public:
            Parameter handleEvent( EventType type, hash_type value, const Array<Parameter> &,
                SmartPtr<ISharedObject>, SmartPtr<ISharedObject>, SmartPtr<IEvent> ) override
            {
                if( type == EventType::UI && value == IEvent::CLICK_HASH ) ++clicks;
                return {};
            }
            unsigned clicks = 0;
        };
        auto previousGame = fixture.application->getGameManager();
        auto previousFactories = fixture.application->getFactoryManager();
        auto factories = make_ptr<FactoryManager>();
        factories->load( nullptr );
        fixture.application->setFactoryManager( factories );
        auto game = make_ptr<DetachedGameManager>();
        game->setLoadingState( LoadingState::Loaded );
        fixture.application->setGameManager( game );
        auto system = make_ptr<scene::LayoutTransformSystem>();
        system->load( nullptr );
        game->addSystem( scene::LayoutTransform::typeInfo(), system );
        FactoryUtil::addFactory<scene::LayoutTransform>();
        FactoryUtil::addFactory<scene::Button>();
        ui::ClawUIManager manager;
        auto root = manager.addElement( ui::IUILayoutWindow::typeInfo() );
        auto native = manager.addElement( ui::IUIButton::typeInfo() );
        root->load( nullptr );
        auto actor = scene::UiUtil::createButton( "SETTINGS", false );
        auto button = actor->getComponent<scene::Button>();
        button->setUseNativeUI( true );
        button->setElement( native );
        button->setEnabled( true );
        button->setTextStr( "SETTINGS" );
        auto counter = make_ptr<ClickCounter>();
        native->addObjectListener( counter );
        root->addChild( native );
        native->setPosition( Vector2F( 20, 20 ) );
        native->setSize( Vector2F( 240, 70 ) );
        auto widget = dynamic_cast<ui::IWorkphoneWidget *>( root.get() );
        auto ctx = manager.getContext();
        bool ok = check( !button->getCascadeInput() && !native->getHandleInputEvents(),
            "UiUtil buttons use non-cascading input by default" );
        const auto frame = [&]( bool down ) {
            wp_input_begin( ctx );
            wp_input_motion( ctx, 100, 50 );
            wp_input_button( ctx, WORKPHONE_BUTTON_LEFT, 100, 50, down );
            wp_input_end( ctx );
            if( wp_begin( ctx, reinterpret_cast<const wp_c8 *>( "UiUtil root input" ),
                          wp_make_rect( 0, 0, 500, 300 ),
                          WORKPHONE_WINDOW_NO_SCROLLBAR | WORKPHONE_WINDOW_BACKGROUND ) )
            {
                wp_layout_space_begin( ctx, WORKPHONE_STATIC, 300, 1 );
                widget->draw( ctx );
                wp_layout_space_end( ctx );
            }
            wp_end( ctx );
            const auto state = ctx->last_widget_state;
            wp_clear( ctx );
            return state;
        };
        const auto hover = frame( false );
        ok &= check( ( hover & WORKPHONE_WIDGET_STATE_HOVER ) != 0,
            "a UiUtil button inside the native root layout must hover despite cascade=false" );
        frame( true ); frame( false );
        ok &= check( counter->clicks == 1,
            "a UiUtil button inside the native root layout must receive exactly one native click" );
        button->setEnabled( false );
        frame( true ); frame( false );
        ok &= check( counter->clicks == 1,
            "a disabled UiUtil button must suppress native clicks" );
        native->removeObjectListener( counter );
        button->setElement( nullptr );
        native->setOwner( nullptr );
        root->removeChild( native );
        manager.clear();
        button = nullptr;
        // Component::unload releases its layout-system slot. Remove the
        // manager routing first so actor detachment only clears the registry.
        game->removeSystem( scene::LayoutTransform::typeInfo() );
        game->destroyActor( actor ); actor = nullptr;
        system->unload( nullptr );
        game->setLoadingState( LoadingState::Unloaded );
        FactoryUtil::removeFactory<scene::Button>();
        FactoryUtil::removeFactory<scene::LayoutTransform>();
        fixture.application->setGameManager( previousGame );
        factories->unload( nullptr );
        fixture.application->setFactoryManager( previousFactories );
        return ok;
    }

    enum class Cleanup { Clear, Unload, Destructor };

    bool testTreeCleanup( Cleanup cleanup )
    {
        auto manager = make_ptr<ui::ClawUIManager>();
        manager->load( nullptr );
        auto root = manager->addElement( ui::IUILayoutWindow::typeInfo() );
        auto container = manager->addElement( ui::IUILayoutContainer::typeInfo() );
        auto leaf = manager->addElement( ui::IUIText::typeInfo() );
        if( !root || !container || !leaf )
            return check( false, "nested UI tree must be created" );
        root->addChild( container );
        container->addChild( leaf );
        WeakPtr<ui::IUIElement> weakRoot( root ), weakContainer( container ), weakLeaf( leaf );
        root = nullptr;
        container = nullptr;
        leaf = nullptr;
        bool ok = check( !weakRoot.expired() && !weakContainer.expired() && !weakLeaf.expired(),
                         "manager must own the tree before cleanup" );
        if( cleanup == Cleanup::Clear )
        {
            manager->clear();
            manager->clear();
            ok &= check( manager->getElements().empty(), "clear must empty the element registry" );
        }
        else if( cleanup == Cleanup::Unload )
        {
            manager->unload( nullptr );
            manager->unload( nullptr );
            ok &= check( manager->getElements().empty() && !manager->isLoaded(),
                         "unload must empty the registry and be safe to repeat" );
        }
        else
        {
            manager = nullptr;
        }
        ok &= check( weakRoot.expired() && weakContainer.expired() && weakLeaf.expired(),
                     "tree cleanup must destroy the layout, container, and leaf" );
        return ok;
    }
}

int main( int argc, char **argv )
{
    Fixture fixture;
    if( argc > 1 && std::strcmp( argv[1], "--traversal-only" ) == 0 )
    {
        try { ui_layout_traversal_contracts::run( fixture.application ); return 0; }
        catch( const std::exception &error ) { std::fprintf( stderr, "%s\n", error.what() ); return 1; }
    }
    if( argc > 1 && std::strcmp( argv[1], "--input-dispatch-only" ) == 0 )
    {
        try { input_listener_dispatch_contracts::run(); return 0; }
        catch( const std::exception &error ) { std::fprintf( stderr, "%s\n", error.what() ); return 1; }
    }
    if( argc > 1 && std::strcmp( argv[1], "--scene-dropdown-lifetime-only" ) == 0 )
    {
        try { scene_dropdown_lifetime_contracts::run(); return 0; }
        catch( const std::exception &error ) { std::fprintf( stderr, "%s\n", error.what() ); return 1; }
    }
    if( argc >= 4 && std::strcmp( argv[1], "--project-font-only" ) == 0 )
    {
        // Optional asset diagnostics: callers supply a project with a valid
        // Default.ttf and a private fixture project containing an invalid TTF.
        // The test never creates or edits project/font files itself.
        bool ok = testStartupFont( fixture, String(), false );
        ok &= testStartupFont( fixture, String( argv[2] ) + "/NoSuchFontProject", false );
        ok &= testStartupFont( fixture, String( argv[2] ), true );
        ok &= testStartupFont( fixture, String( argv[2] ), true, true );
        for( int index = 3; index < argc; ++index )
            ok &= testStartupFont( fixture, String( argv[index] ), false );
        std::printf( "Claw UI startup font contracts: %d cases, %u assertions, %s.\n", argc + 1, assertions,
                     ok ? "passed" : "failed" );
        return ok ? 0 : 1;
    }
    if( argc > 1 && std::strcmp( argv[1], "--collection-sort-only" ) == 0 )
    {
        bool ok = testSortingExcludesOtherThreads();
        ok &= testSortCapturesEachOrderOnce();
        std::printf( "Claw UI sorting contracts: 2 cases, %u assertions, %s.\n", assertions,
                     ok ? "passed" : "failed" );
        return ok ? 0 : 1;
    }
    if( argc > 1 && std::strcmp( argv[1], "--readonly-sliders-only" ) == 0 )
    {
        bool ok = testReadonlySlider( Direction::Horizontal );
        ok &= testReadonlySlider( Direction::Vertical );
        ok &= testSceneReadonlySlider();
        std::printf( "Readonly slider contracts: 3 cases, %u assertions, %s.\n", assertions, ok ? "passed" : "failed" );
        return ok ? 0 : 1;
    }
    bool ok = testRemoveEachElement();
    ok &= testSceneTextAppearance();
    ok &= testSceneButtonAppearance();
    ok &= testButtonDrawStyleIsolation();
    ok &= testReadonlySlider( Direction::Horizontal );
    ok &= testReadonlySlider( Direction::Vertical );
    ok &= testSceneReadonlySlider();
    ok &= testDropdownTextStyleIsolation();
    ok &= testDropdownFrameFontLifetime( fixture );
    ok &= testDropdownFrameFontLifetime( fixture, true );
    try { assertions += ui_layout_traversal_contracts::run( fixture.application ); }
    catch( const std::exception &error ) { ok &= check( false, error.what() ); }
    ok &= testUiUtilButtonOwnInput( fixture );
    ok &= testEditorCameraDuringPlay( fixture );
    ok &= testChildRemoval();
    ok &= testBulkRemoval();
    ok &= testTreeCleanup( Cleanup::Clear );
    ok &= testTreeCleanup( Cleanup::Unload );
    ok &= testTreeCleanup( Cleanup::Destructor );
    ok &= testInputListenerCleanup( fixture );
    try { assertions += input_listener_dispatch_contracts::run(); }
    catch( const std::exception &error ) { ok &= check( false, error.what() ); }
    try { assertions += scene_dropdown_lifetime_contracts::run(); }
    catch( const std::exception &error ) { ok &= check( false, error.what() ); }
    ok &= testSortingExcludesOtherThreads();
    ok &= testSortCapturesEachOrderOnce();
    ok &= testDrawPinsItsFrameMembers( fixture );
    if( ok )
        std::printf( "Claw UI destruction tests: 28 cases, %u assertions passed.\n", assertions );
    return ok ? 0 : 1;
}
