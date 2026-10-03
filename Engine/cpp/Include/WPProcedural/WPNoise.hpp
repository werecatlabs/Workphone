// ============================================================================
// WPNoise.hpp - High-Quality Procedural Noise for AAA Procedural Generation
// ============================================================================
// References techniques from the Claude-of-Duty reference codebase:
//   - Deterministic value/gradient noise
//   - FBM (Fractional Brownian Motion) with configurable octaves
//   - Worley/Voronoi noise for cellular patterns
//   - Ridged noise for cracks and terrain detail
//   - Domain warping for organic distortion
//
// All noise functions are deterministic and seeded - same seed produces
// identical output across runs, platforms, and parallel threads.
// ============================================================================

#ifndef WPNoise_h__
#define WPNoise_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <cmath>
#include <cstdint>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief High-quality procedural noise generator for AAA content.
         *
         * All methods are const and thread-safe. The hash table is built
         * once in the constructor from a seed value, and every subsequent
         * call is a pure function of (x, y, z, seed).
         */
        class WPProcedural_API WPNoise
        {
        public:
            explicit WPNoise( u32 seed = 0x12345678 );
            ~WPNoise() = default;

            // --------------------------------------------------------------
            // Core noise primitives
            // --------------------------------------------------------------

            /// Deterministic integer hash in [0, 1) for any (x, y, z) tuple.
            real_Num hash3( s32 x, s32 y, s32 z ) const;

            /// 3D Perlin-like gradient noise in [-1, 1].
            real_Num noise3( real_Num x, real_Num y, real_Num z ) const;

            /// 2D variant for texture/UV work.
            real_Num noise2( real_Num x, real_Num y ) const;

            /// Worley/Voronoi distance to nearest feature point in [0, ~1.4].
            real_Num worley2( real_Num x, real_Num y ) const;

            /// Worley F2 - F1 (edge enhancement).
            real_Num worleyEdge2( real_Num x, real_Num y ) const;

            /// Ridged multifractal noise in [0, 1].
            real_Num ridged2( real_Num x, real_Num y, s32 octaves = 4 ) const;

            // --------------------------------------------------------------
            // Composite noise (FBM and warped variants)
            // --------------------------------------------------------------

            /// FBM (Fractional Brownian Motion) in [-1, 1].
            real_Num fbm3( real_Num x, real_Num y, real_Num z, s32 octaves = 5,
                           real_Num lacunarity = 2.03f, real_Num gain = 0.5f ) const;

            /// 2D FBM for UV-space noise.
            real_Num fbm2( real_Num x, real_Num y, s32 octaves = 5, real_Num lacunarity = 2.0f,
                           real_Num gain = 0.5f ) const;

            /// Domain-warped FBM (organic distortion).
            real_Num warpedFbm2( real_Num x, real_Num y, real_Num warpStrength = 1.0f,
                                 s32 octaves = 4 ) const;

            /// Turbulence (absolute value) FBM - good for clouds, marble.
            real_Num turbulence2( real_Num x, real_Num y, s32 octaves = 5 ) const;

            // --------------------------------------------------------------
            // Math helpers
            // --------------------------------------------------------------

            static inline real_Num clamp01( real_Num v )
            {
                return v < 0.0f ? 0.0f : ( v > 1.0f ? 1.0f : v );
            }

            static inline real_Num smoothstep( real_Num a, real_Num b, real_Num t )
            {
                t = clamp01( ( t - a ) / ( b - a ) );
                return t * t * ( 3.0f - 2.0f * t );
            }

            static inline real_Num lerp( real_Num a, real_Num b, real_Num t )
            {
                return a + t * ( b - a );
            }

            u32 getSeed() const
            {
                return mSeed;
            }

        private:
            u32 mSeed;
            u8 mPerm[512];  // Doubled permutation table for wraparound

            void buildPermutation();
            real_Num fade( real_Num t ) const;
            real_Num grad( s32 hash, real_Num x, real_Num y, real_Num z ) const;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // WPNoise_h__
