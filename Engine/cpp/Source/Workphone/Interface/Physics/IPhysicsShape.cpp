#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsShape, ISharedObject );

    const hash_type IPhysicsShape::CREATE_SHAPE_HASH = StringUtil::getHash( "createShape" );

    const u32 IPhysicsShape::ShapeFlagReserved = 0 << 0;
    const u32 IPhysicsShape::ShapeFlagEnabled = 1 << 0;
    const u32 IPhysicsShape::ShapeFlagTrigger = 1 << 1;

    IPhysicsShape::~IPhysicsShape() = default;

}  // namespace workphone::physics
