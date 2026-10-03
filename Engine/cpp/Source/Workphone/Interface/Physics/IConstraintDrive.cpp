#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IConstraintDrive.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, IConstraintDrive, IPhysicsSpring );

    IConstraintDrive::~IConstraintDrive() = default;

}  // namespace workphone::physics
