#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/PhysicsBodyMassState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, PhysicsBodyMassState, StateData );

    PhysicsBodyMassState::PhysicsBodyMassState() : StateData( PhysicsBodyMassState::typeInfo() )
    {
    }

    PhysicsBodyMassState::~PhysicsBodyMassState() = default;

}  // namespace workphone
