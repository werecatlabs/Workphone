#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    namespace sceneBuild
    {
        SmartPtr<render::IMaterial> material( SceneAssets &assets, ColourF colour, float roughness,
                                              float metalness )
        {
            reserveOwnerSlot( assets.materials );
            auto m = ApplicationUtil::createDefaultMaterial();
            if( !m )
            {
                throw std::runtime_error( "Cannot create procedural material" );
            }
            assets.materials.push_back( m );
            m->load( nullptr );
            m->setCullMode( 1 );
            m->setDepthTest( 2 );
            m->setSpecular( ColourF( .04f, .04f, .04f, 1 ) );
            m->setDiffuse( colour );
            m->setRoughness( roughness );
            m->setMetalness( metalness );
            return m;
        }

        SmartPtr<render::ITexture> uploadPixels(
            SceneAssets &assets, const u8 *pixels, size_t bytes, u32 width, u32 height,
            render::TextureMipFilter filter, u32 atlasColumns,
            const char *citySurface, bool sourceIsBgra )
        {
            if( !bytes )
            {
                return nullptr;
            }
            if( !pixels || !width || !height || width > 16384 || height > 16384 ||
                u64( width ) * height * 4 != bytes )
            {
                throw std::invalid_argument(
                    "Procedural upload requires exact bounded RGBA/BGRA pixels." );
            }
            // Admit conversion before creating a manager-owned texture. The
            // caller's atlas and this BGRA snapshot remain independently owned.
            auto bgra = makeSourceBudgetSnapshot<u8>();
            bgra.reserve( bytes );
            bgra.resize( bytes );
            std::copy_n( pixels, bytes, bgra.begin() );
            if( !sourceIsBgra )
            {
                for( size_t i = 0; i < bgra.size(); i += 4 )
                {
                    std::swap( bgra[i], bgra[i + 2] );
                }
            }
            auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
            auto manager = graphics->getTextureManager();
            reserveOwnerSlot( assets.textures );
            auto name = assets.resourcePrefix;
            if( citySurface )
            {
                name += String( "/city-surface/" ) + citySurface;
            }
            name += "/texture/" + StringUtil::toString( assets.textures.size() );
            // New sampled textures default to 2D/usage zero. Avoid allocating
            // createManual's complete blank image before replacing its pixels.
            auto texture = dynamic_pointer_cast<render::ITexture>( manager->create( name ) );
            if( !texture )
            {
                throw std::runtime_error( "Cannot create procedural texture." );
            }
            try
            {
                auto properties = texture->getProperties();
                properties->setProperty( "mipmapFilter", static_cast<s32>( filter ) );
                properties->setProperty( "mipmapAtlasColumns", atlasColumns );
                texture->setProperties( properties );
                String error;
                if( !texture->copyByteData( bgra, Vector2I( width, height ), error ) )
                {
                    throw std::runtime_error(
                        ( String( "Cannot stage procedural texture: " ) + error ).c_str() );
                }
                texture->load( nullptr );
                assets.textures.push_back( texture );
                assets.textureBytes += bytes;
            }
            catch( ... )
            {
                try
                {
                    manager->destroyResource( texture );
                }
                catch( ... )
                {
                }
                throw;
            }
            return texture;
        }

        SmartPtr<render::ITexture> upload(
            SceneAssets &assets, const TextureBuffer &buffer,
            render::TextureMipFilter filter, u32 atlasColumns,
            const char *citySurface )
        {
            return uploadPixels( assets, buffer.data(), buffer.pixels.size(), buffer.width,
                                 buffer.height, filter, atlasColumns, citySurface );
        }

        SmartPtr<render::ITexture> uploadLinearRadiance( SceneAssets &assets, const Array<f32> &rgba,
                                                         Vector2I size )
        {
            auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
            reserveOwnerSlot( assets.textures );
            const auto name =
                assets.resourcePrefix + "/radiance/" + StringUtil::toString( assets.textures.size() );
            auto manager = graphics->getTextureManager();
            auto texture = dynamic_pointer_cast<render::ITexture>( manager->create( name ) );
            if( !texture )
            {
                throw std::runtime_error( "Cannot create HDR radiance texture" );
            }
            try
            {
                String error;
                if( !texture->copyLinearFloatData( rgba, size, error ) )
                {
                    throw std::runtime_error(
                        ( String( "Cannot stage HDR radiance face: " ) + error ).c_str() );
                }
                assets.textures.push_back( texture );
                assets.textureBytes += rgba.size() * sizeof( f32 );
            }
            catch( ... )
            {
                try
                {
                    manager->destroyResource( texture );
                }
                catch( ... )
                {
                }
                throw;
            }
            return texture;
        }

        void maps( SceneAssets &assets, SmartPtr<render::IMaterial> m, const TextureBuffer &albedo,
                   const TextureBuffer &normal, const TextureBuffer &orm,
                   const char *citySurface )
        {
            if( !albedo.pixels.empty() )
            {
                m->setDiffuse( ColourF::White );
                m->setTexture(
                    upload( assets, albedo, render::TextureMipFilter::Colour, 1, citySurface ), 0 );
            }
            if( !normal.pixels.empty() )
            {
                m->setTexture(
                    upload( assets, normal, render::TextureMipFilter::Normal, 1, citySurface ), 1 );
            }
            // The generated ORM is AO/Roughness/Metalness. WPGraphics' separate map slots
            // read red, so expand each channel explicitly, without applying an sRGB transfer.
            if( !orm.pixels.empty() )
            {
                for( auto channel : { 0u, 1u, 2u } )
                {
                    const auto split = citySurfaceScalar::split( orm, channel );
                    m->setTexture( upload( assets, split,
                                           channel == 1 ? render::TextureMipFilter::Roughness
                                                        : render::TextureMipFilter::Data,
                                           1, citySurface ),
                                   channel == 0   ? 22
                                   : channel == 1 ? 3
                                                  : 2 );
                }
                m->setRoughness( 1 );
                m->setMetalness( 1 );
            }
        }

        bool applyCitySurface( SceneAssets &assets, SmartPtr<render::IMaterial> material,
                               CitySurface family, u32 size, float normalStrength, u32 projection,
                               u32 seed )
        {
            SurfaceBakeResult surface;
            String diagnostic;
            if( !loadCitySurface( family, size, surface, diagnostic ) )
            {
                WP_LOG( String( "City surface uses procedural fallback: " ) + diagnostic );
                return false;
            }
            if( family == CitySurface::Asphalt && !assets.exportSourceDirectory.empty() )
            {
                std::string error;
                if( !texture_export::exportAsphaltSource(
                        surface, std::filesystem::u8path( assets.exportSourceDirectory.c_str() ), seed,
                        error, true ) )
                {
                    throw std::runtime_error( "City asphalt source export failed: " + error );
                }
            }
            maps( assets, material, surface.albedo, surface.normal, surface.orm,
                  citySurfaceAssetId( family ) );
            material->setUVProjection( projection );
            material->setTriplanarScale( 1.f / citySurfaceTileMetres( family ) );
            material->setNormalStrength( normalStrength );
            material->setEditorFloat( "aoStrength", .65f );
            material->setEditorUInt( "uvFilter", 3 );
            material->setEditorFloat( "uvAniso", 8.f );
            return true;
        }

        SmartPtr<render::IMaterial> raceAsphalt( SceneAssets &assets, SmartPtr<ITextureForge> forge,
                                                 const SurfaceBakeParams &params, u32 seed )
        {
            if( assets.openCity )
            {
                auto scanned = material( assets, ColourF::White, 1 );
                if( applyCitySurface( assets, scanned, CitySurface::Asphalt, params.size, .45f, 3,
                                      seed ) )
                {
                    return scanned;
                }
            }
            auto asphalt = forge->bakeSurface( SurfaceTag::Asphalt, params );
            // Keep the forge's seeded aggregate detail subtle at driving distance.
            // Its general-purpose height/rut contrast is too strong for a race surface.
            for( size_t i = 0; i < asphalt.albedo.pixels.size(); i += 4 )
            {
                for( size_t channel = 0; channel < 3; ++channel )
                {
                    asphalt.albedo.pixels[i + channel] =
                        static_cast<u8>( 60 + asphalt.albedo.pixels[i + channel] / 6 );
                }
                for( size_t channel = 0; channel < 2; ++channel )
                {
                    asphalt.normal.pixels[i + channel] = static_cast<u8>(
                        128 + ( int( asphalt.normal.pixels[i + channel] ) - 128 ) / 10 );
                }
            }
            if( assets.openCity )
            {
                // City streets use isotropic aggregate. The generic asphalt's
                // repeated sinusoidal ruts read as a corrugated road at street level.
                const auto noise = [seed]( u32 x, u32 y, u32 period ) {
                    u32 value = ( x % period ) * 0x9e3779b9u ^ ( y % period ) * 0x85ebca6bu ^ seed;
                    value ^= value >> 16;
                    value *= 0x7feb352du;
                    value ^= value >> 15;
                    value *= 0x846ca68bu;
                    return float( ( value ^ ( value >> 16 ) ) >> 8 ) / 16777215.f;
                };
                const auto smoothNoise = [&]( u32 x, u32 y, u32 cells ) {
                    const float u = float( x ) * cells / params.size,
                                v = float( y ) * cells / params.size;
                    const u32 ix = u32( u ), iy = u32( v );
                    const float fx = u - ix, fy = v - iy;
                    const float sx = fx * fx * ( 3 - 2 * fx ), sy = fy * fy * ( 3 - 2 * fy );
                    const float a =
                        noise( ix, iy, cells ) * ( 1 - sx ) + noise( ix + 1, iy, cells ) * sx;
                    const float b =
                        noise( ix, iy + 1, cells ) * ( 1 - sx ) + noise( ix + 1, iy + 1, cells ) * sx;
                    return a * ( 1 - sy ) + b * sy;
                };
                for( u32 y = 0; y < params.size; ++y )
                {
                    for( u32 x = 0; x < params.size; ++x )
                    {
                        const float grain = noise( x, y, params.size );
                        const float patch = smoothNoise( x, y, 3 ), aggregate = smoothNoise( x, y, 32 );
                        const float linear = .064f + ( patch - .5f ) * .016f +
                                             ( aggregate - .5f ) * .009f + ( grain - .5f ) * .006f;
                        auto pixel = asphalt.albedo.pixel( x, y );
                        for( u32 channel = 0; channel < 3; ++channel )
                        {
                            const float colour = linear * ( channel == 2 ? 1.02f : 1.f );
                            pixel[channel] = u8(
                                std::clamp( ( 1.055f * std::pow( colour, 1.f / 2.4f ) - .055f ) * 255,
                                            0.f, 255.f ) );
                        }
                        asphalt.height.values[size_t( y ) * params.size + x] =
                            .5f + ( grain - .5f ) * .012f;
                        auto orm = asphalt.orm.pixel( x, y );
                        orm[0] = 255;
                        orm[1] = u8( 224 + aggregate * 16 );
                        orm[2] = 0;
                        orm[3] = 255;
                    }
                }
                forge->heightToNormal( asphalt.height, asphalt.normal, .25f );
            }
            if( !assets.exportSourceDirectory.empty() )
            {
                std::string error;
                if( !texture_export::exportAsphaltSource(
                        asphalt, std::filesystem::u8path( assets.exportSourceDirectory.c_str() ), seed,
                        error ) )
                {
                    throw std::runtime_error( "Asphalt source export failed: " + error );
                }
            }
            auto roadMat = material( assets, { .07f, .075f, .085f, 1 }, .9f );
            maps( assets, roadMat, asphalt.albedo, asphalt.normal, asphalt.orm );
            roadMat->setNormalStrength( .2f );
            roadMat->setEditorUInt( "uvFilter", 3 );
            roadMat->setEditorFloat( "uvAniso", 8.f );
            return roadMat;
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
