#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/UiUtil.hpp>
#include <Workphone/Scene/Components/UI/Button.hpp>
#include <Workphone/Scene/Components/UI/Dropdown.hpp>
#include <Workphone/Scene/Components/UI/GridLayout.hpp>
#include <Workphone/Scene/Components/UI/HorizontalLayout.hpp>
#include <Workphone/Scene/Components/UI/Image.hpp>
#include <Workphone/Scene/Components/UI/InputField.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/UI/ScrollBar.hpp>
#include <Workphone/Scene/Components/UI/ScrollView.hpp>
#include <Workphone/Scene/Components/UI/Slider.hpp>
#include <Workphone/Scene/Components/UI/TableCell.hpp>
#include <Workphone/Scene/Components/UI/TableLayout.hpp>
#include <Workphone/Scene/Components/UI/TabItem.hpp>
#include <Workphone/Scene/Components/UI/TabPage.hpp>
#include <Workphone/Scene/Components/UI/TabView.hpp>
#include <Workphone/Scene/Components/UI/Text.hpp>
#include <Workphone/Scene/Components/UI/Thumbnail.hpp>
#include <Workphone/Scene/Components/UI/Toggle.hpp>
#include <Workphone/Scene/Components/UI/ToggleGroup.hpp>
#include <Workphone/Scene/Components/UI/ToolTip.hpp>
#include <Workphone/Scene/Components/UI/VerticalLayout.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>
#include <Workphone/State/States/UIAnchorStateData.hpp>
#include <Workphone/State/States/UILayoutStateData.hpp>
#include <Workphone/State/States/UITransformStateData.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <type_traits>

namespace workphone::scene
{
    const Array<String> UiUtil::directionNames = { "Horizontal", "Vertical" };
    const Array<String> UiUtil::verticalAlignmentTypes = { "Top", "Bottom", "Center", "Custom" };
    const Array<String> UiUtil::horizontalAlignmentTypes = { "Left", "Right", "Center", "Custom" };
    const String UiUtil::directionStr = "direction";
    const String UiUtil::handleStr = "handle";
    const String UiUtil::backgroundStr = "background";
    const String UiUtil::fillStr = "fill";
    const String UiUtil::scrollValueStr = "scrollValue";
    const String UiUtil::sliderValueStr = "sliderValue";
    const String UiUtil::topStr = "Top";
    const String UiUtil::centerStr = "Center";
    const String UiUtil::bottomStr = "Bottom";
    const String UiUtil::leftStr = "Left";
    const String UiUtil::rightStr = "Right";
    const String UiUtil::customStr = "Custom";
    const String UiUtil::dropDownNameStr = "Dropdown";
    const String UiUtil::optionStr = "Option ";
    const String UiUtil::panelStr = "Panel";

    namespace
    {
        void validateRect( f32 x, f32 y, f32 width, f32 height )
        {
            if( !std::isfinite( x ) || !std::isfinite( y ) || !std::isfinite( width ) ||
                !std::isfinite( height ) || width < 0.0f || height < 0.0f )
            {
                throw std::invalid_argument(
                    "UiUtil requires finite coordinates and nonnegative dimensions" );
            }
        }

        SmartPtr<LayoutTransform> requireTransform( SmartPtr<IGameActor> actor )
        {
            auto transform = actor ? actor->getComponent<LayoutTransform>() : nullptr;
            if( !transform )
            {
                throw std::invalid_argument( "UiUtil requires an actor with a LayoutTransform" );
            }
            if( !transform->getComponentStateByType<UITransformStateData>() ||
                !transform->getComponentStateByType<UIAnchorStateData>() ||
                !transform->getComponentStateByType<UILayoutStateData>() )
            {
                throw std::runtime_error( "UiUtil requires LayoutTransformSystem to be installed" );
            }
            return transform;
        }

