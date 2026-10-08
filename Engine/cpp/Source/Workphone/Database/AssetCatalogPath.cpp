#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace workphone
{
    namespace
    {
        constexpr size_t catalogPathLimit = 1024;

        bool fail( String &error, const char *message )
        {
            error = message;
            return false;
        }

#ifdef _WIN32
        bool asciiLetter( char value )
        {
            return ( value >= 'A' && value <= 'Z' ) || ( value >= 'a' && value <= 'z' );
        }
#endif

        bool validUtf8( const std::string &text )
        {
            for( size_t i = 0; i < text.size(); )
            {
                const auto first = static_cast<unsigned char>( text[i++] );
                if( first < 0x80 )
                {
                    continue;
                }
                const size_t count = first >= 0xc2 && first <= 0xdf   ? 1
                                     : first >= 0xe0 && first <= 0xef ? 2
                                     : first >= 0xf0 && first <= 0xf4 ? 3
                                                                      : 0;
                if( count == 0 || text.size() - i < count )
                {
                    return false;
                }
                unsigned value = first & ( 0x7f >> count );
                for( size_t n = 0; n < count; ++n )
                {
                    const auto next = static_cast<unsigned char>( text[i++] );
                    if( ( next & 0xc0 ) != 0x80 )
                    {
                        return false;
                    }
                    value = ( value << 6 ) | ( next & 0x3f );
                }
                const unsigned minimum = count == 1 ? 0x80 : count == 2 ? 0x800 : 0x10000;
                if( value < minimum || value > 0x10ffff || ( value >= 0xd800 && value <= 0xdfff ) )
                {
                    return false;
                }
            }
            return true;
        }

        std::string identityKey( std::string value )
        {
#ifdef _WIN32
            for( auto &character : value )
            {
                if( character >= 'A' && character <= 'Z' )
                {
                    character += 'a' - 'A';
                }
            }
#endif
            return value;
        }

        bool pathText( const String &input, std::string &text, String &error )
        {
            if( input.empty() || input.size() > catalogPathLimit || input.find( '\0' ) != String::npos )
            {
                return fail( error,
                             "Catalog paths must contain 1 to 1024 bytes without NUL characters" );
            }
            text.assign( input.data(), input.size() );
            if( !validUtf8( text ) )
            {
                return fail( error, "Catalog paths must be valid UTF-8" );
            }
            std::replace( text.begin(), text.end(), '\\', '/' );
            if( text.compare( 0, 4, "//?/" ) == 0 || text.compare( 0, 4, "//./" ) == 0 ||
                text.compare( 0, 4, "/??/" ) == 0 || text.compare( 0, 5, "//??/" ) == 0 )
            {
                return fail( error, "Windows device namespace paths are not catalog paths" );
            }
            for( size_t i = 0; i < text.size(); ++i )
            {
                if( text[i] == ':' )
                {
#ifdef _WIN32
                    if( i == 1 && asciiLetter( text[0] ) && text.size() > 2 && text[2] == '/' )
                    {
                        continue;
                    }
#endif
                    return fail( error,
                                 "Drive-relative paths and alternate data streams are unsupported" );
                }
            }
#ifdef _WIN32
            if( text[0] == '/' && ( text.size() == 1 || text[1] != '/' ) )
            {
                return fail( error, "Windows root-relative paths require an explicit drive" );
            }
            size_t start = text.size() > 2 && text[1] == ':' ? 3 : 0;
            if( text.compare( 0, 2, "//" ) == 0 )
            {
                const auto serverEnd = text.find( '/', 2 );
                if( serverEnd == std::string::npos || serverEnd == 2 || serverEnd + 1 == text.size() ||
                    text[serverEnd + 1] == '/' )
                {
                    return fail( error, "UNC catalog paths require a server and share" );
                }
                start = 2;
            }
            while( start < text.size() )
            {
                const auto end = text.find( '/', start );
                const auto component = text.substr( start, end - start );
                if( !component.empty() && component != "." && component != ".." )
                {
                    if( component.back() == '.' || component.back() == ' ' ||
                        component.find_first_of( "<>\"|?*" ) != std::string::npos ||
                        std::any_of( component.begin(), component.end(),
                                     []( unsigned char c ) { return c < 32; } ) )
                    {
                        return fail( error, "Ambiguous or invalid Windows path component" );
                    }
                    const auto basename = identityKey( component.substr( 0, component.find( '.' ) ) );
                    if( basename == "con" || basename == "prn" || basename == "aux" ||
                        basename == "nul" || basename == "conin$" || basename == "conout$" ||
                        ( basename.size() == 4 &&
                          ( basename.compare( 0, 3, "com" ) == 0 ||
                            basename.compare( 0, 3, "lpt" ) == 0 ) &&
                          basename[3] >= '1' && basename[3] <= '9' ) ||
                        ( basename.size() == 5 &&
                          ( basename.compare( 0, 3, "com" ) == 0 ||
                            basename.compare( 0, 3, "lpt" ) == 0 ) &&
                          ( basename.substr( 3 ) == u8"\u00b9" || basename.substr( 3 ) == u8"\u00b2" ||
                            basename.substr( 3 ) == u8"\u00b3" ) ) )
                    {
                        return fail( error, "Windows device names are not catalog paths" );
                    }
                }
                if( end == std::string::npos )
                {
                    break;
                }
                start = end + 1;
            }
#endif
            return true;
        }

        bool relativeUnderRoot( const std::filesystem::path &root, const std::filesystem::path &path,
                                std::string &relative );

        bool resolvableLinks( const std::filesystem::path &path, String &error,
                              const std::filesystem::path *root = nullptr )
        {
            std::filesystem::path prefix;
            for( const auto &part : path )
            {
                prefix /= part;
                if( !prefix.is_absolute() )
                {
                    continue;
                }
#ifdef _WIN32
                // A UNC server name is absolute to std::filesystem, but it is not an
                // inspectable filesystem object until the share component is present.
                if( prefix.root_name().generic_u8string().compare( 0, 2, "//" ) == 0 &&
                    prefix.relative_path().empty() )
                {
                    continue;
                }
#endif
                std::error_code ec;
                const auto status = std::filesystem::symlink_status( prefix, ec );
                if( ec && ec != std::errc::no_such_file_or_directory )
                {
                    return fail( error, "Cannot inspect catalog path prefix" );
                }
                if( std::filesystem::is_symlink( status ) )
                {
                    const auto target = std::filesystem::status( prefix, ec );
                    if( ec || !std::filesystem::exists( target ) )
                    {
                        return fail( error, "Unresolved symbolic link in catalog path" );
                    }
                    if( root )
                    {
                        const auto resolved = std::filesystem::canonical( prefix, ec );
                        std::string relative;
                        if( ec || !relativeUnderRoot( *root, resolved, relative ) )
                        {
                            return fail( error,
                                         "Symbolic link resolves outside the catalog project root" );
                        }
                    }
                }
            }
            return true;
        }

        std::vector<std::string> components( const std::filesystem::path &path )
        {
            std::vector<std::string> result;
            for( const auto &part : path )
            {
                const auto text = part.generic_u8string();
                if( !text.empty() && text != "." )
                {
                    result.push_back( text );
                }
            }
            return result;
        }

        bool relativeUnderRoot( const std::filesystem::path &root, const std::filesystem::path &path,
                                std::string &relative )
        {
            const auto rootParts = components( root );
            const auto pathParts = components( path );
            if( pathParts.size() < rootParts.size() )
            {
                return false;
            }
            for( size_t i = 0; i < rootParts.size(); ++i )
            {
                if( identityKey( rootParts[i] ) != identityKey( pathParts[i] ) )
                {
                    return false;
                }
            }
            relative.clear();
            for( size_t i = rootParts.size(); i < pathParts.size(); ++i )
            {
                if( !relative.empty() )
                {
                    relative += '/';
                }
                relative += pathParts[i];
            }
            return true;
        }

        bool resolveAbsoluteRootSpelling( const std::filesystem::path &root,
                                          const std::filesystem::path &source,
                                          std::filesystem::path &candidate, String &error )
        {
            std::string relative;
            if( relativeUnderRoot( root, source, relative ) )
            {
                candidate = source;
                return true;
            }
            // GetTempPath can return an existing 8.3 ancestor (for example ADMINI~1),
            // while the captured root uses its canonical long spelling. Resolve only
            // the prefix identifying that root. Keep the suffix untouched so '..' and
            // symlink escape-and-return spellings still undergo the normal audit.
            std::filesystem::path prefix;
            for( auto part = source.begin(); part != source.end(); ++part )
            {
                prefix /= *part;
                if( !prefix.is_absolute() )
                {
                    continue;
                }
#ifdef _WIN32
                if( prefix.root_name().generic_u8string().compare( 0, 2, "//" ) == 0 &&
                    prefix.relative_path().empty() )
                {
                    continue;
                }
#endif
                std::error_code ec;
                const auto resolved = std::filesystem::weakly_canonical( prefix, ec );
                if( ec )
                {
                    return fail( error, "Cannot resolve absolute catalog path prefix" );
                }
                if( relativeUnderRoot( root, resolved, relative ) && relative.empty() )
                {
                    candidate = root;
                    for( ++part; part != source.end(); ++part )
                    {
                        candidate /= *part;
                    }
                    return true;
                }
            }
            return fail( error, "Catalog path is outside its project root" );
        }
    }  // namespace

    bool canonicalAssetCatalogRoot( const String &input, String &output, String &error )
    {
        output.clear();
        error.clear();
        std::string text;
        if( !pathText( input, text, error ) )
        {
            return false;
        }
        try
        {
            std::error_code ec;
            auto root = std::filesystem::absolute( std::filesystem::u8path( text ), ec );
            if( ec )
            {
                return fail( error, "Cannot make catalog project root absolute" );
            }
            if( !resolvableLinks( root, error ) )
            {
                return false;
            }
            root = std::filesystem::weakly_canonical( root, ec );
            if( ec || !root.is_absolute() )
            {
                return fail( error, "Cannot resolve catalog project root" );
            }
            const auto result = root.generic_u8string();
            if( result.empty() || result.size() > catalogPathLimit )
            {
                return fail( error, "Resolved catalog project root exceeds path limits" );
            }
            if( std::filesystem::exists( root, ec ) && !std::filesystem::is_directory( root, ec ) )
            {
                return fail( error, "Catalog project root must be a directory" );
            }
            if( ec )
            {
                return fail( error, "Cannot inspect catalog project root" );
            }
            output = String( result.c_str() );
            return true;
        }
        catch( const std::exception & )
        {
            return fail( error, "Invalid or inaccessible catalog project root" );
        }
    }

    bool canonicalAssetCatalogPath( const String &projectRoot, const String &input,
                                    AssetCatalogPath &output, String &error )
    {
        output = {};
        error.clear();
        std::string rootText;
        std::string text;
        if( !pathText( projectRoot, rootText, error ) || !pathText( input, text, error ) )
        {
            return false;
        }
        try
        {
            const auto root = std::filesystem::u8path( rootText ).lexically_normal();
            if( !root.is_absolute() )
            {
                return fail( error, "Catalog project root must first be captured as an absolute path" );
            }
            const auto source = std::filesystem::u8path( text );
            if( !source.is_absolute() )
            {
                size_t depth = 0;
                for( const auto &part : source )
                {
                    if( part == ".." )
                    {
                        if( depth == 0 )
                        {
                            return fail( error, "Catalog path escapes above its project root" );
                        }
                        --depth;
                    }
                    else if( !part.empty() && part != "." )
                    {
                        ++depth;
                    }
                }
            }
            auto candidate = root / source;
            if( source.is_absolute() && !resolveAbsoluteRootSpelling( root, source, candidate, error ) )
            {
                return false;
            }
            std::string relative;
            if( !relativeUnderRoot( root, candidate.lexically_normal(), relative ) )
            {
                return fail( error, "Catalog path is outside its project root" );
            }
            // Absolute spellings cannot walk above the root and then return inside it either.
            if( source.is_absolute() && relativeUnderRoot( root, candidate, relative ) )
            {
                size_t depth = 0;
                for( const auto &part : std::filesystem::u8path( relative ) )
                {
                    if( part == ".." )
                    {
                        if( depth == 0 )
                        {
                            return fail( error, "Catalog path escapes above its project root" );
                        }
                        --depth;
                    }
                    else if( !part.empty() && part != "." )
                    {
                        ++depth;
                    }
                }
            }
            if( !resolvableLinks( candidate, error, &root ) )
            {
                return false;
            }
            std::error_code ec;
            const auto resolved = std::filesystem::weakly_canonical( candidate, ec );
            if( ec )
            {
                return fail( error, "Cannot resolve catalog path" );
            }
            if( !relativeUnderRoot( root, resolved, relative ) )
            {
                return fail( error, "Catalog path resolves outside its project root" );
            }
            if( relative.empty() || relative.size() > catalogPathLimit )
            {
                return fail( error, "Catalog path must name a resource below the project root" );
            }
            output.path = String( relative.c_str() );
            output.key = String( identityKey( relative ).c_str() );
            return true;
        }
        catch( const std::exception & )
        {
            output = {};
            return fail( error, "Invalid or inaccessible catalog path" );
        }
    }
}  // namespace workphone
