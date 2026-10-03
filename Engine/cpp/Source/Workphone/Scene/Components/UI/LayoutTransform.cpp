#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/UI/UIComponent.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/Components/UI/Text.hpp>
#include <Workphone/Scene/UiUtil.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IComponentSystem.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/Scene/Systems/UI/LayoutTransformSystem.hpp>
#include <Workphone/Scene/Components/UI/UIComponent.hpp>
#include <Workphone/State/States/UILayoutStateData.hpp>
#include <Workphone/State/States/UITransformStateData.hpp>

namespace workphone::scene
{
    namespace
    {
        void markLayoutTransformDirty( LayoutTransform *layoutTransform )
        {
            if( !layoutTransform )
            {
                return;
            }

            if( auto layoutState = layoutTransform->getComponentStateByType<UILayoutStateData>() )
            {
                layoutState->flags =
                    BitUtil::setFlagValue<u8>( layoutState->flags, UILayoutStateData::dirtyFlag, true );
            }

            if( auto componentSystem = layoutTransform->getComponentSystemPtr() )
            {
                componentSystem->addDirtyComponent( layoutTransform );
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::scene, LayoutTransform, Component );

    const String LayoutTransform::horizontalStr = String( "Horizontal" );
    const String LayoutTransform::verticalStr = String( "Vertical" );
    const String LayoutTransform::orderStr = String( "order" );
    const String LayoutTransform::autoCalculateOrderStr = String( "autoCalculateOrder" );

    const u8 LayoutTransform::autoCalculateOrderFlag = ( 1 << 1 );

    LayoutTransform::LayoutTransform() = default;

    LayoutTransform::~LayoutTransform() = default;

    void LayoutTransform::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_uiComponentFlags = autoCalculateOrderFlag;
        Component::load( data );
        m_layoutTransforms.reserve( 32 );
        setLoadingState( LoadingState::Loaded );
    }

    void LayoutTransform::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        m_uiComponent = nullptr;
        m_layout = nullptr;

        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void LayoutTransform::updateTransform()
    {
        auto componentState = getState();
        switch( componentState )
        {
        case State::Edit:
        case State::Play:
        {
            auto actor = getActorPtr();

            auto canvas = getLayout();
            if( !canvas )
            {
                setupCanvas();
            }

            if( !m_uiComponent )
            {
                if( actor )
                {
                    if( auto uiComponent = actor->getComponent<UIComponent>() )
                    {
                        m_uiComponent = uiComponent;
                    }
                }
            }

            if( auto state = getComponentStateByType<UILayoutStateData>() )
            {
                state->owner = this;
                state->uiComponent = m_uiComponent.get();
                state->flags =
                    BitUtil::setFlagValue<u8>( state->flags, UILayoutStateData::dirtyFlag, true );
            }

            if( actor )
            {
                auto layoutTransforms = actor->getComponentsInChildrenPtr<LayoutTransform>();
                for( auto layoutTransform : layoutTransforms )
                {
                    layoutTransform->updateTransform();
                }
            }

            if( auto componentSystem = getComponentSystemPtr() )
            {
                componentSystem->addDirtyComponent( this );
            }
        }
        break;
        };
    }

    Vector2<real_Num> LayoutTransform::getMin() const
    {
        auto transformState = getComponentStateByType<UITransformStateData>();
        auto anchorState = getComponentStateByType<UIAnchorStateData>();

        if( transformState && anchorState )
        {
            auto pos = transformState->position;
            auto size = transformState->size;

            auto anchor = anchorState->anchor;
            auto anchorMin = anchorState->anchorMin;

            // Adjust position based on anchor min
            auto adjustedPos = pos - ( anchor - anchorMin ) * size;
            return adjustedPos - size / 2.0f;
        }

        return Vector2<real_Num>::zero();
    }

    Vector2<real_Num> LayoutTransform::getAbsoluteMin() const
    {
        if( auto data = getComponentStateByType<UITransformStateData>() )
        {
            return data->absoluteMin;
        }

        return Vector2<real_Num>::zero();
    }