        // Own every allocation until publication, including children not yet attached to the root.
        // Destroy individually in reverse order to avoid cascading over an actor twice on failure.
        class UiBuilder
        {
        public:
            explicit UiBuilder( bool addToScene )
            {
                auto application = core::IApplicationManager::instance();
                if( !application )
                {
                    throw std::runtime_error( "UiUtil requires an application manager" );
                }
                manager = application->getGameManager();
                factory = application->getFactoryManager();
                if( !manager || !manager->isLoaded() || !factory )
                {
                    throw std::runtime_error(
                        "UiUtil requires a loaded game manager and factory manager" );
                }
                if( addToScene )
                {
                    scene = manager->getCurrentScene();
                    if( !scene )
                    {
                        throw std::runtime_error( "UiUtil cannot publish without a current scene" );
                    }
                }
            }

            UiBuilder( const UiBuilder & ) = delete;
            UiBuilder &operator=( const UiBuilder & ) = delete;

            ~UiBuilder()
            {
                if( !committed )
                {
                    for( auto it = actors.rbegin(); it != actors.rend(); ++it )
                    {
                        manager->destroyActor( *it, false );
                    }
                }
            }

            template <class T>
            SmartPtr<T> add( SmartPtr<IGameActor> actor )
            {
                auto typeManager = TypeManager::instance();
                if( !typeManager || !factory->getFactoryByHash( typeManager->getHash( T::typeInfo() ) ) )
                {
                    throw std::runtime_error( "UiUtil component factory is unavailable" );
                }
                auto component = factory->make_ptr<T>();
                if( !component )
                {
                    throw std::runtime_error( "UiUtil component factory is unavailable" );
                }
                actor->addComponentInstance( component );
                if( component->getActor().get() != actor.get() || actor->getComponent<T>() != component )
                {
                    throw std::runtime_error( "UiUtil could not attach a component" );
                }
                return component;
            }

            SmartPtr<IGameActor> make( const String &name, f32 width, f32 height,
                                       SmartPtr<IGameActor> parent = nullptr )
            {
                auto actor = manager->createActor();
                if( !actor )
                {
                    throw std::runtime_error( "UiUtil could not allocate an actor" );
                }
                try
                {
                    actors.push_back( actor );
                }
                catch( ... )
                {
                    manager->destroyActor( actor );
                    throw;
                }
                actor->setName( name );
                add<LayoutTransform>( actor )->setAnchor( Vector2<real_Num>( 0.5f, 0.5f ) );
                UiUtil::setRect( actor, 0, 0, width, height );
                UiUtil::setAlignment( actor, HorizontalAlignment::CENTER, VerticalAlignment::CENTER );
                if( parent )
                {
                    parent->addChild( actor );
                    if( actor->getParent() != parent )
                    {
                        throw std::runtime_error( "UiUtil could not attach a child actor" );
                    }
                }
                return actor;
            }

            SmartPtr<IGameActor> panel( const String &name, f32 width, f32 height,
                                        SmartPtr<IGameActor> parent )
            {
                auto actor = make( name, width, height, parent );
                auto image = add<Image>( actor );
                image->setHandleInputEvents( false );
                return actor;
            }

            SmartPtr<IGameActor> finish( SmartPtr<IGameActor> root )
            {
                if( scene )
                {
                    scene->addActor( root );
                    auto publishedActors = scene->getActors();
                    if( root->getScene().get() != scene.get() ||
                        std::find( publishedActors.begin(), publishedActors.end(), root ) ==
                            publishedActors.end() )
                    {
                        throw std::runtime_error( "UiUtil could not publish the UI actor" );
                    }
                    for( auto actor : actors )
                    {
                        actor->setScene( scene );
                    }
                }
                committed = true;
                return root;
            }

            SmartPtr<IGameManager> manager;
            SmartPtr<IFactoryManager> factory;
            SmartPtr<IGameScene> scene;
            std::vector<SmartPtr<IGameActor>> actors;
            bool committed = false;
        };

