#ifndef Geometry_h__
#define Geometry_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>

namespace workphone
{
    struct Geometry
    {
        Array<Vector3F> positions{ Allocator<Vector3F>( sourceBudget ) };
        Array<Vector3F> normals{ Allocator<Vector3F>( sourceBudget ) };
        Array<Vector3F> uv{ Allocator<Vector3F>( sourceBudget ) };
        Array<u32> indices{ Allocator<u32>( sourceBudget ) };
        Array<Vector4F> tangents{ Allocator<Vector4F>( sourceBudget ) };

        void quad( Vector3F a, Vector3F b, Vector3F c, Vector3F d, Vector3F normal, float u0 = 0,
                   float u1 = 1, float v0 = 0, float v1 = 1 );

        void beam( Vector3F a, Vector3F b, float radius );

        void box( Vector3F p, Vector3F size );
    };
}

#endif // Geometry_h__
