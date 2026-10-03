#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsShape2, IPhysicsShape );

    IPhysicsShape2::~IPhysicsShape2() = default;

}  // namespace workphone::physics
