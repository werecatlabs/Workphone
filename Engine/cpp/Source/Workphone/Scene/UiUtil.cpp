#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/UiUtil.hpp>
#include <Workphone/Scene/Components/UI/Button.hpp>
#include <Workphone/Scene/Components/UI/Dropdown.hpp>
#include <Workphone/Scene/Components/UI/Image.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/UI/ScrollBar.hpp>
#include <Workphone/Scene/Components/UI/ScrollView.hpp>
#include <Workphone/Scene/Components/UI/Slider.hpp>
#include <Workphone/Scene/Components/UI/Text.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>

namespace workphone::scene
{
    const Array<String> UiUtil::directionNames = Array<String>{ "Horizontal", "Vertical" };

    const Array<String> UiUtil::verticalAlignmentTypes = { "Top", "Bottom", "Center", "Custom" };
    const Array<String> UiUtil::horizontalAlignmentTypes = { "Left", "Right", "Center", "Custom" };

    const String UiUtil::directionStr = "direction";

    const String UiUtil::handleStr = "handle";
    const String UiUtil::backgroundStr = "background";
    const String UiUtil::fillStr = "fill";
    const String UiUtil::scrollValueStr = "scrollValue";
    const String UiUtil::sliderValueStr = "sliderValue";

    const String UiUtil::topStr = String( "Top" );
    const String UiUtil::centerStr = String( "Center" );
    const String UiUtil::bottomStr = String( "Bottom" );

    const String UiUtil::leftStr = String( "Left" );
    const String UiUtil::rightStr = String( "Right" );

    const String UiUtil::dropDownNameStr = String( "Dropdown" );
    const String UiUtil::optionStr = String( "Option " );
    const String UiUtil::panelStr = String( "Panel" );

    String UiUtil::getHorizontalAlignmentString( HorizontalAlignment horizontalAlignment )
    {
        switch( horizontalAlignment )
        {
        case HorizontalAlignment::LEFT:
            return UiUtil::leftStr;
        case HorizontalAlignment::CENTER:
            return UiUtil::centerStr;
        case HorizontalAlignment::RIGHT:
            return UiUtil::rightStr;
        default:
            break;
        }

        return {};
    }

    HorizontalAlignment UiUtil::getHorizontalAlignment( const String &str )
    {
        if( str == UiUtil::leftStr )
        {
            return HorizontalAlignment::LEFT;
        }
        if( str == UiUtil::centerStr )
        {
            return HorizontalAlignment::CENTER;
        }
        if( str == UiUtil::rightStr )
        {
            return HorizontalAlignment::RIGHT;
        }

        return HorizontalAlignment::LEFT;
    }

    String UiUtil::getHorizontalAlignmentTypesString()
    {
        auto enumValues = String();
        enumValues.reserve( 256 );

        for( size_t i = 0; i < static_cast<size_t>( HorizontalAlignment::COUNT ); ++i )
        {
            auto eMaterialType = static_cast<HorizontalAlignment>( i );
            auto eMaterialTypeStr = getHorizontalAlignmentString( eMaterialType );
            enumValues += eMaterialTypeStr + ";";
        }

        return enumValues;
    }

    String UiUtil::getVerticalAlignmentString( VerticalAlignment verticalAlignment )
    {
        switch( verticalAlignment )
        {
        case VerticalAlignment::TOP:
            return UiUtil::topStr;
        case VerticalAlignment::CENTER:
            return UiUtil::centerStr;
        case VerticalAlignment::BOTTOM:
            return UiUtil::bottomStr;
        default:
            break;
        }

        return {};
    }

    VerticalAlignment UiUtil::getVerticalAlignment( const String &str )
    {
        if( str == UiUtil::topStr )
        {
            return VerticalAlignment::TOP;
        }
        if( str == UiUtil::centerStr )
        {
            return VerticalAlignment::CENTER;
        }
        if( str == UiUtil::bottomStr )
        {
            return VerticalAlignment::BOTTOM;
        }

        return VerticalAlignment::TOP;
    }

