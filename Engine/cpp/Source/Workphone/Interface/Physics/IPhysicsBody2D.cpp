#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody2D.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsBody2D, ISharedObject );

    IPhysicsBody2D::~IPhysicsBody2D() = default;

}  // namespace workphone::physics
