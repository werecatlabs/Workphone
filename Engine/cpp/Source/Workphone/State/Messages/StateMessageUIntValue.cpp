#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageUIntValue.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageUIntValue, StateMessage );

    //--------------------------------------------
    auto StateMessageUIntValue::getValue() const -> u32
    {
        return m_value;
    }

    //--------------------------------------------
    void StateMessageUIntValue::setValue( u32 value )
    {
        m_value = value;
    }
}  // namespace workphone
