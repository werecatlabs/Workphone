#ifndef RoadVertex_h__
#define RoadVertex_h__

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

        /// Single vertex in a procedural road mesh.
        ///
        /// Per-vertex color encodes PBR material variation:
        ///   r = wear (exposed aggregate / substrate)
        ///   g = grime / dust accumulation
        ///   b = ambient occlusion proxy
        struct RoadVertex
        {
            Vector3<real_Num> position;
            Vector3<real_Num> normal;
            Vector2<real_Num> uv;
            Vector3<real_Num> color;  ///< PBR variation mask
        };

    }  // namespace procedural
}  // namespace workphone
#endif  // RoadVertex_h__