    Vector2<real_Num> LayoutTransform::getMax() const
    {
        auto transformState = getComponentStateByType<UITransformStateData>();
        auto anchorState = getComponentStateByType<UIAnchorStateData>();

        if( transformState && anchorState )
        {
            auto pos = transformState->position;
            auto size = transformState->size;
            auto anchorMax = anchorState->anchorMax;

            // Adjust position based on anchor max
            auto adjustedPos = pos + ( anchorMax - anchorState->anchor ) * size;
            return adjustedPos + size / 2.0f;
        }

        return Vector2<real_Num>::zero();
    }

    Vector2<real_Num> LayoutTransform::getAbsoluteMax() const
    {
        if( auto data = getComponentStateByType<UITransformStateData>() )
        {
            return data->absoluteMax;
        }

        return Vector2<real_Num>::zero();
    }

    Vector2<real_Num> LayoutTransform::getPosition() const
    {
        if( auto data = getComponentStateByType<UITransformStateData>() )
        {
            return data->position;
        }

        return Vector2<real_Num>::zero();
    }

    void LayoutTransform::setPosition( const Vector2<real_Num> &position )
    {
        auto dirty = false;

        if( auto data = getComponentStateByType<UITransformStateData>() )
        {
            if( !MathUtil<real_Num>::equals( data->position, position ) )
            {
                data->position = position;
                dirty = true;
            }
        }

        if( dirty )
        {
            markLayoutTransformDirty( this );
        }
    }

    Vector2<real_Num> LayoutTransform::getSize() const
    {
        if( auto data = getComponentStateByType<UITransformStateData>() )
        {
            return data->size;
        }

        return Vector2<real_Num>::zero();
    }

    void LayoutTransform::setSize( const Vector2<real_Num> &size )
    {
        auto dirty = false;

        if( auto data = getComponentStateByType<UITransformStateData>() )
        {
            if( !MathUtil<real_Num>::equals( data->size, size ) )
            {
                data->size = size;
                dirty = true;
            }
        }

        if( dirty )
        {
            markLayoutTransformDirty( this );
        }
    }

    Vector2<real_Num> LayoutTransform::getAnchor() const
    {
        if( auto data = getComponentStateByType<UIAnchorStateData>() )
        {
            return data->anchor;
        }

        return Vector2<real_Num>::zero();
    }

    void LayoutTransform::setAnchor( const Vector2<real_Num> &anchor )
    {
        if( auto data = getComponentStateByType<UIAnchorStateData>() )
        {
            if( !MathUtil<real_Num>::equals( data->anchor, anchor ) )
            {
                data->anchor = anchor;
                markLayoutTransformDirty( this );
            }
        }
    }

    Vector2<real_Num> LayoutTransform::getAnchorMin() const
    {
        if( auto data = getComponentStateByType<UIAnchorStateData>() )
        {
            return data->anchorMin;
        }

        return Vector2<real_Num>::zero();
    }

    void LayoutTransform::setAnchorMin( const Vector2<real_Num> &anchorMin )
    {
        if( auto data = getComponentStateByType<UIAnchorStateData>() )
        {
            if( !MathUtil<real_Num>::equals( data->anchorMin, anchorMin ) )
            {
                data->anchorMin = anchorMin;
                markLayoutTransformDirty( this );
            }
        }
    }

    Vector2<real_Num> LayoutTransform::getAnchorMax() const
    {
        if( auto data = getComponentStateByType<UIAnchorStateData>() )
        {
            return data->anchorMax;
        }

        return Vector2<real_Num>::zero();
    }

    void LayoutTransform::setAnchorMax( const Vector2<real_Num> &anchorMax )
    {
        if( auto data = getComponentStateByType<UIAnchorStateData>() )
        {
            if( !MathUtil<real_Num>::equals( data->anchorMax, anchorMax ) )
            {
                data->anchorMax = anchorMax;
                markLayoutTransformDirty( this );
            }
        }
    }

