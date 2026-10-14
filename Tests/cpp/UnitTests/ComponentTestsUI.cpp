#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include "UiUtilTestSupport.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <limits>
#include <stdexcept>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    bool isUiRuntimeAvailable()
    {
        auto applicationManager = core::IApplicationManager::instance();
        return applicationManager && applicationManager->getRenderUI();
    }

    bool skipWhenUiRuntimeUnavailable()
    {
        if( isUiRuntimeAvailable() )
        {
            return false;
        }

        BOOST_TEST_MESSAGE( "Skipping UI component test because no UI runtime plugin is available." );
        return true;
    }
}  // namespace

BOOST_AUTO_TEST_CASE( ui_load_components )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto typeManager = TypeManager::instance();

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto factories = factoryManager->getFactories();
        for( auto factory : factories )
        {
            TestGuard guard;

            auto actor = sceneManager->createActor();

            if( factory->isObjectDerivedFrom<scene::UIComponent>() )
            {
                auto component = factory->make_ptr<scene::UIComponent>();
                if( component )
                {
                    auto componentName = typeManager->getName( component->getTypeInfo() );
                    if( component->isExactly<scene::UIComponent>() )
                    {
                        BOOST_TEST_MESSAGE( "Skipping base UIComponent factory." );
                        continue;
                    }

                    actor->addComponentInstance( component );
                    component->load( nullptr );
                    BOOST_CHECK( component->isLoaded() );

                    if( !component->isLoaded() )
                    {
                        auto message = String( "Component failed to load: " ) + componentName;
                        WP_LOG( message );
                    }

                    // Explicitly unload before destroying
                    component->unload( nullptr );
                }
            }

            sceneManager->destroyActor( actor );
            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_components )
{
    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto timer = applicationManager->getTimer();
        auto stateManager = applicationManager->getStateManager();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_CHECK( timer );
        BOOST_CHECK( stateManager );
        BOOST_CHECK( sceneManager );
        BOOST_CHECK( scene );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_dropdown_initial_state )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        auto dropdown = actor->addComponent<Dropdown>();

        BOOST_CHECK_EQUAL( dropdown->isOpen(), false );
        BOOST_CHECK( dropdown->getOptions().empty() );
        BOOST_CHECK( !dropdown->getButton() );
        BOOST_CHECK( !dropdown->getPanel() );

        // Explicitly unload dropdown to release text objects
        dropdown->unload( nullptr );
        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_dropdown_add_option )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        // Use consistent API (non-Ptr variants)
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        auto dropdown = actor->addComponent<Dropdown>();

        auto numOptions = 10;
        for( int i = 1; i <= numOptions; ++i )
        {
            auto option = workphone::make_ptr<Dropdown::Option>();
            option->text = "Option " + StringUtil::toString( i );
            dropdown->addOption( option );
        }

        auto options = dropdown->getOptions();
        BOOST_CHECK_EQUAL( options.size(), numOptions );
        BOOST_CHECK_EQUAL( options[0]->text.str(), "Option 1" );

        // Clear options before destroying to release text references
        dropdown->removeOptions();
        dropdown->unload( nullptr );
        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_dropdown_remove_option )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        auto dropdown = actor->addComponent<Dropdown>();

        auto option1 = workphone::make_ptr<Dropdown::Option>( "Option 1" );
        auto option2 = workphone::make_ptr<Dropdown::Option>( "Option 2" );

        dropdown->addOption( option1 );
        dropdown->addOption( option2 );
        dropdown->removeOption( option1 );

        auto options = dropdown->getOptions();
        BOOST_CHECK_EQUAL( options.size(), 1 );
        BOOST_CHECK_EQUAL( options[0]->text.str(), "Option 2" );

        // Clear remaining options and unload
        dropdown->removeOptions();
        dropdown->unload( nullptr );
        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_dropdown_set_open )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        auto dropdown = actor->addComponent<Dropdown>();

        dropdown->setOpen( true );
        BOOST_CHECK_EQUAL( dropdown->isOpen(), true );
        dropdown->setOpen( false );
        BOOST_CHECK_EQUAL( dropdown->isOpen(), false );

        dropdown->unload( nullptr );
        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_dropdown_button_callback_toggle )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        auto dropdown = actor->addComponent<Dropdown>();

        auto button = workphone::make_ptr<Button>();
        button->setActor( actor );

        dropdown->setButton( button );
        dropdown->setActor( actor );
        dropdown->setEnabled( true );
        actor->setEnabled( true );

        bool initialOpen = dropdown->isOpen();

        // mock click event
        button->handleEvent( EventType::UI, IEvent::CLICK_HASH, {}, applicationManager, button,
                             nullptr );

        BOOST_CHECK_EQUAL( dropdown->isOpen(), !initialOpen );

        // Clear button reference and unload
        dropdown->setButton( nullptr );
        button->unload( nullptr );
        button = nullptr;
        dropdown->unload( nullptr );
        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_dropdown_set_properties_round_trip )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        UiUtilTestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        auto dropdown = actor->addComponent<Dropdown>();

        auto button = workphone::make_ptr<Button>();
        auto panel = UiUtil::createPanel( nullptr, "", false );
        dropdown->setButton( button );
        dropdown->setPanel( panel );
        dropdown->setOpen( true );

        auto option = workphone::make_ptr<Dropdown::Option>( "Test Option" );
        dropdown->addOption( option );

        auto properties = dropdown->getProperties();

        auto dropdown2 = actor->addComponent<Dropdown>();
        dropdown2->setProperties( properties );

        auto options = dropdown2->getOptions();

        BOOST_CHECK_EQUAL( dropdown2->isOpen(), true );
        BOOST_CHECK_EQUAL( options.size(), 1 );
        BOOST_CHECK_EQUAL( options[0]->text.str(), "Test Option" );

        // Cleanup in reverse order
        dropdown2->removeOptions();
        dropdown2->unload( nullptr );

        dropdown->setButton( nullptr );
        dropdown->setPanel( nullptr );
        dropdown->removeOptions();
        dropdown->unload( nullptr );

        button->unload( nullptr );
        button = nullptr;

        // Destroy panel actor if it was added to scene
        if( panel )
        {
            sceneManager->destroyActor( panel );
        }

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_text_entry )
{
    //    try
    //    {
    //        auto applicationManager = core::IApplicationManager::instance();
    //        BOOST_CHECK( applicationManager );
    //
    //        auto sceneManager = applicationManager->getGameManager();
    //        auto scene = sceneManager->getCurrentScene();
    //
    //        auto actor = sceneManager->createActor();
    //        BOOST_CHECK( actor );
    //
    //        auto textEntry = actor->addComponent<scene::TextEntry>();
    //        BOOST_CHECK( textEntry );
    //        BOOST_CHECK( textEntry->isValid() );
    //
    //        // Test text setting and getting
    //        const String testText = "Test Text";
    //        textEntry->setText( testText );
    //        BOOST_CHECK_EQUAL( textEntry->getText(), testText );
    //
    //        // Test text clearing
    //        textEntry->setText( "" );
    //        BOOST_CHECK( textEntry->getText().empty() );
    //
    //        sceneManager->destroyActor( actor );
    //    }
    //    catch( std::exception &e )
    //    {
    //        WP_LOG_EXCEPTION( e );
    //    }
}

