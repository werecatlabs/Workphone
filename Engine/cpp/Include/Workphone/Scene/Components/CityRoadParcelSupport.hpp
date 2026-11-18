#pragma once
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Interface/Procedural/CityRoadSplineSource.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Scene/Components/CityRoadBlockFaces.hpp>

namespace workphone::scene
{
    struct CityRoadParcelSupportOptions
    {
        // Opt-in: keep the road-facing edge and shrink only fully supported
        // envelopes that exceed the unchanged 1.25 m foundation relief limit.
        bool shrinkForRelief = false;
        f32 minimumBuildingSpan = 6.f;
    };

    /** Filter whole parcel envelopes against canonical roads/junctions and exact
     * retained terrain-cell coverage. Accepted lots preserve order/identity and
     * receive full-footprint minimum/maximum foundation heights; relief >1.25m
     * or unsupported terrain culls a lot. Optional shrinking tries at most25
     * anisotropic subsets, preserving frontage, IDs and the road-facing edge.
     * Terrain holes/steep cells are never rescued by shrinking; supportChecks
     * counts all attempted exact parcel terrain fits.
     * Source/bake/terrain errors and resource/work denial fail transactionally:
     * output, culled and supportChecks remain unchanged. At most400 parcels;
     * each rectangle's shorter dimension must fit the existing <=60m road sweep.
     */
    WPCore_API bool prepareCityRoadParcels( const procedural::OpenCityLayout &input,
                                            const procedural::CityRoadSplineSource &source,
                                            const procedural::CityRoadSplineBake &bake,
                                            render::TerrainSnapshot terrain,
                                            const Transform3<real_Num> &placement,
                                            procedural::OpenCityLayout &output, u32 &culled,
                                            u32 &supportChecks, String &error );
    WPCore_API bool prepareCityRoadParcels( const procedural::OpenCityLayout &input,
                                            const procedural::CityRoadSplineSource &source,
                                            const procedural::CityRoadSplineBake &bake,
                                            render::TerrainSnapshot terrain,
                                            const Transform3<real_Num> &placement,
                                            procedural::OpenCityLayout &output, u32 &culled,
                                            u32 &supportChecks, String &error,
                                            const CityRoadParcelSupportOptions &options );
    /** Admit existing lots first, then use the remaining natural-city lot budget
     * in the least developed enclosed blocks of the authored road network.
     * culled counts rejected input lots. Optional faces describe this same road bake. */
    WPCore_API bool prepareCityRoadParcels( const procedural::OpenCityLayout &input,
                                            const procedural::CityRoadSplineSource &source,
                                            const procedural::CityRoadSplineBake &bake,
                                            render::TerrainSnapshot terrain,
                                            const Transform3<real_Num> &placement,
                                            procedural::OpenCityLayout &output, u32 &culled,
                                            u32 &supportChecks, String &error,
                                            const CityRoadParcelSupportOptions &options, u32 infillSeed,
                                            CityRoadBlockFaces *faces = nullptr );
}  // namespace workphone::scene
