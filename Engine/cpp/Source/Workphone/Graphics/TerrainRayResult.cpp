#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/TerrainRayResult.hpp>

namespace workphone::render
{
    TerrainRayResult::TerrainRayResult() = default;

    TerrainRayResult::~TerrainRayResult() = default;

    bool TerrainRayResult::hasIntersected() const
    {
        return m_intersected;
    }

    void TerrainRayResult::setIntersected( bool intersected )
    {
        m_intersected = intersected;
    }

    SmartPtr<IGraphicsTerrain> TerrainRayResult::getTerrain() const
    {
        return m_terrain;
    }

    void TerrainRayResult::setTerrain( SmartPtr<IGraphicsTerrain> terrain )
    {
        m_terrain = terrain;
    }

    Vector3<real_Num> TerrainRayResult::getPosition() const
    {
        return m_position;
    }

    void TerrainRayResult::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
    }

}  // namespace workphone::render
