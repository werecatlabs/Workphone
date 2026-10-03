#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/PhysicsBodyMotionState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, PhysicsBodyMotionState, StateData );

    PhysicsBodyMotionState::PhysicsBodyMotionState() = default;

    PhysicsBodyMotionState::~PhysicsBodyMotionState() = default;

}  // namespace workphone
