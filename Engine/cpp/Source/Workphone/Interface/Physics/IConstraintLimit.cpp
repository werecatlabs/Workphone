#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IConstraintLimit.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, IConstraintLimit, ISharedObject );

    IConstraintLimit::~IConstraintLimit() = default;

}  // namespace workphone::physics