    void LayoutTransform::updateAnchorFromAlignment()
    {
        auto data = getComponentStateByType<UIAnchorStateData>();
        if( data )
        {
            auto anchorMin = data->anchorMin;
            auto anchorMax = data->anchorMax;

            auto horizontalAlignment = getHorizontalAlignment();
            auto verticalAlignment = getVerticalAlignment();

            switch( horizontalAlignment )
            {
            case HorizontalAlignment::LEFT:
            {
                anchorMin.X() = 0.0f;
                anchorMax.X() = 0.0f;
            }
            break;
            case HorizontalAlignment::CENTER:
            {
                anchorMin.X() = 0.5f;
                anchorMax.X() = 0.5f;
            }
            break;
            case HorizontalAlignment::RIGHT:
            {
                anchorMin.X() = 1.0f;
                anchorMax.X() = 1.0f;
            }
            break;
            }

            switch( verticalAlignment )
            {
            case VerticalAlignment::TOP:
            {
                anchorMin.Y() = 1.0f;
                anchorMax.Y() = 1.0f;
            }
            break;
            case VerticalAlignment::CENTER:
            {
                anchorMin.Y() = 0.5f;
                anchorMax.Y() = 0.5f;
            }
            break;
            case VerticalAlignment::BOTTOM:
            {
                anchorMin.Y() = 0.0f;
                anchorMax.Y() = 0.0f;
            }
            break;
            }

            const auto dirty = !MathUtil<real_Num>::equals( data->anchorMin, anchorMin ) ||
                               !MathUtil<real_Num>::equals( data->anchorMax, anchorMax );

            data->anchorMin = anchorMin;
            data->anchorMax = anchorMax;

            if( dirty )
            {
                markLayoutTransformDirty( this );
            }

            if( auto actor = getActor() )
            {
                auto layoutTransforms = actor->getAllComponentsInChildren<LayoutTransform>();
                for( auto layoutTransform : layoutTransforms )
                {
                    layoutTransform->updateAnchorFromAlignment();
                }
            }
        }
    }

    void LayoutTransform::updateOrder()
    {
        if( auto actor = getActor() )
        {
            auto uiComponent = actor->getComponent<UIComponent>();
            if( uiComponent )
            {
                uiComponent->updateOrder();
            }

            auto childUiComponents = actor->getAllComponentsInChildren<UIComponent>();
            for( auto childUiComponent : childUiComponents )
            {
                childUiComponent->updateOrder();
            }
        }
    }

    u32 LayoutTransform::getZOrder() const
    {
        return m_zOrder;
    }

    void LayoutTransform::setZOrder( u32 zOrder, bool cascade )
    {
        m_zOrder = zOrder;

        if( cascade )
        {
            if( auto actor = getActor() )
            {
                auto childComponents = actor->getComponentsInChildren<LayoutTransform>();
                for( auto childComponent : childComponents )
                {
                    childComponent->setZOrder( zOrder + 1, cascade );
                }
            }
        }
    }

    void LayoutTransform::setAutoCalculateOrder( bool autoCalculateOrder, bool cascade )
    {
        m_uiComponentFlags = BitUtil::setFlagValue(
            m_uiComponentFlags, LayoutTransform::autoCalculateOrderFlag, autoCalculateOrder );

        if( cascade )
        {
            if( auto actor = getActor() )
            {
                auto components = actor->getComponentsInChildren<LayoutTransform>();
                for( auto &component : components )
                {
                    component->setAutoCalculateOrder( autoCalculateOrder, cascade );
                }
            }
        }
    }

    bool LayoutTransform::getAutoCalculateOrder() const
    {
        return BitUtil::getFlagValue( m_uiComponentFlags, LayoutTransform::autoCalculateOrderFlag );
    }

