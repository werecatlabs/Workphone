#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/ToolTip.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ToolTip, Component );

    const String ToolTip::positionStr = String( "position" );
    const String ToolTip::mouseOverTimeStr = String( "mouseOverTime" );
    const String ToolTip::isMouseOverStr = String( "isMouseOver" );
    const String ToolTip::tooltipStr = String( "tooltip" );

    ToolTip::ToolTip()
    {
    }

    ToolTip::~ToolTip()
    {
    }

    void ToolTip::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Component::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void ToolTip::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    Array<SmartPtr<ISharedObject>> ToolTip::getChildObjects() const
    {
        return Component::getChildObjects();
    }

    SmartPtr<Properties> ToolTip::getProperties() const
    {
        auto properties = Component::getProperties();
        if( properties )
        {
            properties->setProperty( positionStr, m_position );
            properties->setProperty( mouseOverTimeStr, m_mouseOverTime );
            properties->setProperty( isMouseOverStr, m_isMouseOver );
            properties->setProperty( tooltipStr, m_tooltip );
        }

        return properties;
    }

    void ToolTip::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );

        if( properties )
        {
            properties->getPropertyValue( positionStr, m_position );
            properties->getPropertyValue( mouseOverTimeStr, m_mouseOverTime );
            properties->getPropertyValue( isMouseOverStr, m_isMouseOver );
            properties->getPropertyValue( tooltipStr, m_tooltip );
        }
    }

    Vector3F ToolTip::getPosition() const
    {
        return m_position;
    }

    void ToolTip::setPosition( const Vector3F &position )
    {
        m_position = position;
    }

    f32 ToolTip::getMouseOverTime() const
    {
        return m_mouseOverTime;
    }

    void ToolTip::setMouseOverTime( f32 mouseOverTime )
    {
        m_mouseOverTime = mouseOverTime;
    }

    bool ToolTip::isMouseOver() const
    {
        return m_isMouseOver;
    }

    void ToolTip::setMouseOver( bool isMouseOver )
    {
        m_isMouseOver = isMouseOver;
    }

    String ToolTip::getTooltip() const
    {
        return m_tooltip;
    }

    void ToolTip::setTooltip( const String &tooltip )
    {
        m_tooltip = tooltip;
    }
}  // namespace workphone::scene
