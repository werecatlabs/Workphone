#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GeneratedTextureSource.hpp>
#include <Workphone/Graphics/ProceduralTextureExport.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <cJSON.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace workphone::render
{
    namespace
    {
        void require( bool condition, const char *message )
        {
            if( !condition )
            {
                throw std::runtime_error( message );
            }
        }

        bool sameFile( const std::filesystem::path &path, const char *bytes, size_t size )
        {
            if( !std::filesystem::is_regular_file( path ) || std::filesystem::file_size( path ) != size )
            {
                return false;
            }
            std::ifstream input( path, std::ios::binary );
            char buffer[4096];
            for( size_t offset = 0; offset < size; )
            {
                const auto count = std::min( sizeof( buffer ), size - offset );
                input.read( buffer, std::streamsize( count ) );
                if( size_t( input.gcount() ) != count ||
                    !std::equal( buffer, buffer + count, bytes + offset ) )
                {
                    return false;
                }
                offset += count;
            }
            return true;
        }

        void writeImmutable( const std::filesystem::path &path, const char *bytes, size_t size )
        {
            if( std::filesystem::exists( path ) )
            {
                require( sameFile( path, bytes, size ),
                         "Generated texture source conflicts with an existing asset." );
                return;
            }
            auto temporary = path;
            temporary += ".tmp-" + std::string( StringUtil::getUUID().c_str() );
            try
            {
                std::ofstream file( temporary, std::ios::binary );
                file.write( bytes, std::streamsize( size ) );
                file.close();
                require( bool( file ), "Cannot write generated texture source." );
                // The destination name is content-derived. A simultaneous export
                // may have published identical bytes while this file was staged.
                if( std::filesystem::exists( path ) )
                {
                    require( sameFile( path, bytes, size ), "Generated texture source changed." );
                }
                else
                {
                    std::filesystem::rename( temporary, path );
                }
                std::error_code ignored;
                std::filesystem::remove( temporary, ignored );
            }
            catch( ... )
            {
                std::error_code ignored;
                std::filesystem::remove( temporary, ignored );
                throw;
            }
        }
    }  // namespace

    bool saveGeneratedTextureSource( const procedural::TextureBuffer &rgba, const String &projectRoot,
                                     const String &assetDirectory, const String &name,
                                     TextureMipFilter filter, String &textureId, String &error )
    {
        namespace fs = std::filesystem;
        try
        {
            require( !projectRoot.empty() && !assetDirectory.empty() && !name.empty() &&
                         projectRoot.find( '\0' ) == String::npos &&
                         assetDirectory.find( '\0' ) == String::npos && name.size() <= 64,
                     "Generated textures require a project root, asset directory and short name." );
            for( const auto c : name )
            {
                require(
                    ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) ||
                        c == '-' || c == '_',
                    "Generated texture names may contain letters, numbers, hyphens and underscores." );
            }
            const char *filters[] = { "none", "colour", "data", "normal", "cutout", "roughness" };
            require( u32( filter ) < 6, "Unknown generated texture mip filter." );
            const auto root = fs::weakly_canonical( fs::absolute( fs::u8path( projectRoot.c_str() ) ) );
            require( fs::is_directory( root ), "Generated texture project root does not exist." );
            const auto requested = fs::u8path( assetDirectory.c_str() );
            const auto folder =
                fs::weakly_canonical( requested.is_absolute() ? requested : root / requested );
            const auto relative = folder.lexically_relative( root );
            require( !relative.empty() && !relative.is_absolute(),
                     "Generated texture directory must be inside the project." );
            for( const auto &part : relative )
            {
                require( part != "..", "Generated texture directory must be inside the project." );
            }

            std::vector<unsigned char> png;
            require( procedural::texture_export::encodeTextureBuffer( rgba, "png", png ),
                     "Cannot encode generated RGBA texture source." );
            u64 hash = 14695981039346656037ull;
            for( auto byte : png )
            {
                hash = ( hash ^ byte ) * 1099511628211ull;
            }
            hash = ( hash ^ u32( filter ) ) * 1099511628211ull;
            std::ostringstream suffix;
            suffix << std::hex << std::setw( 16 ) << std::setfill( '0' ) << hash;
            const auto stem = std::string( name.c_str() ) + "-" + suffix.str();
            const auto imagePath = folder / ( stem + ".png" );
            const auto descriptorPath = folder / ( stem + ".texres" );
            const String imageId =
                "data://" + String( imagePath.lexically_relative( root ).generic_u8string().c_str() );
            const String id =
                "data://" +
                String( descriptorPath.lexically_relative( root ).generic_u8string().c_str() );
            std::unique_ptr<cJSON, decltype( &cJSON_Delete )> descriptor( cJSON_CreateObject(),
                                                                          cJSON_Delete );
            require( bool( descriptor ), "Cannot allocate generated texture descriptor." );
            require(
                cJSON_AddNumberToObject( descriptor.get(), "version", 1 ) &&
                    cJSON_AddStringToObject( descriptor.get(), "source", imageId.c_str() ) &&
                    cJSON_AddStringToObject( descriptor.get(), "mipFilter", filters[u32( filter )] ) &&
                    cJSON_AddNumberToObject( descriptor.get(), "atlasColumns", 1 ) &&
                    cJSON_AddNumberToObject( descriptor.get(), "alphaCutoff", .5 ),
                "Cannot configure generated texture descriptor." );
            std::unique_ptr<char, decltype( &cJSON_free )> json(
                cJSON_PrintUnformatted( descriptor.get() ), cJSON_free );
            require( bool( json ), "Cannot encode generated texture descriptor." );
            fs::create_directories( folder );
            writeImmutable( imagePath, reinterpret_cast<const char *>( png.data() ), png.size() );
            writeImmutable( descriptorPath, json.get(), std::strlen( json.get() ) );
            textureId = id;
            error.clear();
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }
}  // namespace workphone::render
