#pragma once
#include "ProceduralHelperBodyBudgetContracts.hpp"
#include <WPProcedural/WPProcedural.hpp>
#include <WPProcedural/CProceduralTexture.hpp>
#include <WPProcedural/CRoadConnection.hpp>
#include <WPProcedural/CRoadConnectionData.hpp>
#include <WPProcedural/CRoadHitPoint.hpp>
#include <WPProcedural/CRoadMeshElement.hpp>
#include <WPProcedural/ProceduralComponent.hpp>
#include <WPProcedural/PlanarGraph.hpp>
#include <WPProcedural/RoadGeneratorUtil.hpp>
#include <WPProcedural/CVehicleAppearance.hpp>
#include <WPProcedural/CVehicleDamage.hpp>
#include <WPProcedural/CVehicleDynamics.hpp>
#include <WPProcedural/CVehicleEffects.hpp>
#include <WPProcedural/CVehicleGenerator.hpp>
#include <WPProcedural/CVehicleGeometry.hpp>
#include <WPProcedural/CVehiclePhysics.hpp>
#include <WPProcedural/CVehiclePresentation.hpp>

namespace procedural_object_body_budget_contracts
{
    using namespace procedural_helper_body_budget_contracts;

    // Isolate physical object storage from constructor-owned reference metadata,
    // properties and payloads, which have their own independent admissions.
    template<class T> inline void body( Budget &budget )
    {
        static_assert( std::is_convertible_v<T *, SourceBudgetObject *>,
                       "Procedural owners must have one unambiguous admission base" );
        const auto before = budget.used();
        const auto bytes = sourceBudgetStorageBytes( sizeof(T), alignof(std::max_align_t) );
        budget.set(before + bytes - 1);
        denied([]{ auto p = T::operator new(sizeof(T)); T::operator delete(p); });
        check(budget.used() == before, "Denied scalar body leaves no lease");
        budget.set(before + bytes);
        auto p = T::operator new(sizeof(T));
        check(budget.used() == before + bytes, "Scalar owner charges its complete derived body once");
        T::operator delete(p);
        check(budget.used() == before, "Scalar body retires at the full cap");
        budget.set(budget.original);
    }

    template<class T> inline void owner( Budget &budget )
    {
        body<T>(budget);
        FactoryTemplate<T> factory;
        const auto before = budget.used();
        const auto bytes = sourceBudgetStorageBytes(sizeof(T), alignof(T));
        budget.set(before + bytes - 1);
        denied([&]{ auto p = factory.allocateMemory(); factory.freeMemory(p); });
        check(budget.used() == before, "Denied procedural factory body leaves no lease");
        budget.set(before + bytes);
        auto p = factory.allocateMemory();
        check(budget.used() == before + bytes, "Factory admits complete concrete owner body");
        factory.freeMemory(p);
        check(budget.used() == before, "Factory body retires at the full cap");
        budget.set(budget.original);
    }

    template<class T> inline void construction( Budget &budget )
    {
        const auto before = budget.used();
        const auto bytes = sourceBudgetStorageBytes(sizeof(T), alignof(std::max_align_t));
        budget.set(before + bytes - 1);
        denied([]{ auto p = std::make_unique<T>(); });
        check(budget.used() == before, "Constructor body denial unwinds completely");
        budget.set(budget.original);
        auto p = std::make_unique<T>();
        const auto live = budget.used();
        check(live >= before + bytes, "Constructed owner retains its physical body");
        budget.set(live);
        p.reset();
        check(budget.used() == before, "Constructed owner and nested storage retire at cap");
        budget.set(budget.original);
    }

    inline void run()
    {
        Budget budget;
        owner<BlockNode>(budget); owner<CCityBlock>(budget); body<CityCenter>(budget);
        owner<CProceduralTerrain>(budget); owner<CRoad>(budget); owner<CRoadNode>(budget);
        owner<CRoadElement>(budget); owner<CRoadSection>(budget); owner<CRoadConnection>(budget);
        owner<CRoadMeshElement>(budget);
        owner<CityGeneratorDefault>(budget); owner<CBlockGenerator>(budget);
        owner<CRoadGenerator>(budget); owner<CRoadGeneratorCity>(budget); owner<CRoadGeneratorGrid>(budget);
        owner<CTerrainGenerator>(budget); owner<CTerrainGeneratorDefault>(budget);
        owner<CWorldGenerator>(budget); owner<MeshGeneratorDefault>(budget);
        owner<CProceduralCity>(budget); owner<CProceduralManager>(budget);
        owner<CProceduralScene>(budget); owner<CProceduralWorld>(budget); owner<CProceduralTexture>(budget);
        owner<CRoadNetwork>(budget); owner<CRoadConnectionData>(budget); owner<CRoadHitPoint>(budget);
        owner<CVehicleAppearance>(budget); owner<CVehicleDamage>(budget); owner<CVehicleDynamics>(budget);
        owner<CVehicleEffects>(budget); owner<CVehicleGenerator>(budget); owner<CVehicleGeometry>(budget);
        owner<CVehiclePhysics>(budget); owner<CVehiclePresentation>(budget);
        body<CCollisionManager>(budget); body<WPProcedural>(budget);
        body<ProceduralComponent>(budget);
        body<PlanarGraph<Vector2<real_Num>>>(budget);
        body<SCityOptions>(budget); body<STerrainGeneratorOptions>(budget);
        body<WPVehicleAppearance>(budget);
        owner<StraightRoadData>(budget); owner<HighwayInfo>(budget);
        owner<RadialPatternInfo>(budget); owner<StraightRoadInfo>(budget);
        owner<RoadFinishedInfo>(budget); owner<RoadGeneratorUtil>(budget);

        // Real construction checks include the common object's Properties child
        // and the road generator's embedded Properties metadata.
        construction<CCityBlock>(budget);
        construction<CRoadGenerator>(budget);
        construction<CProceduralScene>(budget);
        construction<CVehicleAppearance>(budget);

        const auto before = budget.used();
        const auto bytes = sourceBudgetStorageBytes(sizeof(CCityBlock), alignof(std::max_align_t));
        budget.set(before + bytes);
        denied([]{ auto p = std::make_unique<CCityBlock>(); });
        check(budget.used() == before, "Late Properties denial retires the admitted parent body");
        budget.set(budget.original);

        // Stack/embedded graph and seeded appearance values need no extra body lease.
        PlanarGraph<Vector2<real_Num>> graph;
        WPVehicleAppearance appearance(42);
        const auto stackBaseline = budget.used();
        budget.set(stackBaseline);
        auto copy = appearance;
        check(copy.profileFor(VehicleAppearanceQuality::Preview).microTextureSize > 0,
              "Appearance value copy remains usable at the cap");
        check(budget.used() == stackBaseline, "Copied fixed state adds no body lease");
    }
}
