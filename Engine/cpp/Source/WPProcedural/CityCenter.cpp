#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CityCenter.hpp"
#include "WPProcedural/CRoadNode.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace procedural
    {
        CityCenter::~CityCenter()
        {
        }

        CityCenter::CityCenter()
        {
        }

        Array<SmartPtr<IRoadNode>> CityCenter::getRoadNodesFromCell( const Vector2I &cellIndex )
        {
            Array<SmartPtr<IRoadNode>> roadNodes;
            return roadNodes;
        }

        void CityCenter::setRoadNodeGrid( Grid2 roadNodeGrid )
        {
            m_roadNodeGrid = roadNodeGrid;
        }

        const Grid2 &CityCenter::getRoadNodeGrid() const
        {
            return m_roadNodeGrid;
        }

        Grid2 &CityCenter::getRoadNodeGrid()
        {
            return m_roadNodeGrid;
        }

        void CityCenter::setRadius( f32 radius )
        {
            m_radius = radius;
        }

        f32 CityCenter::getRadius() const
        {
            return m_radius;
        }

        void CityCenter::setRoads( Array<Handle> roads )
        {
            // m_roads = roads;
        }

        const Array<Handle> &CityCenter::getRoads() const
        {
            return m_roads;
        }

        Array<Handle> &CityCenter::getRoads()
        {
            return m_roads;
        }

        void CityCenter::removeRoad( Handle handle )
        {
            //			Array<Handle>::iterator it = std::find(m_roads.begin(), m_roads.end(),
            // handle); 			if ( it != m_roads.end() )
            //			{
            //				m_roads.erase(it);
            //			}
        }

        void CityCenter::addRoad( Handle handle )
        {
            m_roads.push_back( handle );
        }

        bool CityCenter::CityCenterRecord::operator==( const CityCenterRecord &other ) const
        {
            return ( CityLayerA == other.CityLayerA && CityLayerB == other.CityLayerB ) ||
                   ( CityLayerA == other.CityLayerB && CityLayerB == other.CityLayerA );
        }

        bool CityCenter::CityCenterRecord::operator<( const CityCenterRecord &other ) const
        {
            SmartPtr<ICityCenter> values0[2];
            SmartPtr<ICityCenter> values1[2];

            values0[0] = CityLayerA;
            values0[1] = CityLayerB;

            values1[0] = other.CityLayerA;
            values1[1] = other.CityLayerB;

            return memcmp( values0, values1, 2 * sizeof( SmartPtr<ICityCenter> ) ) <= 0;
        }

        bool CityCenter::CityCenterRecord::operator()( const CityCenterRecord &other ) const
        {
            return CityLayerA == other.CityLayerB || CityLayerB == other.CityLayerA;
        }

        CityCenter::CityCenterRecord::CityCenterRecord( SmartPtr<ICityCenter> cityLayerA,
                                                        SmartPtr<ICityCenter> cityLayerB ) :
            CityLayerA( cityLayerA ),
            CityLayerB( cityLayerB )
        {
        }
    }  // namespace procedural
}  // namespace workphone
