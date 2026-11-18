#pragma once
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Scene/Components/CityRoadJunctions.hpp>

namespace workphone::scene
{
    using CityRoadBlockFaces = SourceBudgetVector<SourceBudgetVector<procedural::OpenCityLayout::Point>>;

    // Counterclockwise bounded contours of the fitted XZ road network. Explicit
    // roundabout approaches meet at their centre. A dead end cannot enclose land.
    // Disconnected nested loops retain separate contours, smallest first.
    WPCore_API CityRoadBlockFaces cityRoadBlockFaces( const procedural::CityRoadSplineSource &source,
                                                      const procedural::CityRoadSplineBake &bake,
                                                      const CityRoadJunctionBake &junctions );
}  // namespace workphone::scene
