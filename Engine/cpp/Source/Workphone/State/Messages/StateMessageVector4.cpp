#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageVector4.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageVector4, StateMessage );

    StateMessageVector4::StateMessageVector4() = default;

    StateMessageVector4::StateMessageVector4( const Vector4F &position ) : m_position( position )
    {
    }

    StateMessageVector4::~StateMessageVector4() = default;

    auto StateMessageVector4::getValue() const -> Vector4F
    {
        return m_position;
    }

    void StateMessageVector4::setValue( const Vector4F &value )
    {
        m_position = value;
    }
}  // namespace workphone
