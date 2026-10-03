// ============================================================================
// WPProceduralTextureData.hpp - Raw pixel data structures for procedural textures
// ============================================================================
// Provides CPU-side pixel buffers that can be uploaded to the GPU, with
// sRGB encoding for albedo and linear storage for ORM/normal maps.
// ============================================================================

#ifndef WPProceduralTextureData_h__
#define WPProceduralTextureData_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/RoadTypes.hpp>
#include <cstdint>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief 8-bit RGBA pixel buffer, 32 bits per pixel, row-major.
         *
         * Memory layout matches OpenGL/D3D UNORM8 RGBA so the buffer can be
         * uploaded without further rearrangement.
         */
        struct TextureBuffer
        {
            u32 width = 0;
            u32 height = 0;
            std::vector<u8> pixels;

            TextureBuffer() = default;
            TextureBuffer( u32 w, u32 h ) : width( w ), height( h ), pixels( w * h * 4, 0 )
            {
            }

            u32 sizeBytes() const
            {
                return static_cast<u32>( pixels.size() );
            }
            u8 *data()
            {
                return pixels.data();
            }
            const u8 *data() const
            {
                return pixels.data();
            }

            inline u8 *pixel( u32 x, u32 y )
            {
                return pixels.data() + ( y * width + x ) * 4;
            }
            inline const u8 *pixel( u32 x, u32 y ) const
            {
                return pixels.data() + ( y * width + x ) * 4;
            }
        };

        /**
         * @brief 32-bit float single-channel height buffer (for normal map derivation).
         */
        struct HeightBuffer
        {
            u32 width = 0;
            u32 height = 0;
            std::vector<real_Num> values;

            HeightBuffer() = default;
            HeightBuffer( u32 w, u32 h ) : width( w ), height( h ), values( w * h, 0.0f )
            {
            }

            inline real_Num at( u32 x, u32 y ) const
            {
                u32 xx = x < width ? x : width - 1;
                u32 yy = y < height ? y : height - 1;
                return values[yy * width + xx];
            }
        };

        /// Surface categories used by the materials library.
        enum class SurfaceTag : u8
        {
            Concrete = 0,
            Plaster,
            Brick,
            Wood,
            Metal,
            Asphalt,
            Sand,
            Fabric,
            Foliage,
            Glass,
            Paint,
            Rubber,
            Dirt,
            Stone,
            Count
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // WPProceduralTextureData_h__