        template <class T>
        SmartPtr<IGameActor> createContainer( const String &name, const String &fallback,
                                              bool addToScene )
        {
            UiBuilder builder( addToScene );
            auto actor = builder.make( name.empty() ? fallback : name, 300, 200 );
            builder.add<T>( actor );
            return builder.finish( actor );
        }

        template <class T>
        SmartPtr<IGameActor> makeTrack( UiBuilder &builder, const String &name, const String &label,
                                        bool vertical, SmartPtr<IGameActor> parent = nullptr )
        {
            auto actor =
                builder.make( name, vertical ? 30.0f : 200.0f, vertical ? 200.0f : 30.0f, parent );
            auto control = builder.add<T>( actor );
            control->setLabel( label );
            control->setCascadeInput( false );
            auto background = builder.panel( "Background", vertical ? 10.0f : 200.0f,
                                             vertical ? 200.0f : 10.0f, actor );
            auto fill =
                builder.panel( "Fill", vertical ? 10.0f : 200.0f, vertical ? 200.0f : 10.0f, actor );
            auto handle =
                builder.panel( "Handle", vertical ? 25.0f : 50.0f, vertical ? 50.0f : 25.0f, actor );
            // Control positioning uses positive offsets from the track's origin on either axis.
            for( const auto &part : { background, fill, handle } )
            {
                UiUtil::setAlignment( part,
                                      vertical ? HorizontalAlignment::CENTER : HorizontalAlignment::LEFT,
                                      vertical ? VerticalAlignment::TOP : VerticalAlignment::CENTER );
                part->getComponent<LayoutTransform>()->setAnchor(
                    vertical ? Vector2<real_Num>( 0.5f, 1.0f ) : Vector2<real_Num>( 0.0f, 0.5f ) );
            }
            if constexpr( std::is_same_v<T, Slider> )
            {
                handle->getComponent<LayoutTransform>()->setAnchor( Vector2<real_Num>( 0.5f, 0.5f ) );
            }
            else
            {
                control->setUseNativeUI( false );
            }
            background->getComponent<Image>()->setColour( ColourF( 0.3f, 0.3f, 0.3f, 1.0f ) );
            fill->getComponent<Image>()->setColour( ColourF::Blue );
            control->setBackground( background );
            control->setFill( fill );
            control->setHandleActor( handle );
            control->setDirection( vertical ? Direction::Vertical : Direction::Horizontal );
            return actor;
        }
    }  // namespace

    String UiUtil::getHorizontalAlignmentString( HorizontalAlignment alignment )
    {
        const auto index = static_cast<size_t>( alignment );
        return index < horizontalAlignmentTypes.size() ? horizontalAlignmentTypes[index] : String();
    }

    HorizontalAlignment UiUtil::getHorizontalAlignment( const String &str )
    {
        for( size_t i = 0; i < horizontalAlignmentTypes.size(); ++i )
        {
            if( str == horizontalAlignmentTypes[i] )
            {
                return static_cast<HorizontalAlignment>( i );
            }
        }
        return HorizontalAlignment::LEFT;
    }

    String UiUtil::getHorizontalAlignmentTypesString()
    {
        String result;
        for( const auto &name : horizontalAlignmentTypes )
        {
            result += name + ";";
        }
        return result;
    }

    String UiUtil::getVerticalAlignmentString( VerticalAlignment alignment )
    {
        const auto index = static_cast<size_t>( alignment );
        return index < verticalAlignmentTypes.size() ? verticalAlignmentTypes[index] : String();
    }

    VerticalAlignment UiUtil::getVerticalAlignment( const String &str )
    {
        for( size_t i = 0; i < verticalAlignmentTypes.size(); ++i )
        {
            if( str == verticalAlignmentTypes[i] )
            {
                return static_cast<VerticalAlignment>( i );
            }
        }
        return VerticalAlignment::TOP;
    }

