// ============================================================================
// WPNoise.cpp - Implementation of the AAA-grade procedural noise system
// ============================================================================
#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPNoise.hpp"

namespace workphone
{
    namespace procedural
    {
        // -------------------------------------------------------------------
        // Constructor - builds deterministic permutation table from seed
        // -------------------------------------------------------------------
        WPNoise::WPNoise( u32 seed ) : mSeed( seed )
        {
            buildPermutation();
        }

        // -------------------------------------------------------------------
        // Build the 256-entry permutation table (Fisher-Yates with seed)
        // -------------------------------------------------------------------
        void WPNoise::buildPermutation()
        {
            u8 p[256];
            for( s32 i = 0; i < 256; ++i )
                p[i] = static_cast<u8>( i );

            // Fisher-Yates shuffle using a deterministic LCG driven by mSeed
            u32 s = mSeed;
            for( s32 i = 255; i > 0; --i )
            {
                s = s * 1664525u + 1013904223u;
                s32 j = static_cast<s32>( ( s >> 16 ) % ( i + 1 ) );
                u8 t = p[i];
                p[i] = p[j];
                p[j] = t;
            }

            // Duplicate the table for wraparound indexing
            for( s32 i = 0; i < 512; ++i )
                mPerm[i] = p[i & 255];
        }

        // -------------------------------------------------------------------
        // Hash function - deterministic 32-bit hash for integer coords
        // -------------------------------------------------------------------
        real_Num WPNoise::hash3( s32 x, s32 y, s32 z ) const
        {
            // Same structure as reference: multiply-xors with prime constants
            u32 h = static_cast<u32>( x ) * 0x85ebca6bu;
            h ^= static_cast<u32>( y ) * 0xc2b2ae35u;
            h ^= static_cast<u32>( z ) * 0x27d4eb2fu;
            h ^= h >> 15;
            return static_cast<real_Num>( ( h * 0x85ebca6bu ) >> 0 ) /
                   static_cast<real_Num>( 0xFFFFFFFFu );
        }

        // -------------------------------------------------------------------
        // Smoothstep easing function
        // -------------------------------------------------------------------
        real_Num WPNoise::fade( real_Num t ) const
        {
            return t * t * t * ( t * ( t * 6.0f - 15.0f ) + 10.0f );
        }

        // -------------------------------------------------------------------
        // Gradient vector lookup (12 standard 3D gradient directions)
        // -------------------------------------------------------------------
        real_Num WPNoise::grad( s32 hash, real_Num x, real_Num y, real_Num z ) const
        {
            // 12 edges of a cube
            const real_Num g[12][3] = { { 1.0f, 1.0f, 0.0f },  { -1.0f, 1.0f, 0.0f },
                                        { 1.0f, -1.0f, 0.0f }, { -1.0f, -1.0f, 0.0f },
                                        { 1.0f, 0.0f, 1.0f },  { -1.0f, 0.0f, 1.0f },
                                        { 1.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, -1.0f },
                                        { 0.0f, 1.0f, 1.0f },  { 0.0f, -1.0f, 1.0f },
                                        { 0.0f, 1.0f, -1.0f }, { 0.0f, -1.0f, -1.0f } };

            s32 h = hash & 15;
            s32 gi = h < 12 ? h : h - 12;
            return g[gi][0] * x + g[gi][1] * y + g[gi][2] * z;
        }

        // -------------------------------------------------------------------
        // 3D Perlin-like gradient noise in [-1, 1]
        // -------------------------------------------------------------------
        real_Num WPNoise::noise3( real_Num x, real_Num y, real_Num z ) const
        {
            s32 X = static_cast<s32>( std::floor( x ) ) & 255;
            s32 Y = static_cast<s32>( std::floor( y ) ) & 255;
            s32 Z = static_cast<s32>( std::floor( z ) ) & 255;

            real_Num xf = x - std::floor( x );
            real_Num yf = y - std::floor( y );
            real_Num zf = z - std::floor( z );

            real_Num u = fade( xf );
            real_Num v = fade( yf );
            real_Num w = fade( zf );

            s32 A = mPerm[X] + Y;
            s32 AA = mPerm[A] + Z;
            s32 AB = mPerm[A + 1] + Z;
            s32 B = mPerm[X + 1] + Y;
            s32 BA = mPerm[B] + Z;
            s32 BB = mPerm[B + 1] + Z;

            return lerp(
                lerp(
                    lerp( grad( mPerm[AA], xf, yf, zf ), grad( mPerm[BA], xf - 1, yf, zf ), u ),
                    lerp( grad( mPerm[AB], xf, yf - 1, zf ), grad( mPerm[BB], xf - 1, yf - 1, zf ), u ),
                    v ),
                lerp( lerp( grad( mPerm[AA + 1], xf, yf, zf - 1 ),
                            grad( mPerm[BA + 1], xf - 1, yf, zf - 1 ), u ),
                      lerp( grad( mPerm[AB + 1], xf, yf - 1, zf - 1 ),
                            grad( mPerm[BB + 1], xf - 1, yf - 1, zf - 1 ), u ),
                      v ),
                w );
        }

