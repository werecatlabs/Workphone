#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include "Workphone/Interface/Procedural/ProceduralTextureData.hpp"

namespace workphone
{
    namespace procedural
    {
        /// Parameters controlling the bake of a single surface.
        struct WPCore_API SurfaceBakeParams
        {
            u32 size = 1024;              ///< Power-of-two texture resolution
            real_Num tileUV = 1.0f;       ///< Tile multiplier for the material
            bool sRGB = true;             ///< Apply sRGB encoding (true for albedo)
            real_Num parallax = 0.0f;     ///< Parallax strength for the shader
            real_Num detailScale = 1.0f;  ///< Detail normal tile scale
        };

        /// Result of baking a surface: albedo, normal, ORM, and height.
        struct SurfaceBakeResult
        {
            TextureBuffer albedo;
            TextureBuffer normal;
            TextureBuffer orm;
            HeightBuffer height;
        };

    }  // namespace procedural
}  // namespace workphone
