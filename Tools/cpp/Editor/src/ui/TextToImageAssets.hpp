#pragma once

#include <ui/GeneratedAssetFile.hpp>
#include <iterator>

namespace workphone::editor::image_assets
{
    inline std::string sanitizeBasename( const std::string &name )
    {
        return generated_assets::sanitizeBasename( name, ".png", "generated-image" );
    }

    inline bool savePng( const std::filesystem::path &projectRoot, const std::string &basename,
                         const unsigned char *bytes, std::size_t size,
                         std::filesystem::path &output, std::string &error )
    {
        output.clear();
        error.clear();
        static constexpr unsigned char signature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
        if( !bytes || size < sizeof( signature ) || size > 64u * 1024u * 1024u ||
            !std::equal( std::begin( signature ), std::end( signature ), bytes ) )
        {
            error = "The generated image is not a supported PNG.";
            return false;
        }
        return generated_assets::saveUniqueFile( projectRoot, sanitizeBasename( basename ), ".png",
                                                 bytes, size, output, error );
    }
}
