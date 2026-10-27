#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CityCenter.hpp"
#include "WPProcedural/CRoadNode.hpp"
#include <Workphone/Workphone.hpp>
#include <algorithm>

namespace workphone
{
    namespace procedural
    {
        CityCenter::~CityCenter()
        {
            unload( nullptr );
        }

        void CityCenter::unload( SmartPtr<ISharedObject> data )
        {
            // Detach derived storage before base node callbacks can reenter.
            auto roads = std::move( m_roads );
            CProceduralObject<IProceduralCityCenter>::unload( data );
        }

        CityCenter::CityCenter()
        {
        }

        Array<SmartPtr<IRoadNode>> CityCenter::getRoadNodesFromCell( const Vector2I &cellIndex )
        {
            WP_UNUSED( cellIndex );
            // A spatial road-node index is not implemented by this class.
            return makeSourceBudgetSnapshot<SmartPtr<IRoadNode>>();
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
            if( roads.get_allocator().isSourceBudgeted() )
            {
                m_roads.swap( roads );
                return;
            }
            auto candidate = makeSourceBudgetSnapshot( roads );
            m_roads.swap( candidate );
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
            const auto it = std::find( m_roads.begin(), m_roads.end(), handle );
            if( it == m_roads.end() ) return;

            // Handle keeps its existing hash-only equality. Remove the first
            // matching entry and retain spare capacity, staging before mutation.
            auto candidate = makeSourceBudgetSnapshot<Handle>();
            candidate.reserve( m_roads.capacity() );
            for( auto current = m_roads.begin(); current != m_roads.end(); ++current )
            {
                if( current != it ) candidate.push_back( *current );
            }
            m_roads.swap( candidate );
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
