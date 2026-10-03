#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IConstraintD6.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IConstraintD6, IPhysicsConstraint3 );

    IConstraintD6::~IConstraintD6() = default;

}  // namespace workphone::physics
