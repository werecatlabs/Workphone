#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

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
        TestGuard guard;

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
