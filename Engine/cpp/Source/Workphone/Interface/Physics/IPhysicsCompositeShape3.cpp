#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsCompositeShape3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsCompositeShape3, IPhysicsShape3 );

    IPhysicsCompositeShape3::~IPhysicsCompositeShape3() = default;

}  // namespace workphone::physics
