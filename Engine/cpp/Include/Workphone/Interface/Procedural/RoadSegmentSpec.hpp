#ifndef RoadSegmentSpec_h__
#define RoadSegmentSpec_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/RoadTypes.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    namespace procedural
    {

        /// Complete input for a single road segment build.
        ///
        /// A segment runs from |start| to |end| in world space.  The system
        /// builds a full cross-section (carriageway, kerbs, sidewalks,
        /// markings, dressing, collision proxy) and orients it along the
        /// start->end vector.
        struct RoadSegmentSpec
        {
            Vector3<real_Num> start;
            Vector3<real_Num> end;
            RoadClass roadClass = RoadClass::Residential;
            RoadSurface surface = RoadSurface::Asphalt;
            u32 seed = 0;  ///< Per-segment noise seed
            bool generateSidewalks = true;
            bool generateKerbs = true;
            bool generateMarkings = true;
            bool generateDressing = true;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // RoadSegmentSpec_h__
