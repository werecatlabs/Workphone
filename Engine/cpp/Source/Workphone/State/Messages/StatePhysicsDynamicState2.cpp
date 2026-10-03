#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StatePhysicsDynamicState2.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StatePhysicsDynamicState2, StateMessage );

    auto StatePhysicsDynamicState2::getPosition() const -> Vector2<real_Num>
    {
        return m_position;
    }

    void StatePhysicsDynamicState2::setPosition( const Vector2<real_Num> &value )
    {
        m_position = value;
    }

    auto StatePhysicsDynamicState2::getVelocity() const -> Vector2<real_Num>
    {
        return m_velocity;
    }

    void StatePhysicsDynamicState2::setVelocity( const Vector2<real_Num> &value )
    {
        m_velocity = value;
    }

    auto StatePhysicsDynamicState2::getForce() const -> Vector2<real_Num>
    {
        return m_force;
    }

    void StatePhysicsDynamicState2::setForce( const Vector2<real_Num> &value )
    {
        m_force = value;
    }

    auto StatePhysicsDynamicState2::getAngularVelocity() const -> real_Num
    {
        return m_angularVelocity;
    }

    void StatePhysicsDynamicState2::setAngularVelocity( real_Num value )
    {
        m_angularVelocity = value;
    }

    auto StatePhysicsDynamicState2::getOrientation() const -> real_Num
    {
        return m_orientation;
    }

    void StatePhysicsDynamicState2::setOrientation( real_Num value )
    {
        m_orientation = value;
    }
}  // namespace workphone
