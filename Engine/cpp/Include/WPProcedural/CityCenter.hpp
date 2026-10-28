#ifndef ProceduralCityCenter_h__
#define ProceduralCityCenter_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/CProceduralObject.hpp>
#include <Workphone/Core/Grid2.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Interface/Procedural/IProceduralCityCenter.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>

namespace workphone
{
    namespace procedural
    {

        class WPProcedural_API CityCenter : public CProceduralObject<IProceduralCityCenter>
        {
        public:
            using SourceBudgetObject::operator new;
            using SourceBudgetObject::operator delete;

            class CityCenterRecord : public ISharedObject, public SourceBudgetObject
            {
            public:
                using SourceBudgetObject::operator new;
                using SourceBudgetObject::operator delete;

                CityCenterRecord( SmartPtr<ICityCenter> cityLayerA, SmartPtr<ICityCenter> cityLayerB );

                bool operator()( const CityCenterRecord &other ) const;
                bool operator<( const CityCenterRecord &other ) const;
                bool operator==( const CityCenterRecord &other ) const;

                SmartPtr<ICityCenter> CityLayerA;
                SmartPtr<ICityCenter> CityLayerB;
            };

            CityCenter();
            ~CityCenter() override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void addRoad( Handle handle );

            void removeRoad( Handle handle );

            // Compatibility reference: explicit replacement can substitute a
            // caller allocator. setRoads restores SourceCpu admission policy.
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
            Array<Handle> m_roads{ Allocator<Handle>( sourceBudget ) };
            Grid2 m_roadNodeGrid;
            f32 m_radius = 0;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // ProceduralCityCenter_h__
