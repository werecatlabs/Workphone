#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessageType.hpp"

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageType, StateMessage );

    StateMessageType::StateMessageType() = default;

    StateMessageType::~StateMessageType() = default;

    auto StateMessageType::getTypeValue() const -> u32
    {
        return m_typeValue;
    }

    void StateMessageType::setTypeValue( u32 typeValue )
    {
        m_typeValue = typeValue;
    }
}  // namespace workphone
