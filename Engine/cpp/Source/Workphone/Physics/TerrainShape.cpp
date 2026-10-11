#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/TerrainShape.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    namespace physics
    {

        WP_CLASS_REGISTER_DERIVED( workphone::physics, TerrainShape, PhysicsShape3<ITerrainShape> );
        WP_CLASS_REGISTER_DERIVED( workphone::physics, TerrainCollisionData, ISharedObject );

        TerrainShape::TerrainShape() = default;
        TerrainShape::~TerrainShape() = default;

        render::TerrainSnapshot TerrainShape::getTerrainSnapshot() const
        {
            return m_terrainSnapshot;
        }

        u64 TerrainShape::getTerrainRevision() const
        {
            return m_terrainSnapshot ? m_terrainSnapshot->revision : 0;
        }

        Vector3<real_Num> TerrainShape::getTerrainScale() const
        {
            return m_terrainScale;
        }

    }  // namespace physics
}  // namespace workphone
