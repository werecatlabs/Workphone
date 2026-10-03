// ============================================================================
// WPTextureForge.hpp - AAA Procedural PBR Texture Generator
// ============================================================================
// Mirrors the Claude-of-Duty material/texture system but with a C++17,
// engine-native implementation. All textures are generated from procedural
// noise at load time - no external image files are required.
//
// Each surface produces three packed textures:
//   - albedo    (sRGB encoded RGBA8)
//   - normal    (tangent-space, RGBA8 in [0, 1])
//   - orm       (R=occlusion, G=roughness, B=metalness, A=1)
//
// In addition, an internal height buffer is computed and used to derive the
// tangent-space normal map via a 3x3 Sobel kernel.
// ============================================================================

#ifndef WPTextureForge_h__
#define WPTextureForge_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include "WPProcedural/WPProceduralTextureData.hpp"
#include "WPProcedural/WPNoise.hpp"

#include <Workphone/Interface/Procedural/TextureForgeTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPTextureForge
        {
        public:
            explicit WPTextureForge( u32 seed = 0x2a );
            ~WPTextureForge() = default;

            /// Bake a single surface into all PBR maps.
            SurfaceBakeResult bakeSurface( SurfaceTag tag, const SurfaceBakeParams &params );

            // --------------------------------------------------------------
            // Individual surface generators (also exposed for tests)
            // --------------------------------------------------------------
            void bakeConcrete( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakePlaster( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeBrick( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeWood( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeMetal( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeAsphalt( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeSand( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeFabric( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeFoliage( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeGlass( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakePaint( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeRubber( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeDirt( SurfaceBakeResult &out, const SurfaceBakeParams &params );
            void bakeStone( SurfaceBakeResult &out, const SurfaceBakeParams &params );

            // --------------------------------------------------------------
            // Map derivations
            // --------------------------------------------------------------

            /// Convert a height buffer into a tangent-space normal map (RGB = nx, ny, nz).
            static void heightToNormal( const HeightBuffer &height, TextureBuffer &normalOut,
                                        real_Num strength = 2.0f );

            /// Compute occlusion/roughness/metalness from height and surface cues.
            static void computeORM( const HeightBuffer &height, TextureBuffer &ormOut, SurfaceTag tag );

            /// Apply sRGB encoding to a linear RGB albedo buffer.
            static void encodeSRGB( TextureBuffer &albedo );

            u32 getSeed() const
            {
                return mSeed;
            }

        private:
            u32 mSeed;
            WPNoise mNoise;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // WPTextureForge_h__
