#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessageParamVector4.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageParamVector4, StateMessage );

    StateMessageParamVector4::StateMessageParamVector4() = default;

    StateMessageParamVector4::~StateMessageParamVector4() = default;

    auto StateMessageParamVector4::getId() const -> hash32
    {
        return m_id;
    }

    void StateMessageParamVector4::setId( hash32 value )
    {
        m_id = value;
    }

    auto StateMessageParamVector4::getValue() const -> Vector4F
    {
        return m_value;
    }

    void StateMessageParamVector4::setValue( const Vector4F &value )
    {
        m_value = value;
    }
}  // namespace workphone
