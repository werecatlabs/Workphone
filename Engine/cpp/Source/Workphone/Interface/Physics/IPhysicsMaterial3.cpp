#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsMaterial3, ISharedObject );

    IPhysicsMaterial3::~IPhysicsMaterial3() = default;

}  // namespace workphone::physics
