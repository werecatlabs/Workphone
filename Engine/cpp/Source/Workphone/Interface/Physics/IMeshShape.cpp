#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IMeshShape.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IMeshShape, IPhysicsShape3 );

    IMeshShape::~IMeshShape() = default;

}  // namespace workphone::physics
