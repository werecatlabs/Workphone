#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/IntersectionResult.hpp>
#include <Workphone/Interface/Procedural/RoadMesh.hpp>
#include <Workphone/Interface/Procedural/RoadSegmentResult.hpp>
#include <Workphone/Interface/Procedural/RoadSegmentSpec.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Procedural/RoadTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        // ===================================================================
        // Free types (declared at namespace scope so they can be used
        // unqualified from within namespace procedural, and also reachable
        // as WPRoadSystem::RoadSegmentResult etc. via the aliases inside
        // WPRoadSystem below).
        // ===================================================================

        /// Complete input for an intersection build.
        ///
        /// |center| is the world-space pivot of the junction.  |radius| is
        /// the outer extent used for roundabouts and X crossings.  |roadClass|
        /// determines dressing density and kerb treatment.  |surface| lets the
        /// caller override the default carriageway material.
        struct IntersectionSpec
        {
            Vector3<real_Num> center;
            real_Num radius = 8.0f;
            RoadClass roadClass = RoadClass::Residential;
            RoadSurface surface = RoadSurface::Asphalt;
            IntersectionType type = IntersectionType::Cross;
            u32 seed = 0;
            Array<Vector3<real_Num>> approaches;  ///< Direction vectors of approaching roads
        };

    }  // namespace procedural
}  // namespace workphone
