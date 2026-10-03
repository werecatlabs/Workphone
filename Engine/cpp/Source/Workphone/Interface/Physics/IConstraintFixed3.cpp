#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IConstraintFixed3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, IConstraintFixed3, IPhysicsConstraint3 );

    IConstraintFixed3::~IConstraintFixed3() = default;

}  // namespace workphone::physics
