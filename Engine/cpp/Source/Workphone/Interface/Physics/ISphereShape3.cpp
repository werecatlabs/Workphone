#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/ISphereShape3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, ISphereShape3, IPhysicsShape3 );

    ISphereShape3::~ISphereShape3() = default;

}  // namespace workphone::physics
