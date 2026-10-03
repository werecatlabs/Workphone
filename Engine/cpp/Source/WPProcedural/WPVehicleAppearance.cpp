// ============================================================================
// WPVehicleAppearance.cpp
// ============================================================================

#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPVehicleAppearance.hpp"
#include "WPProcedural/WPNoise.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            constexpr real_Num kPi = 3.14159265358979323846f;

            real_Num saturate( real_Num v )
            {
                return std::max( 0.0f, std::min( 1.0f, v ) );
            }

            real_Num mix( real_Num a, real_Num b, real_Num t )
            {
                return a + ( b - a ) * t;
            }

            real_Num smooth( real_Num a, real_Num b, real_Num x )
            {
                if( std::abs( b - a ) < 1e-8f )
                    return x >= b ? 1.0f : 0.0f;
                const real_Num t = saturate( ( x - a ) / ( b - a ) );
                return t * t * ( 3.0f - 2.0f * t );
            }

            real_Num srgbEncode( real_Num x )
            {
                x = saturate( x );
                return x <= 0.0031308f ? x * 12.92f : 1.055f * std::pow( x, 1.0f / 2.4f ) - 0.055f;
            }

            u8 byteLinear( real_Num x )
            {
                return static_cast<u8>( saturate( x ) * 255.0f + 0.5f );
            }

            u8 byteSRGB( real_Num x )
            {
                return byteLinear( srgbEncode( x ) );
            }

            VehicleLinearColor blend( const VehicleLinearColor &a, const VehicleLinearColor &b,
                                      real_Num t )
            {
                return { mix( a.r, b.r, t ), mix( a.g, b.g, t ), mix( a.b, b.b, t ),
                         mix( a.a, b.a, t ) };
            }

            void writeColor( TextureBuffer &texture, u32 x, u32 y, const VehicleLinearColor &c,
                             bool srgb )
            {
                u8 *p = texture.pixel( x, y );
                p[0] = srgb ? byteSRGB( c.r ) : byteLinear( c.r );
                p[1] = srgb ? byteSRGB( c.g ) : byteLinear( c.g );
                p[2] = srgb ? byteSRGB( c.b ) : byteLinear( c.b );
                p[3] = byteLinear( c.a );
            }

            u32 boundedDimension( u32 value, u32 cap )
            {
                if( cap == 0 )
                    return value;
                return std::max<u32>( 64, std::min( value, cap ) );
            }

            real_Num rectangleCoverage( real_Num px, real_Num py, real_Num x0, real_Num y0, real_Num x1,
                                        real_Num y1, real_Num feather )
            {
                const real_Num dx = std::max( std::max( x0 - px, 0.0f ), px - x1 );
                const real_Num dy = std::max( std::max( y0 - py, 0.0f ), py - y1 );
                return 1.0f - smooth( 0.0f, feather, std::sqrt( dx * dx + dy * dy ) );
            }

            real_Num digitCoverage( real_Num u, real_Num v, u32 digit, real_Num x, real_Num y,
                                    real_Num w, real_Num h, real_Num feather )
            {
                static const u8 masks[10] = {
                    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f
                };
                const real_Num t = w * 0.14f;
                real_Num result = 0.0f;
                const auto add = [&]( u32 bit, real_Num ax, real_Num ay, real_Num bx,
                                      real_Num by ) -> real_Num {
                    return ( masks[digit % 10] & ( 1u << bit ) )
                               ? rectangleCoverage( u, v, ax, ay, bx, by, feather )
                               : 0.0f;
                };
                result = std::max( result, add( 0, x + t, y, x + w - t, y + t ) );
                result = std::max( result, add( 1, x + w - t, y + t, x + w, y + h * 0.5f - t * 0.3f ) );
                result =
                    std::max( result, add( 2, x + w - t, y + h * 0.5f + t * 0.3f, x + w, y + h - t ) );
                result = std::max( result, add( 3, x + t, y + h - t, x + w - t, y + h ) );
                result = std::max( result, add( 4, x, y + h * 0.5f + t * 0.3f, x + t, y + h - t ) );
                result = std::max( result, add( 5, x, y + t, x + t, y + h * 0.5f - t * 0.3f ) );
                result = std::max( result, add( 6, x + t, y + h * 0.5f - t * 0.5f, x + w - t,
                                                y + h * 0.5f + t * 0.5f ) );
                return result;
            }

            void heightToNormal( const HeightBuffer &height, TextureBuffer &normal, real_Num strength,
                                 bool wrapU, bool wrapV )
            {
                normal = TextureBuffer( height.width, height.height );
                if( height.width == 0 || height.height == 0 )
                    return;
                for( u32 y = 0; y < height.height; ++y )
                {
                    const u32 ym = y == 0 ? ( wrapV ? height.height - 1 : 0 ) : y - 1;
                    const u32 yp = y + 1 < height.height ? y + 1 : ( wrapV ? 0 : height.height - 1 );
                    for( u32 x = 0; x < height.width; ++x )
                    {
                        const u32 xm = x == 0 ? ( wrapU ? height.width - 1 : 0 ) : x - 1;
                        const u32 xp = x + 1 < height.width ? x + 1 : ( wrapU ? 0 : height.width - 1 );
                        real_Num nx = -( height.values[y * height.width + xp] -
                                         height.values[y * height.width + xm] ) *
                                      strength;
                        real_Num ny = -( height.values[yp * height.width + x] -
                                         height.values[ym * height.width + x] ) *
                                      strength;
                        real_Num nz = 1.0f;
                        const real_Num inv =
                            1.0f / std::max( 1e-8f, std::sqrt( nx * nx + ny * ny + 1.0f ) );
                        nx *= inv;
                        ny *= inv;
                        nz *= inv;
                        u8 *p = normal.pixel( x, y );
                        p[0] = byteLinear( nx * 0.5f + 0.5f );
                        p[1] = byteLinear( ny * 0.5f + 0.5f );
                        p[2] = byteLinear( nz * 0.5f + 0.5f );
                        p[3] = 255;
                    }
                }
            }

            void generateLivery( const VehicleAppearanceConfig &config,
                                 const VehicleAppearanceQualityProfile &profile, TextureBuffer &out,
                                 const WPNoise &noise )
            {
                const u32 w = profile.liveryWidth;
                const u32 h = profile.liveryHeight;
                out = TextureBuffer( w, h );
                const u32 number = std::min<u32>( config.vehicleNumber, 999 );
                const u32 digits = number >= 100 ? 3 : ( number >= 10 ? 2 : 1 );
                const real_Num fw = 1.25f / static_cast<real_Num>( std::max( w, h ) );

                for( u32 py = 0; py < h; ++py )
                {
                    for( u32 px = 0; px < w; ++px )
                    {
                        VehicleLinearColor accumulated{0,0,0,0};
                        const u32 samples = profile.supersample > 1 ? 4 : 1;
                        for( u32 s = 0; s < samples; ++s )
                        {
                            const real_Num ox = samples == 1 ? 0.5f : ( ( s & 1u ) ? 0.75f : 0.25f );
                            const real_Num oy = samples == 1 ? 0.5f : ( ( s & 2u ) ? 0.75f : 0.25f );
                            const real_Num u = ( static_cast<real_Num>( px ) + ox ) / w;
                            const real_Num v = ( static_cast<real_Num>( py ) + oy ) / h;
                            const real_Num polish = noise.fbm2( u * 5.0f, v * 5.0f, 2 ) * 0.012f;
                            VehicleLinearColor c = config.primary;
                            c.r = saturate( c.r + polish );
                            c.g = saturate( c.g + polish );
                            c.b = saturate( c.b + polish );

                            // A dark spine follows the upper body and narrows into the nose.
                            const real_Num spineWidth = mix( 0.022f, 0.105f, smooth( 0.05f, 0.55f, u ) );
                            const real_Num spine =
                                1.0f - smooth( spineWidth, spineWidth + fw, std::abs( v - 0.25f ) );
                            c = blend( c, config.secondary, spine );

                            // An accent speed-line bends around the cockpit and sidepods.
                            const real_Num center =
                                0.25f + 0.105f * std::sin( ( u * 1.25f - 0.18f ) * kPi );
                            const real_Num ribbon =
                                1.0f - smooth( 0.012f, 0.012f + fw, std::abs( v - center ) );
                            c = blend( c, config.accent, ribbon );

                            // Exposed floor band in shared body UV space.
                            const real_Num floorBand =
                                smooth( 0.575f, 0.59f, v ) * ( 1.0f - smooth( 0.91f, 0.925f, v ) );
                            c = blend( c, VehicleLinearColor{ 0.012f, 0.013f, 0.014f, 1.0f },
                                       floorBand );

                            // High-contrast race number on both flanks, with analytic antialiasing.
                            real_Num ink = 0.0f;
                            for( u32 side = 0; side < 2; ++side )
                            {
                                const real_Num y0 = side == 0 ? 0.055f : 0.365f;
                                const real_Num digitW = 0.034f;
                                const real_Num gap = 0.007f;
                                const real_Num start =
                                    0.57f - ( digitW * digits + gap * ( digits - 1 ) ) * 0.5f;
                                for( u32 d = 0; d < digits; ++d )
                                {
                                    u32 divisor = 1;
                                    for( u32 k = d + 1; k < digits; ++k )
                                        divisor *= 10;
                                    const u32 value = ( number / divisor ) % 10;
                                    ink = std::max(
                                        ink, digitCoverage( u, v, value, start + d * ( digitW + gap ),
                                                            y0, digitW, 0.075f, fw ) );
                                }
                            }
                            const real_Num panel = std::max(
                                rectangleCoverage( u, v, 0.535f, 0.042f, 0.605f, 0.142f, fw * 2.0f ),
                                rectangleCoverage( u, v, 0.535f, 0.352f, 0.605f, 0.452f, fw * 2.0f ) );
                            c = blend( c, VehicleLinearColor{ 0.86f, 0.86f, 0.84f, 1.0f },
                                       panel * 0.92f );
                            c = blend( c, config.secondary, ink );
                            accumulated.r += c.r;
                            accumulated.g += c.g;
                            accumulated.b += c.b;
                        }
                        accumulated.r /= samples;
                        accumulated.g /= samples;
                        accumulated.b /= samples;
                        accumulated.a = 1.0f;
                        writeColor( out, px, py, accumulated, true );
                    }
                }
            }

            void generateBodySurface( const VehicleAppearanceConfig &config,
                                      const VehicleAppearanceQualityProfile &profile,
                                      VehicleTextureAssets &textures, const WPNoise &noise )
            {
                const u32 w = profile.bodySurfaceWidth;
                const u32 h = profile.bodySurfaceHeight;
                HeightBuffer relief( w, h );
                textures.bodyORM = TextureBuffer( w, h );
                textures.bodyClearcoat = TextureBuffer( w, h );
                const real_Num panelLines[] = { 0.115f, 0.285f, 0.455f, 0.665f, 0.835f };
                for( u32 y = 0; y < h; ++y )
                {
                    const real_Num v = ( y + 0.5f ) / h;
                    for( u32 x = 0; x < w; ++x )
                    {
                        const real_Num u = ( x + 0.5f ) / w;
                        real_Num height = 0.5f + noise.fbm2( u * 2.5f, v * 2.5f, 1 ) * 0.0015f;
                        real_Num joint = 0.0f;
                        for( real_Num line : panelLines )
                        {
                            const real_Num d = std::abs( u - line ) * w;
                            joint = std::max( joint, 1.0f - smooth( 0.6f, 2.4f, d ) );
                        }
                        const real_Num floorJoint =
                            std::max( 1.0f - smooth( 0.6f, 2.5f, std::abs( v - 0.585f ) * h ),
                                      1.0f - smooth( 0.6f, 2.5f, std::abs( v - 0.915f ) * h ) );
                        joint = std::max( joint, floorJoint );
                        height -= joint * 0.09f;
                        relief.values[y * w + x] = height;

                        const real_Num under =
                            smooth( 0.575f, 0.595f, v ) * ( 1.0f - smooth( 0.905f, 0.925f, v ) );
                        const real_Num upperDust = std::exp( -( v - 0.25f ) * ( v - 0.25f ) / 0.014f );
                        const real_Num low = std::max( smooth( 0.40f, 0.59f, v ) * ( 1.0f - under ),
                                                       smooth( 0.91f, 1.0f, v ) );
                        const real_Num grimeNoise =
                            saturate( noise.fbm2( u * 90.0f, v * 90.0f, 2 ) * 0.5f + 0.5f );
                        const real_Num grime =
                            config.wear * saturate( low * 0.75f + upperDust * 0.18f +
                                                    grimeNoise * 0.22f + joint * 0.4f );
                        const real_Num wet = saturate( config.wetness );
                        const real_Num ao =
                            saturate( 1.0f - under * 0.28f - joint * 0.16f - grime * 0.1f );
                        real_Num rough = mix( 0.34f, 0.48f, under ) + grime * 0.34f +
                                         noise.fbm2( u * 55.0f, v * 55.0f, 2 ) * 0.025f;
                        rough = mix( rough, std::max( 0.07f, rough * 0.28f ), wet );
                        u8 *orm = textures.bodyORM.pixel( x, y );
                        orm[0] = byteLinear( ao );
                        orm[1] = byteLinear( rough );
                        orm[2] = byteLinear( mix( saturate( config.metallicBasecoat ), 0.08f, under ) );
                        orm[3] = 255;

                        const real_Num dryCoatRough = mix( 0.032f, 0.15f, under );
                        const real_Num wetCoatRough = mix( 0.018f, 0.09f, under );
                        const real_Num coatAmount =
                            saturate( mix( 0.96f, 0.58f, under ) - grime * 0.4f );
                        const real_Num coatRough =
                            saturate( mix( dryCoatRough, wetCoatRough, wet ) + grime * 0.35f );
                        u8 *cc = textures.bodyClearcoat.pixel( x, y );
                        cc[0] = byteLinear( coatAmount );
                        cc[1] = byteLinear( coatRough );
                        cc[2] = 0;
                        cc[3] = 255;
                    }
                }
                // Body u terminates at unrelated nose/tail structures; only circumference v wraps.
                heightToNormal( relief, textures.bodyNormal, 3.4f, false, true );
            }

            void generatePaintCoat( u32 size, TextureBuffer &normal, const WPNoise &noise )
            {
                HeightBuffer h( size, size );
                for( u32 y = 0; y < size; ++y )
                {
                    for( u32 x = 0; x < size; ++x )
                    {
                        const real_Num u = ( x + 0.5f ) / size;
                        const real_Num v = ( y + 0.5f ) / size;
                        real_Num value = noise.fbm2( u * 6.0f, v * 6.0f, 2 ) * 0.54f +
                                         noise.fbm2( u * 34.0f, v * 34.0f, 2 ) * 0.22f;
                        const real_Num cell = noise.worley2( u * 48.0f, v * 48.0f );
                        if( cell < 0.085f )
                            value += ( 0.085f - cell ) * 1.8f;
                        h.values[y * size + x] = value;
                    }
                }
                heightToNormal( h, normal, 0.14f, true, true );
            }

            void generateCarbon( u32 size, VehicleTextureAssets &textures, const WPNoise &noise )
            {
                textures.carbonAlbedo = TextureBuffer( size, size );
                textures.carbonORM = TextureBuffer( size, size );
                textures.carbonAnisotropy = TextureBuffer( size, size );
                HeightBuffer height( size, size );
                constexpr real_Num tows = 50.0f;
                for( u32 y = 0; y < size; ++y )
                {
                    const real_Num v = ( y + 0.5f ) / size;
                    for( u32 x = 0; x < size; ++x )
                    {
                        const real_Num u = ( x + 0.5f ) / size;
                        const real_Num su = u * tows;
                        const real_Num sv = v * tows;
                        const s32 cu = static_cast<s32>( std::floor( su ) );
                        const s32 cv = static_cast<s32>( std::floor( sv ) );
                        const real_Num fu = su - std::floor( su );
                        const real_Num fv = sv - std::floor( sv );
                        const s32 phase = ( ( cv - cu ) % 4 + 4 ) % 4;
                        const bool warp = phase < 2;
                        const real_Num across = warp ? fu : fv;
                        const real_Num along = warp ? fv : fu;
                        const real_Num bulge =
                            std::pow( std::max( 0.0f, std::sin( kPi * across ) ), 0.65f );
                        const real_Num dip =
                            1.0f - 0.16f * std::abs( ( phase + along ) * 0.5f - 0.5f ) * 2.0f;
                        const real_Num filament =
                            noise.fbm2( warp ? u * 3.0f : v * 3.0f, warp ? v * 380.0f : u * 380.0f, 2 );
                        const real_Num grain = noise.fbm2( u * 9.0f, v * 9.0f, 3 );
                        const real_Num luminance =
                            0.026f + bulge * 0.014f * dip + filament * 0.0028f + grain * 0.002f;
                        writeColor( textures.carbonAlbedo, x, y,
                                    { luminance * 1.01f, luminance, luminance * 0.985f, 1.0f }, true );
                        height.values[y * size + x] = bulge * dip * 0.86f + filament * 0.035f;
                        const real_Num edge = 1.0f - bulge;
                        u8 *orm = textures.carbonORM.pixel( x, y );
                        orm[0] = byteLinear( 0.58f + bulge * 0.42f );
                        orm[1] = byteLinear( 0.23f + edge * 0.40f - filament * 0.035f + grain * 0.025f );
                        orm[2] = byteLinear( 0.10f + bulge * 0.16f );
                        orm[3] = 255;
                        u8 *aniso = textures.carbonAnisotropy.pixel( x, y );
                        aniso[0] = byteLinear( warp ? 0.5f : 1.0f );
                        aniso[1] = byteLinear( warp ? 1.0f : 0.5f );
                        aniso[2] = byteLinear( 0.38f + bulge * 0.60f );
                        aniso[3] = 255;
                    }
                }
                heightToNormal( height, textures.carbonNormal, 2.2f, true, true );
            }

            void generateRubber( u32 size, VehicleTextureAssets &textures, const WPNoise &noise,
                                 real_Num wear, real_Num wetness )
            {
                textures.rubberAlbedo = TextureBuffer( size, size );
                textures.rubberORM = TextureBuffer( size, size );
                HeightBuffer height( size, size );
                for( u32 y = 0; y < size; ++y )
                {
                    for( u32 x = 0; x < size; ++x )
                    {
                        const real_Num u = ( x + 0.5f ) / size;
                        const real_Num v = ( y + 0.5f ) / size;
                        const real_Num grain = noise.fbm2( u * 70.0f, v * 70.0f, 3 );
                        const real_Num abrasion = std::abs( noise.fbm2( u * 11.0f, v * 2.0f, 2 ) );
                        // A dry Grand Prix tyre is a slick. Detail belongs to rubber grain, a faint
                        // centre mould seam, pickup and sidewall embossing -- never tread grooves.
                        const real_Num centreSeam =
                            1.0f - smooth( 0.0025f, 0.009f, std::abs( v - 0.5f ) );
                        const real_Num shoulder =
                            std::max( 1.0f - smooth( 0.13f, 0.22f, v ), smooth( 0.78f, 0.87f, v ) );
                        const real_Num ring =
                            std::max( 1.0f - smooth( 0.007f, 0.016f, std::abs( v - 0.105f ) ),
                                      1.0f - smooth( 0.007f, 0.016f, std::abs( v - 0.895f ) ) );
                        // Repeating raised blocks suggest moulded sidewall lettering at normal viewing
                        // distance without baking a trademark or depending on a font rasterizer.
                        const real_Num cell = u * 18.0f - std::floor( u * 18.0f );
                        const real_Num letterStroke = shoulder * smooth( 0.12f, 0.18f, cell ) *
                                                      ( 1.0f - smooth( 0.62f, 0.72f, cell ) );
                        real_Num l = saturate( 0.010f + grain * 0.003f + abrasion * wear * 0.012f +
                                               shoulder * 0.0025f );
                        VehicleLinearColor rubberColor{ l, l * 0.99f, l * 1.02f, 1.0f };
                        rubberColor = blend( rubberColor, { 0.58f, 0.32f, 0.012f, 1.0f }, ring * 0.78f );
                        writeColor( textures.rubberAlbedo, x, y, rubberColor, true );
                        height.values[y * size + x] = grain * 0.10f + centreSeam * 0.025f +
                                                      letterStroke * 0.035f + abrasion * wear * 0.04f;
                        u8 *orm = textures.rubberORM.pixel( x, y );
                        orm[0] = byteLinear( 0.91f - letterStroke * 0.04f );
                        orm[1] = byteLinear(
                            mix( 0.88f - wear * abrasion * 0.18f - ring * 0.06f, 0.16f, wetness ) );
                        orm[2] = 0;
                        orm[3] = 255;
                    }
                }
                heightToNormal( height, textures.rubberNormal, 1.1f, true, true );
            }

            void generateBrushedMetal( u32 size, VehicleTextureAssets &textures, const WPNoise &noise )
            {
                textures.brushedMetalORM = TextureBuffer( size, size );
                HeightBuffer h( size, size );
                for( u32 y = 0; y < size; ++y )
                {
                    for( u32 x = 0; x < size; ++x )
                    {
                        const real_Num u = ( x + 0.5f ) / size;
                        const real_Num v = ( y + 0.5f ) / size;
                        const real_Num fine = noise.fbm2( u * 6.0f, v * 420.0f, 2 );
                        const real_Num broad = noise.fbm2( u * 2.0f, v * 38.0f, 2 );
                        h.values[y * size + x] = fine * 0.13f + broad * 0.06f;
                        u8 *orm = textures.brushedMetalORM.pixel( x, y );
                        orm[0] = 255;
                        orm[1] = byteLinear( 0.27f + std::abs( fine ) * 0.12f );
                        orm[2] = 255;
                        orm[3] = 255;
                    }
                }
                heightToNormal( h, textures.brushedMetalNormal, 0.8f, true, true );
            }

            void generateLight( u32 size, TextureBuffer &out )
            {
                out = TextureBuffer( size, size );
                for( u32 y = 0; y < size; ++y )
                {
                    for( u32 x = 0; x < size; ++x )
                    {
                        const real_Num u = ( x + 0.5f ) / size * 2.0f - 1.0f;
                        const real_Num v = ( y + 0.5f ) / size * 2.0f - 1.0f;
                        const real_Num lens = saturate( 1.0f - std::sqrt( u * u + v * v ) );
                        const real_Num led = std::pow(
                            std::max( 0.0f, std::cos( u * kPi * 8.0f ) * std::cos( v * kPi * 4.0f ) ),
                            12.0f );
                        // Emission data is documented as a linear map; do not apply display transfer.
                        writeColor( out, x, y,
                                    { saturate( lens * 0.65f + led ), lens * 0.025f, lens * 0.006f,
                                      lens > 0.0f ? 1.0f : 0.0f },
                                    false );
                    }
                }
            }

            VehicleMaterialDescriptor makeMaterial( const char *name, VehicleLinearColor color,
                                                    real_Num roughness, real_Num metallic )
            {
                VehicleMaterialDescriptor d;
                d.name = name;
                d.baseColor = color;
                d.fallbackBaseColor = color;
                d.roughness = roughness;
                d.fallbackRoughness = roughness;
                d.metallic = metallic;
                return d;
            }

            void makeMaterials( const VehicleAppearanceConfig &c, VehicleAppearanceBundle &bundle )
            {
                auto &m = bundle.materials;
                m[static_cast<size_t>( VehicleMaterialSlot::BodyPaint )] =
                    makeMaterial( "vehicle.bodyPaint", { 1, 1, 1, 1 }, 1.0f, c.metallicBasecoat );
                auto &paint = m[static_cast<size_t>( VehicleMaterialSlot::BodyPaint )];
                // White/1.0 are texture multipliers; these fallbacks are used only if map upload fails.
                paint.fallbackBaseColor = c.primary;
                paint.fallbackRoughness = mix( 0.34f, 0.11f, c.wetness );
                paint.clearcoat = 1.0f;
                paint.clearcoatRoughness = 1.0f;
                paint.normalScale = 0.65f;
                paint.clearcoatNormalScale = 0.34f;

                m[static_cast<size_t>( VehicleMaterialSlot::SecondaryPaint )] =
                    makeMaterial( "vehicle.secondaryPaint", c.secondary, 0.34f, 0.32f );
                auto &secondary = m[static_cast<size_t>( VehicleMaterialSlot::SecondaryPaint )];
                secondary.clearcoat = 1.0f;
                secondary.clearcoatRoughness = 0.035f;

                m[static_cast<size_t>( VehicleMaterialSlot::CarbonGloss )] =
                    makeMaterial( "vehicle.carbonGloss", { 1, 1, 1, 1 }, 1.0f, 1.0f );
                auto &carbon = m[static_cast<size_t>( VehicleMaterialSlot::CarbonGloss )];
                carbon.fallbackBaseColor = { 0.028f, 0.029f, 0.029f, 1.0f };
                carbon.fallbackRoughness = 0.31f;
                carbon.clearcoat = 1.0f;
                carbon.clearcoatRoughness = 0.055f;
                carbon.anisotropy = 0.95f;
                carbon.normalScale = 1.15f;

                m[static_cast<size_t>( VehicleMaterialSlot::CarbonMatte )] = carbon;
                auto &carbonMatte = m[static_cast<size_t>( VehicleMaterialSlot::CarbonMatte )];
                carbonMatte.name = "vehicle.carbonMatte";
                carbonMatte.baseColor = { 0.365f, 0.385f, 0.425f, 1.0f };
                carbonMatte.fallbackBaseColor = { 0.013f, 0.014f, 0.016f, 1.0f };
                carbonMatte.fallbackRoughness = 0.55f;
                carbonMatte.clearcoat = 0.8f;
                carbonMatte.clearcoatRoughness = 0.12f;
                carbonMatte.anisotropy = 0.82f;

                m[static_cast<size_t>( VehicleMaterialSlot::AnodizedAccent )] =
                    makeMaterial( "vehicle.anodizedAccent", c.accent, 0.24f, 0.85f );
                auto &accent = m[static_cast<size_t>( VehicleMaterialSlot::AnodizedAccent )];
                accent.clearcoat = 0.6f;
                accent.clearcoatRoughness = 0.1f;

                m[static_cast<size_t>( VehicleMaterialSlot::BareMetal )] =
                    makeMaterial( "vehicle.bareMetal", { 0.55f, 0.58f, 0.62f, 1 }, 0.28f, 1.0f );
                m[static_cast<size_t>( VehicleMaterialSlot::BareMetal )].anisotropy = 0.45f;
                m[static_cast<size_t>( VehicleMaterialSlot::DarkMetal )] =
                    makeMaterial( "vehicle.darkMetal", { 0.047f, 0.051f, 0.061f, 1 }, 0.41f, 1.0f );
                m[static_cast<size_t>( VehicleMaterialSlot::Titanium )] =
                    makeMaterial( "vehicle.titanium", { 0.33f, 0.29f, 0.25f, 1 }, 0.34f, 1.0f );
                m[static_cast<size_t>( VehicleMaterialSlot::Titanium )].anisotropy = 0.8f;

                m[static_cast<size_t>( VehicleMaterialSlot::Exhaust )] =
                    makeMaterial( "vehicle.inconelExhaust", { 0.068f, 0.058f, 0.050f, 1 }, 0.36f, 1.0f );
                auto &exhaust = m[static_cast<size_t>( VehicleMaterialSlot::Exhaust )];
                exhaust.iridescence = 0.85f;
                exhaust.iridescenceIOR = 1.9f;
                exhaust.iridescenceThicknessMinNm = 180.0f;
                exhaust.iridescenceThicknessMaxNm = 720.0f;

                m[static_cast<size_t>( VehicleMaterialSlot::Rubber )] =
                    makeMaterial( "vehicle.rubber", { 0.008f, 0.008f, 0.010f, 1 }, 1.0f, 0.0f );
                auto &rubber = m[static_cast<size_t>( VehicleMaterialSlot::Rubber )];
                rubber.sheen = 0.35f;
                rubber.sheenRoughness = 0.9f;

                m[static_cast<size_t>( VehicleMaterialSlot::Glass )] =
                    makeMaterial( "vehicle.glass", { 0.007f, 0.012f, 0.020f, 1 }, 0.05f, 0.0f );
                auto &glass = m[static_cast<size_t>( VehicleMaterialSlot::Glass )];
                glass.transparent = true;
                glass.doubleSided = true;
                glass.depthWrite = false;
                glass.opacity = 0.55f;
                glass.transmission = 0.62f;
                glass.indexOfRefraction = 1.52f;
                glass.iridescence = 0.55f;
                glass.iridescenceIOR = 1.35f;
                glass.iridescenceThicknessMinNm = 120.0f;
                glass.iridescenceThicknessMaxNm = 460.0f;
                glass.clearcoat = 1.0f;
                glass.clearcoatRoughness = 0.02f;

                m[static_cast<size_t>( VehicleMaterialSlot::CockpitTrim )] =
                    makeMaterial( "vehicle.cockpitTrim", { 0.005f, 0.006f, 0.008f, 1 }, 0.94f, 0.0f );
                m[static_cast<size_t>( VehicleMaterialSlot::CockpitTrim )].sheen = 0.5f;
                m[static_cast<size_t>( VehicleMaterialSlot::CockpitTrim )].sheenRoughness = 0.85f;
                m[static_cast<size_t>( VehicleMaterialSlot::BrakeDisc )] =
                    makeMaterial( "vehicle.brakeDisc", { 0.027f, 0.023f, 0.019f, 1 }, 0.62f, 0.25f );
                auto &disc = m[static_cast<size_t>( VehicleMaterialSlot::BrakeDisc )];
                disc.emissiveColor = { 1.0f, 0.033f, 0.0f, 1.0f };
                disc.emissiveIntensity = 0.0f;

                m[static_cast<size_t>( VehicleMaterialSlot::BrakeDuct )] = carbonMatte;
                m[static_cast<size_t>( VehicleMaterialSlot::BrakeDuct )].name = "vehicle.brakeDuct";
                m[static_cast<size_t>( VehicleMaterialSlot::Caliper )] =
                    makeMaterial( "vehicle.caliper", c.accent, 0.3f, 0.9f );
                m[static_cast<size_t>( VehicleMaterialSlot::Caliper )].clearcoat = 0.4f;
                m[static_cast<size_t>( VehicleMaterialSlot::Caliper )].clearcoatRoughness = 0.2f;

                m[static_cast<size_t>( VehicleMaterialSlot::Headlight )] =
                    makeMaterial( "vehicle.headlight", { 0.82f, 0.88f, 1.0f, 1 }, 0.12f, 0.0f );
                auto &headlight = m[static_cast<size_t>( VehicleMaterialSlot::Headlight )];
                headlight.emissiveColor = { 0.86f, 0.92f, 1.0f, 1 };
                headlight.emissiveIntensity = 2.0f;

                m[static_cast<size_t>( VehicleMaterialSlot::TailLight )] =
                    makeMaterial( "vehicle.tailLight", { 0.12f, 0.001f, 0.001f, 1 }, 0.18f, 0.0f );
                auto &tailLight = m[static_cast<size_t>( VehicleMaterialSlot::TailLight )];
                tailLight.emissiveColor = { 1.0f, 0.004f, 0.0f, 1 };
                tailLight.emissiveIntensity = 1.4f;

                m[static_cast<size_t>( VehicleMaterialSlot::RainLight )] =
                    makeMaterial( "vehicle.rainLight", { 0.01f, 0.0009f, 0.0009f, 1 }, 0.22f, 0.0f );
                auto &light = m[static_cast<size_t>( VehicleMaterialSlot::RainLight )];
                light.emissiveColor = { 1.0f, 0.006f, 0.0f, 1 };
                light.emissiveIntensity = 1.0f;

                m[static_cast<size_t>( VehicleMaterialSlot::Decal )] =
                    makeMaterial( "vehicle.decal", { 1, 1, 1, 1 }, 0.28f, 0.0f );
                auto &decal = m[static_cast<size_t>( VehicleMaterialSlot::Decal )];
                decal.transparent = true;
                decal.depthWrite = false;
            }

            u64 hashBytes( u64 hash, const void *data, size_t count )
            {
                const auto *bytes = static_cast<const u8 *>( data );
                for( size_t i = 0; i < count; ++i )
                {
                    hash ^= bytes[i];
                    hash *= 1099511628211ull;
                }
                return hash;
            }

            u64 bundleHash( const VehicleAppearanceBundle &bundle )
            {
                u64 hash = 1469598103934665603ull;
                hash = hashBytes( hash, &bundle.seed, sizeof( bundle.seed ) );
                const TextureBuffer *textures[] = { &bundle.textures.bodyLivery,
                                                    &bundle.textures.bodyNormal,
                                                    &bundle.textures.bodyORM,
                                                    &bundle.textures.bodyClearcoat,
                                                    &bundle.textures.paintCoatNormal,
                                                    &bundle.textures.carbonAlbedo,
                                                    &bundle.textures.carbonNormal,
                                                    &bundle.textures.carbonORM,
                                                    &bundle.textures.carbonAnisotropy,
                                                    &bundle.textures.rubberAlbedo,
                                                    &bundle.textures.rubberNormal,
                                                    &bundle.textures.rubberORM,
                                                    &bundle.textures.brushedMetalNormal,
                                                    &bundle.textures.brushedMetalORM,
                                                    &bundle.textures.lightEmission };
                for( const TextureBuffer *texture : textures )
                {
                    hash = hashBytes( hash, &texture->width, sizeof( texture->width ) );
                    hash = hashBytes( hash, &texture->height, sizeof( texture->height ) );
                    if( !texture->pixels.empty() )
                        hash = hashBytes( hash, texture->pixels.data(), texture->pixels.size() );
                }
                for( const auto &material : bundle.materials )
                {
                    hash = hashBytes( hash, material.name.data(), material.name.size() );
                    const real_Num values[] = { material.baseColor.r,
                                                material.baseColor.g,
                                                material.baseColor.b,
                                                material.fallbackBaseColor.r,
                                                material.fallbackBaseColor.g,
                                                material.fallbackBaseColor.b,
                                                material.roughness,
                                                material.fallbackRoughness,
                                                material.metallic,
                                                material.clearcoat,
                                                material.clearcoatRoughness,
                                                material.anisotropy,
                                                material.transmission,
                                                material.opacity,
                                                material.iridescence };
                    hash = hashBytes( hash, values, sizeof( values ) );
                }
                return hash;
            }

            bool validBuffer( const TextureBuffer &buffer )
            {
                if( buffer.width == 0 || buffer.height == 0 )
                    return false;
                const size_t expected = static_cast<size_t>( buffer.width ) * buffer.height * 4u;
                return expected / 4u / buffer.width == buffer.height && buffer.pixels.size() == expected;
            }

            void addIssue( VehicleAppearanceValidation &result, VehicleAppearanceIssueSeverity severity,
                           const char *code, const char *message )
            {
                result.issues.push_back( { severity, code, message } );
                if( severity == VehicleAppearanceIssueSeverity::Error )
                    result.valid = false;
            }
        }  // namespace





        WPVehicleAppearance::WPVehicleAppearance( u32 seed ) : mSeed( seed )
        {
        }

        VehicleAppearanceQualityProfile WPVehicleAppearance::profileFor(
            VehicleAppearanceQuality quality )
        {
            switch( quality )
            {
            case VehicleAppearanceQuality::Preview:
                return { 256, 128, 256, 128, 128, 1, 4 };
            case VehicleAppearanceQuality::Standard:
                return { 512, 256, 512, 256, 256, 1, 8 };
            case VehicleAppearanceQuality::High:
                return { 1024, 512, 1024, 512, 512, 1, 16 };
            case VehicleAppearanceQuality::Cinematic:
                return { 2048, 1024, 2048, 1024, 1024, 2, 16 };
            default:
                return { 512, 256, 512, 256, 256, 1, 8 };
            }
        }

        VehicleMaterialSlot WPVehicleAppearance::slotForGeometryMaterial( VehicleMaterial material )
        {
            switch( material )
            {
            case VehicleMaterial::Paint:
                return VehicleMaterialSlot::BodyPaint;
            case VehicleMaterial::CarbonGloss:
                return VehicleMaterialSlot::CarbonGloss;
            case VehicleMaterial::CarbonMatte:
                return VehicleMaterialSlot::CarbonMatte;
            case VehicleMaterial::Glass:
                return VehicleMaterialSlot::Glass;
            case VehicleMaterial::Headlight:
                return VehicleMaterialSlot::Headlight;
            case VehicleMaterial::TailLight:
                return VehicleMaterialSlot::TailLight;
            case VehicleMaterial::RainLight:
                return VehicleMaterialSlot::RainLight;
            case VehicleMaterial::Tyre:
                return VehicleMaterialSlot::Rubber;
            case VehicleMaterial::Rim:
                return VehicleMaterialSlot::BareMetal;
            case VehicleMaterial::Brake:
                return VehicleMaterialSlot::BrakeDisc;
            case VehicleMaterial::Caliper:
                return VehicleMaterialSlot::Caliper;
            case VehicleMaterial::BrakeDuct:
                return VehicleMaterialSlot::BrakeDuct;
            case VehicleMaterial::Decal:
                return VehicleMaterialSlot::Decal;
            case VehicleMaterial::Suspension:
                return VehicleMaterialSlot::DarkMetal;
            default:
                return VehicleMaterialSlot::BodyPaint;
            }
        }

        VehicleAppearanceBundle WPVehicleAppearance::build( const VehicleAppearanceConfig &config ) const
        {
            VehicleAppearanceConfig c = config;
            // Hash-combine the per-generator and per-vehicle seeds. A plain XOR would collapse
            // identical defaults to zero and make the most common construction path less random.
            c.seed =
                config.seed ^ ( mSeed + 0x9e3779b9u + ( config.seed << 6u ) + ( config.seed >> 2u ) );
            c.wear = saturate( c.wear );
            c.wetness = saturate( c.wetness );
            c.metallicBasecoat = saturate( c.metallicBasecoat );
            c.vehicleNumber = std::min<u32>( c.vehicleNumber, 999 );
            VehicleAppearanceQualityProfile profile = profileFor( c.quality );
            profile.liveryWidth = boundedDimension( profile.liveryWidth, c.customMaxResolution );
            profile.liveryHeight = boundedDimension(
                profile.liveryHeight,
                c.customMaxResolution ? std::max<u32>( 64, c.customMaxResolution / 2 ) : 0 );
            profile.bodySurfaceWidth =
                boundedDimension( profile.bodySurfaceWidth, c.customMaxResolution );
            profile.bodySurfaceHeight = boundedDimension(
                profile.bodySurfaceHeight,
                c.customMaxResolution ? std::max<u32>( 64, c.customMaxResolution / 2 ) : 0 );
            profile.microTextureSize =
                boundedDimension( profile.microTextureSize, c.customMaxResolution );

            VehicleAppearanceBundle bundle;
            bundle.quality = c.quality;
            bundle.seed = c.seed;
            WPNoise noise( c.seed );
            makeMaterials( c, bundle );
            generateLivery( c, profile, bundle.textures.bodyLivery, noise );
            generateBodySurface( c, profile, bundle.textures, noise );
            generatePaintCoat( profile.microTextureSize, bundle.textures.paintCoatNormal, noise );
            generateCarbon( profile.microTextureSize, bundle.textures, noise );
            generateRubber( profile.microTextureSize, bundle.textures, noise, c.wear, c.wetness );
            generateBrushedMetal( profile.microTextureSize, bundle.textures, noise );
            generateLight( std::max<u32>( 64, profile.microTextureSize / 2 ),
                           bundle.textures.lightEmission );
            bundle.contentHash = bundleHash( bundle );
            return bundle;
        }

        VehicleAppearanceValidation WPVehicleAppearance::validate(
            const VehicleAppearanceConfig &config )
        {
            VehicleAppearanceValidation result;
            const auto finiteColor = []( const VehicleLinearColor &c ) {
                return std::isfinite( c.r ) && std::isfinite( c.g ) && std::isfinite( c.b ) &&
                       std::isfinite( c.a );
            };
            if( !finiteColor( config.primary ) || !finiteColor( config.secondary ) ||
                !finiteColor( config.accent ) )
                addIssue( result, VehicleAppearanceIssueSeverity::Error, "appearance.color.non_finite",
                          "Every authored colour channel must be finite." );
            const auto inReflectanceRange = []( const VehicleLinearColor &c ) {
                return c.r >= 0 && c.r <= 1 && c.g >= 0 && c.g <= 1 && c.b >= 0 && c.b <= 1 &&
                       c.a >= 0 && c.a <= 1;
            };
            if( !inReflectanceRange( config.primary ) || !inReflectanceRange( config.secondary ) ||
                !inReflectanceRange( config.accent ) )
                addIssue( result, VehicleAppearanceIssueSeverity::Error, "appearance.color.range",
                          "Linear reflectance colours must remain in the inclusive [0,1] range." );
            if( !std::isfinite( config.wear ) || config.wear < 0 || config.wear > 1 ||
                !std::isfinite( config.wetness ) || config.wetness < 0 || config.wetness > 1 ||
                !std::isfinite( config.metallicBasecoat ) || config.metallicBasecoat < 0 ||
                config.metallicBasecoat > 1 )
                addIssue( result, VehicleAppearanceIssueSeverity::Error, "appearance.scalar.range",
                          "Wear, wetness, and metallicBasecoat must be finite values in [0,1]." );
            if( config.vehicleNumber > 999 )
                addIssue( result, VehicleAppearanceIssueSeverity::Error, "appearance.number.range",
                          "vehicleNumber must fit the supported 0..999 livery range." );
            if( config.customMaxResolution != 0 && config.customMaxResolution < 64 )
                addIssue( result, VehicleAppearanceIssueSeverity::Error, "appearance.resolution.minimum",
                          "customMaxResolution must be zero or at least 64 pixels." );
            if( config.primary.r > 0.82f || config.primary.g > 0.82f || config.primary.b > 0.82f )
                addIssue(
                    result, VehicleAppearanceIssueSeverity::Warning, "appearance.paint.headroom",
                    "Base paint reflectance above 0.82 can clip direct light and hide reflections." );
            return result;
        }

        VehicleAppearanceValidation WPVehicleAppearance::validate(
            const VehicleAppearanceBundle &bundle )
        {
            VehicleAppearanceValidation result;
            const TextureBuffer *textures[] = { &bundle.textures.bodyLivery,
                                                &bundle.textures.bodyNormal,
                                                &bundle.textures.bodyORM,
                                                &bundle.textures.bodyClearcoat,
                                                &bundle.textures.paintCoatNormal,
                                                &bundle.textures.carbonAlbedo,
                                                &bundle.textures.carbonNormal,
                                                &bundle.textures.carbonORM,
                                                &bundle.textures.carbonAnisotropy,
                                                &bundle.textures.rubberAlbedo,
                                                &bundle.textures.rubberNormal,
                                                &bundle.textures.rubberORM,
                                                &bundle.textures.brushedMetalNormal,
                                                &bundle.textures.brushedMetalORM,
                                                &bundle.textures.lightEmission };
            for( const TextureBuffer *texture : textures )
            {
                if( !validBuffer( *texture ) )
                {
                    addIssue( result, VehicleAppearanceIssueSeverity::Error, "appearance.texture.layout",
                              "A texture is empty or does not contain width*height*4 RGBA8 bytes." );
                    break;
                }
            }
            for( const auto &material : bundle.materials )
            {
                if( material.name.empty() )
                    addIssue( result, VehicleAppearanceIssueSeverity::Error, "appearance.material.name",
                              "Every material slot must have a stable renderer-facing name." );
                const auto colorInRange = []( const VehicleLinearColor &color ) {
                    return std::isfinite( color.r ) && std::isfinite( color.g ) &&
                           std::isfinite( color.b ) && std::isfinite( color.a ) && color.r >= 0.0f &&
                           color.r <= 1.0f && color.g >= 0.0f && color.g <= 1.0f && color.b >= 0.0f &&
                           color.b <= 1.0f && color.a >= 0.0f && color.a <= 1.0f;
                };
                if( !colorInRange( material.baseColor ) || !colorInRange( material.fallbackBaseColor ) ||
                    !colorInRange( material.emissiveColor ) )
                    addIssue( result, VehicleAppearanceIssueSeverity::Error,
                              "appearance.material.color_range",
                              "Material colours must be finite linear values in [0,1]." );
                const real_Num bounded[] = {
                    material.roughness,  material.metallic,           material.fallbackRoughness,
                    material.clearcoat,  material.clearcoatRoughness, material.anisotropy,
                    material.sheen,      material.transmission,       material.opacity,
                    material.iridescence
                };
                for( real_Num value : bounded )
                {
                    if( !std::isfinite( value ) || value < 0 || value > 1 )
                    {
                        addIssue( result, VehicleAppearanceIssueSeverity::Error,
                                  "appearance.material.range",
                                  "A normalized material parameter is non-finite or outside [0,1]." );
                        break;
                    }
                }
            }
            if( bundle.contentHash == 0 )
                addIssue( result, VehicleAppearanceIssueSeverity::Warning, "appearance.hash.missing",
                          "contentHash is zero; deterministic cache identity is unavailable." );
            else if( bundle.contentHash != bundleHash( bundle ) )
                addIssue( result, VehicleAppearanceIssueSeverity::Error, "appearance.hash.mismatch",
                          "contentHash does not match the generated material and texture payload." );
            return result;
        }
    }  // namespace procedural
}  // namespace workphone
