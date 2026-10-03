#ifndef ProceduralCityCenter_h__
#define ProceduralCityCenter_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/CProceduralObject.hpp>
#include <Workphone/Core/Grid2.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Interface/Procedural/IProceduralCityCenter.hpp>

namespace workphone
{
    namespace procedural
    {

        class WPProcedural_API CityCenter  //: public CProceduralObject
        {
        public:
            class CityCenterRecord : public ISharedObject
            {
            public:
                CityCenterRecord( SmartPtr<ICityCenter> cityLayerA, SmartPtr<ICityCenter> cityLayerB );

                bool operator()( const CityCenterRecord &other ) const;
                bool operator<( const CityCenterRecord &other ) const;
                bool operator==( const CityCenterRecord &other ) const;

                SmartPtr<ICityCenter> CityLayerA;
                SmartPtr<ICityCenter> CityLayerB;
            };

            CityCenter();
            ~CityCenter();

            void addRoad( Handle handle );

            void removeRoad( Handle handle );

            Array<Handle> &getRoads();
            const Array<Handle> &getRoads() const;
            void setRoads( Array<Handle> roads );

            f32 getRadius() const;
            void setRadius( f32 radius );

            Grid2 &getRoadNodeGrid();
            const Grid2 &getRoadNodeGrid() const;
            void setRoadNodeGrid( Grid2 roadNodeGrid );

            Array<SmartPtr<IRoadNode>> getRoadNodesFromCell( const Vector2I &cellIndex );

        protected:
            Array<Handle> m_roads;
            Grid2 m_roadNodeGrid;
            f32 m_radius;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // ProceduralCityCenter_h__
