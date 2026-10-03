#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/ScrollView.hpp>
#include <Workphone/Scene/Components/UI/ScrollBar.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    const String ScrollView::scrollBarStr = String( "scrollBar" );
    const String ScrollView::contentPanelStr = String( "contentPanel" );
    const String ScrollView::lastDragPositionStr = String( "lastDragPosition" );
    const String ScrollView::scrollSpeedStr = String( "scrollSpeed" );
    const String ScrollView::inertiaStr = String( "inertia" );
    const String ScrollView::velocityStr = String( "velocity" );
    const String ScrollView::isDraggingStr = String( "isDragging" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ScrollView, UIComponent );

    ScrollView::ScrollView() = default;
    ScrollView::~ScrollView() = default;

    void ScrollView::load( SmartPtr<ISharedObject> data )
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

    void ScrollView::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        UIComponent::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    Array<SmartPtr<ISharedObject>> ScrollView::getChildObjects() const
    {
        auto children = UIComponent::getChildObjects();
        return children;
    }

    SmartPtr<Properties> ScrollView::getProperties() const
    {
        auto properties = UIComponent::getProperties();

        properties->setPropertyAsType<ScrollBar>( "scrollBar", m_scrollBar );
        properties->setPropertyAsType<LayoutTransform>( "contentPanel", m_contentPanel );
        properties->setProperty( ScrollView::lastDragPositionStr, m_lastDragPosition );
        properties->setProperty( ScrollView::scrollSpeedStr, m_scrollSpeed );
        properties->setProperty( ScrollView::inertiaStr, m_inertia );
        properties->setProperty( ScrollView::velocityStr, m_velocity );
        properties->setProperty( ScrollView::isDraggingStr, m_isDragging );

        return properties;
    }

    void ScrollView::setProperties( SmartPtr<Properties> properties )
    {
        UIComponent::setProperties( properties );

        properties->getPropertyAsType<ScrollBar>( "scrollBar", m_scrollBar );
        properties->getPropertyAsType<LayoutTransform>( "contentPanel", m_contentPanel );
        properties->getPropertyValue( ScrollView::lastDragPositionStr, m_lastDragPosition );
        properties->getPropertyValue( ScrollView::scrollSpeedStr, m_scrollSpeed );
        properties->getPropertyValue( ScrollView::inertiaStr, m_inertia );
        properties->getPropertyValue( ScrollView::velocityStr, m_velocity );
        properties->getPropertyValue( ScrollView::isDraggingStr, m_isDragging );

        if( m_scrollBar )
        {
            m_scrollBar->setScrollView( this );
        }
    }

    bool ScrollView::isValid() const
    {
        return UIComponent::isValid();
    }

    bool ScrollView::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        if( !event )
        {
            return false;
        }

        auto baseHandled = UIComponent::handleEvent( event );

        auto mouseState = event->getMouseState();
        if( !mouseState || !m_contentPanel )
        {
            return baseHandled;
        }

        Vector2<real_Num> localMousePosition;
        auto mouseIsInside = getLocalMousePosition( event, localMousePosition );
        if( !mouseIsInside && !isDragging() )
        {
            return baseHandled;
        }

        switch( mouseState->getEventType() )
        {
        case IMouseState::Event::Wheel:
        {
            handleScroll( event );
            return true;
        }
        case IMouseState::Event::LeftPressed:
        {
            handleBeginDrag( event );
            return true;
        }
        case IMouseState::Event::Moved:
        {
            if( mouseState->isDragging() || isDragging() )
            {
                handleDrag( event );
                return true;
            }
        }
        break;
        case IMouseState::Event::LeftReleased:
        {
            if( isDragging() )
            {
                handleEndDrag( event );
                return true;
            }
        }
        break;
        default:
        {
        }
        break;
        }

        return baseHandled;
    }

    void ScrollView::setDragging( bool dragging )
    {
        m_isDragging = dragging;
    }

    bool ScrollView::isDragging() const
    {
        return m_isDragging;
    }

    void ScrollView::setVelocity( f32 velocity )
    {
        m_velocity = velocity;
    }

    f32 ScrollView::getVelocity() const
    {
        return m_velocity;
    }

    void ScrollView::setLastDragPosition( const Vector2<real_Num> &lastDragPosition )
    {
        m_lastDragPosition = lastDragPosition;
    }

    Vector2<real_Num> ScrollView::getLastDragPosition() const
    {
        return m_lastDragPosition;
    }

    void ScrollView::setInertia( f32 inertia )
    {
        m_inertia = inertia;
    }

    f32 ScrollView::getInertia() const
    {
        return m_inertia;
    }

    void ScrollView::setScrollSpeed( f32 scrollSpeed )
    {
        m_scrollSpeed = scrollSpeed;
    }

    f32 ScrollView::getScrollSpeed() const
    {
        return m_scrollSpeed;
    }

    void ScrollView::setContentPanel( SmartPtr<LayoutTransform> contentPanel )
    {
        m_contentPanel = contentPanel;
    }

    SmartPtr<LayoutTransform> ScrollView::getContentPanel() const
    {
        return m_contentPanel;
    }

    void ScrollView::handleScroll( SmartPtr<IInputEvent> inputEvent )
    {
        auto mouseState = inputEvent->getMouseState();
        auto scrollDelta = mouseState->getWheelDelta();
        auto scrollSpeed = getScrollSpeed();

        auto contentPanel = getContentPanel();
        auto newPos = contentPanel->getPosition();
        newPos.y += scrollDelta.y * scrollSpeed;
        contentPanel->setPosition( clampToBounds( newPos ) );

        updateScrollBar();
    }

    Vector2<real_Num> ScrollView::clampToBounds( const Vector2<real_Num> &pos )
    {
        if( !m_contentPanel )
        {
            return pos;
        }

        auto actor = getActor();
        if( !actor )
        {
            return pos;
        }

        auto layoutTransform = actor->getComponent<LayoutTransform>();
        if( !layoutTransform )
        {
            return pos;
        }

        auto contentHeight = m_contentPanel->getSize().Y();
        auto maskHeight = layoutTransform->getSize().Y();

        auto minY = MathF::min( static_cast<real_Num>( 0.0 ), maskHeight - contentHeight );
        auto maxY = 0.0f;

        return Vector2<real_Num>( pos.X(), MathF::clamp( pos.Y(), minY, maxY ) );
    }

    void ScrollView::handleEndDrag( SmartPtr<IInputEvent> inputEvent )
    {
        auto mouseState = inputEvent->getMouseState();
        auto position = mouseState->getAbsolutePosition();

        m_isDragging = false;
        m_velocity = ( position.y - m_lastDragPosition.y ) * m_scrollSpeed;
        updateScrollBar();
    }

    void ScrollView::handleDrag( SmartPtr<IInputEvent> inputEvent )
    {
        auto contentPanel = getContentPanel();
        if( !contentPanel )
        {
            return;
        }

        auto mouseState = inputEvent->getMouseState();
        auto position = mouseState->getAbsolutePosition();

        auto delta = position - m_lastDragPosition;
        auto currentPos = contentPanel->getPosition();
        auto newPos = currentPos + Vector2<real_Num>( 0, delta.y );
        contentPanel->setPosition( clampToBounds( newPos ) );
        m_lastDragPosition = position;
        updateScrollBar();
    }

    void ScrollView::handleBeginDrag( SmartPtr<IInputEvent> inputEvent )
    {
        auto mouseState = inputEvent->getMouseState();

        m_isDragging = true;
        m_lastDragPosition = mouseState->getAbsolutePosition();
        m_velocity = 0;  // Stop momentum when dragging starts
    }

    void ScrollView::setScrollBar( SmartPtr<ScrollBar> scrollBar )
    {
        m_scrollBar = scrollBar;
        if( m_scrollBar )
        {
            m_scrollBar->setScrollView( this );
        }
    }

    SmartPtr<ScrollBar> ScrollView::getScrollBar() const
    {
        return m_scrollBar;
    }

    void ScrollView::syncWithScrollBar()
    {
        if( !m_contentPanel || !m_scrollBar )
        {
            return;
        }

        if( auto actor = getActorPtr() )
        {
            auto contentSize = m_contentPanel->getSize();

            auto layoutTransform = actor->getComponentPtr<LayoutTransform>();
            if( !layoutTransform )
            {
                return;
            }

            auto visibleSize = layoutTransform->getSize();

            auto maxScroll = MathF::max( contentSize.y - visibleSize.y, static_cast<real_Num>( 0.0 ) );
            auto scrollValue = m_scrollBar->getScrollValue();

            Vector2<real_Num> newPos( 0.0, -scrollValue * maxScroll );
            m_contentPanel->setPosition( clampToBounds( newPos ) );
        }
    }

    void ScrollView::updateScrollBar()
    {
        if( !m_contentPanel || !m_scrollBar )
        {
            return;
        }

        auto actor = getActorPtr();
        if( !actor )
        {
            return;
        }

        auto contentSize = m_contentPanel->getSize().Y();
        auto layoutTransform = actor->getComponent<LayoutTransform>();
        if( !layoutTransform )
        {
            return;
        }

        auto visibleSize = layoutTransform->getSize();

        if( contentSize <= visibleSize.Y() )
        {
            m_scrollBar->setEnabled( false );
            return;
        }

        m_scrollBar->setEnabled( true );

        auto scrollRatio = visibleSize.Y() / contentSize;
        auto handleHeight = visibleSize.Y() * scrollRatio;

        auto scrollPos = -m_contentPanel->getPosition().Y();
        auto maxScroll = contentSize - visibleSize.Y();

        auto scrollValue = maxScroll > 0.0f ? MathF::clamp01( scrollPos / maxScroll ) : 0.0f;
        m_scrollBar->setHandleSize( handleHeight );
        m_scrollBar->setScrollValue( scrollValue );
    }

}  // namespace workphone::scene
