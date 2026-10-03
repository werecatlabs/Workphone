#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager2D.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsManager2D, ISharedObject );

    IPhysicsManager2D::~IPhysicsManager2D() = default;

}  // namespace workphone::physics
