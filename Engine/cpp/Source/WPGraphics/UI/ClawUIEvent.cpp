#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIEvent.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    ClawUIEvent::ClawUIEvent()
    {
        setType( "Event" );
    }

    ClawUIEvent::~ClawUIEvent() = default;

    void ClawUIEvent::initialise( SmartPtr<IUIElement> &parent, const TiXmlNode *pNode )
    {
        setParent( parent );
    }

    void ClawUIEvent::setEventType( const String &eventType )
    {
        m_eventType = eventType;
    }

    const String &ClawUIEvent::getEventType() const
    {
        return m_eventType;
    }

    void ClawUIEvent::OnActivateCallback()
    {
        onActivate( this );
    }

    void ClawUIEvent::OnSelectCallback()
    {
        onSelect();
    }

    void ClawUIEvent::OnDeselectCallback()
    {
        onDeselect();
    }
}  // namespace workphone::ui
