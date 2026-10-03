#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IConstraintFixed2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, IConstraintFixed2, ISharedObject );

    IConstraintFixed2::~IConstraintFixed2() = default;

}  // namespace workphone::physics
