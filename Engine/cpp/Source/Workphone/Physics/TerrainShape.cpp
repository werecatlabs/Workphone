#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/TerrainShape.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    namespace physics
    {

        WP_CLASS_REGISTER_DERIVED( workphone::physics, TerrainShape, PhysicsShape3<ITerrainShape> );

        TerrainShape::TerrainShape() = default;
        TerrainShape::~TerrainShape() = default;

    }  // namespace physics
}  // namespace workphone
