#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CProceduralScene.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace procedural
    {
        CProceduralScene::CProceduralScene() = default;

        CProceduralScene::~CProceduralScene() = default;

        void CProceduralScene::addCity( SmartPtr<IProceduralCity> city )
        {
            m_cities.push_back( city );
        }

        void CProceduralScene::removeCity( SmartPtr<IProceduralCity> city )
        {
            auto it = std::find( m_cities.begin(), m_cities.end(), city );
            if( it != m_cities.end() )
            {
                m_cities.erase( it );
            }
        }

        Array<SmartPtr<IProceduralCity>> CProceduralScene::getCities() const
        {
            return m_cities;
        }

        void CProceduralScene::setCities( Array<SmartPtr<IProceduralCity>> cities )
        {
            m_cities = cities;
        }

        void CProceduralScene::addTerrain( SmartPtr<IProceduralTerrain> terrain )
        {
            m_terrains.push_back( terrain );
        }

        void CProceduralScene::removeTerrain( SmartPtr<IProceduralTerrain> terrain )
        {
            auto it = std::find( m_terrains.begin(), m_terrains.end(), terrain );
            if( it != m_terrains.end() )
            {
                m_terrains.erase( it );
            }
        }

        Array<SmartPtr<IProceduralTerrain>> CProceduralScene::getTerrains() const
        {
            return m_terrains;
        }

        void CProceduralScene::setTerrains( Array<SmartPtr<IProceduralTerrain>> terrains )
        {
            m_terrains = terrains;
        }
    }  // namespace procedural
}  // namespace workphone
