#ifndef Circuit_h__
#define Circuit_h__

#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainFit.hpp>
#include <Workphone/Scene/Components/CityTrafficSimulation.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <array>
#include <memory>

namespace workphone::scene::race
{

    /**
     * A single sample point along a race circuit.
     *
     * position: world-space position of the sample.
     * right: world-space right direction at the sample (used for vehicle orientation).
     * distance: accumulated distance along the circuit up to this sample.
     */
    struct CircuitSample
    {
        Vector3F position;
        Vector3F right;
        f32 distance = 0.0f;
    };

    /**
     * Represents a procedural race circuit composed of ordered samples.
     *
     * The samples vector contains the track samples in order; length is the total
     * path length computed from the samples. nearest() returns the index of the
     * sample closest to a given world-space position.
     */
    struct WPCore_API Circuit : SourceBudgetObject
    {
        SourceBudgetVector<CircuitSample> samples;
        float length = 0;
        Circuit();
        Circuit( const Circuit & );
        Circuit( Circuit && ) noexcept;
        Circuit &operator=( Circuit && ) noexcept;

        Circuit &operator=( const Circuit &other );

        void swap( Circuit &other ) noexcept;

        /**
         * Find the index of the nearest circuit sample to the provided position.
         *
         * @param position World-space position to query.
         * @return Index into the samples vector of the nearest sample.
         */
        size_t nearest( const Vector3F &position ) const;
    };

}  // namespace workphone::scene::race

#endif  // Circuit_h__
