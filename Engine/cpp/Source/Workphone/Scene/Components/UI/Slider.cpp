#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/Slider.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/ComponentEvent.hpp>
#include <Workphone/Scene/Components/ComponentEventListener.hpp>
#include <Workphone/Scene/UiUtil.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUISlider.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUILabelSliderPair.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Slider, UIComponent );

    const String Slider::handleStr = "handle";
    const String Slider::backgroundStr = "background";
    const String Slider::fillStr = "fill";
    const String Slider::valueStr = "value";
    const String Slider::minValueStr = "minValue";
    const String Slider::maxValueStr = "maxValue";
    const String Slider::directionStr = "direction";
    const String Slider::isDraggingStr = "isDragging";

    Slider::Slider() :
        m_minValue( 0.0f ),
        m_maxValue( 1.0f ),
        m_sliderValue( 0.0f ),
        m_direction( Direction::Horizontal ),
        m_isDragging( false )
    {
    }

    Slider::~Slider()
    {
    }

    void Slider::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            // createUI();
            UIComponent::load( data );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Slider::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        UIComponent::unload( data );

        setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<Properties> Slider::getProperties() const
    {
        auto properties = UIComponent::getProperties();
        properties->setProperty( handleStr, m_handleActor );
        properties->setProperty( backgroundStr, m_background );
        properties->setProperty( fillStr, m_fill );
        properties->setProperty( valueStr, m_sliderValue );
        properties->setProperty( minValueStr, m_minValue );
        properties->setProperty( maxValueStr, m_maxValue );
        properties->setPropertyAsEnum( directionStr, (u32)m_direction, UiUtil::directionNames );
        properties->setProperty( isDraggingStr, m_isDragging );

        return properties;
    }

    void Slider::setProperties( SmartPtr<Properties> properties )
    {
        UIComponent::setProperties( properties );

        properties->getPropertyValue( handleStr, m_handleActor );
        properties->getPropertyValue( backgroundStr, m_background );
        properties->getPropertyValue( fillStr, m_fill );
        properties->getPropertyValue( valueStr, m_sliderValue );
        properties->getPropertyValue( minValueStr, m_minValue );
        properties->getPropertyValue( maxValueStr, m_maxValue );
        properties->getPropertyValue( directionStr, (u32 &)m_direction );
        properties->getPropertyValue( isDraggingStr, m_isDragging );

        // Ensure min is less than max
        if( m_minValue > m_maxValue )
        {
            std::swap( m_minValue, m_maxValue );
        }

        // Clamp value to range
        m_sliderValue = MathF::clamp( m_sliderValue, m_minValue, m_maxValue );

        updateSliderPosition();
        updateElementState();
    }

    bool Slider::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        if( !event )
        {
            return false;
        }

        if( auto mouseState = event->getMouseState() )
        {
            Vector2<real_Num> localMousePosition;
            if( getLocalMousePosition( event, m_background, localMousePosition ) )
            {
                if( mouseState->isButtonPressed( IMouseState::MOUSE_LEFT ) || mouseState->isDragging() )
                {
                    m_isDragging = true;
                    if( m_background )
                    {
                        auto bgTransform = m_background->getComponent<LayoutTransform>();
                        if( !bgTransform )
                        {
                            return false;
                        }

                        auto bgSize = bgTransform->getAbsoluteSize();
                        auto axisSize = m_direction == Direction::Horizontal ? bgSize.X() : bgSize.Y();
                        if( axisSize > 0.0f )
                        {
                            auto axisPos = m_direction == Direction::Horizontal ? localMousePosition.X()
                                                                                : localMousePosition.Y();
                            f32 normalizedValue = MathF::clamp01( axisPos / axisSize );
                            f32 newValue = m_minValue + normalizedValue * ( m_maxValue - m_minValue );

                            if( !MathF::equals( m_sliderValue, newValue ) )
                            {
                                setValue( newValue );

                                // Trigger callback
                                if( m_callbackFunction )
                                {
                                    m_callbackFunction( IEvent::handleValueChanged );
                                }
                            }
                        }
                    }

                    return true;  // Event was handled
                }
                else
                {
                    m_isDragging = false;
                }
            }
        }

        return false;
    }

    void Slider::setCallbackFunction( std::function<void( hash_type )> callbackFunction )
    {
        m_callbackFunction = callbackFunction;
    }

    void Slider::setFill( SmartPtr<IGameActor> fill )
    {
        m_fill = fill;
    }

    SmartPtr<IGameActor> Slider::getFill() const
    {
        return m_fill;
    }

    void Slider::setBackground( SmartPtr<IGameActor> background )
    {
        m_background = background;
    }

    SmartPtr<IGameActor> Slider::getBackground() const
    {
        return m_background;
    }

    void Slider::setHandleActor( SmartPtr<IGameActor> handle )
    {
        m_handleActor = handle;
    }

    SmartPtr<IGameActor> Slider::getHandleActor() const
    {
        return m_handleActor;
    }

    void Slider::setMinValue( f32 minValue )
    {
        m_minValue = minValue;
        if( m_minValue > m_maxValue )
        {
            std::swap( m_minValue, m_maxValue );
        }
        m_sliderValue = MathF::clamp( m_sliderValue, m_minValue, m_maxValue );
        updateSliderPosition();
        updateElementState();
    }

    f32 Slider::getMinValue() const
    {
        return m_minValue;
    }

    void Slider::setMaxValue( f32 maxValue )
    {
        m_maxValue = maxValue;
        if( m_minValue > m_maxValue )
        {
            std::swap( m_minValue, m_maxValue );
        }
        m_sliderValue = MathF::clamp( m_sliderValue, m_minValue, m_maxValue );
        updateSliderPosition();
        updateElementState();
    }

    f32 Slider::getMaxValue() const
    {
        return m_maxValue;
    }

    void Slider::setValue( f32 sliderValue )
    {
        m_sliderValue = MathF::clamp( sliderValue, m_minValue, m_maxValue );
        updateSliderPosition();
        updateElementState();
    }

    f32 Slider::getValue() const
    {
        return m_sliderValue;
    }

    void Slider::setDirection( Direction direction )
    {
        m_direction = direction;
        updateSliderPosition();
        updateElementState();
    }

    Direction Slider::getDirection() const
    {
        return m_direction;
    }

    void Slider::updateSliderPosition()
    {
        auto sliderValue = getValue();
        auto valueRange = m_maxValue - m_minValue;
        auto normalizedValue = valueRange > 0.0f ? ( sliderValue - m_minValue ) / valueRange : 0.0f;
        normalizedValue = MathF::clamp01( normalizedValue );

        auto direction = getDirection();
        switch( direction )
        {
        case Direction::Horizontal:
        {
            if( auto handle = getHandleActor() )
            {
                auto backgroundTransform =
                    m_background ? m_background->getComponentPtr<LayoutTransform>() : nullptr;
                if( !backgroundTransform )
                {
                    break;
                }

                auto backgroundSize = backgroundTransform->getSize();

                auto newHandlePosition = normalizedValue * backgroundSize.X();

                auto handleTransform = handle->getComponentPtr<LayoutTransform>();
                if( !handleTransform )
                {
                    break;
                }

                auto handlePosition = handleTransform->getPosition();
                auto anchoredPosition = Vector2<real_Num>( newHandlePosition, handlePosition.Y() );

                handleTransform->setPosition( anchoredPosition );
            }

            if( auto fill = getFill() )
            {
                auto backgroundTransform =
                    m_background ? m_background->getComponentPtr<LayoutTransform>() : nullptr;
                if( !backgroundTransform )
                {
                    break;
                }

                auto backgroundSize = backgroundTransform->getSize();

                auto fillTransform = fill->getComponentPtr<LayoutTransform>();
                if( !fillTransform )
                {
                    break;
                }

                auto fillSize = fillTransform->getSize();

                auto sizeDelta = Vector2<real_Num>( normalizedValue * backgroundSize.X(), fillSize.Y() );
                fillTransform->setSize( sizeDelta );
            }
        }
        break;
        case Direction::Vertical:
        {
            if( auto handle = getHandleActor() )
            {
                auto backgroundTransform =
                    m_background ? m_background->getComponentPtr<LayoutTransform>() : nullptr;
                if( !backgroundTransform )
                {
                    break;
                }

                auto backgroundSize = backgroundTransform->getSize();

                auto newHandlePosition = normalizedValue * backgroundSize.Y();

                auto handleTransform = handle->getComponentPtr<LayoutTransform>();
                if( !handleTransform )
                {
                    break;
                }

                auto handlePosition = handleTransform->getPosition();

                auto anchoredPosition = Vector2<real_Num>( handlePosition.X(), newHandlePosition );
                handleTransform->setPosition( anchoredPosition );
            }

            if( auto fill = getFill() )
            {
                auto backgroundTransform =
                    m_background ? m_background->getComponentPtr<LayoutTransform>() : nullptr;
                if( !backgroundTransform )
                {
                    break;
                }

                auto backgroundSize = backgroundTransform->getSize();

                auto fillTransform = fill->getComponentPtr<LayoutTransform>();
                if( !fillTransform )
                {
                    break;
                }

                auto fillSize = fillTransform->getSize();

                auto sizeDelta = Vector2<real_Num>( fillSize.X(), normalizedValue * backgroundSize.Y() );
                fillTransform->setSize( sizeDelta );
            }
        }
        break;
        default:
        {
        }
        break;
        };
    }

    auto Slider::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
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
                updateSliderPosition();
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
        };

        return FSMReturnType::Ok;
    }

    bool Slider::isDragging() const
    {
        return m_isDragging;
    }

    void Slider::setDragging( bool dragging )
    {
        m_isDragging = dragging;
        updateElementState();
    }

    void Slider::createUI()
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

                auto slider = renderUI->addElementByType<ui::IUISlider>();
                setElement( slider );

                setLabel( "Slider" );

                updateVisibility();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Slider::updateElementState()
    {
        if( auto slider = workphone::dynamic_pointer_cast<ui::IUISlider>( getElement() ) )
        {
            slider->setMinValue( m_minValue );
            slider->setMaxValue( m_maxValue );
            slider->setValue( m_sliderValue );
            slider->setDirection( m_direction );
            slider->setDragging( m_isDragging );
        }
    }

}  // namespace workphone::scene