    String UiUtil::getVerticalAlignmentTypesString()
    {
        String result;
        for( const auto &name : verticalAlignmentTypes )
        {
            result += name + ";";
        }
        return result;
    }

    void UiUtil::setRect( SmartPtr<IGameActor> actor, f32 x, f32 y, f32 width, f32 height )
    {
        validateRect( x, y, width, height );
        auto transform = requireTransform( actor );
        transform->setPosition( Vector2<real_Num>( x, y ) );
        transform->setSize( Vector2<real_Num>( width, height ) );
    }

    void UiUtil::setAlignment( SmartPtr<IGameActor> actor, HorizontalAlignment horizontal,
                               VerticalAlignment vertical )
    {
        if( horizontal >= HorizontalAlignment::COUNT || vertical >= VerticalAlignment::COUNT )
        {
            throw std::invalid_argument( "UiUtil received an invalid alignment" );
        }
        auto transform = requireTransform( actor );
        auto pivot = transform->getAnchor();
        if( horizontal != HorizontalAlignment::CUSTOM )
        {
            pivot.x = horizontal == HorizontalAlignment::LEFT ? 0.0f :
                      horizontal == HorizontalAlignment::RIGHT ? 1.0f : 0.5f;
        }
        if( vertical != VerticalAlignment::CUSTOM )
        {
            pivot.y = vertical == VerticalAlignment::TOP ? 1.0f :
                      vertical == VerticalAlignment::BOTTOM ? 0.0f : 0.5f;
        }
        transform->setAnchor( pivot );
        transform->setHorizontalAlignment( horizontal );
        transform->setVerticalAlignment( vertical );
        transform->updateAnchorFromAlignment();
    }

    SmartPtr<IGameActor> UiUtil::createText( const String &label, bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor = builder.make( "Text", 300, 100 );
        auto text = builder.add<Text>( actor );
        text->setText( label );
        text->setVerticalAlignment( static_cast<u8>( VerticalAlignment::CENTER ) );
        text->setHorizontalAlignment( static_cast<u8>( HorizontalAlignment::CENTER ) );
        return builder.finish( actor );
    }

    void UiUtil::loadHierarchy( SmartPtr<IGameActor> actor )
    {
        if( !actor )
        {
            throw std::invalid_argument( "UiUtil cannot load a null hierarchy" );
        }
        for( auto component : actor->getComponents() )
        {
            if( component && !component->isLoaded() )
            {
                component->load( nullptr );
                if( !component->isLoaded() )
                {
                    throw std::runtime_error( "UiUtil could not load a hierarchy component" );
                }
            }
        }
        for( auto child : actor->getChildren() )
        {
            if( child->getScene() && child->getScene() != actor->getScene() )
            {
                throw std::invalid_argument( "UiUtil cannot load a child owned by another scene" );
            }
            child->setScene( actor->getScene() );
            loadHierarchy( child );
        }
    }

    SmartPtr<IGameActor> UiUtil::createButton( const String &label, bool addToScene )
    {
        return createButtonEx( label, 0, 0, 300, 60, addToScene );
    }

