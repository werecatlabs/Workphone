#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIItemTemplate.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    ClawUIItemTemplate::ClawUIItemTemplate()
    {
        setType( String( "GUIItemTemplate" ) );
    }

    ClawUIItemTemplate::~ClawUIItemTemplate()
    {
        removeAllChildren();
    }

    void ClawUIItemTemplate::initialise( SmartPtr<IUIElement> &parent, const TiXmlNode *pNode )
    {
        setParent( parent );
    }

    const Properties &ClawUIItemTemplate::getPropertyGroup() const
    {
        return m_propertyGroup;
    }

    Properties &ClawUIItemTemplate::getPropertyGroup()
    {
        return m_propertyGroup;
    }
}  // namespace workphone::ui
