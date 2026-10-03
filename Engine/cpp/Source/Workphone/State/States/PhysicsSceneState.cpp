#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/PhysicsSceneState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, PhysicsSceneState, StateData );

    PhysicsSceneState::PhysicsSceneState() = default;

    PhysicsSceneState::~PhysicsSceneState() = default;

}  // namespace workphone
