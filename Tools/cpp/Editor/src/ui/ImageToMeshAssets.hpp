#pragma once

#include <ui/GeneratedAssetFile.hpp>
#include <Workphone/AI/MeshyClient.hpp>
#include <fstream>

namespace workphone::editor::mesh_assets
{
    inline std::string sanitizeBasename( const std::string &name )
    {
        return generated_assets::sanitizeBasename( name, ".glb", "generated-mesh" );
    }

    inline bool readImage( const std::filesystem::path &path, std::vector<std::uint8_t> &bytes,
                           std::string &mimeType, std::string &error )
    {
        bytes.clear();
        mimeType.clear();
        error.clear();
        try
        {
            if( path.empty() || !std::filesystem::is_regular_file( path ) )
            {
                error = "Choose a PNG or JPEG image file.";
                return false;
            }
            auto extension = path.extension().u8string();
            for( auto &c : extension )
                if( c >= 'A' && c <= 'Z' ) c += 'a' - 'A';
            if( extension != ".png" && extension != ".jpg" && extension != ".jpeg" )
            {
                error = "Choose a PNG or JPEG image file.";
                return false;
            }
            const auto size = std::filesystem::file_size( path );
            if( !size || size > mesh_generation::maximumSourceImageBytes )
            {
                error = "The source image must be nonempty and no larger than 20 MiB.";
                return false;
            }
            std::ifstream stream( path, std::ios::binary );
            std::vector<std::uint8_t> candidate( static_cast<std::size_t>( size ) );
            stream.read( reinterpret_cast<char *>( candidate.data() ), static_cast<std::streamsize>( size ) );
            if( !stream || stream.peek() != std::char_traits<char>::eof() )
            {
                error = "Could not read the complete image. Check permissions or choose it again.";
                return false;
            }
            const std::string mime = extension == ".png" ? "image/png" : "image/jpeg";
            if( !mesh_generation::validateSourceImage( candidate.data(), candidate.size(), mime, error ) )
                return false;
            bytes = std::move( candidate );
            mimeType = mime;
            return true;
        }
        catch( const std::exception & )
        {
            error = "Could not read the image. Check the file and available memory.";
            return false;
        }
    }

    inline bool saveGlb( const std::filesystem::path &projectRoot, const std::string &basename,
                         const std::uint8_t *bytes, std::size_t size,
                         std::filesystem::path &output, std::string &error )
    {
        output.clear();
        error.clear();
        if( !mesh_generation::validateGeneratedGlb( bytes, size, error ) ) return false;
        return generated_assets::saveUniqueFile( projectRoot, sanitizeBasename( basename ), ".glb",
                                                 bytes, size, output, error );
    }
}
