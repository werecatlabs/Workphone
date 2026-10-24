#ifndef WP_ASSIMP_EMBEDDED_TEXTURE_CACHE_HPP
#define WP_ASSIMP_EMBEDDED_TEXTURE_CACHE_HPP

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace workphone::detail
{
    // Content is part of the resource identity: reimporting a changed image must
    // not reuse a texture whose decoder/GPU cache still contains the old pixels.
    inline std::filesystem::path cacheEmbeddedTexture( const std::filesystem::path &cacheFolder,
                                                       const std::string &sourceKey,
                                                       unsigned int textureIndex,
                                                       std::string formatHint, const void *data,
                                                       size_t byteLength, std::string &error )
    {
        error.clear();
        if( cacheFolder.empty() || !data || !byteLength )
        {
            error = "missing embedded image data or cache folder";
            return {};
        }
        std::transform( formatHint.begin(), formatHint.end(), formatHint.begin(),
                        []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
        // Never derive a path from an exporter-supplied filename or unchecked hint.
        if( formatHint == "jpeg" ) formatHint = "jpg";
        if( formatHint.empty() || formatHint.size() > 8 ||
            !std::all_of( formatHint.begin(), formatHint.end(),
                          []( unsigned char c ) { return std::isalnum( c ) != 0; } ) )
        {
            error = "invalid embedded image format hint";
            return {};
        }

        const auto hashBytes = []( const void *bytes, size_t size ) {
            uint64_t hash = 14695981039346656037ull;
            const auto input = static_cast<const unsigned char *>( bytes );
            for( size_t i = 0; i < size; ++i )
            {
                hash ^= input[i];
                hash *= 1099511628211ull;
            }
            return hash;
        };
        const auto fileName = std::to_string( hashBytes( sourceKey.data(), sourceKey.size() ) ) +
                              "_" + std::to_string( textureIndex ) + "_" +
                              std::to_string( hashBytes( data, byteLength ) ) + "." + formatHint;
        const auto path = cacheFolder / "EmbeddedTextures" / fileName;

        try
        {
            std::error_code statusError;
            if( std::filesystem::file_size( path, statusError ) == byteLength && !statusError )
            {
                std::ifstream existing( path, std::ios::binary );
                std::array<char, 4096> block;
                const auto input = static_cast<const char *>( data );
                size_t offset = 0;
                while( existing && offset < byteLength )
                {
                    const auto count = ( std::min )( block.size(), byteLength - offset );
                    existing.read( block.data(), static_cast<std::streamsize>( count ) );
                    if( existing.gcount() != static_cast<std::streamsize>( count ) ||
                        std::memcmp( block.data(), input + offset, count ) != 0 )
                    {
                        break;
                    }
                    offset += count;
                }
                if( offset == byteLength ) return path;
            }

            std::filesystem::create_directories( path.parent_path() );
            std::ofstream output( path, std::ios::binary | std::ios::trunc );
            output.write( static_cast<const char *>( data ), static_cast<std::streamsize>( byteLength ) );
            output.close();
            if( !output )
            {
                error = "failed to write embedded image cache";
                return {};
            }
            return path;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return {};
        }
    }
}

#endif
