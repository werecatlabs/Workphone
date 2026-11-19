#pragma once

#include <Workphone/Interface/Procedural/CityRoadSplineSource.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <memory>

namespace workphone::scene
{
    struct CityTrafficSettings
    {
        u32 densityPercent = 35, maximumVehicles = 32;
        f32 cruiseSpeed = 10.f, maxAcceleration = 2.5f, maxBraking = 5.f;
        f32 minimumGap = 5.f;
        bool driveOnLeft = false;
    };

    // Position is the fitted road contact, not the centre of a render mesh.
    struct CityTrafficVehicle
    {
        u32 id = 0, variant = 0, lane = 0;
        Vector3F position{}, forward{};
        f32 speed = 0;
        bool yielding = false;
        u32 respawns = 0;
    };

    struct CityTrafficStats
    {
        u32 active = 0, moving = 0, yielding = 0;
        u32 intersectionTransfers = 0, roundaboutTransfers = 0;
        f32 travelledMetres = 0, minimumSeparation = 0;
        bool valid = false;
    };

    /** Bounded, seeded traffic on the fitted city road network. The pure model
     * owns no actors or physics bodies. Calls with the same source, seed and
     * fixed-step sequence produce identical poses. An edit requires initialize
     * again with the new source and bake, never a stale lane cache.
     */
    class WPCore_API CityTrafficSimulation
    {
    public:
        CityTrafficSimulation();
        ~CityTrafficSimulation();
        CityTrafficSimulation( CityTrafficSimulation && ) noexcept;
        CityTrafficSimulation &operator=( CityTrafficSimulation && ) noexcept;
        CityTrafficSimulation( const CityTrafficSimulation & ) = delete;
        CityTrafficSimulation &operator=( const CityTrafficSimulation & ) = delete;

        bool initialize( const procedural::CityRoadSplineSource &source,
                         const procedural::CityRoadSplineBake &bake,
                         const procedural::CityRoadHeightSampler &terrainHeight,
                         const CityTrafficSettings &settings, u32 seed, String &error );
        void step( f32 fixedDt );
        // Drive a route with a physical body. Route progress follows its measured
        // position, while the published speed becomes the desired driving speed.
        void setVehicleMotion( u32 id, Vector3F position, f32 speed );
        bool getDrivingTarget( u32 id, f32 lookAhead, Vector3F &target ) const;
        void setExternalObstacle( Vector3F position, f32 radius, bool enabled = true );
        void clear();

        const SourceBudgetVector<CityTrafficVehicle> &vehicles() const;
        CityTrafficStats stats() const;
        u64 sourceRevision() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}  // namespace workphone::scene
