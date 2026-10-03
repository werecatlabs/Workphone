#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessageProperties.hpp"
#include "Workphone/Core/Properties.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageProperties, StateMessage );

    StateMessageProperties::StateMessageProperties() = default;

    StateMessageProperties::~StateMessageProperties() = default;

    auto StateMessageProperties::getProperties() const -> SmartPtr<Properties>
    {
        return m_properties;
    }

    void StateMessageProperties::setProperties( SmartPtr<Properties> properties )
    {
        m_properties = properties;
    }
}  // namespace workphone