    String UiUtil::getVerticalAlignmentTypesString()
    {
        auto enumValues = String();
        enumValues.reserve( 256 );

        for( size_t i = 0; i < static_cast<size_t>( VerticalAlignment::COUNT ); ++i )
        {
            auto eMaterialType = static_cast<VerticalAlignment>( i );
            auto eMaterialTypeStr = getVerticalAlignmentString( eMaterialType );
            enumValues += eMaterialTypeStr + ";";
        }

        return enumValues;
    }

    SmartPtr<IGameActor> UiUtil::createDropdown( const String &label, bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();

        auto sceneManager = applicationManager->getGameManagerPtr();
        auto scene = sceneManager->getCurrentScenePtr();

        auto actor = sceneManager->createActorPtr();

        actor->setName( UiUtil::dropDownNameStr );

        auto canvasTransform = actor->addComponentPtr<LayoutTransform>();

        auto dropdown = actor->addComponentPtr<Dropdown>();

        const auto numOptions = 3;
        for( s32 i = 0; i < numOptions; i++ )
        {
            auto optionName = optionStr + StringUtil::toString( i + 1 );
            auto option = factoryManager->make_ptr<Dropdown::Option>( optionName );

            dropdown->addOption( option );
        }

        /*
        auto buttonActor = createButton( UiUtil::dropDownNameStr, false );
        actor->addChild( buttonActor );

        auto button = buttonActor->getComponentPtr<Button>();

        auto buttonNormalColour = ColourF::White * 0.3f;
        buttonNormalColour.a = 1.0f;

        button->setNormalColour( buttonNormalColour );
        dropdown->setButton( button );

        if( auto buttonTransform = buttonActor->getComponentPtr<LayoutTransform>() )
        {
            buttonTransform->setPosition( Vector2<real_Num>( 150, 0 ) );
            buttonTransform->setAnchor( Vector2<real_Num>( 0.5f, 0.5f ) );
            buttonTransform->setHorizontalAlignment( HorizontalAlignment::CENTER );
            buttonTransform->setVerticalAlignment( VerticalAlignment::CENTER );
            buttonTransform->updateAnchorFromAlignment();
        }

        if( auto buttonImage = buttonActor->getComponent<Image>() )
        {
        }

        auto textActor = createText( "Dropdown", false );
        actor->addChild( textActor );
        dropdown->setLabelActor( textActor );

        if( auto textLayout = textActor->getComponentPtr<LayoutTransform>() )
        {
            textLayout->setPosition( Vector2<real_Num>( -150.0f, 0.0f ) );
            textLayout->setHorizontalAlignment( HorizontalAlignment::CENTER );
            textLayout->setVerticalAlignment( VerticalAlignment::CENTER );
            textLayout->updateAnchorFromAlignment();
        }
        */

        return actor;
    }

    SmartPtr<IGameActor> UiUtil::createButton( const String &label, bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManagerPtr();
        auto scene = sceneManager->getCurrentScenePtr();

        auto actor = sceneManager->createActorPtr();
        WP_ASSERT( actor );

        auto name = String( "Button" );
        actor->setName( name );

        auto size = Vector2<real_Num>( 300, 60 );

        auto anchor = Vector2<real_Num>( 0.5f, 0.5f );

        auto canvasTransform = actor->addComponent<LayoutTransform>();
        if( canvasTransform )
        {
            canvasTransform->setSize( size );

            canvasTransform->setAnchor( anchor );

            canvasTransform->setHorizontalAlignment( HorizontalAlignment::CENTER );
            canvasTransform->setVerticalAlignment( VerticalAlignment::CENTER );
            canvasTransform->updateAnchorFromAlignment();
        }

        auto button = actor->addComponent<Button>();
        WP_ASSERT( button );

        button->setCascadeInput( false );

        //if( auto image = actor->addComponent<Image>() )
        //{
        //    auto textureName = String( "rounded_filled_1024.png" );
        //    image->setTextureName( textureName );

        //    auto colour = ColourF::White * 0.3f;
        //    colour.a = 1.0f;

        //    image->setColour( colour );

        //    button->setImage( image );
        //}

        //auto actorText = sceneManager->createActor();
        //actor->addChild( actorText );

        //if( auto textCanvasTransform = actorText->addComponent<LayoutTransform>() )
        //{
        //    auto textSize = Vector2<real_Num>( 300, 60 );
        //    textCanvasTransform->setSize( textSize );

        //    textCanvasTransform->setAnchor( anchor );

        //    textCanvasTransform->setHorizontalAlignment( HorizontalAlignment::CENTER );
        //    textCanvasTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        //    textCanvasTransform->updateAnchorFromAlignment();
        //}

        //if( auto text = actorText->addComponent<Text>() )
        //{
        //    text->setVerticalAlignment( 2 );
        //    text->setHorizontalAlignment( 2 );
        //    text->setText( label );

        //    auto textName = String( "Text" );
        //    actorText->setName( textName );

        //    button->setText( text );
        //    text->updateElementState();
        //}

        if( addToScene )
        {
            scene->addActor( actor );
        }

        return actor;
    }

