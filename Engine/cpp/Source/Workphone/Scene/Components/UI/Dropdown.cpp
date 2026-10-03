#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/Dropdown.hpp>
#include <Workphone/Scene/Components/UI/Button.hpp>
#include <Workphone/Scene/Components/UI/Image.hpp>
#include <Workphone/Scene/Components/UI/ScrollBar.hpp>
#include <Workphone/Scene/Components/UI/ScrollView.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/UI/VerticalLayout.hpp>
#include <Workphone/Scene/UiUtil.hpp>
#include <Workphone/Interface/IApplication.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/UI/IUIDropdown.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Dropdown, UIComponent );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Dropdown::Option, ISharedObject );

    const String Dropdown::dropdownButtonStr = "dropdownButtonStr";
    const String Dropdown::dropdownPanelStr = "dropdownPanelStr";
    const String Dropdown::isOpenStr = "isOpenStr";
    const String Dropdown::optionStr = "Option";
    const String Dropdown::imageStr = "image";
    const String Dropdown::textStr = "text";

    const String Dropdown::panelColourStr = "panelColour";
    const String Dropdown::contentOffsetStr = "contentOffset";
    const String Dropdown::contentHeightStr = "contentHeight";
    const String Dropdown::panelSizeStr = "panelSize";
    const String Dropdown::contentZOrderStr = "contentZOrder";
    const String Dropdown::optionButtonColourStr = "optionButtonColour";
    const String Dropdown::contentNameStr = "contentName";
    const String Dropdown::panelNameStr = "panelName";
    const String Dropdown::scrollbarNameStr = "scrollbarName";
    const String Dropdown::scrollbarBackgroundNameStr = "scrollbarBackgroundName";
    const String Dropdown::scrollbarZOrderOffsetStr = "scrollbarZOrderOffset";
    const String Dropdown::initialOptionCapacityStr = "initialOptionCapacity";

    Dropdown::Dropdown()
    {
    }

    Dropdown::~Dropdown()
    {
    }

    void Dropdown::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            m_options.reserve( m_initialOptionCapacity );
            createUI();

            UIComponent::load( data );

            if( data )
            {
                if( data->isExactly<Properties>() )
                {
                    setProperties( data );
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Dropdown::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                UIComponent::unload( data );

                m_panel = nullptr;
                m_content = nullptr;
                m_options.clear();
                m_button = nullptr;
                m_optionPrefab = nullptr;

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Array<SmartPtr<ISharedObject>> Dropdown::getChildObjects() const
    {
        auto objects = UIComponent::getChildObjects();

        objects.push_back( m_button );
        objects.push_back( m_panel );
        return objects;
    }

    SmartPtr<Properties> Dropdown::getProperties() const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto properties = UIComponent::getProperties();
        properties->setPropertyAsType<Button>( dropdownButtonStr, m_button );
        properties->setPropertyAsType( dropdownPanelStr, m_panel );
        properties->setProperty( isOpenStr, m_isOpen );

        properties->setProperty( panelColourStr, m_bgPanelColour );
        properties->setProperty( optionButtonColourStr, m_optionButtonColour );
        properties->setProperty( contentOffsetStr, m_contentOffset );
        properties->setProperty( contentHeightStr, m_contentHeight );
        properties->setProperty( panelSizeStr, m_panelSize );
        properties->setProperty( contentZOrderStr, m_contentZOrder );
        properties->setProperty( contentNameStr, m_contentName );
        properties->setProperty( panelNameStr, m_panelName );
        properties->setProperty( scrollbarNameStr, m_scrollbarName );
        properties->setProperty( scrollbarBackgroundNameStr, m_scrollbarBackgroundName );
        properties->setProperty( scrollbarZOrderOffsetStr, m_scrollbarZOrderOffset );
        properties->setProperty( initialOptionCapacityStr, m_initialOptionCapacity );

        for( auto &option : m_options )
        {
            if( !option )
            {
                WP_LOG_ERROR( "Skipping null dropdown option while serialising properties." );
                continue;
            }

            auto optionProperties = factoryManager->make_ptr<Properties>();
            optionProperties->setName( optionStr );
            optionProperties->setProperty( textStr, option->text );
            optionProperties->setProperty( imageStr, option->imageTexture );
            properties->addChild( optionProperties );
        }

        return properties;
    }

    void Dropdown::setProperties( SmartPtr<Properties> properties )
    {
        UIComponent::setProperties( properties );

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto open = isOpen();

        SmartPtr<Button> button;

        properties->getPropertyAsType( dropdownButtonStr, button );
        properties->getPropertyAsType( dropdownPanelStr, m_panel );
        properties->getPropertyValue( isOpenStr, open );

        ColourF panelColour = m_bgPanelColour;
        if( properties->getPropertyValue( panelColourStr, panelColour ) )
        {
            setPanelColour( panelColour );
        }

        ColourF optionButtonColour = m_optionButtonColour;
        if( properties->getPropertyValue( optionButtonColourStr, optionButtonColour ) )
        {
            setOptionButtonColour( optionButtonColour );
        }

        f32 contentOffset = m_contentOffset;
        if( properties->getPropertyValue( contentOffsetStr, contentOffset ) )
        {
            setContentOffset( contentOffset );
        }

        f32 contentHeight = m_contentHeight;
        if( properties->getPropertyValue( contentHeightStr, contentHeight ) )
        {
            setContentHeight( contentHeight );
        }

        Vector2<real_Num> panelSize = m_panelSize;
        if( properties->getPropertyValue( panelSizeStr, panelSize ) )
        {
            setPanelSize( panelSize );
        }

        s32 contentZOrder = m_contentZOrder;
        if( properties->getPropertyValue( contentZOrderStr, contentZOrder ) )
        {
            setContentZOrder( contentZOrder );
        }

        String contentName = m_contentName;
        if( properties->getPropertyValue( contentNameStr, contentName ) )
        {
            setContentName( contentName );
        }

        String panelName = m_panelName;
        if( properties->getPropertyValue( panelNameStr, panelName ) )
        {
            setPanelName( panelName );
        }

        String scrollbarName = m_scrollbarName;
        if( properties->getPropertyValue( scrollbarNameStr, scrollbarName ) )
        {
            setScrollbarName( scrollbarName );
        }

        String scrollbarBackgroundName = m_scrollbarBackgroundName;
        if( properties->getPropertyValue( scrollbarBackgroundNameStr, scrollbarBackgroundName ) )
        {
            setScrollbarBackgroundName( scrollbarBackgroundName );
        }

        s32 scrollbarZOrderOffset = m_scrollbarZOrderOffset;
        if( properties->getPropertyValue( scrollbarZOrderOffsetStr, scrollbarZOrderOffset ) )
        {
            setScrollbarZOrderOffset( scrollbarZOrderOffset );
        }

        u32 initialOptionCapacity = m_initialOptionCapacity;
        if( properties->getPropertyValue( initialOptionCapacityStr, initialOptionCapacity ) )
        {
            setInitialOptionCapacity( initialOptionCapacity );
        }

        setButton( button );

        m_options.clear();

        auto optionProperties = properties->getChildrenByName( optionStr );
        for( auto optionProperty : optionProperties )
        {
            String text;

            auto option = factoryManager->make_ptr<Option>();
            optionProperty->getPropertyValue( textStr, text );
            optionProperty->getPropertyValue( imageStr, option->imageTexture );

            option->text = text;
            m_options.push_back( option );
        }

        m_isOpen = open;
        buildOptionsPanel();

        if( m_content )
        {
            m_content->setEnabled( m_isOpen );
        }
    }

    bool Dropdown::isValid() const
    {
        auto valid = UIComponent::isValid();
        return valid;
    }

    void Dropdown::removeOption( SmartPtr<Option> option )
    {
        m_options.erase( std::remove( m_options.begin(), m_options.end(), option ), m_options.end() );
    }

    void Dropdown::removeOptions()
    {
        m_options.clear();
    }

    void Dropdown::addOption( SmartPtr<Option> option )
    {
        if( !option )
        {
            WP_LOG_ERROR( "Cannot add a null dropdown option." );
            return;
        }

        m_options.push_back( option );
    }

    void Dropdown::setOptions( const Array<SmartPtr<Option>> &options )
    {
        m_options = options;
    }

    Array<SmartPtr<Dropdown::Option>> Dropdown::getOptions() const
    {
        return m_options;
    }

    void Dropdown::setOpen( bool open )
    {
        m_isOpen = open;
    }

    bool Dropdown::isOpen() const
    {
        return m_isOpen;
    }

    void Dropdown::setOptionPrefab( SmartPtr<IGameActor> optionPrefab )
    {
        m_optionPrefab = optionPrefab;
    }

    SmartPtr<IGameActor> Dropdown::getOptionPrefab() const
    {
        return m_optionPrefab;
    }

    void Dropdown::setContent( SmartPtr<IGameActor> content )
    {
        m_content = content;
    }

    IGameActor *Dropdown::getContentPtr() const
    {
        return m_content.get();
    }

    SmartPtr<IGameActor> Dropdown::getContent() const
    {
        return m_content;
    }

    void Dropdown::setPanel( SmartPtr<IGameActor> panel )
    {
        m_panel = panel;
    }

    SmartPtr<IGameActor> Dropdown::getPanel() const
    {
        return m_panel;
    }

    void Dropdown::setButton( SmartPtr<Button> button )
    {
        auto pButton = button.get();

        if( m_button != pButton )
        {
            m_button = pButton;

            if( m_button )
            {
                auto func = [this]( hash_type value ) {
                    if( auto actor = getActorPtr() )
                    {
                        auto enabled = isEnabled() && actor->isEnabledInScene();
                        if( enabled )
                        {
                            m_isOpen = !m_isOpen;
                            updateOptionsPanelVisibility();
                        }
                    }
                };

                m_button->setCallbackFunction( func );
            }
        }
    }

    SmartPtr<Button> Dropdown::getButton() const
    {
        return m_button;
    }

    FSMReturnType Dropdown::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        switch( eventType )
        {
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                buildOptionsPanel();
                updateOptionsPanelVisibility();
            }
            break;
            default:
            {
            }
            break;
            };
        }
        break;
        case FSMEvent::Leave:
        {
        }
        break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    Parameter Dropdown::handleEvent( EventType eventType, hash_type eventValue,
                                     const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                     SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventType == EventType::UI )
        {
            if( eventValue == IEvent::CLICK_HASH )
            {
                if( auto actor = getActorPtr() )
                {
                    auto enabled = isEnabled() && actor->isEnabledInScene();
                    if( enabled )
                    {
                        m_isOpen = !m_isOpen;
                        updateOptionsPanelVisibility();
                    }
                }
            }
        }

        return {};
    }

    void Dropdown::buildOptionsPanel()
    {
        if( auto element = getElement() )
        {
            auto dropdown = workphone::dynamic_pointer_cast<ui::IUIDropdown>( element );
            if( dropdown )
            {
                auto options = Array<String>();
                options.reserve( m_options.size() );

                for( auto &option : m_options )
                {
                    if( option )
                    {
                        options.push_back( option->text );
                    }
                    else
                    {
                        WP_LOG_ERROR( "Skipping null dropdown option." );
                    }
                }

                dropdown->setOptions( options );
            }
            else
            {
                WP_LOG_ERROR( "Dropdown element cannot be cast to IUIDropdown; options were not set." );
            }

            return;
        }

        if( auto actor = getActorPtr() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto application = applicationManager->getApplication();
            if( !application )
            {
                WP_LOG_ERROR( "Application is null." );
                return;
            }

            if( m_content )
            {
                m_content->destroyChildren();
            }

            m_panel = nullptr;

            auto panelColour = getPanelColour();

            if( !m_content )
            {
                m_content = actor->findChildByName( m_contentName );
            }

            if( !m_content )
            {
                m_content = application->createPanel( nullptr, "", false );

                m_content->setName( m_contentName );
                actor->addChild( m_content );

                auto layoutTransform = m_content->getComponent<LayoutTransform>();

                Vector2<real_Num> buttonPosition;
                Vector2<real_Num> buttonSize;

                if( auto button = getButton() )
                {
                    auto buttonActor = button->getActor();
                    if( buttonActor )
                    {
                        auto buttonLayoutTransform = buttonActor->getComponent<LayoutTransform>();
                        if( buttonLayoutTransform )
                        {
                            buttonPosition = buttonLayoutTransform->getPosition();
                            buttonSize = buttonLayoutTransform->getSize();
                        }
                        else
                        {
                            WP_LOG_ERROR( "Dropdown button is missing a LayoutTransform component." );
                        }
                    }
                }

                auto contentOffset = m_contentOffset;

                auto contentPosition = Vector2<real_Num>( buttonPosition.x, buttonSize.y );
                contentPosition.y += contentOffset;

                auto contentSize = Vector2<real_Num>( buttonSize.x, m_contentHeight );

                layoutTransform->setPosition( contentPosition );
                layoutTransform->setSize( contentSize );

                if( auto imageComponent = m_content->getComponent<Image>() )
                {
                    imageComponent->setEnabled( false );
                    imageComponent->setColour( panelColour );
                }
            }

            auto contentOrder = m_contentZOrder;
            auto buttonOrder = 1;

            auto panelPos = Vector2<real_Num>( 0.0f, 0.0f );
            auto panelSize = m_panelSize;

            if( !m_panel )
            {
                m_panel = application->createPanel( nullptr, "", false );
                m_panel->setName( m_panelName );
                m_content->addChild( m_panel );

                auto layoutTransform = m_panel->getComponent<LayoutTransform>();
                if( layoutTransform )
                {
                    layoutTransform->setPosition( panelPos );
                    layoutTransform->setSize( panelSize );
                    layoutTransform->setAutoCalculateOrder( false );
                    layoutTransform->setZOrder( contentOrder );
                }

                if( auto imageComponent = m_panel->getComponent<Image>() )
                {
                    imageComponent->setColour( panelColour );
                }
            }

            auto verticalLayout = m_panel->getComponent<VerticalLayout>();
            if( !verticalLayout )
            {
                verticalLayout = m_panel->addComponent<VerticalLayout>();
            }

            auto scrollView = m_panel->getComponent<ScrollView>();
            if( !scrollView )
            {
                scrollView = m_panel->addComponent<ScrollView>();
            }

            auto scrollbarActor = m_content->findChildByName( m_scrollbarName );
            if( !scrollbarActor )
            {
                scrollbarActor = UiUtil::createScrollbarVertical( m_scrollbarName, nullptr, "", false );
                m_content->addChild( scrollbarActor );
            }

            auto scrollbarLayout = scrollbarActor->getComponent<LayoutTransform>();
            if( scrollbarLayout )
            {
                scrollbarLayout->setAutoCalculateOrder( false, true );
                scrollbarLayout->setZOrder( contentOrder + m_scrollbarZOrderOffset, true );
            }
            else
            {
                WP_LOG_ERROR( "Dropdown scrollbar is missing a LayoutTransform component." );
            }

            auto scrollBar = scrollbarActor->getComponentPtr<ScrollBar>();
            if( scrollBar )
            {
                scrollBar->setScrollView( scrollView.get() );
                scrollBar->setScrollValue( 0.0f );
                scrollBar->setDirection( Direction::Vertical );
            }

            scrollView->setContentPanel( m_panel->getComponent<LayoutTransform>() );
            scrollView->setScrollBar( scrollBar );
            m_scrollView = scrollView.get();

            auto scrollBarLayout = scrollbarActor->getComponentPtr<LayoutTransform>();
            if( scrollBarLayout )
            {
                auto scrollBarSize = scrollBarLayout->getSize();  // Get the size of the scrollbar
                auto scrollBarPos = Vector2<real_Num>( panelSize.x + ( scrollBarSize.x * 0.5f ), 0.0f );

                auto scrollBarPanelSize = Vector2<real_Num>( scrollBarSize.x, panelSize.y );

                scrollBarLayout->setPosition( scrollBarPos );
                scrollBarLayout->setSize( scrollBarPanelSize );

                auto scrollbarBackground = scrollbarActor->findChildByName( m_scrollbarBackgroundName );
                if( scrollbarBackground )
                {
                    auto backgroundLayout = scrollbarBackground->getComponent<LayoutTransform>();
                    if( backgroundLayout )
                    {
                        backgroundLayout->setSize(
                            Vector2<real_Num>( scrollBarSize.x, scrollBarPanelSize.y ) );
                    }
                }
            }

            verticalLayout->setChildHorizontalAlignment( HorizontalAlignment::CENTER );
            verticalLayout->setChildVerticalAlignment( VerticalAlignment::TOP );

            for( auto option : m_options )
            {
                if( !option )
                {
                    WP_LOG_ERROR( "Skipping null dropdown option while building options panel." );
                    continue;
                }

                auto buttonActor = application->createButton( option->text, nullptr, "", false );
                if( buttonActor )
                {
                    m_panel->addChild( buttonActor );

                    if( auto button = buttonActor->getComponent<Button>() )
                    {
                        button->setNormalColour( m_optionButtonColour );
                    }

                    if( auto button = buttonActor->getComponent<Button>() )
                    {
                        button->setCallbackFunction( [this, option]( hash_type value ) {
                            // Update the selected text
                            if( m_button && option )
                            {
                                m_button->setTextStr( option->text );
                            }

                            // Close the dropdown
                            setOpen( false );
                            updateOptionsPanelVisibility();
                        } );
                    }

                    if( auto buttonLayoutTransform = buttonActor->getComponent<LayoutTransform>() )
                    {
                        buttonLayoutTransform->setVerticalAlignment( VerticalAlignment::CENTER );
                    }

                    auto buttonComponents = buttonActor->getComponentsByType<UIComponent>();
                    for( auto &buttonComponent : buttonComponents )
                    {
                        //buttonComponent->setAutoCalculateOrder( false, true );
                        //buttonComponent->setZOrder( contentOrder + buttonOrder, true );
                    }
                }
            }

            verticalLayout->updateTransform();

            m_content->setFlag( IGameActor::ActorFlagHidden, true, true );
            m_panel->setFlag( IGameActor::ActorFlagDontSave, true, true );

            auto actorState = actor->getState();
            m_content->setState( actorState, true );

            auto args = Array<Parameter>();
            applicationManager->triggerEvent( EventType::Scene, IEvent::sceneChanged, args, this, actor,
                                              nullptr );
        }
    }

    void Dropdown::updateOptionsPanelVisibility()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto sceneManager = applicationManager->getGameManagerPtr();
        auto scene = sceneManager->getCurrentScenePtr();

        auto opened = isOpen();

        if( !m_content || !m_panel )
        {
            if( opened )
            {
                buildOptionsPanel();
            }
        }

        if( m_content )
        {
            m_content->setEnabled( opened );
        }
    }

    void Dropdown::createUI()
    {
        try
        {
            auto element = getElement();
            if( !element )
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto renderUI = applicationManager->getRenderUI();
                if( !renderUI )
                {
                    return;
                }

                auto dropdown = renderUI->addElementByType<ui::IUIDropdown>();
                setElement( dropdown );

                setLabel( "Dropdown" );

                updateVisibility();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Dropdown::updateElementState()
    {
    }

    void Dropdown::setPanelColour( const ColourF &bgPanelColour )
    {
        m_bgPanelColour = bgPanelColour;
    }

    ColourF Dropdown::getPanelColour() const
    {
        return m_bgPanelColour;
    }

    ColourF Dropdown::getOptionButtonColour() const
    {
        return m_optionButtonColour;
    }

    void Dropdown::setOptionButtonColour( const ColourF &colour )
    {
        m_optionButtonColour = colour;
    }

    f32 Dropdown::getContentOffset() const
    {
        return m_contentOffset;
    }

    void Dropdown::setContentOffset( f32 offset )
    {
        m_contentOffset = offset;
    }

    f32 Dropdown::getContentHeight() const
    {
        return m_contentHeight;
    }

    void Dropdown::setContentHeight( f32 height )
    {
        m_contentHeight = MathF::max( height, 1.0f );
    }

    Vector2<real_Num> Dropdown::getPanelSize() const
    {
        return m_panelSize;
    }

    void Dropdown::setPanelSize( const Vector2<real_Num> &size )
    {
        m_panelSize = Vector2<real_Num>( MathF::max( size.x, 1.0f ), MathF::max( size.y, 1.0f ) );
    }

    s32 Dropdown::getContentZOrder() const
    {
        return m_contentZOrder;
    }

    void Dropdown::setContentZOrder( s32 zOrder )
    {
        m_contentZOrder = zOrder;
    }

    String Dropdown::getContentName() const
    {
        return m_contentName;
    }

    void Dropdown::setContentName( const String &name )
    {
        if( name.empty() )
        {
            WP_LOG_ERROR( "Dropdown content name cannot be empty." );
            return;
        }

        m_contentName = name;
    }

    String Dropdown::getPanelName() const
    {
        return m_panelName;
    }

    void Dropdown::setPanelName( const String &name )
    {
        if( name.empty() )
        {
            WP_LOG_ERROR( "Dropdown panel name cannot be empty." );
            return;
        }

        m_panelName = name;
    }

    String Dropdown::getScrollbarName() const
    {
        return m_scrollbarName;
    }

    void Dropdown::setScrollbarName( const String &name )
    {
        if( name.empty() )
        {
            WP_LOG_ERROR( "Dropdown scrollbar name cannot be empty." );
            return;
        }

        m_scrollbarName = name;
    }

    String Dropdown::getScrollbarBackgroundName() const
    {
        return m_scrollbarBackgroundName;
    }

    void Dropdown::setScrollbarBackgroundName( const String &name )
    {
        if( name.empty() )
        {
            WP_LOG_ERROR( "Dropdown scrollbar background name cannot be empty." );
            return;
        }

        m_scrollbarBackgroundName = name;
    }

    s32 Dropdown::getScrollbarZOrderOffset() const
    {
        return m_scrollbarZOrderOffset;
    }

    void Dropdown::setScrollbarZOrderOffset( s32 offset )
    {
        m_scrollbarZOrderOffset = offset;
    }

    u32 Dropdown::getInitialOptionCapacity() const
    {
        return m_initialOptionCapacity;
    }

    void Dropdown::setInitialOptionCapacity( u32 capacity )
    {
        m_initialOptionCapacity = capacity;
        m_options.reserve( m_initialOptionCapacity );
    }

    Dropdown::Option::~Option() = default;

    Dropdown::Option::Option( const String &text ) : text( text )
    {
    }

    Dropdown::Option::Option() = default;

}  // namespace workphone::scene
