// ============================================================================
// WPTextureForge.cpp - Implementation of AAA procedural PBR texture baking
// ============================================================================
// References the Claude-of-Duty texture system: concrete with formwork lines
// and aggregate, plaster with cracks and water stains, brick with mortar gaps
// and spalled corners, etc. All using multi-octave FBM + Worley + ridged.
// ============================================================================
#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPTextureForge.hpp"
#include <cmath>
#include <algorithm>

namespace workphone
{
    namespace procedural
    {
        // -------------------------------------------------------------------
        // Helpers
        // -------------------------------------------------------------------
        namespace
        {
            inline u8 toByte( real_Num v )
            {
                v = v < 0.0f ? 0.0f : ( v > 1.0f ? 1.0f : v );
                return static_cast<u8>( v * 255.0f + 0.5f );
            }

            inline real_Num srgbEncode( real_Num linear )
            {
                linear = linear < 0.0f ? 0.0f : ( linear > 1.0f ? 1.0f : linear );
                if( linear <= 0.0031308f )
                    return linear * 12.92f;
                return 1.055f * std::pow( linear, 1.0f / 2.4f ) - 0.055f;
            }

            inline u8 srgbToByte( real_Num linear )
            {
                return toByte( srgbEncode( linear ) );
            }
        }  // namespace

        // -------------------------------------------------------------------
        // Constructor
        // -------------------------------------------------------------------
        WPTextureForge::WPTextureForge( u32 seed ) : mSeed( seed ), mNoise( seed )
        {
        }

        // -------------------------------------------------------------------
        // Top-level bake: route by surface tag
        // -------------------------------------------------------------------
        SurfaceBakeResult WPTextureForge::bakeSurface( SurfaceTag tag, const SurfaceBakeParams &p )
        {
            SurfaceBakeResult out;
            switch( tag )
            {
            case SurfaceTag::Concrete:
                bakeConcrete( out, p );
                break;
            case SurfaceTag::Plaster:
                bakePlaster( out, p );
                break;
            case SurfaceTag::Brick:
                bakeBrick( out, p );
                break;
            case SurfaceTag::Wood:
                bakeWood( out, p );
                break;
            case SurfaceTag::Metal:
                bakeMetal( out, p );
                break;
            case SurfaceTag::Asphalt:
                bakeAsphalt( out, p );
                break;
            case SurfaceTag::Sand:
                bakeSand( out, p );
                break;
            case SurfaceTag::Fabric:
                bakeFabric( out, p );
                break;
            case SurfaceTag::Foliage:
                bakeFoliage( out, p );
                break;
            case SurfaceTag::Glass:
                bakeGlass( out, p );
                break;
            case SurfaceTag::Paint:
                bakePaint( out, p );
                break;
            case SurfaceTag::Rubber:
                bakeRubber( out, p );
                break;
            case SurfaceTag::Dirt:
                bakeDirt( out, p );
                break;
            case SurfaceTag::Stone:
                bakeStone( out, p );
                break;
            default:
                bakeConcrete( out, p );
                break;
            }

            // Derive normal map from height buffer
            TextureBuffer normal( p.size, p.size );
            heightToNormal( out.height, normal, 2.0f );
            out.normal = std::move( normal );

            // Compute ORM from height + tag-specific rules
            TextureBuffer orm( p.size, p.size );
            computeORM( out.height, orm, tag );
            out.orm = std::move( orm );

            // Apply sRGB encoding to the linear albedo
            if( p.sRGB )
                encodeSRGB( out.albedo );

            return out;
        }