    SmartPtr<IGameActor> UiUtil::createText( const String &label, bool addToScene )
    {
        using namespace scene;

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        auto name = String( "Text" );
        actor->setName( name );

        auto canvasTransform = actor->addComponent<LayoutTransform>();
        if( canvasTransform )
        {
            auto size = Vector2<real_Num>( 300, 100 );
            canvasTransform->setSize( size );

            auto anchor = Vector2<real_Num>( 0.5, 0.5 );
            canvasTransform->setAnchor( anchor );

            canvasTransform->setVerticalAlignment( VerticalAlignment::CENTER );
            canvasTransform->setHorizontalAlignment( HorizontalAlignment::CENTER );
            canvasTransform->updateAnchorFromAlignment();
        }

        auto text = actor->addComponent<Text>();
        if( text )
        {
            text->setText( label );
            text->setVerticalAlignment( static_cast<u8>( VerticalAlignment::CENTER ) );
            text->setHorizontalAlignment( static_cast<u8>( HorizontalAlignment::CENTER ) );
        }

        return actor;
    }

    SmartPtr<IGameActor> UiUtil::createScrollbar( const String &label, SmartPtr<IBuildDirector> director,
                                                  const String &hint, bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        static const auto name = String( "ScrollBar" );
        actor->setName( name );

        auto canvasTransform = actor->addComponent<LayoutTransform>();
        if( canvasTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            canvasTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 200, 60 );
            canvasTransform->setSize( size );
        }

        auto slider = actor->addComponent<ScrollBar>();
        slider->setScrollValue( 0.5f );

        auto background = createPanel( nullptr, String(), false );
        background->setName( "Background" );
        actor->addChild( background );
        slider->setBackground( background );

        auto backgroundTransform = background->getComponent<LayoutTransform>();
        if( backgroundTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            backgroundTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 200, 10 );
            backgroundTransform->setSize( size );

            backgroundTransform->setAnchor( Vector2<real_Num>( 0.0, 0.5 ) );

            backgroundTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            backgroundTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        }

        if( auto imageBackground = background->getComponent<Image>() )
        {
            imageBackground->setColour( ColourF::White * 0.3f );
        }

        auto fill = createPanel( nullptr, String(), false );
        fill->setName( "Fill" );
        actor->addChild( fill );
        //slider->setFill( fill );

        auto fillTransform = fill->getComponent<LayoutTransform>();
        if( fillTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            fillTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 200, 10 );
            fillTransform->setSize( size );

            fillTransform->setAnchor( Vector2<real_Num>( 0.0, 0.5 ) );

            fillTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            fillTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        }

        if( auto imageFill = fill->getComponent<Image>() )
        {
            imageFill->setColour( ColourF::Blue );
        }

        auto handle = createPanel( nullptr, String(), false );
        handle->setName( "Handle" );
        actor->addChild( handle );
        slider->setHandleActor( handle );

        auto handleTransform = handle->getComponent<LayoutTransform>();
        if( handleTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            handleTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 50, 25 );
            handleTransform->setSize( size );

            handleTransform->setAnchor( Vector2<real_Num>( 0.5, 0.5 ) );

            handleTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            handleTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        }

        auto imageHandle = handle->getComponent<Image>();
        if( imageHandle )
        {
            imageHandle->setTextureName( "circle_filled_1024.png" );
        }

        return actor;
    }

    SmartPtr<IGameActor> UiUtil::createScrollbarVertical( const String &label,
                                                          SmartPtr<IBuildDirector> director,
                                                          const String &hint, bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        static const auto name = String( "ScrollBar Vertical" );
        actor->setName( name );

        auto canvasTransform = actor->addComponent<LayoutTransform>();
        if( canvasTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            canvasTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 60, 200 );
            canvasTransform->setSize( size );
        }

        auto slider = actor->addComponent<ScrollBar>();
        slider->setScrollValue( 0.5f );

        /*
        auto background = createPanel( nullptr, String(), false );
        background->setName( "Background" );
        actor->addChild( background );
        slider->setBackground( background );

        auto backgroundTransform = background->getComponent<LayoutTransform>();
        if( backgroundTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            backgroundTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 10, 200 );
            backgroundTransform->setSize( size );

            backgroundTransform->setAnchor( Vector2<real_Num>( 0.0, 1.0 ) );

            backgroundTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            backgroundTransform->setVerticalAlignment( VerticalAlignment::TOP );
        }

        if( auto imageBackground = background->getComponent<Image>() )
        {
            imageBackground->setColour( ColourF::White * 0.3f );
        }

        auto fill = createPanel( nullptr, String(), false );
        fill->setName( "Fill" );
        actor->addChild( fill );
        slider->setFill( fill );

        auto fillTransform = fill->getComponent<LayoutTransform>();
        if( fillTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            fillTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 10, 200 );
            fillTransform->setSize( size );

            fillTransform->setAnchor( Vector2<real_Num>( 0.0, 1.0 ) );

            fillTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            fillTransform->setVerticalAlignment( VerticalAlignment::TOP );
        }

        if( auto imageFill = fill->getComponent<Image>() )
        {
            imageFill->setColour( ColourF::Blue );
        }

        auto handle = createPanel( nullptr, String(), false );
        handle->setName( "Handle" );
        actor->addChild( handle );
        slider->setHandleActor( handle );

        auto handleTransform = handle->getComponent<LayoutTransform>();
        if( handleTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            handleTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 50, 25 );
            handleTransform->setSize( size );

            handleTransform->setAnchor( Vector2<real_Num>( 0.5, 0.5 ) );

            handleTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            handleTransform->setVerticalAlignment( VerticalAlignment::TOP );
        }

        auto imageHandle = handle->getComponent<Image>();
        if( imageHandle )
        {
            imageHandle->setTextureName( "circle_filled_1024.png" );
        }
        */

        return actor;
    }

    SmartPtr<IGameActor> UiUtil::createScrollview( const String &label,
                                                   SmartPtr<IBuildDirector> director, const String &hint,
                                                   bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        static const auto name = String( "ScrollView" );
        actor->setName( name );

        auto canvasTransform = actor->addComponent<LayoutTransform>();
        if( canvasTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            canvasTransform->setPosition( pos );
            auto size = Vector2<real_Num>( 200, 60 );
            canvasTransform->setSize( size );
        }

        auto scrollView = actor->addComponent<ScrollView>();
        //scrollView->setScrollValue( 0.5f );
        return actor;
    }

    SmartPtr<IGameActor> UiUtil::createSlider( const String &label, SmartPtr<IBuildDirector> director,
                                               const String &hint, bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        static const auto name = String( "Slider" );
        actor->setName( name );

        auto canvasTransform = actor->addComponent<LayoutTransform>();
        if( canvasTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            canvasTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 200, 60 );
            canvasTransform->setSize( size );
        }

        auto slider = actor->addComponent<Slider>();
        slider->setValue( 0.5f );

        auto background = createPanel( nullptr, String(), false );
        background->setName( "Background" );
        actor->addChild( background );
        slider->setBackground( background );

        auto backgroundTransform = background->getComponent<LayoutTransform>();
        if( backgroundTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            backgroundTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 200, 10 );
            backgroundTransform->setSize( size );

            backgroundTransform->setAnchor( Vector2<real_Num>( 0.0, 0.5 ) );

            backgroundTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            backgroundTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        }

        if( auto imageBackground = background->getComponent<Image>() )
        {
            imageBackground->setColour( ColourF::White * 0.3f );
        }

        auto fill = createPanel( nullptr, String(), false );
        fill->setName( "Fill" );
        actor->addChild( fill );
        slider->setFill( fill );

        auto fillTransform = fill->getComponent<LayoutTransform>();
        if( fillTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            fillTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 200, 10 );
            fillTransform->setSize( size );

            fillTransform->setAnchor( Vector2<real_Num>( 0.0, 0.5 ) );

            fillTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            fillTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        }

        if( auto imageFill = fill->getComponent<Image>() )
        {
            imageFill->setColour( ColourF::Blue );
        }

        auto handle = createPanel( nullptr, String(), false );
        handle->setName( "Handle" );
        actor->addChild( handle );
        slider->setHandleActor( handle );

        auto handleTransform = handle->getComponent<LayoutTransform>();
        if( handleTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            handleTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 50, 25 );
            handleTransform->setSize( size );

            handleTransform->setAnchor( Vector2<real_Num>( 0.5, 0.5 ) );

            handleTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            handleTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        }

        auto imageHandle = handle->getComponent<Image>();
        if( imageHandle )
        {
            imageHandle->setTextureName( "circle_filled_1024.png" );
        }

        return actor;
    }

    SmartPtr<IGameActor> UiUtil::createSliderVertical( const String &label,
                                                       SmartPtr<IBuildDirector> director,
                                                       const String &hint, bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManagerPtr();
        auto scene = sceneManager->getCurrentScenePtr();

        auto actor = sceneManager->createActor();

        static const auto name = String( "Slider Vertical" );
        actor->setName( name );

        auto canvasTransform = actor->addComponentPtr<LayoutTransform>();
        if( canvasTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            canvasTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 60, 200 );
            canvasTransform->setSize( size );
        }

        auto slider = actor->addComponent<Slider>();
        slider->setValue( 0.5f );

        /*
        auto background = createPanel( nullptr, String(), false );
        background->setName( "Background" );
        actor->addChild( background );
        slider->setBackground( background );

        auto backgroundTransform = background->getComponent<LayoutTransform>();
        if( backgroundTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            backgroundTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 10, 200 );
            backgroundTransform->setSize( size );

            backgroundTransform->setAnchor( Vector2<real_Num>( 0.0, 1.0 ) );

            backgroundTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            backgroundTransform->setVerticalAlignment( VerticalAlignment::TOP );
        }

        if( auto imageBackground = background->getComponent<Image>() )
        {
            imageBackground->setColour( ColourF::White * 0.3f );
        }

        auto fill = createPanel( nullptr, String(), false );
        fill->setName( "Fill" );
        actor->addChild( fill );
        slider->setFill( fill );

        auto fillTransform = fill->getComponent<LayoutTransform>();
        if( fillTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            fillTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 10, 200 );
            fillTransform->setSize( size );

            fillTransform->setAnchor( Vector2<real_Num>( 0.0, 1.0 ) );

            fillTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            fillTransform->setVerticalAlignment( VerticalAlignment::TOP );
        }

        if( auto imageFill = fill->getComponent<Image>() )
        {
            imageFill->setColour( ColourF::Blue );
        }

        auto handle = createPanel( nullptr, String(), false );
        handle->setName( "Handle" );
        actor->addChild( handle );
        slider->setHandleActor( handle );

        auto handleTransform = handle->getComponent<LayoutTransform>();
        if( handleTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            handleTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 50, 25 );
            handleTransform->setSize( size );

            handleTransform->setAnchor( Vector2<real_Num>( 0.5, 0.5 ) );

            handleTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            handleTransform->setVerticalAlignment( VerticalAlignment::TOP );
        }

        auto imageHandle = handle->getComponent<Image>();
        if( imageHandle )
        {
            imageHandle->setTextureName( "circle_filled_1024.png" );
        }
        */

        return actor;
    }

    SmartPtr<IGameActor> UiUtil::createPanel( SmartPtr<IBuildDirector> director, const String &hint,
                                              bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        actor->setName( panelStr );

        auto canvasTransform = actor->addComponent<LayoutTransform>();
        if( canvasTransform )
        {
            auto size = Vector2<real_Num>( 1920, 1080 );
            canvasTransform->setSize( size );
        }

        auto image = actor->addComponent<Image>();
        image->setTextureName( "panel.png" );

        return actor;
    }
}  // namespace workphone::scene
