#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsShape3, IPhysicsShape );

    IPhysicsShape3::~IPhysicsShape3() = default;

}  // namespace workphone::physics
