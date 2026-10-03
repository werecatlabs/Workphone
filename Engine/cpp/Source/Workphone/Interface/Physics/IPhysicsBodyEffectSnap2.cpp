#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsBodyEffectSnap2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsBodyEffectSnap2, IPhysicsBodyEffect2 );

    IPhysicsBodyEffectSnap2::~IPhysicsBodyEffectSnap2() = default;

}  // namespace workphone::physics
