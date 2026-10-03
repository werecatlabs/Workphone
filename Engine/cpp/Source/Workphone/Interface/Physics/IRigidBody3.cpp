#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IRigidBody3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IRigidBody3, IPhysicsBody3 );

    IRigidBody3::~IRigidBody3() = default;

}  // namespace workphone::physics
