#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/UIComponent.hpp>
#include <Workphone/Jobs/EventJob.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/IRenderer2.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/Scene/IComponentSystem.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/UI/IUILayoutWindow.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, UIComponent, Component );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, UIComponent::UIElementListener, IEventListener );

    const String UIComponent::colourStr = "colour";
    const String UIComponent::orderStr = "order";
    const String UIComponent::cascadeInputStr = "cascadeInput";
    const String UIComponent::handleInputEventsStr = "handleInputEvents";
    const String UIComponent::resetStr = "Reset";
    const String UIComponent::updateTransformStr = String( "updateTransform" );
    const String UIComponent::updateOrderStr = String( "updateOrder" );
    const String UIComponent::updateVisibilityStr = String( "updateVisibility" );
    const String UIComponent::showLabelStr = String( "showLabel" );
    const String UIComponent::labelStr = String( "label" );
    const String UIComponent::labelActorStr = String( "labelActor" );
    const String UIComponent::autoCalculateOrderStr = String( "autoCalculateOrder" );
    const String UIComponent::layoutTransformStr = String( "layoutTransform" );
    const String UIComponent::editorWindowBorderSizeStr = String( "editorWindowBorderSize" );
    const String UIComponent::setupCanvasStr = String( "setupCanvas" );
    const String UIComponent::createUIStr = String( "createUI" );
    const String UIComponent::updateElementStateStr = String( "updateElementState" );

    const u8 UIComponent::cascadeInputFlag = 1 << 1;
    const u8 UIComponent::handleInputEventsFlag = 1 << 2;
    const u8 UIComponent::showLabelFlag = 1 << 3;
    const u8 UIComponent::autoCalculateOrderFlag = 1 << 4;

    UIComponent::UIComponent()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto elementListener = factoryManager->make_ptr<UIElementListener>();
        elementListener->setOwner( this );
        setElementListener( elementListener );

        m_uiComponentFlags = cascadeInputFlag | autoCalculateOrderFlag;
    }

    UIComponent::~UIComponent()
    {
    }

    void UIComponent::load( SmartPtr<ISharedObject> data )
    {
        Component::load( data );
    }

    void UIComponent::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto renderUI = applicationManager->getRenderUI();
            if( renderUI )
            {
                if( auto element = getElement() )
                {
                    renderUI->removeElement( element );
                    setElement( nullptr );
                }
            }

            setCanvas( nullptr );

            Component::unload( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IEventListener> UIComponent::getElementListener() const
    {
        return m_elementListener;
    }

    void UIComponent::setElementListener( SmartPtr<IEventListener> elementListener )
    {
        m_elementListener = elementListener;
    }

    SmartPtr<ui::IUIElement> UIComponent::getElement() const
    {
        return m_element;
    }

    void UIComponent::setElement( SmartPtr<ui::IUIElement> element )
    {
#if !WP_FINAL
        if( ( m_element && element ) == true )
        {
            WP_LOG( "Element already set." );
        }
#endif

        if( m_element )
        {
            m_element->removeObjectListener( m_elementListener );
        }

        m_element = element;

        if( m_element )
        {
            m_element->addObjectListener( m_elementListener );
            m_element->setOwner( this );
        }
    }

    SmartPtr<UIComponent> UIComponent::getCanvas() const
    {
        return m_canvas;
    }

    void UIComponent::setCanvas( SmartPtr<UIComponent> canvas )
    {
        m_canvas = canvas;
    }

    void UIComponent::updateDimensions()
    {
    }

    void UIComponent::updateMaterials()
    {
    }

    Array<SmartPtr<ISharedObject>> UIComponent::getChildObjects() const
    {
        auto objects = Component::getChildObjects();

        objects.emplace_back( m_element.load() );
        objects.emplace_back( m_canvas.load() );
        objects.emplace_back( m_layoutTransform.load() );
        return objects;
    }

    SmartPtr<Properties> UIComponent::getProperties() const
    {
        ScopedLock lock( this, false );

        if( auto properties = Component::getProperties() )
        {
            auto cascadeInput = getCascadeInput();
            auto handleInputEvents = getHandleInputEvents();
            auto showLabel = getShowLabel();
            auto autoCalculateOrder = getAutoCalculateOrder();
            auto editorWindowBorderSize = getEditorWindowBorderSize();

            auto label = getLabel();

            properties->setProperty( colourStr, m_colour );
            properties->setProperty( cascadeInputStr, cascadeInput );
            properties->setProperty( handleInputEventsStr, handleInputEvents );
            properties->setProperty( showLabelStr, showLabel );
            properties->setProperty( autoCalculateOrderStr, autoCalculateOrder );
            properties->setProperty( labelStr, label );
            properties->setProperty( labelActorStr, m_labelActor.load() );
            properties->setProperty( editorWindowBorderSizeStr, editorWindowBorderSize );

            properties->setButtonPressed( resetStr );
            properties->setButtonPressed( updateTransformStr );

            properties->setButtonPressed( setupCanvasStr );
            properties->setButtonPressed( createUIStr );
            properties->setButtonPressed( updateOrderStr );
            properties->setButtonPressed( updateTransformStr );
            properties->setButtonPressed( updateVisibilityStr );
            properties->setButtonPressed( updateElementStateStr );

            return properties;
        }

        return nullptr;
    }

    void UIComponent::setProperties( SmartPtr<Properties> properties )
    {
        ScopedLock lock( this, true );

        Component::setProperties( properties );

        auto actor = getActorPtr();
        if( !actor )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto pSceneManager = applicationManager->getGameManagerPtr();
        auto sceneManager = (GameManager *)pSceneManager;
        WP_ASSERT( sceneManager );

        ColourF colour;
        String label;
        SmartPtr<IGameActor> labelActor;

        properties->getPropertyValue( colourStr, colour );

        auto cascadeInput = getCascadeInput();
        auto handleInputEvents = getHandleInputEvents();
        auto showLabel = getShowLabel();
        auto autoCalculateOrder = getAutoCalculateOrder();
        auto editorWindowBorderSize = getEditorWindowBorderSize();

        properties->getPropertyValue( cascadeInputStr, cascadeInput );
        properties->getPropertyValue( handleInputEventsStr, handleInputEvents );
        properties->getPropertyValue( showLabelStr, showLabel );
        properties->getPropertyValue( autoCalculateOrderStr, autoCalculateOrder );
        properties->getPropertyValue( labelStr, label );
        properties->getPropertyValue( labelActorStr, labelActor );
        properties->getPropertyValue( editorWindowBorderSizeStr, editorWindowBorderSize );

        setColour( colour );
        setLabel( label );
        setLabelActor( labelActor );

        setCascadeInput( cascadeInput );
        setHandleInputEvents( handleInputEvents );
        setShowLabel( showLabel );
        setAutoCalculateOrder( autoCalculateOrder );
        setEditorWindowBorderSize( editorWindowBorderSize );

        if( properties->isButtonPressed( updateTransformStr ) )
        {
            auto pActor = SmartPtr<IGameActor>( actor );
            sceneManager->addDirtyActor( pActor );
        }

        auto element = getElementPtr();

        //if( m_zOrder != zorder )
        //{
        //    m_zOrder = zorder;

        //    if( element )
        //    {
        //        element->setOrder( m_zOrder );
        //    }
        //}

        if( element )
        {
            element->setHandleInputEvents( cascadeInput );
        }

        if( properties->isButtonPressed( setupCanvasStr ) )
        {
            setupCanvas();
        }

        if( properties->isButtonPressed( createUIStr ) )
        {
            createUI();
        }

        if( properties->isButtonPressed( updateOrderStr ) )
        {
            updateOrder();

            auto childUiComponents = actor->getAllComponentsInChildren<UIComponent>();
            for( auto &childUiComponent : childUiComponents )
            {
                childUiComponent->updateOrder();
            }
        }

        if( properties->isButtonPressed( updateTransformStr ) )
        {
            updateTransform();
        }

        if( properties->isButtonPressed( updateVisibilityStr ) )
        {
            updateVisibility();
        }

        if( properties->isButtonPressed( updateElementStateStr ) )
        {
            updateElementState();
        }

        updateTransform();
        updateElementState();

        updateVisibility();
        updateColour();

        if( auto componentSystem = getComponentSystemPtr() )
        {
            componentSystem->makeDirty();
        }
    }

    void UIComponent::updateFlags( u32 flags, u32 oldFlags )
    {
        if( auto actor = getActor() )
        {
            auto eState = getState();
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                    BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
                {
                    auto enabled = isEnabled() && actor->isEnabledInScene();
                    if( enabled )
                    {
                        if( auto element = getElementPtr() )
                        {
                            element->invalidate();
                        }

                        auto components = actor->getAllComponentsInChildren<UIComponent>();
                        for( auto &component : components )
                        {
                            if( auto element = component->getElementPtr() )
                            {
                                element->invalidate();
                            }
                        }

                        createUI();
                        setupCanvas();
                        updateElementState();
                        updateOrder();

                        if( auto root = actor->getSceneRoot() )
                        {
                            root->updateTransform();
                        }

                        if( auto componentSystem = getComponentSystemPtr() )
                        {
                            componentSystem->makeDirty();
                        }
                    }

                    updateVisibility();
                }
            }
            break;
            }
        }
    }

    void UIComponent::updateTransform()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto applicationTask = applicationManager->getApplicationTask();
        auto task = Thread::getCurrentTask();

        if( applicationTask == task )
        {
            if( auto actor = getActorPtr() )
            {
                auto eState = getState();
                switch( eState )
                {
                case State::Edit:
                case State::Play:
                {
                    if( auto layoutTransform = getLayoutTransform() )
                    {
                        layoutTransform->updateTransform();
                    }
                }
                break;
                default:
                {
                }
                break;
                };
            }
        }
    }

    bool UIComponent::getCascadeInput() const
    {
        ScopedLock lock( &m_uiComponentFlags, false );
        return BitUtil::getFlagValue( m_uiComponentFlags.load(), UIComponent::cascadeInputFlag );
    }

    void UIComponent::setCascadeInput( bool cascadeInput )
    {
        ScopedLock lock( &m_uiComponentFlags, true );
        m_uiComponentFlags = BitUtil::setFlagValue( m_uiComponentFlags.load(),
                                                    UIComponent::cascadeInputFlag, cascadeInput );
    }

    bool UIComponent::getAutoCalculateOrder() const
    {
        if( auto layoutTransform = m_layoutTransform.load() )
        {
            return layoutTransform->getAutoCalculateOrder();
        }

        ScopedLock lock( &m_uiComponentFlags, false );
        return BitUtil::getFlagValue( m_uiComponentFlags.load(), UIComponent::autoCalculateOrderFlag );
    }

    void UIComponent::setAutoCalculateOrder( bool autoCalculateOrder )
    {
        {
            ScopedLock lock( &m_uiComponentFlags, true );
            m_uiComponentFlags = BitUtil::setFlagValue(
                m_uiComponentFlags.load(), UIComponent::autoCalculateOrderFlag, autoCalculateOrder );
        }

        auto layoutTransform = m_layoutTransform.load();
        if( !layoutTransform )
        {
            if( auto actor = getActorPtr() )
            {
                layoutTransform = actor->getComponentPtr<LayoutTransform>();
                m_layoutTransform = layoutTransform;
            }
        }

        if( layoutTransform )
        {
            layoutTransform->setAutoCalculateOrder( autoCalculateOrder, false );
        }
    }

    Parameter UIComponent::handleEvent( EventType eventType, hash_type eventValue,
                                        const Array<Parameter> &arguments,
                                        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                        SmartPtr<IEvent> event )
    {
        ScopedLock lock( this, true );

        if( eventValue == IEvent::loadingStateChanged )
        {
            if( object )
            {
                if( object->isDerived<ui::IUIManager>() )
                {
                    setupCanvas();

                    if( !getElement() )
                    {
                        createUI();
                    }

                    updateOrder();
                    updateTransform();
                    updateVisibility();
                    updateElementState();
                }
            }
        }
        else if( eventValue == hierarchyChanged || eventValue == parentChanged ||
                 eventValue == childAddedInHierarchy || eventValue == childRemovedInHierarchy )
        {
            setupCanvas();

            if( !getElement() )
            {
                createUI();
            }

            updateOrder();
            updateTransform();
            updateVisibility();
            updateElementState();

            auto actor = getActor();
            if( sender != actor )
            {
                if( sender && object && sender->isDerived<IGameActor>() &&
                    object->isDerived<IGameActor>() )
                {
                    auto parent = workphone::static_pointer_cast<IGameActor>( sender );
                    auto child = workphone::static_pointer_cast<IGameActor>( object );

                    auto parentUiCompnent = parent ? parent->getComponent<UIComponent>() : nullptr;
                    auto childUiCompnent = child ? child->getComponent<UIComponent>() : nullptr;

                    if( parentUiCompnent )
                    {
                        auto parentElement = parentUiCompnent->getElement();
                        auto childElement = childUiCompnent ? childUiCompnent->getElement() : nullptr;

                        if( childElement )
                        {
                            auto childParent = childElement->getParent();
                            if( parentElement != childParent )
                            {
                                if( parentElement )
                                {
                                    parentElement->addChild( childElement );
                                }
                            }
                        }
                    }
                }
            }
        }
        else if( eventValue == visibilityChanged )
        {
            if( auto actor = getActorPtr() )
            {
                auto visible = actor->isVisible();
                if( visible )
                {
                    auto element = getElementPtr();
                    if( !element )
                    {
                        createUI();
                    }
                }

                if( auto element = getElement() )
                {
                    element->setVisible( visible );
                }
            }
        }
        else if( eventValue == hierarchyChanged )
        {
            if( auto element = getElementPtr() )
            {
                if( auto stateContext = element->getStateContext() )
                {
                    stateContext->setDirty( true );
                }
            }

            if( auto actor = getActorPtr() )
            {
                auto visible = isEnabled() && actor->isEnabledInScene();
                if( visible )
                {
                    auto element = getElementPtr();
                    if( !element )
                    {
                        createUI();
                    }
                }

                actor->updateTransform();
            }
        }
        else if( eventValue == parentChanged )
        {
            if( auto actor = getActorPtr() )
            {
                auto visible = isEnabled() && actor->isEnabledInScene();
                if( visible )
                {
                    auto element = getElementPtr();
                    if( !element )
                    {
                        createUI();
                    }
                }

                actor->updateTransform();
            }
        }
        else if( eventValue == childAddedInHierarchy )
        {
            if( auto element = getElementPtr() )
            {
                if( auto stateContext = element->getStateContext() )
                {
                    stateContext->setDirty( true );
                }
            }

            if( auto actor = getActorPtr() )
            {
                auto visible = isEnabled() && actor->isEnabledInScene();
                if( visible )
                {
                    auto element = getElementPtr();
                    if( !element )
                    {
                        createUI();
                    }
                }

                actor->updateTransform();
            }
        }
        else if( eventValue == childRemovedInHierarchy )
        {
            if( auto element = getElementPtr() )
            {
                if( auto stateContext = element->getStateContext() )
                {
                    stateContext->setDirty( true );
                }
            }

            if( auto actor = getActorPtr() )
            {
                auto visible = isEnabled() && actor->isEnabledInScene();
                if( visible )
                {
                    auto element = getElement();
                    if( !element )
                    {
                        createUI();
                    }
                }

                actor->updateTransform();
            }
        }
        else if( eventValue == actorReset )
        {
            if( auto actor = getActorPtr() )
            {
                auto visible = isEnabled() && actor->isEnabledInScene();
                if( visible )
                {
                    auto element = getElementPtr();
                    if( !element )
                    {
                        createUI();
                    }
                }
            }
        }
        else if( eventValue == scene::IGameManager::sceneLoadedHash )
        {
            setupCanvas();

            if( !getElement() )
            {
                createUI();
            }

            updateOrder();
            updateTransform();
            updateVisibility();
            updateElementState();

            if( auto element = getElement() )
            {
                element->invalidate();
            }
        }

        return Component::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

    bool UIComponent::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        ScopedLock lock( this, true );

        if( !BitUtil::getFlagValue( m_uiComponentFlags.load(), UIComponent::handleInputEventsFlag ) )
        {
            return false;
        }

        if( auto actor = getActorPtr() )
        {
            auto enabled = isEnabled() && actor->isEnabledInScene();
            if( enabled )
            {
                auto layoutTransform = actor->getComponentPtr<LayoutTransform>();
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

                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto mainWindow = applicationManager->getWindow();

                auto windowSize = mainWindow->getSize();
                auto windowSizeF = Vector2<real_Num>( static_cast<f32>( windowSize.x ),
                                                      static_cast<f32>( windowSize.y ) );
                auto editorWindowBorderSize = getEditorWindowBorderSize() / windowSizeF;
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
                        viewportPosition = Vector2<real_Num>( 0.0, 0.0 );
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
                                auto rootActor = actor->getSceneRoot();
                                handleEvent( EventType::UI, IEvent::CLICK_HASH, Array<Parameter>(),
                                             rootActor, this, nullptr );

                                return true;
                            }
                            if( mouseEventType == IMouseState::Event::LeftReleased )
                            {
                                return true;
                            }
                            if( mouseEventType == IMouseState::Event::MiddleReleased )
                            {
                            }
                            else if( mouseEventType == IMouseState::Event::Moved )
                            {
                            }
                        }
                        else
                        {
                            auto mouseEventType = mouseState->getEventType();
                            if( mouseEventType == IMouseState::Event::Moved )
                            {
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

        return false;
    }

    FSMReturnType UIComponent::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        ScopedLock lock( this, true );

        Component::handleComponentEvent( state, eventType );

        switch( eventType )
        {
        case FSMEvent::Change:
        {
        }
        break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Destroyed:
            {
            }
            break;
            case State::Edit:
            case State::Play:
            {
                if( auto actor = getActorPtr() )
                {
                    m_layoutTransform = actor->getComponentPtr<LayoutTransform>();

                    setupCanvas();

                    if( !getElement() )
                    {
                        createUI();
                    }

                    updateOrder();
                    updateTransform();
                    updateVisibility();
                    updateElementState();
                }
            }
            break;
            default:
            {
            }
            };
        }
        break;
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                if( auto element = getElement() )
                {
                    if( auto canvas = workphone::static_pointer_cast<Layout>( getCanvas() ) )
                    {
                        if( auto layout = canvas->getLayout() )
                        {
                            layout->removeChild( element );
                        }
                    }
                }
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Pending:
        {
        }
        break;
        case FSMEvent::Complete:
        {
        }
        break;
        case FSMEvent::NewState:
        {
        }
        break;
        case FSMEvent::WaitForChange:
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

    void UIComponent::updateOrder()
    {
        ScopedLock lock( this );

        if( auto actor = getActorPtr() )
        {
            if( auto layoutTransform = getLayoutTransform() )
            {
                auto autoCalculateOrder = layoutTransform->getAutoCalculateOrder();
                if( autoCalculateOrder )
                {
                    auto pCanvas = getCanvas();
                    if( !pCanvas )
                    {
                        setupCanvas();
                        pCanvas = getCanvas();
                    }

                    if( pCanvas )
                    {
                        auto canvas = workphone::static_pointer_cast<Layout>( pCanvas );

                        if( auto layout = canvas->getLayout() )
                        {
                            if( auto element = getElement() )
                            {
                                // A canvas is its own UI element, not its own child.
                                if( element.get() != layout.get() )
                                {
                                    element->setLayout( layout );
                                    if( element->getParent() != layout )
                                    {
                                        layout->addChild( element );
                                    }
                                }

                                auto order = canvas->getZOrder( actor );
                                element->setOrder( order );

                                layoutTransform->setZOrder( order );
                            }
                        }
                    }
                }
                else
                {
                    if( auto element = getElement() )
                    {
                        auto zOrder = layoutTransform->getZOrder();
                        element->setOrder( zOrder );
                    }
                }
            }

            auto children = actor->getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    child->updateOrder();
                }
            }
        }
    }

    //void UIComponent::setZOrder( u32 zOrder, bool cascade )
    //{
    //    m_zOrder = zOrder;

    //    if( auto element = getElement() )
    //    {
    //        element->setOrder( m_zOrder );
    //    }

    //    if( cascade )
    //    {
    //        if( auto actor = getActor() )
    //        {
    //            auto childUiComponents = actor->getComponentsInChildren<UIComponent>();
    //            for( auto childUiComponent : childUiComponents )
    //            {
    //                childUiComponent->setZOrder( zOrder + 1, cascade );
    //            }
    //        }
    //    }
    //}

    Array<SmartPtr<IGameActor>> UIComponent::getActorListeners() const
    {
        ScopedLock lock( this, false );
        return m_actorListeners.snapshot();
    }

    void UIComponent::removeListener( SmartPtr<IComponent> component )
    {
        ScopedLock lock( this, true );
        m_componentListeners.erase(
            std::remove( m_componentListeners.begin(), m_componentListeners.end(), component ),
            m_componentListeners.end() );
    }

    void UIComponent::removeActorListener( SmartPtr<IGameActor> actor )
    {
        ScopedLock lock( this, true );
        m_actorListeners.erase( std::remove( m_actorListeners.begin(), m_actorListeners.end(), actor ),
                                m_actorListeners.end() );
    }

    Array<SmartPtr<IComponent>> UIComponent::getComponentListeners() const
    {
        ScopedLock lock( this, false );
        return m_componentListeners.snapshot();
    }

    void UIComponent::addListener( SmartPtr<IComponent> component )
    {
        ScopedLock lock( this );
        m_componentListeners.push_back( component );
    }

    void UIComponent::addActorListener( SmartPtr<IGameActor> actor )
    {
        ScopedLock lock( this );
        m_actorListeners.push_back( actor );
    }

    void UIComponent::setupCanvas()
    {
        ScopedLock lock( this );

        if( auto actor = getActor() )
        {
            for( auto ancestor = actor; ancestor; ancestor = ancestor->getParent() )
            {
                if( auto canvas = ancestor->getComponent<Layout>() )
                {
                    setCanvas( canvas );
                    return;
                }
            }
            setCanvas( nullptr );
        }
    }

    void UIComponent::createUI()
    {
    }

    void UIComponent::updateVisibility()
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();

        if( auto actor = getActorPtr() )
        {
            auto enabled = isEnabled() && actor->isEnabledInScene();
            auto visible = applicationManager->isPlaying() ? enabled : enabled && actor->isVisible();

            if( auto element = getElement() )
            {
                element->setEnabled( enabled, false );
                element->setVisible( visible, false );
            }
        }

        //if( auto canvas = getCanvas() )
        //{
        //    canvas->updateVisibility();
        //}
    }

    void UIComponent::updateElementState()
    {
    }

    void UIComponent::updateColour()
    {
        if( auto element = getElement() )
        {
            element->setColour( m_colour );
        }
    }

    AABB2<real_Num> UIComponent::getDebugBounds() const
    {
        if( auto layoutTransform = getLayoutTransform() )
        {
            return AABB2<real_Num>( layoutTransform->getAbsoluteMin(),
                                    layoutTransform->getAbsoluteMax() );
        }

        return {};
    }

    void UIComponent::drawDebugBounds( render::IRenderer2 &renderer, const ColourF &colour ) const
    {
        const auto bounds = getDebugBounds();
        if( bounds.isValid() )
        {
            renderer.drawRect( bounds, colour );
        }
    }

    bool UIComponent::isDebugDrawEnabled() const
    {
        return m_debugDrawEnabled.load();
    }

    void UIComponent::setDebugDrawEnabled( bool enabled )
    {
        m_debugDrawEnabled = enabled;
    }

    void UIComponent::postUpdate()
    {
        if( !isDebugDrawEnabled() )
        {
            return;
        }

        if( auto applicationManager = core::IApplicationManager::instancePtr() )
        {
            if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
            {
                auto renderer = graphicsSystem->getRenderer();
                if( auto renderer2 = workphone::dynamic_pointer_cast<render::IRenderer2>( renderer ) )
                {
                    drawDebugBounds( *renderer2 );
                }
            }
        }
    }

    void UIComponent::setColour( const ColourF &colour )
    {
        if( m_colour != colour )
        {
            m_colour = colour;
            updateColour();
        }
    }

    AABB2<real_Num> UIComponent::getBounds( SmartPtr<IGameActor> actor ) const
    {
        if( actor )
        {
            if( auto transform = actor->getComponent<LayoutTransform>() )
            {
                auto position = transform->getPosition();
                auto size = transform->getSize();
                return AABB2<real_Num>( position, size, true );
            }
        }

        return {};
    }

    AABB2<real_Num> UIComponent::getBounds() const
    {
        auto actor = getActor();
        return getBounds( actor );
    }

    bool UIComponent::getLocalMousePosition( const SmartPtr<IInputEvent> &event,
                                             Vector2<real_Num> &localPos ) const
    {
        auto actor = getActor();
        return getLocalMousePosition( event, actor, localPos );
    }

    bool UIComponent::getLocalMousePosition( const SmartPtr<IInputEvent> &event,
                                             SmartPtr<IGameActor> actor,
                                             Vector2<real_Num> &localPos ) const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto ui = applicationManager->getUIPtr();

        if( actor )
        {
            auto enabled = isEnabled() && actor->isEnabledInScene();
            if( enabled )
            {
                auto layoutTransform = actor->getComponentPtr<LayoutTransform>();
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
                auto editorWindowBorderSize = getEditorWindowBorderSize() / windowSizeF;
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
                    //Mouse event
                    auto mouseState = event->getMouseState();
                    auto mouseEventType = mouseState->getEventType();

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
                        auto elementRect =
                            AABB2<real_Num>( absolutePosition, absolutePosition + absoluteSize );

                        auto point = relativeMousePosition - pos;
                        point = point / size;
                        point *= referenceSizeF;

                        //WP_LOG( "Point: " + StringUtil::toString( point.x ) + ", " +
                        //        StringUtil::toString( point.y ) );

                        if( elementRect.isInside( point ) )
                        {
                            localPos = point - absolutePosition;
                            //WP_LOG( "LocalPos: " + StringUtil::toString( localPos.x ) + ", " +
                            //        StringUtil::toString( localPos.y ) );
                            return true;
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

        return false;
    }

    void UIComponent::setHandleInputEvents( bool handleInputEvents )
    {
        ScopedLock lock( &m_uiComponentFlags, true );
        m_uiComponentFlags = BitUtil::setFlagValue(
            m_uiComponentFlags.load(), UIComponent::handleInputEventsFlag, handleInputEvents );
    }

    bool UIComponent::getHandleInputEvents() const
    {
        ScopedLock lock( &m_uiComponentFlags, false );
        return BitUtil::getFlagValue( m_uiComponentFlags.load(), UIComponent::handleInputEventsFlag );
    }

    void UIComponent::setLabel( const String &label )
    {
        m_label = label;
    }

    String UIComponent::getLabel() const
    {
        auto s = m_label.load();
        return s.str();
    }

    void UIComponent::setShowLabel( bool showLabel )
    {
        ScopedLock lock( &m_uiComponentFlags, true );
        m_uiComponentFlags =
            BitUtil::setFlagValue( m_uiComponentFlags.load(), UIComponent::showLabelFlag, showLabel );
    }

    bool UIComponent::getShowLabel() const
    {
        ScopedLock lock( &m_uiComponentFlags, false );
        return BitUtil::getFlagValue( m_uiComponentFlags.load(), UIComponent::showLabelFlag );
    }

    void UIComponent::setLabelActor( SmartPtr<IGameActor> labelActor )
    {
        m_labelActor = labelActor;
    }

    SmartPtr<IGameActor> UIComponent::getLabelActor() const
    {
        return m_labelActor;
    }

    Vector2<real_Num> UIComponent::getEditorWindowBorderSize() const
    {
        return m_editorWindowBorderSize;
    }

    void UIComponent::setEditorWindowBorderSize( const Vector2<real_Num> &editorWindowBorderSize )
    {
        m_editorWindowBorderSize = editorWindowBorderSize;
    }

    void UIComponent::setLayoutTransform( LayoutTransform *layoutTransform )
    {
        m_layoutTransform = layoutTransform;
    }

    LayoutTransform *UIComponent::getLayoutTransform() const
    {
        if( !m_layoutTransform )
        {
            if( auto actor = getActorPtr() )
            {
                ScopedLock lock( this, true );
                m_layoutTransform = actor->getComponentPtr<LayoutTransform>();
            }
        }

        return m_layoutTransform;
    }

    UIComponent::UIElementListener::UIElementListener() = default;

    UIComponent::UIElementListener::~UIElementListener() = default;

    Parameter UIComponent::UIElementListener::handleEvent( EventType eventType, hash_type eventValue,
                                                           const Array<Parameter> &arguments,
                                                           SmartPtr<ISharedObject> sender,
                                                           SmartPtr<ISharedObject> object,
                                                           SmartPtr<IEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( applicationManager->getQuit() )
        {
            return {};
        }

        if( !applicationManager->isRunning() )
        {
            return {};
        }

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto taskManager = applicationManager->getTaskManager();
        if( !taskManager )
        {
            return {};
        }

        auto applicationTask = taskManager->getTask( TaskId::Application );
        auto task = Thread::getCurrentTask();

        if( task == TaskId::Application )
        {
            auto owner = getOwner();
            auto actors = owner->getActorListeners();
            for( auto actor : actors )
            {
                actor->handleEvent( eventType, eventValue, arguments, sender, getOwner(), event );
            }

            if( auto actor = owner->getActor() )
            {
                auto components = actor->getComponents();
                for( auto component : components )
                {
                    if( component )
                    {
                        component->handleEvent( eventType, eventValue, arguments, sender, object,
                                                event );
                    }
                }
            }

            auto componentListeners = owner->getComponentListeners();
            for( auto componentListener : componentListeners )
            {
                if( componentListener )
                {
                    componentListener->handleEvent( eventType, eventValue, arguments, sender, getOwner(),
                                                    event );
                }
            }
        }

        return {};
    }

    SmartPtr<UIComponent> UIComponent::UIElementListener::getOwner() const
    {
        return m_owner.lock();
    }

    void UIComponent::UIElementListener::setOwner( SmartPtr<UIComponent> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::scene
