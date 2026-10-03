#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/PhysicsBodyState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, PhysicsBodyState, StateData );

    PhysicsBodyState::PhysicsBodyState() : StateData( PhysicsBodyState::typeInfo() )
    {
    }

    PhysicsBodyState::~PhysicsBodyState() = default;
}  // namespace workphone
