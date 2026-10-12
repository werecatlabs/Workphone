#pragma once
#include <Workphone/Graphics/ProceduralTextureExport.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace workphone::procedural::texture_export
{
    // Source-root-relative descriptors consumed by GraphicsResourceCompiler. The current
    // cooked material tier binds base colour only; auxiliary maps remain explicit sources.
    template <class Surface>
    bool exportAsphaltSource( const Surface &surface, const std::filesystem::path &directory,
                              unsigned int seed, std::string &error )
    {
        namespace fs = std::filesystem;
        error.clear();
        try
        {
            const auto valid = [&]( const auto &buffer ) {
                const auto count = std::uint64_t( buffer.width ) * buffer.height;
                return buffer.width > 0 && buffer.height > 0 && buffer.width <= 8192 &&
                    buffer.height <= 8192 && count <= 16ull * 1024 * 1024 &&
                    buffer.width == surface.albedo.width && buffer.height == surface.albedo.height &&
                    count * 4 == buffer.pixels.size();
            };
            if( !valid( surface.albedo ) || !valid( surface.normal ) || !valid( surface.orm ) )
                throw std::runtime_error( "Material source maps require matching bounded RGBA8 dimensions." );
            if( directory.empty() || fs::exists( directory ) )
                throw std::runtime_error( "Material export requires a new, nonexistent directory." );
            if( !fs::create_directory( directory ) )
                throw std::runtime_error( "Cannot create material export directory; parent must exist." );
            const auto write = [&]( const std::string &name, const char *data, size_t size ) {
                std::ofstream stream( directory / name, std::ios::binary );
                stream.write( data, std::streamsize( size ) );
                stream.close();
                if( !stream )
                    throw std::runtime_error( "Cannot write procedural material source: " + name );
            };
            const auto map = [&]( const std::string &name, const auto &buffer,
                                  const std::string &filter ) {
                std::vector<unsigned char> encoded;
                if( !encodeTextureBuffer( buffer, "tga", encoded ) )
                    throw std::runtime_error( "Cannot encode generated material map: " + name );
                write( name + ".tga", reinterpret_cast<const char *>( encoded.data() ), encoded.size() );
                const auto descriptor = std::string( "{\"version\":1,\"source\":\"data://" ) + name +
                    ".tga\",\"mipFilter\":\"" + filter +
                    "\",\"atlasColumns\":1,\"alphaCutoff\":0.5}\n";
                write( name + ".texres", descriptor.data(), descriptor.size() );
            };
            map( "asphalt-albedo", surface.albedo, "colour" );
            map( "asphalt-normal", surface.normal, "normal" );
            map( "asphalt-orm", surface.orm, "data" );
            for( unsigned channel = 0; channel < 3; ++channel )
            {
                auto split = surface.orm;
                for( size_t i = 0; i < split.pixels.size(); i += 4 )
                {
                    split.pixels[i] = split.pixels[i + 1] = split.pixels[i + 2] =
                        surface.orm.pixels[i + channel];
                    split.pixels[i + 3] = 255;
                }
                map( channel == 0 ? "asphalt-ao" : channel == 1 ? "asphalt-roughness" : "asphalt-metalness",
                     split, channel == 1 ? "roughness" : "data" );
            }
            const std::string material =
                "{\"version\":1,\"baseColour\":[1,1,1,1],\"metalness\":0,\"roughness\":0.9,"
                "\"baseColourTexture\":\"data://asphalt-albedo.texres\"}\n";
            write( "asphalt.matres", material.data(), material.size() );
            std::ostringstream provenance;
            provenance << "{\"format\":\"workphone.procedural-material-source\",\"version\":1,"
                "\"generator\":\"racing-asphalt\",\"generatorVersion\":1,\"seed\":" << seed <<
                ",\"width\":" << surface.albedo.width << ",\"height\":" << surface.albedo.height <<
                ",\"tileMetres\":1.5,\"maps\":{\"albedo\":\"srgb\",\"normal\":\"linear-tangent\","
                "\"orm\":\"linear-AO-roughness-metalness\",\"ao\":\"linear\","
                "\"roughness\":\"linear\",\"metalness\":\"linear\"},"
                "\"cookedMaterialTier\":\"baseColour-only\","
                "\"auxiliaryMapsBoundByCookedMaterial\":false}\n";
            const auto manifest = provenance.str();
            // Written last: its presence certifies the complete source export, not a GPU cook.
            write( "material-source.json.tmp", manifest.data(), manifest.size() );
            fs::rename( directory / "material-source.json.tmp", directory / "material-source.json" );
            return true;
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return false;
        }
    }
}
