#pragma once

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <filesystem>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#    include <io.h>
#else
#    include <unistd.h>
#endif

namespace workphone::editor::generated_assets
{
    inline std::string sanitizeBasename( const std::string &name, const std::string &extension,
                                         const std::string &fallback )
    {
        auto source = name;
        if( source.size() >= extension.size() )
        {
            auto suffix = source.substr( source.size() - extension.size() );
            for( auto &c : suffix )
                if( c >= 'A' && c <= 'Z' ) c += 'a' - 'A';
            if( suffix == extension ) source.resize( source.size() - extension.size() );
        }
        std::string result;
        for( const unsigned char c : source )
        {
            if( result.size() == 80 ) break;
            const bool valid = ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
                               ( c >= '0' && c <= '9' ) || c == '-' || c == '_';
            const char next = valid ? static_cast<char>( c ) : '_';
            if( next != '_' || ( !result.empty() && result.back() != '_' ) ) result += next;
        }
        while( !result.empty() && ( result.back() == '_' || result.back() == '-' ) ) result.pop_back();
        if( result.empty() ) result = fallback;
        auto upper = result;
        for( auto &c : upper )
            if( c >= 'a' && c <= 'z' ) c -= 'a' - 'A';
        if( upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL" ||
            ( upper.size() == 4 && ( upper.substr( 0, 3 ) == "COM" || upper.substr( 0, 3 ) == "LPT" ) &&
              upper[3] >= '0' && upper[3] <= '9' ) ) result = "generated-" + result;
        return result;
    }

    // Format validation belongs to the caller. Both generation tools share the
    // same project confinement and exclusive-create publication policy.
    inline bool saveUniqueFile( const std::filesystem::path &projectRoot, const std::string &basename,
                                const std::string &extension, const unsigned char *bytes,
                                std::size_t size, std::filesystem::path &output, std::string &error )
    {
        namespace fs = std::filesystem;
        output.clear();
        error.clear();
        if( !bytes || !size || size > 128u * 1024u * 1024u ||
            ( extension != ".png" && extension != ".glb" ) )
        {
            error = "The generated asset is empty, too large or has an unsupported format.";
            return false;
        }
        try
        {
            if( projectRoot.empty() || !projectRoot.is_absolute() || !fs::is_directory( projectRoot ) )
            {
                error = "Open a saved project before generating an asset.";
                return false;
            }
            const auto root = fs::canonical( projectRoot );
            auto folder = fs::weakly_canonical( root / "Assets" / "Generated" );
            const auto contained = [&root]( const fs::path &path ) {
                auto part = path.begin();
                for( const auto &component : root )
                {
                    if( part == path.end() || *part != component ) return false;
                    ++part;
                }
                return part != path.end();
            };
            if( !contained( folder ) )
            {
                error = "Assets/Generated must be inside the active project.";
                return false;
            }
            fs::create_directories( folder );
            folder = fs::canonical( folder );
            if( !contained( folder ) )
            {
                error = "Assets/Generated must be inside the active project.";
                return false;
            }
            const auto stem = sanitizeBasename( basename, extension, "generated-asset" );
            for( unsigned index = 1; index <= 10000; ++index )
            {
                const auto suffix = index == 1 ? std::string() : "-" + std::to_string( index );
                const auto candidate = folder / fs::u8path( stem + suffix + extension );
#ifdef _WIN32
                const int file = _wopen( candidate.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                                         _S_IREAD | _S_IWRITE );
#else
                const int file = ::open( candidate.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644 );
#endif
                if( file < 0 )
                {
                    if( errno == EEXIST ) continue;
                    error = "Could not create the generated asset. Check project folder permissions.";
                    return false;
                }
                std::size_t written = 0;
                while( written < size )
                {
#ifdef _WIN32
                    const auto count = _write( file, bytes + written,
                                               static_cast<unsigned>( size - written ) );
#else
                    const auto count = ::write( file, bytes + written, size - written );
#endif
                    if( count <= 0 ) break;
                    written += static_cast<std::size_t>( count );
                }
#ifdef _WIN32
                const bool closed = _close( file ) == 0;
#else
                const bool closed = ::close( file ) == 0;
#endif
                if( written != size || !closed )
                {
                    std::error_code ignored;
                    fs::remove( candidate, ignored );
                    error = "Could not finish saving the generated asset. Check free disk space.";
                    return false;
                }
                output = candidate;
                return true;
            }
            error = "No unused asset filename is available. Choose another name.";
        }
        catch( const fs::filesystem_error & )
        {
            error = "Could not save the generated asset. Check the project folder and permissions.";
        }
        return false;
    }
}