BOOST_AUTO_TEST_CASE( ui_checkbox )
{
    //    try
    //    {
    //        auto applicationManager = core::IApplicationManager::instance();
    //        BOOST_CHECK( applicationManager );
    //
    //        auto sceneManager = applicationManager->getGameManager();
    //        auto scene = sceneManager->getCurrentScene();
    //
    //        auto actor = sceneManager->createActor();
    //        BOOST_CHECK( actor );
    //
    //        auto checkbox = actor->addComponent<scene::Checkbox>();
    //        BOOST_CHECK( checkbox );
    //        BOOST_CHECK( checkbox->isValid() );
    //
    //        // Test initial state
    //        BOOST_CHECK_EQUAL( checkbox->isChecked(), false );
    //
    //        // Test checking
    //        checkbox->setChecked( true );
    //        BOOST_CHECK_EQUAL( checkbox->isChecked(), true );
    //
    //        // Test unchecking
    //        checkbox->setChecked( false );
    //        BOOST_CHECK_EQUAL( checkbox->isChecked(), false );
    //
    //        sceneManager->destroyActor( actor );
    //    }
    //    catch( std::exception &e )
    //    {
    //        WP_LOG_EXCEPTION( e );
    //    }
}

BOOST_AUTO_TEST_CASE( ui_button )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        BOOST_CHECK( actor );

        auto button = actor->addComponent<scene::Button>();
        BOOST_CHECK( button );
        BOOST_CHECK( button->isValid() );

        // Test label setting and getting
        const String testLabel = "Test Button";
        button->setLabel( testLabel );
        BOOST_CHECK_EQUAL( button->getLabel(), testLabel );

        // Test enabled state
        BOOST_CHECK_EQUAL( button->isEnabled(), true );
        button->setEnabled( false );
        BOOST_CHECK_EQUAL( button->isEnabled(), false );

        // Clear label text before unload
        button->setLabel( "" );
        button->unload( nullptr );
        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_panel )
{
    //    try
    //    {
    //        auto applicationManager = core::IApplicationManager::instance();
    //        BOOST_CHECK( applicationManager );
    //
    //        auto sceneManager = applicationManager->getGameManager();
    //        auto scene = sceneManager->getCurrentScene();
    //
    //        auto actor = sceneManager->createActor();
    //        BOOST_CHECK( actor );
    //
    //        auto panel = actor->addComponent<scene::Panel>();
    //        BOOST_CHECK( panel );
    //        BOOST_CHECK( panel->isValid() );
    //
    //        // Test size setting and getting
    //        Vector2F testSize( 100.0f, 100.0f );
    //        panel->setSize( testSize );
    //        BOOST_CHECK_EQUAL( panel->getSize(), testSize );
    //
    //        // Test position setting and getting
    //        Vector2F testPosition( 50.0f, 50.0f );
    //        panel->setPosition( testPosition );
    //        BOOST_CHECK_EQUAL( panel->getPosition(), testPosition );
    //
    //        sceneManager->destroyActor( actor );
    //    }
    //    catch( std::exception &e )
    //    {
    //        WP_LOG_EXCEPTION( e );
    //    }
}

