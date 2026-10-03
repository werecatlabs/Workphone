#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/ConstraintStateData.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ConstraintStateData, StateData );

    ConstraintStateData::ConstraintStateData() = default;

    ConstraintStateData::~ConstraintStateData() = default;

}  // namespace workphone
