#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/Array.hpp>
#include <array>

namespace workphone
{
    struct MeshImposterTriangle
    {
        std::array<Vector3F, 3> positions;
        ColourF colour;
    };
    struct MeshImposterView
    {
        Vector3F right, up, towardCamera;
        Vector2F minimum, maximum;
    };
    struct MeshImposterAtlas
    {
        u32 width = 0, height = 0, tileWidth = 0;
        Array<u8> rgba;
        std::array<MeshImposterView, 3> views;
    };
    /// CPU orthographic rasterization of mesh triangles into two side views and a top view.
    /// Output is straight-alpha sRGB RGBA, suitable for depth-writing cutout materials.
    WPCore_API MeshImposterAtlas generateMeshImposters( const Array<MeshImposterTriangle> &triangles,
                                                        const Vector3F &boundsMin,
                                                        const Vector3F &boundsMax, u32 tileWidth = 128,
                                                        u32 tileHeight = 256 );
}  // namespace workphone
