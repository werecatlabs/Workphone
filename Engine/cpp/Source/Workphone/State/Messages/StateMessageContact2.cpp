#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessageContact2.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageContact2, StateMessage );

    StateMessageContact2::StateMessageContact2() = default;

    StateMessageContact2::~StateMessageContact2() = default;

    auto StateMessageContact2::getContactType() const -> hash32
    {
        return m_contactType;
    }

    void StateMessageContact2::setContactType( hash32 value )
    {
        m_contactType = value;
    }

    auto StateMessageContact2::getPosition() const -> Vector2<real_Num>
    {
        return m_position;
    }

    void StateMessageContact2::setPosition( const Vector2<real_Num> &value )
    {
        m_position = value;
    }

    auto StateMessageContact2::getNormal() const -> Vector2<real_Num>
    {
        return m_normal;
    }

    void StateMessageContact2::setNormal( const Vector2<real_Num> &value )
    {
        m_normal = value;
    }
}  // namespace workphone
