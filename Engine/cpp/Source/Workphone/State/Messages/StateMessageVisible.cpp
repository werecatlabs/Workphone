#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessageVisible.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageVisible, StateMessage );

    //---------------------------------------------
    auto StateMessageVisible::isVisible() const -> bool
    {
        return m_isVisible;
    }

    //---------------------------------------------
    void StateMessageVisible::setVisible( bool value )
    {
        m_isVisible = value;
    }

    //---------------------------------------------
    auto StateMessageVisible::getCascade() const -> bool
    {
        return m_cascade;
    }

    //---------------------------------------------
    void StateMessageVisible::setCascade( bool value )
    {
        m_cascade = value;
    }
}  // namespace workphone
