#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsParticle2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsParticle2, ISharedObject );

    IPhysicsParticle2::~IPhysicsParticle2() = default;

}  // namespace workphone::physics
