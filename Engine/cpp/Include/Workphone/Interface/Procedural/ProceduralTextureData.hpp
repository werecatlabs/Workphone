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
#include <Workphone/System/SourceBudgetVector.hpp>
#include <limits>
#include <stdexcept>

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
        struct TextureBuffer : SourceBudgetObject
        {
            u32 width = 0;
            u32 height = 0;
            SourceBudgetVector<u8> pixels;

            TextureBuffer() = default;
            static size_t elementCount( u32 w, u32 h )
            {
                if(h && size_t(w)>std::numeric_limits<u32>::max()/size_t(h)/4)
                    throw std::length_error("Procedural texture dimensions overflow byte count");
                return size_t(w)*h*4;
            }
            TextureBuffer( u32 w, u32 h ) : width( w ), height( h ), pixels( elementCount(w,h), 0 )
            {
            }
            TextureBuffer(const TextureBuffer&)=default;
            TextureBuffer(TextureBuffer&&) noexcept=default;
            TextureBuffer &operator=(TextureBuffer&&) noexcept=default;
            TextureBuffer &operator=(const TextureBuffer &other)
            { if(this!=&other){TextureBuffer candidate(other);swap(candidate);} return *this; }
            void swap(TextureBuffer &other) noexcept
            { using std::swap;swap(width,other.width);swap(height,other.height);pixels.swap(other.pixels); }

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
                return pixels.data() + ( size_t(y) * width + x ) * 4;
            }
            inline const u8 *pixel( u32 x, u32 y ) const
            {
                return pixels.data() + ( size_t(y) * width + x ) * 4;
            }
        };

        /**
         * @brief 32-bit float single-channel height buffer (for normal map derivation).
         */
        struct HeightBuffer : SourceBudgetObject
        {
            u32 width = 0;
            u32 height = 0;
            SourceBudgetVector<real_Num> values;

            HeightBuffer() = default;
            static size_t elementCount( u32 w, u32 h )
            {
                if(h && size_t(w)>std::numeric_limits<size_t>::max()/sizeof(real_Num)/h)
                    throw std::length_error("Procedural height dimensions overflow storage");
                return size_t(w)*h;
            }
            HeightBuffer( u32 w, u32 h ) : width( w ), height( h ), values( elementCount(w,h), 0.0f )
            {
            }
            HeightBuffer(const HeightBuffer&)=default;
            HeightBuffer(HeightBuffer&&) noexcept=default;
            HeightBuffer &operator=(HeightBuffer&&) noexcept=default;
            HeightBuffer &operator=(const HeightBuffer &other)
            { if(this!=&other){HeightBuffer candidate(other);swap(candidate);} return *this; }
            void swap(HeightBuffer &other) noexcept
            { using std::swap;swap(width,other.width);swap(height,other.height);values.swap(other.values); }

            inline real_Num at( u32 x, u32 y ) const
            {
                if(!width || !height || values.size()<size_t(width)*height)
                    throw std::out_of_range("Empty or incomplete procedural height buffer");
                u32 xx = x < width ? x : width - 1;
                u32 yy = y < height ? y : height - 1;
                return values[size_t(yy) * width + xx];
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