BOOST_AUTO_TEST_CASE( ui_slider )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        BOOST_CHECK( actor );

        auto slider = actor->addComponent<scene::Slider>();
        BOOST_CHECK( slider );
        BOOST_CHECK( slider->isValid() );

        // Test value range setting
        slider->setMinValue( 0.0f );
        slider->setMaxValue( 100.0f );
        BOOST_CHECK_EQUAL( slider->getMinValue(), 0.0f );
        BOOST_CHECK_EQUAL( slider->getMaxValue(), 100.0f );

        // Test value setting and getting
        slider->setValue( 50.0f );
        BOOST_CHECK_EQUAL( slider->getValue(), 50.0f );

        slider->unload( nullptr );
        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_progress_bar )
{
    //    try
    //    {
    //        auto applicationManager = core::IApplicationManager::instance();
    //        BOOST_CHECK( applicationManager );
    //
    //        auto sceneManager = applicationManager->getGameManager();
    //        auto scene = sceneManager->getCurrentScene();
    //
    //        auto actor = sceneManager->createActor();
    //        BOOST_CHECK( actor );
    //
    //        auto progressBar = actor->addComponent<scene::ProgressBar>();
    //        BOOST_CHECK( progressBar );
    //        BOOST_CHECK( progressBar->isValid() );
    //
    //        // Test progress setting and getting
    //        progressBar->setProgress( 0.5f );
    //        BOOST_CHECK_EQUAL( progressBar->getProgress(), 0.5f );
    //
    //        // Test min/max values
    //        progressBar->setMinValue( 0.0f );
    //        progressBar->setMaxValue( 100.0f );
    //        BOOST_CHECK_EQUAL( progressBar->getMinValue(), 0.0f );
    //        BOOST_CHECK_EQUAL( progressBar->getMaxValue(), 100.0f );
    //
    //        sceneManager->destroyActor( actor );
    //    }
    //    catch( std::exception &e )
    //    {
    //        WP_LOG_EXCEPTION( e );
    //    }
}

BOOST_AUTO_TEST_CASE( ui_label )
{
    //    try
    //    {
    //        auto applicationManager = core::IApplicationManager::instance();
    //        BOOST_CHECK( applicationManager );
    //
    //        auto sceneManager = applicationManager->getGameManager();
    //        auto scene = sceneManager->getCurrentScene();
    //
    //        auto actor = sceneManager->createActor();
    //        BOOST_CHECK( actor );
    //
    //        auto label = actor->addComponent<scene::Label>();
    //        BOOST_CHECK( label );
    //        BOOST_CHECK( label->isValid() );
    //
    //        // Test text setting and getting
    //        const String testText = "Test Label";
    //        label->setText( testText );
    //        BOOST_CHECK_EQUAL( label->getText(), testText );
    //
    //        // Test font size
    //        label->setFontSize( 24 );
    //        BOOST_CHECK_EQUAL( label->getFontSize(), 24 );
    //
    //        sceneManager->destroyActor( actor );
    //    }
    //    catch( std::exception &e )
    //    {
    //        WP_LOG_EXCEPTION( e );
    //    }
}

BOOST_AUTO_TEST_CASE( ui_image )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        BOOST_CHECK( actor );

        auto image = actor->addComponent<scene::Image>();
        BOOST_CHECK( image );
        BOOST_CHECK( image->isValid() );

        // Test image path setting and getting
        //const String testPath = "test_image.png";
        //image->setImagePath( testPath );
        //BOOST_CHECK_EQUAL( image->getImagePath(), testPath );

        //// Test size setting and getting
        //Vector2F testSize( 200.0f, 200.0f );
        //image->setSize( testSize );
        //BOOST_CHECK_EQUAL( image->getSize(), testSize );

        image->unload( nullptr );
        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_scroll_view )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        BOOST_CHECK( actor );

        auto scrollView = actor->addComponent<scene::ScrollView>();
        BOOST_CHECK( scrollView );
        BOOST_CHECK( scrollView->isValid() );

        // Test content size setting and getting
        Vector2F testContentSize( 500.0f, 1000.0f );
        //scrollView->setContentSize( testContentSize );
        //BOOST_CHECK_EQUAL( scrollView->getContentSize(), testContentSize );

        // Test scroll position
        Vector2F testScrollPosition( 0.0f, 100.0f );
        //scrollView->setScrollPosition( testScrollPosition );
        //BOOST_CHECK_EQUAL( scrollView->getScrollPosition(), testScrollPosition );

        scrollView->unload( nullptr );
        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( ui_grid_layout )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        BOOST_CHECK( actor );

        auto gridLayout = actor->addComponent<scene::GridLayout>();
        BOOST_CHECK( gridLayout );
        BOOST_CHECK( gridLayout->isValid() );

        // Test grid dimensions
        //gridLayout->setColumns( 3 );
        //gridLayout->setRows( 2 );
        //BOOST_CHECK_EQUAL( gridLayout->getColumns(), 3 );
        //BOOST_CHECK_EQUAL( gridLayout->getRows(), 2 );

        // Test cell spacing
        //Vector2F testSpacing( 10.0f, 10.0f );
        //gridLayout->setCellSpacing( testSpacing );
        //BOOST_CHECK_EQUAL( gridLayout->getCellSpacing(), testSpacing );

        gridLayout->unload( nullptr );
        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

