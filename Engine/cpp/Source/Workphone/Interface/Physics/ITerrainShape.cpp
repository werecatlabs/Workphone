#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/ITerrainShape.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, ITerrainShape, IPhysicsShape3 );

    ITerrainShape::~ITerrainShape() = default;

}  // namespace workphone::physics
