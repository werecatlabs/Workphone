#ifndef IntersectionResult_h__
#define IntersectionResult_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/RoadTypes.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Sphere3.hpp>
#include "Workphone/Interface/Procedural/RoadMesh.hpp"

namespace workphone
{
    namespace procedural
    {

        /// Complete output of an intersection build.
        ///
        /// surface:      main intersection tarmac
        /// centerIsland: grass / foliage island inside roundabouts
        /// markings:     stop lines, zebra stripes, arrows
        /// kerbs:        junction kerb geometry
        /// dressing:     junction debris / grit detail
        struct IntersectionResult : SourceBudgetObject
        {
            IntersectionResult() = default;
            IntersectionResult( const IntersectionResult & ) = default;
            IntersectionResult( IntersectionResult && ) noexcept = default;
            IntersectionResult &operator=( IntersectionResult && ) noexcept = default;
            IntersectionResult &operator=( const IntersectionResult &other )
            {
                if( this != &other )
                {
                    IntersectionResult candidate( other );
                    swap( candidate );
                }
                return *this;
            }
            void swap( IntersectionResult &other ) noexcept
            {
                surface.swap( other.surface );
                centerIsland.swap( other.centerIsland );
                markings.swap( other.markings );
                kerbs.swap( other.kerbs );
                dressing.swap( other.dressing );
            }

            RoadMesh surface;
            RoadMesh centerIsland;
            RoadMesh markings;
            RoadMesh kerbs;
            RoadMesh dressing;
        };

    }  // namespace procedural
}  // namespace workphone

#endif  // IntersectionResult_h__
