#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageIntValue.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageIntValue, StateMessage );

    auto StateMessageIntValue::getValue() const -> s32
    {
        return m_value;
    }

    void StateMessageIntValue::setValue( s32 value )
    {
        m_value = value;
    }
}  // namespace workphone
