#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageVector3.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageVector3, StateMessage );

    StateMessageVector3::StateMessageVector3() = default;

    StateMessageVector3::StateMessageVector3( const Vector3<real_Num> &value ) : m_value( value )
    {
    }

    StateMessageVector3::~StateMessageVector3() = default;

    auto StateMessageVector3::getValue() const -> Vector3<real_Num>
    {
        return m_value;
    }

    void StateMessageVector3::setValue( const Vector3<real_Num> &value )
    {
        m_value = value;
    }
}  // namespace workphone
