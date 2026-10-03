#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsConstraint2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsConstraint2, IPhysicsConstraint );

    IPhysicsConstraint2::~IPhysicsConstraint2() = default;

}  // namespace workphone::physics
