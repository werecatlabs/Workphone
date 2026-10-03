#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsBodyEffect2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsBodyEffect2, IPhysicsEffect2 );

    IPhysicsBodyEffect2::~IPhysicsBodyEffect2() = default;

}  // namespace workphone::physics