    SmartPtr<IGameActor> UiUtil::createButtonEx( const String &label, f32 x, f32 y, f32 width,
                                                 f32 height, bool addToScene )
    {
        validateRect( x, y, width, height );
        UiBuilder builder( addToScene );
        auto actor = builder.make( "Button", width, height );
        setRect( actor, x, y, width, height );
        auto button = builder.add<Button>( actor );
        button->setLabel( label );
        button->setCascadeInput( false );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createDropdown( const String &label, bool addToScene )
    {
        return createDropdownWithOptions( label, {}, addToScene );
    }

    SmartPtr<IGameActor> UiUtil::createDropdownWithOptions( const String &label,
                                                            const Array<String> &options,
                                                            bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor = builder.make( dropDownNameStr, 300, 60 );
        auto dropdown = builder.add<Dropdown>( actor );
        dropdown->setLabel( label );
        auto trigger = builder.make( "Dropdown Button", 300, 60, actor );
        setAlignment( trigger, HorizontalAlignment::LEFT, VerticalAlignment::TOP );
        trigger->getComponent<LayoutTransform>()->setAnchor( Vector2<real_Num>( 0, 1 ) );
        auto button = builder.add<Button>( trigger );
        button->setUseNativeUI( false );
        button->setLabel( label );
        button->setCascadeInput( false );
        dropdown->setButton( button );
        for( const auto &text : options )
        {
            auto option = builder.factory->make_ptr<Dropdown::Option>( text );
            if( !option )
            {
                throw std::runtime_error( "UiUtil dropdown option factory is unavailable" );
            }
            dropdown->addOption( option );
        }
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createImage( const String &textureName, bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor = builder.panel( "Image", 100, 100, nullptr );
        if( !textureName.empty() )
        {
            actor->getComponent<Image>()->setTextureName( textureName );
        }
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createPanel( SmartPtr<IBuildDirector>, const String &hint,
                                              bool addToScene )
    {
        UiBuilder builder( addToScene );
        return builder.finish( builder.panel( hint.empty() ? panelStr : hint, 1920, 1080, nullptr ) );
    }

    SmartPtr<IGameActor> UiUtil::createInputField( const String &text, const String &placeholder,
                                                   bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor = builder.make( "InputField", 300, 60 );
        auto input = builder.add<InputField>( actor );
        input->setText( text );
        input->setPlaceholder( placeholder );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createToggle( const String &label, bool toggled, bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor = builder.make( "Toggle", 300, 60 );
        auto toggle = builder.add<Toggle>( actor );
        toggle->setLabel( label );
        toggle->setToggled( toggled );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createSlider( const String &label, SmartPtr<IBuildDirector>,
                                               const String &hint, bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor =
            makeTrack<Slider>( builder, hint.empty() ? String( "Slider" ) : hint, label, false );
        actor->getComponent<Slider>()->setValue( 0.5f );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createSliderVertical( const String &label, SmartPtr<IBuildDirector>,
                                                       const String &hint, bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor =
            makeTrack<Slider>( builder, hint.empty() ? String( "Slider Vertical" ) : hint, label, true );
        actor->getComponent<Slider>()->setValue( 0.5f );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createScrollbar( const String &label, SmartPtr<IBuildDirector>,
                                                  const String &hint, bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor =
            makeTrack<ScrollBar>( builder, hint.empty() ? String( "ScrollBar" ) : hint, label, false );
        auto bar = actor->getComponent<ScrollBar>();
        bar->setHandleSize( 50.0f );
        bar->setScrollValue( 0.0f );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createScrollbarVertical( const String &label, SmartPtr<IBuildDirector>,
                                                          const String &hint, bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor = makeTrack<ScrollBar>( builder, hint.empty() ? String( "ScrollBar Vertical" ) : hint,
                                           label, true );
        auto bar = actor->getComponent<ScrollBar>();
        bar->setHandleSize( 50.0f );
        bar->setScrollValue( 0.0f );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createScrollview( const String &label, SmartPtr<IBuildDirector>,
                                                   const String &hint, bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor = builder.make( hint.empty() ? String( "ScrollView" ) : hint, 300, 200 );
        auto view = builder.add<ScrollView>( actor );
        view->setUseNativeUI( false );
        view->setLabel( label );
        auto content = builder.make( "Content", 270, 200, actor );
        setAlignment( content, HorizontalAlignment::LEFT, VerticalAlignment::TOP );
        content->getComponent<LayoutTransform>()->setAnchor( Vector2<real_Num>( 0, 1 ) );
        view->setContentPanel( content->getComponent<LayoutTransform>() );
        auto scrollbar = makeTrack<ScrollBar>( builder, "ScrollBar", "", true, actor );
        setRect( scrollbar, 270, 0, 30, 200 );
        setAlignment( scrollbar, HorizontalAlignment::LEFT, VerticalAlignment::TOP );
        scrollbar->getComponent<LayoutTransform>()->setAnchor( Vector2<real_Num>( 0, 1 ) );
        view->setScrollBar( scrollbar->getComponent<ScrollBar>() );
        view->updateScrollBar();
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createScrollView( const String &label, bool addToScene )
    {
        return createScrollview( label, nullptr, "", addToScene );
    }

    SmartPtr<IGameActor> UiUtil::createGridLayout( const String &name, s32 columns, bool addToScene )
    {
        if( columns <= 0 )
        {
            throw std::invalid_argument( "UiUtil grid column count must be positive" );
        }
        UiBuilder builder( addToScene );
        auto actor = builder.make( name.empty() ? String( "GridLayout" ) : name, 300, 200 );
        builder.add<GridLayout>( actor )->setColumnCount( columns );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createTabItem( const String &label, bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor = builder.make( "TabItem", 150, 40 );
        builder.add<TabItem>( actor )->setLabel( label );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createToolTip( const String &text, bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor = builder.make( "ToolTip", 300, 60 );
        builder.add<ToolTip>( actor )->setTooltip( text );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createThumbnail( const String &label, const String &textureName,
                                                  bool addToScene )
    {
        UiBuilder builder( addToScene );
        auto actor = builder.make( "Thumbnail", 100, 130 );
        auto thumbnail = builder.add<Thumbnail>( actor );
        auto imageActor = builder.panel( "Image", 100, 100, actor );
        auto image = imageActor->getComponent<Image>();
        if( !textureName.empty() )
        {
            image->setTextureName( textureName );
        }
        setAlignment( imageActor, HorizontalAlignment::LEFT, VerticalAlignment::TOP );
        imageActor->getComponent<LayoutTransform>()->setAnchor( Vector2<real_Num>( 0, 1 ) );
        thumbnail->setThumb( image );
        auto textActor = builder.make( "Text", 100, 30, actor );
        setAlignment( textActor, HorizontalAlignment::LEFT, VerticalAlignment::TOP );
        textActor->getComponent<LayoutTransform>()->setAnchor( Vector2<real_Num>( 0, 1 ) );
        setRect( textActor, 0, 100, 100, 30 );
        auto text = builder.add<Text>( textActor );
        text->setText( label );
        thumbnail->setLabelText( text );
        return builder.finish( actor );
    }

    SmartPtr<IGameActor> UiUtil::createLayout( const String &name, bool addToScene )
    {
        return createContainer<Layout>( name, "Layout", addToScene );
    }

    SmartPtr<IGameActor> UiUtil::createHorizontalLayout( const String &name, bool addToScene )
    {
        return createContainer<HorizontalLayout>( name, "HorizontalLayout", addToScene );
    }

    SmartPtr<IGameActor> UiUtil::createVerticalLayout( const String &name, bool addToScene )
    {
        return createContainer<VerticalLayout>( name, "VerticalLayout", addToScene );
    }

    SmartPtr<IGameActor> UiUtil::createTableLayout( const String &name, bool addToScene )
    {
        return createContainer<TableLayout>( name, "TableLayout", addToScene );
    }

    SmartPtr<IGameActor> UiUtil::createTableCell( const String &name, bool addToScene )
    {
        return createContainer<TableCell>( name, "TableCell", addToScene );
    }

    SmartPtr<IGameActor> UiUtil::createTabView( const String &name, bool addToScene )
    {
        return createContainer<TabView>( name, "TabView", addToScene );
    }

    SmartPtr<IGameActor> UiUtil::createTabPage( const String &name, bool addToScene )
    {
        return createContainer<TabPage>( name, "TabPage", addToScene );
    }

    SmartPtr<IGameActor> UiUtil::createToggleGroup( const String &name, bool addToScene )
    {
        return createContainer<ToggleGroup>( name, "ToggleGroup", addToScene );
    }
}  // namespace workphone::scene
