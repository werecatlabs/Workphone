#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/PhysicsMaterialStateData.hpp>
#include <Workphone/Interface/Physics/IRigidBody3.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, PhysicsMaterialStateData, StateData );

    PhysicsMaterialStateData::PhysicsMaterialStateData() = default;

    PhysicsMaterialStateData::~PhysicsMaterialStateData() = default;

}  // namespace workphone
