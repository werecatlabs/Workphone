#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageStringValue.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageStringValue, StateMessage );

    StateMessageStringValue::StateMessageStringValue() = default;

    StateMessageStringValue::~StateMessageStringValue() = default;

    void StateMessageStringValue::setValue( const String &value )
    {
        m_value = value;
    }

    auto StateMessageStringValue::getValue() const -> String
    {
        return m_value;
    }
}  // namespace workphone
