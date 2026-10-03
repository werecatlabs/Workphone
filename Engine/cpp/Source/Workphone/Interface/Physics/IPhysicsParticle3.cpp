#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsParticle3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsParticle3, ISharedObject );

    IPhysicsParticle3::~IPhysicsParticle3() = default;

}  // namespace workphone::physics
