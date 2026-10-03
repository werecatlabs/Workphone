#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IRigidBody2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IRigidBody2, IPhysicsBody2D );

    IRigidBody2::~IRigidBody2() = default;

}  // namespace workphone::physics
