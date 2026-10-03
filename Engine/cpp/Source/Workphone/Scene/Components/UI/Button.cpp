#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/Button.hpp>
#include <Workphone/Scene/Components/UI/Image.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/Text.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/ComponentEvent.hpp>
#include <Workphone/Scene/Components/ComponentEventListener.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Button, UIComponent );

    const String Button::imageStr = "image";
    const String Button::textStr = "text";
    const String Button::normalColourStr = "normalColour";
    const String Button::highlightedColourStr = "highlightedColour";
    const String Button::pressedColourStr = "pressedColour";
    const String Button::disabledColourStr = "disabledColour";
    const String Button::textSizeStr = "textSize";

    Button::Button()
    {
        ColourF normalColour = ColourF::White * 0.3f;
        ColourF highlightedColour = ColourF::White * 0.6f;
        ColourF pressedColour = ColourF::White * 0.5f;
        ColourF disabledColour = ColourF::White * 0.2f;

        normalColour.a = 1.0f;
        highlightedColour.a = 1.0f;
        pressedColour.a = 1.0f;
        disabledColour.a = 1.0f;

        m_normalColour = normalColour;
        m_highlightedColour = highlightedColour;
        m_pressedColour = pressedColour;
        m_disabledColour = disabledColour;

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto clickedEvent = factoryManager->make_ptr<ComponentEvent>();
        clickedEvent->setEventHash( IEvent::ACTIVATE_HASH );
        addEvent( clickedEvent );

        auto onClickEventListener = factoryManager->make_ptr<ComponentEventListener>();
        onClickEventListener->setEvent( clickedEvent );
        onClickEventListener->setComponent( this );
        clickedEvent->addListener( onClickEventListener );

        auto hoverEvent = factoryManager->make_ptr<ComponentEvent>();
        hoverEvent->setEventHash( IEvent::TOGGLE_HIGHLIGHT_HASH );
        addEvent( hoverEvent );

        auto onHoverEventListener = factoryManager->make_ptr<ComponentEventListener>();
        onHoverEventListener->setEvent( hoverEvent );
        onHoverEventListener->setComponent( this );
        hoverEvent->addListener( onHoverEventListener );
    }

    Button::~Button()
    {
    }

    void Button::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

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

    void Button::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                m_image = nullptr;
                m_text = nullptr;

                UIComponent::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Parameter Button::handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventType == EventType::UI )
        {
            if( eventValue == IEvent::CLICK_HASH )
            {
                ScopedLock lock( this );

                if( auto image = getImage() )
                {
                    image->setColour( m_pressedColour );
                }

                if( m_callbackFunction )
                {
                    m_callbackFunction( eventValue );
                }
            }

            auto events = getEvents();
            for( auto &event : events )
            {
                if( event )
                {
                    auto listeners = event->getListeners();
                    for( auto &listener : listeners )
                    {
                        if( listener )
                        {
                            if( auto eventActor = listener->getActor() )
                            {
                                auto components = eventActor->getComponents();
                                for( auto component : components )
                                {
                                    if( component != this )
                                    {
                                        component->handleEvent( eventType, eventValue, arguments, sender,
                                                                this, event );
                                    }
                                }
                            }

                            listener->handleEvent( eventType, eventValue, arguments, sender, object,
                                                   event );
                        }
                    }
                }
            }
        }

        return UIComponent::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

    bool Button::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( auto actor = getActor() )
        {
            auto enabled = isEnabled() && actor->isEnabledInScene();
            if( enabled )
            {
                auto layoutTransform = actor->getComponent<LayoutTransform>();
                if( !layoutTransform )
                {
                    return false;
                }

                auto absolutePosition = layoutTransform->getAbsolutePosition();
                auto absoluteSize = layoutTransform->getAbsoluteSize();
                auto absoluteMin = layoutTransform->getAbsoluteMin();
                auto absoluteMax = layoutTransform->getAbsoluteMax();

                auto layoutComponent = getCanvas();
                if( !layoutComponent )
                {
                    return false;
                }

                auto mainWindow = applicationManager->getWindow();

                auto windowSize = mainWindow->getSize();
                auto windowSizeF = Vector2<real_Num>( static_cast<f32>( windowSize.x ),
                                                      static_cast<f32>( windowSize.y ) );
                auto editorWindowBorderSize = Vector2<real_Num>( 0.0f, 20.0f / windowSizeF.y );
                //editorWindowBorderSize = Vector2F( 0.0f, 0.0f );

                auto layout = workphone::dynamic_pointer_cast<Layout>( layoutComponent );
                auto referenceSize = layout->getReferenceSize();
                auto referenceSizeF = Vector2<real_Num>( static_cast<f32>( referenceSize.x ),
                                                         static_cast<f32>( referenceSize.y ) );

                auto eventType = event->getEventType();
                switch( eventType )
                {
                case IInputEvent::EventType::Mouse:
                {
                    auto mouseState = event->getMouseState();

                    auto ui = applicationManager->getUI();

                    auto absoluteMousePosition = mouseState->getAbsolutePosition();
                    auto relativeMousePosition = mouseState->getRelativePosition();

                    auto mainWindowSize = Vector2I();
                    auto mainWindowSizeF = Vector2<real_Num>();
                    auto viewportPosition = Vector2<real_Num>();
                    auto viewportSize = Vector2<real_Num>();

                    if( ui && ui->getMainWindow() )
                    {
                        auto uiWindow = ui->getMainWindow();
                        if( mainWindow )
                        {
                            mainWindowSize = mainWindow->getSize();
                            mainWindowSizeF = Vector2<real_Num>( static_cast<f32>( mainWindowSize.x ),
                                                                 static_cast<f32>( mainWindowSize.y ) );
                            viewportPosition = uiWindow->getPosition();
                            viewportSize = uiWindow->getSize();
                        }
                    }
                    else if( mainWindow )
                    {
                        mainWindowSize = mainWindow->getSize();
                        mainWindowSizeF = Vector2<real_Num>( static_cast<f32>( mainWindowSize.x ),
                                                             static_cast<f32>( mainWindowSize.y ) );
                        viewportPosition = Vector2<real_Num>( 0.0f, 0.0f );
                        viewportSize = mainWindowSizeF;
                    }

                    auto pos = viewportPosition / mainWindowSizeF;
                    auto size = viewportSize / mainWindowSizeF;

                    auto aabb = AABB2<real_Num>( pos, size, true );
                    if( aabb.isInside( relativeMousePosition ) )
                    {
                        auto elementPos = absolutePosition / referenceSizeF;
                        auto elementSize = absoluteSize / referenceSizeF;

                        auto rect = AABB2<real_Num>( elementPos, elementPos + elementSize );

                        auto point = relativeMousePosition - pos;
                        point = point - editorWindowBorderSize;
                        point *= mainWindowSizeF;

                        point /= viewportSize;

                        if( rect.isInside( point ) )
                        {
                            auto mouseEventType = mouseState->getEventType();
                            if( mouseEventType == IMouseState::Event::LeftPressed )
                            {
                                if( auto image = getImage() )
                                {
                                    image->setColour( m_pressedColour );
                                }

                                auto rootActor = actor->getSceneRoot();
                                handleEvent( EventType::UI, IEvent::CLICK_HASH, Array<Parameter>(),
                                             rootActor, this, nullptr );

                                return true;
                            }
                            if( mouseEventType == IMouseState::Event::LeftReleased )
                            {
                                if( auto image = getImage() )
                                {
                                    image->setColour( m_highlightedColour );
                                }

                                return true;
                            }
                            if( mouseEventType == IMouseState::Event::MiddleReleased )
                            {
                            }
                            else if( mouseEventType == IMouseState::Event::Moved )
                            {
                                auto image = getImage();
                                if( image )
                                {
                                    image->setColour( m_highlightedColour );
                                }
                            }
                        }
                        else
                        {
                            auto mouseEventType = mouseState->getEventType();
                            if( mouseEventType == IMouseState::Event::Moved )
                            {
                                auto image = getImage();
                                if( image )
                                {
                                    image->setColour( m_normalColour );
                                }
                            }
                        }
                    }
                }
                break;
                case IInputEvent::EventType::User:
                case IInputEvent::EventType::Key:
                {
                }
                break;
                default:
                {
                }
                }
            }
        }

        return UIComponent::handleEvent( event );
    }

    void Button::createUI()
    {
        try
        {
            if( auto actor = getActorPtr() )
            {
                auto enabled = isEnabled() && actor->isEnabledInScene();
                if( enabled )
                {
                    if( !getElement() )
                    {
                        auto applicationManager = core::IApplicationManager::instancePtr();
                        WP_ASSERT( applicationManager );

                        auto renderUI = applicationManager->getRenderUIPtr();
                        if( !renderUI )
                        {
                            return;
                        }

                        if( auto parentActor = actor->getSceneRoot() )
                        {
                            auto button = renderUI->addElementByType<ui::IUIButton>();
                            setElement( button );

                            setLabel( "Button" );

                            updateElementState();
                            updateVisibility();
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Array<SmartPtr<ISharedObject>> Button::getChildObjects() const
    {
        auto objects = UIComponent::getChildObjects();

        return objects;
    }

    SmartPtr<Properties> Button::getProperties() const
    {
        if( auto properties = UIComponent::getProperties() )
        {
            properties->setPropertyAsType( imageStr, m_image.load() );
            properties->setPropertyAsType( textStr, m_text.load() );

            properties->setProperty( normalColourStr, m_normalColour );
            properties->setProperty( highlightedColourStr, m_highlightedColour );
            properties->setProperty( pressedColourStr, m_pressedColour );
            properties->setProperty( disabledColourStr, m_disabledColour );

            properties->setProperty( textSizeStr, m_textSize );

            return properties;
        }

        return nullptr;
    }

    void Button::setProperties( SmartPtr<Properties> properties )
    {
        ColourF normalColour = ColourF::White;
        ColourF highlightedColour = ColourF::White;
        ColourF pressedColour = ColourF::White;
        ColourF disabledColour = ColourF::White;
        SmartPtr<Image> image;
        SmartPtr<Text> text;
        u32 textSize = 12;

        properties->getPropertyAsType( imageStr, image );
        properties->getPropertyAsType( textStr, text );

        properties->getPropertyValue( normalColourStr, normalColour );
        properties->getPropertyValue( highlightedColourStr, highlightedColour );
        properties->getPropertyValue( pressedColourStr, pressedColour );
        properties->getPropertyValue( disabledColourStr, disabledColour );

        properties->getPropertyValue( textSizeStr, textSize );

        UIComponent::setProperties( properties );

        m_normalColour = normalColour;
        m_highlightedColour = highlightedColour;
        m_pressedColour = pressedColour;
        m_disabledColour = disabledColour;
        m_image = image;
        m_text = text;
        m_textSize = textSize;

        updateElementState();
    }

    void Button::updateElementState()
    {
        auto element = getElement();
        if( element && element->isDerived<ui::IUIButton>() )
        {
            auto button = workphone::static_pointer_cast<ui::IUIButton>( element );
            button->setTextSize( static_cast<f32>( m_textSize ) );

            auto label = getLabel();
            button->setLabel( label );

            auto cascadeInput = getCascadeInput();
            button->setHandleInputEvents( cascadeInput );

            if( auto image = getImage() )
            {
                image->setColour( m_normalColour );
            }
        }
    }

    void Button::setText( SmartPtr<Text> text )
    {
        m_text = text;
    }

    SmartPtr<Text> Button::getText() const
    {
        return m_text;
    }

    String Button::getTextStr() const
    {
        if( m_text )
        {
            return m_text->getText();
        }

        return {};
    }

    void Button::setTextStr( const String &textStr )
    {
        if( m_text )
        {
            m_text->setText( textStr );
        }
    }

    void Button::setImage( SmartPtr<Image> image )
    {
        m_image = image;
    }

    SmartPtr<Image> Button::getImage() const
    {
        return m_image;
    }

    void Button::setCallbackFunction( std::function<void( hash_type )> callbackFunction )
    {
        ScopedLock lock( this );
        WP_ASSERT( m_callbackFunction == nullptr );
        m_callbackFunction = callbackFunction;
    }

    void Button::setDisabledColour( const ColourF &disabledColour )
    {
        m_disabledColour = disabledColour;
    }

    ColourF Button::getDisabledColour() const
    {
        return m_disabledColour;
    }

    void Button::setPressedColour( const ColourF &pressedColour )
    {
        m_pressedColour = pressedColour;
    }

    ColourF Button::getPressedColour() const
    {
        return m_pressedColour;
    }

    void Button::setHighlightedColour( const ColourF &highlightedColour )
    {
        m_highlightedColour = highlightedColour;
    }

    ColourF Button::getHighlightedColour() const
    {
        return m_highlightedColour;
    }

    void Button::setNormalColour( const ColourF &normalColour )
    {
        m_normalColour = normalColour;
    }

    ColourF Button::getNormalColour() const
    {
        return m_normalColour;
    }

    void Button::setTextSize( u32 textSize )
    {
        m_textSize = textSize;
    }

    u32 Button::getTextSize() const
    {
        return m_textSize;
    }

}  // namespace workphone::scene
