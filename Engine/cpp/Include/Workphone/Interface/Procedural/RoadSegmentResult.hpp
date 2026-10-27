#ifndef RoadSegmentResult_h__
#define RoadSegmentResult_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/RoadTypes.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/RoadVertex.hpp>
#include "Workphone/Interface/Procedural/RoadMesh.hpp"
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    namespace procedural
    {
        /// Complete output of a road segment build.
        ///
        /// Individual RoadMesh fields correspond to distinct render layers.
        /// sidewalks[0] = left side, sidewalks[1] = right side (when facing
        /// from start -> end).  dressing carries sand/gravel/kerb debris
        /// detail.  collision is a flat heightfield proxy for physics queries.
        struct RoadSegmentResult : SourceBudgetObject
        {
            RoadSegmentResult() = default;
            RoadSegmentResult( const RoadSegmentResult & ) = default;
            RoadSegmentResult( RoadSegmentResult && ) noexcept = default;
            RoadSegmentResult &operator=( RoadSegmentResult && ) noexcept = default;
            RoadSegmentResult &operator=( const RoadSegmentResult &other )
            {
                if( this != &other )
                {
                    RoadSegmentResult candidate( other );
                    swap( candidate );
                }
                return *this;
            }
            void swap( RoadSegmentResult &other ) noexcept
            {
                roadSurface.swap( other.roadSurface );
                sidewalks[0].swap( other.sidewalks[0] );
                sidewalks[1].swap( other.sidewalks[1] );
                kerbs[0].swap( other.kerbs[0] );
                kerbs[1].swap( other.kerbs[1] );
                markings.swap( other.markings );
                dressing.swap( other.dressing );
                collision.swap( other.collision );
                oldTarmac.swap( other.oldTarmac );
                potholes.swap( other.potholes );
                manholes.swap( other.manholes );
                gullyGrates.swap( other.gullyGrates );
                arrows.swap( other.arrows );
                pedXing.swap( other.pedXing );
            }

            RoadMesh roadSurface;   ///< Tarmac / carriageway
            RoadMesh sidewalks[2];  ///< Left and right footpaths
            RoadMesh kerbs[2];      ///< Left and right kerb stones
            RoadMesh markings;      ///< Lane lines, crosswalks
            RoadMesh dressing;      ///< Sand drifts, pebbles, grit
            RoadMesh collision;     ///< Flat collision proxy

            RoadMesh oldTarmac;
            RoadMesh potholes;
            RoadMesh manholes;
            RoadMesh gullyGrates;
            RoadMesh arrows;
            RoadMesh pedXing;
        };

    }  // namespace procedural
}  // namespace workphone
#endif  // RoadSegmentResult_h__
