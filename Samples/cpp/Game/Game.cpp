#include "Game.h"
#include "Types.h"
#include <Workphone/Workphone.hpp>
#include <WPSQLite/WPSQLite.hpp>

#if WP_BUILD_PHYSX
#    include <WPPhysx/WPPhysx.hpp>
#elif WP_BUILD_ODE
#    include <WPODE3/CPhysicsManagerODE.hpp>
#endif

namespace workphone
{

    Game::Game() = default;

    Game::~Game() = default;

    void Game::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto task = TaskId::Primary;
            Thread::setCurrentTask( task );

            auto threadId = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( threadId );

            auto applicationManager = new core::ApplicationManager;
            core::ApplicationManager::setInstance( applicationManager );

            Application::load( data );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Game::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            if( applicationManager )
            {
                applicationManager->unload( nullptr );
                core::ApplicationManager::setInstance( nullptr );
                applicationManager = nullptr;
                WP_ASSERT( core::ApplicationManager::instance() == nullptr );
            }

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Game::createScene()
    {
        using namespace scene;

        auto application = core::IApplicationManager::instance();
        auto manager = application->getGameManager();
        auto window = UiUtil::createLayout( "UiUtil showcase", true );

        UiUtil::setRect( window, 0, 0, 1100, 650 );
        UiUtil::setAlignment( window, HorizontalAlignment::LEFT, VerticalAlignment::TOP );
        window->getComponent<LayoutTransform>()->setAnchor( Vector2<real_Num>( 0, 1 ) );
        window->getComponent<Layout>()->setReferenceSize( Vector2I( 1280, 720 ) );

        auto place = [window]( SmartPtr<IGameActor> actor, f32 x, f32 y, f32 width,
                               f32 height ) mutable {
            window->addChild( actor );
            UiUtil::setRect( actor, x, y, width, height );
            UiUtil::setAlignment( actor, HorizontalAlignment::LEFT, VerticalAlignment::TOP );
            actor->getComponent<LayoutTransform>()->setAnchor( Vector2<real_Num>( 0, 1 ) );
            return actor;
        };

        auto label = [&place]( const String &text, f32 x, f32 y, f32 width = 300.0f ) {
            auto actor = place( UiUtil::createText( text, false ), x, y, width, 32 );
            actor->getComponent<Text>()->setHorizontalAlignment(
                static_cast<u8>( HorizontalAlignment::LEFT ) );
            actor->getComponent<Text>()->setColour( ColourF::White );
            return actor;
        };

        label( "UiUtil - scene UI showcase", 32, 24, 650 );
        auto statusActor = label( "Click the button to update this label", 32, 68, 500 );
        auto status = WeakPtr<Text>( statusActor->getComponent<Text>() );
        
        auto testBtn = UiUtil::createButton( "Test Click me", false );
        window->addChild( testBtn );

        auto buttonActor = place( UiUtil::createButton( "Click me", false ), 32, 112, 260, 48 );
        
        buttonActor->getComponent<Button>()->setCallbackFunction(
            [status, count = 0]( hash_type ) mutable {
                if( auto text = status.lock() )
                {
                    text->setText( "Button clicks: " + StringUtil::toString( ++count ) );
                }
            } );

        auto sceneButton = place( UiUtil::createButton( "Scene button", false ), 320, 112, 260, 48 );
        sceneButton->getComponent<Button>()->setUseNativeUI( false );
        sceneButton->getComponent<Button>()->setCallbackFunction( [status]( hash_type ) {
            if( auto text = status.lock() )
            {
                text->setText( "Scene button clicked" );
            }
        } );

        label( "Text entry", 32, 184 );
        place( UiUtil::createInputField( "", "Enter your name", false ), 32, 222, 350, 48 );
        place( UiUtil::createToggle( "Enable notifications", true, false ), 32, 290, 350, 48 );
        label( "Native slider", 32, 370 );
        place( UiUtil::createSlider( "Volume", nullptr, "", false ), 32, 412, 200, 30 );
        label( "Native / scene vertical", 420, 184, 220 );
        place( UiUtil::createSliderVertical( "", nullptr, "", false ), 470, 230, 30, 200 );

        auto sceneSlider =
            place( UiUtil::createSlider( "Scene slider", nullptr, "", false ), 32, 510, 200, 30 );
        sceneSlider->getComponent<Slider>()->setUseNativeUI( false );
        label( "Scene slider", 32, 468 );
        auto sceneVertical =
            place( UiUtil::createSliderVertical( "", nullptr, "", false ), 540, 230, 30, 200 );
        sceneVertical->getComponent<Slider>()->setUseNativeUI( false );
        label( "Native quality dropdown", 640, 68 );
        place( UiUtil::createDropdownWithOptions( "Choose quality", { "Low", "Medium", "High" }, false ),
               640, 112, 300, 60 );
        auto sceneDropdown = place(
            UiUtil::createDropdownWithOptions( "Scene quality", { "Low", "Medium", "High" }, false ),
            640, 212, 300, 60 );
        sceneDropdown->getComponent<Dropdown>()->setUseNativeUI( false );
        label( "Scrollable list", 640, 280 );
        auto list = place( UiUtil::createScrollView( "Items", false ), 640, 312, 300, 200 );
        auto view = list->getComponent<ScrollView>();
        auto content = view->getContentPanel();
        for( s32 i = 0; i < 10; ++i )
        {
            auto item = UiUtil::createText( "List item " + StringUtil::toString( i + 1 ), false );
            content->getActor()->addChild( item );
            UiUtil::setRect( item, 8, static_cast<f32>( i * 34 ), 240, 30 );
            UiUtil::setAlignment( item, HorizontalAlignment::LEFT, VerticalAlignment::TOP );
            item->getComponent<LayoutTransform>()->setAnchor( Vector2<real_Num>( 0, 1 ) );
            item->getComponent<Text>()->setColour( ColourF::White );
            item->getComponent<Text>()->setHorizontalAlignment(
                static_cast<u8>( HorizontalAlignment::LEFT ) );
        }
        content->setSize( Vector2<real_Num>( 270, 340 ) );
        view->updateScrollBar();

        UiUtil::loadHierarchy( window );
        // Layout::load installs its defaults; pin this showcase after loading.
        window->getComponent<Layout>()->setPanelFlags( PanelFlags::NO_SCROLLBAR );
        window->setState( IGameActor::State::Play, true );
        manager->play();
        if( auto graphicsWindow = application->getGraphicsSystem()->getDefaultWindow() )
        {
            graphicsWindow->setTitle( "Workphone - UiUtil showcase" );
        }
    }

    void Game::createPlugins()
    {
        const auto configFilePath = String( "wp_plugins_samples.cfg" );
        setPluginsConfigFilePath( configFilePath );

        Application::createPlugins();

#ifdef _WP_STATIC_LIB_
        auto applicationManager = core::ApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto corePlugin = workphone::make_ptr<WPCore>();
        applicationManager->addPlugin( corePlugin );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
        auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgreNext>();
        applicationManager->addPlugin( graphicsPlugin );
#    elif WP_GRAPHICS_SYSTEM_OGRE
        auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgre>();
        applicationManager->addPlugin( graphicsPlugin );
#    endif
#endif
    }

}  // namespace workphone

int main( int argc, char *argv[] )
{
    using namespace workphone;

    try
    {
        auto typeManager = std::make_shared<TypeManager>();
        typeManager->load();
        TypeManager::setInstance( typeManager.get() );

        auto app = std::make_shared<Game>();
        app->load( nullptr );
        app->run();
        app->unload( nullptr );

        TypeManager::setInstance( nullptr );
    }
    catch( Exception &e )
    {
        std::cout << e.what() << std::endl;
    }
    catch( std::exception &e )
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
