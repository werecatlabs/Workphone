#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/UI/IUILayoutContainer.hpp>
#include <Workphone/Interface/UI/IUILayoutWindow.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    const String Layout::referenceSizeStr = String( "referenceSize" );

    const String Layout::flagBorderStr = String( "flagBorder" );
    const String Layout::flagMovableStr = String( "flagMovable" );
    const String Layout::flagScalableStr = String( "flagScalable" );
    const String Layout::flagClosableStr = String( "flagClosable" );
    const String Layout::flagMinimizableStr = String( "flagMinimizable" );
    const String Layout::flagNoScrollbarStr = String( "flagNoScrollbar" );
    const String Layout::flagTitleStr = String( "flagTitle" );
    const String Layout::flagScrollAutoHideStr = String( "flagScrollAutoHide" );
    const String Layout::flagBackgroundStr = String( "flagBackground" );
    const String Layout::flagScaleLeftStr = String( "flagScaleLeft" );
    const String Layout::flagNoInputStr = String( "flagNoInput" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Layout, UIComponent );

    Layout::Layout()
    {
        m_referenceSize = Vector2I( 1920, 1080 );
    }

    Layout::~Layout()
    {
    }

    void Layout::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            UIComponent::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            m_panelFlags = (PanelFlags)( (u32)PanelFlags::MOVABLE | (u32)PanelFlags::SCALABLE );

            createUI();

            auto inputMgr = applicationManager->getInputDeviceManager();
            if( inputMgr )
            {
                inputMgr->addListener( m_elementListener );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Layout::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &state = getLoadingState();
            if( state != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto renderUI = applicationManager->getRenderUI();
                if( !renderUI )
                {
                    WP_LOG_ERROR( "Render UI is not available" );
                    setLoadingState( LoadingState::Unloaded );
                    return;
                }

                auto inputMgr = applicationManager->getInputDeviceManager();
                if( inputMgr )
                {
                    inputMgr->removeListener( m_elementListener );
                }

                if( auto element = getElement() )
                {
                    renderUI->removeElement( element );
                    setElement( nullptr );
                }

                if( auto layout = getLayout() )
                {
                    layout->removeAllChildren();
                    layout->setContainer( nullptr );

                    // auto container = getContainer();
                    // if (container)
                    //{
                    //	layout->removeChild(container);
                    // }

                    renderUI->removeElement( layout );
                    setLayout( nullptr );
                }

                UIComponent::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Layout::createUI()
    {
        auto currentLayout = getLayout();
        if( !currentLayout )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto renderUI = applicationManager->getRenderUI();
            if( !renderUI )
            {
                WP_LOG_ERROR( "Render UI is not available" );
                return;
            }

            auto layout = renderUI->addElementByType<ui::IUILayoutWindow>();
            if( layout )
            {
                setLayout( layout );
                setElement( layout );
                applyFlagsToLayout();
            }
        }
    }

    SmartPtr<ui::IUILayoutWindow> Layout::getLayout() const
    {
        return m_layout;
    }

    void Layout::setLayout( SmartPtr<ui::IUILayoutWindow> layout )
    {
        m_layout = layout;
    }

    Vector2I Layout::getReferenceSize() const
    {
        return m_referenceSize;
    }

    void Layout::setReferenceSize( const Vector2I &referenceSize )
    {
        m_referenceSize = referenceSize;
    }

    Array<SmartPtr<ISharedObject>> Layout::getChildObjects() const
    {
        auto objects = UIComponent::getChildObjects();
        objects.emplace_back( m_layout );
        return objects;
    }

    SmartPtr<Properties> Layout::getProperties() const
    {
        auto properties = UIComponent::getProperties();
        properties->setProperty( Layout::referenceSizeStr, m_referenceSize );

        properties->setProperty( Layout::flagBorderStr, hasPanelFlag( PanelFlags::BORDER ) );
        properties->setProperty( Layout::flagMovableStr, hasPanelFlag( PanelFlags::MOVABLE ) );
        properties->setProperty( Layout::flagScalableStr, hasPanelFlag( PanelFlags::SCALABLE ) );
        properties->setProperty( Layout::flagClosableStr, hasPanelFlag( PanelFlags::CLOSABLE ) );
        properties->setProperty( Layout::flagMinimizableStr, hasPanelFlag( PanelFlags::MINIMIZABLE ) );
        properties->setProperty( Layout::flagNoScrollbarStr, hasPanelFlag( PanelFlags::NO_SCROLLBAR ) );
        properties->setProperty( Layout::flagTitleStr, hasPanelFlag( PanelFlags::TITLE ) );
        properties->setProperty( Layout::flagScrollAutoHideStr,
                                 hasPanelFlag( PanelFlags::SCROLL_AUTO_HIDE ) );
        properties->setProperty( Layout::flagBackgroundStr, hasPanelFlag( PanelFlags::BACKGROUND ) );
        properties->setProperty( Layout::flagScaleLeftStr, hasPanelFlag( PanelFlags::SCALE_LEFT ) );
        properties->setProperty( Layout::flagNoInputStr, hasPanelFlag( PanelFlags::NO_INPUT ) );

        return properties;
    }

    void Layout::setProperties( SmartPtr<Properties> properties )
    {
        UIComponent::setProperties( properties );

        properties->getPropertyValue( Layout::referenceSizeStr, m_referenceSize );

        // Reconstruct m_panelFlags from individual bool properties.
        auto readFlag = [&]( const String &key, PanelFlags flag ) {
            bool value = false;
            if( properties->getPropertyValue( key, value ) )
            {
                if( value )
                    addPanelFlag( flag );
                else
                    removePanelFlag( flag );
            }
        };

        readFlag( Layout::flagBorderStr, PanelFlags::BORDER );
        readFlag( Layout::flagMovableStr, PanelFlags::MOVABLE );
        readFlag( Layout::flagScalableStr, PanelFlags::SCALABLE );
        readFlag( Layout::flagClosableStr, PanelFlags::CLOSABLE );
        readFlag( Layout::flagMinimizableStr, PanelFlags::MINIMIZABLE );
        readFlag( Layout::flagNoScrollbarStr, PanelFlags::NO_SCROLLBAR );
        readFlag( Layout::flagTitleStr, PanelFlags::TITLE );
        readFlag( Layout::flagScrollAutoHideStr, PanelFlags::SCROLL_AUTO_HIDE );
        readFlag( Layout::flagBackgroundStr, PanelFlags::BACKGROUND );
        readFlag( Layout::flagScaleLeftStr, PanelFlags::SCALE_LEFT );
        readFlag( Layout::flagNoInputStr, PanelFlags::NO_INPUT );

        applyFlagsToLayout();

        if( auto actor = getActor() )
        {
            if( auto transform = actor->getComponent<LayoutTransform>() )
            {
                auto sz = Vector2<real_Num>( static_cast<f32>( m_referenceSize.X() ),
                                             static_cast<f32>( m_referenceSize.Y() ) );
                transform->setSize( sz );
            }
        }
    }

    void Layout::updateFlags( u32 flags, u32 oldFlags )
    {
        ScopedLock lock( this, true );

        auto eState = getState();
        switch( eState )
        {
        case State::Edit:
        case State::Play:
        {
            if( auto actor = getActorPtr() )
            {
                if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagInScene ) !=
                    BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagInScene ) )
                {
                    auto visible = isEnabled() && actor->isEnabledInScene();

                    if( auto layout = getLayout() )
                    {
                        layout->setVisible( visible );
                    }

                    auto canvasTransforms = actor->getComponentsAndInChildren<LayoutTransform>();
                    for( auto canvasTransform : canvasTransforms )
                    {
                        canvasTransform->updateTransform();
                    }
                }
                else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                         BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
                {
                    auto visible = isEnabled() && actor->isEnabledInScene();

                    if( auto layout = getLayout() )
                    {
                        layout->setVisible( visible );
                    }

                    auto canvasTransforms = actor->getComponentsAndInChildren<LayoutTransform>();
                    for( auto canvasTransform : canvasTransforms )
                    {
                        canvasTransform->updateTransform();
                    }
                }
                else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabledInScene ) !=
                         BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabledInScene ) )
                {
                    auto visible = isEnabled() && actor->isEnabledInScene();

                    if( auto layout = getLayout() )
                    {
                        layout->setVisible( visible );
                    }

                    auto canvasTransforms = actor->getComponentsAndInChildren<LayoutTransform>();
                    for( auto canvasTransform : canvasTransforms )
                    {
                        canvasTransform->updateTransform();
                    }
                }

                auto children = actor->getAllComponentsInChildren<UIComponent>();
                for( auto &child : children )
                {
                    child->updateOrder();
                }

                if( auto element = getElement() )
                {
                    element->updateZOrder();
                }

                for( auto &child : children )
                {
                    if( auto element = child->getElement() )
                    {
                        child->updateOrder();
                    }
                }

                if( auto element = getElement() )
                {
                    if( auto stateContext = element->getStateContext() )
                    {
                        stateContext->setDirty( true );
                    }
                }

                for( auto &child : children )
                {
                    if( auto element = child->getElement() )
                    {
                        if( auto stateContext = element->getStateContext() )
                        {
                            stateContext->setDirty( true );
                        }
                    }
                }
            }
        }
        break;
        default:
        {
        }
        };
    }

    FSMReturnType Layout::handleComponentEvent( u32 state, FSMEvent eventType )
    {
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
                auto actor = getActor();
                if( actor )
                {
                    auto canvasTransforms = actor->getComponentsAndInChildren<LayoutTransform>();
                    for( auto canvasTransform : canvasTransforms )
                    {
                        canvasTransform->updateTransform();
                    }

                    auto visible = isEnabled() && actor->isEnabledInScene();
                    if( auto layout = getLayout() )
                    {
                        layout->setVisible( visible );
                    }
                }

                updateOrder();

                if( actor )
                {
                    if( auto transform = actor->getComponent<LayoutTransform>() )
                    {
                        transform->setSize(
                            Vector2<real_Num>( static_cast<real_Num>( m_referenceSize.X() ),
                                               static_cast<real_Num>( m_referenceSize.Y() ) ) );
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
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
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

    void Layout::updateVisibility()
    {
        if( auto actor = getActor() )
        {
            auto visible = isEnabled() && actor->isEnabledInScene();
            if( auto layout = getLayout() )
            {
                layout->setVisible( visible );
            }
        }
    }

    u32 Layout::getElementOrder( SmartPtr<UIComponent> component ) const
    {
        if( auto actor = getActor() )
        {
            auto elements = actor->getAllComponentsInChildren<UIComponent>();
            auto it = std::find( elements.begin(), elements.end(), component );
            if( it != elements.end() )
            {
                return static_cast<u32>( std::distance( elements.begin(), it ) );
            }
        }

        return 0;
    }

    u32 Layout::getElementOrderReversed( SmartPtr<UIComponent> component ) const
    {
        if( auto actor = getActor() )
        {
            auto elements = actor->getAllComponentsInChildren<UIComponent>();
            auto it = std::find( elements.begin(), elements.end(), component );
            if( it != elements.end() )
            {
                return static_cast<u32>( elements.size() - std::distance( elements.begin(), it ) );
            }
        }

        return 0;
    }

    s32 Layout::getZOrder( SmartPtr<IGameActor> obj )
    {
        if( auto actor = getActor() )
        {
            auto children = actor->getAllComponentsInChildren<UIComponent>();

            auto count = 0;
            for( auto &child : children )
            {
                if( child->getActor() == obj )
                {
                    return count;
                }

                ++count;
            }
        }

        return 0;
    }

    Parameter Layout::handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == childAdded )
        {
            ScopedLock lock( this );

            auto eState = getState();
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                if( auto actor = getActor() )
                {
                    actor->updateOrder();

                    if( auto element = getElement() )
                    {
                        element->updateZOrder();
                    }
                }
            }
            }
        }
        else if( eventValue == childRemoved )
        {
            ScopedLock lock( this );

            auto eState = getState();
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                updateOrder();

                if( auto actor = getActor() )
                {
                    auto children = actor->getComponentsInChildren<UIComponent>();
                    for( auto child : children )
                    {
                        child->updateOrder();
                    }

                    if( auto element = getElement() )
                    {
                        element->updateZOrder();
                    }
                }
            }
            }
        }
        else if( eventValue == childAddedInHierarchy )
        {
            ScopedLock lock( this );

            auto eState = getState();
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                updateOrder();

                if( auto actor = getActor() )
                {
                    auto children = actor->getComponentsInChildren<UIComponent>();
                    for( auto child : children )
                    {
                        child->updateOrder();
                    }

                    if( auto element = getElement() )
                    {
                        element->updateZOrder();
                    }
                }
            }
            }
        }
        else if( eventValue == childRemovedInHierarchy )
        {
            ScopedLock lock( this );

            auto eState = getState();
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                updateOrder();

                if( auto actor = getActor() )
                {
                    auto children = actor->getComponentsInChildren<UIComponent>();
                    for( auto child : children )
                    {
                        child->updateOrder();
                    }

                    if( auto element = getElement() )
                    {
                        element->updateZOrder();
                    }
                }
            }
            }
        }
        else if( eventValue == sceneWasLoaded )
        {
            ScopedLock lock( this );

            if( auto actor = getActorPtr() )
            {
                auto visible = isEnabled() && actor->isEnabledInScene();

                if( auto layout = getLayout() )
                {
                    layout->setVisible( visible );

                    if( auto stateContext = layout->getStateContext() )
                    {
                        stateContext->setDirty( true );
                    }
                }

                if( auto element = getElement() )
                {
                    if( auto stateContext = element->getStateContext() )
                    {
                        stateContext->setDirty( true );
                    }
                }

                auto children = actor->getAllComponentsInChildrenPtr<UIComponent>();
                for( auto child : children )
                {
                    if( auto element = child->getElementPtr() )
                    {
                        if( auto stateContext = element->getStateContext() )
                        {
                            stateContext->setDirty( true );
                        }
                    }
                }
            }
        }
        else if( eventValue == IEvent::inputEvent )
        {
            handleEvent( event );
        }

        return {};
    }

    bool Layout::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        auto actor = getActor();
        return handleEvent( actor, event );
    }

    bool Layout::handleEvent( SmartPtr<IGameActor> actor, const SmartPtr<IInputEvent> &event )
    {
        auto children = actor->getAllChildren();
        for( auto &child : children )
        {
            if( child )
            {
                if( child->isEnabled() )
                {
                    ScopedLock lock( this );

                    auto elements = child->getComponentsByTypePtr<UIComponent>();
                    for( auto element : elements )
                    {
                        if( !element->isDerived<Layout>() )
                        {
                            if( element->handleEvent( event ) )
                            {
                                return true;
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    void Layout::updateOrder()
    {
        if( auto actor = getActorPtr() )
        {
            if( auto layoutTransform = getLayoutTransform() )
            {
                auto autoCalculateOrder = layoutTransform->getAutoCalculateOrder();
                if( autoCalculateOrder )
                {
                    if( actor )
                    {
                        auto order = getZOrder( actor );

                        layoutTransform->setZOrder( order );
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
    }

    void Layout::setPanelFlags( PanelFlags panelFlags )
    {
        m_panelFlags = panelFlags;
        applyFlagsToLayout();
    }

    PanelFlags Layout::getPanelFlags() const
    {
        return m_panelFlags;
    }

    bool Layout::hasPanelFlag( PanelFlags flag ) const
    {
        return ( static_cast<unsigned int>( m_panelFlags ) & static_cast<unsigned int>( flag ) ) != 0u;
    }

    void Layout::addPanelFlag( PanelFlags flag )
    {
        m_panelFlags = static_cast<PanelFlags>( static_cast<unsigned int>( m_panelFlags ) |
                                                static_cast<unsigned int>( flag ) );
    }

    void Layout::removePanelFlag( PanelFlags flag )
    {
        m_panelFlags = static_cast<PanelFlags>( static_cast<unsigned int>( m_panelFlags ) &
                                                ~static_cast<unsigned int>( flag ) );
    }

    void Layout::applyFlagsToLayout()
    {
        if( auto layout = getLayout() )
        {
            using WF = ui::IUILayoutWindow::WindowFlags;
            layout->setWindowFlags( static_cast<WF>( static_cast<unsigned int>( m_panelFlags ) ) );
        }
    }

}  // namespace workphone::scene
