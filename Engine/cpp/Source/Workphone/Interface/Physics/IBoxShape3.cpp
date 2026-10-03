#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IBoxShape3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IBoxShape3, IPhysicsShape3 );

    IBoxShape3::~IBoxShape3() = default;

}  // namespace workphone::physics
