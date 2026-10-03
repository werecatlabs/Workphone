#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPlaneShape3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPlaneShape3, IPhysicsShape3 );

    IPlaneShape3::~IPlaneShape3() = default;

}  // namespace workphone::physics
