#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StatePhysicsForce2.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StatePhysicsForce2, StateMessage );

    const hash_type StatePhysicsForce2::ADD_FORCE_HASH = StringUtil::getHash( "addForce" );
    const hash_type StatePhysicsForce2::SET_FORCE_HASH = StringUtil::getHash( "setForce" );

    StatePhysicsForce2::StatePhysicsForce2() = default;

    auto StatePhysicsForce2::getForce() const -> Vector2<real_Num>
    {
        return m_force;
    }

    void StatePhysicsForce2::setForce( const Vector2<real_Num> &value )
    {
        m_force = value;
    }

    auto StatePhysicsForce2::getRelativePosition() const -> Vector2<real_Num>
    {
        return m_relativePosition;
    }

    void StatePhysicsForce2::setRelativePosition( const Vector2<real_Num> &value )
    {
        m_relativePosition = value;
    }
}  // namespace workphone
