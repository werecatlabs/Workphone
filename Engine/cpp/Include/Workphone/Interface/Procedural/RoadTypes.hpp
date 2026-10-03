#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
namespace workphone::procedural
{
    /// Functional road classification.
    ///
    /// Determines default widths, surface types, and dressing layers.
    enum class RoadClass
    {
        Highway,
        Arterial,
        Residential,
        Alley,
        Footway
    };

    /// Material type of the road carriageway.
    enum class RoadSurface
    {
        Asphalt,
        Concrete,
        Dirt
    };

    /// Geometric template for road junctions.
    ///
    /// Enumerator names use underscores and mirror the values of
    /// IRoadConnection::EType so the two can be mapped directly.
    enum class IntersectionType
    {
        Cross,       ///< 4-way X crossing (alias of X_Crossing)
        T_Junction,  ///< 3-way T junction
        X_Crossing,  ///< General 4-arm intersection
        Roundabout,  ///< Circular roundabout with central island
        L_Corner,    ///< 90-degree bend
        DeadEnd      ///< Cul-de-sac / no exit
    };

}  // namespace workphone::procedural
