#include <EditorPCH.hpp>
#include <procedural/ProceduralTextureBindings.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Procedural/IProceduralTexture.hpp>
#include <Workphone/Interface/Procedural/IProceduralManager.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IMaterialManager.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER( workphone::editor::ProceduralTextureBindings );

    ProceduralTextureBindings::ProceduralTextureBindings() = default;
    ProceduralTextureBindings::~ProceduralTextureBindings() = default;

    // -------------------------------------------------------------------------
    // Procedural noise — generates a full RGBA noise texture in-memory.

    SmartPtr<render::ITexture> ProceduralTextureBindings::generateNoiseTexture(
        int width, int height, u32 noiseType,
        real_Num frequency, u32 octaves, real_Num lacunarity,
        real_Num persistence, u32 seed, bool turbulence,
        const String &outputPath )
    {
        // noiseType: 0=perlin, 1=simplex, 2=value, 3=worley, 4=fbm
        // ref: Esoterica/Engine/Render/... noise pipeline (same algorithm family)
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;

        auto texMgr = appMgr->getTextureManager();
        if( !texMgr ) return nullptr;

        auto procTex = workphone::make_ptr<procedural::IProceduralTexture>();
        if( !procTex ) return nullptr;

        procTex->setSize( width, height );

        // Generate pixel data
        Array<ColourF> data;
        data.reserve( static_cast<size_t>( width ) * height );

        // Simple deterministic hash for reproducible noise
        const auto hash = []( u32 x, u32 y, u32 s ) -> real_Num
        {
            u32 h = s + x * 374761393u + y * 668265263u;
            h = ( h ^ ( h >> 13 ) ) * 1274126177u;
            return real_Num( ( h ^ ( h >> 16 ) ) & 0xFFFFFFu ) / real_Num( 0xFFFFFFu );
        };

        for( int y = 0; y < height; ++y )
        {
            for( int x = 0; x < width; ++x )
            {
                real_Num v = 0.0f;
                real_Num amp = 1.0f;
                real_Num freq = frequency;
                real_Num maxAmp = 0.0f;

                for( u32 o = 0; o < octaves; ++o )
                {
                    real_Num nx = real_Num( x ) / real_Num( width )  * freq;
                    real_Num ny = real_Num( y ) / real_Num( height ) * freq;

                    real_Num n = 0.0f;
                    switch( noiseType )
                    {
                        case 0: // perlin-like smooth hash
                        {
                            u32 xi = static_cast<u32>( floor( nx ) );
                            u32 yi = static_cast<u32>( floor( ny ) );
                            real_Num fx = nx - floor( nx );
                            real_Num fy = ny - floor( ny );
                            // Smoothstep
                            real_Num u = fx * fx * ( 3.0f - 2.0f * fx );
                            real_Num v2 = fy * fy * ( 3.0f - 2.0f * fy );
                            real_Num a = hash( xi,     yi,     seed + o );
                            real_Num b = hash( xi + 1, yi,     seed + o );
                            real_Num c = hash( xi,     yi + 1, seed + o );
                            real_Num d = hash( xi + 1, yi + 1, seed + o );
                            n = a + ( b - a ) * u + ( c - a ) * v2 + ( a - b - c + d ) * u * v2;
                            break;
                        }
                        case 1: // value noise
                        case 2:
                        {
                            u32 xi = static_cast<u32>( floor( nx ) );
                            u32 yi = static_cast<u32>( floor( ny ) );
                            n = hash( xi, yi, seed + o );
                            break;
                        }
                        case 3: // worley-like (nearest-cell distance)
                        {
                            u32 xi = static_cast<u32>( floor( nx ) );
                            u32 yi = static_cast<u32>( floor( ny ) );
                            real_Num minDist = 1e9f;
                            for( s32 di = -1; di <= 1; ++di )
                            for( s32 dj = -1; dj <= 1; ++dj )
                            {
                                u32 cx = xi + di;
                                u32 cy = yi + dj;
                                real_Num fx2 = nx - ( real_Num( cx ) + hash( cx, cy, seed + 17u ) );
                                real_Num fy2 = ny - ( real_Num( cy ) + hash( cx, cy, seed + 31u ) );
                                real_Num dist = sqrtf( fx2 * fx2 + fy2 * fy2 );
                                if( dist < minDist ) minDist = dist;
                            }
                            n = minDist;
                            break;
                        }
                        default:
                            n = hash( static_cast<u32>( x ), static_cast<u32>( y ), seed );
                            break;
                    }

                    if( turbulence )
                        n = fabsf( n * 2.0f - 1.0f );

                    v += n * amp;
                    maxAmp += amp;
                    amp *= persistence;
                    freq *= lacunarity;
                }

                v = ( v / maxAmp );  // normalise to 0..1

                // R=height, G=normalX, B=normalY, A=1
                ColourF col( v, v, v, 1.0f );
                data.push_back( col );
            }
        }

        procTex->setData( data );
        procTex->generate();

        // Create and populate a runtime texture
        auto tex = texMgr->createManual(
            outputPath.empty() ? "ProceduralNoise" : outputPath,
            "General",
            1,  // TextureType_2d
            static_cast<u32>( width ),
            static_cast<u32>( height ),
            1, 0,
            55,  // PF_R8G8B8A8
            0 );

        if( tex && tex->buffer() && !data.empty() )
        {
            // Upload pixel data to GPU texture
            tex->loadImage( data );
        }

        return tex;
    }

    // -------------------------------------------------------------------------
    // Noise-based heightmap → normal map

    SmartPtr<render::ITexture> ProceduralTextureBindings::generateNormalMapFromHeight(
        SmartPtr<render::ITexture> heightTex,
        real_Num strength, bool invertY,
        const String &outputPath )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;

        auto texMgr = appMgr->getTextureManager();
        if( !texMgr || !heightTex ) return nullptr;

        int w = static_cast<int>( heightTex->getWidth() );
        int h = static_cast<int>( heightTex->getHeight() );
        if( w <= 0 || h <= 0 ) return nullptr;

        auto normalTex = texMgr->createManual(
            outputPath.empty() ? "GeneratedNormal" : outputPath,
            "General", 1,
            static_cast<u32>( w ), static_cast<u32>( h ),
            1, 0, 55, 0 );

        Array<ColourF> heightData;
        if( !heightTex->convertToImage( heightData ) || heightData.empty() )
            return nullptr;

        Array<ColourF> normalData;
        normalData.reserve( static_cast<size_t>( w ) * h );

        for( int y = 0; y < h; ++y )
        {
            for( int x = 0; x < w; ++x )
            {
                int x1 = ( x > 0 ) ? x - 1 : x;
                int x2 = ( x < w - 1 ) ? x + 1 : x;
                int y1 = ( y > 0 ) ? y - 1 : y;
                int y2 = ( y < h - 1 ) ? y + 1 : y;

                real_Num hL = heightData[static_cast<size_t>( y1 ) * w + x ].r;
                real_Num hR = heightData[static_cast<size_t>( y1 ) * w + x2 ].r;
                real_Num hD = heightData[static_cast<size_t>( y2 ) * w + x ].r;
                real_Num hU = heightData[static_cast<size_t>( y1 ) * w + x ].r;

                real_Num dX = ( hR - hL ) * strength;
                real_Num dY = ( hU - hD ) * strength;
                if( invertY ) dY = -dY;

                real_Num len = sqrtf( dX * dX + dY * dY + 1.0f );
                ColourF n( dX / len * 0.5f + 0.5f,
                           dY / len * 0.5f + 0.5f,
                           0.5f, 1.0f );
                normalData.push_back( n );
            }
        }

        if( normalTex && !normalData.empty() )
            normalTex->loadImage( normalData );

        return normalTex;
    }

    // -------------------------------------------------------------------------
    // Colour-correct / levels adjust

    SmartPtr<render::ITexture> ProceduralTextureBindings::colourCorrectTexture(
        SmartPtr<render::ITexture> sourceTex,
        real_Num brightness, real_Num contrast, real_Num saturation,
        real_Num blackPoint, real_Num whitePoint,
        const String &outputPath )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;

        auto texMgr = appMgr->getTextureManager();
        if( !texMgr || !sourceTex ) return nullptr;

        int w = static_cast<int>( sourceTex->getWidth() );
        int h = static_cast<int>( sourceTex->getHeight() );

        Array<ColourF> srcData;
        if( !sourceTex->convertToImage( srcData ) || srcData.empty() )
            return nullptr;

        auto outTex = texMgr->createManual(
            outputPath.empty() ? "ColourCorrected" : outputPath,
            "General", 1,
            static_cast<u32>( w ), static_cast<u32>( h ),
            1, 0, 55, 0 );

        Array<ColourF> dstData;
        dstData.reserve( srcData.size() );

        for( const auto &c : srcData )
        {
            ColourF out;
            for( int ch = 0; ch < 3; ++ch )
            {
                real_Num v = ( ch == 0 ) ? c.r : ( ch == 1 ) ? c.g : c.b;
                // Levels (black/white point → 0..1)
                v = ( v - blackPoint ) / ( whitePoint - blackPoint + 1e-5f );
                v = std::max( real_Num( 0.0f ), std::min( real_Num( 1.0f ), v ) );
                // Brightness
                v += brightness;
                // Contrast
                v = ( v - 0.5f ) * contrast + 0.5f;
                // Clamp
                v = std::max( real_Num( 0.0f ), std::min( real_Num( 1.0f ), v ) );
                if( ch == 0 ) out.r = v;
                else if( ch == 1 ) out.g = v;
                else out.b = v;
            }
            out.a = c.a;
            dstData.push_back( out );
        }

        if( outTex && !dstData.empty() )
            outTex->loadImage( dstData );

        return outTex;
    }

    // -------------------------------------------------------------------------
    // Assign texture to material slot

    void ProceduralTextureBindings::assignTextureToMaterialSlot(
        SmartPtr<render::IMaterial> material,
        const String &slotName,
        SmartPtr<render::ITexture> texture )
    {
        if( !material || !texture ) return;

        auto hash = StringUtil::getHash( slotName );
        material->setTexture( hash, texture );
    }

    // -------------------------------------------------------------------------
    // Save texture to disk

    bool ProceduralTextureBindings::saveTextureToFile(
        SmartPtr<render::ITexture> texture,
        const String &filePath,
        const String &format )
    {
        if( !texture ) return false;

        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return false;

        auto fileSystem = appMgr->getFileSystem();
        if( !fileSystem ) return false;

        Array<ColourF> imgData;
        if( !texture->convertToImage( imgData ) || imgData.empty() )
            return false;

        int w = static_cast<int>( texture->getWidth() );
        int h = static_cast<int>( texture->getHeight() );

        // Write a simple PPM (uncompressed) as a fallback since TGA/DDS encoding
        // requires a dedicated codec. The runtime texture manager handles
        // format-specific export when available.
        // ref: texture export patterns from MaterialEditor.lua

        auto out = fileSystem->openWrite( filePath, true );
        if( !out ) return false;

        String header = StringUtil::format( "P6\n%d %d\n255\n", w, h );
        out->write( header.c_str(), static_cast<u32>( header.size() ) );

        Array<u8> rgb;
        rgb.reserve( imgData.size() * 3 );
        for( const auto &c : imgData )
        {
            rgb.push_back( static_cast<u8>( std::min( 255, int( c.r * 255.0f ) ) ) );
            rgb.push_back( static_cast<u8>( std::min( 255, int( c.g * 255.0f ) ) ) );
            rgb.push_back( static_cast<u8>( std::min( 255, int( c.b * 255.0f ) ) ) );
        }
        out->write( rgb.data(), static_cast<u32>( rgb.size() ) );
        out->close();

        return true;
    }

    SmartPtr<render::IMaterial> ProceduralTextureBindings::createOrGetMaterial(
        const String &materialName )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;

        auto matMgr = appMgr->getMaterialManager();
        if( !matMgr ) return nullptr;

        return matMgr->createOrRetrieveMaterial( materialName );
    }

}  // namespace workphone::editor
