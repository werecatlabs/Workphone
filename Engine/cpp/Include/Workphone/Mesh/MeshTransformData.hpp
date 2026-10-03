#ifndef MeshTransformData_h__
#define MeshTransformData_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{

    /// A structure to store mesh transform data.
    struct MeshTransformData
    {
        MeshTransformData();

        MeshTransformData( const Vector3<real_Num> &position, const Vector3<real_Num> &scale,
                           const Quaternion<real_Num> &orientation, const SmartPtr<IMesh> &mesh );

        Vector3<real_Num> Position;
        Vector3<real_Num> Scale;
        Quaternion<real_Num> Orientation;
        SmartPtr<IMesh> Mesh;

        Array<Vector2<real_Num>> UVOffsets;
        Array<Vector2<real_Num>> UVScaleData;
    };
}  // namespace workphone

#endif  // MeshTransformData_h__
