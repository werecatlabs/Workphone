#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IBoxShape2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IBoxShape2, IPhysicsShape2 );

    IBoxShape2::~IBoxShape2() = default;

}  // namespace workphone::physics