        // -------------------------------------------------------------------
        // 2D noise (z=0 slice of 3D noise)
        // -------------------------------------------------------------------
        real_Num WPNoise::noise2( real_Num x, real_Num y ) const
        {
            return noise3( x, y, 0.0f );
        }

        // -------------------------------------------------------------------
        // Worley/Voronoi - distance to nearest feature point
        // -------------------------------------------------------------------
        real_Num WPNoise::worley2( real_Num x, real_Num y ) const
        {
            s32 xi = static_cast<s32>( std::floor( x ) );
            s32 yi = static_cast<s32>( std::floor( y ) );
            real_Num minDist = 1e9f;

            for( s32 dy = -1; dy <= 1; ++dy )
            {
                for( s32 dx = -1; dx <= 1; ++dx )
                {
                    real_Num fx = hash3( xi + dx, yi + dy, 0 );
                    real_Num fy = hash3( xi + dx, yi + dy, 1 );
                    real_Num rx = static_cast<real_Num>( dx ) + fx;
                    real_Num ry = static_cast<real_Num>( dy ) + fy;
                    real_Num dist = std::sqrt( ( x - rx ) * ( x - rx ) + ( y - ry ) * ( y - ry ) );
                    if( dist < minDist )
                        minDist = dist;
                }
            }
            return minDist;
        }

        // -------------------------------------------------------------------
        // Worley F2 - F1 - edge enhancement
        // -------------------------------------------------------------------
        real_Num WPNoise::worleyEdge2( real_Num x, real_Num y ) const
        {
            s32 xi = static_cast<s32>( std::floor( x ) );
            s32 yi = static_cast<s32>( std::floor( y ) );
            real_Num f1 = 1e9f;
            real_Num f2 = 1e9f;

            for( s32 dy = -1; dy <= 1; ++dy )
            {
                for( s32 dx = -1; dx <= 1; ++dx )
                {
                    real_Num fx = hash3( xi + dx, yi + dy, 0 );
                    real_Num fy = hash3( xi + dx, yi + dy, 1 );
                    real_Num rx = static_cast<real_Num>( dx ) + fx;
                    real_Num ry = static_cast<real_Num>( dy ) + fy;
                    real_Num dist = std::sqrt( ( x - rx ) * ( x - rx ) + ( y - ry ) * ( y - ry ) );
                    if( dist < f1 )
                    {
                        f2 = f1;
                        f1 = dist;
                    }
                    else if( dist < f2 )
                    {
                        f2 = dist;
                    }
                }
            }
            return f2 - f1;
        }

        // -------------------------------------------------------------------
        // Ridged multifractal noise - cracks, veins, mountain ridges
        // -------------------------------------------------------------------
        real_Num WPNoise::ridged2( real_Num x, real_Num y, s32 octaves ) const
        {
            real_Num sum = 0.0f;
            real_Num amp = 1.0f;
            real_Num freq = 1.0f;
            real_Num max = 0.0f;

            for( s32 i = 0; i < octaves; ++i )
            {
                real_Num n = std::abs( noise3( x * freq, y * freq, 0.0f ) );
                n = 1.0f - n;
                sum += n * amp;
                max += amp;
                amp *= 0.5f;
                freq *= 2.1f;
            }
            return sum / max;
        }

        // -------------------------------------------------------------------
        // FBM - layered noise
        // -------------------------------------------------------------------
        real_Num WPNoise::fbm3( real_Num x, real_Num y, real_Num z, s32 octaves, real_Num lacunarity,
                                real_Num gain ) const
        {
            real_Num sum = 0.0f;
            real_Num amplitude = 0.5f;
            real_Num frequency = 1.0f;

            for( s32 i = 0; i < octaves; ++i )
            {
                sum += amplitude * noise3( x * frequency, y * frequency, z * frequency );
                amplitude *= gain;
                frequency *= lacunarity;
            }
            return sum;
        }

        real_Num WPNoise::fbm2( real_Num x, real_Num y, s32 octaves, real_Num lacunarity,
                                real_Num gain ) const
        {
            return fbm3( x, y, 0.0f, octaves, lacunarity, gain );
        }

        // -------------------------------------------------------------------
        // Domain-warped FBM - organic, fluid-like distortion
        // -------------------------------------------------------------------
        real_Num WPNoise::warpedFbm2( real_Num x, real_Num y, real_Num warpStrength, s32 octaves ) const
        {
            // Two channels of offset noise
            real_Num wx = fbm2( x + 5.2f, y + 1.3f, octaves );
            real_Num wy = fbm2( x + 8.7f, y + 2.8f, octaves );
            return fbm2( x + warpStrength * wx, y + warpStrength * wy, octaves );
        }

        // -------------------------------------------------------------------
        // Turbulence - |FBM| for marble, cloud-like patterns
        // -------------------------------------------------------------------
        real_Num WPNoise::turbulence2( real_Num x, real_Num y, s32 octaves ) const
        {
            real_Num sum = 0.0f;
            real_Num amplitude = 0.5f;
            real_Num frequency = 1.0f;

            for( s32 i = 0; i < octaves; ++i )
            {
                sum += amplitude * std::abs( noise3( x * frequency, y * frequency, 0.0f ) );
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }
            return sum;
        }
    }  // namespace procedural
}  // namespace workphone
