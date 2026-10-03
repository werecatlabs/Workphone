#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StatePhysicsVelocity2.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StatePhysicsVelocity2, StateMessage );

    const hash_type StatePhysicsVelocity2::ADD_VELOCITY_HASH = StringUtil::getHash( "addVelocity" );
    const hash_type StatePhysicsVelocity2::SET_VELOCITY_HASH = StringUtil::getHash( "setVelocity" );

    StatePhysicsVelocity2::StatePhysicsVelocity2( const Vector2<real_Num> &velocity ) :
        m_velocity( velocity )
    {
    }

    StatePhysicsVelocity2::StatePhysicsVelocity2() = default;
    StatePhysicsVelocity2::~StatePhysicsVelocity2() = default;

    auto StatePhysicsVelocity2::getVelocity() const -> Vector2<real_Num>
    {
        return m_velocity;
    }

    void StatePhysicsVelocity2::setVelocity( const Vector2<real_Num> &velocity )
    {
        m_velocity = velocity;
    }

    auto StatePhysicsVelocity2::getRelativePosition() const -> Vector2<real_Num>
    {
        return m_relativePosition;
    }

    void StatePhysicsVelocity2::setRelativePosition( const Vector2<real_Num> &relativePosition )
    {
        m_relativePosition = relativePosition;
    }
}  // namespace workphone
