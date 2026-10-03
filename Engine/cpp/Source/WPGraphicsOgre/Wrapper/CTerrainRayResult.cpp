#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CTerrainRayResult.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>

namespace workphone
{
    namespace render
    {
        CTerrainRayResult::CTerrainRayResult() : m_isIntersected( false )
        {
        }

        CTerrainRayResult::~CTerrainRayResult()
        {
        }

        bool CTerrainRayResult::hasIntersected() const
        {
            return m_isIntersected;
        }

        void CTerrainRayResult::setIntersected( bool intersected )
        {
            m_isIntersected = intersected;
        }

        SmartPtr<IGraphicsTerrain> CTerrainRayResult::getTerrain() const
        {
            return m_terrain;
        }

        void CTerrainRayResult::setTerrain( SmartPtr<IGraphicsTerrain> terrain )
        {
            m_terrain = terrain;
        }

        Vector3F CTerrainRayResult::getPosition() const
        {
            return m_position;
        }

        void CTerrainRayResult::setPosition( const Vector3F &position )
        {
            m_position = position;
        }
    }  // namespace render
}  // namespace workphone
