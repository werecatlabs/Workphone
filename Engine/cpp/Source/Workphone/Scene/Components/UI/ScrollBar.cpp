#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/ScrollBar.hpp>
#include <Workphone/Scene/Components/UI/ScrollView.hpp>
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
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ScrollBar, UIComponent );

    ScrollBar::ScrollBar() = default;

    ScrollBar::~ScrollBar()
    {
    }

    void ScrollBar::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        UIComponent::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void ScrollBar::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        UIComponent::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<Properties> ScrollBar::getProperties() const
    {
        auto props = UIComponent::getProperties();

        props->setProperty( UiUtil::handleStr, m_handleActor );
        props->setProperty( UiUtil::backgroundStr, m_background );
        props->setProperty( UiUtil::fillStr, m_fill );
        props->setProperty( UiUtil::scrollValueStr, m_scrollValue );

        props->setPropertyAsEnum( UiUtil::directionStr, (u32)m_direction, UiUtil::directionNames );

        return props;
    }

    void ScrollBar::setProperties( SmartPtr<Properties> props )
    {
        UIComponent::setProperties( props );

        props->getPropertyValue( UiUtil::handleStr, m_handleActor );
        props->getPropertyValue( UiUtil::backgroundStr, m_background );
        props->getPropertyValue( UiUtil::fillStr, m_fill );
        props->getPropertyValue( UiUtil::scrollValueStr, m_scrollValue );
        props->getPropertyValue( UiUtil::directionStr, (u32 &)m_direction );

        m_scrollValue = MathF::clamp01( m_scrollValue );

        updateHandlePosition();
    }

    bool ScrollBar::handleEvent( const SmartPtr<IInputEvent> &event )
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
                            f32 newValue = MathF::clamp01( axisPos / axisSize );
                            if( !MathF::equals( m_scrollValue, newValue ) )
                            {
                                setScrollValue( newValue );

                                if( m_callbackFunction )
                                {
                                    m_callbackFunction( IEvent::handleValueChanged );
                                }

                                if( m_scrollView )
                                {
                                    m_scrollView->syncWithScrollBar();
                                }

                                return true;
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    void ScrollBar::setCallbackFunction( std::function<void( hash_type )> callbackFunction )
    {
        m_callbackFunction = callbackFunction;
    }

    void ScrollBar::setFill( SmartPtr<IGameActor> fill )
    {
        m_fill = fill;
    }

    SmartPtr<IGameActor> ScrollBar::getFill() const
    {
        return m_fill;
    }

    void ScrollBar::setHandleActor( SmartPtr<IGameActor> handle )
    {
        m_handleActor = handle;
    }

    SmartPtr<IGameActor> ScrollBar::getHandleActor() const
    {
        return m_handleActor;
    }

    void ScrollBar::setBackground( SmartPtr<IGameActor> background )
    {
        m_background = background;
    }

    SmartPtr<IGameActor> ScrollBar::getBackground() const
    {
        return m_background;
    }

    void ScrollBar::setScrollValue( f32 value )
    {
        m_scrollValue = MathF::clamp01( value );
        updateHandlePosition();
    }

    void ScrollBar::updateHandlePosition()
    {
        if( !m_background )
        {
            return;
        }

        const auto scrollValue = getScrollValue();
        const auto direction = getDirection();

        switch( direction )
        {
        case Direction::Horizontal:
        {
            if( auto handle = getHandleActor() )
            {
                auto backgroundSize = Vector2<real_Num>::unit();
                if( auto backgroundTransform = m_background->getComponent<LayoutTransform>() )
                {
                    backgroundSize = backgroundTransform->getSize();
                }

                auto handleSize = MathF::clamp( m_handleSize, 0.0f, backgroundSize.X() );
                auto travel =
                    MathF::max( backgroundSize.X() - handleSize, static_cast<real_Num>( 0.0 ) );
                auto newHandlePosition = scrollValue * travel;

                if( auto handleTransform = handle->getComponent<LayoutTransform>() )
                {
                    auto handlePos = handleTransform->getPosition();
                    auto anchoredPosition = Vector2<real_Num>( newHandlePosition, handlePos.Y() );

                    handleTransform->setPosition( anchoredPosition );
                    if( handleSize > 0.0f )
                    {
                        auto size = handleTransform->getSize();
                        handleTransform->setSize( Vector2<real_Num>( handleSize, size.Y() ) );
                    }
                }
            }

            if( auto fill = getFill() )
            {
                auto backgroundSize = Vector2<real_Num>::unit();
                if( auto backgroundTransform = m_background->getComponentPtr<LayoutTransform>() )
                {
                    backgroundSize = backgroundTransform->getSize();
                }

                if( auto fillTransform = fill->getComponentPtr<LayoutTransform>() )
                {
                    auto fillSize = fillTransform->getSize();

                    auto sizeDelta = Vector2<real_Num>( scrollValue * backgroundSize.X(), fillSize.Y() );
                    fillTransform->setSize( sizeDelta );
                }
            }
        }
        break;
        case Direction::Vertical:
        {
            if( auto handle = getHandleActor() )
            {
                auto backgroundSize = Vector2<real_Num>::unit();
                if( auto backgroundTransform = m_background->getComponent<LayoutTransform>() )
                {
                    backgroundSize = backgroundTransform->getSize();
                }

                auto handleSize = MathF::clamp( m_handleSize, 0.0f, backgroundSize.Y() );
                auto travel =
                    MathF::max( backgroundSize.Y() - handleSize, static_cast<real_Num>( 0.0 ) );
                auto newHandlePosition = scrollValue * travel;

                if( auto handleTransform = handle->getComponent<LayoutTransform>() )
                {
                    auto handlePos = handleTransform->getPosition();
                    auto anchoredPosition = Vector2<real_Num>( handlePos.X(), newHandlePosition );

                    handleTransform->setPosition( anchoredPosition );
                    if( handleSize > 0.0f )
                    {
                        auto size = handleTransform->getSize();
                        handleTransform->setSize( Vector2<real_Num>( size.X(), handleSize ) );
                    }
                }
            }

            if( auto fill = getFill() )
            {
                auto backgroundSize = Vector2<real_Num>::unit();
                if( auto backgroundTransform = m_background->getComponentPtr<LayoutTransform>() )
                {
                    backgroundSize = backgroundTransform->getSize();
                }

                if( auto fillTransform = fill->getComponentPtr<LayoutTransform>() )
                {
                    auto fillSize = fillTransform->getSize();

                    auto sizeDelta = Vector2<real_Num>( fillSize.X(), scrollValue * backgroundSize.Y() );
                    fillTransform->setSize( sizeDelta );
                }
            }
        }
        break;
        default:
        {
        }
        break;
        };
    }

    auto ScrollBar::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        Component::handleComponentEvent( state, eventType );

        switch( eventType )
        {
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            if( eState == State::Edit || eState == State::Play )
            {
                updateHandlePosition();
            }
        }
        break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    void ScrollBar::setHandleSize( f32 handleSize )
    {
        m_handleSize = MathF::max( handleSize, 0.0f );
        updateHandlePosition();
    }

    void ScrollBar::setDirection( Direction direction )
    {
        m_direction = direction;
        updateHandlePosition();
    }

    ScrollView *ScrollBar::getScrollView() const
    {
        return m_scrollView;
    }

    void ScrollBar::setScrollView( ScrollView *scrollView )
    {
        m_scrollView = scrollView;
    }

}  // namespace workphone::scene