// Factory contracts run without a renderer: scene ownership and component configuration must
// remain usable by importers, scripts, and headless tooling.
BOOST_AUTO_TEST_SUITE( UiUtilTests )

BOOST_AUTO_TEST_CASE( alignment_round_trip )
{
    for( u8 i = 0; i < static_cast<u8>( HorizontalAlignment::COUNT ); ++i )
    {
        auto alignment = static_cast<HorizontalAlignment>( i );
        BOOST_CHECK_EQUAL( UiUtil::getHorizontalAlignmentString( alignment ),
                           UiUtil::horizontalAlignmentTypes[i] );
        BOOST_CHECK( UiUtil::getHorizontalAlignment(
                         UiUtil::getHorizontalAlignmentString( alignment ) ) == alignment );
    }
    for( u8 i = 0; i < static_cast<u8>( VerticalAlignment::COUNT ); ++i )
    {
        auto alignment = static_cast<VerticalAlignment>( i );
        BOOST_CHECK_EQUAL( UiUtil::getVerticalAlignmentString( alignment ),
                           UiUtil::verticalAlignmentTypes[i] );
        BOOST_CHECK( UiUtil::getVerticalAlignment( UiUtil::getVerticalAlignmentString( alignment ) ) ==
                     alignment );
    }
    BOOST_CHECK_EQUAL( UiUtil::getHorizontalAlignmentTypesString(), "Left;Right;Center;Custom;" );
    BOOST_CHECK_EQUAL( UiUtil::getVerticalAlignmentTypesString(), "Top;Bottom;Center;Custom;" );
    BOOST_CHECK( UiUtil::getHorizontalAlignmentString( HorizontalAlignment::COUNT ).empty() );
    BOOST_CHECK( UiUtil::getVerticalAlignmentString( static_cast<VerticalAlignment>( 255 ) ).empty() );
    BOOST_CHECK( UiUtil::getHorizontalAlignment( "unknown" ) == HorizontalAlignment::LEFT );
    BOOST_CHECK( UiUtil::getVerticalAlignment( "" ) == VerticalAlignment::TOP );
}

