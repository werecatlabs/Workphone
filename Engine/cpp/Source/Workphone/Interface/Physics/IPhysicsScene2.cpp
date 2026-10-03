#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsScene2, ISharedObject );

    IPhysicsScene2::~IPhysicsScene2() = default;

}  // namespace workphone::physics
