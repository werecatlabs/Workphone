#pragma once
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Interface/Procedural/CityRoadSplineSource.hpp>
#include <Workphone/Graphics/TerrainData.hpp>

namespace workphone::scene
{
    /** Filter whole parcel envelopes against canonical roads/junctions and exact
     * retained terrain-cell coverage. Accepted lots preserve order/identity and
     * receive full-footprint minimum/maximum foundation heights; relief >1.25m
     * or unsupported terrain culls a lot. supportChecks counts parcel terrain fits.
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
}  // namespace workphone::scene