        // ===================================================================
        // Height -> Normal (Sobel kernel)
        // ===================================================================
        void WPTextureForge::heightToNormal( const HeightBuffer &h, TextureBuffer &normalOut,
                                             real_Num strength )
        {
            const u32 W = h.width;
            const u32 H = h.height;
            normalOut = TextureBuffer( W, H );

            for( u32 y = 0; y < H; ++y )
            {
                for( u32 x = 0; x < W; ++x )
                {
                    real_Num hl = h.at( x > 0 ? x - 1 : x, y );
                    real_Num hr = h.at( x + 1 < W ? x + 1 : x, y );
                    real_Num hu = h.at( x, y > 0 ? y - 1 : y );
                    real_Num hd = h.at( x, y + 1 < H ? y + 1 : y );

                    real_Num gx = ( hr - hl ) * strength;
                    real_Num gy = ( hd - hu ) * strength;

                    // Tangent-space normal: n = normalize( -gx, -gy, 1 )
                    real_Num nx = -gx;
                    real_Num ny = -gy;
                    real_Num nz = 1.0f;
                    real_Num l = std::sqrt( nx * nx + ny * ny + nz * nz );
                    if( l > 1e-6f )
                    {
                        nx /= l;
                        ny /= l;
                        nz /= l;
                    }
                    else
                    {
                        nx = 0;
                        ny = 0;
                        nz = 1.0f;
                    }

                    u8 *px = normalOut.pixel( x, y );
                    px[0] = toByte( nx * 0.5f + 0.5f );
                    px[1] = toByte( ny * 0.5f + 0.5f );
                    px[2] = toByte( nz * 0.5f + 0.5f );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // Height -> ORM (Occlusion, Roughness, Metalness)
        // ===================================================================
        void WPTextureForge::computeORM( const HeightBuffer &h, TextureBuffer &ormOut, SurfaceTag tag )
        {
            const u32 W = h.width;
            const u32 H = h.height;
            ormOut = TextureBuffer( W, H );

            // Per-surface baseline roughness/metalness
            real_Num baseRough = 0.85f;
            real_Num baseMetal = 0.0f;
            switch( tag )
            {
            case SurfaceTag::Metal:
                baseRough = 0.35f;
                baseMetal = 0.85f;
                break;
            case SurfaceTag::Glass:
                baseRough = 0.05f;
                baseMetal = 0.0f;
                break;
            case SurfaceTag::Paint:
                baseRough = 0.55f;
                baseMetal = 0.0f;
                break;
            case SurfaceTag::Rubber:
                baseRough = 0.92f;
                baseMetal = 0.0f;
                break;
            case SurfaceTag::Fabric:
                baseRough = 0.95f;
                baseMetal = 0.0f;
                break;
            case SurfaceTag::Foliage:
                baseRough = 0.85f;
                baseMetal = 0.0f;
                break;
            default:
                baseRough = 0.85f;
                baseMetal = 0.0f;
                break;
            }

            for( u32 y = 0; y < H; ++y )
            {
                for( u32 x = 0; x < W; ++x )
                {
                    real_Num hv = h.at( x, y );

                    // Approximate AO from local concavity
                    real_Num hL = h.at( x > 0 ? x - 1 : x, y );
                    real_Num hR = h.at( x + 1 < W ? x + 1 : x, y );
                    real_Num hU = h.at( x, y > 0 ? y - 1 : y );
                    real_Num hD = h.at( x, y + 1 < H ? y + 1 : y );
                    real_Num concavity = ( hL + hR + hU + hD ) * 0.25f - hv;
                    real_Num ao = std::max( 0.0f, 0.7f + concavity * 1.5f );

                    // Roughness varies slightly with height (high spots are drier)
                    real_Num rough = WPNoise::clamp01( baseRough + ( hv - 0.5f ) * 0.15f );

                    u8 *px = ormOut.pixel( x, y );
                    px[0] = toByte( ao );
                    px[1] = toByte( rough );
                    px[2] = toByte( baseMetal );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // Linear -> sRGB encoding for albedo
        // ===================================================================
        void WPTextureForge::encodeSRGB( TextureBuffer &albedo )
        {
            const u32 count = albedo.width * albedo.height;
            for( u32 i = 0; i < count; ++i )
            {
                u8 *px = albedo.pixels.data() + i * 4;
                px[0] = srgbToByte( px[0] / 255.0f );
                px[1] = srgbToByte( px[1] / 255.0f );
                px[2] = srgbToByte( px[2] / 255.0f );
            }
        }

        // ===================================================================
        // CONCRETE - Multi-octave FBM, aggregate bumps, formwork lines, pores
        // ===================================================================
        void WPTextureForge::bakeConcrete( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S * 8.0f;
                    const real_Num v = real_Num( y ) / S * 8.0f;

                    real_Num base = mNoise.fbm3( u, v, 0, 5 ) * 0.5f + 0.5f;
                    real_Num detail = mNoise.noise3( u * 4, v * 4, 0 ) * 0.5f + 0.5f;
                    real_Num pores = mNoise.worley2( u * 12, v * 12 );
                    real_Num agg = mNoise.fbm3( u * 20, v * 20, 0, 3 ) * 0.15f;
                    real_Num plank = std::sin( v * 40 ) * 0.02f;

                    real_Num r = base * 0.4f + detail * 0.3f + pores * 0.2f + agg + plank;
                    real_Num g = r * 0.98f;
                    real_Num b = r * 0.95f;
                    real_Num h = base * 0.6f + detail * 0.2f + pores * 0.3f + agg + plank;

                    out.height.at( x, y );  // ensure sizing
                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // PLASTER - FBM base, ridged cracks, water stains
        // ===================================================================
        void WPTextureForge::bakePlaster( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S * 4.0f;
                    const real_Num v = real_Num( y ) / S * 4.0f;

                    real_Num base = mNoise.fbm3( u, v, 0, 4 ) * 0.5f + 0.5f;
                    real_Num cracks = mNoise.ridged2( u * 3, v * 3, 2 ) * 0.4f;
                    real_Num stain = mNoise.fbm3( u * 2, v * 8, 42, 3 ) * 0.5f + 0.5f;

                    real_Num stainMask = WPNoise::smoothstep( 0.4f, 0.6f, stain );
                    real_Num r = base - cracks * 0.2f - stainMask * 0.15f;
                    real_Num g = r * 0.95f;
                    real_Num b = r * 0.90f;
                    real_Num h = base - cracks * 0.3f;

                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // BRICK - brick pattern with mortar joints, color variation, spalled corners
        // ===================================================================
        void WPTextureForge::bakeBrick( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            const real_Num brickH = 0.18f;
            const real_Num brickW = 0.40f;
            const real_Num mortarH = 0.02f;
            const real_Num mortarW = 0.015f;

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S;
                    const real_Num v = real_Num( y ) / S;
                    s32 row = static_cast<s32>( v / brickH );
                    real_Num offset = ( row % 2 ) * brickW * 0.5f;
                    real_Num bx = std::fmod( u + offset, brickW );
                    real_Num by = std::fmod( v, brickH );

                    bool inMortar =
                        bx < mortarW || bx > brickW - mortarW || by < mortarH || by > brickH - mortarH;

                    real_Num r, g, b, h;
                    if( inMortar )
                    {
                        // Mortar - lighter, higher AO
                        r = 0.55f + mNoise.noise3( u * 8, v * 8, 0 ) * 0.1f;
                        g = r * 0.95f;
                        b = r * 0.88f;
                        h = 0.4f;
                    }
                    else
                    {
                        s32 brickId = static_cast<s32>( ( u + offset ) / brickW ) * 100 + row;
                        real_Num brickSeed = mNoise.hash3( brickId, 0, 0 );
                        real_Num surfaceNoise = mNoise.noise3( u * 15, v * 15, brickId );

                        r = 0.45f + brickSeed * 0.25f + surfaceNoise * 0.15f;
                        g = r * 0.85f;
                        b = r * 0.75f;
                        h = 0.5f;

                        // Spalled corners
                        real_Num cornerDist =
                            std::min( std::min( bx, by ), std::min( brickW - bx, brickH - by ) );
                        if( cornerDist < 0.025f )
                        {
                            r *= 0.85f;
                            h = 0.3f;
                        }
                    }

                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // WOOD - grain, knots, rings
        // ===================================================================
        void WPTextureForge::bakeWood( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S;
                    const real_Num v = real_Num( y ) / S;

                    real_Num grain = mNoise.noise3( u * 80, v * 4, 0 ) * 0.5f + 0.5f;
                    real_Num grain2 = mNoise.noise3( u * 200, v * 2, 42 ) * 0.3f;
                    real_Num knots = mNoise.worley2( u * 8, v * 8 );
                    real_Num rings = std::sin( v * 60 + mNoise.noise3( u * 2, v * 2, 0 ) * 4 ) * 0.1f;

                    real_Num knotMask = std::max( 0.0f, 1.0f - knots * 3.0f );
                    real_Num r = 0.45f + grain * 0.25f + knotMask * 0.15f + grain2 * 0.1f;
                    real_Num g = r * 0.7f;
                    real_Num b = r * 0.5f;
                    real_Num h = grain * 0.5f + grain2 * 0.2f + rings + knots * 0.1f;

                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // METAL - scratches, rust, grime
        // ===================================================================
        void WPTextureForge::bakeMetal( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S;
                    const real_Num v = real_Num( y ) / S;

                    real_Num scratches = mNoise.noise3( u * 200, v * 3, 0 ) * 0.5f + 0.5f;
                    real_Num rust = mNoise.worley2( u * 4, v * 4 );
                    real_Num grime = mNoise.fbm3( u * 8, v * 8, 0, 3 ) * 0.5f + 0.5f;

                    real_Num rustMask = std::max( 0.0f, 1.0f - rust * 3.0f ) * 0.4f;
                    real_Num r = 0.65f - scratches * 0.1f + rustMask * 0.2f;
                    real_Num g = 0.65f - scratches * 0.1f + rustMask * 0.05f;
                    real_Num b = 0.70f - scratches * 0.1f;
                    real_Num h = scratches * 0.2f + rust * 0.3f + grime * 0.2f;

                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // ASPHALT - aggregate with wheel ruts
        // ===================================================================
        void WPTextureForge::bakeAsphalt( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S * 16.0f;
                    const real_Num v = real_Num( y ) / S * 16.0f;

                    real_Num base = mNoise.fbm3( u, v, 0, 4 ) * 0.5f + 0.5f;
                    real_Num aggregate = mNoise.noise3( u * 30, v * 30, 0 ) * 0.3f;
                    real_Num rut =
                        std::exp( -( std::sin( v * 0.5f ) * 0.5f * std::sin( v * 0.5f ) * 0.5f ) /
                                  0.1f ) *
                        0.2f;

                    real_Num r = base * 0.15f + aggregate * 0.1f - rut * 0.3f;
                    real_Num g = r;
                    real_Num b = r * 0.98f;
                    real_Num h = base * 0.6f + aggregate * 0.2f - rut;

                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // SAND - wind-blown grain
        // ===================================================================
        void WPTextureForge::bakeSand( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S * 8.0f;
                    const real_Num v = real_Num( y ) / S * 8.0f;

                    real_Num base = mNoise.fbm3( u, v, 0, 4 ) * 0.5f + 0.5f;
                    real_Num grain = mNoise.noise3( u * 60, v * 60, 0 ) * 0.3f;
                    real_Num drift = mNoise.fbm3( u * 3, v * 3, 7, 3 ) * 0.2f;

                    real_Num r = 0.65f + base * 0.2f + grain * 0.1f;
                    real_Num g = r * 0.85f;
                    real_Num b = r * 0.65f;
                    real_Num h = base * 0.4f + grain * 0.4f + drift * 0.3f;

                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // FABRIC - weave pattern
        // ===================================================================
        void WPTextureForge::bakeFabric( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S;
                    const real_Num v = real_Num( y ) / S;

                    real_Num weaveX = std::sin( u * 200.0f ) * 0.5f + 0.5f;
                    real_Num weaveY = std::sin( v * 200.0f ) * 0.5f + 0.5f;
                    real_Num weave = ( weaveX + weaveY ) * 0.25f;
                    real_Num tear = mNoise.fbm3( u * 4, v * 4, 0, 3 ) * 0.2f;
                    real_Num stain = mNoise.worley2( u * 8, v * 8 );

                    real_Num r = 0.6f + weave * 0.1f + stain * 0.2f;
                    real_Num g = r * 0.85f;
                    real_Num b = r * 0.7f;
                    real_Num h = weave + tear;

                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // FOLIAGE - leaf-like
        // ===================================================================
        void WPTextureForge::bakeFoliage( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S * 16.0f;
                    const real_Num v = real_Num( y ) / S * 16.0f;

                    real_Num base = mNoise.fbm3( u, v, 0, 4 ) * 0.5f + 0.5f;
                    real_Num vein = mNoise.noise3( u * 80, v * 5, 0 ) * 0.3f;

                    real_Num r = 0.3f + base * 0.15f + vein * 0.1f;
                    real_Num g = 0.5f + base * 0.25f + vein * 0.1f;
                    real_Num b = 0.2f + base * 0.1f;
                    real_Num h = base * 0.4f + vein * 0.2f;

                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // GLASS - smooth, slightly grime-streaked
        // ===================================================================
        void WPTextureForge::bakeGlass( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S;
                    const real_Num v = real_Num( y ) / S;

                    real_Num base = mNoise.noise3( u * 40, v * 40, 0 ) * 0.5f + 0.5f;
                    real_Num smear = mNoise.noise3( u * 8, v * 8, 0 ) * 0.3f;

                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( 0.95f + base * 0.05f );
                    px[1] = toByte( 0.97f );
                    px[2] = toByte( 1.0f );
                    px[3] = 255;

                    out.height.values[y * S + x] = base * 0.2f + smear * 0.1f;
                }
            }
        }

        // ===================================================================
        // PAINT - smooth with chips
        // ===================================================================
        void WPTextureForge::bakePaint( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S;
                    const real_Num v = real_Num( y ) / S;

                    real_Num base = mNoise.fbm3( u * 8, v * 8, 0, 3 ) * 0.5f + 0.5f;
                    real_Num chips = mNoise.worley2( u * 30, v * 30 );

                    u8 *px = out.albedo.pixel( x, y );
                    real_Num r = 0.7f + base * 0.1f;
                    px[0] = toByte( r );
                    px[1] = toByte( r * 0.9f );
                    px[2] = toByte( r * 0.85f );
                    px[3] = 255;

                    out.height.values[y * S + x] = base * 0.1f + chips * 0.05f;
                }
            }
        }

        // ===================================================================
        // RUBBER - dark, tread-patterned
        // ===================================================================
        void WPTextureForge::bakeRubber( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S;
                    const real_Num v = real_Num( y ) / S;

                    real_Num base = mNoise.noise3( u * 30, v * 30, 0 ) * 0.5f + 0.5f;
                    real_Num tread = std::abs( std::sin( v * 40.0f ) );

                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( 0.1f + base * 0.05f );
                    px[1] = px[0];
                    px[2] = toByte( 0.1f + base * 0.05f ) - 2;
                    px[3] = 255;

                    out.height.values[y * S + x] = tread * 0.2f + base * 0.1f;
                }
            }
        }

        // ===================================================================
        // DIRT - brown, lumpy
        // ===================================================================
        void WPTextureForge::bakeDirt( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S * 6.0f;
                    const real_Num v = real_Num( y ) / S * 6.0f;

                    real_Num base = mNoise.fbm3( u, v, 0, 5 ) * 0.5f + 0.5f;
                    real_Num lumps = mNoise.worley2( u * 3, v * 3 );

                    real_Num r = 0.35f + base * 0.25f;
                    real_Num g = 0.25f + base * 0.15f;
                    real_Num b = 0.15f + base * 0.1f;
                    real_Num h = base * 0.6f + lumps * 0.3f;

                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }

        // ===================================================================
        // STONE - cobbled, weathered
        // ===================================================================
        void WPTextureForge::bakeStone( SurfaceBakeResult &out, const SurfaceBakeParams &p )
        {
            const u32 S = p.size;
            out.albedo = TextureBuffer( S, S );
            out.height = HeightBuffer( S, S );

            for( u32 y = 0; y < S; ++y )
            {
                for( u32 x = 0; x < S; ++x )
                {
                    const real_Num u = real_Num( x ) / S;
                    const real_Num v = real_Num( y ) / S;

                    real_Num stones = mNoise.worley2( u * 6, v * 6 );
                    real_Num grain = mNoise.noise3( u * 60, v * 60, 0 ) * 0.3f;

                    real_Num stoneEdge = WPNoise::smoothstep( 0.0f, 0.1f, stones );
                    real_Num r = 0.5f + ( 1.0f - stoneEdge ) * 0.3f + grain * 0.1f;
                    real_Num g = 0.5f + ( 1.0f - stoneEdge ) * 0.3f + grain * 0.1f;
                    real_Num b = 0.5f + ( 1.0f - stoneEdge ) * 0.3f + grain * 0.1f;
                    real_Num h = stoneEdge * 0.4f + grain * 0.3f;

                    out.height.values[y * S + x] = h;
                    u8 *px = out.albedo.pixel( x, y );
                    px[0] = toByte( r );
                    px[1] = toByte( g );
                    px[2] = toByte( b );
                    px[3] = 255;
                }
            }
        }
    }  // namespace procedural
}  // namespace workphone
