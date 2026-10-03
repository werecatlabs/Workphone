#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsConstraint3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsConstraint3, IPhysicsConstraint );

    IPhysicsConstraint3::~IPhysicsConstraint3() = default;

}  // namespace workphone::physics
