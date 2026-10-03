#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsSpring.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsSpring, ISharedObject );

    IPhysicsSpring::~IPhysicsSpring() = default;

}  // namespace workphone::physics