    SmartPtr<Properties> LayoutTransform::getProperties() const
    {
        try
        {
            auto properties = Component::getProperties();

            auto autoCalculateOrder = getAutoCalculateOrder();

            auto zorder = getZOrder();
            properties->setProperty( orderStr, zorder );
            properties->setProperty( autoCalculateOrderStr, autoCalculateOrder );

            auto layoutState = getComponentStateByType<UILayoutStateData>();
            auto transformState = getComponentStateByType<UITransformStateData>();
            auto anchorState = getComponentStateByType<UIAnchorStateData>();

            if( transformState && anchorState )
            {
                auto position = transformState->position;
                auto size = transformState->size;

                auto absolutePosition = transformState->absolutePosition;
                auto absoluteSize = transformState->absoluteSize;

                auto anchor = anchorState->anchor;
                auto anchorMin = anchorState->anchorMin;
                auto anchorMax = anchorState->anchorMax;

                properties->setProperty( "Position", position );
                properties->setProperty( "Size", size );
                properties->setProperty( "AbsolutePosition", absolutePosition );
                properties->setProperty( "AbsoluteSize", absoluteSize );
                properties->setProperty( "Anchor", anchor );
                properties->setProperty( "AnchorMin", anchorMin );
                properties->setProperty( "AnchorMax", anchorMax );
            }

            properties->setProperty( horizontalStr, "" );
            properties->setProperty( verticalStr, "" );

            if( layoutState )
            {
                auto stateDirty =
                    BitUtil::getFlagValue<u8>( layoutState->flags, UILayoutStateData::dirtyFlag );
                properties->setProperty( "stateDirty", stateDirty );
            }

            auto horizontalAlignment = getHorizontalAlignment();
            auto horizontalValueStr = UiUtil::getHorizontalAlignmentString( horizontalAlignment );
            properties->setProperty( horizontalStr, horizontalValueStr );

            auto &horizontalPathProperty = properties->getPropertyObject( horizontalStr );
            horizontalPathProperty.setTypeName( "enum" );

            auto enumValues = UiUtil::getHorizontalAlignmentTypesString();
            horizontalPathProperty.setAttribute( "enum", enumValues );

            auto verticalAlignment = getVerticalAlignment();
            auto verticalValueStr = UiUtil::getVerticalAlignmentString( verticalAlignment );
            properties->setProperty( verticalStr, verticalValueStr );

            auto &verticalPathProperty = properties->getPropertyObject( verticalStr );
            verticalPathProperty.setTypeName( "enum" );

            auto verticalEnumValues = UiUtil::getVerticalAlignmentTypesString();
            verticalPathProperty.setAttribute( "enum", verticalEnumValues );

            properties->setButtonPressed( "updateOrder" );
            properties->setButtonPressed( "updateTransform" );

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void LayoutTransform::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            //WP_ASSERT( !getStates().empty() );
            Component::setProperties( properties );

            auto autoCalculateOrder = getAutoCalculateOrder();

            auto zorder = getZOrder();
            properties->getPropertyValue( orderStr, zorder );

            properties->getPropertyValue( autoCalculateOrderStr, autoCalculateOrder );

            //WP_ASSERT( !getStates().empty() );
            auto data = getComponentStateByType<UITransformStateData>();
            //WP_ASSERT( !getStates().empty() );

            auto position = Vector2<real_Num>::zero();
            auto size = Vector2<real_Num>( 1920.0f, 1080.0f );

            properties->getPropertyValue( "Position", position );
            properties->getPropertyValue( "Size", size );

            setAutoCalculateOrder( autoCalculateOrder );

            auto layoutState = getComponentStateByType<UILayoutStateData>();
            auto transformState = getComponentStateByType<UITransformStateData>();
            auto anchorState = getComponentStateByType<UIAnchorStateData>();

            if( transformState && anchorState )
            {
                properties->getPropertyValue( "Anchor", anchorState->anchor );
                properties->getPropertyValue( "AnchorMin", anchorState->anchorMin );
                properties->getPropertyValue( "AnchorMax", anchorState->anchorMax );

                String horizontalAlignmentStr;
                properties->getPropertyValue( horizontalStr, horizontalAlignmentStr );

                String verticalAlignmentStr;
                properties->getPropertyValue( verticalStr, verticalAlignmentStr );

                auto horizontalAlignment = UiUtil::getHorizontalAlignment( horizontalAlignmentStr );
                auto verticalAlignment = UiUtil::getVerticalAlignment( verticalAlignmentStr );

                auto alignmentChanged = false;
                if( getHorizontalAlignment() != horizontalAlignment )
                {
                    setHorizontalAlignment( horizontalAlignment );
                    alignmentChanged = true;
                }

                if( getVerticalAlignment() != verticalAlignment )
                {
                    setVerticalAlignment( verticalAlignment );
                    alignmentChanged = true;
                }

                if( alignmentChanged )
                {
                    updateAnchorFromAlignment();
                }

                auto stateDirty =
                    BitUtil::getFlagValue<u8>( layoutState->flags, UILayoutStateData::dirtyFlag );
                auto wasDirty = stateDirty;

                properties->setProperty( "stateDirty", stateDirty );
                layoutState->flags = BitUtil::setFlagValue<u8>(
                    layoutState->flags, UILayoutStateData::dirtyFlag, stateDirty );

                if( wasDirty != stateDirty )
                {
                    updateTransform();
                }
            }

            auto dirty = false;

            if( data )
            {
                if( !MathUtil<real_Num>::equals( data->position, position ) )
                {
                    data->position = position;
                    dirty = true;
                }

                if( !MathUtil<real_Num>::equals( data->size, size ) )
                {
                    data->size = size;
                    dirty = true;
                }
            }

            if( dirty )
            {
                if( auto actor = getActor() )
                {
                    if( auto transform = actor->getTransform() )
                    {
                        auto pos = getPosition();

                        auto localPosition = transform->getLocalPosition();
                        transform->setPosition(
                            Vector3<real_Num>( pos.X(), pos.Y(), localPosition.Z() ) );
                    }
                }
            }

            updateTransform();

            if( properties->isButtonPressed( "updateOrder" ) )
            {
                updateOrder();
            }

            if( properties->isButtonPressed( "updateTransform" ) )
            {
                updateTransform();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void LayoutTransform::setHorizontalAlignment( HorizontalAlignment gha )
    {
        if( auto data = getComponentStateByType<UILayoutStateData>() )
        {
            data->horizontalAlignment = static_cast<u8>( gha );
        }

        updateAnchorFromAlignment();
        updateTransform();
    }

    HorizontalAlignment LayoutTransform::getHorizontalAlignment() const
    {
        if( auto data = getComponentStateByType<UILayoutStateData>() )
        {
            return static_cast<HorizontalAlignment>( (u8)data->horizontalAlignment );
        }

        return HorizontalAlignment::CENTER;
    }

    void LayoutTransform::setVerticalAlignment( VerticalAlignment gva )
    {
        if( auto data = getComponentStateByType<UILayoutStateData>() )
        {
            data->verticalAlignment = static_cast<u8>( gva );
        }

        updateAnchorFromAlignment();
        updateTransform();
    }

    VerticalAlignment LayoutTransform::getVerticalAlignment() const
    {
        if( auto data = getComponentStateByType<UILayoutStateData>() )
        {
            return static_cast<VerticalAlignment>( (u8)data->verticalAlignment );
        }

        return VerticalAlignment::CENTER;
    }

    Vector2<real_Num> LayoutTransform::getAbsolutePosition() const
    {
        if( auto data = getComponentStateByType<UITransformStateData>() )
        {
            return data->absolutePosition;
        }

        return Vector2<real_Num>::zero();
    }

    void LayoutTransform::setAbsolutePosition( const Vector2<real_Num> &absolutePosition )
    {
        if( auto data = getComponentStateByType<UITransformStateData>() )
        {
            data->absolutePosition = absolutePosition;
        }
    }

    Vector2<real_Num> LayoutTransform::getAbsoluteSize() const
    {
        if( auto data = getComponentStateByType<UITransformStateData>() )
        {
            return data->absoluteSize;
        }

        return Vector2<real_Num>::zero();
    }

    void LayoutTransform::setAbsoluteSize( const Vector2<real_Num> &absoluteSize )
    {
        if( auto data = getComponentStateByType<UITransformStateData>() )
        {
            data->absoluteSize = absoluteSize;
        }
    }

    Vector2<real_Num> LayoutTransform::getRelativePosition() const
    {
        if( auto layout = workphone::static_pointer_cast<Layout>( getLayout() ) )
        {
            auto position = getAbsolutePosition();
            auto referenceSize = layout->getReferenceSize();
            return position / Vector2<real_Num>( (real_Num)referenceSize.x, (real_Num)referenceSize.y );
        }

        return {};
    }

    Vector2<real_Num> LayoutTransform::getRelativeSize() const
    {
        if( auto layout = workphone::static_pointer_cast<Layout>( getLayout() ) )
        {
            auto size = getAbsoluteSize();
            auto referenceSize = layout->getReferenceSize();
            return size / Vector2<real_Num>( (real_Num)referenceSize.x, (real_Num)referenceSize.y );
        }

        return {};
    }

    UIComponent *LayoutTransform::getLayoutPtr() const
    {
        return m_layout.get();
    }

    SmartPtr<UIComponent> LayoutTransform::getLayout() const
    {
        return m_layout;
    }

    void LayoutTransform::setLayout( SmartPtr<UIComponent> layout )
    {
        m_layout = layout;
    }

    FSMReturnType LayoutTransform::handleComponentEvent( u32 state, FSMEvent eventType )
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
                updateLayoutTransforms();

                if( auto actor = getActorPtr() )
                {
                    if( auto uiComponent = actor->getComponentPtr<UIComponent>() )
                    {
                        m_uiComponent = uiComponent;
                    }
                }

                setupCanvas();

                updateTransform();
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
            break;
        case FSMEvent::Complete:
            break;
        case FSMEvent::NewState:
            break;
        case FSMEvent::WaitForChange:
            break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    Parameter LayoutTransform::handleEvent( EventType eventType, hash_type eventValue,
                                            const Array<Parameter> &arguments,
                                            SmartPtr<ISharedObject> sender,
                                            SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            if( eventValue == hierarchyChanged )
            {
                if( auto actor = getActorPtr() )
                {
                    auto uiComponent = actor->getComponentPtr<UIComponent>();
                    if( uiComponent )
                    {
                        uiComponent->updateOrder();
                    }

                    auto childUiComponents = actor->getAllComponentsInChildrenPtr<UIComponent>();
                    for( auto childUiComponent : childUiComponents )
                    {
                        childUiComponent->updateOrder();
                    }
                }

                updateTransform();
            }
            else if( eventValue == IComponent::childAdded )
            {
                auto componentState = getState();
                switch( componentState )
                {
                case State::Edit:
                case State::Play:
                {
                    updateLayoutTransforms();
                }
                break;
                };
            }

            return Component::handleEvent( eventType, eventValue, arguments, sender, object, event );
        }
        break;
        default:
        {
        }
        }

        return {};
    }

    void LayoutTransform::setupCanvas()
    {
        if( auto actor = getActorPtr() )
        {
            if( auto rootActor = actor->getSceneRootPtr() )
            {
                auto layout = rootActor->getComponentInThisAndChildren<Layout>();
                setLayout( layout );
            }
            else
            {
                auto layout = actor->getComponentInThisAndChildren<Layout>();
                setLayout( layout );
            }
        }
    }

    void LayoutTransform::setUIComponent( SmartPtr<UIComponent> uiComponent )
    {
        m_uiComponent = uiComponent;
    }

    SmartPtr<UIComponent> LayoutTransform::getUIComponent() const
    {
        if( !m_uiComponent )
        {
            if( auto actor = getActor() )
            {
                m_uiComponent = actor->getComponent<UIComponent>();
            }
        }

        return m_uiComponent;
    }

    void LayoutTransform::updateLayoutTransforms()
    {
        if( auto actor = getActorPtr() )
        {
            auto layoutTransforms = actor->getComponentsInChildrenPtr<LayoutTransform>();

            auto size = layoutTransforms.size();
            m_layoutTransforms.reserve( size );

            for( auto layoutTransform : layoutTransforms )
            {
                m_layoutTransforms.push_back( layoutTransform );
            }

            std::sort( m_layoutTransforms.begin(), m_layoutTransforms.end() );
        }
    }

}  // namespace workphone::scene
