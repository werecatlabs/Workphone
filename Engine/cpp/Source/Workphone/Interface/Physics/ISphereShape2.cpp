#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/ISphereShape2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, ISphereShape2, IPhysicsShape );

    ISphereShape2::~ISphereShape2() = default;

}  // namespace workphone::physics