BOOST_AUTO_TEST_CASE( all_factories_honor_scene_ownership )
{
    UiUtilTestGuard guard;
    const std::vector<std::function<SmartPtr<IGameActor>( bool )>> factories = {
        []( bool publish ) { return UiUtil::createText( "Text", publish ); },
        []( bool publish ) { return UiUtil::createButton( "Button", publish ); },
        []( bool publish ) { return UiUtil::createButtonEx( "Button", 1, 2, 30, 40, publish ); },
        []( bool publish ) { return UiUtil::createDropdown( "Dropdown", publish ); },
        []( bool publish ) {
            return UiUtil::createDropdownWithOptions( "Quality", { "Low", "High" }, publish );
        },
        []( bool publish ) { return UiUtil::createImage( "", publish ); },
        []( bool publish ) { return UiUtil::createPanel( nullptr, "Panel", publish ); },
        []( bool publish ) { return UiUtil::createSlider( "Slider", nullptr, "", publish ); },
        []( bool publish ) { return UiUtil::createSliderVertical( "Slider", nullptr, "", publish ); },
        []( bool publish ) { return UiUtil::createScrollbar( "Scrollbar", nullptr, "", publish ); },
        []( bool publish ) {
            return UiUtil::createScrollbarVertical( "Scrollbar", nullptr, "", publish );
        },
        []( bool publish ) { return UiUtil::createScrollview( "Scrollview", nullptr, "", publish ); },
        []( bool publish ) { return UiUtil::createScrollView( "ScrollView", publish ); },
        []( bool publish ) { return UiUtil::createInputField( "Value", "Hint", publish ); },
        []( bool publish ) { return UiUtil::createToggle( "Toggle", false, publish ); },
        []( bool publish ) { return UiUtil::createLayout( "Layout", publish ); },
        []( bool publish ) { return UiUtil::createHorizontalLayout( "Row", publish ); },
        []( bool publish ) { return UiUtil::createVerticalLayout( "Column", publish ); },
        []( bool publish ) { return UiUtil::createGridLayout( "Grid", 3, publish ); },
        []( bool publish ) { return UiUtil::createTableLayout( "Table", publish ); },
        []( bool publish ) { return UiUtil::createTableCell( "Cell", publish ); },
        []( bool publish ) { return UiUtil::createTabView( "Tabs", publish ); },
        []( bool publish ) { return UiUtil::createTabPage( "Page", publish ); },
        []( bool publish ) { return UiUtil::createTabItem( "Tab", publish ); },
        []( bool publish ) { return UiUtil::createToggleGroup( "Group", publish ); },
        []( bool publish ) { return UiUtil::createToolTip( "Help", publish ); },
        []( bool publish ) { return UiUtil::createThumbnail( "Asset", "", publish ); }
    };
    for( auto publish : { false, true } )
    {
        for( size_t i = 0; i < factories.size(); ++i )
        {
            BOOST_TEST_CONTEXT( "factory " << i << ", publish " << publish )
            {
                const auto sceneCount = guard.scene->getActors().size();
                const auto managerCount = guard.sceneManager->getNumActors();
                auto actor = factories[i]( publish );
                BOOST_REQUIRE( actor );
                BOOST_CHECK( actor->getComponent<LayoutTransform>() );
                BOOST_CHECK_EQUAL( guard.scene->getActors().size(), sceneCount + ( publish ? 1 : 0 ) );
                BOOST_CHECK( ( actor->getScene() == guard.scene ) == publish );
                for( auto child : actor->getChildren() )
                {
                    BOOST_CHECK( child->getParent() == actor );
                    BOOST_CHECK( ( child->getScene() == guard.scene ) == publish );
                }
                guard.sceneManager->destroyActor( actor );
                BOOST_CHECK_EQUAL( guard.scene->getActors().size(), sceneCount );
                BOOST_CHECK_EQUAL( guard.sceneManager->getNumActors(), managerCount );
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( content_and_placement )
{
    UiUtilTestGuard guard;
    auto buttonActor = UiUtil::createButtonEx( "Save", 12, -7, 80, 25, false );
    auto button = buttonActor->getComponent<Button>();
    BOOST_REQUIRE( button );
    BOOST_CHECK_EQUAL( button->getLabel(), "Save" );
    BOOST_CHECK_EQUAL( button->getTextStr(), "Save" );
    button->setTextStr( "Saved" );
    BOOST_CHECK_EQUAL( button->getLabel(), "Saved" );
    auto transform = buttonActor->getComponent<LayoutTransform>();
    BOOST_CHECK_EQUAL( transform->getPosition().x, 12 );
    BOOST_CHECK_EQUAL( transform->getPosition().y, -7 );
    BOOST_CHECK_EQUAL( transform->getSize().x, 80 );
    BOOST_CHECK_EQUAL( transform->getSize().y, 25 );
    UiUtil::setAlignment( buttonActor, HorizontalAlignment::RIGHT, VerticalAlignment::BOTTOM );
    auto anchor = transform->getAnchor();
    auto anchorMin = transform->getAnchorMin();
    auto anchorMax = transform->getAnchorMax();
    BOOST_CHECK_EQUAL( anchorMin.x, 1 );
    BOOST_CHECK_EQUAL( anchorMin.y, 0 );
    UiUtil::setAlignment( buttonActor, HorizontalAlignment::CUSTOM, VerticalAlignment::CUSTOM );
    BOOST_CHECK( transform->getAnchor() == anchor );
    BOOST_CHECK( transform->getAnchorMin() == anchorMin );
    BOOST_CHECK( transform->getAnchorMax() == anchorMax );

    auto text = UiUtil::createText( "Title", false )->getComponent<Text>();
    BOOST_CHECK_EQUAL( text->getText(), "Title" );
    auto empty = UiUtil::createDropdown( "Choose", false )->getComponent<Dropdown>();
    BOOST_CHECK( empty->getOptions().empty() );
    BOOST_CHECK_EQUAL( empty->getLabel(), "Choose" );
    auto dropdown = UiUtil::createDropdownWithOptions( "Quality", { "Low", "High" }, false )
                        ->getComponent<Dropdown>();
    BOOST_REQUIRE_EQUAL( dropdown->getOptions().size(), 2 );
    BOOST_CHECK_EQUAL( dropdown->getOptions()[1]->text.str(), "High" );
    auto input = UiUtil::createInputField( "Alice", "Name", false )->getComponent<InputField>();
    BOOST_CHECK_EQUAL( input->getText(), "Alice" );
    BOOST_CHECK_EQUAL( input->getPlaceholder(), "Name" );
    auto toggle = UiUtil::createToggle( "Enabled", false, false )->getComponent<Toggle>();
    BOOST_CHECK_EQUAL( toggle->getLabel(), "Enabled" );
    BOOST_CHECK( !toggle->isToggled() );
    auto thumbnail = UiUtil::createThumbnail( "Asset", "", false )->getComponent<Thumbnail>();
    BOOST_CHECK( thumbnail->getThumb() );
    BOOST_REQUIRE( thumbnail->getLabelText() );
    BOOST_CHECK_EQUAL( thumbnail->getLabelText()->getText(), "Asset" );
    auto grid = UiUtil::createGridLayout( "Grid", 5, false )->getComponent<GridLayout>();
    BOOST_CHECK_EQUAL( grid->getColumnCount(), 5 );
}

BOOST_AUTO_TEST_CASE( track_endpoints_and_scroll_view_links )
{
    UiUtilTestGuard guard;
    for( auto vertical : { false, true } )
    {
        auto actor = vertical
                         ? UiUtil::createSliderVertical( "Volume", nullptr, "Volume control", false )
                         : UiUtil::createSlider( "Volume", nullptr, "Volume control", false );
        BOOST_CHECK_EQUAL( actor->getName(), "Volume control" );
        auto slider = actor->getComponent<Slider>();
        BOOST_REQUIRE( slider->getBackground() );
        BOOST_REQUIRE( slider->getFill() );
        BOOST_REQUIRE( slider->getHandleActor() );
        BOOST_CHECK( slider->getDirection() ==
                     ( vertical ? Direction::Vertical : Direction::Horizontal ) );
        auto handle = slider->getHandleActor()->getComponent<LayoutTransform>();
        auto fill = slider->getFill()->getComponent<LayoutTransform>();
        BOOST_CHECK_EQUAL( vertical ? handle->getPosition().y : handle->getPosition().x, 100 );
        for( f32 value : { 0.0f, 1.0f } )
        {
            slider->setValue( value );
            BOOST_CHECK_EQUAL( vertical ? handle->getPosition().y : handle->getPosition().x,
                               200 * value );
            BOOST_CHECK_EQUAL( vertical ? fill->getSize().y : fill->getSize().x, 200 * value );
        }
        auto barActor = vertical ? UiUtil::createScrollbarVertical( "Scroll", nullptr, "", false )
                                 : UiUtil::createScrollbar( "Scroll", nullptr, "", false );
        auto bar = barActor->getComponent<ScrollBar>();
        BOOST_REQUIRE( bar->getFill() );
        bar->setScrollValue( 1.0f );
        auto barHandle = bar->getHandleActor()->getComponent<LayoutTransform>();
        BOOST_CHECK_EQUAL( vertical ? barHandle->getPosition().y : barHandle->getPosition().x, 150 );
    }
    auto view = UiUtil::createScrollView( "Items", false )->getComponent<ScrollView>();
    auto content = view->getContentPanel();
    auto bar = view->getScrollBar();
    BOOST_REQUIRE( content );
    BOOST_REQUIRE( bar );
    BOOST_CHECK( !bar->getActor()->isEnabled() );
    BOOST_CHECK( bar->getScrollView() == view.get() );
    content->setSize( Vector2<real_Num>( 270, 400 ) );
    view->updateScrollBar();
    BOOST_CHECK( bar->isEnabled() );
    BOOST_CHECK( bar->getActor()->isEnabled() );
    BOOST_CHECK_EQUAL( bar->getHandleSize(), 100 );
    bar->setScrollValue( 0.5f );
    view->syncWithScrollBar();
    BOOST_CHECK_EQUAL( content->getPosition().y, -100 );
    auto replacement =
        UiUtil::createScrollbarVertical( "Replacement", nullptr, "", false )->getComponent<ScrollBar>();
    view->setScrollBar( replacement );
    BOOST_CHECK( !bar->getScrollView() );
    BOOST_CHECK( replacement->getScrollView() == view.get() );
    view->unload( nullptr );
    BOOST_CHECK( !replacement->getScrollView() );
    BOOST_CHECK( !view->getContentPanel() );
}

BOOST_AUTO_TEST_CASE( invalid_arguments_do_not_allocate_or_mutate )
{
    UiUtilTestGuard guard;
    const auto count = guard.sceneManager->getNumActors();
    BOOST_CHECK_THROW( UiUtil::createButtonEx( "Bad", 0, 0, -1, 10, true ), std::invalid_argument );
    BOOST_CHECK_THROW(
        UiUtil::createButtonEx( "Bad", std::numeric_limits<f32>::infinity(), 0, 10, 10, false ),
        std::invalid_argument );
    BOOST_CHECK_THROW( UiUtil::createGridLayout( "Bad", 0, false ), std::invalid_argument );
    BOOST_CHECK_EQUAL( guard.sceneManager->getNumActors(), count );
    BOOST_CHECK_THROW( UiUtil::setRect( nullptr, 0, 0, 10, 10 ), std::invalid_argument );
    auto actor = UiUtil::createButton( "Valid", false );
    auto transform = actor->getComponent<LayoutTransform>();
    const auto size = transform->getSize();
    BOOST_CHECK_THROW( UiUtil::setRect( actor, 5, 10, std::numeric_limits<f32>::quiet_NaN(), 20 ),
                       std::invalid_argument );
    BOOST_CHECK( transform->getSize() == size );
    BOOST_CHECK_THROW( UiUtil::setAlignment( actor, HorizontalAlignment::COUNT, VerticalAlignment::TOP ),
                       std::invalid_argument );
    BOOST_CHECK( transform->getHorizontalAlignment() == HorizontalAlignment::CENTER );
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE( uiutil_failed_composite_construction_rolls_back )
{
    UiUtilTestGuard guard;
    auto factory = guard.factoryManager->getFactoryByHash(
        TypeManager::instance()->getHash( ScrollBar::typeInfo() ) );
    BOOST_REQUIRE( factory );
    const auto actorCount = guard.sceneManager->getNumActors();
    const auto sceneCount = guard.scene->getActors().size();

    // Fail after allocating the root, content, and scrollbar actor. Restore the registry even
    // when a test assertion throws, before TestGuard starts tearing down the scene.
    struct RestoreFactory
    {
        SmartPtr<IFactoryManager> manager;
        SmartPtr<IFactory> factory;

        ~RestoreFactory()
        {
            manager->addFactory( factory );
        }
    } restore{ guard.factoryManager, factory };

    BOOST_REQUIRE( guard.factoryManager->removeFactory( factory ) );
    BOOST_CHECK_THROW( UiUtil::createScrollView( "Failure", true ), std::runtime_error );
    BOOST_CHECK_EQUAL( guard.sceneManager->getNumActors(), actorCount );
    BOOST_CHECK_EQUAL( guard.scene->getActors().size(), sceneCount );
}

BOOST_AUTO_TEST_CASE( uiutil_missing_layout_system_is_reported )
{
    TestGuard guard;
    const auto count = guard.sceneManager->getNumActors();
    BOOST_CHECK_THROW( UiUtil::createText( "No layout system", false ), std::runtime_error );
    BOOST_CHECK_EQUAL( guard.sceneManager->getNumActors(), count );
}

BOOST_AUTO_TEST_CASE( uiutil_renderer_preserves_labels )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }
    UiUtilTestGuard guard;
    auto window = UiUtil::createLayout( "Settings", true );
    auto layout = window->getComponent<Layout>();
    UiUtil::loadHierarchy( window );
    BOOST_REQUIRE( layout->getElement() );
    auto actor = UiUtil::createButton( "Save changes", false );
    window->addChild( actor );
    auto button = actor->getComponent<Button>();
    UiUtil::loadHierarchy( window );
    auto properties = button->getProperties();
    properties->setButtonPressed( UIComponent::createUIStr, true );
    button->setProperties( properties );
    BOOST_REQUIRE( button->getElement() );
    BOOST_CHECK_EQUAL( button->getLabel(), "Save changes" );
    BOOST_CHECK_EQUAL( button->getElement()->getLabel(), "Save changes" );
    auto dropdownActor = UiUtil::createDropdownWithOptions( "Quality", { "Low", "High" }, false );
    window->addChild( dropdownActor );
    auto dropdown = dropdownActor->getComponent<Dropdown>();
    dropdown->setUseNativeUI( false );
    UiUtil::loadHierarchy( window );
    auto trigger = dropdown->getButton();
    BOOST_REQUIRE( trigger );
    auto triggerProperties = trigger->getProperties();
    triggerProperties->setButtonPressed( UIComponent::createUIStr, true );
    trigger->setProperties( triggerProperties );
    BOOST_REQUIRE( trigger->getElement() );
    BOOST_CHECK_EQUAL( dropdown->getLabel(), "Quality" );
    BOOST_CHECK_EQUAL( trigger->getElement()->getLabel(), "Quality" );
    dropdown->setOpen( true );
    BOOST_REQUIRE( dropdown->getContent() );
    BOOST_REQUIRE( dropdown->getPanel() );
    BOOST_REQUIRE_EQUAL( dropdown->getPanel()->getChildren().size(), 2 );
    auto optionActor = dropdown->getPanel()->getChildren()[1];
    auto optionButton = optionActor->getComponent<Button>();
    BOOST_REQUIRE( optionButton );
    BOOST_CHECK_EQUAL( optionButton->getLabel(), "High" );
    optionButton->handleEvent( EventType::UI, IEvent::CLICK_HASH, {}, nullptr, nullptr, nullptr );
    BOOST_CHECK_EQUAL( dropdown->getLabel(), "High" );
    BOOST_CHECK_EQUAL( trigger->getLabel(), "High" );
    BOOST_CHECK( !dropdown->isOpen() );
    BOOST_CHECK( !dropdown->getContent()->isEnabled() );
    auto sliderActor = UiUtil::createSlider( "Volume", nullptr, "", false );
    window->addChild( sliderActor );
    UiUtil::loadHierarchy( window );
    auto slider = sliderActor->getComponent<Slider>();
    auto sliderProperties = slider->getProperties();
    sliderProperties->setButtonPressed( UIComponent::createUIStr, true );
    slider->setProperties( sliderProperties );
    BOOST_CHECK_EQUAL( slider->getLabel(), "Volume" );
    if( auto element = slider->getElement() )
    {
        BOOST_CHECK_EQUAL( element->getLabel(), "Volume" );
    }
    else
    {
        BOOST_CHECK( slider->getBackground()->getComponent<scene::Image>()->getElement() );
    }
    auto buttonFsm = button->getFsm();
    UiUtil::loadHierarchy( window );
    BOOST_CHECK( button->getFsm() == buttonFsm );
    button->setUseNativeUI( false );
    BOOST_REQUIRE( button->getImage() );
    BOOST_REQUIRE( button->getText() );
    BOOST_CHECK( !button->getElement()->isVisible() );
    BOOST_CHECK( button->getImage()->getActor()->isEnabled() );
    button->setUseNativeUI( true );
    BOOST_CHECK( button->getElement()->isVisible() );
    BOOST_CHECK( !button->getImage()->getActor()->isEnabled() );
}

BOOST_AUTO_TEST_CASE( uiutil_native_controls_and_mode_properties )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }
    UiUtilTestGuard guard;
    auto window = UiUtil::createLayout( "Native controls", true );
    auto dropdownActor = UiUtil::createDropdownWithOptions( "Quality", { "Low", "High" }, false );
    window->addChild( dropdownActor );
    auto sliderActor = UiUtil::createSlider( "Volume", nullptr, "", false );
    window->addChild( sliderActor );
    UiUtil::loadHierarchy( window );
    auto dropdown = dropdownActor->getComponent<Dropdown>();
    auto slider = sliderActor->getComponent<Slider>();
    auto nativeDropdown = workphone::dynamic_pointer_cast<ui::IUIDropdown>( dropdown->getElement() );
    auto nativeSlider = workphone::dynamic_pointer_cast<ui::IUISlider>( slider->getElement() );
    BOOST_REQUIRE( nativeDropdown );
    BOOST_REQUIRE( nativeSlider );
    BOOST_REQUIRE_EQUAL( nativeDropdown->getOptions().size(), 2 );
    BOOST_CHECK( dropdown->getUseNativeUI() );
    BOOST_CHECK( !dropdown->getButton()->getActor()->isEnabled() );
    BOOST_CHECK( !slider->getBackground()->isEnabled() );
    auto previousTask = Thread::getCurrentTask();

    struct RestoreTask
    {
        TaskId task;

        ~RestoreTask()
        {
            Thread::setCurrentTask( task );
        }
    } restore{ previousTask };

    Thread::setCurrentTask( TaskId::Application );
    nativeDropdown->setSelectedOption( 1 );
    dropdown->getElementListener()->handleEvent( EventType::UI, IEvent::SELECT_HASH, {}, nativeDropdown,
                                                 nullptr, nullptr );
    BOOST_CHECK_EQUAL( dropdown->getLabel(), "High" );
    nativeSlider->setValue( 0.8f );
    slider->getElementListener()->handleEvent( EventType::UI, IEvent::CHANGED_STATE_HASH, {},
                                               nativeSlider, nullptr, nullptr );
    BOOST_CHECK_CLOSE( slider->getValue(), 0.8f, 0.001f );
    Thread::setCurrentTask( previousTask );
    slider->setUseNativeUI( false );
    BOOST_CHECK( slider->getBackground()->isEnabled() );
    BOOST_CHECK( !nativeSlider->isVisible() );
    slider->updateVisibility();
    BOOST_CHECK( !nativeSlider->isVisible() );
    slider->setUseNativeUI( true );
    BOOST_CHECK( nativeSlider->isVisible() );
    BOOST_CHECK( !slider->getBackground()->isEnabled() );
    auto properties = slider->getProperties();
    properties->setProperty( "useNativeUI", false );
    slider->setProperties( properties );
    BOOST_CHECK( !slider->getUseNativeUI() );
    BOOST_CHECK( !nativeSlider->isVisible() );
    dropdown->setUseNativeUI( false );
    BOOST_CHECK( !nativeDropdown->isVisible() );
    BOOST_CHECK( dropdown->getButton()->getActor()->isEnabled() );
    dropdown->setOpen( true );
    BOOST_REQUIRE( dropdown->getContent() );
    dropdown->setUseNativeUI( true );
    BOOST_CHECK( nativeDropdown->isVisible() );
    BOOST_CHECK( !dropdown->getContent()->isEnabled() );
}
